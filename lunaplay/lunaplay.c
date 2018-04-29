/* vi: set ts=4: */
/* TODO: LICENSE */

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include "lunaplay.h"
#include "psgconv.h"

#define VERSION "0.1"

/* global */
int opt_v;		// verbose
char *opt_firmware;	// firmware file

/* ***** static func ***** */

static
bool
isextension(const char *fname, const char *ext)
{
	char *p = strrchr(fname, '.');
	if (p == NULL) {
		return false;
	}
	return strncasecmp(p, ext, strlen(ext)) == 0;
}

static
int
enc_stride(int enc)
{
	switch (enc) {
	 case ENC_U8:
		return 1;
	 case ENC_PCM1:
		return 1;
	 case ENC_PCM2:
	 case ENC_PAM2:
		return 2;
	 case ENC_PCM3:
	 case ENC_PAM3:
		return 4;
	 default:
		errx(EXIT_FAILURE, "unknown encoding");
	}
}

static
CONVERTER
get_conv_u8_to(int enc)
{
	switch (enc) { 
	 case ENC_U8:
		return conv_pass;
	 case ENC_PCM1:
		return conv_u8_pcm1;
	 case ENC_PCM2:
		return conv_u8_pcm2;
	 case ENC_PCM3:
		return conv_u8_pcm3;
	 case ENC_PAM2:
		return conv_u8_pam2;
	 case ENC_PAM3:
		return conv_u8_pam3;
	 default:
		errx(EXIT_FAILURE, "unknown encoding");
	}
}

static
CONVERTER
get_conv_from_u8(int enc)
{
	switch (enc) { 
	 case ENC_U8:
		return conv_pass;
	 case ENC_PCM1:
		return conv_pcm1_u8;
	 case ENC_PCM2:
		return conv_pcm2_u8;
	 case ENC_PCM3:
		return conv_pcm3_u8;
	 case ENC_PAM2:
		return conv_pam2_u8;
	 case ENC_PAM3:
		return conv_pam3_u8;
	 default:
		errx(EXIT_FAILURE, "unknown encoding");
	}
}

static
void
buffer_free(BUFFER *buf)
{
	if (buf->isfree) {
		free(buf->ptr);
	}
}

static
void
memset8(void *buf, uint8_t val, int len)
{
	memset(buf, val, len);
}

static
void
memset16(void *buf, uint16_t val, int len)
{
	uint16_t *p = (uint16_t *)buf;
	for (int i = 0; i < len; i += 2) {
		*p++ = val;
	}
}

static
void
memset32(void *buf, uint32_t val, int len)
{
	uint32_t *p = (uint32_t *)buf;
	for (int i = 0; i < len; i += 4) {
		*p++ = val;
	}
}

static
void
filltail(BUFFER *buf, int stride)
{
	uint8_t *p = buf->ptr + buf->length;
	int len = buf->bufsize - buf->length;
	if (stride == 1) {
		memset8(p, *(p - 1), len);
	} else if (stride == 2) {
		memset16(p, *(p - 2), len);
	} else if (stride == 4) {
		memset32(p, *(p - 4), len);
	} else {
		errx(EXIT_FAILURE, "invalid stride");
	}
}

_Noreturn
static
void
usage()
{
	fprintf(stderr,
"Play PCM on LUNA-I version %s\n"
"%s <options> <file>\n"
"  file  input file\n"
"\n"
"options\n"
"  -i<format>\n"
"        override input format\n"
"  -o<format>\n"
"        set output format\n"
"  -O<file>\n"
"        output file\n"
"  -v    verbose level +1\n"
"  -h    show help\n"
"\n"
"format (ignore case)\n"
"  WAV   WAV file format (input default)\n"
"  AU    (not implemented)\n"
"  PCM1  PCM1 format\n"
"  PCM2  PCM1 format\n"
"  PCM3  PCM1 format\n"
"  PAM2  PAM2 format\n"
"  PAM3  PAM3 format (output default)\n"
		,
		VERSION,
		getprogname()
	);
	exit(1);
}

