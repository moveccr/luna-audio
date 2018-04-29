/* vi: set ts=4: */
/* see LICENSE */ 

/* PSGPCM format reader writer */

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/endian.h>
#include "lunaplay.h"
#include "filehelper.h"

static int psgpcm_read(DESC *desc, BUFFER *buf);
static int psgpcm_write(DESC *desc, BUFFER *buf);

static int psgpcm_close(DESC *desc);

/* ***** initializer ***** */
#define PSGPCM_TAG "PSGP"

int
psgpcm_read_init(DESC *desc, int fd)
{
	int n;
	int r;

	uint32_t tag = 0;
	uint16_t enc = 0;
	uint32_t freq;

	r =
	readtag(fd, &tag) &&
	cmptag(tag, PSGPCM_TAG) &&
	read16le(fd, &enc) &&
	read32le(fd, &freq)
	;

	if (!r) {
		fprintf(stderr, "PSGPCM header read error\n");
		return -1;
	}

	if (opt_v) {
		fprintf(stderr, "enc    : %d\n", enc);
		fprintf(stderr, "freq   : %d\n", freq);
	}

	switch (enc) {
	 case ENC_PCM1:
	 case ENC_PCM2:
	 case ENC_PCM3:
	 case ENC_PAM2:
	 case ENC_PAM3:
		break;
	 default:
		fprintf(stderr, "enc %d is not supported\n", enc);
		return -1;
	}
	desc->enc = enc;

	if (opt_v) {
		fprintf(stderr, "PSGPCM encoding: %s\n", enc_tostr(enc));
	}

	desc->reader = psgpcm_read;
	desc->closer = psgpcm_close;

	desc->fd = fd;
	desc->freq = freq;
}

int
psgpcm_write_init(DESC *desc, int fd)
{
	writetag(fd, PSGPCM_TAG);
	write16le(fd, desc->enc);
	write32le(fd, desc->freq);

	desc->writer = psgpcm_write;
	desc->closer = psgpcm_close;

	desc->fd = fd;
}

/* ***** reader ***** */

static
int
psgpcm_read(DESC *desc, BUFFER *buf)
{
	int n = readbuf(desc->fd, buf->ptr + buf->length, buf->bufsize - buf->length);
	if (n < 0) {
		return n;
	}
	if (n == 0) {
		return buf->length;
	}
	buf->length += n;
	return buf->length;
}

/* ***** writer ***** */

static
int
psgpcm_write(DESC *desc, BUFFER *buf)
{
	int rv = writebuf(desc->fd, buf->ptr, buf->length);
	buf->length = 0;
	return rv;
}

/* ***** closer ***** */

static
int
psgpcm_close(DESC *desc)
{
	close(desc->fd);
}

