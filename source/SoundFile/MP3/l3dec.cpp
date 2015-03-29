#include <string.h>
#include <math.h>
#include "MpegDecoder.h"

//////////////////////////////////////////////////////////////////////////
// i3alias.c
void CDecompressMpeg::alias_init()
{
	float Ci[8] = {
		-0.6f, -0.535f, -0.33f, -0.185f, -0.095f, -0.041f, -0.0142f, -0.0037f
	};

	int i;

	for (i = 0; i < 8; i++) {
		csa[i][0] = (float) (1.0 / sqrt(1.0 + Ci[i] * Ci[i]));
		csa[i][1] = (float) (Ci[i] / sqrt(1.0 + Ci[i] * Ci[i]));
	}
}

void CDecompressMpeg::antialias(float x[], int n)
{
	int i, k;
	float a, b;

	for (k = 0; k < n; k++) {
		for (i = 0; i < 8; i++) {
			a = x[17 - i];
			b = x[18 + i];
			x[17 - i] = a * csa[i][0] - b * csa[i][1];
			x[18 + i] = b * csa[i][0] + a * csa[i][1];
		}
		x += 18;
	}
}

/*===============================================================*/
void CDecompressMpeg::msis_init1()
{
	int i;
	double s, c;
	double pi;
	double t;

	pi = 4.0 * atan(1.0);
	t = pi / 12.0;
	for (i = 0; i < 7; i++) {
		s = sin(i * t);
		c = cos(i * t);
		/* ms_mode = 0 */
		lr[0][i][0] = (float) (s / (s + c));
		lr[0][i][1] = (float) (c / (s + c));
		/* ms_mode = 1 */
		lr[1][i][0] = (float) (sqrt(2.0) * (s / (s + c)));
		lr[1][i][1] = (float) (sqrt(2.0) * (c / (s + c)));
	}
	/* sf = 7 */
	/* ms_mode = 0 */
	lr[0][i][0] = 1.0f;
	lr[0][i][1] = 0.0f;
	/* ms_mode = 1, in is bands is routine does ms processing */
	lr[1][i][0] = 1.0f;
	lr[1][i][1] = 1.0f;


	/*
	for(i=0;i<21;i++) m_nBand[0][i] = 
				sfBandTable[sr_index].l[i+1] - sfBandTable[sr_index].l[i];
	for(i=0;i<12;i++) m_nBand[1][i] = 
				sfBandTable[sr_index].s[i+1] - sfBandTable[sr_index].s[i];
	*/
}
/*===============================================================*/
void CDecompressMpeg::msis_init2()
{
	int k, n;
	double t;
	int intensity_scale, ms_mode, sf, sflen;
	float ms_factor[2];


	ms_factor[0] = 1.0;
	ms_factor[1] = (float) sqrt(2.0);

	/* intensity stereo MPEG2 */
	/* lr2[intensity_scale][ms_mode][sflen_offset+sf][left/right] */

	for (intensity_scale = 0; intensity_scale < 2; intensity_scale++) {
		t = pow(2.0, -0.25 * (1 + intensity_scale));
		for (ms_mode = 0; ms_mode < 2; ms_mode++) {
			n = 1;
			k = 0;
			for (sflen = 0; sflen < 6; sflen++) {
				for (sf = 0; sf < (n - 1); sf++, k++) {
					if (sf == 0) {
						lr2[intensity_scale][ms_mode][k][0] = ms_factor[ms_mode] * 1.0f;
						lr2[intensity_scale][ms_mode][k][1] = ms_factor[ms_mode] * 1.0f;
					} else if ((sf & 1)) {
						lr2[intensity_scale][ms_mode][k][0] = (float)
							(ms_factor[ms_mode] * pow(t, (sf + 1) / 2));
						lr2[intensity_scale][ms_mode][k][1] = ms_factor[ms_mode] * 1.0f;
					} else {
						lr2[intensity_scale][ms_mode][k][0] = ms_factor[ms_mode] * 1.0f;
						lr2[intensity_scale][ms_mode][k][1] = (float)
							(ms_factor[ms_mode] * pow(t, sf / 2));
					}
				}

				/* illegal is_pos used to do ms processing */
				if (ms_mode == 0) {
					/* ms_mode = 0 */
					lr2[intensity_scale][ms_mode][k][0] = 1.0f;
					lr2[intensity_scale][ms_mode][k][1] = 0.0f;
				} else {
					/* ms_mode = 1, in is bands is routine does ms processing */
					lr2[intensity_scale][ms_mode][k][0] = 1.0f;
					lr2[intensity_scale][ms_mode][k][1] = 1.0f;
				}
				k++;
				n = n + n;
			}
		}
	}
}

void CDecompressMpeg::msis_init()
{
	msis_init1();
	msis_init2();
}

//===============================================================
/* marat
void CDecompressMpeg::ms_process(float x[][1152], int n)		// sum-difference stereo
{
   int i;
   float xl, xr;

//-- note: sqrt(2) done scaling by dequant ---
   for (i = 0; i < n; i++)
   {
	  xl = x[0][i] + x[1][i];
	  xr = x[0][i] - x[1][i];
	  x[0][i] = xl;
	  x[1][i] = xr;
   }
   return;
}
*/
void CDecompressMpeg::ms_process(float x[], int n)		// sum-difference stereo
{
	int i;
	float xl, xr;

	//-- note: sqrt(2) done scaling by dequant ---
	for (i = 0; i < n; i++) {
		xl = x[0 * 1152 + i] + x[1 * 1152 + i];
		xr = x[0 * 1152 + i] - x[1 * 1152 + i];
		x[0 * 1152 + i] = xl;
		x[1 * 1152 + i] = xr;
	}
	return;
}
/* marat
void CDecompressMpeg::is_process1(float x[][1152], SCALE_FACTOR* sf, CB_INFO cb_info[2], int nsamp)
{
   int i, j, n, cb, w;
   float fl, fr;
   int m;
   int isf;
   float fls[3], frs[3];
   int cb0;


   cb0 = cb_info[1].cbmax;	// start at end of right
   i = m_sfBandIndex[cb_info[1].cbtype][cb0];
   cb0++;
   m = nsamp - i;		// process to len of left

   if (cb_info[1].cbtype)
	  goto short_blocks;
//------------------------
// long_blocks: 
   for (cb = cb0; cb < 21; cb++)
   {
	  isf = sf->l[cb];
	  n = m_nBand[0][cb];
	  fl = lr[m_ms_mode][isf][0];
	  fr = lr[m_ms_mode][isf][1];
	  for (j = 0; j < n; j++, i++)
	  {
	 if (--m < 0)
		goto exit;
	 x[1][i] = fr * x[0][i];
	 x[0][i] = fl * x[0][i];
	  }
   }
   return;
//------------------------
 short_blocks:
   for (cb = cb0; cb < 12; cb++)
   {
	  for (w = 0; w < 3; w++)
	  {
	 isf = sf->s[w][cb];
	 fls[w] = lr[m_ms_mode][isf][0];
	 frs[w] = lr[m_ms_mode][isf][1];
	  }
	  n = m_nBand[1][cb];
	  for (j = 0; j < n; j++)
	  {
	 m -= 3;
	 if (m < 0)
		goto exit;
	 x[1][i] = frs[0] * x[0][i];
	 x[0][i] = fls[0] * x[0][i];
	 x[1][1 + i] = frs[1] * x[0][1 + i];
	 x[0][1 + i] = fls[1] * x[0][1 + i];
	 x[1][2 + i] = frs[2] * x[0][2 + i];
	 x[0][2 + i] = fls[2] * x[0][2 + i];
	 i += 3;
	  }
   }

 exit:
   return;
}
*/

void CDecompressMpeg::is_process1(float x[], SCALE_FACTOR* sf,
	CB_INFO cb_info[2], int nsamp)
{
	int i, j, n, cb, w;
	float fl, fr;
	int m;
	int isf;
	float fls[3], frs[3];
	int cb0;


	cb0 = cb_info[1].cbmax;	// start at end of right
	i = m_sfBandIndex[cb_info[1].cbtype][cb0];
	cb0++;
	m = nsamp - i;		// process to len of left

	if (cb_info[1].cbtype)
		goto short_blocks;
	//------------------------
	// long_blocks: 
	for (cb = cb0; cb < 21; cb++) {
		isf = sf->l[cb];
		n = m_nBand[0][cb];
		fl = lr[m_ms_mode][isf][0];
		fr = lr[m_ms_mode][isf][1];
		for (j = 0; j < n; j++, i++) {
			if (--m < 0)
				goto exit;
			x[1 * 1152 + i] = fr * x[0 * 1152 + i];
			x[0 * 1152 + i] = fl * x[0 * 1152 + i];
		}
	}
	return;
	//------------------------
	short_blocks:
	for (cb = cb0; cb < 12; cb++) {
		for (w = 0; w < 3; w++) {
			isf = sf->s[w][cb];
			fls[w] = lr[m_ms_mode][isf][0];
			frs[w] = lr[m_ms_mode][isf][1];
		}
		n = m_nBand[1][cb];
		for (j = 0; j < n; j++) {
			m -= 3;
			if (m < 0)
				goto exit;
			x[1 * 1152 + i] = frs[0] * x[0 * 1152 + i];
			x[0 * 1152 + i] = fls[0] * x[0 * 1152 + i];
			x[1 * 1152 + (1 + i)] = frs[1] * x[0 * 1152 + (1 + i)];
			x[0 * 1152 + (1 + i)] = fls[1] * x[0 * 1152 + (1 + i)];
			x[1 * 1152 + (2 + i)] = frs[2] * x[0 * 1152 + (2 + i)];
			x[0 * 1152 + (2 + i)] = fls[2] * x[0 * 1152 + (2 + i)];
			i += 3;
		}
	}

	exit : return;
}


typedef float ARRAY2[2];
/* marat
void CDecompressMpeg::is_process2(float x[][1152], SCALE_FACTOR* sf, CB_INFO cb_info[2], int nsamp)
{
   int i, j, k, n, cb, w;
   float fl, fr;
   int m;
   int isf;
   int il[21];
   int tmp;
   int r;
   ARRAY2 *lr;
   int cb0, cb1;

   lr = lr2[m_is_sf_info.intensity_scale][m_ms_mode];

   if (cb_info[1].cbtype)
	  goto short_blocks;

//------------------------
// long_blocks: 
   cb0 = cb_info[1].cbmax;	// start at end of right 
   i = m_sfBandIndex[0][cb0];
   m = nsamp - i;		// process to len of left 
// gen sf info 
   for (k = r = 0; r < 3; r++)
   {
	  tmp = (1 << m_is_sf_info.slen[r]) - 1;
	  for (j = 0; j < m_is_sf_info.nr[r]; j++, k++)
	 il[k] = tmp;
   }
   for (cb = cb0 + 1; cb < 21; cb++)
   {
	  isf = il[cb] + sf->l[cb];
	  fl = lr[isf][0];
	  fr = lr[isf][1];
	  n = m_nBand[0][cb];
	  for (j = 0; j < n; j++, i++)
	  {
	 if (--m < 0)
		goto exit;
	 x[1][i] = fr * x[0][i];
	 x[0][i] = fl * x[0][i];
	  }
   }
   return;
//------------------------
 short_blocks:

   for (k = r = 0; r < 3; r++)
   {
	  tmp = (1 << m_is_sf_info.slen[r]) - 1;
	  for (j = 0; j < m_is_sf_info.nr[r]; j++, k++)
	 il[k] = tmp;
   }

   for (w = 0; w < 3; w++)
   {
	  cb0 = cb_info[1].cbmax_s[w];	// start at end of right 
	  i = m_sfBandIndex[1][cb0] + w;
	  cb1 = cb_info[0].cbmax_s[w];	// process to end of left 

	  for (cb = cb0 + 1; cb <= cb1; cb++)
	  {
	 isf = il[cb] + sf->s[w][cb];
	 fl = lr[isf][0];
	 fr = lr[isf][1];
	 n = m_nBand[1][cb];
	 for (j = 0; j < n; j++)
	 {
		x[1][i] = fr * x[0][i];
		x[0][i] = fl * x[0][i];
		i += 3;
	 }
	  }

   }

 exit:
   return;
}
*/

