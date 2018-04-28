/* vi: set ts=4: */
/* TODO: LICENSE */

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include "lunaplay.h"
#include "psgconv.h"

#define VERSION "0.1"

/* global */
int opt_v;		// verbose

static const char *in_file;		// input file
static const char *out_file;	// output file
static int in_format;	// input format
static int out_format;	// output format
static int out_freq;	// output frequency

CONVERTER converters[][7] = {
	{
		conv_pass,	// WAV
		NULL,		// AU
		conv_u8_pcm1, conv_u8_pcm2, conv_u8_pcm3,
		conv_u8_pam2, conv_u8_pam3,
	},
	{ conv_pcm1_u8, NULL, conv_pass, NULL, NULL, NULL, NULL, },
	{ conv_pcm2_u8, NULL, NULL, conv_pass, NULL, NULL, NULL, },
	{ conv_pcm3_u8, NULL, NULL, NULL, conv_pass, NULL, NULL, },
	{ conv_pam2_u8, NULL, NULL, NULL, NULL, conv_pass, NULL, },
	{ conv_pam3_u8, NULL, NULL, NULL, NULL, NULL, conv_pass, },
};


_Noreturn
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
"        set input format\n"
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

/* フォーマットコード順に並べること。 */
static const char *FMTSTR_list[] = {
	FMTSTR_WAV,
	FMTSTR_AU,
	FMTSTR_PCM1,
	FMTSTR_PCM2,
	FMTSTR_PCM3,
	FMTSTR_PAM2,
	FMTSTR_PAM3,
};

/* フォーマット文字列から、フォーマットコードを返します。 */
static
int
parse_format(const char *format_str)
{
	for (int i = 0; i < countof(FMTSTR_list); i++) {
		if (strcasecmp(format_str, FMTSTR_list[i]) == 0) {
			return i;
		}
	}
	return -1;
}

/* フォーマットコードから、文字列表現を返します。 */
static
const char *
format_tostr(const int format)
{
	if (format < 0 || format >= countof(FMTSTR_list)) {
		return "?";
	}
	return FMTSTR_list[format];
}

int
main(int ac, char *av[])
{
	int c;
	char *endp;
	double freq;

	opt_v = 0;

	in_file = "";
	in_format = FMT_WAV;
	out_format = FMT_PAM3;
	out_file = "";

	while ((c = getopt(ac, av, "f:i:O:o:hv")) != -1) {
		switch (c) {
		 case 'f':
			freq = strtod(optarg, &endp);
			if (*endp == 'k') {
				freq *= 1000;
			}
			if (freq < 0) {
				errx(1, "Invalid frequency: %s", optarg);
			}
			break;
		 case 'i':
			in_format = parse_format(optarg);
			if (in_format < 0) {
				errx(1, "Invalid format: %s", optarg);
			}
			break;
		 case 'O':
			out_file = optarg;
			break;
		 case 'o':
			out_format = parse_format(optarg);
			if (out_format < 0) {
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

	if (opt_v >= 2) {
		printf("verbose level :%d\n", opt_v);
		printf("input format  :%s\n", format_tostr(in_format));
		printf("input file    :%s\n", in_file);
		printf("output format :%s\n", format_tostr(out_format));
		printf("output file   :%s\n", out_file);
		printf("output freq   :%f\n", out_freq);
	}


	BUFFER src0, *src;
	BUFFER dst0, *dst;
	CONVERTER conv;
	int in_fd;
	int out_fd;

	src = &src0;
	dst = &dst0;

	conv = converters[in_format][out_format];
	if (conv == NULL) {
		errx(1, "unsupported conversion");
	}

	if (strcmp(in_file, "-") == 0) {
		in_fd = STDIN_FILENO;
	} else {
		in_fd = open(in_file, O_RDONLY);
		if (in_fd == -1) {
			err(1, "open: %s", in_file);
		}
	}

	if (in_format == FMT_WAV) {
		wav_read_init(src, in_fd);
	} else if (in_format == FMT_AU) {
		errx(1, "notimplemented");
	} else {
		lunaplay_read_init(src, in_fd);
	}

	if (strcmp(out_file, "") == 0) {
		xp_write_init(dst);
		// TODO: audio
	} else {
		if (strcmp(out_file, "-") == 0) {
			out_fd = STDOUT_FILENO;
		} else {
			out_fd = open(out_file, O_WRONLY);
			if (out_fd == -1) {
				err(1, "open: %s", out_file);
			}
		}
		if (out_format == FMT_WAV) {
			wav_write_init(dst, out_fd);
		} else if (out_format == FMT_AU) {
		} else {
			lunaplay_write_init(dst, out_fd);
		}
	}

	dst->freq = src->freq;

	while (1) {
		src->reader(src);
		if (src->count == 0) break;
		conv(dst, src);
		dst->writer(dst);
	}

	src->closer(src);
	dst->closer(dst);

	return 0;
}
