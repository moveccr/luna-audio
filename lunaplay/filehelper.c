/* vi: set ts=4: */
/* see LICENSE */ 

#include <errno.h>
#include <unistd.h>
#include <sys/endian.h>

// 可能な限り length バイト読み込む。
// EOF の場合はそれまでに読み込めたバイト数を返す。
// エラーの場合は負数を返す。
int
readbuf(int fd, uint8_t *buf, int length)
{
	int r;
	for (int m = 0; m < length; m += r) {
		r = read(fd, buf + m, length - m);
		if (r < 0) {
			if (errno == EAGAIN) {
				r = 0;
				continue;
			}
			return r;
		}
		if (r == 0) {
			return m;
		}
	}
	return length;
}

// 可能な限り length バイト書き込む。
int
writebuf(int fd, uint8_t *buf, int length)
{
	int r;
	for (int m = 0; m < length; m += r) {
		r = write(fd, buf + m, length - m);
		if (r < 0) {
			if (errno == EAGAIN) {
				r = 0;
				continue;
			}
			return r;
		}
	}
	return length;
}

/* ***** read helper ***** */
int
read32le(int fd, int32_t *rv)
{
	int n = read(fd, rv, 4);
	if (n != 4) return 0;
	*rv = le32toh(*rv);
	return n;
}

int
read16le(int fd, int16_t *rv)
{
	int n = read(fd, rv, 2);
	if (n != 2) return 0;
	*rv = le16toh(*rv);
	return n;
}

int
read8(int fd, int8_t *rv)
{
	int n = read(fd, rv, 1);
	if (n != 1) return 0;
	return n;
}

int
readskip(int fd, int bytes)
{
	int b;
	for (int i = 0; i < bytes; i++) {
		if (read(fd, &b, 1) != 1) {
			return 0;
		}
	}
	return bytes;
}

/* ***** write helper ***** */
int
write32le(int fd, int32_t v)
{
	v = htole32(v);
	int n = write(fd, &v, 4);
	if (n != 4) return 0;
	return n;
}

int
write16le(int fd, int16_t v)
{
	v = htole16(v);
	int n = write(fd, &v, 2);
	if (n != 2) return 0;
	return n;
}

int
write8(int fd, int8_t v)
{
	int n = write(fd, &v, 1);
	if (n != 1) return 0;
	return n;
}

/* ***** tag helper ***** */

// 扱いやすさ優先のため型が非対称です。
int
readtag(int fd, uint32_t *rv)
{
	int n = read(fd, rv, 4);
	if (n != 4) return 0;
	*rv = be32toh(*rv);
	return n;
}

int
writetag(int fd, char strtag[4])
{
	int n = write(fd, strtag, 4);
	if (n != 4) return 0;
	return n;
}

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

