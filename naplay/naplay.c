/*
 * COPYRIGHT 2024 @moveccr
 */

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/audioio.h>
#include <sys/ioctl.h>

#define min(a, b) ((a) < (b) ? (a) : (b))

#if defined(DEBUG)
 #define PRINTF	printf
#else
 #define PRINTF(...)
#endif

void
usage()
{
	printf(
"naplay [option] <file>\n"
" -Ln: loop n times. <0 as INF\n"
" file: .au or .wav\n"
);
	exit(1);
}

ssize_t
read_buf(int fd, uint8_t *buf, int buflen)
{
	static uint8_t ibuf[4096];
	static int head;
	static int len;
	int n = 0;

	if (len == 0) {
		n = read(fd, ibuf, sizeof(ibuf));
		if (n < 0) {
			err(EXIT_FAILURE, "read");
		}
		if (n == 0)
			return 0;

		head = 0;
		len = n;
	} else {
		n = len;
	}

	if (buflen < n) {
		n = buflen;
	}

	memcpy(buf, &ibuf[head], n);
	head += n;
	len -= n;

	return n;
}

void
read_exact(int fd, uint8_t *buf, int len)
{
	int n;

	do {
		n = read_buf(fd, buf, len);
		if (n < 0)
			err(EXIT_FAILURE, "read");
		if (n == 0)
			errx(EXIT_FAILURE, "EOF");
		buf += n;
		len -= n;
	} while (len > 0);
}

int32_t
read_be32(int fd)
{
	uint8_t buf[4];

	read_exact(fd, buf, 4);
	return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

int32_t
read_le32(int fd)
{
	uint8_t buf[4];

	read_exact(fd, buf, 4);
	return (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
}

int32_t
read_le16(int fd)
{
	uint8_t buf[2];

	read_exact(fd, buf, 2);
	return (buf[1] << 8) | buf[0];
}




int opt_v;

int
main(int ac, char *av[])
{
	int fd;
	int afd;
	const int buflen = 4096;
	uint8_t buf[buflen];
	struct audio_info info;
	char *filename = NULL;
	int loop = 1;
	off_t start_ofs;

	{
		int i;
		for (i = 1; i < ac; i++) {
			if (av[i][0] == '-') {
				if (av[i][1] == 'L') {
					if (av[i][2]) {
						loop = atoi(&av[i][2]);
					} else if (i + 1 < ac) {
						i += 1;
						loop = atoi(av[i]);
					} else {
						usage();
					}
				}
			} else {
				filename = av[i];
				break;
			}
		}
	}

	if (filename == NULL) {
		usage();
	}

	afd = open("/dev/audio", O_WRONLY);
	if (afd == -1) {
		err(EXIT_FAILURE, "open");
	}

	fd = open(filename, O_RDONLY | O_DIRECT);
	if (fd == -1) {
		err(EXIT_FAILURE, "open");
	}

	memset(buf, 0, buflen);

	AUDIO_INITINFO(&info);

	read_exact(fd, buf, 4);
	if (strncmp(buf, ".snd", 4) == 0) {
		/* au */
		int au_offset = read_be32(fd);
		int au_datasize = read_be32(fd);
		int au_enc = read_be32(fd);
		int au_freq = read_be32(fd);
		int au_ch = read_be32(fd);

		au_offset -= 24;
		while (au_offset > 0) {
			int n = min(au_offset, buflen);
			read_exact(fd, buf, n);
			au_offset -= n;
		}

		switch (au_enc) {
		 case 1:
			info.play.encoding = AUDIO_ENCODING_ULAW;
			info.play.precision = 8;
			PRINTF("enc=ULAW prec=8\n");
			break;
		 default:
			errx(EXIT_FAILURE, "unsupported encoding");
		}
		info.play.sample_rate = au_freq;
		info.play.channels = au_ch;

	} else if (strncmp(buf, "RIFF", 4) ==0) {
		for (;;) {
			read_exact(fd, buf, 4);
			if (strncmp(buf, "fmt ", 4) == 0) {
				break;
			}
		}
		int wav_len = read_le32(fd);
		int wav_fmt = read_le16(fd);
		int wav_ch = read_le16(fd);
		int wav_freq = read_le32(fd);
		int wav_avg = read_le32(fd);
		int wav_bs = read_le16(fd);
		int wav_bits = read_le16(fd);

		for (;;) {
			read_exact(fd, buf, 4);
			if (strncmp(buf, "data", 4) == 0) {
				break;
			}
		}

		if (wav_bits == 8) {
			info.play.encoding = AUDIO_ENCODING_ULINEAR_LE;
			PRINTF("enc=ULINEAR_LE prec=8\n");
		} else {
			info.play.encoding = AUDIO_ENCODING_SLINEAR_LE;
			PRINTF("enc=SLINEAR_LE prec=%d\n", wav_bits);
		}
		info.play.precision = wav_bits;
		info.play.sample_rate = wav_freq;
		info.play.channels = wav_ch;

	} else {
		errx(EXIT_FAILURE, "unsupported");
	}

	PRINTF("rate=%d ch=%d\n", info.play.sample_rate, info.play.channels);

	if (ioctl(afd, AUDIO_SETINFO, &info) < 0) {
		err(EXIT_FAILURE, "AUDIO_SETINFO");
	}

	if (loop < 0 || loop > 1) {
		start_ofs = lseek(fd, 0, SEEK_CUR);
		if (start_ofs < 0) {
			err(EXIT_FAILURE, "lseek");
		}
	}

	while (loop != 0) {
		if (loop < 0 || loop > 1) {
			if (lseek(fd, start_ofs, SEEK_SET) < 0) {
				err(EXIT_FAILURE, "lseek");
			}
		}
		if (loop > 0) {
			loop--;
		}

		for (;;) {
			ssize_t r, w;
			r = read_buf(fd, buf, buflen);
			if (r == 0) break;
			if (r < 0) {
				err(EXIT_FAILURE, "read");
			}
			w = write(afd, buf, r);
			if (w < 0) {
				err(EXIT_FAILURE, "write");
			}
			if (r != w) {
				errx(EXIT_FAILURE, "r != w");
			}
		}
	}

	close(fd);
	close(afd);
	return 0;
}
