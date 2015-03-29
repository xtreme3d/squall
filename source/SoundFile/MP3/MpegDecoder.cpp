//-----------------------------------------------------------------------------
//	Декодер Mpeg Layer 1,2,3
//	Копонент звукового двигателя Шквал
//	команда		: AntiTank
//	разработчик	: Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------

// включения
#include <string.h>
#include <math.h>

#include "MpegDecoder.h"

void CDecompressMpeg::imdct_init()
{
	int k, p, n;
	double t, pi;
	n = 18;
	pi = 4.0 * atan(1.0);
	t = pi / (4 * n);

	for (p = 0; p < n; p++)
		w[p] = (float) (2.0 * cos(t * (2 * p + 1)));

	for (p = 0; p < 9; p++)
		w2[p] = (float) (2.0 * cos(2 * t * (2 * p + 1)));

	t = pi / (2 * n);
	for (k = 0; k < 9; k++) {
		for (p = 0; p < 4; p++)
			coef[k][p] = (float) (cos(t * (2 * k) * (2 * p + 1)));
	}

	n = 6;
	pi = 4.0 * atan(1.0);
	t = pi / (4 * n);

	for (p = 0; p < n; p++)
		v[p] = (float) (2.0 * cos(t * (2 * p + 1)));

	for (p = 0; p < 3; p++)
		v2[p] = (float) (2.0 * cos(2 * t * (2 * p + 1)));

	t = pi / (2 * n);
	k = 1;
	p = 0;
	coef87 = (float) (cos(t * (2 * k) * (2 * p + 1)));

	for (p = 0; p < 6; p++)
		v[p] = v[p] / 2.0f;

	coef87 = (float) (2.0 * coef87);
}

void CDecompressMpeg::imdct18(float f[18])	/* 18 point */
{
	int p;
	float a[9], b[9];
	float ap, bp, a8p, b8p;
	float g1, g2;

	for (p = 0; p < 4; p++) {
		g1 = w[p] * f[p];
		g2 = w[17 - p] * f[17 - p];
		ap = g1 + g2;		// a[p]

		bp = w2[p] * (g1 - g2);	// b[p]

		g1 = w[8 - p] * f[8 - p];
		g2 = w[9 + p] * f[9 + p];
		a8p = g1 + g2;		// a[8-p]

		b8p = w2[8 - p] * (g1 - g2);	// b[8-p]

		a[p] = ap + a8p;
		a[5 + p] = ap - a8p;
		b[p] = bp + b8p;
		b[5 + p] = bp - b8p;
	}

	g1 = w[p] * f[p];
	g2 = w[17 - p] * f[17 - p];
	a[p] = g1 + g2;
	b[p] = w2[p] * (g1 - g2);


	f[0] = 0.5f * (a[0] + a[1] + a[2] + a[3] + a[4]);
	f[1] = 0.5f * (b[0] + b[1] + b[2] + b[3] + b[4]);

	f[2] = coef[1][0] * a[5] +
		coef[1][1] * a[6] +
		coef[1][2] * a[7] +
		coef[1][3] * a[8];
	f[3] = coef[1][0] * b[5] +
		coef[1][1] * b[6] +
		coef[1][2] * b[7] +
		coef[1][3] * b[8] -
		f[1];
	f[1] = f[1] - f[0];
	f[2] = f[2] - f[1];

	f[4] = coef[2][0] * a[0] +
		coef[2][1] * a[1] +
		coef[2][2] * a[2] +
		coef[2][3] * a[3] -
		a[4];
	f[5] = coef[2][0] * b[0] +
		coef[2][1] * b[1] +
		coef[2][2] * b[2] +
		coef[2][3] * b[3] -
		b[4] -
		f[3];
	f[3] = f[3] - f[2];
	f[4] = f[4] - f[3];

	f[6] = coef[3][0] * (a[5] - a[7] - a[8]);
	f[7] = coef[3][0] * (b[5] - b[7] - b[8]) - f[5];
	f[5] = f[5] - f[4];
	f[6] = f[6] - f[5];

	f[8] = coef[4][0] * a[0] +
		coef[4][1] * a[1] +
		coef[4][2] * a[2] +
		coef[4][3] * a[3] +
		a[4];
	f[9] = coef[4][0] * b[0] +
		coef[4][1] * b[1] +
		coef[4][2] * b[2] +
		coef[4][3] * b[3] +
		b[4] -
		f[7];
	f[7] = f[7] - f[6];
	f[8] = f[8] - f[7];

	f[10] = coef[5][0] * a[5] +
		coef[5][1] * a[6] +
		coef[5][2] * a[7] +
		coef[5][3] * a[8];
	f[11] = coef[5][0] * b[5] +
		coef[5][1] * b[6] +
		coef[5][2] * b[7] +
		coef[5][3] * b[8] -
		f[9];
	f[9] = f[9] - f[8];
	f[10] = f[10] - f[9];

	f[12] = 0.5f * (a[0] + a[2] + a[3]) - a[1] - a[4];
	f[13] = 0.5f * (b[0] + b[2] + b[3]) - b[1] - b[4] - f[11];
	f[11] = f[11] - f[10];
	f[12] = f[12] - f[11];

	f[14] = coef[7][0] * a[5] +
		coef[7][1] * a[6] +
		coef[7][2] * a[7] +
		coef[7][3] * a[8];
	f[15] = coef[7][0] * b[5] +
		coef[7][1] * b[6] +
		coef[7][2] * b[7] +
		coef[7][3] * b[8] -
		f[13];
	f[13] = f[13] - f[12];
	f[14] = f[14] - f[13];

	f[16] = coef[8][0] * a[0] +
		coef[8][1] * a[1] +
		coef[8][2] * a[2] +
		coef[8][3] * a[3] +
		a[4];
	f[17] = coef[8][0] * b[0] +
		coef[8][1] * b[1] +
		coef[8][2] * b[2] +
		coef[8][3] * b[3] +
		b[4] -
		f[15];
	f[15] = f[15] - f[14];
	f[16] = f[16] - f[15];
	f[17] = f[17] - f[16];
}

/*--------------------------------------------------------------------*/
/* does 3, 6 pt dct.  changes order from f[i][window] c[window][i] */
void CDecompressMpeg::imdct6_3(float f[])	/* 6 point */
{
	int w;
	float buf[18];
	float* a,* c;		// b[i] = a[3+i]

	float g1, g2;
	float a02, b02;

	c = f;
	a = buf;
	for (w = 0; w < 3; w++) {
		g1 = v[0] * f[3 * 0];
		g2 = v[5] * f[3 * 5];
		a[0] = g1 + g2;
		a[3 + 0] = v2[0] * (g1 - g2);

		g1 = v[1] * f[3 * 1];
		g2 = v[4] * f[3 * 4];
		a[1] = g1 + g2;
		a[3 + 1] = v2[1] * (g1 - g2);

		g1 = v[2] * f[3 * 2];
		g2 = v[3] * f[3 * 3];
		a[2] = g1 + g2;
		a[3 + 2] = v2[2] * (g1 - g2);

		a += 6;
		f++;
	}

	a = buf;
	for (w = 0; w < 3; w++) {
		a02 = (a[0] + a[2]);
		b02 = (a[3 + 0] + a[3 + 2]);
		c[0] = a02 + a[1];
		c[1] = b02 + a[3 + 1];
		c[2] = coef87 * (a[0] - a[2]);
		c[3] = coef87 * (a[3 + 0] - a[3 + 2]) - c[1];
		c[1] = c[1] - c[0];
		c[2] = c[2] - c[1];
		c[4] = a02 - a[1] - a[1];
		c[5] = b02 - a[3 + 1] - a[3 + 1] - c[3];
		c[3] = c[3] - c[2];
		c[4] = c[4] - c[3];
		c[5] = c[5] - c[4];
		a += 6;
		c += 6;
	}
}


void CDecompressMpeg::fdct_init()		/* gen coef for N=32 (31 coefs) */
{
	int p, n, i, k;
	double t, pi;

	pi = 4.0 * atan(1.0);
	n = 16;
	k = 0;
	for (i = 0; i < 5; i++, n = n / 2) {
		for (p = 0; p < n; p++, k++) {
			t = (pi / (4 * n)) * (2 * p + 1);
			coef32[k] = (float) (0.50 / cos(t));
		}
	}
}

void CDecompressMpeg::forward_bf(int m, int n, float x[], float f[],
	float coef[])
{
	int i, j, n2;
	int p, q, p0, k;

	p0 = 0;
	n2 = n >> 1;
	for (i = 0; i < m; i++, p0 += n) {
		k = 0;
		p = p0;
		q = p + n - 1;
		for (j = 0; j < n2; j++, p++, q--, k++) {
			f[p] = x[p] + x[q];
			f[n2 + p] = coef[k] * (x[p] - x[q]);
		}
	}
}

void CDecompressMpeg::back_bf(int m, int n, float x[], float f[])
{
	int i, j, n2, n21;
	int p, q, p0;

	p0 = 0;
	n2 = n >> 1;
	n21 = n2 - 1;
	for (i = 0; i < m; i++, p0 += n) {
		p = p0;
		q = p0;
		for (j = 0; j < n2; j++, p += 2, q++)
			f[p] = x[q];
		p = p0 + 1;
		for (j = 0; j < n21; j++, p += 2, q++)
			f[p] = x[q] + x[q + 1];
		f[p] = x[q];
	}
}

void CDecompressMpeg::fdct32(float x[], float c[])
{
	float a[32];			/* ping pong buffers */
	float b[32];
	int p, q;

	// если эквалайзер включен занести значения
	/*	if (m_enableEQ) {
			for (p = 0; p < 32; p++)
				x[p] *= m_equalizer[p];
		}*/
	/* special first stage */
	for (p = 0, q = 31; p < 16; p++, q--) {
		a[p] = x[p] + x[q];
		a[16 + p] = coef32[p] * (x[p] - x[q]);
	}

	forward_bf(2, 16, a, b, coef32 + 16);
	forward_bf(4, 8, b, a, coef32 + 16 + 8);
	forward_bf(8, 4, a, b, coef32 + 16 + 8 + 4);
	forward_bf(16, 2, b, a, coef32 + 16 + 8 + 4 + 2);
	back_bf(8, 4, a, b);
	back_bf(4, 8, b, a);
	back_bf(2, 16, a, b);
	back_bf(1, 32, b, c);
}

void CDecompressMpeg::fdct32_dual(float x[], float c[])
{
	float a[32];			/* ping pong buffers */
	float b[32];
	int p, pp, qq;

	/*	if (m_enableEQ) {
			for (p = 0; p < 32; p++)
				x[p] *= m_equalizer[p];
		}*/

	/* special first stage for dual chan (interleaved x) */
	pp = 0;
	qq = 2 * 31;
	for (p = 0; p < 16; p++, pp += 2, qq -= 2) {
		a[p] = x[pp] + x[qq];
		a[16 + p] = coef32[p] * (x[pp] - x[qq]);
	}
	forward_bf(2, 16, a, b, coef32 + 16);
	forward_bf(4, 8, b, a, coef32 + 16 + 8);
	forward_bf(8, 4, a, b, coef32 + 16 + 8 + 4);
	forward_bf(16, 2, b, a, coef32 + 16 + 8 + 4 + 2);
	back_bf(8, 4, a, b);
	back_bf(4, 8, b, a);
	back_bf(2, 16, a, b);
	back_bf(1, 32, b, c);
}

void CDecompressMpeg::fdct32_dual_mono(float x[], float c[])
{
	float a[32];			/* ping pong buffers */
	float b[32];
	float t1, t2;
	int p, pp, qq;

	/* special first stage  */
	pp = 0;
	qq = 2 * 31;
	for (p = 0; p < 16; p++, pp += 2, qq -= 2) {
		t1 = 0.5F * (x[pp] + x[pp + 1]);
		t2 = 0.5F * (x[qq] + x[qq + 1]);
		a[p] = t1 + t2;
		a[16 + p] = coef32[p] * (t1 - t2);
	}
	forward_bf(2, 16, a, b, coef32 + 16);
	forward_bf(4, 8, b, a, coef32 + 16 + 8);
	forward_bf(8, 4, a, b, coef32 + 16 + 8 + 4);
	forward_bf(16, 2, b, a, coef32 + 16 + 8 + 4 + 2);
	back_bf(8, 4, a, b);
	back_bf(4, 8, b, a);
	back_bf(2, 16, a, b);
	back_bf(1, 32, b, c);
}

