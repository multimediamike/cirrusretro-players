/*
 *
 * vm.c - Virtual Machine using kmz80.c for KSSPLAY written by Mitsutaka Okazaki 2001.
 *
 * 2001-06-15 : Version 0.00
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <malloc.h>
#include <assert.h>
#
#include "mmap.h"
#include "vm.h"

/* Callback functions from KMZ80 */
static k_uint32 memread(VM *vm, k_uint32 a)
{
  return MMAP_read_memory(vm->mmap, a) ;
}

static void memwrite(VM *vm, k_uint32 a, k_uint32 d)
{
  k_uint32 page = a>>13 ;

  if((!vm->scc_disable)&&((0x9800<=a&&a<=0x988F)||(0xB800<=a&&a<=0xB8AF)))
  {
    SCC_write(vm->scc, a, d) ;
  }

  if((vm->DA8_enable)&&(0x5000<=a&&a<=0x5FFF))
  {
    vm->DA8 >>= 1 ;
    vm->DA8 += ((k_int32)(d&0xff) - 0x80) << 3 ;
  }
  
  if((vm->bank_mode==KSS_8K)&&((a==0x9000)||(a==0xB000))) 
  {
      MMAP_select_bank(vm->mmap, page ,KSS_BANK_SLOT,d) ;
  }

  MMAP_write_memory(vm->mmap, a, d);
  
}

static k_uint32 ioread(VM *vm, k_uint32 a)
{
  a&=0xff ;

  if((vm->psg)&&((a&0xff)==0xA2)) return PSG_readIO(vm->psg) ;
  else if(a==0xC1) return OPL_readIO(vm->opl) ;
  else if(a==0xC0) return OPL_status(vm->opl) ;

  return 0xff;
}

static void iowrite(VM *vm, k_uint32 a, k_uint32 d)
{
  a &= 0xff ;

  if(((a==STOPIO)||(a==LOOPIO)||(a==ADRLIO)||(a==ADRHIO))&&(vm->IO[EXTIO]!=0x7f)) return ;

  vm->IO[a] = (k_uint8)(d&0xff) ;

  if(vm->WIOPROC[a])
    vm->WIOPROC[a](vm, a,d);

  switch(a)
  {
  case 0xA0:
  case 0xA1:
    if(vm->psg) PSG_writeIO(vm->psg, a, d) ;
    break ;

  case 0xAA:
    vm->DA1 >>= 1 ;
    vm->DA1 += d&0x80 ? 256<<3 : 0 ;
    break ;

  case 0xAB:
    vm->DA1 >>= 1 ;
    vm->DA1 += d&0x01 ? 256<<3 : 0 ;
    break;

  case 0x7E:
  case 0x7F:
    if(vm->sng) SNG_writeIO(vm->sng, d);
    break ;

  case 0x7C:
  case 0x7D:
  case 0xF0:
  case 0xF1:
    if(vm->opll) OPLL_writeIO(vm->opll, a, d) ;
    break ;

  case 0xC0:
  case 0xC1:
    if(vm->opl) OPL_writeIO(vm->opl, a, d) ;
    break ;

  default:
    break ;
  }

  if((vm->bank_mode == KSS_16K)&&(a==0xfe))
  {
    if((vm->bank_min<=d)&&(d<vm->bank_max))
      MMAP_select_bank(vm->mmap, 4,KSS_BANK_SLOT,d) ;
    else
      MMAP_select_bank(vm->mmap, 4,KSS_MAIN_SLOT, 2) ; 
  }
}

static k_uint32 busread(VM *vm, k_uint32 mode)
{
  return 0x38 ;
}

static void exec_setup(VM *vm, k_uint32 pc)
{
  k_uint32 sp = 0xf380, rp ;


  MMAP_write_memory(vm->mmap,--sp, 0) ;
	MMAP_write_memory(vm->mmap,--sp,0xfe) ;
	MMAP_write_memory(vm->mmap,--sp,0x18) ;	/* JR +0 */
	MMAP_write_memory(vm->mmap,--sp,0x76) ;	/* HALT */
	rp = sp;
	MMAP_write_memory(vm->mmap,--sp,rp>>8) ;
	MMAP_write_memory(vm->mmap,--sp,rp&0xff) ;

  vm->context.sp = sp;
	vm->context.pc = pc;
	vm->context.regs8[REGID_HALTED] = 0;
}

