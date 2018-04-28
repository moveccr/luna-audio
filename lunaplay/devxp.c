/* vi: set ts=4: */

#define XP_DEV	"/dev/xp"

int
xp_write_init(BUFFER *buf)
{
	buf->fd = open(XP_DEV, O_RDWR);
	if (fd == -1) {
		err(EXIT_FAILURE, "open XP device");
	}

	ptr = mmap(NULL, XP_MAX_SIZE, PROT_WRITE | PROT_READ,
		MAP_SHARED, buf->fd, 0);
	if (ptr == MAP_FAILED) {
		err(EXIT_FAILURE, "mmap");
	}

}

int
xp_write(BUFFER *buf)
{
}


/* TODO:
 ファームウェアのロード
 割り込み制御
 カウンタ制御
*/
