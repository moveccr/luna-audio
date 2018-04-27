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

/* PSG voltage table */
const double PSG_VT[16] = {
	0,
	0.0078125,
	1.104854346e-2,
	0.015625,
	2.209708691e-2,
	0.03125,
	4.419417382e-2,
	0.0625,
	8.838834765e-2,
	0.125,
	0.1767766953,
	0.25,
	0.3535533906,
	0.5,
	0.7071067812,
	1,
};

#define countof(x) (sizeof(x)/sizeof((x)[0]))

extern int opt_v;

