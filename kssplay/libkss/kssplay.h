#ifndef _KSSPLAY_H_
#define _KSSPLAY_H_

#include "filter.h"
#include "rc_filter.h"
#include "dc_filter.h"
#include "kssobj.h"
#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KSSPLAY_VOL_BITS 9
#define KSSPLAY_VOL_RANGE (48.0*2)
#define KSSPLAY_VOL_STEP (KSSPLAY_VOL_RANGE/(1<<KSSPLAY_VOL_BITS))
#define KSSPLAY_VOL_MAX ((1<<(KSSPLAY_VOL_BITS-1))-1)
#define KSSPLAY_VOL_MIN (-(1<<(KSSPLAY_VOL_BITS-1)))
#define KSSPLAY_MUTE (1<<KSSPLAY_VOL_BITS)
#define KSSPLAY_VOL_MASK ((1<<KSSPLAY_VOL_BITS)-1)

typedef struct tagKSSPLAY_CH_STATUS
{
  k_uint8 name[8];
  k_uint8 note;   /* note number or rest if zero. */
  k_uint8 volume; /* volume */
  k_uint8 voice;  /* voice number */
  k_uint16 fnum;  /* direct value of frequency register */
  double freq;    /* frequency */
  k_int16 output; /* real output value */
} KSSPLAY_CH_STATUS ;

typedef struct tagKSSPLAY_STATUS
{
  KSSPLAY_CH_STATUS ch[3+5+9+9];
} KSSPLAY_STATUS;

typedef struct tagKSSPLAY
{
  KSS *kss ;

  k_uint8 *main_data ;
  k_uint8 *bank_data ;

  VM *vm ;

  k_uint32 rate ;
  k_uint32 nch ;
  k_uint32 bps ;

  k_uint32 step ;
  k_uint32 step_rest ;
  k_uint32 step_left ;

  k_int32 master_volume;
  k_int32 device_volume[EDSC_MAX];
  k_uint32 device_mute[EDSC_MAX];
  k_int32 device_pan[EDSC_MAX];
  FIR *device_fir[2][EDSC_MAX];
  RCF *rcf[2];
  DCF *dcf[2];

  k_uint32 fade ;
  k_uint32 fade_step ;
  k_uint32 fade_flag ;

  k_uint32 cpu_speed ; /* 0: AutoDetect, 1...N: x1 ... xN */
  k_uint32 vsync_freq ;

  int scc_disable;
  int opll_stereo;

} KSSPLAY ;

enum { KSSPLAY_FADE_NONE=0, KSSPLAY_FADE_OUT=1, KSSPLAY_FADE_END=2 } ;

/* Create KSSPLAY object */
KSSPLAY *KSSPLAY_new(k_uint32 rate, k_uint32 nch, k_uint32 bps) ;

/* Set KSS data to KSSPLAY object */
int KSSPLAY_set_data(KSSPLAY *, KSS *kss) ;

/* Reset KSSPLAY object */
void KSSPLAY_reset(KSSPLAY *, k_uint32 song, k_uint32 cpu_speed) ;

/* Emulate some seconds and Generate Wave data */
void KSSPLAY_calc(KSSPLAY *, k_int16 *buf, k_uint32 length) ;

/* Emulate some seconds */
void KSSPLAY_calc_silent(KSSPLAY *, k_uint32 length) ;

/* Delete KSSPLAY object */
void KSSPLAY_delete(KSSPLAY *) ;

/* Start fadeout */
void KSSPLAY_fade_start(KSSPLAY *kssplay, k_uint32 fade_time) ;

/* Stop fadeout */
void KSSPLAY_fade_stop(KSSPLAY *kssplay) ;


int KSSPLAY_read_memory(KSSPLAY *kssplay, k_uint32 address) ;

int KSSPLAY_get_loop_count(KSSPLAY *kssplay) ;
int KSSPLAY_get_stop_flag(KSSPLAY *kssplay) ;
int KSSPLAY_get_fade_flag(KSSPLAY *kssplay) ;

void KSSPLAY_set_opll_patch(KSSPLAY *kssplay, k_uint8 *illdata) ;
void KSSPLAY_set_speed(KSSPLAY *kssplay, k_uint32 cpu_speed) ;
void KSSPLAY_set_device_lpf(KSSPLAY *kssplay, k_uint32 device, k_uint32 cutoff) ;
void KSSPLAY_set_device_mute(KSSPLAY *kssplay, k_uint32 devnum, k_uint32 mute);
void KSSPLAY_set_device_pan(KSSPLAY *kssplay, k_uint32 devnum, k_int32 pan);
void KSSPLAY_set_channel_pan(KSSPLAY *kssplay, k_uint32 device, k_uint32 ch, k_uint32 pan);
void KSSPLAY_set_device_quality(KSSPLAY *kssplay, k_uint32 device, k_uint32 quality);
void KSSPLAY_set_device_volume(KSSPLAY *kssplay, k_uint32 devnum, k_int32 vol);
void KSSPLAY_set_master_volume(KSSPLAY *kssplay, k_int32 vol);
void KSSPLAY_set_device_mute(KSSPLAY *kssplay, k_uint32 devnum, k_uint32 mute);
void KSSPLAY_set_channel_mask(KSSPLAY *kssplay, k_uint32 device, k_uint32 mask);

k_uint32 KSSPLAY_get_device_volume(KSSPLAY *kssplay, k_uint32 devnum);
void KSSPLAY_get_MGStext(KSSPLAY *kssplay, char *buf, int max);

void KSSPLAY_get_status(KSSPLAY *kssplay, KSSPLAY_STATUS *kstat);

#ifdef __cplusplus
}
#endif
#endif