void CDecompressMpeg::is_process2(float x[], SCALE_FACTOR* sf,
	CB_INFO cb_info[2], int nsamp)
{
	int i, j, k, n, cb, w;
	float fl, fr;
	int m;
	int isf;
	int il[21];
	int tmp;
	int r;
	ARRAY2* lr;
	int cb0, cb1;

	lr = lr2[m_is_sf_info.intensity_scale][m_ms_mode];

	if (cb_info[1].cbtype)
		goto short_blocks;

	//------------------------
	// long_blocks: 
	cb0 = cb_info[1].cbmax;	// start at end of right 
	i = m_sfBandIndex[0][cb0];
	m = nsamp - i;		// process to len of left 
	// gen sf info 
	for (k = r = 0; r < 3; r++) {
		tmp = (1 << m_is_sf_info.slen[r]) - 1;
		for (j = 0; j < m_is_sf_info.nr[r]; j++, k++)
			il[k] = tmp;
	}
	for (cb = cb0 + 1; cb < 21; cb++) {
		isf = il[cb] + sf->l[cb];
		fl = lr[isf][0];
		fr = lr[isf][1];
		n = m_nBand[0][cb];
		for (j = 0; j < n; j++, i++) {
			if (--m < 0)
				goto exit;
			x[1 * 1152 + i] = fr * x[0 * 1152 + i];
			x[0 * 1152 + i] = fl * x[0 * 1152 + i];
		}
	}
	return;
	//------------------------
	short_blocks:

	for (k = r = 0; r < 3; r++) {
		tmp = (1 << m_is_sf_info.slen[r]) - 1;
		for (j = 0; j < m_is_sf_info.nr[r]; j++, k++)
			il[k] = tmp;
	}

	for (w = 0; w < 3; w++) {
		cb0 = cb_info[1].cbmax_s[w];	// start at end of right 
		i = m_sfBandIndex[1][cb0] + w;
		cb1 = cb_info[0].cbmax_s[w];	// process to end of left 

		for (cb = cb0 + 1; cb <= cb1; cb++) {
			isf = il[cb] + sf->s[w][cb];
			fl = lr[isf][0];
			fr = lr[isf][1];
			n = m_nBand[1][cb];
			for (j = 0; j < n; j++) {
				x[1 * 1152 + i] = fr * x[0 * 1152 + i];
				x[0 * 1152 + i] = fl * x[0 * 1152 + i];
				i += 3;
			}
		}
	}

	exit : return;
}

HUFF_ELEMENT CDecompressMpeg::huff_table_0[4] = {
	0, 0, 0, 64
};			/* dummy must not use */

/*-- 6 bit lookup (purgebits, value) --*/
BYTE CDecompressMpeg::quad_table_a[64][2] = {
	6, 11, 6, 15, 6, 13, 6, 14, 6, 7, 6, 5, 5, 9, 5, 9, 5, 6, 5, 6, 5, 3, 5,
	3, 5, 10, 5, 10, 5, 12, 5, 12, 4, 2, 4, 2, 4, 2, 4, 2, 4, 1, 4, 1, 4, 1,
	4, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 8, 4, 8, 4, 8, 4, 8, 1, 0, 1, 0, 1, 0, 1,
	0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0,
	1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
	0, 1, 0, 1, 0, 1, 0, 
};

HUFF_SETUP CDecompressMpeg::table_look[34] = {
	huff_table_0, 0, no_bits, huff_table_1, 0, one_shot, huff_table_2, 0,
	one_shot, huff_table_3, 0, one_shot, huff_table_0, 0, no_bits,
	huff_table_5, 0, one_shot, huff_table_6, 0, one_shot, huff_table_7, 0,
	no_linbits, huff_table_8, 0, no_linbits, huff_table_9, 0, no_linbits,
	huff_table_10, 0, no_linbits, huff_table_11, 0, no_linbits, huff_table_12,
	0, no_linbits, huff_table_13, 0, no_linbits, huff_table_0, 0, no_bits,
	huff_table_15, 0, no_linbits, huff_table_16, 1, have_linbits,
	huff_table_16, 2, have_linbits, huff_table_16, 3, have_linbits,
	huff_table_16, 4, have_linbits, huff_table_16, 6, have_linbits,
	huff_table_16, 8, have_linbits, huff_table_16, 10, have_linbits,
	huff_table_16, 13, have_linbits, huff_table_24, 4, have_linbits,
	huff_table_24, 5, have_linbits, huff_table_24, 6, have_linbits,
	huff_table_24, 7, have_linbits, huff_table_24, 8, have_linbits,
	huff_table_24, 9, have_linbits, huff_table_24, 11, have_linbits,
	huff_table_24, 13, have_linbits, huff_table_0, 0, quad_a, huff_table_0, 0,
	quad_b, 
};

/*========================================================*/
/* marat
void CDecompressMpeg::huffman(int xy[][2], int n, int ntable)
{
   int i;
   HUFF_ELEMENT *t;
   HUFF_ELEMENT *t0;
   int linbits;
   int bits;
   int code;
   int x, y;

   if (n <= 0)
	  return;
   n = n >> 1;			// huff in pairs 
//-------------
   t0 = table_look[ntable].table;
   linbits = table_look[ntable].linbits;
   switch (table_look[ntable].ncase)
   {
	  default:
//------------------------------------------
	  case no_bits:
//- table 0, no data, x=y=0--
	 for (i = 0; i < n; i++)
	 {
		xy[i][0] = 0;
		xy[i][1] = 0;
	 }
	 return;
//------------------------------------------
	  case one_shot:
//- single lookup, no escapes -
	 for (i = 0; i < n; i++)
	 {
		mac_bitget_check((MAXBITS + 2));
		bits = t0[0].b.signbits;
		code = mac_bitget2(bits);
		mac_bitget_purge(t0[1 + code].b.purgebits);
		x = t0[1 + code].b.x;
		y = t0[1 + code].b.y;
		if (x)
		   if (mac_bitget_1bit())
		  x = -x;
		if (y)
		   if (mac_bitget_1bit())
		  y = -y;
		xy[i][0] = x;
		xy[i][1] = y;
		if (bitget_overrun())
		   break;		// bad data protect

	 }
	 return;
//------------------------------------------
	  case no_linbits:
	 for (i = 0; i < n; i++)
	 {
		t = t0;
		for (;;)
		{
		   mac_bitget_check((MAXBITS + 2));
		   bits = t[0].b.signbits;
		   code = mac_bitget2(bits);
		   if (t[1 + code].b.purgebits)
		  break;
		   t += t[1 + code].ptr;	// ptr include 1+code
		   mac_bitget_purge(bits);
		}
		mac_bitget_purge(t[1 + code].b.purgebits);
		x = t[1 + code].b.x;
		y = t[1 + code].b.y;
		if (x)
		   if (mac_bitget_1bit())
		  x = -x;
		if (y)
		   if (mac_bitget_1bit())
		  y = -y;
		xy[i][0] = x;
		xy[i][1] = y;
		if (bitget_overrun())
		   break;		// bad data protect

	 }
	 return;
//------------------------------------------
	  case have_linbits:
	 for (i = 0; i < n; i++)
	 {
		t = t0;
		for (;;)
		{
		   bits = t[0].b.signbits;
		   code = bitget2(bits);
		   if (t[1 + code].b.purgebits)
		  break;
		   t += t[1 + code].ptr;	// ptr includes 1+code
		   mac_bitget_purge(bits);
		}
		mac_bitget_purge(t[1 + code].b.purgebits);
		x = t[1 + code].b.x;
		y = t[1 + code].b.y;
		if (x == 15)
		   x += bitget_lb(linbits);
		if (x)
		   if (mac_bitget_1bit())
		  x = -x;
		if (y == 15)
		   y += bitget_lb(linbits);
		if (y)
		   if (mac_bitget_1bit())
		  y = -y;
		xy[i][0] = x;
		xy[i][1] = y;
		if (bitget_overrun())
		   break;		// bad data protect

	 }
	 return;
   }
//--- end switch ---
}
*/

