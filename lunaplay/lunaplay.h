/* vi: set ts=4: */
/* TODO: LICENSE */
#pragma once

#define FMTSTR_WAV			"WAV"
#define FMTSTR_LUNAPAM2		"LUNAPAM2"
#define FMTSTR_LUNAPCM1		"LUNAPCM1"
#define FMTSTR_LUNAPCM2		"LUNAPCM2"
#define FMTSTR_LUNAPCM3		"LUNAPCM3"
#define FMTSTR_AU			"AU"

enum {
	FMT_WAV = 0,
	FMT_LUNAPAM2,
	FMT_LUNAPCM1,
	FMT_LUNAPCM2,
	FMT_LUNAPCM3,
	FMT_AU,
};

#define countof(x) (sizeof(x)/sizeof((x)[0]))

extern int opt_v;