/* Set the next event */
inline void adjust_vsync_cycles(VM *vm)
{
  vm->vsync_cycles_left+=vm->vsync_cycles_step;
  if(vm->vsync_cycles_left>=vm->vsync_freq)
  {
    vm->vsync_cycles_left-=vm->vsync_freq;
    kmevent_settimer(&vm->kme, vm->vsync_id, vm->vsync_cycles+1);
  }
  else
  {
    kmevent_settimer(&vm->kme, vm->vsync_id, vm->vsync_cycles);
  }
}
/* Handler for KMEVENT */
static void vsync(KMEVENT *event, KMEVENT_ITEM_ID curid, VM *vm)
{
  adjust_vsync_cycles(vm);
  if (vm->context.regs8[REGID_HALTED]) exec_setup(vm, vm->vsync_adr);
}

/* Interfaces for class VM  */
VM *VM_new(int rate)
{	
  VM *vm ;

  if(!(vm = malloc(sizeof(VM)))) return NULL ;

  memset(vm, 0, sizeof(VM)) ;

  vm->sng = SNG_new(MSX_CLK, rate) ;
  vm->psg = PSG_new(MSX_CLK, rate) ;
  vm->scc = SCC_new(MSX_CLK, rate) ;
  vm->opll = OPLL_new(MSX_CLK, rate) ;
  vm->opl = OPL_new(MSX_CLK, rate) ;

  vm->mmap = MMAP_new() ;
  vm->ram_mode = 0 ;
  vm->DA8_enable = 0 ;
  vm->bank_mode = BANK_16K ;
  assert(vm->mmap) ;

  //vm->fp = fopen("VMDBG","wb") ;

  return vm ;
}

/* Delete Object */
void VM_delete(VM *vm)
{
  //fclose(vm->fp) ;
  MMAP_delete(vm->mmap) ;
  SNG_delete(vm->sng) ;
  PSG_delete(vm->psg) ;
  SCC_delete(vm->scc) ;
  OPLL_delete(vm->opll) ;
  OPL_delete(vm->opl) ;
  kmevent_free(&vm->kme, vm->vsync_id) ;
  free(vm) ;
}

void VM_exec(VM *vm, k_uint32 cycles)
{
   kmz80_exec(&vm->context, cycles) ;
}

void VM_exec_func(VM *vm, k_uint32 func_adr)
{
  int i ;
  exec_setup(vm, func_adr);
  for(i=0;i<(int)vm->clock;i+=256)
  {
    kmz80_exec(&vm->context, 256) ;
    if(vm->context.regs8[REGID_HALTED]) break ;
  }
}

void VM_set_wioproc(VM *vm, k_uint32 a, VM_WIOPROC p)
{
  vm->WIOPROC[a] = p;
}

void VM_set_clock(VM *vm, k_uint32 clock, k_uint32 vsync_freq)
{
  vm->clock = clock ;
  vm->vsync_freq = vsync_freq ;
  vm->vsync_cycles = clock / vsync_freq ;
  vm->vsync_cycles_left = 0 ;
  vm->vsync_cycles_step = clock % vsync_freq ;
  kmevent_settimer(&vm->kme, vm->vsync_id, vm->vsync_cycles);
}