void CDecompressMpeg::huffman(int xy[], int n, int ntable)
{
	int i;
	HUFF_ELEMENT* t;
	HUFF_ELEMENT* t0;
	int linbits;
	int bits;
	int code;
	int x, y;

	if (n <= 0)
		return;
	n = n >> 1;			// huff in pairs 
	//-------------
	t0 = table_look[ntable].table;
	linbits = table_look[ntable].linbits;
	switch (table_look[ntable].ncase) {
	default:
		//------------------------------------------
	case no_bits:
		//- table 0, no data, x=y=0--
		for (i = 0; i < n; i++) {
			xy[i * 2 + 0] = 0;
			xy[i * 2 + 1] = 0;
		}
		return;
		//------------------------------------------
	case one_shot:
		//- single lookup, no escapes -
		for (i = 0; i < n; i++) {
			mac_bitget_check((MAXBITS + 2));
			bits = t0[0].b.signbits;
			code = mac_bitget2(bits);
			mac_bitget_purge(t0[1 + code].b.purgebits);
			x = t0[1 + code].b.x;
			y = t0[1 + code].b.y;
			if (x)
				if (mac_bitget_1bit())
					x = -x;
			if (y)
				if (mac_bitget_1bit())
					y = -y;
			xy[i * 2 + 0] = x;
			xy[i * 2 + 1] = y;
			if (bitget_overrun())
				break;		// bad data protect
		}
		return;
		//------------------------------------------
	case no_linbits:
		for (i = 0; i < n; i++) {
			t = t0;
			for (; ;) {
				mac_bitget_check((MAXBITS + 2));
				bits = t[0].b.signbits;
				code = mac_bitget2(bits);
				if (t[1 + code].b.purgebits)
					break;
				t += t[1 + code].ptr;	// ptr include 1+code
				mac_bitget_purge(bits);
			}
			mac_bitget_purge(t[1 + code].b.purgebits);
			x = t[1 + code].b.x;
			y = t[1 + code].b.y;
			if (x)
				if (mac_bitget_1bit())
					x = -x;
			if (y)
				if (mac_bitget_1bit())
					y = -y;
			xy[i * 2 + 0] = x;
			xy[i * 2 + 1] = y;
			if (bitget_overrun())
				break;		// bad data protect
		}
		return;
		//------------------------------------------
	case have_linbits:
		for (i = 0; i < n; i++) {
			t = t0;
			for (; ;) {
				bits = t[0].b.signbits;
				code = bitget2(bits);
				if (t[1 + code].b.purgebits)
					break;
				t += t[1 + code].ptr;	// ptr includes 1+code
				mac_bitget_purge(bits);
			}
			mac_bitget_purge(t[1 + code].b.purgebits);
			x = t[1 + code].b.x;
			y = t[1 + code].b.y;
			if (x == 15)
				x += bitget_lb(linbits);
			if (x)
				if (mac_bitget_1bit())
					x = -x;
			if (y == 15)
				y += bitget_lb(linbits);
			if (y)
				if (mac_bitget_1bit())
					y = -y;
			xy[i * 2 + 0] = x;
			xy[i * 2 + 1] = y;
			if (bitget_overrun())
				break;		// bad data protect
		}
		return;
	}
	//--- end switch ---
}
/* marat
int CDecompressMpeg::huffman_quad(int vwxy[][4], int n, int nbits, int ntable)
{
   int i;
   int code;
   int x, y, v, w;
   int tmp;
   int i_non_zero, tmp_nz;

   tmp_nz = 15;
   i_non_zero = -1;

   n = n >> 2;			// huff in quads

   if (ntable)
	  goto case_quad_b;

// case_quad_a: 
   for (i = 0; i < n; i++)
   {
	  if (nbits <= 0)
	 break;
	  mac_bitget_check(10);
	  code = mac_bitget2(6);
	  nbits -= quad_table_a[code][0];
	  mac_bitget_purge(quad_table_a[code][0]);
	  tmp = quad_table_a[code][1];
	  if (tmp)
	  {
	 i_non_zero = i;
	 tmp_nz = tmp;
	  }
	  v = (tmp >> 3) & 1;
	  w = (tmp >> 2) & 1;
	  x = (tmp >> 1) & 1;
	  y = tmp & 1;
	  if (v)
	  {
	 if (mac_bitget_1bit())
		v = -v;
	 nbits--;
	  }
	  if (w)
	  {
	 if (mac_bitget_1bit())
		w = -w;
	 nbits--;
	  }
	  if (x)
	  {
	 if (mac_bitget_1bit())
		x = -x;
	 nbits--;
	  }
	  if (y)
	  {
	 if (mac_bitget_1bit())
		y = -y;
	 nbits--;
	  }
	  vwxy[i][0] = v;
	  vwxy[i][1] = w;
	  vwxy[i][2] = x;
	  vwxy[i][3] = y;
	  if (bitget_overrun())
	 break;			// bad data protect

   }
   if (nbits < 0)
   {
	  i--;
	  vwxy[i][0] = 0;
	  vwxy[i][1] = 0;
	  vwxy[i][2] = 0;
	  vwxy[i][3] = 0;
   }

   i_non_zero = (i_non_zero + 1) << 2;

   if ((tmp_nz & 3) == 0)
	  i_non_zero -= 2;

   return i_non_zero;

//--------------------
 case_quad_b:
   for (i = 0; i < n; i++)
   {
	  if (nbits < 4)
	 break;
	  nbits -= 4;
	  mac_bitget_check(8);
	  tmp = mac_bitget(4) ^ 15;	// one's complement of bitstream
	  if (tmp)
	  {
	 i_non_zero = i;
	 tmp_nz = tmp;
	  }
	  v = (tmp >> 3) & 1;
	  w = (tmp >> 2) & 1;
	  x = (tmp >> 1) & 1;
	  y = tmp & 1;
	  if (v)
	  {
	 if (mac_bitget_1bit())
		v = -v;
	 nbits--;
	  }
	  if (w)
	  {
	 if (mac_bitget_1bit())
		w = -w;
	 nbits--;
	  }
	  if (x)
	  {
	 if (mac_bitget_1bit())
		x = -x;
	 nbits--;
	  }
	  if (y)
	  {
	 if (mac_bitget_1bit())
		y = -y;
	 nbits--;
	  }
	  vwxy[i][0] = v;
	  vwxy[i][1] = w;
	  vwxy[i][2] = x;
	  vwxy[i][3] = y;
	  if (bitget_overrun())
	 break;			// bad data protect

   }
   if (nbits < 0)
   {
	  i--;
	  vwxy[i][0] = 0;
	  vwxy[i][1] = 0;
	  vwxy[i][2] = 0;
	  vwxy[i][3] = 0;
   }

   i_non_zero = (i_non_zero + 1) << 2;

   if ((tmp_nz & 3) == 0)
	  i_non_zero -= 2;

   return i_non_zero;		// return non-zero sample (to nearest pair)
}
*/
int CDecompressMpeg::huffman_quad(int vwxy[], int n, int nbits, int ntable)
{
	int i;
	int code;
	int x, y, v, w;
	int tmp;
	int i_non_zero, tmp_nz;

	tmp_nz = 15;
	i_non_zero = -1;

	n = n >> 2;			// huff in quads

	if (ntable)
		goto case_quad_b;

	// case_quad_a: 
	for (i = 0; i < n; i++) {
		if (nbits <= 0)
			break;
		mac_bitget_check(10);
		code = mac_bitget2(6);
		nbits -= quad_table_a[code][0];
		mac_bitget_purge(quad_table_a[code][0]);
		tmp = quad_table_a[code][1];
		if (tmp) {
			i_non_zero = i;
			tmp_nz = tmp;
		}
		v = (tmp >> 3) & 1;
		w = (tmp >> 2) & 1;
		x = (tmp >> 1) & 1;
		y = tmp & 1;
		if (v) {
			if (mac_bitget_1bit())
				v = -v;
			nbits--;
		}
		if (w) {
			if (mac_bitget_1bit())
				w = -w;
			nbits--;
		}
		if (x) {
			if (mac_bitget_1bit())
				x = -x;
			nbits--;
		}
		if (y) {
			if (mac_bitget_1bit())
				y = -y;
			nbits--;
		}
		vwxy[i * 4 + 0] = v;
		vwxy[i * 4 + 1] = w;
		vwxy[i * 4 + 2] = x;
		vwxy[i * 4 + 3] = y;
		if (bitget_overrun())
			break;			// bad data protect
	}
	if (nbits < 0) {
		i--;
		vwxy[i * 4 + 0] = 0;
		vwxy[i * 4 + 1] = 0;
		vwxy[i * 4 + 2] = 0;
		vwxy[i * 4 + 3] = 0;
	}

	i_non_zero = (i_non_zero + 1) << 2;

	if ((tmp_nz & 3) == 0)
		i_non_zero -= 2;

	return i_non_zero;

	//--------------------
	case_quad_b:
	for (i = 0; i < n; i++) {
		if (nbits < 4)
			break;
		nbits -= 4;
		mac_bitget_check(8);
		tmp = mac_bitget(4) ^ 15;	// one's complement of bitstream
		if (tmp) {
			i_non_zero = i;
			tmp_nz = tmp;
		}
		v = (tmp >> 3) & 1;
		w = (tmp >> 2) & 1;
		x = (tmp >> 1) & 1;
		y = tmp & 1;
		if (v) {
			if (mac_bitget_1bit())
				v = -v;
			nbits--;
		}
		if (w) {
			if (mac_bitget_1bit())
				w = -w;
			nbits--;
		}
		if (x) {
			if (mac_bitget_1bit())
				x = -x;
			nbits--;
		}
		if (y) {
			if (mac_bitget_1bit())
				y = -y;
			nbits--;
		}
		vwxy[i * 4 + 0] = v;
		vwxy[i * 4 + 1] = w;
		vwxy[i * 4 + 2] = x;
		vwxy[i * 4 + 3] = y;
		if (bitget_overrun())
			break;			// bad data protect
	}
	if (nbits < 0) {
		i--;
		vwxy[i * 4 + 0] = 0;
		vwxy[i * 4 + 1] = 0;
		vwxy[i * 4 + 2] = 0;
		vwxy[i * 4 + 3] = 0;
	}

	i_non_zero = (i_non_zero + 1) << 2;

	if ((tmp_nz & 3) == 0)
		i_non_zero -= 2;

	return i_non_zero;		// return non-zero sample (to nearest pair)
}


void CDecompressMpeg::hwin_init()
{
	int i, j;
	double pi;

	pi = 4.0 * atan(1.0);

	/* type 0 */
	for (i = 0; i < 36; i++)
		win[0][i] = (float) sin(pi / 36 * (i + 0.5));

	/* type 1 */
	for (i = 0; i < 18; i++)
		win[1][i] = (float) sin(pi / 36 * (i + 0.5));
	for (i = 18; i < 24; i++)
		win[1][i] = 1.0F;
	for (i = 24; i < 30; i++)
		win[1][i] = (float) sin(pi / 12 * (i + 0.5 - 18));
	for (i = 30; i < 36; i++)
		win[1][i] = 0.0F;

	/* type 3 */
	for (i = 0; i < 6; i++)
		win[3][i] = 0.0F;
	for (i = 6; i < 12; i++)
		win[3][i] = (float) sin(pi / 12 * (i + 0.5 - 6));
	for (i = 12; i < 18; i++)
		win[3][i] = 1.0F;
	for (i = 18; i < 36; i++)
		win[3][i] = (float) sin(pi / 36 * (i + 0.5));

	/* type 2 */
	for (i = 0; i < 12; i++)
		win[2][i] = (float) sin(pi / 12 * (i + 0.5));
	for (i = 12; i < 36; i++)
		win[2][i] = 0.0F;

	/*--- invert signs by region to match mdct 18pt --> 36pt mapping */
	for (j = 0; j < 4; j++) {
		if (j == 2)
			continue;
		for (i = 9; i < 36; i++)
			win[j][i] = -win[j][i];
	}

	/*-- invert signs for short blocks --*/
	for (i = 3; i < 12; i++)
		win[2][i] = -win[2][i];

	return;
}
/*====================================================================*/
/* marat
int CDecompressMpeg::hybrid(float xin[], float xprev[], float y[18][32], int btype, int nlong, int ntot, int nprev)
{
   int i, j;
   float *x, *x0;
   float xa, xb;
   int n;
   int nout;
   int band_limit_nsb;

   if (btype == 2)
	  btype = 0;
   x = xin;
   x0 = xprev;

//-- do long blocks (if any) --
   n = (nlong + 17) / 18;	// number of dct's to do
   for (i = 0; i < n; i++)
   {
	  imdct18(x);
	  for (j = 0; j < 9; j++)
	  {
	 y[j][i] = x0[j] + win[btype][j] * x[9 + j];
	 y[9 + j][i] = x0[9 + j] + win[btype][9 + j] * x[17 - j];
	  }
	// window x for next time x0
	  for (j = 0; j < 4; j++)
	  {
	 xa = x[j];
	 xb = x[8 - j];
	 x[j] = win[btype][18 + j] * xb;
	 x[8 - j] = win[btype][(18 + 8) - j] * xa;
	 x[9 + j] = win[btype][(18 + 9) + j] * xa;
	 x[17 - j] = win[btype][(18 + 17) - j] * xb;
	  }
	  xa = x[j];
	  x[j] = win[btype][18 + j] * xa;
	  x[9 + j] = win[btype][(18 + 9) + j] * xa;

	  x += 18;
	  x0 += 18;
   }

//-- do short blocks (if any) --
   n = (ntot + 17) / 18;	// number of 6 pt dct's triples to do
   for (; i < n; i++)
   {
	  imdct6_3(x);
	  for (j = 0; j < 3; j++)
	  {
	 y[j][i] = x0[j];
	 y[3 + j][i] = x0[3 + j];

	 y[6 + j][i] = x0[6 + j] + win[2][j] * x[3 + j];
	 y[9 + j][i] = x0[9 + j] + win[2][3 + j] * x[5 - j];

	 y[12 + j][i] = x0[12 + j] + win[2][6 + j] * x[2 - j] + win[2][j] * x[(6 + 3) + j];
	 y[15 + j][i] = x0[15 + j] + win[2][9 + j] * x[j] + win[2][3 + j] * x[(6 + 5) - j];
	  }
	// window x for next time x0
	  for (j = 0; j < 3; j++)
	  {
	 x[j] = win[2][6 + j] * x[(6 + 2) - j] + win[2][j] * x[(12 + 3) + j];
	 x[3 + j] = win[2][9 + j] * x[6 + j] + win[2][3 + j] * x[(12 + 5) - j];
	  }
	  for (j = 0; j < 3; j++)
	  {
	 x[6 + j] = win[2][6 + j] * x[(12 + 2) - j];
	 x[9 + j] = win[2][9 + j] * x[12 + j];
	  }
	  for (j = 0; j < 3; j++)
	  {
	 x[12 + j] = 0.0f;
	 x[15 + j] = 0.0f;
	  }
	  x += 18;
	  x0 += 18;
   }

//--- overlap prev if prev longer that current --
   n = (nprev + 17) / 18;
   for (; i < n; i++)
   {
	  for (j = 0; j < 18; j++)
	 y[j][i] = x0[j];
	  x0 += 18;
   }
   nout = 18 * i;

//--- clear remaining only to band limit --
	band_limit_nsb = (m_band_limit + 17) / 18;	// limit nsb's rounded up
   for (; i < band_limit_nsb; i++)
   {
	  for (j = 0; j < 18; j++)
	 y[j][i] = 0.0f;
   }

   return nout;
}
*/

