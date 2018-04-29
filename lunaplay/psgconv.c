/* vi: set ts=4: */
/* TODO: LICENSE */

#include <string.h>
#include <sys/endian.h>
#include "lunaplay.h"
#include "psgconv.h"

#include "pcm1.tbl"
#include "pcm2.tbl"
#include "pcm3.tbl"

void
conv_pass(BUFFER *dst, BUFFER *src)
{
	/* no memory copy */
	dst->length = src->length;
}

/* ----- PAM2 ----- */
/* PAM2 ではとりあえず PCM2 と同じテーブルを使ってみる。
やってみないとわからない。*/

void
conv_u8_pam2(BUFFER *dst, BUFFER *src)
{
	conv_u8_pcm2(dst, src);
}

void
conv_pam2_u8(BUFFER *dst, BUFFER *src)
{
	conv_pcm2_u8(dst, src);
}

/* ----- PAM3 ----- */
/* PAM3 ではとりあえず PCM3 と同じテーブルを使ってみる。
やってみないとわからない。*/

void
conv_u8_pam3(BUFFER *dst, BUFFER *src)
{
	conv_u8_pcm3(dst, src);
}

void
conv_pam3_u8(BUFFER *dst, BUFFER *src)
{
	conv_pcm3_u8(dst, src);
}

/* ----- PCM1 ----- */
void
conv_u8_pcm1(BUFFER *dst, BUFFER *src)
{
	int count = src->length;

	uint8_t *s = src->ptr;
	uint8_t *d = dst->ptr;

	for (int i = 0; i < count; i++) {
		*d++ = PCM1_TABLE[*s++];
	}
	dst->length = count;
}

void
conv_pcm1_u8(BUFFER *dst, BUFFER *src)
{
	int count = src->length;

	uint8_t *s = src->ptr;
	uint8_t *d = dst->ptr;

	for (int i = 0; i < count; i++) {
		double v;
		int a;
		a = (*s++) & 15;
		v = PSG_VT[a];
		v -= PCM1_TABLE_offset;
		v /= PCM1_TABLE_gain;
		v *= 255;
		if (v < 0) v = 0;
		if (v > 255) v = 255;
		*d++ = (uint8_t)v;
	}
	dst->length = count;
}

/* ----- PCM2 ----- */
void
conv_u8_pcm2(BUFFER *dst, BUFFER *src)
{
	int count = src->length;

	uint8_t *s = src->ptr;
	uint16_t *d = (uint16_t*)dst->ptr;

	for (int i = 0; i < count; i++) {
		*d++ = htobe16(PCM2_TABLE[*s++]);
	}
	dst->length = count * 2;
}

void
conv_pcm2_u8(BUFFER *dst, BUFFER *src)
{
	int count = src->length / 2;

	uint8_t *s = src->ptr;
	uint8_t *d = dst->ptr;

	for (int i = 0; i < count; i++) {
		double v;
		int a;
		a = (*s++) & 15;
		v = PSG_VT[a];
		a = (*s++) & 15;
		v += PSG_VT[a];
		v -= PCM2_TABLE_offset;
		v /= PCM2_TABLE_gain;
		v *= 255;
		if (v < 0) v = 0;
		if (v > 255) v = 255;
		*d++ = (uint8_t)v;
	}
	dst->length = count;
}

/* ----- PCM3 ----- */
void
conv_u8_pcm3(BUFFER *dst, BUFFER *src)
{
	int count = src->length;

	uint8_t *s = src->ptr;
	uint32_t *d = (uint32_t*)dst->ptr;

	for (int i = 0; i < count; i++) {
		*d++ = htobe32(PCM3_TABLE[*s++]);
	}
	dst->length = count * 4;
}

void
conv_pcm3_u8(BUFFER *dst, BUFFER *src)
{
	int count = src->length / 4;

	uint8_t *s = src->ptr;
	uint8_t *d = dst->ptr;

	for (int i = 0; i < count; i++) {
		double v;
		int a;
		s++;
		a = (*s++) & 15;
		v = PSG_VT[a];
		a = (*s++) & 15;
		v += PSG_VT[a];
		a = (*s++) & 15;
		v += PSG_VT[a];
		v -= PCM3_TABLE_offset;
		v /= PCM3_TABLE_gain;
		v *= 255;
		if (v < 0) v = 0;
		if (v > 255) v = 255;
		*d++ = (uint8_t)v;
	}
	dst->length = count;
}

#if 0
// 16bit はお蔵入り
/* ----- 16bit linear support ----- */

/* 16bit の変換は再生の便利用なので、PSGPCM への片側変換だけ用意する。 */

/* ----- linear16 to PAM2 ----- */
void
conv_s16BE_pam2(BUFFER *dst, BUFFER *src)
{
	conv_s16BE_pcm2(dst, src);
}

void
conv_s16LE_pam2(BUFFER *dst, BUFFER *src)
{
	conv_s16LE_pcm2(dst, src);
}

/* ----- linear16 to PAM3 ----- */
void
conv_s16BE_pam3(BUFFER *dst, BUFFER *src)
{
	conv_s16BE_pcm3(dst, src);
}

void
conv_s16LE_pam3(BUFFER *dst, BUFFER *src)
{
	conv_s16LE_pcm3(dst, src);
}

/* ----- linear16 to PCM1 ----- */
void
conv_s16BE_pcm1(BUFFER *dst, BUFFER *src)
{
	int count = src->count;

	uint8_t *s = src->buf;
	uint8_t *d = dst->buf;

	for (int i = 0; i < count; i++) {
		*d++ = PCM1_TABLE[*s ^ 0x80];
		s += 2;
	}
}

void
conv_s16LE_pcm1(BUFFER *dst, BUFFER *src)
{
	int count = src->count;

	uint8_t *s = src->buf;
	s += 1;
	uint8_t *d = dst->buf;

	for (int i = 0; i < count; i++) {
		*d++ = PCM1_TABLE[*s ^ 0x80];
		s += 2;
	}
}

/* ----- linear16 to PCM2 ----- */
void
conv_s16BE_pcm2(BUFFER *dst, BUFFER *src)
{
	int count = src->count;

	uint8_t *s = src->buf;
	uint16_t *d = dst->buf;

	for (int i = 0; i < count; i++) {
		*d++ = PCM2_TABLE[*s ^ 0x80];
		s += 2;
	}
}

void
conv_s16LE_pcm2(BUFFER *dst, BUFFER *src)
{
	int count = src->count;

	uint8_t *s = src->buf;
	s += 1;
	uint16_t *d = dst->buf;

	for (int i = 0; i < count; i++) {
		*d++ = PCM2_TABLE[*s ^ 0x80];
		s += 2;
	}
}

/* ----- linear16 to PCM3 ----- */
void
conv_s16BE_pcm3(BUFFER *dst, BUFFER *src)
{
	int count = src->count;

	uint8_t *s = src->buf;
	uint32_t *d = dst->buf;

	for (int i = 0; i < count; i++) {
		*d++ = PCM3_TABLE[*s ^ 0x80];
		s += 2;
	}
}

void
conv_s16LE_pcm3(BUFFER *dst, BUFFER *src)
{
	int count = src->count;

	uint8_t *s = src->buf;
	s += 1;
	uint32_t *d = dst->buf;

	for (int i = 0; i < count; i++) {
		*d++ = PCM3_TABLE[*s ^ 0x80];
		s += 2;
	}
}

#endif	// 16bit はお蔵入り