void CDecompressMpeg::fdct16(float x[], float c[])
{
	float a[16];			/* ping pong buffers */
	float b[16];
	int p, q;

	/* special first stage (drop highest sb) */
	a[0] = x[0];
	a[8] = coef32[16] * x[0];
	for (p = 1, q = 14; p < 8; p++, q--) {
		a[p] = x[p] + x[q];
		a[8 + p] = coef32[16 + p] * (x[p] - x[q]);
	}
	forward_bf(2, 8, a, b, coef32 + 16 + 8);
	forward_bf(4, 4, b, a, coef32 + 16 + 8 + 4);
	forward_bf(8, 2, a, b, coef32 + 16 + 8 + 4 + 2);
	back_bf(4, 4, b, a);
	back_bf(2, 8, a, b);
	back_bf(1, 16, b, c);
}

void CDecompressMpeg::fdct16_dual(float x[], float c[])
{
	float a[16];			/* ping pong buffers */
	float b[16];
	int p, pp, qq;

	/* special first stage for interleaved input */
	a[0] = x[0];
	a[8] = coef32[16] * x[0];
	pp = 2;
	qq = 2 * 14;
	for (p = 1; p < 8; p++, pp += 2, qq -= 2) {
		a[p] = x[pp] + x[qq];
		a[8 + p] = coef32[16 + p] * (x[pp] - x[qq]);
	}
	forward_bf(2, 8, a, b, coef32 + 16 + 8);
	forward_bf(4, 4, b, a, coef32 + 16 + 8 + 4);
	forward_bf(8, 2, a, b, coef32 + 16 + 8 + 4 + 2);
	back_bf(4, 4, b, a);
	back_bf(2, 8, a, b);
	back_bf(1, 16, b, c);
}

void CDecompressMpeg::fdct16_dual_mono(float x[], float c[])
{
	float a[16];			/* ping pong buffers */
	float b[16];
	float t1, t2;
	int p, pp, qq;

	/* special first stage  */
	a[0] = 0.5F * (x[0] + x[1]);
	a[8] = coef32[16] * a[0];
	pp = 2;
	qq = 2 * 14;
	for (p = 1; p < 8; p++, pp += 2, qq -= 2) {
		t1 = 0.5F * (x[pp] + x[pp + 1]);
		t2 = 0.5F * (x[qq] + x[qq + 1]);
		a[p] = t1 + t2;
		a[8 + p] = coef32[16 + p] * (t1 - t2);
	}
	forward_bf(2, 8, a, b, coef32 + 16 + 8);
	forward_bf(4, 4, b, a, coef32 + 16 + 8 + 4);
	forward_bf(8, 2, a, b, coef32 + 16 + 8 + 4 + 2);
	back_bf(4, 4, b, a);
	back_bf(2, 8, a, b);
	back_bf(1, 16, b, c);
}

void CDecompressMpeg::fdct8(float x[], float c[])
{
	float a[8];			/* ping pong buffers */
	float b[8];
	int p, q;

	/* special first stage  */

	b[0] = x[0] + x[7];
	b[4] = coef32[16 + 8] * (x[0] - x[7]);
	for (p = 1, q = 6; p < 4; p++, q--) {
		b[p] = x[p] + x[q];
		b[4 + p] = coef32[16 + 8 + p] * (x[p] - x[q]);
	}

	forward_bf(2, 4, b, a, coef32 + 16 + 8 + 4);
	forward_bf(4, 2, a, b, coef32 + 16 + 8 + 4 + 2);
	back_bf(2, 4, b, a);
	back_bf(1, 8, a, c);
}

void CDecompressMpeg::fdct8_dual(float x[], float c[])
{
	float a[8];			/* ping pong buffers */
	float b[8];
	int p, pp, qq;

	/* special first stage for interleaved input */
	b[0] = x[0] + x[14];
	b[4] = coef32[16 + 8] * (x[0] - x[14]);
	pp = 2;
	qq = 2 * 6;
	for (p = 1; p < 4; p++, pp += 2, qq -= 2) {
		b[p] = x[pp] + x[qq];
		b[4 + p] = coef32[16 + 8 + p] * (x[pp] - x[qq]);
	}
	forward_bf(2, 4, b, a, coef32 + 16 + 8 + 4);
	forward_bf(4, 2, a, b, coef32 + 16 + 8 + 4 + 2);
	back_bf(2, 4, b, a);
	back_bf(1, 8, a, c);
}

void CDecompressMpeg::fdct8_dual_mono(float x[], float c[])
{
	float a[8];			/* ping pong buffers */
	float b[8];
	float t1, t2;
	int p, pp, qq;

	/* special first stage  */
	t1 = 0.5F * (x[0] + x[1]);
	t2 = 0.5F * (x[14] + x[15]);
	b[0] = t1 + t2;
	b[4] = coef32[16 + 8] * (t1 - t2);
	pp = 2;
	qq = 2 * 6;
	for (p = 1; p < 4; p++, pp += 2, qq -= 2) {
		t1 = 0.5F * (x[pp] + x[pp + 1]);
		t2 = 0.5F * (x[qq] + x[qq + 1]);
		b[p] = t1 + t2;
		b[4 + p] = coef32[16 + 8 + p] * (t1 - t2);
	}
	forward_bf(2, 4, b, a, coef32 + 16 + 8 + 4);
	forward_bf(4, 2, a, b, coef32 + 16 + 8 + 4 + 2);
	back_bf(2, 4, b, a);
	back_bf(1, 8, a, c);
}

void CDecompressMpeg::bitget_init(unsigned char* buf)
{
	bs_ptr0 = bs_ptr = buf;
	bits = 0;
	bitbuf = 0;
}

int CDecompressMpeg::bitget(int n)
{
	unsigned int x;

	if (bits < n) {
		/* refill bit buf if necessary */
		while (bits <= 24) {
			bitbuf = (bitbuf << 8) | *bs_ptr++;
			bits += 8;
		}
	}
	bits -= n;
	x = bitbuf >> bits;
	bitbuf -= x << bits;
	return x;
}

void CDecompressMpeg::bitget_skip(int n)
{
	unsigned int k;

	if (bits < n) {
		n -= bits;
		k = n >> 3;
		/*--- bytes = n/8 --*/
		bs_ptr += k;
		n -= k << 3;
		bitbuf = *bs_ptr++;
		bits = 8;
	}
	bits -= n;
	bitbuf -= (bitbuf >> bits) << bits;
}

void CDecompressMpeg::bitget_init_end(unsigned char* buf_end)
{
	bs_ptr_end = buf_end;
}

int CDecompressMpeg::bitget_overrun()
{
	return bs_ptr > bs_ptr_end;
}

int CDecompressMpeg::bitget_bits_used()
{
	unsigned int n;

	n = ((bs_ptr - bs_ptr0) << 3) - bits;
	return n;
}

void CDecompressMpeg::bitget_check(int n)
{
	if (bits < n) {
		while (bits <= 24) {
			bitbuf = (bitbuf << 8) | *bs_ptr++;
			bits += 8;
		}
	}
}

/* only huffman */

/*----- get n bits  - checks for n+2 avail bits (linbits+sign) -----*/
int CDecompressMpeg::bitget_lb(int n)
{
	unsigned int x;

	if (bits < (n + 2)) {
		/* refill bit buf if necessary */
		while (bits <= 24) {
			bitbuf = (bitbuf << 8) | *bs_ptr++;
			bits += 8;
		}
	}
	bits -= n;
	x = bitbuf >> bits;
	bitbuf -= x << bits;
	return x;
}

/*------------- get n bits but DO NOT remove from bitstream --*/
int CDecompressMpeg::bitget2(int n)
{
	unsigned int x;

	if (bits < (MAXBITS + 2)) {
		/* refill bit buf if necessary */
		while (bits <= 24) {
			bitbuf = (bitbuf << 8) | *bs_ptr++;
			bits += 8;
		}
	}
	x = bitbuf >> (bits - n);
	return x;
}

/*------------- remove n bits from bitstream ---------*/
void CDecompressMpeg::bitget_purge(int n)
{
	bits -= n;
	bitbuf -= (bitbuf >> bits) << bits;
}

void CDecompressMpeg::mac_bitget_check(int n)
{
	if (bits < n) {
		while (bits <= 24) {
			bitbuf = (bitbuf << 8) | *bs_ptr++;
			bits += 8;
		}
	}
}

int CDecompressMpeg::mac_bitget(int n)
{
	unsigned int code;

	bits -= n;
	code = bitbuf >> bits;
	bitbuf -= code << bits;
	return code;
}

int CDecompressMpeg::mac_bitget2(int n)
{
	return (bitbuf >> (bits - n));
}

int CDecompressMpeg::mac_bitget_1bit()
{
	unsigned int code;

	bits--;
	code = bitbuf >> bits;
	bitbuf -= code << bits;
	return code;
}

void CDecompressMpeg::mac_bitget_purge(int n)
{
	bits -= n;
	bitbuf -= (bitbuf >> bits) << bits;
}

void CDecompressMpeg::windowB(float* vbuf, int vb_ptr, unsigned char* pcm)
{
	int i, j;
	int si, bx;
	float* coef;
	float sum;
	long tmp;

	si = vb_ptr + 16;
	bx = (si + 32) & 511;
	coef = wincoef;

	/*-- first 16 --*/
	for (i = 0; i < 16; i++) {
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef++) * vbuf[si];
			si = (si + 64) & 511;
			sum -= (*coef++) * vbuf[bx];
			bx = (bx + 64) & 511;
		}
		si++;
		bx--;
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm++ = ((unsigned char) (tmp >> 8)) ^ 0x80;
	}
	/*--  special case --*/
	sum = 0.0F;
	for (j = 0; j < 8; j++) {
		sum += (*coef++) * vbuf[bx];
		bx = (bx + 64) & 511;
	}
	tmp = (long) sum;
	if (tmp > 32767)
		tmp = 32767;
	else if (tmp < -32768)
		tmp = -32768;
	*pcm++ = ((unsigned char) (tmp >> 8)) ^ 0x80;
	/*-- last 15 --*/
	coef = wincoef + 255;	/* back pass through coefs */
	for (i = 0; i < 15; i++) {
		si--;
		bx++;
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef--) * vbuf[si];
			si = (si + 64) & 511;
			sum += (*coef--) * vbuf[bx];
			bx = (bx + 64) & 511;
		}
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm++ = ((unsigned char) (tmp >> 8)) ^ 0x80;
	}
}

void CDecompressMpeg::windowB_dual(float* vbuf, int vb_ptr, unsigned char* pcm)
{
	int i, j;			/* dual window interleaves output */
	int si, bx;
	float* coef;
	float sum;
	long tmp;

	si = vb_ptr + 16;
	bx = (si + 32) & 511;
	coef = wincoef;

	/*-- first 16 --*/
	for (i = 0; i < 16; i++) {
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef++) * vbuf[si];
			si = (si + 64) & 511;
			sum -= (*coef++) * vbuf[bx];
			bx = (bx + 64) & 511;
		}
		si++;
		bx--;
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm = ((unsigned char) (tmp >> 8)) ^ 0x80;
		pcm += 2;
	}
	/*--  special case --*/
	sum = 0.0F;
	for (j = 0; j < 8; j++) {
		sum += (*coef++) * vbuf[bx];
		bx = (bx + 64) & 511;
	}
	tmp = (long) sum;
	if (tmp > 32767)
		tmp = 32767;
	else if (tmp < -32768)
		tmp = -32768;
	*pcm = ((unsigned char) (tmp >> 8)) ^ 0x80;
	pcm += 2;
	/*-- last 15 --*/
	coef = wincoef + 255;	/* back pass through coefs */
	for (i = 0; i < 15; i++) {
		si--;
		bx++;
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef--) * vbuf[si];
			si = (si + 64) & 511;
			sum += (*coef--) * vbuf[bx];
			bx = (bx + 64) & 511;
		}
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm = ((unsigned char) (tmp >> 8)) ^ 0x80;
		pcm += 2;
	}
}

