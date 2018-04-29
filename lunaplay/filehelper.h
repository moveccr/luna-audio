
#pragma once

int readbuf(int fd, uint8_t *buf, int length);
int writebuf(int fd, uint8_t *buf, int length);

// 成功すると!=0 を返します。
// 失敗すると 0 を返します。
int read32le(int fd, int32_t *rv);
int read16le(int fd, int16_t *rv);
int read8(int fd, int8_t *rv);
int readskip(int fd, int bytes);
int write32le(int fd, int32_t v);
int write16le(int fd, int16_t v);
int write8(int fd, int8_t v);

int readtag(int fd, uint32_t *rv);
int writetag(int fd, char strtag[4]);
int cmptag(uint32_t tag, const char strtag[4]);

