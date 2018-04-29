/* vi: set ts=4: */

#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

// XXX: local test now
// TODO: "" to <>
#include "machine/xpio.h"

#include "lunaplay.h"

#define XP_DEV	"/dev/xp"

#define XP_VAR_BASE		0x0100
#define XP_MAGIC		(XP_VAR_BASE + 0)
#define XP_CMD_START	(XP_VAR_BASE + 8)
#define XP_TIMER		(XP_VAR_BASE + 9)
#define XP_ENC			(XP_VAR_BASE + 10)
#define XP_STAT_READY	(XP_VAR_BASE + 11)
#define XP_STAT_ERROR	(XP_VAR_BASE + 12)
#define XP_PAGEENDL		(XP_VAR_BASE + 13)
#define XP_PAGEENDH		(XP_VAR_BASE + 14)

#define XP_FIRMSIZE_MIN	0x0200
#define XP_FIRMSIZE_MAX	0x0fe00

#define XP_MAX_SIZE 0xfe00

volatile uint8_t *xp_ptr;

int xp_curpage;
int xp_isstart;

uint8_t xp_builtin_firmware[] = {
#include "firmware.inc"
};
ssize_t xp_firmware_len = sizeof(xp_builtin_firmware);
uint8_t *xp_firmware = xp_builtin_firmware;

int xp_write(DESC *desc, BUFFER *buf);
int xp_close(DESC *desc);

int
xp_readmem8(int offset)
{
	return xp_ptr[offset];
}

void
xp_writemem8(int offset, int v)
{
	xp_ptr[offset] = v;
}

void
xp_load_firmware(const char *fname)
{
	int fd = open(fname, O_RDONLY);
	if (fd == -1) {
		err(EXIT_FAILURE, "open %s", fname);
	}
	struct stat sb;
	if (fstat(fd, &sb) == -1) {
		err(EXIT_FAILURE, "stat %s", fname);
	}
	if (sb.st_size < XP_FIRMSIZE_MIN || sb.st_size > XP_FIRMSIZE_MAX) {
		errx(EXIT_FAILURE, "invalid firmware size");
	}

	xp_firmware_len = sb.st_size;
	xp_firmware = malloc(xp_firmware_len);
	if (xp_firmware == NULL) {
		err(EXIT_FAILURE, "malloc");
	}
	if (read(fd, xp_firmware, xp_firmware_len) != xp_firmware_len) {
		errx(EXIT_FAILURE, "read error firmware %s", fname);
	}
	if (memcmp(&xp_firmware[XP_MAGIC], "LUNAPSG", 8) != 0) {
		errx(EXIT_FAILURE, "firmware MAGIC error");
	}
	close(fd);
}

int
xp_write_init(DESC *desc)
{
	int r;
	int xpfd;
	struct xp_download xpdl;

	xpfd = open(XP_DEV, O_RDWR);
	if (xpfd == -1) {
		err(EXIT_FAILURE, "open XP device");
	}

	if (opt_firmware != NULL) {
		xp_load_firmware(opt_firmware);
	}

	xpdl.size = xp_firmware_len;
	xpdl.data = xp_firmware;

	r = ioctl(xpfd, XPIOCDOWNLD, &xpdl);
	if (r != 0) {
		err(EXIT_FAILURE, "ioctl XPIOCDOWNLD");
	}

	xp_ptr = mmap(NULL, XP_MAX_SIZE, PROT_WRITE | PROT_READ,
		MAP_SHARED, desc->fd, 0);
	if (xp_ptr == MAP_FAILED) {
		err(EXIT_FAILURE, "mmap");
	}

	// freq to timer
#define XP_CPU_FREQ 6144000
#define XP_TIMER_DIV 20
#define XP_TIMER_BASEFREQ (XP_CPU_FREQ / XP_TIMER_DIV)
	int divisor = XP_TIMER_BASEFREQ / desc->freq;
	if (opt_v) {
		printf("xp freq: %d\n", divisor * XP_TIMER_BASEFREQ);
	}
	if (divisor < 6) {
		// 51.2kHz
		fprintf(stderr, "freq too high: %d\n", desc->freq);
		return -1;
	}
	if (divisor > 77) {
		// 3989Hz
		fprintf(stderr, "freq too low: %d\n", desc->freq);
	}
	int timer = divisor - 1;
	xp_writemem8(XP_TIMER, timer);

	xp_writemem8(XP_ENC, desc->enc);

	xp_curpage = 0;
	xp_isstart = 0;

	desc->fd = xpfd;
	desc->writer = xp_write;
	desc->closer = xp_close;
}

static
void
xp_sleep()
{
}

int
xp_start()
{
	while (xp_readmem8(XP_STAT_READY) != 1) {
		xp_sleep();
	}
	xp_writemem8(XP_CMD_START, 1);
}

int
xp_write(DESC *desc, BUFFER *buf)
{
	int curpagetop = xp_curpage == 0 ? 0x4000 : 0x8000;
	int curpageendH = xp_curpage == 0 ? 0x80 : 0xc0;

	if (xp_isstart) {
		while (xp_readmem8(XP_PAGEENDH) == curpageendH) {
			xp_sleep();
		}
	}

	int n = buf->length;
	memcpy((void*)&xp_ptr[curpagetop], buf->ptr, n);

	if (xp_isstart == 0) {
		xp_start();
	}
	xp_curpage ^= 1;
	buf->length = 0;
	return n;
}

int
xp_close(DESC *desc)
{
	munmap((void*)xp_ptr, XP_MAX_SIZE);
	close(desc->fd);
	return 0;
}


/* TODO:
 割り込み制御
 カウンタ制御
*/