int CDecompressMpeg::hybrid(float xin[], float xprev[], float y[], int btype,
	int nlong, int ntot, int nprev)
{
	int i, j;
	float* x,* x0;
	float xa, xb;
	int n;
	int nout;
	int band_limit_nsb;

	if (btype == 2)
		btype = 0;
	x = xin;
	x0 = xprev;

	//-- do long blocks (if any) --
	n = (nlong + 17) / 18;	// number of dct's to do
	for (i = 0; i < n; i++) {
		imdct18(x);
		for (j = 0; j < 9; j++) {
			y[j * 32 + i] = x0[j] + win[btype][j] * x[9 + j];
			y[(9 + j) * 32 + i] = x0[9 + j] + win[btype][9 + j] * x[17 - j];
		}
		// window x for next time x0
		for (j = 0; j < 4; j++) {
			xa = x[j];
			xb = x[8 - j];
			x[j] = win[btype][18 + j] * xb;
			x[8 - j] = win[btype][(18 + 8) - j] * xa;
			x[9 + j] = win[btype][(18 + 9) + j] * xa;
			x[17 - j] = win[btype][(18 + 17) - j] * xb;
		}
		xa = x[j];
		x[j] = win[btype][18 + j] * xa;
		x[9 + j] = win[btype][(18 + 9) + j] * xa;

		x += 18;
		x0 += 18;
	}

	//-- do short blocks (if any) --
	n = (ntot + 17) / 18;	// number of 6 pt dct's triples to do
	for (; i < n; i++) {
		imdct6_3(x);
		for (j = 0; j < 3; j++) {
			y[j * 32 + i] = x0[j];
			y[(3 + j) * 32 + i] = x0[3 + j];

			y[(6 + j) * 32 + i] = x0[6 + j] + win[2][j] * x[3 + j];
			y[(9 + j) * 32 + i] = x0[9 + j] + win[2][3 + j] * x[5 - j];

			y[(12 + j) * 32 + i] = x0[12 + j] +
				win[2][6 + j] * x[2 - j] +
				win[2][j] * x[(6 + 3) + j];
			y[(15 + j) * 32 + i] = x0[15 + j] +
				win[2][9 + j] * x[j] +
				win[2][3 + j] * x[(6 + 5) - j];
		}
		// window x for next time x0
		for (j = 0; j < 3; j++) {
			x[j] = win[2][6 + j] * x[(6 + 2) - j] +
				win[2][j] * x[(12 + 3) + j];
			x[3 + j] = win[2][9 + j] * x[6 + j] +
				win[2][3 + j] * x[(12 + 5) - j];
		}
		for (j = 0; j < 3; j++) {
			x[6 + j] = win[2][6 + j] * x[(12 + 2) - j];
			x[9 + j] = win[2][9 + j] * x[12 + j];
		}
		for (j = 0; j < 3; j++) {
			x[12 + j] = 0.0f;
			x[15 + j] = 0.0f;
		}
		x += 18;
		x0 += 18;
	}

	//--- overlap prev if prev longer that current --
	n = (nprev + 17) / 18;
	for (; i < n; i++) {
		for (j = 0; j < 18; j++)
			y[j * 32 + i] = x0[j];
		x0 += 18;
	}
	nout = 18 * i;

	//--- clear remaining only to band limit --
	band_limit_nsb = (m_band_limit + 17) / 18;	// limit nsb's rounded up
	for (; i < band_limit_nsb; i++) {
		for (j = 0; j < 18; j++)
			y[j * 32 + i] = 0.0f;
	}

	return nout;
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
//-- convert to mono, add curr result to y,
//    window and add next time to current left
/* marat
int CDecompressMpeg::hybrid_sum(float xin[], float xin_left[], float y[18][32], int btype, int nlong, int ntot)
{
   int i, j;
   float *x, *x0;
   float xa, xb;
   int n;
   int nout;

   if (btype == 2)
	  btype = 0;
   x = xin;
   x0 = xin_left;

//-- do long blocks (if any) --
   n = (nlong + 17) / 18;	// number of dct's to do
   for (i = 0; i < n; i++)
   {
	  imdct18(x);
	  for (j = 0; j < 9; j++)
	  {
	 y[j][i] += win[btype][j] * x[9 + j];
	 y[9 + j][i] += win[btype][9 + j] * x[17 - j];
	  }
	// window x for next time x0 
	  for (j = 0; j < 4; j++)
	  {
	 xa = x[j];
	 xb = x[8 - j];
	 x0[j] += win[btype][18 + j] * xb;
	 x0[8 - j] += win[btype][(18 + 8) - j] * xa;
	 x0[9 + j] += win[btype][(18 + 9) + j] * xa;
	 x0[17 - j] += win[btype][(18 + 17) - j] * xb;
	  }
	  xa = x[j];
	  x0[j] += win[btype][18 + j] * xa;
	  x0[9 + j] += win[btype][(18 + 9) + j] * xa;

	  x += 18;
	  x0 += 18;
   }

//-- do short blocks (if any) --
   n = (ntot + 17) / 18;	// number of 6 pt dct's triples to do
   for (; i < n; i++)
   {
	  imdct6_3(x);
	  for (j = 0; j < 3; j++)
	  {
	 y[6 + j][i] += win[2][j] * x[3 + j];
	 y[9 + j][i] += win[2][3 + j] * x[5 - j];

	 y[12 + j][i] += win[2][6 + j] * x[2 - j] + win[2][j] * x[(6 + 3) + j];
	 y[15 + j][i] += win[2][9 + j] * x[j] + win[2][3 + j] * x[(6 + 5) - j];
	  }
	// window x for next time
	  for (j = 0; j < 3; j++)
	  {
	 x0[j] += win[2][6 + j] * x[(6 + 2) - j] + win[2][j] * x[(12 + 3) + j];
	 x0[3 + j] += win[2][9 + j] * x[6 + j] + win[2][3 + j] * x[(12 + 5) - j];
	  }
	  for (j = 0; j < 3; j++)
	  {
	 x0[6 + j] += win[2][6 + j] * x[(12 + 2) - j];
	 x0[9 + j] += win[2][9 + j] * x[12 + j];
	  }
	  x += 18;
	  x0 += 18;
   }

   nout = 18 * i;

   return nout;
}
*/

int CDecompressMpeg::hybrid_sum(float xin[], float xin_left[], float y[],
	int btype, int nlong, int ntot)
{
	int i, j;
	float* x,* x0;
	float xa, xb;
	int n;
	int nout;

	if (btype == 2)
		btype = 0;
	x = xin;
	x0 = xin_left;

	//-- do long blocks (if any) --
	n = (nlong + 17) / 18;	// number of dct's to do
	for (i = 0; i < n; i++) {
		imdct18(x);
		for (j = 0; j < 9; j++) {
			y[j * 32 + i] += win[btype][j] * x[9 + j];
			y[(9 + j) * 32 + i] += win[btype][9 + j] * x[17 - j];
		}
		// window x for next time x0 
		for (j = 0; j < 4; j++) {
			xa = x[j];
			xb = x[8 - j];
			x0[j] += win[btype][18 + j] * xb;
			x0[8 - j] += win[btype][(18 + 8) - j] * xa;
			x0[9 + j] += win[btype][(18 + 9) + j] * xa;
			x0[17 - j] += win[btype][(18 + 17) - j] * xb;
		}
		xa = x[j];
		x0[j] += win[btype][18 + j] * xa;
		x0[9 + j] += win[btype][(18 + 9) + j] * xa;

		x += 18;
		x0 += 18;
	}

	//-- do short blocks (if any) --
	n = (ntot + 17) / 18;	// number of 6 pt dct's triples to do
	for (; i < n; i++) {
		imdct6_3(x);
		for (j = 0; j < 3; j++) {
			y[(6 + j) * 32 + i] += win[2][j] * x[3 + j];
			y[(9 + j) * 32 + i] += win[2][3 + j] * x[5 - j];

			y[(12 + j) * 32 + i] += win[2][6 + j] * x[2 - j] +
				win[2][j] * x[(6 + 3) + j];
			y[(15 + j) * 32 + i] += win[2][9 + j] * x[j] +
				win[2][3 + j] * x[(6 + 5) - j];
		}
		// window x for next time
		for (j = 0; j < 3; j++) {
			x0[j] += win[2][6 + j] * x[(6 + 2) - j] +
				win[2][j] * x[(12 + 3) + j];
			x0[3 + j] += win[2][9 + j] * x[6 + j] +
				win[2][3 + j] * x[(12 + 5) - j];
		}
		for (j = 0; j < 3; j++) {
			x0[6 + j] += win[2][6 + j] * x[(12 + 2) - j];
			x0[9 + j] += win[2][9 + j] * x[12 + j];
		}
		x += 18;
		x0 += 18;
	}

	nout = 18 * i;

	return nout;
}


/*--------------------------------------------------------------------*/
void CDecompressMpeg::sum_f_bands(float a[], float b[], int n)
{
	int i;

	for (i = 0; i < n; i++)
		a[i] += b[i];
}
/*--------------------------------------------------------------------*/
/* marat
void CDecompressMpeg::freq_invert(float y[18][32], int n)
{
	int i, j;

	n = (n + 17) / 18;
	for (j = 0; j < 18; j += 2) {
		for (i = 0; i < n; i += 2) {
			y[1 + j][1 + i] = -y[1 + j][1 + i];
		}
	}
}
*/
void CDecompressMpeg::freq_invert(float y[], int n)
{
	int i, j;

	n = (n + 17) / 18;
	for (j = 0; j < 18; j += 2) {
		for (i = 0; i < n; i += 2) {
			y[(1 + j) * 32 + (1 + i)] = -y[(1 + j) * 32 + (1 + i)];
		}
	}
}

/*--------------------------------------------------------------------*/


int CDecompressMpeg::pretab[2][22] = {
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 3, 3, 2, 0}, 
};

/*-- reorder buffer ---*/
typedef float ARRAY3[3];

