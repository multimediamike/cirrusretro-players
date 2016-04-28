#ifndef _VM_H_
#define _VM_H_

#include "../device/emu2149.h"
#include "../device/emu2212.h"
#include "../device/emu2413.h"
#include "../device/emu8950.h"
#include "../device/emu76489.h"
#include "ksstypes.h"
#include "../kmz80/kmz80.h"
#include "mmap.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EXTIO (0x40)
#define STOPIO (0x41)
#define LOOPIO (0x42)
#define ADRLIO (0x43)
#define ADRHIO (0x44)

#define VM_INVALID_ADDRESS (0x10000)

enum { KSS_MAIN_SLOT=0, KSS_BANK_SLOT=1} ;

typedef struct tagVM VM;
typedef int (*VM_WIOPROC)(VM *,k_uint32,k_uint32);

struct tagVM
{
  KMZ80_CONTEXT context ;
  KMEVENT kme ;
  KMEVENT_ITEM_ID vsync_id ;
  k_uint32 vsync_cycles ;
  k_uint32 vsync_cycles_step ;
  k_uint32 vsync_cycles_left ;
  k_uint32 vsync_freq ;
  k_uint32 vsync_adr ;
  MMAP *mmap ;

  k_uint8 IO[0x100] ;
  VM_WIOPROC WIOPROC[0x100];

  k_uint32 clock ; /* CPU clock */

  k_uint32 bank_mode ;
  k_uint32 bank_min ;
  k_uint32 bank_max ;
  k_uint32 ram_mode ;
  k_uint32 scc_disable ;
  k_uint32 DA8_enable ;

  k_int32 DA1 ; /* 1bit D/A (I/O mapped 0xAA) */
  k_int32 DA8 ; /* 8bit D/A (memory mapped 0x5000 - 0x5FFF) */
  

  PSG *psg ;
  SCC *scc ;
  OPLL *opll ;
  OPL *opl ;
  SNG *sng ;

  void *fp ;

};



#define MSX_CLK (3579545)

VM *VM_new() ;
void VM_delete(VM *vm) ;
void VM_reset(VM *vm, k_uint32 cpu_clk, k_uint32 pc, k_uint32 play_adr, k_uint32 vsync_freq, k_uint32 song, k_uint32 DA8) ;
void VM_init_memory(VM *vm, k_uint32 ram_mode, k_uint32 offset, k_uint32 num, k_uint8 *data) ;
void VM_init_bank(VM *vm, k_uint32 mode, k_uint32 num, k_uint32 offset, k_uint8 *data) ;
void VM_exec(VM *vm, k_uint32 cycles) ;
void VM_exec_func(VM *vm, k_uint32 init_adr) ;
void VM_set_clock(VM *vm, k_uint32 clock, k_uint32 vsync_freq) ;
void VM_set_wioproc(VM *vm, k_uint32 a, VM_WIOPROC p);

#ifdef __cplusplus
}
#endif

#endif
