#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <malloc.h>
#include "kssobj.h"

KSS *KSS_new(k_uint8 *data, k_uint32 size)
{
   KSS *kss ;

   if((kss=malloc(sizeof(KSS)))==0) return NULL ;
   memset(kss, 0, sizeof(KSS));
   if((kss->data=malloc(size))==0)
   {
     free(kss) ;
     return NULL ;
   }

   memcpy(kss->data, data, size) ;
   kss->size = size ;
   kss->type = 0 ;
   kss->title[0] = '\0' ;
   kss->idstr[0] = '\0' ;
   kss->extra = NULL ;
   kss->loop_detectable = 0 ;
   kss->stop_detectable = 0 ;

   return kss ;
}

void KSS_delete(KSS *kss)
{
  if(kss)
  {
    free(kss->data) ;
    free(kss->extra) ;
    free(kss) ;
  }
}

int KSS_check_type(k_uint8 *data, k_uint32 size, const char *filename)
{
  char * p;

  if(size<0x4)
    return KSSDATA ;

  if(KSS_isMGSdata(data,size)) return MGSDATA ;
  else if(KSS_isMPK106data(data,size)) return MPK106DATA ;
  else if(KSS_isMPK103data(data,size)) return MPK103DATA ;
  else if(!strncmp("KSCC",(char *)data,4)) return KSSDATA ;
  else if(!strncmp("KSSX",(char *)data,4)) return KSSDATA ;
  else if(KSS_isOPXdata(data,size)) return OPXDATA ;
  else if(KSS_isBGMdata(data,size)) return BGMDATA ;
  else if(filename)
  {
    p=strrchr(filename, '.');
    if(p&&(strcmp(p,".MBM")==0||strcmp(p,".mbm")==0))
      return MBMDATA;
  }

  return KSS_TYPE_UNKNOWN;
}

void KSS_make_header(k_uint8 *header, k_uint16 load_adr, k_uint16 load_size, k_uint16 init_adr, k_uint16 play_adr)
{
  header[0x00] = 'K' ;
  header[0x01] = 'S' ;
  header[0x02] = 'S' ;
  header[0x03] = 'X' ;
  header[0x04] = (k_uint8)(load_adr & 0xff) ;
  header[0x05] = (k_uint8)(load_adr >> 8) ;
  header[0x06] = (k_uint8)(load_size & 0xff) ;
  header[0x07] = (k_uint8)(load_size >> 8) ;
  header[0x08] = (k_uint8)(init_adr & 0xff) ;
  header[0x09] = (k_uint8)(init_adr >> 8);
  header[0x0A] = (k_uint8)(play_adr & 0xff);
  header[0x0B] = (k_uint8)(play_adr >> 8);
  
  header[0x0C] = 0x00 ;
  header[0x0D] = 0x00 ;
  header[0x0E] = 0x10 ;
  header[0x0F] = 0x05 ;

  header[0x1C] = 0x00;
  header[0x1D] = 0x00;
  header[0x1E] = 0x00;
  header[0x1F] = 0x00;
}

static void get_legacy_header(KSS *kss)
{
    kss->load_adr = (kss->data[0x5]<<8) + kss->data[0x4] ;
    kss->load_len = (kss->data[0x7]<<8) + kss->data[0x6] ;
    kss->init_adr = (kss->data[0x9]<<8) + kss->data[0x8] ;
    kss->play_adr = (kss->data[0xB]<<8) + kss->data[0xA] ;
    kss->bank_offset = kss->data[0xC] ;
    kss->bank_num = kss->data[0xD]&0x7F ;
    kss->bank_mode = (kss->data[0xD]>>7)?KSS_8K:KSS_16K ;
    kss->extra_size = kss->data[0xE] ;
    kss->device_flag = kss->data[0x0F] ;
}

