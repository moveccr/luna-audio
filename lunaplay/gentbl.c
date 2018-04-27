/* vi: set ts=4: */
#include <err.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#define countof(x) (sizeof(x) / sizeof((x)[0]))

double VT[16] = {
	0,
	0.0078125,
	1.104854346e-2,
	0.015625,
	2.209708691e-2,
	0.03125,
	4.419417382e-2,
	0.0625,
	8.838834765e-2,
	0.125,
	0.1767766953,
	0.25,
	0.3535533906,
	0.5,
	0.7071067812,
	1,
};

struct PTE {
	double v;
	int a, b, c;
};

struct PTE PAM2[16*16];
struct PTE PAM3[16*16*16];
struct PTE PCM1[16];
struct PTE PCM2[16*16];
struct PTE PCM3[16*16*16];

struct PTO {
	double original;
	double gained;
	struct PTE psg;
};

double
u8_v(uint8_t u)
{
	return (double)u / 255;
}

int
pte_comp_v(const void *a, const void *b)
{
	const struct PTE *x = a;
	const struct PTE *y = b;

	if (x->v < y->v) return -1;
	if (x->v > y->v) return 1;
	return 0;
}

void
pte_sort(struct PTE *pt, int count)
{
	qsort(pt, count, sizeof(struct PTE), pte_comp_v);
}

int
pte_search(struct PTE *pt, int count, double v)
{
	int L, R, C;
	L = 0;
	R = count - 1;

	while (L < R) {
		C = (L + R) / 2;
		double dv = pt[C].v - v;
		if (dv < 0) {
			L = C + 1;
		} else if (dv > 0) {
			R = C - 1;
		} else {
			break;
		}
	}

	int rv = C;
	double min_dv = DBL_MAX;
	L = C - 1;
	if (L < 0) L = 0;
	R = C + 1;
	if (R > count - 1) R = count - 1;
	for (int i = L; i <= R; i++) {

		double dv = fabs(pt[i].v - v);
		if (dv < min_dv) {
			min_dv = dv;
			rv = i;
		}
	}
	return rv;
}

double
pto_stddev(struct PTO *pto, int count)
{
	double sum = 0;
	for (int i = 0; i < count; i++) {
		sum += pto[i].gained - pto[i].psg.v;
	}
	double mean = sum / count;

	sum = 0;
	for (int i = 0; i < count; i++) {
		double d = pto[i].gained - pto[i].psg.v - mean;
		sum += d * d;
	}
	return sqrt(sum / count);
}

void
pto_make(struct PTO *pto, int pto_count,
	struct PTE *table, int table_count,
	double gain, double offset)
{
	for (int i = 0; i < pto_count; i++) {
		double v = u8_v(i);
		double g = v * gain + offset;
		int n = pte_search(table, table_count, g);

		pto[i].original = v;
		pto[i].gained = g;
		pto[i].psg = table[n];
	}
}

int
pto_region(struct PTO *pto, int pto_count, int idx)
{
	int rv = 1;
	double v0 = pto[idx].psg.v;
	for (int i = idx - 1; i >= 0; i--) {
		if (pto[i].psg.v == v0) {
			rv++;
		} else {
			break;
		}
	}
	for (int i = idx + 1; i < pto_count; i++) {
		if (pto[i].psg.v == v0) {
			rv++;
		} else {
			break;
		}
	}
	return rv;
}

// 中央付近の分離度を求めます。小さい方が良好。
double
pto_centerfactor(struct PTO *pto, int pto_count)
{
	return
		(
		pto_region(pto, pto_count, pto_count / 2 - 2) +
		pto_region(pto, pto_count, pto_count / 2 - 1) * 2 +
		pto_region(pto, pto_count, pto_count / 2 + 0) * 2 +
		pto_region(pto, pto_count, pto_count / 2 + 1)
	) / 6.0;
}

int
pto_level(struct PTO *pto, int pto_count)
{
	int rv = 1;
	struct PTE *p = &pto[0].psg;

	for (int i = 1; i < pto_count; i++) {
		if (p->v != pto[i].psg.v) {
			p = &pto[i].psg;
			rv++;
		}
	}
	return rv;
}

void
pto_print(struct PTO *pto, int pto_count,
	const char *table_name,
	int channel_count,
	double gain, double offset)
{
	const char *table_type;

	if (channel_count == 1) {
		table_type = "uint8_t";
	} else if (channel_count == 2) {
		table_type = "uint16_t";
	} else if (channel_count == 3) {
		table_type = "uint32_t";
	} else {
		errx(1, "internal error, invalid channel_count %d", channel_count);
	}

	printf("const %s %s[] = {\n", table_type, table_name);
	printf("\t %*s/* u8 v : gained : out v */\n", channel_count * 2 + 3, "");

	for (int i = 0; i < pto_count; i++) {
		printf("\t");
		if (channel_count == 1) {
			printf("0x%02x, ", pto[i].psg.a);
		} else if (channel_count == 2) {
			printf("0x%04x, ", pto[i].psg.a << 8 | pto[i].psg.b);
		} else {
			printf("0x%06x, ",
				pto[i].psg.a << 16 | pto[i].psg.b << 8 | pto[i].psg.c);
		}
		printf("/* %d %g : %g : %g */\n",
			i,
			pto[i].original,
			pto[i].gained,
			pto[i].psg.v);
	}
	printf("\t/* gain=%g offset=%g */\n", gain, offset);
	printf("\t/* stddev=%g dynamic range=%g */\n",
		pto_stddev(pto, pto_count),
		fabs(pto[0].psg.v - pto[pto_count - 1].psg.v));
	printf("\t/* center-factor=%g */\n",
		pto_centerfactor(pto, pto_count));
	printf("\t/* level=%d */\n",
		pto_level(pto, pto_count));

	printf("};\n");
}