int
main(int ac, char *av[])
{
	int r;
	int c;
	char *endp;
	int freq = 0;
	double dfreq = 0;
	char *in_file = NULL;
	char *out_file = NULL;
	int in_enc = ENC_UNKNOWN;
	int out_enc = ENC_UNKNOWN;
	int in_format = FMT_UNKNOWN;
	int out_format = FMT_UNKNOWN;
	int in_fd;
	int out_fd;

	DESC in0, *in = &in0;
	DESC out0, *out = &out0;
	BUFFER src0, *src = &src0;
	BUFFER dst0, *dst = &dst0;
	CONVERTER conv = NULL;

	opt_v = 0;
	memset(in, 0, sizeof(DESC));
	memset(out, 0, sizeof(DESC));
	memset(src, 0, sizeof(BUFFER));
	memset(dst, 0, sizeof(BUFFER));

	while ((c = getopt(ac, av, "f:i:O:o:hv")) != -1) {
		switch (c) {
		 case 'f':
			dfreq = strtod(optarg, &endp);
			if (*endp == 'k') {
				dfreq *= 1000;
			}
			if (dfreq < 0) {
				errx(1, "Invalid frequency: %s", optarg);
			}
			freq = (int)dfreq;
			break;
		 case 'i':
			r = parse_arg_format_enc(optarg, &in_format, &in_enc);
			if (r < 0) {
				errx(1, "Invalid format: %s", optarg);
			}
			break;
		 case 'O':
			out_file = optarg;
			break;
		 case 'o':
			r = parse_arg_format_enc(optarg, &out_format, &out_enc);
			if (r < 0) {
				errx(1, "Invalid format: %s", optarg);
			}
			break;
		 case 'v':
			opt_v++;
			break;
		 case 'h':
		 default:
			usage();
		}
	}
	if (optind >= ac) {
		errx(1, "missing input file");
	}

	in_file = av[optind];
	// XP デバイス宛?
	bool isdevxp = out_file == NULL;

	if (opt_v >= 1) {
		printf("arguments...\n");
		printf("verbose level  :%d\n", opt_v);
		printf("input format   :%s\n", format_tostr(in_format));
		printf("input encoding :%s\n", enc_tostr(in_enc));
		printf("input file     :%s\n", in_file);
		printf("output format  :%s\n", format_tostr(out_format));
		printf("output encoding:%s\n", enc_tostr(out_enc));
		printf("output file    :%s\n", isdevxp ? "XP device" : out_file);
		printf("output freq    :%d\n", freq);
	}

	if (in_format == FMT_UNKNOWN) {
		if (isextension(in_file, "." STR_WAV)) {
			in_format = FMT_WAV;
		} else if (isextension(in_file, "." STR_PSGPCM)) {
			in_format = FMT_PSGPCM;
		} else {
			errx(EXIT_FAILURE, "input format undeterminate");
		}
		if (opt_v >= 1) {
			printf("input format change to %s\n", format_tostr(in_format));
		}
	}

	if (out_format == FMT_UNKNOWN) {
		if (isextension(out_file, "." STR_WAV)) {
			out_format = FMT_WAV;
		} else if (isextension(out_file, "." STR_PSGPCM)) {
			out_format = FMT_PSGPCM;
		} else {
			errx(EXIT_FAILURE, "output format undeterminate");
		}
		if (opt_v >= 1) {
			printf("output format change to %s\n", format_tostr(out_format));
		}
		if (out_enc == ENC_UNKNOWN) {
			if (out_format == FMT_WAV) out_enc = ENC_U8;
			if (out_format == FMT_PSGPCM) out_enc = ENC_PAM3;
		}
	}

	if (strcmp(in_file, "-") == 0) {
		if (opt_v) printf("opening stdin\n");
		in_fd = STDIN_FILENO;
	} else {
		if (opt_v) printf("opening %s\n", in_file);
		in_fd = open(in_file, O_RDONLY);
		if (in_fd == -1) {
			err(1, "open: %s", in_file);
		}
	}

	if (opt_v) printf("reading %s\n", format_tostr(in_format));
	if (in_format == FMT_WAV) {
		if (wav_read_init(in, in_fd) < 0) {
			errx(EXIT_FAILURE, "wav read init error");
		}
	} else if (in_format == FMT_AU) {
		errx(1, "notimplemented");
	} else {
		if (psgpcm_read_init(in, in_fd) < 0) {
			errx(EXIT_FAILURE, "psgpcm read init error");
		}
	}

	if (opt_v) {
		printf("freq=%d, in->freq=%d\n", freq, in->freq);
	}
	if (freq == 0) {
		if (opt_v) printf("set out->freq = in->freq = %d\n", in->freq);
		out->freq = in->freq;
	} else {
		if (opt_v) printf("set out->freq = freq = %d\n", freq);
		out->freq = freq;
	}

	if (out_enc == ENC_UNKNOWN) {
		out->enc = in->enc;
	} else {
		out->enc = out_enc;
	}

	dst->bufsize = XP_BUFSIZE;
	dst->ptr = malloc(dst->bufsize);
	dst->isfree = true;
	if (in->enc == out->enc) {
		src->bufsize = dst->bufsize;
		src->ptr = dst->ptr;
		src->isfree = false;
	} else {
		if (in->enc == ENC_U8) {
			conv = get_conv_u8_to(out->enc);
		} else if (out->enc == ENC_U8) {
			conv = get_conv_from_u8(in->enc);
		} else {
			errx(EXIT_FAILURE, "unsupported encoding pair");
		}
		src->bufsize = dst->bufsize * enc_stride(in->enc) / enc_stride(out->enc);
		src->ptr = malloc(src->bufsize);
		src->isfree = true;
	}

	if (out_file == NULL) {
		if (opt_v) printf("xp write initializing\n");
		if (xp_write_init(out) < 0) {
			errx(EXIT_FAILURE, "xp write init error");
		}
	} else {
		if (strcmp(out_file, "-") == 0) {
			out_fd = STDOUT_FILENO;
		} else {
			out_fd = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
			if (out_fd == -1) {
				err(1, "open: %s", out_file);
			}
		}
		if (opt_v) printf("%s write initializing\n", format_tostr(out_format));
		if (out_format == FMT_WAV) {
			if (wav_write_init(out, out_fd) < 0) {
				errx(EXIT_FAILURE, "wav write init error");
			}
		} else if (out_format == FMT_AU) {
			errx(1, "notimplemented");
		} else {
			if (psgpcm_write_init(out, out_fd) < 0) {
				errx(EXIT_FAILURE, "psgpcm write init error");
			}
		}
	}

	if (opt_v >= 1) {
		printf("running...\n");
		printf("input format   :%s\n", format_tostr(in_format));
		printf("input encoding :%s\n", enc_tostr(in->enc));
		printf("input file     :%s\n", in_file);
		printf("output format  :%s\n", format_tostr(out_format));
		printf("output encoding:%s\n", enc_tostr(out->enc));
		printf("output file    :%s\n", out_file == NULL ? "XP device" : out_file);
		printf("output freq    :%d\n", out->freq);
		printf("input bufsize  :%d\n", src->bufsize);
		printf("output bufsize :%d\n", dst->bufsize);
	}

	for (;;) {
		src->length = 0;
		dst->length = 0;
		r = in->reader(in, src);
		if (r < 0) {
			fprintf(stderr, "read error %s", strerror(errno));
			break;
		}
		if (r == 0) {
			break;
		}
		if (isdevxp && src->length < src->bufsize) {
			// XP デバイス宛の書き込みはブロック単位なのでフィル
			// ファイル終端でしか成立はしない
			filltail(src, enc_stride(in->enc));
		}
		conv(dst, src);
		r = out->writer(out, dst);
		if (r < 0) {
			fprintf(stderr, "write error %s", strerror(errno));
			break;
		}
	}

	in->closer(in);
	out->closer(out);

	buffer_free(src);
	buffer_free(dst);

	return 0;
}