void CDecompressMpeg::windowB16(float* vbuf, int vb_ptr, unsigned char* pcm)
{
	int i, j;
	unsigned char si, bx;
	float* coef;
	float sum;
	long tmp;

	si = vb_ptr + 8;
	bx = si + 16;
	coef = wincoef;

	/*-- first 8 --*/
	for (i = 0; i < 8; i++) {
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef++) * vbuf[si];
			si += 32;
			sum -= (*coef++) * vbuf[bx];
			bx += 32;
		}
		si++;
		bx--;
		coef += 16;
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm++ = ((unsigned char) (tmp >> 8)) ^ 0x80;
	}
	/*--  special case --*/
	sum = 0.0F;
	for (j = 0; j < 8; j++) {
		sum += (*coef++) * vbuf[bx];
		bx += 32;
	}
	tmp = (long) sum;
	if (tmp > 32767)
		tmp = 32767;
	else if (tmp < -32768)
		tmp = -32768;
	*pcm++ = ((unsigned char) (tmp >> 8)) ^ 0x80;
	/*-- last 7 --*/
	coef = wincoef + 255;	/* back pass through coefs */
	for (i = 0; i < 7; i++) {
		coef -= 16;
		si--;
		bx++;
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef--) * vbuf[si];
			si += 32;
			sum += (*coef--) * vbuf[bx];
			bx += 32;
		}
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm++ = ((unsigned char) (tmp >> 8)) ^ 0x80;
	}
}

void CDecompressMpeg::windowB16_dual(float* vbuf, int vb_ptr,
	unsigned char* pcm)
{
	int i, j;
	unsigned char si, bx;
	float* coef;
	float sum;
	long tmp;

	si = vb_ptr + 8;
	bx = si + 16;
	coef = wincoef;

	/*-- first 8 --*/
	for (i = 0; i < 8; i++) {
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef++) * vbuf[si];
			si += 32;
			sum -= (*coef++) * vbuf[bx];
			bx += 32;
		}
		si++;
		bx--;
		coef += 16;
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm = ((unsigned char) (tmp >> 8)) ^ 0x80;
		pcm += 2;
	}
	/*--  special case --*/
	sum = 0.0F;
	for (j = 0; j < 8; j++) {
		sum += (*coef++) * vbuf[bx];
		bx += 32;
	}
	tmp = (long) sum;
	if (tmp > 32767)
		tmp = 32767;
	else if (tmp < -32768)
		tmp = -32768;
	*pcm = ((unsigned char) (tmp >> 8)) ^ 0x80;
	pcm += 2;
	/*-- last 7 --*/
	coef = wincoef + 255;	/* back pass through coefs */
	for (i = 0; i < 7; i++) {
		coef -= 16;
		si--;
		bx++;
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef--) * vbuf[si];
			si += 32;
			sum += (*coef--) * vbuf[bx];
			bx += 32;
		}
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm = ((unsigned char) (tmp >> 8)) ^ 0x80;
		pcm += 2;
	}
}

void CDecompressMpeg::windowB8(float* vbuf, int vb_ptr, unsigned char* pcm)
{
	int i, j;
	int si, bx;
	float* coef;
	float sum;
	long tmp;

	si = vb_ptr + 4;
	bx = (si + 8) & 127;
	coef = wincoef;

	/*-- first 4 --*/
	for (i = 0; i < 4; i++) {
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef++) * vbuf[si];
			si = (si + 16) & 127;
			sum -= (*coef++) * vbuf[bx];
			bx = (bx + 16) & 127;
		}
		si++;
		bx--;
		coef += 48;
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm++ = ((unsigned char) (tmp >> 8)) ^ 0x80;
	}
	/*--  special case --*/
	sum = 0.0F;
	for (j = 0; j < 8; j++) {
		sum += (*coef++) * vbuf[bx];
		bx = (bx + 16) & 127;
	}
	tmp = (long) sum;
	if (tmp > 32767)
		tmp = 32767;
	else if (tmp < -32768)
		tmp = -32768;
	*pcm++ = ((unsigned char) (tmp >> 8)) ^ 0x80;
	/*-- last 3 --*/
	coef = wincoef + 255;	/* back pass through coefs */
	for (i = 0; i < 3; i++) {
		coef -= 48;
		si--;
		bx++;
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef--) * vbuf[si];
			si = (si + 16) & 127;
			sum += (*coef--) * vbuf[bx];
			bx = (bx + 16) & 127;
		}
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm++ = ((unsigned char) (tmp >> 8)) ^ 0x80;
	}
}
/*--------------- 8 pt dual window (interleaved output) -----------------*/
void CDecompressMpeg::windowB8_dual(float* vbuf, int vb_ptr,
	unsigned char* pcm)
{
	int i, j;
	int si, bx;
	float* coef;
	float sum;
	long tmp;

	si = vb_ptr + 4;
	bx = (si + 8) & 127;
	coef = wincoef;

	/*-- first 4 --*/
	for (i = 0; i < 4; i++) {
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef++) * vbuf[si];
			si = (si + 16) & 127;
			sum -= (*coef++) * vbuf[bx];
			bx = (bx + 16) & 127;
		}
		si++;
		bx--;
		coef += 48;
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm = ((unsigned char) (tmp >> 8)) ^ 0x80;
		pcm += 2;
	}
	/*--  special case --*/
	sum = 0.0F;
	for (j = 0; j < 8; j++) {
		sum += (*coef++) * vbuf[bx];
		bx = (bx + 16) & 127;
	}
	tmp = (long) sum;
	if (tmp > 32767)
		tmp = 32767;
	else if (tmp < -32768)
		tmp = -32768;
	*pcm = ((unsigned char) (tmp >> 8)) ^ 0x80;
	pcm += 2;
	/*-- last 3 --*/
	coef = wincoef + 255;	/* back pass through coefs */
	for (i = 0; i < 3; i++) {
		coef -= 48;
		si--;
		bx++;
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef--) * vbuf[si];
			si = (si + 16) & 127;
			sum += (*coef--) * vbuf[bx];
			bx = (bx + 16) & 127;
		}
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm = ((unsigned char) (tmp >> 8)) ^ 0x80;
		pcm += 2;
	}
}

void CDecompressMpeg::sbtB_mono(float* sample, void* in_pcm, int n)
{
	int i;
	unsigned char * pcm = (unsigned char *) in_pcm;

	for (i = 0; i < n; i++) {
		fdct32(sample, vbuf + vb_ptr);
		windowB(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 32) & 511;
		pcm += 32;
	}
}

void CDecompressMpeg::sbtB_dual(float* sample, void* in_pcm, int n)
{
	int i;
	unsigned char * pcm = (unsigned char *) in_pcm;

	for (i = 0; i < n; i++) {
		fdct32_dual(sample, vbuf + vb_ptr);
		fdct32_dual(sample + 1, vbuf2 + vb_ptr);
		windowB_dual(vbuf, vb_ptr, pcm);
		windowB_dual(vbuf2, vb_ptr, pcm + 1);
		sample += 64;
		vb_ptr = (vb_ptr - 32) & 511;
		pcm += 64;
	}
}

void CDecompressMpeg::sbtB_dual_mono(float* sample, void* in_pcm, int n)
{
	int i;
	unsigned char * pcm = (unsigned char *) in_pcm;

	for (i = 0; i < n; i++) {
		fdct32_dual_mono(sample, vbuf + vb_ptr);
		windowB(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 32) & 511;
		pcm += 32;
	}
}

void CDecompressMpeg::sbtB_dual_left(float* sample, void* in_pcm, int n)
{
	int i;
	unsigned char * pcm = (unsigned char *) in_pcm;

	for (i = 0; i < n; i++) {
		fdct32_dual(sample, vbuf + vb_ptr);
		windowB(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 32) & 511;
		pcm += 32;
	}
}

void CDecompressMpeg::sbtB_dual_right(float* sample, void* in_pcm, int n)
{
	int i;
	unsigned char * pcm = (unsigned char *) in_pcm;

	sample++;			/* point to right chan */
	for (i = 0; i < n; i++) {
		fdct32_dual(sample, vbuf + vb_ptr);
		windowB(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 32) & 511;
		pcm += 32;
	}
}

void CDecompressMpeg::sbtB16_mono(float* sample, void* in_pcm, int n)
{
	int i;
	unsigned char * pcm = (unsigned char *) in_pcm;

	for (i = 0; i < n; i++) {
		fdct16(sample, vbuf + vb_ptr);
		windowB16(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 16) & 255;
		pcm += 16;
	}
}

void CDecompressMpeg::sbtB16_dual(float* sample, void* in_pcm, int n)
{
	int i;
	unsigned char * pcm = (unsigned char *) in_pcm;

	for (i = 0; i < n; i++) {
		fdct16_dual(sample, vbuf + vb_ptr);
		fdct16_dual(sample + 1, vbuf2 + vb_ptr);
		windowB16_dual(vbuf, vb_ptr, pcm);
		windowB16_dual(vbuf2, vb_ptr, pcm + 1);
		sample += 64;
		vb_ptr = (vb_ptr - 16) & 255;
		pcm += 32;
	}
}

void CDecompressMpeg::sbtB16_dual_mono(float* sample, void* in_pcm, int n)
{
	int i;
	unsigned char * pcm = (unsigned char *) in_pcm;

	for (i = 0; i < n; i++) {
		fdct16_dual_mono(sample, vbuf + vb_ptr);
		windowB16(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 16) & 255;
		pcm += 16;
	}
}

void CDecompressMpeg::sbtB16_dual_left(float* sample, void* in_pcm, int n)
{
	int i;
	unsigned char * pcm = (unsigned char *) in_pcm;

	for (i = 0; i < n; i++) {
		fdct16_dual(sample, vbuf + vb_ptr);
		windowB16(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 16) & 255;
		pcm += 16;
	}
}

void CDecompressMpeg::sbtB16_dual_right(float* sample, void* in_pcm, int n)
{
	int i;
	unsigned char * pcm = (unsigned char *) in_pcm;

	sample++;
	for (i = 0; i < n; i++) {
		fdct16_dual(sample, vbuf + vb_ptr);
		windowB16(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 16) & 255;
		pcm += 16;
	}
}

void CDecompressMpeg::sbtB8_mono(float* sample, void* in_pcm, int n)
{
	int i;
	unsigned char * pcm = (unsigned char *) in_pcm;

	for (i = 0; i < n; i++) {
		fdct8(sample, vbuf + vb_ptr);
		windowB8(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 8) & 127;
		pcm += 8;
	}
}

void CDecompressMpeg::sbtB8_dual(float* sample, void* in_pcm, int n)
{
	int i;
	unsigned char * pcm = (unsigned char *) in_pcm;

	for (i = 0; i < n; i++) {
		fdct8_dual(sample, vbuf + vb_ptr);
		fdct8_dual(sample + 1, vbuf2 + vb_ptr);
		windowB8_dual(vbuf, vb_ptr, pcm);
		windowB8_dual(vbuf2, vb_ptr, pcm + 1);
		sample += 64;
		vb_ptr = (vb_ptr - 8) & 127;
		pcm += 16;
	}
}

void CDecompressMpeg::sbtB8_dual_mono(float* sample, void* in_pcm, int n)
{
	int i;
	unsigned char * pcm = (unsigned char *) in_pcm;

	for (i = 0; i < n; i++) {
		fdct8_dual_mono(sample, vbuf + vb_ptr);
		windowB8(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 8) & 127;
		pcm += 8;
	}
}

void CDecompressMpeg::sbtB8_dual_left(float* sample, void* in_pcm, int n)
{
	int i;
	unsigned char * pcm = (unsigned char *) in_pcm;

	for (i = 0; i < n; i++) {
		fdct8_dual(sample, vbuf + vb_ptr);
		windowB8(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 8) & 127;
		pcm += 8;
	}
}

void CDecompressMpeg::sbtB8_dual_right(float* sample, void* in_pcm, int n)
{
	int i;
	unsigned char * pcm = (unsigned char *) in_pcm;

	sample++;
	for (i = 0; i < n; i++) {
		fdct8_dual(sample, vbuf + vb_ptr);
		windowB8(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 8) & 127;
		pcm += 8;
	}
}

void CDecompressMpeg::sbtB_mono_L3(float* sample, void* in_pcm, int ch)
{
	int i;
	unsigned char * pcm = (unsigned char *) in_pcm;

	ch = 0;
	for (i = 0; i < 18; i++) {
		fdct32(sample, vbuf + vb_ptr);
		windowB(vbuf, vb_ptr, pcm);
		sample += 32;
		vb_ptr = (vb_ptr - 32) & 511;
		pcm += 32;
	}
}

