/*
  KMZ80 common header
  by Mamiya
*/

#ifndef KMZ80_H_
#define KMZ80_H_

#include "kmtypes.h"
#include "kmevent.h"
#ifdef __cplusplus
extern "C" {
#endif

enum {
	REGID_B,
	REGID_C,
	REGID_D,
	REGID_E,
	REGID_H,
	REGID_L,
	REGID_F,
	REGID_A,
	REGID_IXL,
	REGID_IXH,
	REGID_IYL,
	REGID_IYH,
	REGID_R,
	REGID_R7,
	REGID_I,
	REGID_IFF1,
	REGID_IFF2,
	REGID_IMODE,
	REGID_NMIREQ,
	REGID_INTREQ,
	REGID_HALTED,
	REGID_M1CYCLE,
	REGID_MEMCYCLE,
	REGID_IOCYCLE,
	REGID_STATE,
	REGID_FDMG,
	REGID_MAX,
	REGID_REGS8SIZE = ((REGID_MAX + (sizeof(int) - 1)) & ~(sizeof(int) - 1))
};

typedef struct KMZ80_CONTEXT_TAG KMZ80_CONTEXT;
struct KMZ80_CONTEXT_TAG {
	Uint8 regs8[REGID_REGS8SIZE];
	Uint32 sp;
	Uint32 pc;
	/* �����W�X�^ */
	Uint32 saf;
	Uint32 sbc;
	Uint32 sde;
	Uint32 shl;
	/* �e���|�����t���O���W�X�^(�Öق̃L�����[�t���O) */
	Uint32 t_fl;
	/* �e���|�����f�[�^�[���W�X�^(�Öق�35�t���O) */
	Uint32 t_dx;
	/* �����܂ł͕ۑ�����ׂ� */
	/* �e���|�����v���O�����J�E���^ */
	Uint32 t_pc;
	/* �e���|�����I�y�����h���W�X�^ */
	Uint32 t_op;
	/* �e���|�����A�h���X���W�X�^ */
	Uint32 t_ad;
	/* �T�C�N���J�E���^ */
	Uint32 cycle;
	/* �I�y�R�[�h�e�[�u�� */
	void *opt;
	/* �I�y�R�[�hCB�e�[�u�� */
	void *optcb;
	/* �I�y�R�[�hED�e�[�u�� */
	void *opted;
	/* �ǉ��T�C�N���e�[�u�� */
	void *cyt;
	/* R800�������[�y�[�W(�y�[�W�u���C�N�̊m�F�p) */
	Uint32 mempage;
	/* ����p�r���荞�݃x�N�^ */
	Uint32 vector[5];
	/* RST��ѐ��{�A�h���X */
	Uint32 rstbase;
	/* �ǉ��t���O */
	/*   bit0: �Öق̃L�����[�L�� */
	/*   bit1: ���荞�ݗv�������N���A */
	Uint32 exflag;
	/* ������`�R�[���o�b�N */
	Uint32 (*sysmemfetch)(KMZ80_CONTEXT *context);
	Uint32 (*sysmemread)(KMZ80_CONTEXT *context, Uint32 a);
	void (*sysmemwrite)(KMZ80_CONTEXT *context, Uint32 a, Uint32 d);
	/* ���[�U�[�f�[�^�[�|�C���^ */
	void *user;
	/* ���[�U�[��`�R�[���o�b�N */
	Uint32 (*memread)(void *u, Uint32 a);
	void (*memwrite)(void *u, Uint32 a, Uint32 d);
	Uint32 (*ioread)(void *u, Uint32 a);
	void (*iowrite)(void *u, Uint32 a, Uint32 d);
	Uint32 (*busread)(void *u, Uint32 mode);
	Uint32 (*checkbreak)(void *u, KMZ80_CONTEXT *context);
	Uint32 (*patchedfe)(void *u, KMZ80_CONTEXT *context);
	/* ���[�U�[��`�C�x���g�^�C�} */
	KMEVENT *kmevent;
};

void kmz80_reset(KMZ80_CONTEXT *context);
void kmr800_reset(KMZ80_CONTEXT *context);
void kmdmg_reset(KMZ80_CONTEXT *context);
Uint32 kmz80_exec(KMZ80_CONTEXT *context, Uint32 cycle);
void kmz80_reset_common(KMZ80_CONTEXT *context);

#ifdef __cplusplus
}
#endif
#endif
