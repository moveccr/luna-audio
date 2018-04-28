/* vi: set ts=4: */
/* see LICENSE */ 

/* WAV format reader writer */

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

static int wav_read_1u8(BUFFER *buf);
static int wav_read_2u8(BUFFER *buf);
static int wav_read_1s16le(BUFFER *buf);
static int wav_read_2s16le(BUFFER *buf);

/* ***** read helper ***** */
static
int
read32le(int fd, int32_t *rv)
{
	int n = read(fd, rv, 4);
	if (n != 4) return 0;
	*rv = le32toh(*rv);
	return n;
}

static
int
read16le(int fd, int16_t *rv)
{
	int n = read(fd, rv, 2);
	if (n != 2) return 0;
	*rv = le16toh(*rv);
	return n;
}

static
int
readtag(int fd, uint32_t *rv)
{
	int n = read(fd, rv, 4);
	if (n != 4) return 0;
	*rv = be16toh(*rv);
	return n;
}

static
int
cmptag(uint32_t tag, const char strtag[4])
{
	uint32_t t =
		((uint8_t)strtag[0]) << 24
	 |  ((uint8_t)strtag[1]) << 16
	 |  ((uint8_t)strtag[2]) << 8
	 |  ((uint8_t)strtag[3]);

	return tag == t;
}

static
void
skip(int fd, int bytes)
{
	int b;
	for (int i = 0; i < bytes; i++) {
		read(fd, &b, 1);
	}
}

/* ***** write helper ***** */
static
int
writetag(int fd, char strtag[4])
{
	int n = write(fd, strtag, 4);
	if (n != 4) return 0;
	return n;
}

static
int
write32le(int fd, int32_t v)
{
	v = htole32(v);
	int n = write(fd, &v, 4);
	if (n != 4) return 0;
	return n;
}

static
int
write16le(int fd, int16_t v)
{
	v = htole16(v);
	int n = write(fd, &v, 2);
	if (n != 2) return 0;
	return n;
}

/* ***** initializer ***** */

int
wav_read_init(BUFFER *buf, int fd)
{
	int n;
	int r;

	uint32_t tag = 0;
	int32_t rifflen = 0;
	int32_t fmtlen = 0;
	uint16_t fmtid = 0;
	uint16_t channelcount = 0;
	uint32_t freq;
	uint32_t dummy32;
	uint16_t dummy16;
	uint16_t bitpersample;
	int32_t datalen;

	r =
	readtag(fd, &tag) &&
	cmptag(tag, "RIFF") &&
	read32le(fd, &rifflen) &&
	readtag(fd, &tag) &&
	cmptag(tag, "WAVE");
	if (!r) {
		fprintf(stderr, "WAVE header read error\n");
		return -1;
	}

	while (1) {
		if (!readtag(fd, &tag)) {
			fprintf(stderr, "TAG read error\n");
			return -1;
		}

		if (cmptag(tag, "fmt ")) {
			r = 
			read32le(fd, &fmtlen) &&
			read16le(fd, &fmtid) &&
			read16le(fd, &channelcount) &&
			read32le(fd, &freq) &&
			read32le(fd, &dummy32) &&
			read16le(fd, &dummy16) &&
			read16le(fd, &bitpersample)
			;
			if (!r) {
				fprintf(stderr, "fmt heaer error\n");
				return -1;
			}
			skip(fd, fmtlen - (2+2+4+4+2+2));
		} else if (cmptag(tag, "data")) {
			if (!read32le(fd, &datalen)) {
				fprintf(stderr, "data len error\n");
				return -1;
			}
			break;
		} else {
			int32_t chunklen;
			if (read32le(fd, &chunklen)) {
				fprintf(stderr, "unknown chunk error\n");
				return -1;
			}
			skip(fd, chunklen);
		}
	}
	if (opt_v) {
		fprintf(stderr, "fmtid  : %d\n", fmtid);
		fprintf(stderr, "channel: %d\n", channelcount);
		fprintf(stderr, "freq   : %d\n", freq);
		fprintf(stderr, "bit    : %d\n", bitpersample);
	}

	if (fmtid != 1) {
		fprintf(stderr, "fmtid %d is not supported\n", fmtid);
		return -1;
	}

	const char *wavfmt;
	buf->reader = NULL;
	if (channelcount == 1) {
		if (bitpersample == 8) {
			wavfmt = "1u8";
			buf->reader = wav_read_1u8;
		} else if (bitpersample == 16) {
			wavfmt = "1s16le";
			buf->reader = wav_read_1s16le;
		}
	} else if (channelcount == 2) {
		if (bitpersample == 8) {
			wavfmt = "2u8";
			buf->reader = wav_read_2u8;
		} else if (bitpersample == 16) {
			wavfmt = "2s16le";
			buf->reader = wav_read_2s16le;
		}
	}
	if (buf->reader == NULL) {
		fprintf(stderr, "WAV unsupported format\n");
		return -1;
	}

	if (opt_v) {
		fprintf(stderr, "WAV format: %s\n", wavfmt);
	}

	buf->buf = malloc(BUFFER_SIZE);
	buf->count = 0;
	buf->fd = fd;
	buf->freq = freq;
	buf->total = datalen;
}