void CDecompressMpeg::sbtB_dual_L3(float* sample, void* in_pcm, int ch)
{
	int i;
	unsigned char * pcm = (unsigned char *) in_pcm;

	if (ch == 0)
		for (i = 0; i < 18; i++) {
			fdct32(sample, vbuf + vb_ptr);
			windowB_dual(vbuf, vb_ptr, pcm);
			sample += 32;
			vb_ptr = (vb_ptr - 32) & 511;
			pcm += 64;
		}
	else
		for (i = 0; i < 18; i++) {
			fdct32(sample, vbuf2 + vb2_ptr);
			windowB_dual(vbuf2, vb2_ptr, pcm + 1);
			sample += 32;
			vb2_ptr = (vb2_ptr - 32) & 511;
			pcm += 64;
		}
}

void CDecompressMpeg::sbtB16_mono_L3(float* sample, void* in_pcm, int ch)
{
	int i;
	unsigned char * pcm = (unsigned char *) in_pcm;

	ch = 0;
	for (i = 0; i < 18; i++) {
		fdct16(sample, vbuf + vb_ptr);
		windowB16(vbuf, vb_ptr, pcm);
		sample += 32;
		vb_ptr = (vb_ptr - 16) & 255;
		pcm += 16;
	}
}

void CDecompressMpeg::sbtB16_dual_L3(float* sample, void* in_pcm, int ch)
{
	int i;
	unsigned char * pcm = (unsigned char *) in_pcm;

	if (ch == 0) {
		for (i = 0; i < 18; i++) {
			fdct16(sample, vbuf + vb_ptr);
			windowB16_dual(vbuf, vb_ptr, pcm);
			sample += 32;
			vb_ptr = (vb_ptr - 16) & 255;
			pcm += 32;
		}
	} else {
		for (i = 0; i < 18; i++) {
			fdct16(sample, vbuf2 + vb2_ptr);
			windowB16_dual(vbuf2, vb2_ptr, pcm + 1);
			sample += 32;
			vb2_ptr = (vb2_ptr - 16) & 255;
			pcm += 32;
		}
	}
}

void CDecompressMpeg::sbtB8_mono_L3(float* sample, void* in_pcm, int ch)
{
	int i;
	unsigned char * pcm = (unsigned char *) in_pcm;

	ch = 0;
	for (i = 0; i < 18; i++) {
		fdct8(sample, vbuf + vb_ptr);
		windowB8(vbuf, vb_ptr, pcm);
		sample += 32;
		vb_ptr = (vb_ptr - 8) & 127;
		pcm += 8;
	}
}

void CDecompressMpeg::sbtB8_dual_L3(float* sample, void* in_pcm, int ch)
{
	int i;
	unsigned char * pcm = (unsigned char *) in_pcm;

	if (ch == 0) {
		for (i = 0; i < 18; i++) {
			fdct8(sample, vbuf + vb_ptr);
			windowB8_dual(vbuf, vb_ptr, pcm);
			sample += 32;
			vb_ptr = (vb_ptr - 8) & 127;
			pcm += 16;
		}
	} else {
		for (i = 0; i < 18; i++) {
			fdct8(sample, vbuf2 + vb2_ptr);
			windowB8_dual(vbuf2, vb2_ptr, pcm + 1);
			sample += 32;
			vb2_ptr = (vb2_ptr - 8) & 127;
			pcm += 16;
		}
	}
}

// window coefs
float CDecompressMpeg::wincoef[264] = {
	0.000000000f, 0.000442505f, -0.003250122f, 0.007003784f, -0.031082151f,
	0.078628540f, -0.100311279f, 0.572036743f, -1.144989014f, -0.572036743f,
	-0.100311279f, -0.078628540f, -0.031082151f, -0.007003784f, -0.003250122f,
	-0.000442505f, 0.000015259f, 0.000473022f, -0.003326416f, 0.007919312f,
	-0.030517576f, 0.084182739f, -0.090927124f, 0.600219727f, -1.144287109f,
	-0.543823242f, -0.108856201f, -0.073059082f, -0.031478882f, -0.006118774f,
	-0.003173828f, -0.000396729f, 0.000015259f, 0.000534058f, -0.003387451f,
	0.008865356f, -0.029785154f, 0.089706421f, -0.080688477f, 0.628295898f,
	-1.142211914f, -0.515609741f, -0.116577141f, -0.067520142f, -0.031738281f,
	-0.005294800f, -0.003082275f, -0.000366211f, 0.000015259f, 0.000579834f,
	-0.003433228f, 0.009841919f, -0.028884888f, 0.095169067f, -0.069595337f,
	0.656219482f, -1.138763428f, -0.487472534f, -0.123474121f, -0.061996460f,
	-0.031845093f, -0.004486084f, -0.002990723f, -0.000320435f, 0.000015259f,
	0.000625610f, -0.003463745f, 0.010848999f, -0.027801514f, 0.100540161f,
	-0.057617184f, 0.683914185f, -1.133926392f, -0.459472656f, -0.129577637f,
	-0.056533810f, -0.031814575f, -0.003723145f, -0.002899170f, -0.000289917f,
	0.000015259f, 0.000686646f, -0.003479004f, 0.011886597f, -0.026535034f,
	0.105819702f, -0.044784546f, 0.711318970f, -1.127746582f, -0.431655884f,
	-0.134887695f, -0.051132202f, -0.031661987f, -0.003005981f, -0.002792358f,
	-0.000259399f, 0.000015259f, 0.000747681f, -0.003479004f, 0.012939452f,
	-0.025085449f, 0.110946655f, -0.031082151f, 0.738372803f, -1.120223999f,
	-0.404083252f, -0.139450073f, -0.045837402f, -0.031387329f, -0.002334595f,
	-0.002685547f, -0.000244141f, 0.000030518f, 0.000808716f, -0.003463745f,
	0.014022826f, -0.023422241f, 0.115921021f, -0.016510010f, 0.765029907f,
	-1.111373901f, -0.376800537f, -0.143264771f, -0.040634155f, -0.031005858f,
	-0.001693726f, -0.002578735f, -0.000213623f, 0.000030518f, 0.000885010f,
	-0.003417969f, 0.015121460f, -0.021575928f, 0.120697014f, -0.001068115f,
	0.791213989f, -1.101211548f, -0.349868774f, -0.146362305f, -0.035552979f,
	-0.030532837f, -0.001098633f, -0.002456665f, -0.000198364f, 0.000030518f,
	0.000961304f, -0.003372192f, 0.016235352f, -0.019531250f, 0.125259399f,
	0.015228271f, 0.816864014f, -1.089782715f, -0.323318481f, -0.148773193f,
	-0.030609131f, -0.029937742f, -0.000549316f, -0.002349854f, -0.000167847f,
	0.000030518f, 0.001037598f, -0.003280640f, 0.017349243f, -0.017257690f,
	0.129562378f, 0.032379150f, 0.841949463f, -1.077117920f, -0.297210693f,
	-0.150497437f, -0.025817871f, -0.029281614f, -0.000030518f, -0.002243042f,
	-0.000152588f, 0.000045776f, 0.001113892f, -0.003173828f, 0.018463135f,
	-0.014801024f, 0.133590698f, 0.050354004f, 0.866363525f, -1.063217163f,
	-0.271591187f, -0.151596069f, -0.021179199f, -0.028533936f, 0.000442505f,
	-0.002120972f, -0.000137329f, 0.000045776f, 0.001205444f, -0.003051758f,
	0.019577026f, -0.012115479f, 0.137298584f, 0.069168091f, 0.890090942f,
	-1.048156738f, -0.246505737f, -0.152069092f, -0.016708374f, -0.027725220f,
	0.000869751f, -0.002014160f, -0.000122070f, 0.000061035f, 0.001296997f,
	-0.002883911f, 0.020690918f, -0.009231566f, 0.140670776f, 0.088775635f,
	0.913055420f, -1.031936646f, -0.221984863f, -0.151962280f, -0.012420653f,
	-0.026840210f, 0.001266479f, -0.001907349f, -0.000106812f, 0.000061035f,
	0.001388550f, -0.002700806f, 0.021789551f, -0.006134033f, 0.143676758f,
	0.109161377f, 0.935195923f, -1.014617920f, -0.198059082f, -0.151306152f,
	-0.008316040f, -0.025909424f, 0.001617432f, -0.001785278f, -0.000106812f,
	0.000076294f, 0.001480103f, -0.002487183f, 0.022857666f, -0.002822876f,
	0.146255493f, 0.130310059f, 0.956481934f, -0.996246338f, -0.174789429f,
	-0.150115967f, -0.004394531f, -0.024932859f, 0.001937866f, -0.001693726f,
	-0.000091553f, -0.001586914f, -0.023910521f, -0.148422241f, -0.976852417f,
	0.152206421f, 0.000686646f, -0.002227783f, 0.000076294f, 
};

void CDecompressMpeg::window(float* vbuf, int vb_ptr, short* pcm)
{
	int i, j;
	int si, bx;
	float* coef;
	float sum;
	long tmp;

	si = vb_ptr + 16;
	bx = (si + 32) & 511;
	coef = wincoef;

	/*-- first 16 --*/
	for (i = 0; i < 16; i++) {
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef++) * vbuf[si];
			si = (si + 64) & 511;
			sum -= (*coef++) * vbuf[bx];
			bx = (bx + 64) & 511;
		}
		si++;
		bx--;
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm++ = (short) tmp;
	}
	/*--  special case --*/
	sum = 0.0F;
	for (j = 0; j < 8; j++) {
		sum += (*coef++) * vbuf[bx];
		bx = (bx + 64) & 511;
	}
	tmp = (long) sum;
	if (tmp > 32767)
		tmp = 32767;
	else if (tmp < -32768)
		tmp = -32768;
	*pcm++ = (short) tmp;
	/*-- last 15 --*/
	coef = wincoef + 255;	/* back pass through coefs */
	for (i = 0; i < 15; i++) {
		si--;
		bx++;
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef--) * vbuf[si];
			si = (si + 64) & 511;
			sum += (*coef--) * vbuf[bx];
			bx = (bx + 64) & 511;
		}
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm++ = (short) tmp;
	}
}

void CDecompressMpeg::window_dual(float* vbuf, int vb_ptr, short* pcm)
{
	int i, j;			/* dual window interleaves output */
	int si, bx;
	float* coef;
	float sum;
	long tmp;

	si = vb_ptr + 16;
	bx = (si + 32) & 511;
	coef = wincoef;

	/*-- first 16 --*/
	for (i = 0; i < 16; i++) {
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef++) * vbuf[si];
			si = (si + 64) & 511;
			sum -= (*coef++) * vbuf[bx];
			bx = (bx + 64) & 511;
		}
		si++;
		bx--;
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm = (short) tmp;
		pcm += 2;
	}
	/*--  special case --*/
	sum = 0.0F;
	for (j = 0; j < 8; j++) {
		sum += (*coef++) * vbuf[bx];
		bx = (bx + 64) & 511;
	}
	tmp = (long) sum;
	if (tmp > 32767)
		tmp = 32767;
	else if (tmp < -32768)
		tmp = -32768;
	*pcm = (short) tmp;
	pcm += 2;
	/*-- last 15 --*/
	coef = wincoef + 255;	/* back pass through coefs */
	for (i = 0; i < 15; i++) {
		si--;
		bx++;
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef--) * vbuf[si];
			si = (si + 64) & 511;
			sum += (*coef--) * vbuf[bx];
			bx = (bx + 64) & 511;
		}
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm = (short) tmp;
		pcm += 2;
	}
}

void CDecompressMpeg::window16(float* vbuf, int vb_ptr, short* pcm)
{
	int i, j;
	unsigned char si, bx;
	float* coef;
	float sum;
	long tmp;

	si = vb_ptr + 8;
	bx = si + 16;
	coef = wincoef;

	/*-- first 8 --*/
	for (i = 0; i < 8; i++) {
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef++) * vbuf[si];
			si += 32;
			sum -= (*coef++) * vbuf[bx];
			bx += 32;
		}
		si++;
		bx--;
		coef += 16;
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm++ = (short) tmp;
	}
	/*--  special case --*/
	sum = 0.0F;
	for (j = 0; j < 8; j++) {
		sum += (*coef++) * vbuf[bx];
		bx += 32;
	}
	tmp = (long) sum;
	if (tmp > 32767)
		tmp = 32767;
	else if (tmp < -32768)
		tmp = -32768;
	*pcm++ = (short) tmp;
	/*-- last 7 --*/
	coef = wincoef + 255;	/* back pass through coefs */
	for (i = 0; i < 7; i++) {
		coef -= 16;
		si--;
		bx++;
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef--) * vbuf[si];
			si += 32;
			sum += (*coef--) * vbuf[bx];
			bx += 32;
		}
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm++ = (short) tmp;
	}
}

