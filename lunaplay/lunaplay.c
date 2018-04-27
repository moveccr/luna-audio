/* vi: set ts=4: */
/* TODO: LICENSE */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include "lunaplay.h"

#define VERSION "0.1"

/* global */
int opt_v;		// verbose

static const char *in_file;		// input file
static const char *out_file;	// output file
static int in_format;	// input format
static int out_format;	// output format
static int out_freq;	// output frequency

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
"  LUNAPAM2  LUNAPAM2 format\n"
"  LUNAPCM1  LUNAPCM1 format\n"
"  WAV       WAV file format\n"
"  AU        (not implemented)\n"
		,
		VERSION,
		getprogname()
	);
	exit(1);
}

/* フォーマットコード順に並べること。 */
static const char *FMTSTR_list[] = {
	FMTSTR_WAV,
	FMTSTR_LUNAPAM2,
	FMTSTR_LUNAPCM1,
	FMTSTR_LUNAPCM2,
	FMTSTR_LUNAPCM3,
	FMTSTR_AU,
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
	out_format = FMT_LUNAPAM2;
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




	return 0;
}
