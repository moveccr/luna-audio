/* vi: set ts=4: */
/* TODO: LICENSE */

void
conv_u8_pam(BUFFER *dst, BUFFER *src)
{
	int count = src->count;

	uint8_t *s = src->buf;
	uint16_t *d = dst->buf;

	for (int i = 0; i < count; i++) {
		*d++ = PCM2_TABLE[*s++];
	}
}

void
conv_s16BE_pam(BUFFER *dst, BUFFER *src)
{
	int count = src->count;

	uint8_t *s = src->buf;
	uint16_t *d = dst->buf;

	for (int i = 0; i < count; i++) {
		*d++ = PCM2_TABLE[*s ^ 0x80];
		s += 2;
	}
}

void
conv_s16LE_pam(BUFFER *dst, BUFFER *src)
{
	int count = src->count;

	uint8_t *s = src->buf;
	s += 1;
	uint16_t *d = dst->buf;

	for (int i = 0; i < count; i++) {
		*d++ = PCM2_TABLE[*s ^ 0x80];
		s += 2;
	}
}