void CDecompressMpeg::quant_init()
{
	int i;
	int scalefact_scale, preemp, scalefac;
	double tmp;

	/* 8 bit plus 2 lookup x = pow(2.0, 0.25*(global_gain-210)) */
	/* extra 2 for ms scaling by 1/sqrt(2) */
	/* extra 4 for cvt to mono scaling by 1/2 */
	for (i = 0; i < 256 + 2 + 4; i++)
		look_global[i] = (float)
			pow(2.0,
				0.25 * ((i - (2 + 4)) - 210 + GLOBAL_GAIN_SCALE));

	/* x = pow(2.0, -0.5*(1+scalefact_scale)*scalefac + preemp) */
	for (scalefact_scale = 0; scalefact_scale < 2; scalefact_scale++) {
		for (preemp = 0; preemp < 4; preemp++) {
			for (scalefac = 0; scalefac < 32; scalefac++) {
				look_scale[scalefact_scale][preemp][scalefac] = (float)
					pow(2.0,
						-0.5 * (1 + scalefact_scale) * (scalefac + preemp));
			}
		}
	}

	/*--- iSample**(4/3) lookup, -32<=i<=31 ---*/
	for (i = 0; i < 64; i++) {
		tmp = i - 32;
		look_pow[i] = (float) (tmp * pow(fabs(tmp), (1.0 / 3.0)));
	}

	/*-- pow(2.0, -0.25*8.0*subblock_gain)  3 bits --*/
	for (i = 0; i < 8; i++) {
		look_subblock[i] = (float) pow(2.0, 0.25 * -8.0 * i);
	}
	// quant_init_sf_band(sr_index);   replaced by code in sup.c
}

void CDecompressMpeg::dequant(SAMPLE Sample[], int gr, int ch)
{
	SCALE_FACTOR* sf =& m_scale_fac[gr][ch];
	GR_INFO* gr_info =& m_side_info.gr[gr][ch];
	CB_INFO* cb_info =& m_cb_info[gr][ch];
	int* nsamp =& m_nsamp[gr][ch];

	int i, j;
	int cb, n, w;
	float x0, xs;
	float xsb[3];
	double tmp;
	int ncbl;
	int cbs0;
	ARRAY3* buf;			/* short block reorder */
	int nbands;
	int i0;
	int non_zero;
	int cbmax[3];

	nbands = *nsamp;


	ncbl = 22;			/* long block cb end */
	cbs0 = 12;			/* short block cb start */
	/* ncbl_mixed = 8 or 6  mpeg1 or 2 */
	if (gr_info->block_type == 2) {
		ncbl = 0;
		cbs0 = 0;
		if (gr_info->mixed_block_flag) {
			ncbl = m_ncbl_mixed;
			cbs0 = 3;
		}
	}
	/* fill in cb_info -- */
	cb_info->lb_type = gr_info->block_type;
	if (gr_info->block_type == 2)
		cb_info->lb_type;
	cb_info->cbs0 = cbs0;
	cb_info->ncbl = ncbl;

	cbmax[2] = cbmax[1] = cbmax[0] = 0;
	/* global gain pre-adjusted by 2 if ms_mode, 0 otherwise */
	x0 = look_global[(2 + 4) + gr_info->global_gain];
	i = 0;
	/*----- long blocks ---*/
	for (cb = 0; cb < ncbl; cb++) {
		non_zero = 0;
		xs = x0 * look_scale[gr_info->scalefac_scale][pretab[gr_info->preflag][cb]][sf->l[cb]];
		n = m_nBand[0][cb];
		for (j = 0; j < n; j++, i++) {
			if (Sample[i].s == 0)
				Sample[i].x = 0.0F;
			else {
				non_zero = 1;
				if ((Sample[i].s >= (-ISMAX)) && (Sample[i].s < ISMAX))
					Sample[i].x = xs * look_pow[ISMAX + Sample[i].s];
				else {
					tmp = (double) Sample[i].s;
					Sample[i].x = (float)
						(xs * tmp * pow(fabs(tmp), (1.0 / 3.0)));
				}
			}
		}
		if (non_zero)
			cbmax[0] = cb;
		if (i >= nbands)
			break;
	}

	cb_info->cbmax = cbmax[0];
	cb_info->cbtype = 0;		// type = long

	if (cbs0 >= 12)
		return;
	/*---------------------------
	block type = 2  short blocks
	----------------------------*/
	cbmax[2] = cbmax[1] = cbmax[0] = cbs0;
	i0 = i;			/* save for reorder */
	buf = re_buf;
	for (w = 0; w < 3; w++)
		xsb[w] = x0 * look_subblock[gr_info->subblock_gain[w]];
	for (cb = cbs0; cb < 13; cb++) {
		n = m_nBand[1][cb];
		for (w = 0; w < 3; w++) {
			non_zero = 0;
			xs = xsb[w] * look_scale[gr_info->scalefac_scale][0][sf->s[w][cb]];
			for (j = 0; j < n; j++, i++) {
				if (Sample[i].s == 0)
					buf[j][w] = 0.0F;
				else {
					non_zero = 1;
					if ((Sample[i].s >= (-ISMAX)) && (Sample[i].s < ISMAX))
						buf[j][w] = xs * look_pow[ISMAX + Sample[i].s];
					else {
						tmp = (double) Sample[i].s;
						buf[j][w] = (float)
							(xs * tmp * pow(fabs(tmp), (1.0 / 3.0)));
					}
				}
			}
			if (non_zero)
				cbmax[w] = cb;
		}
		if (i >= nbands)
			break;
		buf += n;
	}


	memmove(&Sample[i0].x, & re_buf[0][0], sizeof(float) * (i - i0));

	*nsamp = i;			/* update nsamp */
	cb_info->cbmax_s[0] = cbmax[0];
	cb_info->cbmax_s[1] = cbmax[1];
	cb_info->cbmax_s[2] = cbmax[2];
	if (cbmax[1] > cbmax[0])
		cbmax[0] = cbmax[1];
	if (cbmax[2] > cbmax[0])
		cbmax[0] = cbmax[2];

	cb_info->cbmax = cbmax[0];
	cb_info->cbtype = 1;		/* type = short */
}

int CDecompressMpeg::L3get_side_info1()
{
	int gr, ch, size;

	m_side_info.main_data_begin = bitget(9);
	if (m_channels == 1) {
		m_side_info.private_bits = bitget(5);
		size = 17;
	} else {
		m_side_info.private_bits = bitget(3);
		size = 32;
	}
	for (ch = 0; ch < m_channels; ch ++)
		m_side_info.scfsi[ch] = bitget(4);

	for (gr = 0; gr < 2; gr ++) {
		for (ch = 0; ch < m_channels; ch ++) {
			GR_INFO* gr_info =& m_side_info.gr[gr][ch];
			gr_info->part2_3_length = bitget(12);
			gr_info->big_values = bitget(9);
			gr_info->global_gain = bitget(8);
			//gr_info->global_gain += gain_adjust;
			if (m_ms_mode)
				gr_info->global_gain -= 2;
			gr_info->scalefac_compress = bitget(4);
			gr_info->window_switching_flag = bitget(1);
			if (gr_info->window_switching_flag) {
				gr_info->block_type = bitget(2);
				gr_info->mixed_block_flag = bitget(1);
				gr_info->table_select[0] = bitget(5);
				gr_info->table_select[1] = bitget(5);
				gr_info->subblock_gain[0] = bitget(3);
				gr_info->subblock_gain[1] = bitget(3);
				gr_info->subblock_gain[2] = bitget(3);
				/* region count set in terms of long block cb's/bands */
				/* r1 set so r0+r1+1 = 21 (lookup produces 576 bands ) */
				/* if(window_switching_flag) always 36 samples in region0 */
				gr_info->region0_count = (8 - 1);	/* 36 samples */
				gr_info->region1_count = 20 - (8 - 1);
			} else {
				gr_info->mixed_block_flag = 0;
				gr_info->block_type = 0;
				gr_info->table_select[0] = bitget(5);
				gr_info->table_select[1] = bitget(5);
				gr_info->table_select[2] = bitget(5);
				gr_info->region0_count = bitget(4);
				gr_info->region1_count = bitget(3);
			}
			gr_info->preflag = bitget(1);
			gr_info->scalefac_scale = bitget(1);
			gr_info->count1table_select = bitget(1);
		}
	}
	return size;
}

int CDecompressMpeg::L3get_side_info2(int gr)
{
	int ch, size;

	m_side_info.main_data_begin = bitget(8);
	if (m_channels == 1) {
		m_side_info.private_bits = bitget(1);
		size = 9;
	} else {
		m_side_info.private_bits = bitget(2);
		size = 17;
	}
	m_side_info.scfsi[0] = 0;
	m_side_info.scfsi[1] = 0;

	for (ch = 0; ch < m_channels; ch ++) {
		m_side_info.gr[gr][ch].part2_3_length = bitget(12);
		m_side_info.gr[gr][ch].big_values = bitget(9);
		m_side_info.gr[gr][ch].global_gain = bitget(8);// + gain_adjust;
		if (m_ms_mode)
			m_side_info.gr[gr][ch].global_gain -= 2;
		m_side_info.gr[gr][ch].scalefac_compress = bitget(9);
		m_side_info.gr[gr][ch].window_switching_flag = bitget(1);
		if (m_side_info.gr[gr][ch].window_switching_flag) {
			m_side_info.gr[gr][ch].block_type = bitget(2);
			m_side_info.gr[gr][ch].mixed_block_flag = bitget(1);
			m_side_info.gr[gr][ch].table_select[0] = bitget(5);
			m_side_info.gr[gr][ch].table_select[1] = bitget(5);
			m_side_info.gr[gr][ch].subblock_gain[0] = bitget(3);
			m_side_info.gr[gr][ch].subblock_gain[1] = bitget(3);
			m_side_info.gr[gr][ch].subblock_gain[2] = bitget(3);
			/* region count set in terms of long block cb's/bands  */
			/* r1 set so r0+r1+1 = 21 (lookup produces 576 bands ) */
			/* bt=1 or 3	   54 samples */
			/* bt=2 mixed=0    36 samples */
			/* bt=2 mixed=1    54 (8 long sf) samples? or maybe 36 */
			/* region0 discussion says 54 but this would mix long */
			/* and short in region0 if scale factors switch */
			/* at band 36 (6 long scale factors) */
			if ((m_side_info.gr[gr][ch].block_type == 2)) {
				m_side_info.gr[gr][ch].region0_count = (6 - 1);	/* 36 samples */
				m_side_info.gr[gr][ch].region1_count = 20 - (6 - 1);
			} else {
				/* long block type 1 or 3 */
				m_side_info.gr[gr][ch].region0_count = (8 - 1);	/* 54 samples */
				m_side_info.gr[gr][ch].region1_count = 20 - (8 - 1);
			}
		} else {
			m_side_info.gr[gr][ch].mixed_block_flag = 0;
			m_side_info.gr[gr][ch].block_type = 0;
			m_side_info.gr[gr][ch].table_select[0] = bitget(5);
			m_side_info.gr[gr][ch].table_select[1] = bitget(5);
			m_side_info.gr[gr][ch].table_select[2] = bitget(5);
			m_side_info.gr[gr][ch].region0_count = bitget(4);
			m_side_info.gr[gr][ch].region1_count = bitget(3);
		}
		m_side_info.gr[gr][ch].preflag = 0;
		m_side_info.gr[gr][ch].scalefac_scale = bitget(1);
		m_side_info.gr[gr][ch].count1table_select = bitget(1);
	}
	return size;
}

int CDecompressMpeg::slen_table[16][2] = {
	0, 0, 0, 1, 0, 2, 0, 3, 3, 0, 1, 1, 1, 2, 1, 3, 2, 1, 2, 2, 2, 3, 3, 1, 3,
	2, 3, 3, 4, 2, 4, 3, 
};

