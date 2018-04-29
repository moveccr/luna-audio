/* vi: set ts=4: */
/* TODO: LICENSE */
#pragma once

#include <stdbool.h>
#include <stdint.h>

/* ----- constants ----- */

#define STR_UNKNOWN		"?"
#define STR_WAV			"WAV"
#define STR_AU			"AU"
#define STR_PSGPCM		"PSGPCM"

#define STR_U8			"U8"
#define STR_PCM1		"PCM1"
#define STR_PCM2		"PCM2"
#define STR_PCM3		"PCM3"
#define STR_PAM2		"PAM2"
#define STR_PAM3		"PAM3"

enum {
	FMT_UNKNOWN = 0,
	FMT_WAV,
	FMT_AU,
	FMT_PSGPCM,
};

enum {
	ENC_UNKNOWN = 0,
	ENC_U8 = 1,
	ENC_PCM1 = 0x41,
	ENC_PCM2,
	ENC_PCM3,
	ENC_PAM2,
	ENC_PAM3,
};

/* PSG voltage table */
extern const double PSG_VT[16];

/* ----- types ----- */

struct DESC_T;
struct BUFFER_T;

typedef int (*READER)(struct DESC_T *desc, struct BUFFER_T *buf);
typedef int (*WRITER)(struct DESC_T *desc, struct BUFFER_T *buf);
typedef int (*CLOSER)(struct DESC_T *desc);

typedef struct DESC_T
{
	int fd;
	int originalfd;		// save for original fd (for use temporary file)
	int format;			// file format
	int enc;			// encoding
	int freq;			// Hz

	READER reader;
	WRITER writer;
	CLOSER closer;
} DESC;

typedef struct BUFFER_T
{
	size_t bufsize;	// buffer byte length (capacity)
	size_t length;	// valid byte length
	bool isfree;	// call free() if true
	uint8_t *ptr;	// buffer pointer
} BUFFER;

/* fixed (fit as XP(Z80) buffer, 16KiB) */
#define XP_BUFSIZE	(16384)

typedef void (*CONVERTER)(BUFFER *dst, BUFFER *src);

/* ----- functions ----- */

#define countof(x) (sizeof(x)/sizeof((x)[0]))

extern int wav_read_init(DESC *desc, int fd);
extern int wav_write_init(DESC *desc, int fd);
extern int psgpcm_read_init(DESC *desc, int fd);
extern int psgpcm_write_init(DESC *desc, int fd);
extern int xp_write_init(DESC *desc);

extern int parse_arg_format_enc(const char *arg, int *format, int *enc);
extern const char *format_tostr(const int format);
extern const char *enc_tostr(const int enc);

/* ----- variables ----- */

extern int opt_v;
extern char *opt_firmware;

