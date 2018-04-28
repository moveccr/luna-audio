/* vi: set ts=4: */
/* TODO: LICENSE */

#pragma once

#include "lunaplay.h"

extern void conv_pass(BUFFER *dst, BUFFER *src);

extern void conv_u8_pam2(BUFFER *dst, BUFFER *src);
extern void conv_pam2_u8(BUFFER *dst, BUFFER *src);
extern void conv_u8_pam3(BUFFER *dst, BUFFER *src);
extern void conv_pam3_u8(BUFFER *dst, BUFFER *src);
extern void conv_u8_pcm1(BUFFER *dst, BUFFER *src);
extern void conv_pcm1_u8(BUFFER *dst, BUFFER *src);
extern void conv_u8_pcm2(BUFFER *dst, BUFFER *src);
extern void conv_pcm2_u8(BUFFER *dst, BUFFER *src);
extern void conv_u8_pcm3(BUFFER *dst, BUFFER *src);
extern void conv_pcm3_u8(BUFFER *dst, BUFFER *src);

/* obsolete */
extern void conv_s16BE_pam2(BUFFER *dst, BUFFER *src);
extern void conv_s16LE_pam2(BUFFER *dst, BUFFER *src);
extern void conv_s16BE_pam3(BUFFER *dst, BUFFER *src);
extern void conv_s16LE_pam3(BUFFER *dst, BUFFER *src);
extern void conv_s16BE_pcm1(BUFFER *dst, BUFFER *src);
extern void conv_s16LE_pcm1(BUFFER *dst, BUFFER *src);
extern void conv_s16BE_pcm2(BUFFER *dst, BUFFER *src);
extern void conv_s16LE_pcm2(BUFFER *dst, BUFFER *src);
extern void conv_s16BE_pcm3(BUFFER *dst, BUFFER *src);
extern void conv_s16LE_pcm3(BUFFER *dst, BUFFER *src);