/* nr_table[size+3*is_right][block type 0,1,3  2, 2+mixed][4]  */
/* for bt=2 nr is count for group of 3 */
int CDecompressMpeg::nr_table[6][3][4] = {
	6, 5, 5, 5, 3, 3, 3, 3, 6, 3, 3, 3, 6, 5, 7, 3, 3, 3, 4, 2, 6, 3, 4, 2,
	11, 10, 0, 0, 6, 6, 0, 0, 6, 3, 6, 0,
	/* adjusted *//* 15, 18, 0, 0,   */
	/*-intensity stereo right chan--*/
	7, 7, 7, 0, 4, 4, 4, 0, 6, 5, 4, 0, 6, 6, 6, 3, 4, 3, 3, 2, 6, 4, 3, 2, 8,
	8, 5, 0, 5, 4, 3, 0, 6, 6, 3, 0, 
};

void CDecompressMpeg::L3get_scale_factor1(int gr, int ch)
{
	SCALE_FACTOR* sf =& m_scale_fac[gr][ch];
	GR_INFO* grdat =& m_side_info.gr[gr][ch];
	int scfsi = m_side_info.scfsi[ch];

	int sfb;
	int slen0, slen1;
	int block_type, mixed_block_flag, scalefac_compress;

	block_type = grdat->block_type;
	mixed_block_flag = grdat->mixed_block_flag;
	scalefac_compress = grdat->scalefac_compress;

	slen0 = slen_table[scalefac_compress][0];
	slen1 = slen_table[scalefac_compress][1];

	if (block_type == 2) {
		if (mixed_block_flag) {
			/* mixed */
			for (sfb = 0; sfb < 8; sfb++)
				sf[0].l[sfb] = bitget(slen0);
			for (sfb = 3; sfb < 6; sfb++) {
				sf[0].s[0][sfb] = bitget(slen0);
				sf[0].s[1][sfb] = bitget(slen0);
				sf[0].s[2][sfb] = bitget(slen0);
			}
			for (sfb = 6; sfb < 12; sfb++) {
				sf[0].s[0][sfb] = bitget(slen1);
				sf[0].s[1][sfb] = bitget(slen1);
				sf[0].s[2][sfb] = bitget(slen1);
			}
			return;
		}
		for (sfb = 0; sfb < 6; sfb++) {
			sf[0].s[0][sfb] = bitget(slen0);
			sf[0].s[1][sfb] = bitget(slen0);
			sf[0].s[2][sfb] = bitget(slen0);
		}
		for (; sfb < 12; sfb++) {
			sf[0].s[0][sfb] = bitget(slen1);
			sf[0].s[1][sfb] = bitget(slen1);
			sf[0].s[2][sfb] = bitget(slen1);
		}
		return;
	}

	/* long blocks types 0 1 3, first granule */
	if (gr == 0) {
		for (sfb = 0; sfb < 11; sfb++)
			sf[0].l[sfb] = bitget(slen0);
		for (; sfb < 21; sfb++)
			sf[0].l[sfb] = bitget(slen1);
		return;
	}

	/* long blocks 0, 1, 3, second granule */
	sfb = 0;
	if (scfsi & 8)
		for (; sfb < 6; sfb++)
			sf[0].l[sfb] = sf[-2].l[sfb];
	else
		for (; sfb < 6; sfb++)
			sf[0].l[sfb] = bitget(slen0);
	if (scfsi & 4)
		for (; sfb < 11; sfb++)
			sf[0].l[sfb] = sf[-2].l[sfb];
	else
		for (; sfb < 11; sfb++)
			sf[0].l[sfb] = bitget(slen0);
	if (scfsi & 2)
		for (; sfb < 16; sfb++)
			sf[0].l[sfb] = sf[-2].l[sfb];
	else
		for (; sfb < 16; sfb++)
			sf[0].l[sfb] = bitget(slen1);
	if (scfsi & 1)
		for (; sfb < 21; sfb++)
			sf[0].l[sfb] = sf[-2].l[sfb];
	else
		for (; sfb < 21; sfb++)
			sf[0].l[sfb] = bitget(slen1);
}

void CDecompressMpeg::L3get_scale_factor2(int gr, int ch)
{
	SCALE_FACTOR* sf =& m_scale_fac[gr][ch];
	GR_INFO* grdat =& m_side_info.gr[gr][ch];
	int is_and_ch = m_is_mode& ch;

	int sfb;
	int slen1, slen2, slen3, slen4;
	int nr1, nr2, nr3, nr4;
	int i, k;
	int preflag, intensity_scale;
	int block_type, mixed_block_flag, scalefac_compress;


	block_type = grdat->block_type;
	mixed_block_flag = grdat->mixed_block_flag;
	scalefac_compress = grdat->scalefac_compress;

	preflag = 0;
	intensity_scale = 0;		/* to avoid compiler warning */
	if (is_and_ch == 0) {
		if (scalefac_compress < 400) {
			slen2 = scalefac_compress >> 4;
			slen1 = slen2 / 5;
			slen2 = slen2 % 5;
			slen4 = scalefac_compress & 15;
			slen3 = slen4 >> 2;
			slen4 = slen4 & 3;
			k = 0;
		} else if (scalefac_compress < 500) {
			scalefac_compress -= 400;
			slen2 = scalefac_compress >> 2;
			slen1 = slen2 / 5;
			slen2 = slen2 % 5;
			slen3 = scalefac_compress & 3;
			slen4 = 0;
			k = 1;
		} else {
			scalefac_compress -= 500;
			slen1 = scalefac_compress / 3;
			slen2 = scalefac_compress % 3;
			slen3 = slen4 = 0;
			if (mixed_block_flag) {
				slen3 = slen2;	/* adjust for long/short mix logic */
				slen2 = slen1;
			}
			preflag = 1;
			k = 2;
		}
	} else {
		/* intensity stereo ch = 1 (right) */
		intensity_scale = scalefac_compress & 1;
		scalefac_compress >>= 1;
		if (scalefac_compress < 180) {
			slen1 = scalefac_compress / 36;
			slen2 = scalefac_compress % 36;
			slen3 = slen2 % 6;
			slen2 = slen2 / 6;
			slen4 = 0;
			k = 3 + 0;
		} else if (scalefac_compress < 244) {
			scalefac_compress -= 180;
			slen3 = scalefac_compress & 3;
			scalefac_compress >>= 2;
			slen2 = scalefac_compress & 3;
			slen1 = scalefac_compress >> 2;
			slen4 = 0;
			k = 3 + 1;
		} else {
			scalefac_compress -= 244;
			slen1 = scalefac_compress / 3;
			slen2 = scalefac_compress % 3;
			slen3 = slen4 = 0;
			k = 3 + 2;
		}
	}

	i = 0;
	if (block_type == 2)
		i = (mixed_block_flag & 1) + 1;
	nr1 = nr_table[k][i][0];
	nr2 = nr_table[k][i][1];
	nr3 = nr_table[k][i][2];
	nr4 = nr_table[k][i][3];


	/* return is scale factor info (for right chan is mode) */
	if (is_and_ch) {
		m_is_sf_info.nr[0] = nr1;
		m_is_sf_info.nr[1] = nr2;
		m_is_sf_info.nr[2] = nr3;
		m_is_sf_info.slen[0] = slen1;
		m_is_sf_info.slen[1] = slen2;
		m_is_sf_info.slen[2] = slen3;
		m_is_sf_info.intensity_scale = intensity_scale;
	}
	grdat->preflag = preflag;	/* return preflag */

	/*--------------------------------------*/
	if (block_type == 2) {
		if (mixed_block_flag) {
			/* mixed */
			if (slen1 != 0)	/* long block portion */
				for (sfb = 0; sfb < 6; sfb++)
					sf[0].l[sfb] = bitget(slen1);
			else
				for (sfb = 0; sfb < 6; sfb++)
					sf[0].l[sfb] = 0;
			sfb = 3;		/* start sfb for short */
		} else {
			/* all short, initial short blocks */
			sfb = 0;
			if (slen1 != 0)
				for (i = 0; i < nr1; i++, sfb++) {
					sf[0].s[0][sfb] = bitget(slen1);
					sf[0].s[1][sfb] = bitget(slen1);
					sf[0].s[2][sfb] = bitget(slen1);
				}
			else
				for (i = 0; i < nr1; i++, sfb++) {
					sf[0].s[0][sfb] = 0;
					sf[0].s[1][sfb] = 0;
					sf[0].s[2][sfb] = 0;
				}
		}
		/* remaining short blocks */
		if (slen2 != 0)
			for (i = 0; i < nr2; i++, sfb++) {
				sf[0].s[0][sfb] = bitget(slen2);
				sf[0].s[1][sfb] = bitget(slen2);
				sf[0].s[2][sfb] = bitget(slen2);
			}
		else
			for (i = 0; i < nr2; i++, sfb++) {
				sf[0].s[0][sfb] = 0;
				sf[0].s[1][sfb] = 0;
				sf[0].s[2][sfb] = 0;
			}
		if (slen3 != 0)
			for (i = 0; i < nr3; i++, sfb++) {
				sf[0].s[0][sfb] = bitget(slen3);
				sf[0].s[1][sfb] = bitget(slen3);
				sf[0].s[2][sfb] = bitget(slen3);
			}
		else
			for (i = 0; i < nr3; i++, sfb++) {
				sf[0].s[0][sfb] = 0;
				sf[0].s[1][sfb] = 0;
				sf[0].s[2][sfb] = 0;
			}
		if (slen4 != 0)
			for (i = 0; i < nr4; i++, sfb++) {
				sf[0].s[0][sfb] = bitget(slen4);
				sf[0].s[1][sfb] = bitget(slen4);
				sf[0].s[2][sfb] = bitget(slen4);
			}
		else
			for (i = 0; i < nr4; i++, sfb++) {
				sf[0].s[0][sfb] = 0;
				sf[0].s[1][sfb] = 0;
				sf[0].s[2][sfb] = 0;
			}
		return;
	}


	/* long blocks types 0 1 3 */
	sfb = 0;
	if (slen1 != 0)
		for (i = 0; i < nr1; i++, sfb++)
			sf[0].l[sfb] = bitget(slen1);
	else
		for (i = 0; i < nr1; i++, sfb++)
			sf[0].l[sfb] = 0;

	if (slen2 != 0)
		for (i = 0; i < nr2; i++, sfb++)
			sf[0].l[sfb] = bitget(slen2);
	else
		for (i = 0; i < nr2; i++, sfb++)
			sf[0].l[sfb] = 0;

	if (slen3 != 0)
		for (i = 0; i < nr3; i++, sfb++)
			sf[0].l[sfb] = bitget(slen3);
	else
		for (i = 0; i < nr3; i++, sfb++)
			sf[0].l[sfb] = 0;

	if (slen4 != 0)
		for (i = 0; i < nr4; i++, sfb++)
			sf[0].l[sfb] = bitget(slen4);
	else
		for (i = 0; i < nr4; i++, sfb++)
			sf[0].l[sfb] = 0;
}