void
pcm1()
{
	for (int i = 0; i < 16; i++) {
		PCM1[i].v = VT[i];
		PCM1[i].a = i;
	}

	pte_sort(PCM1, countof(PCM1));

}

void
pcm2()
{
	for (int a = 0; a < 16; a++) {
		for (int b = 0; b < 16; b++) {
			struct PTE *p = &PCM2[a * 16 + b];
			p->v = VT[a] + VT[b];
			p->a = a;
			p->b = b;
			p->c = 0;
		}
	}

	pte_sort(PCM2, countof(PCM2));
}

void
pcm3()
{
	for (int a = 0; a < 16; a++) {
		for (int b = 0; b < 16; b++) {
			for (int c = 0; c < 16; c++) {
				struct PTE *p = &PCM3[a * 16 * 16 + b * 16 + c];
				p->v = VT[a] + VT[b] + VT[c];
				p->a = a;
				p->b = b;
				p->c = c;
			}
		}
	}

	pte_sort(PCM3, countof(PCM3));
}

static double min_cf = DBL_MAX;
static int max_level = 0;

int
filter_c(struct PTO *pto, int pto_count)
{
	double cf = pto_centerfactor(pto, pto_count);
	if (cf < min_cf) {
		min_cf = cf;
		return -1;
	} else if (cf == min_cf) {
		return 1;
	} else {
		return 0;
	}
}

int
filter_l(struct PTO *pto, int pto_count)
{
	int level = pto_level(pto, pto_count);
	if (level > max_level) {
		max_level = level;
		return -1;
	} else if (level == max_level) {
		return 1;
	} else {
		return 0;
	}
}

int
filter_cl(struct PTO *pto, int pto_count)
{
	int r = filter_c(pto, pto_count);
	if (r) {
		if (r < 0) max_level = 0;
		return filter_l(pto, pto_count);
	}
	return 0;
}

int
filter_lc(struct PTO *pto, int pto_count)
{
	int r = filter_l(pto, pto_count);
	if (r) {
		if (r < 0) min_cf = DBL_MAX;
		return filter_c(pto, pto_count);
	}
	return 0;
}

typedef int (*FILTER)(struct PTO *pto, int pto_count);

int
main(int ac, char *av[])
{
	double gain, offset;
	double gainshift = 1;
	double offsetshift = 1;
	int c;
	int opt_a = 0;
	int opt_m = 1000;
	int opt_n = 10;
	FILTER filter = filter_lc;

	gain = 1;
	offset = 0;

	while ((c = getopt(ac, av, "f:am:n:G:O:g:o:")) != -1) {
		switch (c) {
		 case 'f':
			if (strcasecmp(optarg, "L") == 0) {
				filter = filter_l;
			} else if (strcasecmp(optarg, "C") == 0) {
				filter = filter_c;
			} else if (strcasecmp(optarg, "LC") == 0) {
				filter = filter_lc;
			} else if (strcasecmp(optarg, "CL") == 0) {
				filter = filter_cl;
			} else {
				errx(1, "filter");
			}
			break;
		 case 'a':
			opt_a++;
			break;
		 case 'm':
			opt_m = atoi(optarg);
			break;
		 case 'n':
			opt_n = atoi(optarg);
			break;
		 case 'G':
			gainshift = atof(optarg);
			break;
		 case 'O':
			offsetshift = atof(optarg);
			break;
		 case 'g':
			gain = atof(optarg);
			break;
		 case 'o':
			offset = atof(optarg);
			break;
		 default:
			errx(1, "unknown option");
		}
	}

	char *arg = "PCM1";
	if (optind < ac) {
		arg = av[optind];
	}
	if (opt_m <= 0) {
		errx(1, "m");
	}
	if (opt_n <= 0) {
		errx(1, "n");
	}

	struct PTE *table;
	int count;
	const char *name;
	int channel_count;

	if (strcasecmp(arg, "PCM2") == 0) {
		pcm2();
		table = PCM2;
		count = countof(PCM2);
		name = "PCM2_TABLE";
		channel_count = 2;
	} else if (strcasecmp(arg, "PCM3") == 0) {
		pcm3();
		table = PCM3;
		count = countof(PCM3);
		name = "PCM3_TABLE";
		channel_count = 3;
	} else {
		pcm1();
		table = PCM1;
		count = countof(PCM1);
		name = "PCM1_TABLE";
		channel_count = 1;
	}

	struct PTO pto[256];
	if (opt_a) {
		double best_g, best_o;
		best_g = gain - gainshift;
		best_o = offset - offsetshift;

		for (int i = -opt_m; i <= opt_m; i++) {
			double g = gain + gainshift * i / opt_m;
			for (int j = -opt_n; j <= opt_n; j++) {
				double o = offset + offsetshift * j / opt_n;
				pto_make(pto, countof(pto), table, count, g, o);
				if (filter(pto, countof(pto))) {
					best_g = g;
					best_o = o;
				}
			}
		}
		gain = best_g;
		offset = best_o;
	}

	pto_make(pto, countof(pto), table, count, gain, offset);
	pto_print(pto, countof(pto), name, channel_count, gain, offset);

	return 0;
}