void CDecompressMpeg::window16_dual(float* vbuf, int vb_ptr, short* pcm)
{
	int i, j;
	unsigned char si, bx;
	float* coef;
	float sum;
	long tmp;

	si = vb_ptr + 8;
	bx = si + 16;
	coef = wincoef;

	/*-- first 8 --*/
	for (i = 0; i < 8; i++) {
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef++) * vbuf[si];
			si += 32;
			sum -= (*coef++) * vbuf[bx];
			bx += 32;
		}
		si++;
		bx--;
		coef += 16;
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm = (short) tmp;
		pcm += 2;
	}
	/*--  special case --*/
	sum = 0.0F;
	for (j = 0; j < 8; j++) {
		sum += (*coef++) * vbuf[bx];
		bx += 32;
	}
	tmp = (long) sum;
	if (tmp > 32767)
		tmp = 32767;
	else if (tmp < -32768)
		tmp = -32768;
	*pcm = (short) tmp;
	pcm += 2;
	/*-- last 7 --*/
	coef = wincoef + 255;	/* back pass through coefs */
	for (i = 0; i < 7; i++) {
		coef -= 16;
		si--;
		bx++;
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef--) * vbuf[si];
			si += 32;
			sum += (*coef--) * vbuf[bx];
			bx += 32;
		}
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm = (short) tmp;
		pcm += 2;
	}
}

void CDecompressMpeg::window8(float* vbuf, int vb_ptr, short* pcm)
{
	int i, j;
	int si, bx;
	float* coef;
	float sum;
	long tmp;

	si = vb_ptr + 4;
	bx = (si + 8) & 127;
	coef = wincoef;

	/*-- first 4 --*/
	for (i = 0; i < 4; i++) {
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef++) * vbuf[si];
			si = (si + 16) & 127;
			sum -= (*coef++) * vbuf[bx];
			bx = (bx + 16) & 127;
		}
		si++;
		bx--;
		coef += 48;
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm++ = (short) tmp;
	}
	/*--  special case --*/
	sum = 0.0F;
	for (j = 0; j < 8; j++) {
		sum += (*coef++) * vbuf[bx];
		bx = (bx + 16) & 127;
	}
	tmp = (long) sum;
	if (tmp > 32767)
		tmp = 32767;
	else if (tmp < -32768)
		tmp = -32768;
	*pcm++ = (short) tmp;
	/*-- last 3 --*/
	coef = wincoef + 255;	/* back pass through coefs */
	for (i = 0; i < 3; i++) {
		coef -= 48;
		si--;
		bx++;
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef--) * vbuf[si];
			si = (si + 16) & 127;
			sum += (*coef--) * vbuf[bx];
			bx = (bx + 16) & 127;
		}
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm++ = (short) tmp;
	}
}

void CDecompressMpeg::window8_dual(float* vbuf, int vb_ptr, short* pcm)
{
	int i, j;
	int si, bx;
	float* coef;
	float sum;
	long tmp;

	si = vb_ptr + 4;
	bx = (si + 8) & 127;
	coef = wincoef;

	/*-- first 4 --*/
	for (i = 0; i < 4; i++) {
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef++) * vbuf[si];
			si = (si + 16) & 127;
			sum -= (*coef++) * vbuf[bx];
			bx = (bx + 16) & 127;
		}
		si++;
		bx--;
		coef += 48;
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm = (short) tmp;
		pcm += 2;
	}
	/*--  special case --*/
	sum = 0.0F;
	for (j = 0; j < 8; j++) {
		sum += (*coef++) * vbuf[bx];
		bx = (bx + 16) & 127;
	}
	tmp = (long) sum;
	if (tmp > 32767)
		tmp = 32767;
	else if (tmp < -32768)
		tmp = -32768;
	*pcm = (short) tmp;
	pcm += 2;
	/*-- last 3 --*/
	coef = wincoef + 255;	/* back pass through coefs */
	for (i = 0; i < 3; i++) {
		coef -= 48;
		si--;
		bx++;
		sum = 0.0F;
		for (j = 0; j < 8; j++) {
			sum += (*coef--) * vbuf[si];
			si = (si + 16) & 127;
			sum += (*coef--) * vbuf[bx];
			bx = (bx + 16) & 127;
		}
		tmp = (long) sum;
		if (tmp > 32767)
			tmp = 32767;
		else if (tmp < -32768)
			tmp = -32768;
		*pcm = (short) tmp;
		pcm += 2;
	}
}

void CDecompressMpeg::sbt_init()
{
	int i;

	/* clear window vbuf */
	for (i = 0; i < 512; i++) {
		vbuf[i] = 0.0F;
		vbuf2[i] = 0.0F;
	}
	vb2_ptr = vb_ptr = 0;
}

void CDecompressMpeg::sbt_mono(float* sample, void* in_pcm, int n)
{
	int i;
	short* pcm = (short*) in_pcm;

	for (i = 0; i < n; i++) {
		fdct32(sample, vbuf + vb_ptr);
		window(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 32) & 511;
		pcm += 32;
	}
}

void CDecompressMpeg::sbt_dual(float* sample, void* in_pcm, int n)
{
	int i;
	short* pcm = (short*) in_pcm;


	for (i = 0; i < n; i++) {
		fdct32_dual(sample, vbuf + vb_ptr);
		fdct32_dual(sample + 1, vbuf2 + vb_ptr);
		window_dual(vbuf, vb_ptr, pcm);
		window_dual(vbuf2, vb_ptr, pcm + 1);
		sample += 64;
		vb_ptr = (vb_ptr - 32) & 511;
		pcm += 64;
	}
}

void CDecompressMpeg::sbt_dual_mono(float* sample, void* in_pcm, int n)
{
	int i;
	short* pcm = (short*) in_pcm;

	for (i = 0; i < n; i++) {
		fdct32_dual_mono(sample, vbuf + vb_ptr);
		window(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 32) & 511;
		pcm += 32;
	}
}

void CDecompressMpeg::sbt_dual_left(float* sample, void* in_pcm, int n)
{
	int i;
	short* pcm = (short*) in_pcm;

	for (i = 0; i < n; i++) {
		fdct32_dual(sample, vbuf + vb_ptr);
		window(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 32) & 511;
		pcm += 32;
	}
}

void CDecompressMpeg::sbt_dual_right(float* sample, void* in_pcm, int n)
{
	int i;
	short* pcm = (short*) in_pcm;

	sample++;			/* point to right chan */
	for (i = 0; i < n; i++) {
		fdct32_dual(sample, vbuf + vb_ptr);
		window(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 32) & 511;
		pcm += 32;
	}
}

void CDecompressMpeg::sbt16_mono(float* sample, void* in_pcm, int n)
{
	int i;
	short* pcm = (short*) in_pcm;

	for (i = 0; i < n; i++) {
		fdct16(sample, vbuf + vb_ptr);
		window16(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 16) & 255;
		pcm += 16;
	}
}

void CDecompressMpeg::sbt16_dual(float* sample, void* in_pcm, int n)
{
	int i;
	short* pcm = (short*) in_pcm;

	for (i = 0; i < n; i++) {
		fdct16_dual(sample, vbuf + vb_ptr);
		fdct16_dual(sample + 1, vbuf2 + vb_ptr);
		window16_dual(vbuf, vb_ptr, pcm);
		window16_dual(vbuf2, vb_ptr, pcm + 1);
		sample += 64;
		vb_ptr = (vb_ptr - 16) & 255;
		pcm += 32;
	}
}

void CDecompressMpeg::sbt16_dual_mono(float* sample, void* in_pcm, int n)
{
	int i;
	short* pcm = (short*) in_pcm;

	for (i = 0; i < n; i++) {
		fdct16_dual_mono(sample, vbuf + vb_ptr);
		window16(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 16) & 255;
		pcm += 16;
	}
}

void CDecompressMpeg::sbt16_dual_left(float* sample, void* in_pcm, int n)
{
	int i;
	short* pcm = (short*) in_pcm;

	for (i = 0; i < n; i++) {
		fdct16_dual(sample, vbuf + vb_ptr);
		window16(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 16) & 255;
		pcm += 16;
	}
}

void CDecompressMpeg::sbt16_dual_right(float* sample, void* in_pcm, int n)
{
	int i;
	short* pcm = (short*) in_pcm;

	sample++;
	for (i = 0; i < n; i++) {
		fdct16_dual(sample, vbuf + vb_ptr);
		window16(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 16) & 255;
		pcm += 16;
	}
}

void CDecompressMpeg::sbt8_mono(float* sample, void* in_pcm, int n)
{
	int i;
	short* pcm = (short*) in_pcm;

	for (i = 0; i < n; i++) {
		fdct8(sample, vbuf + vb_ptr);
		window8(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 8) & 127;
		pcm += 8;
	}
}

void CDecompressMpeg::sbt8_dual(float* sample, void* in_pcm, int n)
{
	int i;
	short* pcm = (short*) in_pcm;

	for (i = 0; i < n; i++) {
		fdct8_dual(sample, vbuf + vb_ptr);
		fdct8_dual(sample + 1, vbuf2 + vb_ptr);
		window8_dual(vbuf, vb_ptr, pcm);
		window8_dual(vbuf2, vb_ptr, pcm + 1);
		sample += 64;
		vb_ptr = (vb_ptr - 8) & 127;
		pcm += 16;
	}
}

void CDecompressMpeg::sbt8_dual_mono(float* sample, void* in_pcm, int n)
{
	int i;
	short* pcm = (short*) in_pcm;

	for (i = 0; i < n; i++) {
		fdct8_dual_mono(sample, vbuf + vb_ptr);
		window8(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 8) & 127;
		pcm += 8;
	}
}

void CDecompressMpeg::sbt8_dual_left(float* sample, void* in_pcm, int n)
{
	int i;
	short* pcm = (short*) in_pcm;

	for (i = 0; i < n; i++) {
		fdct8_dual(sample, vbuf + vb_ptr);
		window8(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 8) & 127;
		pcm += 8;
	}
}

void CDecompressMpeg::sbt8_dual_right(float* sample, void* in_pcm, int n)
{
	int i;
	short* pcm = (short*) in_pcm;

	sample++;
	for (i = 0; i < n; i++) {
		fdct8_dual(sample, vbuf + vb_ptr);
		window8(vbuf, vb_ptr, pcm);
		sample += 64;
		vb_ptr = (vb_ptr - 8) & 127;
		pcm += 8;
	}
}

void CDecompressMpeg::sbt_mono_L3(float* sample, void* in_pcm, int ch)
{
	int i;
	short* pcm = (short*) in_pcm;

	ch = 0;
	for (i = 0; i < 18; i++) {
		fdct32(sample, vbuf + vb_ptr);
		window(vbuf, vb_ptr, pcm);
		sample += 32;
		vb_ptr = (vb_ptr - 32) & 511;
		pcm += 32;
	}
}

void CDecompressMpeg::sbt_dual_L3(float* sample, void* in_pcm, int ch)
{
	int i;
	short* pcm = (short*) in_pcm;

	if (ch == 0)
		for (i = 0; i < 18; i++) {
			fdct32(sample, vbuf + vb_ptr);
			window_dual(vbuf, vb_ptr, pcm);
			sample += 32;
			vb_ptr = (vb_ptr - 32) & 511;
			pcm += 64;
		}
	else
		for (i = 0; i < 18; i++) {
			fdct32(sample, vbuf2 + vb2_ptr);
			window_dual(vbuf2, vb2_ptr, pcm + 1);
			sample += 32;
			vb2_ptr = (vb2_ptr - 32) & 511;
			pcm += 64;
		}
}

void CDecompressMpeg::sbt16_mono_L3(float* sample, void* in_pcm, int ch)
{
	int i;
	short* pcm = (short*) in_pcm;

	ch = 0;
	for (i = 0; i < 18; i++) {
		fdct16(sample, vbuf + vb_ptr);
		window16(vbuf, vb_ptr, pcm);
		sample += 32;
		vb_ptr = (vb_ptr - 16) & 255;
		pcm += 16;
	}
}