SFBT CDecompressMpeg::sfBandTable[3][3] = {
	// MPEG-1
	{{
	{0, 4, 8, 12, 16, 20, 24, 30, 36, 44, 52, 62, 74, 90, 110, 134, 162, 196, 238, 288, 342, 418, 576},
	{0, 4, 8, 12, 16, 22, 30, 40, 52, 66, 84, 106, 136, 192}
	},{
	{0, 4, 8, 12, 16, 20, 24, 30, 36, 42, 50, 60, 72, 88, 106, 128, 156, 190, 230, 276, 330, 384, 576},
	{0, 4, 8, 12, 16, 22, 28, 38, 50, 64, 80, 100, 126, 192}
	},{
	{0, 4, 8, 12, 16, 20, 24, 30, 36, 44, 54, 66, 82, 102, 126, 156, 194, 240, 296, 364, 448, 550, 576},
	{0, 4, 8, 12, 16, 22, 30, 42, 58, 78, 104, 138, 180, 192}
	}},
	// MPEG-2
	{{
	{0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238, 284, 336, 396, 464, 522, 576},
	{0, 4, 8, 12, 18, 24, 32, 42, 56, 74, 100, 132, 174, 192}
	},{
	{0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 114, 136, 162, 194, 232, 278, 332, 394, 464, 540, 576},
	{0, 4, 8, 12, 18, 26, 36, 48, 62, 80, 104, 136, 180, 192}
	},{
	{0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238, 284, 336, 396, 464, 522, 576},
	{0, 4, 8, 12, 18, 26, 36, 48, 62, 80, 104, 134, 174, 192}
	}},
	// MPEG-2.5, 11 & 12 KHz seem ok, 8 ok
	{{
	{0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238, 284, 336, 396, 464, 522, 576},
	{0, 4, 8, 12, 18, 26, 36, 48, 62, 80, 104, 134, 174, 192}
	},{
	{0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238, 284, 336, 396, 464, 522, 576},
	{0, 4, 8, 12, 18, 26, 36, 48, 62, 80, 104, 134, 174, 192}
	},{
	// this 8khz table, and only 8khz, from mpeg123)
	{0, 12, 24, 36, 48, 60, 72, 88, 108, 132, 160, 192, 232, 280, 336, 400, 476, 566, 568, 570, 572, 574, 576},
	{0, 8, 16, 24, 36, 52, 72, 96, 124, 160, 162, 164, 166, 192}
	}}, 
};

void CDecompressMpeg::L3table_init()
{
	quant_init();
	alias_init();
	msis_init();
	fdct_init();
	imdct_init();
	hwin_init();
}

int CDecompressMpeg::L3decode_start(MPEG_HEADER* h)
{
	int i, j, k, v;
	int channels, limit;
	int bit_code;

	m_buf_ptr0 = 0;
	m_buf_ptr1 = 0;
	m_gr = 0;
	v = h->version - 1;
	if (h->version == 1) //MPEG-1
		m_ncbl_mixed = 8;
	else //MPEG-2, MPEG-2.5
		m_ncbl_mixed = 6;

	// compute nsb_limit
	m_nsb_limit = (m_option.freqLimit * 64L + m_frequency / 2) / m_frequency;
	// caller limit
	limit = (32 >> m_option.reduction);
	if (limit > 8)
		limit--;
	if (m_nsb_limit > limit)
		m_nsb_limit = limit;
	limit = 18 * m_nsb_limit;

	if (h->version == 1) {
		//MPEG-1
		m_band_limit12 = 3 * sfBandTable[v][h->fr_index].s[13];
		m_band_limit = m_band_limit21 = sfBandTable[v][h->fr_index].l[22];
	} else {
		//MPEG-2, MPEG-2.5
		m_band_limit12 = 3 * sfBandTable[v][h->fr_index].s[12];
		m_band_limit = m_band_limit21 = sfBandTable[v][h->fr_index].l[21];
	}
	m_band_limit += 8;	// allow for antialias
	if (m_band_limit > limit)
		m_band_limit = limit;
	if (m_band_limit21 > m_band_limit)
		m_band_limit21 = m_band_limit;
	if (m_band_limit12 > m_band_limit)
		m_band_limit12 = m_band_limit;
	m_band_limit_nsb = (m_band_limit + 17) / 18;	// limit nsb's rounded up
	/*
		gain_adjust = 0;	// adjust gain e.g. cvt to mono sum channel
		if ((h->mode != 3) && (m_option.convert == 1))
			gain_adjust = -4;
	*/
	m_channels = (h->mode == 3) ? 1 : 2;
	if (m_option.convert)
		channels = 1;
	else
		channels = m_channels;

	bit_code = (m_option.convert & 8) ? 1 : 0;

	//!!!todo	m_sbt_proc = sbt_table[bit_code][m_option.reduction][channels - 1];//[2][3][2]

	m_sbt_wrk = bit_code * 3 * 2 + m_option.reduction * 2 + channels - 1;

	k = (h->mode != 3) ? (1 + m_option.convert) : 0;
	//!!!todo	m_xform_proc = xform_table[k];//[5]
	m_xform_wrk = k;

	/*
		if (bit_code)
			zero_level_pcm = 128;// 8 bit output
		else
			zero_level_pcm = 0;
	*/
	// init band tables
	for (i = 0; i < 22; i ++)
		m_sfBandIndex[0][i] = sfBandTable[v][h->fr_index].l[i + 1];

	for (i = 0; i < 13; i ++)
		m_sfBandIndex[1][i] = 3 * sfBandTable[v][h->fr_index].s[i + 1];

	for (i = 0; i < 22; i ++)
		m_nBand[0][i] = sfBandTable[v][h->fr_index].l[i + 1] -
			sfBandTable[v][h->fr_index].l[i];

	for (i = 0; i < 13; i ++)
		m_nBand[1][i] = sfBandTable[v][h->fr_index].s[i + 1] -
			sfBandTable[v][h->fr_index].s[i];

	// clear buffers
	for (i = 0; i < 576; i++)
		m_yout[i] = 0.0f;
	// !!!marat!!!
	for (i = 0; i < 2; i ++) {
		for (j = 0; j < 2; j ++) {
			for (k = 0; k < 576; k++) {
				m_sample[i][j][k].x = 0.0f;
				m_sample[i][j][k].s = 0;
			}
		}
	}

	/*	for (i = 0; i < 2304; i++)
			m_sample[i] = 0;*/

	return 1;
}


void CDecompressMpeg::L3decode_reset()
{
	m_buf_ptr0 = 0;
	m_buf_ptr1 = 0;
}

void CDecompressMpeg::L3decode_frame(MPEG_HEADER* h, byte* mpeg, byte* pcm)
{
	int crc_size, side_size;
	int copy_size;

	if (h->mode == 1) {
		m_ms_mode = h->mode_ext >> 1;
		m_is_mode = h->mode_ext & 1;
	} else {
		m_ms_mode = 0;
		m_is_mode = 0;
	}

	crc_size = (h->error_prot) ? 2 : 0;
	bitget_init(mpeg + 4 + crc_size);
	if (h->version == 1)
		side_size = L3get_side_info1();
	else
		side_size = L3get_side_info2(m_gr);

	/* decode start point */
	m_buf_ptr0 = m_buf_ptr1 - m_side_info.main_data_begin;

	/* shift buffer */
	if (m_buf_ptr1 > BUF_TRIGGER) {
		memmove(m_buf, m_buf + m_buf_ptr0, m_side_info.main_data_begin);
		m_buf_ptr0 = 0;
		m_buf_ptr1 = m_side_info.main_data_begin;
	}
	copy_size = m_frame_size - (4 + crc_size + side_size);
	memmove(m_buf + m_buf_ptr1, mpeg + (4 + crc_size + side_size), copy_size);
	m_buf_ptr1 += copy_size;

	if (m_buf_ptr0 >= 0) {
		m_main_pos_bit = m_buf_ptr0 << 3;
		if (h->version == 1) {
			L3decode_main(h, pcm, 0);
			L3decode_main(h, pcm + (m_pcm_size / 2), 1);
		} else {
			L3decode_main(h, pcm, m_gr);
			m_gr = m_gr ^ 1;
		}
	}
}

void CDecompressMpeg::L3decode_main(MPEG_HEADER* h, byte* pcm, int gr)
{
	int ch;
	int n1, n2, n3, n4, nn2, nn3, nn4;
	int bit0, qbits, m0;

	for (ch = 0; ch < m_channels; ch ++) {
		bitget_init(m_buf + (m_main_pos_bit >> 3));
		bit0 = (m_main_pos_bit & 7);
		if (bit0)
			bitget(bit0);
		m_main_pos_bit += m_side_info.gr[gr][ch].part2_3_length;
		bitget_init_end(m_buf + ((m_main_pos_bit + 39) >> 3));
		// scale factors
		if (h->version == 1)
			L3get_scale_factor1(gr, ch);
		else
			L3get_scale_factor2(gr, ch);
		// huff data
		n1 = m_sfBandIndex[0][m_side_info.gr[gr][ch].region0_count];
		n2 = m_sfBandIndex[0][m_side_info.gr[gr][ch].region0_count + m_side_info.gr[gr][ch].region1_count + 1];
		n3 = m_side_info.gr[gr][ch].big_values;
		n3 = n3 + n3;

		if (n3 > m_band_limit)
			n3 = m_band_limit;
		if (n2 > n3)
			n2 = n3;
		if (n1 > n3)
			n1 = n3;
		nn3 = n3 - n2;
		nn2 = n2 - n1;
		huffman((int *) m_sample[ch][gr],
			n1,
			m_side_info.gr[gr][ch].table_select[0]);
		huffman((int *) m_sample[ch][gr] + n1,
			nn2,
			m_side_info.gr[gr][ch].table_select[1]);
		huffman((int *) m_sample[ch][gr] + n2,
			nn3,
			m_side_info.gr[gr][ch].table_select[2]);
		qbits = m_side_info.gr[gr][ch].part2_3_length -
			(bitget_bits_used() - bit0);
		nn4 = huffman_quad((int *) m_sample[ch][gr] + n3,
			  	m_band_limit - n3,
			  	qbits,
			  	m_side_info.gr[gr][ch].count1table_select);
		n4 = n3 + nn4;
		m_nsamp[gr][ch] = n4;
		// limit n4 or allow deqaunt to sf band 22
		if (m_side_info.gr[gr][ch].block_type == 2)
			n4 = min(n4, m_band_limit12);
		else
			n4 = min(n4, m_band_limit21);
		if (n4 < 576)
			memset(m_sample[ch][gr] + n4, 0, sizeof(SAMPLE) * (576 - n4));
		if (bitget_overrun())
			memset(m_sample[ch][gr], 0, sizeof(SAMPLE) * (576));
	}
	// dequant
	for (ch = 0; ch < m_channels; ch++) {
		//		dequant(m_sample + (ch * 2 * 576 + gr * 576), gr, ch);
		dequant(m_sample[ch][gr], gr, ch);
	}
	// ms stereo processing
	if (m_ms_mode) {
		if (m_is_mode == 0) {
			m0 = m_nsamp[gr][0];	// process to longer of left/right
			if (m0 < m_nsamp[gr][1])
				m0 = m_nsamp[gr][1];
		} else {
			// process to last cb in right
			m0 = m_sfBandIndex[m_cb_info[gr][1].cbtype][m_cb_info[gr][1].cbmax];
		}
		ms_process((float *) m_sample[0][gr], m0);
	}
	// is stereo processing
	if (m_is_mode) {
		if (h->version == 1)
			is_process1((float *) m_sample[0][gr],
				& m_scale_fac[gr][1],
				m_cb_info[gr],
				m_nsamp[gr][0]);
		else
			is_process2((float *) m_sample[0][gr],
				& m_scale_fac[gr][1],
				m_cb_info[gr],
				m_nsamp[gr][0]);
	}
	// adjust ms and is modes to max of left/right
	if (m_ms_mode || m_is_mode) {
		if (m_nsamp[gr][0] < m_nsamp[gr][1])
			m_nsamp[gr][0] = m_nsamp[gr][1];
		else
			m_nsamp[gr][1] = m_nsamp[gr][0];
	}

	// antialias
	for (ch = 0; ch < m_channels; ch ++) {
		if (m_cb_info[gr][ch].ncbl == 0)
			continue;		// have no long blocks
		if (m_side_info.gr[gr][ch].mixed_block_flag)
			n1 = 1;		// 1 -> 36 samples
		else
			n1 = (m_nsamp[gr][ch] + 7) / 18;
		if (n1 > 31)
			n1 = 31;
		//		antialias((float*)(m_sample + (ch * 2 * 576 + gr * 576)), n1);
		antialias((float *) m_sample[ch][gr], n1);
		n1 = 18 * n1 + 8;		// update number of samples
		if (n1 > m_nsamp[gr][ch])
			m_nsamp[gr][ch] = n1;
	}
	// hybrid + sbt
	//!!!todo m_xform_proc(pcm, gr);
	switch (m_xform_wrk) {
	case 0:
		xform_mono(pcm, gr);
		break;
	case 1:
		xform_dual(pcm, gr);
		break;
	case 2:
		xform_dual_mono(pcm, gr);
		break;
	case 3:
		xform_mono(pcm, gr);
		break;
	case 4:
		xform_dual_right(pcm, gr);
		break;
	}
}

