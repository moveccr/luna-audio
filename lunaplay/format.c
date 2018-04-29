/* vi: set ts=4: */
/* TODO: LICENSE */

#include <string.h>
#include "lunaplay.h"

struct format_item {
	const char *str;
	int format;
	int enc;
};

static const struct format_item arg_list[] = {
	{ STR_WAV,  FMT_WAV,    ENC_U8 },
	{ STR_AU,   FMT_AU,     ENC_U8 },
	{ STR_PCM1, FMT_PSGPCM, ENC_PCM1 },
	{ STR_PCM2, FMT_PSGPCM, ENC_PCM2 },
	{ STR_PCM3, FMT_PSGPCM, ENC_PCM3 },
	{ STR_PAM2, FMT_PSGPCM, ENC_PAM2 },
	{ STR_PAM3, FMT_PSGPCM, ENC_PAM3 },
};

static const struct format_item format_list[] = {
	{ STR_WAV, FMT_WAV, 0 },
	{ STR_AU,  FMT_AU, 0 },
	{ STR_PSGPCM, FMT_PSGPCM, 0 },
};

static const struct format_item enc_list[] = {
	{ STR_U8, 0, ENC_U8 },
	{ STR_PCM1, 0, ENC_PCM1 },
	{ STR_PCM2, 0, ENC_PCM2 },
	{ STR_PCM3, 0, ENC_PCM3 },
	{ STR_PAM2, 0, ENC_PAM2 },
	{ STR_PAM3, 0, ENC_PAM3 },
};


/*
 文字列から、フォーマットコードとエンコーディングを求めます。
 成功すれば 0 を返します。
 失敗すると -1 を返します。
 */
int
parse_arg_format_enc(const char *arg, int *format, int *enc)
{
	for (int i = 0; i < countof(arg_list); i++) {
		if (strcasecmp(arg, arg_list[i].str) == 0) {
			*format = arg_list[i].format;
			*enc = arg_list[i].enc;
			return 0;
		}
	}
	return -1;
}

const char *
format_tostr(const int format)
{
	for (int i = 0; i < countof(format_list); i++) {
		if (format_list[i].format == format) {
			return format_list[i].str;
		}
	}
	return STR_UNKNOWN;
}

const char *
enc_tostr(const int enc)
{
	for (int i = 0; i < countof(enc_list); i++) {
		if (enc_list[i].enc == enc) {
			return enc_list[i].str;
		}
	}
	return STR_UNKNOWN;
}