void CDecompressMpeg::sbt16_dual_L3(float* sample, void* in_pcm, int ch)
{
	int i;
	short* pcm = (short*) in_pcm;


	if (ch == 0) {
		for (i = 0; i < 18; i++) {
			fdct16(sample, vbuf + vb_ptr);
			window16_dual(vbuf, vb_ptr, pcm);
			sample += 32;
			vb_ptr = (vb_ptr - 16) & 255;
			pcm += 32;
		}
	} else {
		for (i = 0; i < 18; i++) {
			fdct16(sample, vbuf2 + vb2_ptr);
			window16_dual(vbuf2, vb2_ptr, pcm + 1);
			sample += 32;
			vb2_ptr = (vb2_ptr - 16) & 255;
			pcm += 32;
		}
	}
}

void CDecompressMpeg::sbt8_mono_L3(float* sample, void* in_pcm, int ch)
{
	int i;
	short* pcm = (short*) in_pcm;

	ch = 0;
	for (i = 0; i < 18; i++) {
		fdct8(sample, vbuf + vb_ptr);
		window8(vbuf, vb_ptr, pcm);
		sample += 32;
		vb_ptr = (vb_ptr - 8) & 127;
		pcm += 8;
	}
}

void CDecompressMpeg::sbt8_dual_L3(float* sample, void* in_pcm, int ch)
{
	int i;
	short* pcm = (short*) in_pcm;

	if (ch == 0) {
		for (i = 0; i < 18; i++) {
			fdct8(sample, vbuf + vb_ptr);
			window8_dual(vbuf, vb_ptr, pcm);
			sample += 32;
			vb_ptr = (vb_ptr - 8) & 127;
			pcm += 16;
		}
	} else {
		for (i = 0; i < 18; i++) {
			fdct8(sample, vbuf2 + vb2_ptr);
			window8_dual(vbuf2, vb2_ptr, pcm + 1);
			sample += 32;
			vb2_ptr = (vb2_ptr - 8) & 127;
			pcm += 16;
		}
	}
}

int CDecompressMpeg::br_tbl[3][3][16] = {
	{// MPEG-1
	// Layer1
	{ 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0 },
	// Layer2
	{ 0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0 },
	// Layer3
	{ 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0 },
	}, {// MPEG-2
	// Layer1
	{ 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0 },
	// Layer2
	{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 },
	// Layer3
	{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 },
	}, {// MPEG-2.5
	// Layer1 (not available)
	{ 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0 },
	// Layer2 (not available)
	{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 },
	// Layer3
	{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 },
	}, 
};

int CDecompressMpeg::fr_tbl[3][4] = {
	{	44100,	48000,	32000,	0	},	// MPEG-1
	{	22050,	24000,	16000,	0	},	// MPEG-2
	{	11025,	12000,	8000,	0	},	// MPEG-2.5

};

void CDecompressMpeg::mp3DecodeInit()
{
	m_option.reduction = 0;
	m_option.convert = 0;
	m_option.freqLimit = 24000;

	L1table_init();
	L2table_init();
	L3table_init();
}

int CDecompressMpeg::mp3GetHeader(BYTE* buf, MPEG_HEADER* h)
{
	h->version = (buf[1] & 0x08) >> 3;
	h->layer = (buf[1] & 0x06) >> 1;
	h->error_prot = (buf[1] & 0x01);
	h->br_index = (buf[2] & 0xf0) >> 4;
	h->fr_index = (buf[2] & 0x0c) >> 2;
	h->padding = (buf[2] & 0x02) >> 1;
	h->extension = (buf[2] & 0x01);
	h->mode = (buf[3] & 0xc0) >> 6;
	h->mode_ext = (buf[3] & 0x30) >> 4;
	h->copyright = (buf[3] & 0x08) >> 3;
	h->original = (buf[3] & 0x04) >> 2;
	h->emphasis = (buf[3] & 0x03);

	if (buf[0] != 0xFF) {
		//sync error
		m_last_error = MP3_ERROR_INVALID_SYNC;
		return 0;
	}
	if ((buf[1] & 0xF0) == 0xF0)		//MPEG-1, MPEG-2
		h->version = (h->version) ? 1 : 2;
	else if ((buf[1] & 0xF0) == 0xE0)	//MPEG-2.5
		h->version = 3;
	else {
		m_last_error = MP3_ERROR_INVALID_SYNC;
		return 0;
	}
	if (h->fr_index >= 3 ||
		h->br_index == 0 ||
		h->br_index >= 15 ||
		h->layer == 0 ||
		h->layer >= 4) {
		m_last_error = MP3_ERROR_INVALID_HEADER;
		return 0;
	}
	h->layer = 4 - h->layer;
	h->error_prot = (h->error_prot) ? 0 : 1;

	return 1;
}

bool CDecompressMpeg::mp3GetHeaderInfo(BYTE* buffer, MPEG_HEADER_INFO* info)
{
	int ch, ver;
	MPEG_HEADER* h =& info->header;
	// получим информацию из заголовка
	if (!mp3GetHeader(buffer, h))
		return false;

	// расчет нужных данных
	info->curBitRate = br_tbl[h->version - 1][h->layer - 1][h->br_index] * 1000;
	switch (h->layer) {
	case 1:
		//layer1
		info->curFrameSize = (12 * info->curBitRate / m_frequency + h->padding) * 4;
		break;
	case 2:
		//layer2
		info->curFrameSize = 144 * info->curBitRate /
			m_frequency +
			h->padding;
		break;
	case 3:
		//layer3
		if (h->version == 1)
			info->curFrameSize = 144 * info->curBitRate /
				m_frequency +
				h->padding;
		else
			info->curFrameSize = (144 * info->curBitRate / m_frequency) /
				2 +
				h->padding;
		break;
	}
	ch = (h->mode == 3) ? 1 : 2;
	ver = (h->version == 1) ? 1 : 2;
	info->samplesInFrame = (1152 >> m_option.reduction) / ver;
	info->outputSize = info->samplesInFrame * 2 * ch;
	return true;
}


int CDecompressMpeg::mp3GetLastError()
{
	return m_last_error;
}

int CDecompressMpeg::mp3FindSync(BYTE* buf, int size, int* sync)
{
	int i;
	MPEG_HEADER h;

	*sync = 0;
	size -= 3;
	if (size <= 0) {
		m_last_error = MP3_ERROR_OUT_OF_BUFFER;
		return 0;
	}

	// поиск данных
	for (i = 0; i < size; i++) {
		if (buf[i] == 0xFF) {
			if (mp3GetHeader(buf + i, & h)) {
				if ((h.layer == _layer) &&
					(h.version == _version) &&
					(h.br_index == _br_index) &&
					(h.fr_index == _fr_index) &&
					(h.mode == _mode))
					break;
			}
		}
	}

	if (i == size) {
		m_last_error = MP3_ERROR_OUT_OF_BUFFER;
		return 0;
	}

	*sync = i;
	return 1;
}

void CDecompressMpeg::mp3GetDecodeOption(MPEG_DECODE_OPTION* option)
{
	*option = m_option;
}

int CDecompressMpeg::mp3SetDecodeOption(MPEG_DECODE_OPTION* option)
{
	m_option = *option;
	return 1;
}
/*
//-----------------------------------------------------------------------------
//	Установка эквалайзера
//	value	- указатель на параметры эквалайзера
//-----------------------------------------------------------------------------
int CDecompressMpeg::mp3SetEqualizer(int* value)
{
	int i;
	if (value == (void*)0) {
		m_enableEQ = 0;
		return 1;
	}
	m_enableEQ = 1;
	//60, 170, 310, 600, 1K, 3K
	for (i = 0; i < 6; i ++) {
		m_equalizer[i] = (float)pow(10,(double)value[i]/200);
	}
	//6K
	m_equalizer[6] = (float)pow(10,(double)value[6]/200);
	m_equalizer[7] = m_equalizer[6];
	//12K
	m_equalizer[8] = (float)pow(10,(double)value[7]/200);
	m_equalizer[9] = m_equalizer[8];
	m_equalizer[10] = m_equalizer[8];
	m_equalizer[11] = m_equalizer[8];
	//14K
	m_equalizer[12] = (float)pow(10,(double)value[8]/200);
	m_equalizer[13] = m_equalizer[12];
	m_equalizer[14] = m_equalizer[12];
	m_equalizer[15] = m_equalizer[12];
	m_equalizer[16] = m_equalizer[12];
	m_equalizer[17] = m_equalizer[12];
	m_equalizer[18] = m_equalizer[12];
	m_equalizer[19] = m_equalizer[12];
	//16K
	m_equalizer[20] = (float)pow(10,(double)value[9]/200);
	m_equalizer[21] = m_equalizer[20];
	m_equalizer[22] = m_equalizer[20];
	m_equalizer[23] = m_equalizer[20];
	m_equalizer[24] = m_equalizer[20];
	m_equalizer[25] = m_equalizer[20];
	m_equalizer[26] = m_equalizer[20];
	m_equalizer[27] = m_equalizer[20];
	m_equalizer[28] = m_equalizer[20];
	m_equalizer[29] = m_equalizer[20];
	m_equalizer[30] = m_equalizer[20];
	m_equalizer[31] = m_equalizer[20];
	return 1;
}
*/
#define VBR_FRAMES_FLAG		0x0001
#define VBR_BYTES_FLAG		0x0002
#define VBR_TOC_FLAG		0x0004
#define VBR_SCALE_FLAG		0x0008