int
wav_write_init(BUFFER *buf, int fd)
{
	/*
	 * WAV ファイルは書き込みサイズが決まらないとヘッダが作れないので
	 * テンポラリファイルに書きだしてあとで処理する。
	 */
	int tmpfd;
	char template[] = "/tmp/lunaplay.XXXXXX";

	tmpfd = mkstemp(template);
	if (tmpfd == -1) {
		err(1, "mkstemp");
	}
	unlink(template);

	buf->buf = malloc(BUFFER_SIZE);
	buf->count = 0;
	buf->fd = tmpfd;
	buf->originalfd = fd;
}

/* ***** reader ***** */

static
int
wav_read_1u8(BUFFER *buf)
{
	int n = read(buf->fd, buf->buf, BUFFER_SIZE);
	buf->count = n;
}

static
int
wav_read_2u8(BUFFER *buf)
{
	uint8_t tmp[BUFFER_SIZE * 2];

	int n = read(buf->fd, tmp, BUFFER_SIZE * 2);
	int count = n / 2;
	uint8_t *s = tmp;
	uint8_t *d = buf->buf;
	for (int i = 0; i < count; i++) {
		uint16_t a, b;
		a = *s++;
		b = *s++;
		*d++ = (a + b) >> 1;
	}
	buf->count = count;
}

static
int
wav_read_1s16le(BUFFER *buf)
{
	uint8_t tmp[BUFFER_SIZE * 2];

	int n = read(buf->fd, tmp, BUFFER_SIZE * 2);
	int count = n / 2;
	uint8_t *s = tmp;
	s++;
	uint8_t *d = buf->buf;
	for (int i = 0; i < count; i++) {
		*d++ = (*s) ^ 0x80;
		s += 2;
	}
	buf->count = count;
}

static
int
wav_read_2s16le(BUFFER *buf)
{
	uint8_t tmp[BUFFER_SIZE * 4];

	int n = read(buf->fd, tmp, BUFFER_SIZE * 4);
	int count = n / 4;
	uint8_t *s = tmp;
	s++;
	uint8_t *d = buf->buf;
	for (int i = 0; i < count; i++) {
		uint16_t a, b;
		a = (*s) ^ 0x80;
		s += 2;
		b = (*s) ^ 0x80;
		s += 2;
		*d++ = (a + b) >> 1;
	}
	buf->count = count;
}

/* ***** writer ***** */

static
int
wav_write_1u8(BUFFER *buf)
{
	int n = buf->count;
	int r;
	for (int m = 0; m < n; m += r) {
		r = write(buf->fd, ((char*)buf->buf) + m, n - m);
		if (r == -1) {
			return -1;
		}
	}
	return n;
}

/* ***** closer ***** */

static
int
wav_read_close(BUFFER *buf)
{
	close(buf->fd);
	free(buf->buf);
}

static
int
wav_write_close(BUFFER *buf)
{
	/*
	 * テンポラリファイルとヘッダを合体させて最終ファイルを作る
	 */

	int fd = buf->originalfd;
	int tmpfd = buf->fd;
	int r;
	int rv = -1;

	int32_t datalen = (int32_t)lseek(buf->fd, 0, SEEK_CUR);

	r = 
	writetag(fd, "RIFF") &&
	write32le(fd,
		4	// WAVE
		+4	// fmt 
		+4	// fmt chunk length
		+16	// fmt chunk
		+4	// data
		+4	// data chunk length
		+ datalen) &&
	writetag(fd, "WAVE") && 
	writetag(fd, "fmt ") &&
	write32le(fd, 16) &&	// fmt chunk length
	write16le(fd, 1) &&		// PCM FORMAT
	write16le(fd, 1) &&		// channel count
	write32le(fd, buf->freq) &&	// freq
	write32le(fd, buf->freq) &&	// data rate (1ch 1byte)
	write16le(fd, 1) &&
	write16le(fd, 8) &&
	writetag(fd, "data") &&
	write32le(fd, datalen)
	;
	if (!r) {
		fprintf(stderr, "write: %s\n", strerror(errno));
		goto done;
	}

	lseek(tmpfd, 0, SEEK_SET);
	for (;;) {
		char buf[4096];
		int n = read(tmpfd, buf, sizeof(buf));
		if (n == 0) break;
		if (n == -1) {
			fprintf(stderr, "read: %s\n", strerror(errno));
			goto done;
		}
		int r;
		for (int m = 0; m < n; m += r) {
			r = write(fd, buf + m, n - m);
			if (r == -1) {
				fprintf(stderr, "write: %s\n", strerror(errno));
				goto done;
			}
		}
	}
	rv = 0;

 done:
	close(buf->fd);
	close(buf->originalfd);
	free(buf->buf);
	return rv;
}

