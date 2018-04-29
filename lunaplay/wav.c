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
#include "lunaplay.h"
#include "filehelper.h"

static int wav_read_1u8(DESC *desc, BUFFER *buf);
static int wav_read_2u8(DESC *desc, BUFFER *buf);
static int wav_read_1s16le(DESC *desc, BUFFER *buf);
static int wav_read_2s16le(DESC *desc, BUFFER *buf);

static int wav_write_1u8(DESC *desc, BUFFER *buf);

static int wav_read_close(DESC *desc);
static int wav_write_close(DESC *desc);

/* ***** initializer ***** */

int
wav_read_init(DESC *desc, int fd)
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
			if (opt_v) {
				printf("fmtlen       :%d\n", fmtlen);
				printf("fmtid        :%d\n", fmtid);
				printf("channelcount :%d\n", channelcount);
				printf("freq         :%d\n", freq);
				printf("bitpersample :%d\n", bitpersample);
			}

			if (!r) {
				fprintf(stderr, "fmt header read error\n");
				return -1;
			}
			int skip = fmtlen - (2+2+4+4+2+2);
			if (skip < 0) {
				fprintf(stderr, "fmt length error\n");
				return -1;
			} else if (skip > 0) {
				r = readskip(fd, fmtlen - (2+2+4+4+2+2));
				if (!r) {
					fprintf(stderr, "fmt header skip error\n");
					return -1;
				}
			}
		} else if (cmptag(tag, "data")) {
			if (!read32le(fd, &datalen)) {
				fprintf(stderr, "data len error\n");
				return -1;
			}
			break;
		} else {
			int32_t chunklen;
			if (!read32le(fd, &chunklen)) {
				fprintf(stderr, "unknown chunk error\n");
				return -1;
			}
			if (!readskip(fd, chunklen)) {
				fprintf(stderr, "unknown chunk read error\n");
				return -1;
			}
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

	READER readers[] = {
		wav_read_1u8, wav_read_2u8, wav_read_1s16le, wav_read_2s16le,
	};
	const char *readers_name[] = {
		"1u8", "2u8", "1s16le", "2s16le",
	};

	int rdid = 0;
	if (channelcount == 1) {
	} else if (channelcount == 2) {
		rdid += 1;
	} else {
		fprintf(stderr, "WAV unsupported channel count %d\n", channelcount);
		return -1;
	}
	if (bitpersample == 8) {
	} else if (bitpersample == 16) {
		rdid += 2;
	} else {
		fprintf(stderr, "WAV unsupported bitpersample %d\n", bitpersample);
		return -1;
	}
	desc->reader = readers[rdid];

	if (opt_v) {
		fprintf(stderr, "WAV format: %s\n", readers_name[rdid]);
	}

	desc->closer = wav_read_close;

	// reader が u8 に変換する
	desc->enc = ENC_U8;
	desc->fd = fd;
	desc->freq = freq;
	return 0;
}

int
wav_write_init(DESC *desc, int fd)
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

	desc->writer = wav_write_1u8;
	desc->closer = wav_write_close;

	desc->fd = tmpfd;
	desc->originalfd = fd;
}

/* ***** reader ***** */

static
int
wav_read_1u8(DESC *desc, BUFFER *buf)
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

static
int
wav_read_2u8(DESC *desc, BUFFER *buf)
{
	uint8_t tmp[XP_BUFSIZE * 2];

	int n = readbuf(desc->fd, tmp, (buf->bufsize - buf->length) * 2);
	if (n < 0) {
		return n;
	}
	if (n == 0) {
		return buf->length;
	}

	int count = n / 2;
	uint8_t *s = tmp;
	uint8_t *d = buf->ptr + buf->length;
	for (int i = 0; i < count; i++) {
		uint16_t a, b;
		a = *s++;
		b = *s++;
		*d++ = (a + b) >> 1;
	}
	buf->length += n;
	return buf->length;
}

static
int
wav_read_1s16le(DESC *desc, BUFFER *buf)
{
	uint8_t tmp[XP_BUFSIZE * 2];

	int n = readbuf(desc->fd, tmp, (buf->bufsize - buf->length) * 2);
	if (n < 0) {
		return n;
	}
	if (n == 0) {
		return buf->length;
	}
	int count = n / 2;
	uint8_t *s = tmp + 1;
	uint8_t *d = buf->ptr + buf->length;
	for (int i = 0; i < count; i++) {
		*d++ = (*s) ^ 0x80;
		s += 2;
	}
	buf->length += count;
	return buf->length;
}

static
int
wav_read_2s16le(DESC *desc, BUFFER *buf)
{
	uint8_t tmp[XP_BUFSIZE * 4];

	int n = readbuf(desc->fd, tmp, (buf->bufsize - buf->length) * 4);
	if (n < 0) {
		return n;
	}
	if (n == 0) {
		return buf->length;
	}
	int count = n / 4;
	uint8_t *s = tmp + 1;
	uint8_t *d = buf->ptr + buf->length;
	for (int i = 0; i < count; i++) {
		uint16_t a, b;
		a = (*s) ^ 0x80;
		s += 2;
		b = (*s) ^ 0x80;
		s += 2;
		*d++ = (a + b) >> 1;
	}
	buf->length += count;
	return buf->length;
}

/* ***** writer ***** */
/* support only 1u8 */

static
int
wav_write_1u8(DESC *desc, BUFFER *buf)
{
	int rv = writebuf(desc->fd, buf->ptr, buf->length);
	buf->length = 0;
	return rv;
}

/* ***** closer ***** */

static
int
wav_read_close(DESC *desc)
{
	close(desc->fd);
	return 0;
}

static
int
wav_write_close(DESC *desc)
{
	/*
	 * テンポラリファイルとヘッダを合体させて最終ファイルを作る
	 */

	int fd = desc->originalfd;
	int tmpfd = desc->fd;
	int r;
	int rv = -1;

	int32_t datalen = (int32_t)lseek(desc->fd, 0, SEEK_CUR);

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
	write32le(fd, desc->freq) &&	// freq
	write32le(fd, desc->freq) &&	// data rate (1ch 1byte)
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
		int n = readbuf(tmpfd, buf, sizeof(buf));
		if (n == 0) break;
		if (n < 0) {
			fprintf(stderr, "read: %s\n", strerror(errno));
			goto done;
		}
		if (writebuf(fd, buf, n) < 0) {
			fprintf(stderr, "write: %s\n", strerror(errno));
			goto done;
		}
	}
	rv = 0;

 done:
	close(desc->fd);
	close(desc->originalfd);
	return rv;
}