// big endian extract
int CDecompressMpeg::extractInt4(BYTE* buf)
{
	return buf[3] | (buf[2] << 8) | (buf[1] << 16) | (buf[0] << 24);
}
//-----------------------------------------------------------------------------
// извленение заголовка и важных данных
//	mpeg	- указатель на буфер с данными
//	size	- размер буфера с данными
//	info	- указатель на структуру куда поместить расширенные данные
//	decFlag	- ? помоему использовать настройки частоты из файла
//-----------------------------------------------------------------------------
int CDecompressMpeg::mp3GetDecodeInfo(BYTE* mpeg, int size,
	MPEG_DECODE_INFO* info, int decFlag)
{
	MPEG_HEADER* h =& info->header;
	byte* p = mpeg;
	int vbr;
	DWORD minBitRate, maxBitRate;
	DWORD i, j, flags;

	//int bitRate;
	//int frame_size;

	//	if (size < 156) {//max vbr header size
	//		m_last_error = MP3_ERROR_OUT_OF_BUFFER;
	//		return 0;
	//	}
	if (!mp3GetHeader(p, h)) {
		return 0;
	}
	//check VBR Header
	p += 4;//skip mpeg header
	if (h->error_prot)
		p += 2;//skip crc
	if (h->layer == 3) {
		//skip side info
		if (h->version == 1) {
			//MPEG-1
			if (h->mode != 3)
				p += 32;
			else
				p += 17;
		} else {
			//MPEG-2, MPEG-2.5
			if (h->mode != 3)
				p += 17;
			else
				p += 9;
		}
	}

	info->bitRate = br_tbl[h->version - 1][h->layer - 1][h->br_index] * 1000;
	info->frequency = fr_tbl[h->version - 1][h->fr_index];
	if (memcmp(p, "Xing", 4) == 0) {
		//VBR
		p += 4;
		flags = extractInt4(p);
		p += 4;
		if (!(flags & (VBR_FRAMES_FLAG | VBR_BYTES_FLAG))) {
			m_last_error = MP3_ERROR_INVALID_HEADER;
			return 0;
		}
		info->frames = extractInt4(p);
		p += 4;
		info->dataSize = extractInt4(p);
		p += 4;
		if (flags & VBR_TOC_FLAG)
			p += 100;
		if (flags & VBR_SCALE_FLAG)
			p += 4;
		/*
					//•WЏЂVBR‘О‰ћ
					if ( p[0] == mpeg[0] && p[1] == mpeg[1] ) {
						info->skipSize = (int)(p - mpeg);
					} else {
						bitRate = br_tbl[h->version-1][h->layer-1][h->br_index] * 1000;
						switch (h->layer) {
						case 1://layer1
							frame_size = (12 * bitRate / fr_tbl[h->version-1][h->fr_index]) * 4;//one slot is 4 bytes long
							if (h->padding) frame_size += 4;
							break;
						case 2://layer2
							frame_size = 144 * bitRate / fr_tbl[h->version-1][h->fr_index];
							if (h->padding) frame_size ++;
							break;
						case 3://layer3
							frame_size = 144 * bitRate / fr_tbl[h->version-1][h->fr_index];
							if (h->version != 1) //MPEG-2, MPEG-2.5
								frame_size /= 2;
							if (h->padding) frame_size ++;
							break;
						}
						info->skipSize = (int)(frame_size);
					}
					info->bitRate = 0;
			*/
		vbr = 1;
		minBitRate = 0xffffffff;
		maxBitRate = 0;
		for (i = 1; i < 15; i ++) {
			j = br_tbl[h->version - 1][h->layer - 1][i] * 1000;
			if (j < minBitRate)
				minBitRate = j;
			if (j > maxBitRate)
				maxBitRate = j;
		}
	} else if (memcmp(p, "VBRI", 4) == 0) {
		//VBRI
		p += 10;
		info->dataSize = extractInt4(p);
		p += 4;
		info->frames = extractInt4(p);
		p += 4;
		vbr = 1;
		minBitRate = 0xffffffff;
		maxBitRate = 0;
		for (i = 1; i < 15; i ++) {
			j = br_tbl[h->version - 1][h->layer - 1][i] * 1000;
			if (j < minBitRate)
				minBitRate = j;
			if (j > maxBitRate)
				maxBitRate = j;
		}
	} else {
		//not VBR
		vbr = 0;
		info->frames = 0;
		//info->skipSize = 0;
		info->dataSize = 0;
		//info->bitRate = br_tbl[h->version-1][h->layer-1][h->br_index] * 1000;
	}

	//	info->frequency = fr_tbl[h->version-1][h->fr_index];
	//	info->msPerFrame = ms_p_f_table[h->layer-1][h->fr_index];
	//	if (h->version == 3) info->msPerFrame *= 2;
	switch (h->layer) {
	case 1:
		//layer1
		info->outputSize = 384 >> m_option.reduction;
		//if (info->bitRate) {
		if (!vbr) {
			info->skipSize = 0;
			info->minInputSize = (12 * info->bitRate / info->frequency) * 4;//one slot is 4 bytes long
			info->maxInputSize = info->minInputSize + 4;
		} else {
			info->skipSize = (12 * info->bitRate /
				info->frequency +
				h->padding) * 4;
			info->minInputSize = (12 * minBitRate / info->frequency) * 4;
			info->maxInputSize = (12 * maxBitRate / info->frequency) * 4 + 4;
		}
		break;
	case 2:
		//layer2
		info->outputSize = 1152 >> m_option.reduction;
		//if (info->bitRate) {
		if (!vbr) {
			info->skipSize = 0;
			info->minInputSize = 144 * info->bitRate / info->frequency;
			info->maxInputSize = info->minInputSize + 1;
		} else {
			info->skipSize = 144 * info->bitRate /
				info->frequency +
				h->padding;
			info->minInputSize = 144 * minBitRate / info->frequency;
			info->maxInputSize = 144 * maxBitRate / info->frequency + 1;
		}
		break;
	case 3:
		//layer3
		i = (h->version == 1) ? 1 : 2;
		//info->outputSize = 1152 >> m_option.reduction;
		info->outputSize = (1152 >> m_option.reduction) / i;
		//if (info->bitRate) {
		if (!vbr) {
			info->skipSize = 0;
			info->minInputSize = 144 * info->bitRate / info->frequency / i;
			info->maxInputSize = info->minInputSize + 1;
		} else {
			info->skipSize = 144 * info->bitRate /
				info->frequency /
				i +
				h->padding;
			info->minInputSize = 144 * minBitRate / info->frequency / i;
			info->maxInputSize = 144 * maxBitRate / info->frequency / i + 1;
		}
		break;
		/*
					if (h->version != 1) {
						//MPEG-2, MPEG-2.5
						info->outputSize /= 2;
						info->minInputSize /= 2;
						info->maxInputSize /= 2;
					}
					info->maxInputSize ++;
					break;
			*/
	}

	if ((h->mode == 3) || (m_option.convert & 3))
		info->channels = 1;
	else
		info->channels = 2;
	if (m_option.convert & 8) {
		//not available
		info->bitsPerSample = 8;
		info->outputSize *= info->channels;
	} else {
		info->bitsPerSample = 16;
		info->outputSize *= info->channels * 2;
	}
	if (decFlag == 1) {
		m_frequency = info->frequency;
		m_pcm_size = info->outputSize;
	}
	info->frequency >>= m_option.reduction;
	info->HeadBitRate = info->bitRate;
	if (vbr)
		info->bitRate = 0;

	return 1;
}

// начало декодирования
int CDecompressMpeg::mp3DecodeStart(BYTE* mpeg, int size)
{
	MPEG_DECODE_INFO info;
	MPEG_HEADER* h =& info.header;

	// распаковка заголовка и предрасчет важных данных
	if (!mp3GetDecodeInfo(mpeg, size, & info, 1))
		return 0;

	// инициализация
	sbt_init();

	// вызов методов инициализации слоя
	switch (h->layer) {
	case 1:
		L1decode_start(h);
		break;
	case 2:
		L2decode_start(h);
		break;
	case 3:
		L3decode_start(h);
		break;
	}
	return 1;
}

// декодирование 1 фрейма
int CDecompressMpeg::mp3DecodeFrame(MPEG_DECODE_PARAM* param)
{
	MPEG_HEADER* h =& param->header;

	// проверка размера входных данных
	if (param->inputSize <= 4) {
		m_last_error = MP3_ERROR_OUT_OF_BUFFER;
		return 0;
	}

	// прочитаем заголовок
	if (!mp3GetHeader((unsigned char *) param->inputBuf, h)) {
		return 0;
	}

	// вычисление размера данных в фрейме
	param->bitRate = br_tbl[h->version - 1][h->layer - 1][h->br_index] * 1000;
	switch (h->layer) {
		//layer1
	case 1:
		m_frame_size = (12 * param->bitRate / m_frequency + h->padding) * 4;
		break;
		//layer2
	case 2:
		m_frame_size = 144 * param->bitRate / m_frequency + h->padding;
		break;
		//layer3
	case 3:
		if (h->version == 1)
			m_frame_size = 144 * param->bitRate / m_frequency + h->padding;
		else
			m_frame_size = (144 * param->bitRate / m_frequency) /
				2 +
				h->padding;
		break;
	}
	// проверка размера входных данных
	if (param->inputSize < m_frame_size) {
		m_last_error = MP3_ERROR_OUT_OF_BUFFER;
		return 0;
	}

	// подбор декодера
	switch (h->layer) {
	case 1:
		L1decode_frame(h,
			(unsigned char *) param->inputBuf,
			(unsigned char *) param->outputBuf);
		break;
	case 2:
		L2decode_frame(h,
			(unsigned char *) param->inputBuf,
			(unsigned char *) param->outputBuf);
		break;
	case 3:
		L3decode_frame(h,
			(unsigned char *) param->inputBuf,
			(unsigned char *) param->outputBuf);
		break;
	}
	//!!!todo	m_frame_proc(h, (unsigned char*)param->inputBuf, (unsigned char *)param->outputBuf);
	// скоректируем размеры входного и выходного буфера
	param->inputSize = m_frame_size;
	param->outputSize = m_pcm_size;
	return 1;
}

void CDecompressMpeg::mp3Reset(void)
{
	sbt_init();
	L3decode_reset();
}

