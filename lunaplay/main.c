/* vi:set ts=4 */
/* TODO: LICENSE */

#include "stdio.h"

/* global */
const int opt_v;		// verbose

static const char *in_file;		// input file
static const char *in_format;	// input format
static const char *out_file;	// output file
static const char *out_format;	// output format
static const int   out_freq;	// output frequency

_Noreturn
void
usage()
{
	fprintf(stderr, "\
Play PCM on LUNA-I version %s
%s <options> <file>
  file  input file

options
  -i<format>
        set input format
  -o<format>
        set output format
  -O<file>
        output file
  -v    verbose level +1
  -h    show help

format (ignore case)
  LUNAPAM2  LUNAPAM2 format
  LUNAPCM1  LUNAPCM1 format
  WAV       WAV file format
  AU        (not implemented)
",
	VERSION,
	getprogname()
);
	exit(1);
}

typedef struct BUFFER_T
{
	int32_t	count;	// contain sample count
	int32_t freq;	// freq
	uint8_t *buf;
} BUFFER;

typedef int (*BUFFER_ALLOC)(BUFFER* buf, int32_t count, int32_t freq);
typedef int (*READ)(int fd, BUFFER* buf);
typedef int (*WRITE)(int fd, BUFFER* buf);
typedef int (*CONVERT)(BUFFER* outbuf, BUFFER* inbuf);

typedef struct PLAYER_T
{
	BUFFER_ALLOC in_alloc;
	BUFFER_ALLOC out_alloc;

	READ read;
	WRITE write;
	CONVERT read_convert;
	CONVERT freq_convert;
	CONVERT write_convert;
} PLAYER;

static
PLAYER
parse_format(const char *out_format, const char *in_format)
{
	PLAYER rv;
	if (strcasecmp(in_format, FMT_WAV) == 0) {
		rv.in_alloc = wav_alloc;
		rv.read = wav_read;
		rv.read_convert = wav_to_u8;
	} else if (strcasecmp(in_format, FMT_LUNAPAM2) == 0) {
		rv.in_alloc = pam2_alloc;
		rv.read = pam2_read;
		rv.read_convert = pam2_to_u8;
	} else if (strcasecmp(in_format, FMT_LUNAPCM1) == 0) {
		rv.in_alloc = pcm1_alloc;
		rv.read = pcm1_read;
		rv.read_convert = pcm1_to_u8;
	} else {
		errx(1, "unknown format");
	}

	
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

	while ((c = getopt("f:i:O:o:hv")) != -1) {
		case 'f':
			freq = strtod(optarg, &endp);
			if (endp == 'k') {
				freq *= 1000;
			}
			break;
		case 'i':
			in_format = optarg;
			break;
		case 'O':
			out_file = optarg;
			break;
		case 'o':
			out_format = optarg;
			break;
		case 'v':
			opt_v++;
			break;
		case 'h':
		default:
			usage();
	}
	if (optind >= argc) {
		errx(1, "missing input file");
	}

	in_file = argv[optind];

	if (opt_v >= 2) {
		printf("verbose level :%d\n", opt_v);
		printf("input format  :%s\n", opt_i);
		printf("input file    :%s\n", in_file);
		printf("output format :%s\n", opt_o);
		printf("output file   :%s\n", opt_O);
	}

	/* parse format */
	if (stricmp(fmt, 

	if (stricmp(

	int in_fd;
	int out_fd;

	if (strcmp(in_file, "-") == 0) {
		in_fd = STDIN_FILENO;
	} else {
		in_fd = open(in_file, "r");
		if (in_fd == -1) {
			err("open input file");
		}
	}




	return 0;
}