void VM_reset(VM *vm, k_uint32 clock, k_uint32 init_adr, k_uint32 vsync_adr, k_uint32 vsync_freq, k_uint32 song, k_uint32 DA8)
{
  /* Reset KMZ80 */
  kmz80_reset(&vm->context) ;

  /* Reset Devices */
  PSG_reset(vm->psg) ;
  SCC_reset(vm->scc) ;
  OPLL_reset(vm->opll) ;
  OPL_reset(vm->opl) ;
  SNG_reset(vm->sng) ;

  memset(vm->IO,0,sizeof(vm->IO));
  memset(vm->WIOPROC,0,sizeof(vm->IO));
  vm->DA1 = 0 ;
  vm->DA8_enable = DA8 ;

  vm->context.user = vm ;
	vm->context.memread = (void *)memread ;
	vm->context.memwrite = (void *)memwrite ;
	vm->context.ioread = (void *)ioread ;
	vm->context.iowrite = (void *)iowrite ;
	vm->context.busread = (void *)busread ;

	vm->context.regs8[REGID_M1CYCLE] = 2 ;
	vm->context.regs8[REGID_A] = (k_uint8)(song&0xff) ;
	vm->context.regs8[REGID_HALTED] = 0;
	vm->context.exflag = 3;
	vm->context.regs8[REGID_IFF1] = 0;
  vm->context.regs8[REGID_IFF2] = 0;
	vm->context.regs8[REGID_INTREQ] = 0;
	vm->context.regs8[REGID_IMODE] = 1;

  /* Execute init code : Wait until return the init (max 1 sec). */
  VM_exec_func(vm, init_adr);

  /* VSYNC SETTINGS */
  vm->vsync_adr = vsync_adr ;

  kmevent_init(&vm->kme) ;
  vm->vsync_id = kmevent_alloc(&vm->kme) ;
  kmevent_setevent(&vm->kme, vm->vsync_id, vsync, vm) ;
  vm->context.kmevent = &vm->kme ;
  VM_set_clock(vm, clock, vsync_freq) ;

}

void VM_init_memory(VM *vm, k_uint32 ram_mode, k_uint32 offset, k_uint32 size, k_uint8 *data)
{
  int i ;
  k_uint8 *main_memory ;
  
  assert(vm) ;
  assert(vm->mmap) ;

  vm->ram_mode = ram_mode ;

  if(!(main_memory = malloc(0x10000)))
  {
    assert(0) ;
    return ;
  }

  memset(main_memory,0xC9,0x10000);
	memcpy(main_memory+0x93, "\xC3\x01\x00\xC3\x09\x00", 6);
	memcpy(main_memory+0x01, "\xD3\xA0\xF5\x7B\xD3\xA1\xF1\xC9", 8);
	memcpy(main_memory+0x09, "\xD3\xA0\xDB\xA2\xC9", 5);
	memset(main_memory+0x4000, 0, 0xC000);
  if((offset + size) > 0x10000) size = 0x10000 - offset ;
  memcpy(main_memory + offset, data, size) ;

  for(i=0;i<4;i++)
  {
    MMAP_set_bank_data(vm->mmap, KSS_MAIN_SLOT, i, BANK_16K, main_memory + 0x4000 * i) ;
    MMAP_set_bank_attr(vm->mmap, KSS_MAIN_SLOT, i, BANK_READABLE|BANK_WRITEABLE) ;
    MMAP_select_bank(vm->mmap, i<<1, KSS_MAIN_SLOT, i) ;
  }

  if(!ram_mode) MMAP_set_bank_attr(vm->mmap, KSS_MAIN_SLOT, 2, BANK_READABLE) ;

  free(main_memory) ;
}

void VM_init_bank(VM *vm, k_uint32 mode, k_uint32 num, k_uint32 offset, k_uint8 *data)
{
  k_uint32 size, i ;

  assert(vm) ;
  assert(vm->mmap) ;

  vm->bank_mode = mode ;
  vm->bank_min = offset ;
  vm->bank_max = offset + num ;

  if(mode==KSS_16K)
  {
    size = 0x4000 ;
    for(i=0;i<num;i++)
      MMAP_set_bank_data(vm->mmap, KSS_BANK_SLOT, i + offset, BANK_16K, data + i * size) ;
  }
  else if(mode==KSS_8K)
  {
    size = 0x2000 ;
    for(i=0;i<num;i++)
      MMAP_set_bank_data(vm->mmap, KSS_BANK_SLOT, i + offset, BANK_8K, data + i * size) ;
  }
  else
  {
    assert(0) ;
  }

}



