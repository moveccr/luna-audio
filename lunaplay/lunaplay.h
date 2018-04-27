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

typedef struct BUFFER_T
{
	int32_t	count;	// contain sample count
	void *buf;
} BUFFER;

/* ----- functions ----- */

#define countof(x) (sizeof(x)/sizeof((x)[0]))

/* ----- variables ----- */

extern int opt_v;