static void check_device(KSS *kss, k_uint32 flag)
{
  kss->sn76489 = flag & 2 ;
  if(flag&2)
  {
    kss->fmunit = kss->fmpac = flag & 1 ;
    kss->stereo = 0; /* (flag&4)>>2 ;*/
    kss->ram_mode = (flag&8)>>3 ;
    kss->pal_mode = (flag&64)>>6;
    kss->mode = KSS_SEGA ;
  }
  else
  {
    if((flag&0x18)==0x10) kss->DA8_enable = 1 ;
    else kss->DA8_enable = 0 ;

    kss->fmpac = kss->fmunit = flag & 1 ;
    kss->ram_mode = (flag&4)>>2 ;
    kss->msx_audio = (flag&8)>>3 ;
    if(kss->msx_audio)
      kss->stereo = (flag&16)>>4 ;
    else
      kss->stereo = 0;
    kss->pal_mode = (flag&64)>>6;
    kss->mode = KSS_MSX ;
  }
}

static void scan_info(KSS *kss)
{
  int i;

  if(strncmp("KSCC", (char *)kss->data, 4)==0)
  {
    kss->kssx = 0 ;
    get_legacy_header(kss) ;
    check_device(kss,kss->device_flag) ;
    kss->trk_min = 0;
    kss->trk_max = 255;
    for(i=0;i<EDSC_MAX;i++)
      kss->vol[i] = 0x80;
  }
  else if(strncmp("KSSX", (char *)kss->data, 4)==0)
  {
    kss->kssx = 1 ;
    get_legacy_header(kss) ;
    check_device(kss,kss->device_flag) ;
    kss->trk_min = (kss->data[0x19]<<8) + kss->data[0x18] ;
    kss->trk_max = (kss->data[0x1B]<<8) + kss->data[0x1A] ;
    for(i=0;i<EDSC_MAX;i++)
      kss->vol[i] = kss->data[0x1C+i];
  }
}

static int is_sjis_prefix(int c)
{
  if((0x81<=c&&c<=0x9F)||(0xE0<=c&&c<=0xFC)) return 1 ;
  else return 0 ;
}

static void msx_kanji_fix(unsigned char *title)
{
  unsigned char *p = title;

  while(p[0])
  {
    if(p[0]==0x81&&0xAF<=p[1]&&p[1]<=0xB8)
    {
      p[0] = 0x87;
      p[1] = p[1] - 0xAF + 0x54;
      p+=2;
    }
    else if(is_sjis_prefix(p[0])) p+=2;
    else p+=1;
  }

  return;
}

KSS *KSS_bin2kss(k_uint8 *data, k_uint32 data_size, const char *filename)
{
  KSS *kss;
  int type;

  if(data==NULL) return NULL;

  type = KSS_check_type(data,data_size, filename);

  switch(type)
  {
  case MBMDATA:
    kss = KSS_mbm2kss(data,data_size);
    if(kss) KSS_get_info_mbmdata(kss, data, data_size);
    break;

  case MGSDATA:
    kss = KSS_mgs2kss(data,data_size);
    if(kss) KSS_get_info_mgsdata(kss, data, data_size);
    break;

  case BGMDATA:
    kss = KSS_bgm2kss(data,data_size) ;
    if(kss) KSS_get_info_bgmdata(kss, data, data_size);
    break;

  case MPK103DATA:
    kss = KSS_mpk1032kss(data,data_size) ;
    if(kss) KSS_get_info_mpkdata(kss, data, data_size);
    break;

  case MPK106DATA:
    kss = KSS_mpk1062kss(data,data_size) ;
    if(kss) KSS_get_info_mpkdata(kss, data, data_size);
    break;

  case OPXDATA:
    kss = KSS_opx2kss(data,data_size) ;
    if(kss) KSS_get_info_opxdata(kss, data, data_size);
    break;

  case KSSDATA:
    kss = KSS_kss2kss(data,data_size) ;
    if(kss) KSS_get_info_kssdata(kss, data, data_size);
    break;

  default:
    return NULL ;
  }

  if(kss == NULL) return NULL;
  kss->type = type;
  msx_kanji_fix(kss->title);
  scan_info(kss);
  return kss;
}