void CDecompressMpeg::xform_mono(void* pcm, int igr)
{
	int igr_prev, n1, n2;

	// hybrid + sbt
	n1 = n2 = m_nsamp[igr][0];	// total number bands
	if (m_side_info.gr[igr][0].block_type == 2) {
		// long bands
		if (m_side_info.gr[igr][0].mixed_block_flag)
			n1 = m_sfBandIndex[0][m_ncbl_mixed - 1];
		else
			n1 = 0;
	}
	if (n1 > m_band_limit)
		n1 = m_band_limit;
	if (n2 > m_band_limit)
		n2 = m_band_limit;
	igr_prev = igr ^ 1;

	m_nsamp[igr][0] = hybrid((float *) m_sample[0][igr],
					  	(float *) m_sample[0][igr_prev],
					  	m_yout,
					  	m_side_info.gr[igr][0].block_type,
					  	n1,
					  	n2,
					  	m_nsamp[igr_prev][0]);
	freq_invert(m_yout, m_nsamp[igr][0]);
	//!!!todo m_sbt_proc(m_yout, pcm, 0);
	switch (m_sbt_wrk) {
	case 0:
		sbt_mono_L3(m_yout, pcm, 0);
		break;
	case 1:
		sbt_dual_L3(m_yout, pcm, 0);
		break;
	case 2:
		sbt16_mono_L3(m_yout, pcm, 0);
		break;
	case 3:
		sbt16_dual_L3(m_yout, pcm, 0);
		break;
	case 4:
		sbt8_mono_L3(m_yout, pcm, 0);
		break;
	case 5:
		sbt8_dual_L3(m_yout, pcm, 0);
		break;
	case 6:
		sbtB_mono_L3(m_yout, pcm, 0);
		break;
	case 7:
		sbtB_dual_L3(m_yout, pcm, 0);
		break;
	case 8:
		sbtB16_mono_L3(m_yout, pcm, 0);
		break;
	case 9:
		sbtB16_dual_L3(m_yout, pcm, 0);
		break;
	case 10:
		sbtB8_mono_L3(m_yout, pcm, 0);
		break;
	case 11:
		sbtB8_dual_L3(m_yout, pcm, 0);
		break;
	}
}

void CDecompressMpeg::xform_dual_right(void* pcm, int igr)
{
	int igr_prev, n1, n2;

	// hybrid + sbt
	n1 = n2 = m_nsamp[igr][1];	// total number bands
	if (m_side_info.gr[igr][1].block_type == 2) {
		// long bands
		if (m_side_info.gr[igr][1].mixed_block_flag)
			n1 = m_sfBandIndex[0][m_ncbl_mixed - 1];
		else
			n1 = 0;
	}
	if (n1 > m_band_limit)
		n1 = m_band_limit;
	if (n2 > m_band_limit)
		n2 = m_band_limit;
	igr_prev = igr ^ 1;
	m_nsamp[igr][1] = hybrid((float *) m_sample[1][igr],
					  	(float *) m_sample[1][igr_prev],
					  	m_yout,
					  	m_side_info.gr[igr][1].block_type,
					  	n1,
					  	n2,
					  	m_nsamp[igr_prev][1]);
	freq_invert(m_yout, m_nsamp[igr][1]);
	//!!!todo m_sbt_proc(m_yout, pcm, 0);
	switch (m_sbt_wrk) {
	case 0:
		sbt_mono_L3(m_yout, pcm, 0);
		break;
	case 1:
		sbt_dual_L3(m_yout, pcm, 0);
		break;
	case 2:
		sbt16_mono_L3(m_yout, pcm, 0);
		break;
	case 3:
		sbt16_dual_L3(m_yout, pcm, 0);
		break;
	case 4:
		sbt8_mono_L3(m_yout, pcm, 0);
		break;
	case 5:
		sbt8_dual_L3(m_yout, pcm, 0);
		break;
	case 6:
		sbtB_mono_L3(m_yout, pcm, 0);
		break;
	case 7:
		sbtB_dual_L3(m_yout, pcm, 0);
		break;
	case 8:
		sbtB16_mono_L3(m_yout, pcm, 0);
		break;
	case 9:
		sbtB16_dual_L3(m_yout, pcm, 0);
		break;
	case 10:
		sbtB8_mono_L3(m_yout, pcm, 0);
		break;
	case 11:
		sbtB8_dual_L3(m_yout, pcm, 0);
		break;
	}
}

void CDecompressMpeg::xform_dual(void* pcm, int igr)
{
	int ch;
	int igr_prev, n1, n2;

	// hybrid + sbt
	igr_prev = igr ^ 1;
	for (ch = 0; ch < m_channels; ch++) {
		n1 = n2 = m_nsamp[igr][ch];	// total number bands
		if (m_side_info.gr[igr][ch].block_type == 2) {
			// long bands
			if (m_side_info.gr[igr][ch].mixed_block_flag)
				n1 = m_sfBandIndex[0][m_ncbl_mixed - 1];
			else
				n1 = 0;
		}
		if (n1 > m_band_limit)
			n1 = m_band_limit;
		if (n2 > m_band_limit)
			n2 = m_band_limit;
		m_nsamp[igr][ch] = hybrid((float *) m_sample[ch][igr],
						   	(float *) m_sample[ch][igr_prev],
						   	m_yout,
						   	m_side_info.gr[igr][ch].block_type,
						   	n1,
						   	n2,
						   	m_nsamp[igr_prev][ch]);
		freq_invert(m_yout, m_nsamp[igr][ch]);
		//!!!todo m_sbt_proc(m_yout, pcm, 0);
		switch (m_sbt_wrk) {
		case 0:
			sbt_mono_L3(m_yout, pcm, ch);
			break;
		case 1:
			sbt_dual_L3(m_yout, pcm, ch);
			break;
		case 2:
			sbt16_mono_L3(m_yout, pcm, ch);
			break;
		case 3:
			sbt16_dual_L3(m_yout, pcm, ch);
			break;
		case 4:
			sbt8_mono_L3(m_yout, pcm, ch);
			break;
		case 5:
			sbt8_dual_L3(m_yout, pcm, ch);
			break;
		case 6:
			sbtB_mono_L3(m_yout, pcm, ch);
			break;
		case 7:
			sbtB_dual_L3(m_yout, pcm, ch);
			break;
		case 8:
			sbtB16_mono_L3(m_yout, pcm, ch);
			break;
		case 9:
			sbtB16_dual_L3(m_yout, pcm, ch);
			break;
		case 10:
			sbtB8_mono_L3(m_yout, pcm, ch);
			break;
		case 11:
			sbtB8_dual_L3(m_yout, pcm, ch);
			break;
		}
	}
}

void CDecompressMpeg::xform_dual_mono(void* pcm, int igr)
{
	int igr_prev, n1, n2, n3;

	// hybrid + sbt
	igr_prev = igr ^ 1;
	if ((m_side_info.gr[igr][0].block_type ==
		m_side_info.gr[igr][1].block_type) &&
		(m_side_info.gr[igr][0].mixed_block_flag == 0) &&
		(m_side_info.gr[igr][1].mixed_block_flag == 0)) {
		n2 = m_nsamp[igr][0];	// total number bands max of L R
		if (n2 < m_nsamp[igr][1])
			n2 = m_nsamp[igr][1];
		if (n2 > m_band_limit)
			n2 = m_band_limit;
		if (m_side_info.gr[igr][0].block_type == 2)
			n1 = 0;
		else
			n1 = n2;		// n1 = number long bands
		sum_f_bands((float *) m_sample[0][igr], (float *) m_sample[1][igr], n2);
		n3 = m_nsamp[igr][0] = hybrid((float *) m_sample[0][igr],
							   	(float *) m_sample[0][igr_prev],
							   	m_yout,
							   	m_side_info.gr[igr][0].block_type,
							   	n1,
							   	n2,
							   	m_nsamp[igr_prev][0]);
	} else {
		// transform and then sum (not tested - never happens in test)
		// left chan
		n1 = n2 = m_nsamp[igr][0];	// total number bands
		if (m_side_info.gr[igr][0].block_type == 2) {
			// long bands
			if (m_side_info.gr[igr][0].mixed_block_flag)
				n1 = m_sfBandIndex[0][m_ncbl_mixed - 1];
			else
				n1 = 0;
		}
		n3 = m_nsamp[igr][0] = hybrid((float *) m_sample[0][igr],
							   	(float *) m_sample[0][igr_prev],
							   	m_yout,
							   	m_side_info.gr[igr][0].block_type,
							   	n1,
							   	n2,
							   	m_nsamp[igr_prev][0]);
		// right chan
		n1 = n2 = m_nsamp[igr][1];	// total number bands
		if (m_side_info.gr[igr][1].block_type == 2) {
			// long bands
			if (m_side_info.gr[igr][1].mixed_block_flag)
				n1 = m_sfBandIndex[0][m_ncbl_mixed - 1];
			else
				n1 = 0;
		}
		m_nsamp[igr][1] = hybrid_sum((float *) m_sample[1][igr],
						  	(float *) m_sample[0][igr],
						  	m_yout,
						  	m_side_info.gr[igr][1].block_type,
						  	n1,
						  	n2);
		if (n3 < m_nsamp[igr][1])
			n1 = m_nsamp[igr][1];
	}

	freq_invert(m_yout, n3);
	//!!!todo m_sbt_proc(m_yout, pcm, 0);
	switch (m_sbt_wrk) {
	case 0:
		sbt_mono_L3(m_yout, pcm, 0);
		break;
	case 1:
		sbt_dual_L3(m_yout, pcm, 0);
		break;
	case 2:
		sbt16_mono_L3(m_yout, pcm, 0);
		break;
	case 3:
		sbt16_dual_L3(m_yout, pcm, 0);
		break;
	case 4:
		sbt8_mono_L3(m_yout, pcm, 0);
		break;
	case 5:
		sbt8_dual_L3(m_yout, pcm, 0);
		break;
	case 6:
		sbtB_mono_L3(m_yout, pcm, 0);
		break;
	case 7:
		sbtB_dual_L3(m_yout, pcm, 0);
		break;
	case 8:
		sbtB16_mono_L3(m_yout, pcm, 0);
		break;
	case 9:
		sbtB16_dual_L3(m_yout, pcm, 0);
		break;
	case 10:
		sbtB8_mono_L3(m_yout, pcm, 0);
		break;
	case 11:
		sbtB8_dual_L3(m_yout, pcm, 0);
		break;
	}
}

