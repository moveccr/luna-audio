/* vi: set ts=4: */
/* TODO: LICENSE */
#pragma once

#include <stdint.h>

/* ----- constants ----- */

#define FMTSTR_WAV			"WAV"
#define FMTSTR_AU			"AU"
#define FMTSTR_PCM1			"PCM1"
#define FMTSTR_PCM2			"PCM2"
#define FMTSTR_PCM3			"PCM3"
#define FMTSTR_PAM2			"PAM2"
#define FMTSTR_PAM3			"PAM3"

enum {
	FMT_WAV = 0,
	FMT_AU,
	FMT_PCM1,
	FMT_PCM2,
	FMT_PCM3,
	FMT_PAM2,
	FMT_PAM3,
};

/* PSG voltage table */
extern const double PSG_VT[16];

/* ----- types ----- */

struct BUFFER_T;

typedef int (*READER)(struct BUFFER_T *buf);
typedef int (*WRITER)(struct BUFFER_T *buf);
typedef int (*CLOSER)(struct BUFFER_T *buf);

typedef struct BUFFER_T
{
	int32_t	count;	// contain sample count
	void *buf;

	READER reader;
	WRITER writer;
	CLOSER closer;

	int fd;
	int originalfd;
	int freq;
	uint32_t total;
} BUFFER;

/* fixed (fit as XP(Z80) buffer, 16KiB) */
#define BUFFER_SIZE (16384)

typedef void (*CONVERTER)(BUFFER *dst, BUFFER *src);

/* ----- functions ----- */

#define countof(x) (sizeof(x)/sizeof((x)[0]))

extern int wav_read_init(BUFFER *buf, int fd);
extern int wav_write_init(BUFFER *buf, int fd);
extern int lunaplay_read_init(BUFFER *buf, int fd);
extern int lunaplay_write_init(BUFFER *buf, int fd);
extern int xp_write_init(BUFFER *buf);

/* ----- variables ----- */

extern int opt_v;