//-----------------------------------------------------------------------------
//	Установка новой позиции файла
//	на входе	:	pos	- новая позиция в файле
//	на выходе	:	*
//-----------------------------------------------------------------------------
int CDecompressMpeg::mp3seek(DWORD frame)
{
	// инициализация переменных
	DWORD cur = 0;
	DWORD back = 3;
	int off = 0;
	DWORD need_frame_offset = 0;

	// позиционируемся на данных
	if (_curFrame != frame) {
		if (_curFrame != (frame - 1)) {
			// прочитаем на несколько фреймов назад
			if (frame > back)
				frame -= back;
			else {
				back = frame;
				frame = 0;
			}

			if (!_vbr) {
				// приблизительный расчет положения фрейма
				need_frame_offset = (DWORD)
					floor(((double) frame * _bitPerFrame) /
												8);

				// поиск начала фрейма
				while (1) {
					// установка позиции чтения
					if (SourceData->seek(need_frame_offset, 0) !=
						need_frame_offset)
						return 0;

					// проверка на конец файла
					if (SourceData->eof())
						return 0;

					// прочитаем данные для поиска начала
					if (SourceData->peek(_frameBuffer, _minFrameSize) !=
						_minFrameSize)
						return 0;

					// поиск начала файла
					if (!mp3FindSync(_frameBuffer, _minFrameSize, & off)) {
						need_frame_offset += (_minFrameSize - 3);
					} else {
						need_frame_offset += off;
						break;
					}
				};
			} else {
				need_frame_offset = _vbrFrameOffTable[frame];
			}

			if (SourceData->seek(need_frame_offset, 0) != need_frame_offset)
				return 0;

			mp3Reset();

			// сбросим декодер
			for (int ch = 0; ch < 2; ch++) {
				for (int gr = 0; gr < 2; gr++) {
					for (int sam = 0; sam < 576; sam++) {
						m_sample[ch][gr][sam].s = 0;
						m_sample[ch][gr][sam].x = 0;
					}
				}
			}

			for (cur = 0; cur < back; cur++) {
				SourceData->peek(_frameBuffer, 4);
				if (!mp3GetHeaderInfo(_frameBuffer, & _mpegHI))
					return 0;

				_curFrameSize = _mpegHI.curFrameSize;
				if (SourceData->read(_frameBuffer, _curFrameSize) !=
					_curFrameSize)
					return 0;

				_mpegDP.header = _mpegHI.header;
				_mpegDP.bitRate = _mpegHI.curBitRate;
				_mpegDP.inputBuf = _frameBuffer;
				_mpegDP.inputSize = _mpegHI.curFrameSize;
				_mpegDP.outputBuf = _sampleBuffer;
				_mpegDP.outputSize = _mpegHI.outputSize;

				// декодирование одного фрейма
				if (!mp3DecodeFrame(&_mpegDP))
					return 0;
			}
		}
	}
	return 1;
}
//-----------------------------------------------------------------------------
//	Конструктор декодера
//	на входе	:	a	- указатель на данные файла
//	на выходе	:	*
//-----------------------------------------------------------------------------
CDecompressMpeg::CDecompressMpeg(WAVEFORMATEX* pcm_format, bool& flag,
	CAbstractSoundFile* a)
	: CAbstractDecompressor(pcm_format, flag, a)
{
	DWORD cur;
	DWORD pos;
	MPEG_HEADER_INFO info;
	BYTE head[156];

	// файл не определен
	flag = false;

	// инициализация декодера
	mp3DecodeInit();

	// инициализация данных декодера
	m_cs_factorL1 = m_cs_factor[0];
	//	m_enableEQ		= 0;
	memset(&m_side_info, 0, sizeof(SIDE_INFO));
	memset(&m_scale_fac, 0, sizeof(SCALE_FACTOR) * 4);
	memset(&m_cb_info, 0, sizeof(CB_INFO) * 4);
	memset(&m_nsamp, 0, sizeof(int) * 4);

	// очистим указатели на буфера
	_frameBuffer = 0;
	_vbr = 0;
	_vbrFrameOffTable = 0;

	// получение информаци о файле
	if (SourceData->peek(head, sizeof(head)) != sizeof(head))
		return;

	if (!mp3GetDecodeInfo(head, sizeof(head), & _mpegDI, 1))
		return;

	if (!mp3GetHeaderInfo(head, & _mpegHI))
		return;

	// получим интерисующую нас информацию
	_channels = _mpegDI.channels;
	_frequency = _mpegDI.frequency;
	_bitrate = _mpegDI.HeadBitRate;
	_vbr = _mpegDI.bitRate ? false : true;
	_minFrameSize = _mpegDI.minInputSize;
	_maxFrameSize = _mpegDI.maxInputSize;
	_samplesInFrame = _mpegHI.samplesInFrame;
	_curFrameSize = _mpegHI.curFrameSize;

	_version = _mpegDI.header.version;
	_layer = _mpegDI.header.layer;
	_br_index = _mpegDI.header.br_index;
	_fr_index = _mpegDI.header.fr_index;
	_mode = _mpegDI.header.mode;

	_slotSize = (_mpegDI.header.layer == 1) ? 4 : 1;
	_bitPerFrame = (_mpegDI.header.version == 1) ?
		(double) (144 * 8 * _bitrate) /
		(double) _frequency :
		(double) (144 * 8 * _bitrate) /
		(double) (_frequency * 2);
	_frames = _vbr ?
		_mpegDI.frames :
		(DWORD) floor(((double) ((SourceData->size + _slotSize) * 8)) /
											  	_bitPerFrame);
	_samplesInFile = _frames * _samplesInFrame;


	//*********************************************************************************
	// отладка
	// заполним таблицу смещений
	cur = 0;
	pos = 0;
	while (!SourceData->eof()) {
		SourceData->seek(pos, 0);

		if (SourceData->peek(head, 4) != 4)
			break;
		if (!mp3GetHeaderInfo(head, & info))
			break;
		pos += info.curFrameSize;
		cur++;
	}
	SourceData->seek(0, 0);

	if (cur != _frames)
		_frames = cur;

	_vbr = true;
	//**********************************************************************************

	// файл с переменным битрейтом ?
	if (_vbr) {
		// выделим память под таблицу смещений на фреймы
#if AGSS_USE_MALLOC
		_vbrFrameOffTable = (DWORD *) malloc(_frames * sizeof(DWORD));
#else
		_vbrFrameOffTable = (DWORD *) GlobalAlloc(GPTR,
									  	_frames * sizeof(DWORD));
#endif
		if (!_vbrFrameOffTable)
			return;

		cur = 0;
		pos = 0;
		// заполним таблицу смещений
		while (cur != _frames) {
			SourceData->seek(pos, 0);
			SourceData->peek(head, 4);
			if (!mp3GetHeaderInfo(head, & info))
				break;
			_vbrFrameOffTable[cur] = pos;
			pos += info.curFrameSize;
			cur++;
		}
		SourceData->seek(0, 0);
	}

	// выделим феймовый буфер
#if AGSS_USE_MALLOC
	_frameBuffer = (BYTE *) malloc(_mpegDI.maxInputSize);
#else
	_frameBuffer = (BYTE *) GlobalAlloc(GPTR, _mpegDI.maxInputSize);
#endif
	if (!_frameBuffer)
		return;

	// прочитаем один фрейм
	if (SourceData->read(_frameBuffer, _curFrameSize) != _curFrameSize) {
#if AGSS_USE_MALLOC
		free(_frameBuffer);
#else
		GlobalFree(_frameBuffer);
#endif
		_frameBuffer = 0;
		return;
	}

	// начало декодирования
	if (!mp3DecodeStart(_frameBuffer, _curFrameSize)) {
#if AGSS_USE_MALLOC
		free(_frameBuffer);
#else
		GlobalFree(_frameBuffer);
#endif
		_frameBuffer = 0;
		return;
	}

	// подготовка к декодированию первого фрейма
	_mpegDP.header = _mpegDI.header;
	_mpegDP.bitRate = _mpegDI.bitRate;
	_mpegDP.inputBuf = _frameBuffer;
	_mpegDP.inputSize = _curFrameSize;
	_mpegDP.outputBuf = _sampleBuffer;
	_mpegDP.outputSize = _mpegDI.outputSize;

	// декодируем первый фрейм
	if (!mp3DecodeFrame(&_mpegDP)) {
#if AGSS_USE_MALLOC
		free(_frameBuffer);
#else
		GlobalFree(_frameBuffer);
#endif
		_frameBuffer = 0;
		return;
	}

	// установим дополнительные параметры
	_curFrame = 0;
	_curSampleOffset = 0;

	// преобразуем данные для Direct X (иначе Direct X не сможет создать буфер)
	pcm_format->wFormatTag = 1;
	pcm_format->wBitsPerSample = 16;
	pcm_format->nSamplesPerSec = _frequency;
	pcm_format->nChannels = _channels;
	pcm_format->nBlockAlign = (pcm_format->nChannels * pcm_format->wBitsPerSample) >>
		3;
	pcm_format->nAvgBytesPerSec = pcm_format->nBlockAlign * pcm_format->nSamplesPerSec;

	// файл определен
	flag = true;
}
//-----------------------------------------------------------------------------
//	Деструктор декодера
//	на входе	:	*
//	на выходе	:	*
//-----------------------------------------------------------------------------
CDecompressMpeg::~CDecompressMpeg()
{
	if (_vbrFrameOffTable) {
#if AGSS_USE_MALLOC
		free(_vbrFrameOffTable);
#else
		GlobalFree(_vbrFrameOffTable);
#endif
		_vbrFrameOffTable = 0;
	}

	if (_frameBuffer) {
#if AGSS_USE_MALLOC
		free(_frameBuffer);
#else
		GlobalFree(_frameBuffer);
#endif
		_frameBuffer = 0;
	}
}
//-----------------------------------------------------------------------------
//	Декомпрессия Mp3 формата в моно данные
//	на входе	:	buffer	- указатель на буфер
//					start	- смещение в данных звука, в семплах
//					length	- количество семплов для декодирования
//	на выходе	:	На сколько байт сдвинулся буфер в который
//					читали семплы
//-----------------------------------------------------------------------------
DWORD CDecompressMpeg::GetMonoSamples(void* buffer, DWORD start, DWORD length,
	bool loop)
{
	DWORD NeedFrame;
	DWORD NeedOffset;
	DWORD samples;
	DWORD i;
	BYTE head[4];

	short* dst = (short*) buffer;

	// проверка выхода за пределы
	if (start > _samplesInFile)
		return 0;

	// проверка на чтение сверх нормы
	if ((start + length) > _samplesInFile)
		length = _samplesInFile - start;

	// вычислим текущую позицию чтения
	NeedFrame = start / _samplesInFrame;
	NeedOffset = start % _samplesInFrame;

	// позиционируемся на данных
	if (!mp3seek(NeedFrame))
		return 0;


	DWORD remaining = length;
	DWORD readsize = 0;
	bool readframe = false;
	while (remaining) {
		if ((_channels == 1) &&
			(NeedOffset == 0) &&
			(remaining > _samplesInFrame))
			readframe = true;
		else
			readframe = false;

		if (_curFrame != NeedFrame) {
			_curFrame = NeedFrame;
			if (SourceData->peek(&head, 4) != 4)
				break;

			if (!mp3GetHeaderInfo(head, & _mpegHI))
				return 0;

			_curFrameSize = _mpegHI.curFrameSize;
			if (SourceData->read(_frameBuffer, _curFrameSize) != _curFrameSize)
				return 0;

			_mpegDP.header = _mpegHI.header;
			_mpegDP.bitRate = _mpegHI.curBitRate;
			_mpegDP.inputBuf = _frameBuffer;
			_mpegDP.inputSize = _mpegHI.curFrameSize;
			_mpegDP.outputBuf = (readframe) ? dst : _sampleBuffer;
			_mpegDP.outputSize = _mpegHI.outputSize;

			// декодирование одного фрейма
			if (!mp3DecodeFrame(&_mpegDP))
				return 0;
		}

		samples = _samplesInFrame - NeedOffset;
		readsize = (remaining > samples) ? samples : remaining;

		short* src = _sampleBuffer + (NeedOffset* _channels);

		if (_channels == 1) {
			if (!readframe)
				memcpy(dst, src, readsize * 2);
			dst += readsize;
		} else {
			for (i = 0; i < readsize; i++) {
				int s = ((int) src[0] + (int) src[1]) >> 1;
				s = (s < -32768) ? -32768 : (s > 32767) ? 32767 : s;
				*dst++ = (short) s;
				src += 2;
			}
		}
		NeedOffset = 0;
		remaining -= readsize;
		if (remaining)
			NeedFrame++;
	}
	return ((DWORD) dst - (DWORD) buffer);
}

//-----------------------------------------------------------------------------
//	Декомпрессия Mp3 формата в стерео данные
//	на входе	:	buffer	- указатель на буфер
//					start	- смещение в данных звука, в семплах
//					length	- количество семплов для декодирования
//	на выходе	:	На сколько байт сдвинулся буфер в который
//					читали семплы
//-----------------------------------------------------------------------------
DWORD CDecompressMpeg::GetStereoSamples(void* buffer, DWORD start,
	DWORD length, bool loop)
{
	DWORD NeedFrame;
	DWORD NeedOffset;
	//	DWORD NeedFrameOffset;
	DWORD samples;
	DWORD i;
	BYTE head[4];
	//	int off;

	short* dst = (short*) buffer;

	// проверка выхода за пределы
	if (start > _samplesInFile)
		return 0;

	// проверка на чтение сверх нормы
	if ((start + length) > _samplesInFile)
		length = _samplesInFile - start;

	// вычислим текущую позицию чтения
	NeedFrame = start / _samplesInFrame;
	NeedOffset = start % _samplesInFrame;

	// позиционируемся на данных
	if (!mp3seek(NeedFrame))
		return 0;

	DWORD remaining = length;
	DWORD readsize = 0;
	bool readframe = false;
	while (remaining) {
		if ((_channels == 2) &&
			(NeedOffset == 0) &&
			(remaining > _samplesInFrame))
			readframe = true;
		else
			readframe = false;

		if (_curFrame != NeedFrame) {
			_curFrame = NeedFrame;
			SourceData->peek(&head, 4);
			if (!mp3GetHeaderInfo(head, & _mpegHI))
				return 0;

			_curFrameSize = _mpegHI.curFrameSize;
			if (SourceData->read(_frameBuffer, _curFrameSize) != _curFrameSize)
				return 0;

			_mpegDP.header = _mpegHI.header;
			_mpegDP.bitRate = _mpegHI.curBitRate;
			_mpegDP.inputBuf = _frameBuffer;
			_mpegDP.inputSize = _mpegHI.curFrameSize;
			_mpegDP.outputBuf = (readframe) ? dst : _sampleBuffer;
			_mpegDP.outputSize = _mpegHI.outputSize;

			// декодирование одного фрейма
			if (!mp3DecodeFrame(&_mpegDP))
				return 0;
		}

		samples = _samplesInFrame - NeedOffset;
		readsize = (remaining > samples) ? samples : remaining;

		short* src = _sampleBuffer + (NeedOffset* _channels);

		if (_channels == 1) {
			for (i = 0; i < readsize; i++) {
				*dst++ = *src;
				*dst++ = *src;
				src++;
			}
		} else {
			if (!readframe)
				memcpy(dst, src, readsize * 4);
			dst += readsize * 2;
		}
		NeedOffset = 0;
		remaining -= readsize;
		if (remaining)
			NeedFrame++;
	}
	return ((DWORD) dst - (DWORD) buffer);
}

//-----------------------------------------------------------------------------
//	Создание тишины на заданом отрезке буфера моно режим
//	на входе	:	buffer	- указатель на буфер
//					length	- количество семплов
//	на выходе	:	На сколько байт сдвинулся буфер
//-----------------------------------------------------------------------------
DWORD CDecompressMpeg::GetMonoMute(void* buffer, DWORD length)
{
	length <<= 1;
	memset(buffer, 0, length);
	return length;
}

//-----------------------------------------------------------------------------
//	Создание тишины на заданом отрезке буфера стерео режим
//	на входе	:	buffer	- указатель на буфер
//					length	- количество семплов
//	на выходе	:	На сколько байт сдвинулся буфер
//-----------------------------------------------------------------------------
DWORD CDecompressMpeg::GetStereoMute(void* buffer, DWORD length)
{
	length <<= 2;
	memset(buffer, 0, length);
	return length;
}

//-----------------------------------------------------------------------------
//	Получение количества семплов в файле
//	на входе	:	*
//	на выходе	:	Количество семплов в файла
//-----------------------------------------------------------------------------
DWORD CDecompressMpeg::GetSamplesInFile(void)
{
	return _samplesInFile;
}

//-----------------------------------------------------------------------------
//	Получение количества байт в треке моно режим
//	на входе	:	*
//	на выходе	:	Количество баит в треке
//-----------------------------------------------------------------------------
DWORD CDecompressMpeg::GetRealMonoDataSize(void)
{
	return _samplesInFile * 2;
}

//-----------------------------------------------------------------------------
//	Получение количества байт в треке стерео режим
//	на входе	:	*
//	на выходе	:	Количество баит в треке
//-----------------------------------------------------------------------------
DWORD CDecompressMpeg::GetRealStereoDataSize(void)
{
	return _samplesInFile * 4;
}

