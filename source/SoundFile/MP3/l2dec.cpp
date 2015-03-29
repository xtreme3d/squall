#include <math.h>
#include "MpegDecoder.h"

/* ABCD_INDEX = lookqt[mode][sr_index][br_index]  */
/* -1 = invalid  */
char CDecompressMpeg::lookqt[4][3][16] = {
	1, -1, -1, -1, 2, -1, 2, 0, 0, 0, 1, 1, 1, 1, 1, -1,
	/*  44ks stereo */
	0, -1, -1, -1, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, -1,		/*  48ks */
	1, -1, -1, -1, 3, -1, 3, 0, 0, 0, 1, 1, 1, 1, 1, -1,		/*  32ks */
	1, -1, -1, -1, 2, -1, 2, 0, 0, 0, 1, 1, 1, 1, 1, -1,
	/*  44ks joint stereo */
	0, -1, -1, -1, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, -1,		/*  48ks */
	1, -1, -1, -1, 3, -1, 3, 0, 0, 0, 1, 1, 1, 1, 1, -1,		/*  32ks */
	1, -1, -1, -1, 2, -1, 2, 0, 0, 0, 1, 1, 1, 1, 1, -1,
	/*  44ks dual chan */
	0, -1, -1, -1, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, -1,		/*  48ks */
	1, -1, -1, -1, 3, -1, 3, 0, 0, 0, 1, 1, 1, 1, 1, -1,		/*  32ks */
	// mono extended beyond legal br index
	//  1,2,2,0,0,0,1,1,1,1,1,1,1,1,1,-1,   	   /*  44ks single chan */
	//  0,2,2,0,0,0,0,0,0,0,0,0,0,0,0,-1,   	   /*  48ks */
	//  1,3,3,0,0,0,1,1,1,1,1,1,1,1,1,-1,   	   /*  32ks */
	// legal mono
	1, 2, 2, 0, 0, 0, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1,
	/*  44ks single chan */
	0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1,		/*  48ks */
	1, 3, 3, 0, 0, 0, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1,		/*  32ks */
};

/* bit allocation table look up */
/* table per mpeg spec tables 3b2a/b/c/d  /e is mpeg2 */
/* look_bat[abcd_index][4][16]  */
BYTE CDecompressMpeg::look_bat[5][4][16] = {
	/* LOOK_BATA */
	0, 1, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 0, 1, 2, 3, 4, 5,
	6, 7, 8, 9, 10, 11, 12, 13, 14, 17, 0, 1, 2, 3, 4, 5, 6, 17, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 2, 17, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* LOOK_BATB */
	0, 1, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 0, 1, 2, 3, 4, 5,
	6, 7, 8, 9, 10, 11, 12, 13, 14, 17, 0, 1, 2, 3, 4, 5, 6, 17, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 2, 17, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* LOOK_BATC */
	0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 4, 5, 6, 7, 8, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* LOOK_BATD */
	0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 4, 5, 6, 7, 8, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* LOOK_BATE */
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 4, 5, 6, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 2, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
};


/* look_nbat[abcd_index]][4] */
BYTE CDecompressMpeg::look_nbat[5][4] = {
	3, 8, 12, 4, 3, 8, 12, 7, 2, 0, 6, 0, 2, 0, 10, 0, 4, 0, 7, 19, 
};

/*-------------------------------------------------------------------------*/
#define UNPACK_N(n) s[k]	 =  m_cs_factor[i][k]*(bitget(n)-((1 << (n-1)) -1));   \
	s[k+64]  =  m_cs_factor[i][k]*(bitget(n)-((1 << (n-1)) -1));   \
	s[k+128] =  m_cs_factor[i][k]*(bitget(n)-((1 << (n-1)) -1));   \
	goto dispatch;
#define UNPACK_N2(n) bitget_check(3*n); 										\
	s[k]	 =  m_cs_factor[i][k]*(mac_bitget(n)-((1 << (n-1)) -1));   \
	s[k+64]  =  m_cs_factor[i][k]*(mac_bitget(n)-((1 << (n-1)) -1));   \
	s[k+128] =  m_cs_factor[i][k]*(mac_bitget(n)-((1 << (n-1)) -1));   \
	goto dispatch;
#define UNPACK_N3(n) bitget_check(2*n); 										\
	s[k]	 =  m_cs_factor[i][k]*(mac_bitget(n)-((1 << (n-1)) -1));   \
	s[k+64]  =  m_cs_factor[i][k]*(mac_bitget(n)-((1 << (n-1)) -1));   \
	bitget_check(n);										   \
	s[k+128] =  m_cs_factor[i][k]*(mac_bitget(n)-((1 << (n-1)) -1));   \
	goto dispatch;
#define UNPACKJ_N(n) tmp		=  (bitget(n)-((1 << (n-1)) -1)); 				\
	s[k]	   =  m_cs_factor[i][k]*tmp;					   \
	s[k+1]     =  m_cs_factor[i][k+1]*tmp;  				   \
	tmp 	   =  (bitget(n)-((1 << (n-1)) -1));  			   \
	s[k+64]    =  m_cs_factor[i][k]*tmp;					   \
	s[k+64+1]  =  m_cs_factor[i][k+1]*tmp;  				   \
	tmp 	   =  (bitget(n)-((1 << (n-1)) -1));  			   \
	s[k+128]   =  m_cs_factor[i][k]*tmp;					   \
	s[k+128+1] =  m_cs_factor[i][k+1]*tmp;  				   \
	k++;	   /* skip right chan dispatch */   			 \
	goto dispatch;
/*-------------------------------------------------------------------------*/

void CDecompressMpeg::L2table_init()
{
	int i, j, code;
	long stepL2[18] = {
		0, 3, 5, 7, 9, 15, 31, 63, 127, 255, 511, 1023, 2047, 4095, 8191,
		16383, 32767, 65535
	};
	//c_values (dequant)
	for (i = 1; i < 18; i++) {
		m_look_c_valueL2[i] = 2.0F / stepL2[i];
	}
	//scale factor table, scale by 32768 for 16 pcm output
	for (i = 0; i < 64; i++) {
		m_sf_table[i] = (float) (32768.0 * 2.0 * pow(2.0, -i / 3.0));
	}
	//grouped 3 level lookup table 5 bit token
	for (i = 0; i < 32; i++) {
		code = i;
		for (j = 0; j < 3; j++) {
			m_group3_table[i][j] = (char) ((code % 3) - 1);
			code /= 3;
		}
	}
	//grouped 5 level lookup table 7 bit token
	for (i = 0; i < 128; i++) {
		code = i;
		for (j = 0; j < 3; j++) {
			m_group5_table[i][j] = (char) ((code % 5) - 2);
			code /= 5;
		}
	}
	//grouped 9 level lookup table 10 bit token
	for (i = 0; i < 1024; i++) {
		code = i;
		for (j = 0; j < 3; j++) {
			m_group9_table[i][j] = (short) ((code % 9) - 4);
			code /= 9;
		}
	}
}

int CDecompressMpeg::L2decode_start(MPEG_HEADER* h)
{
	int i, j, k, bit_code, limit;
	int abcd_index;

	// compute abcd index for bit allo table selection
	if (h->version == 1) // MPEG-1
		abcd_index = lookqt[h->mode][h->fr_index][h->br_index];
	else
		abcd_index = 4;	// MPEG-2, MPEG-2.5
	if (abcd_index < 0)
		return 0;		// fail invalid Layer II bit rate index

	for (i = 0; i < 4; i++) {
		m_nbat[i] = look_nbat[abcd_index][i];
		for (j = 0; j < 16; j++) {
			m_bat[i][j] = look_bat[abcd_index][i][j];
		}
	}
	m_max_sb = m_nbat[0] + m_nbat[1] + m_nbat[2] + m_nbat[3];
	// compute nsb_limit
	m_nsb_limit = (m_option.freqLimit * 64L + m_frequency / 2) / m_frequency;
	// caller limit
	// limit = 0.94*(32>>reduction_code);
	limit = (32 >> m_option.reduction);
	if (limit > 8)
		limit--;
	if (m_nsb_limit > limit)
		m_nsb_limit = limit;
	if (m_nsb_limit > m_max_sb)
		m_nsb_limit = m_max_sb;

	if (h->mode != 3) {
		// adjust for 2 channel modes
		for (i = 0; i < 4; i++)
			m_nbat[i] *= 2;
		m_max_sb *= 2;
		m_nsb_limit *= 2;
	}

	// set sbt function
	bit_code = (m_option.convert & 8) ? 1 : 0;
	k = (h->mode == 3) ? 0 : (1 + m_option.convert);
	//!!!todo!!!m_sbt_proc = sbt_table[bit_code][m_option.reduction][k];//[2][3][2]
	m_sbt_wrk = bit_code * 3 * 2 + m_option.reduction * 2 + k;

	// clear sample buffer, unused sub bands must be 0
	for (i = 0; i < 2304; i++)
		((float *) m_sample)[i] = 0.0F;
	return 1;
}

int CDecompressMpeg::bat_bit_masterL2[18] = {
	0, 5, 7, 9, 10, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48
};

//////////////////////////////////////////////////////////////////////////
void CDecompressMpeg::unpack_ba()
{
	int i, j, k;
	int nstereo;
	int nbit[4] = {
		4, 4, 3, 2
	};

	m_bit_skip = 0;
	nstereo = m_stereo_sb;
	k = 0;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < m_nbat[i]; j++, k++) {
			bitget_check(4);
			m_ballo[k] = m_samp_dispatch[k] = m_bat[i][mac_bitget(nbit[i])];
			if (k >= m_nsb_limit)
				m_bit_skip += bat_bit_masterL2[m_samp_dispatch[k]];
			m_c_value[k] = m_look_c_valueL2[m_samp_dispatch[k]];
			if (--nstereo < 0) {
				m_ballo[k + 1] = m_ballo[k];
				m_samp_dispatch[k] += 18;	/* flag as joint */
				m_samp_dispatch[k + 1] = m_samp_dispatch[k];	/* flag for sf */
				m_c_value[k + 1] = m_c_value[k];
				k++;
				j++;
			}
		}
	}
	m_samp_dispatch[m_nsb_limit] = 37;	/* terminate the dispatcher with skip */
	m_samp_dispatch[k] = 36;	/* terminate the dispatcher */
}
//////////////////////////////////////////////////////////////////////////
void CDecompressMpeg::unpack_sfs()	/* unpack scale factor selectors */
{
	int i;

	for (i = 0; i < m_max_sb; i++) {
		bitget_check(2);
		if (m_ballo[i])
			m_sf_dispatch[i] = mac_bitget(2);
		else
			m_sf_dispatch[i] = 4;	/* no allo */
	}
	m_sf_dispatch[i] = 5;		/* terminate dispatcher */
}
//////////////////////////////////////////////////////////////////////////
void CDecompressMpeg::unpack_sf()		/* unpack scale factor */
{
	/* combine dequant and scale factors */
	int i;

	i = -1;
	dispatch:switch (m_sf_dispatch[++i]) {
			 case 0:
			 	/* 3 factors 012 */
			 	bitget_check(18);
			 	m_cs_factor[0][i] = m_c_value[i] * m_sf_table[mac_bitget(6)];
			 	m_cs_factor[1][i] = m_c_value[i] * m_sf_table[mac_bitget(6)];
			 	m_cs_factor[2][i] = m_c_value[i] * m_sf_table[mac_bitget(6)];
			 	goto dispatch;
			 case 1:
			 	/* 2 factors 002 */
			 	bitget_check(12);
			 	m_cs_factor[1][i] = m_cs_factor[0][i] = m_c_value[i] * m_sf_table[mac_bitget(6)];
			 	m_cs_factor[2][i] = m_c_value[i] * m_sf_table[mac_bitget(6)];
			 	goto dispatch;
			 case 2:
			 	/* 1 factor 000 */
			 	bitget_check(6);
			 	m_cs_factor[2][i] = m_cs_factor[1][i] = m_cs_factor[0][i] = m_c_value[i] * m_sf_table[mac_bitget(6)];
			 	goto dispatch;
			 case 3:
			 	/* 2 factors 022 */
			 	bitget_check(12);
			 	m_cs_factor[0][i] = m_c_value[i] * m_sf_table[mac_bitget(6)];
			 	m_cs_factor[2][i] = m_cs_factor[1][i] = m_c_value[i] * m_sf_table[mac_bitget(6)];
			 	goto dispatch;
			 case 4:
			 	/* no allo */
			 	/*-- m_cs_factor[2][i] = m_cs_factor[1][i] = m_cs_factor[0][i] = 0.0;  --*/
			 	goto dispatch;
			 case 5:
			 	/* all done */
		 ;
	}				/* end switch */
}

void CDecompressMpeg::unpack_samp()	/* unpack samples */
{
	int i, j, k;
	float* s;
	int n;
	long tmp;

	s = (float *) m_sample;
	for (i = 0; i < 3; i++) {
		/* 3 groups of scale factors */
		for (j = 0; j < 4; j++) {
			k = -1;
			dispatch:switch (m_samp_dispatch[++k]) {
					 case 0:
					 	s[k + 128] = s[k + 64] = s[k] = 0.0F;
					 	goto dispatch;
					 case 1:
					 	/* 3 levels grouped 5 bits */
					 	bitget_check(5);
					 	n = mac_bitget(5);
					 	s[k] = m_cs_factor[i][k] * m_group3_table[n][0];
					 	s[k + 64] = m_cs_factor[i][k] * m_group3_table[n][1];
					 	s[k + 128] = m_cs_factor[i][k] * m_group3_table[n][2];
					 	goto dispatch;
					 case 2:
					 	/* 5 levels grouped 7 bits */
					 	bitget_check(7);
					 	n = mac_bitget(7);
					 	s[k] = m_cs_factor[i][k] * m_group5_table[n][0];
					 	s[k + 64] = m_cs_factor[i][k] * m_group5_table[n][1];
					 	s[k + 128] = m_cs_factor[i][k] * m_group5_table[n][2];
					 	goto dispatch;
					 case 3:
					 	UNPACK_N2(3)	/* 7 levels */
					 case 4:
					 	/* 9 levels grouped 10 bits */
					 	bitget_check(10);
					 	n = mac_bitget(10);
					 	s[k] = m_cs_factor[i][k] * m_group9_table[n][0];
					 	s[k + 64] = m_cs_factor[i][k] * m_group9_table[n][1];
					 	s[k + 128] = m_cs_factor[i][k] * m_group9_table[n][2];
					 	goto dispatch;
					 case 5:
					 	UNPACK_N2(4)	/* 15 levels */
					 case 6:
					 	UNPACK_N2(5)	/* 31 levels */
					 case 7:
					 	UNPACK_N2(6)	/* 63 levels */
					 case 8:
					 	UNPACK_N2(7)	/* 127 levels */
					 case 9:
					 	UNPACK_N2(8)	/* 255 levels */
					 case 10:
					 	UNPACK_N3(9)	/* 511 levels */
					 case 11:
					 	UNPACK_N3(10)	/* 1023 levels */
					 case 12:
					 	UNPACK_N3(11)	/* 2047 levels */
					 case 13:
					 	UNPACK_N3(12)	/* 4095 levels */
					 case 14:
					 	UNPACK_N(13)	/* 8191 levels */
					 case 15:
					 	UNPACK_N(14)	/* 16383 levels */
					 case 16:
					 	UNPACK_N(15)	/* 32767 levels */
					 case 17:
					 	UNPACK_N(16)	/* 65535 levels */
					 	/* -- joint ---- */
					 case 18 + 0:
					 	s[k + 128 + 1] = s[k + 128] = s[k + 64 + 1] = s[k + 64] = s[k + 1] = s[k] = 0.0F;
					 	k++;		/* skip right chan dispatch */
					 	goto dispatch;
					 case 18 + 1:
					 	/* 3 levels grouped 5 bits */
					 	n = bitget(5);
					 	s[k] = m_cs_factor[i][k] * m_group3_table[n][0];
					 	s[k + 1] = m_cs_factor[i][k + 1] * m_group3_table[n][0];
					 	s[k + 64] = m_cs_factor[i][k] * m_group3_table[n][1];
					 	s[k + 64 + 1] = m_cs_factor[i][k + 1] * m_group3_table[n][1];
					 	s[k + 128] = m_cs_factor[i][k] * m_group3_table[n][2];
					 	s[k + 128 + 1] = m_cs_factor[i][k + 1] * m_group3_table[n][2];
					 	k++;		/* skip right chan dispatch */
					 	goto dispatch;
					 case 18 + 2:
					 	/* 5 levels grouped 7 bits */
					 	n = bitget(7);
					 	s[k] = m_cs_factor[i][k] * m_group5_table[n][0];
					 	s[k + 1] = m_cs_factor[i][k + 1] * m_group5_table[n][0];
					 	s[k + 64] = m_cs_factor[i][k] * m_group5_table[n][1];
					 	s[k + 64 + 1] = m_cs_factor[i][k + 1] * m_group5_table[n][1];
					 	s[k + 128] = m_cs_factor[i][k] * m_group5_table[n][2];
					 	s[k + 128 + 1] = m_cs_factor[i][k + 1] * m_group5_table[n][2];
					 	k++;		/* skip right chan dispatch */
					 	goto dispatch;
					 case 18 + 3:
					 	UNPACKJ_N(3)	/* 7 levels */
					 case 18 + 4:
					 	/* 9 levels grouped 10 bits */
					 	n = bitget(10);
					 	s[k] = m_cs_factor[i][k] * m_group9_table[n][0];
					 	s[k + 1] = m_cs_factor[i][k + 1] * m_group9_table[n][0];
					 	s[k + 64] = m_cs_factor[i][k] * m_group9_table[n][1];
					 	s[k + 64 + 1] = m_cs_factor[i][k + 1] * m_group9_table[n][1];
					 	s[k + 128] = m_cs_factor[i][k] * m_group9_table[n][2];
					 	s[k + 128 + 1] = m_cs_factor[i][k + 1] * m_group9_table[n][2];
					 	k++;		/* skip right chan dispatch */
					 	goto dispatch;
					 case 18 + 5:
					 	UNPACKJ_N(4)	/* 15 levels */
					 case 18 + 6:
					 	UNPACKJ_N(5)	/* 31 levels */
					 case 18 + 7:
					 	UNPACKJ_N(6)	/* 63 levels */
					 case 18 + 8:
					 	UNPACKJ_N(7)	/* 127 levels */
					 case 18 + 9:
					 	UNPACKJ_N(8)	/* 255 levels */
					 case 18 + 10:
					 	UNPACKJ_N(9)	/* 511 levels */
					 case 18 + 11:
					 	UNPACKJ_N(10)	/* 1023 levels */
					 case 18 + 12:
					 	UNPACKJ_N(11)	/* 2047 levels */
					 case 18 + 13:
					 	UNPACKJ_N(12)	/* 4095 levels */
					 case 18 + 14:
					 	UNPACKJ_N(13)	/* 8191 levels */
					 case 18 + 15:
					 	UNPACKJ_N(14)	/* 16383 levels */
					 case 18 + 16:
					 	UNPACKJ_N(15)	/* 32767 levels */
					 case 18 + 17:
					 	UNPACKJ_N(16)	/* 65535 levels */
					 	/* -- end of dispatch -- */
					 case 37:
					 	bitget_skip(m_bit_skip);
					 case 36:
					 	s += 3 * 64;
			}			/* end switch */
		}				/* end j loop */
	}				/* end i loop */
}
//////////////////////////////////////////////////////////////////////////
void CDecompressMpeg::L2decode_frame(MPEG_HEADER* h, byte* mpeg, byte* pcm)
{
	int crc_size;

	crc_size = (h->error_prot) ? 2 : 0;
	bitget_init(mpeg + 4 + crc_size);

	m_stereo_sb = look_joint[(h->mode << 2) + h->mode_ext];
	unpack_ba();		// unpack bit allocation
	unpack_sfs();		// unpack scale factor selectors
	unpack_sf();		// unpack scale factor
	unpack_samp();		// unpack samples

	//!!!todo!!!m_sbt_proc(m_sample, pcm, 36);

	switch (m_sbt_wrk) {
	case 0:
		sbt_mono((float *) m_sample, pcm, 36);
		break;
	case 1:
		sbt_dual((float *) m_sample, pcm, 36);
		break;
	case 2:
		sbt16_mono((float *) m_sample, pcm, 36);
		break;
	case 3:
		sbt16_dual((float *) m_sample, pcm, 36);
		break;
	case 4:
		sbt8_mono((float *) m_sample, pcm, 36);
		break;
	case 5:
		sbt8_dual((float *) m_sample, pcm, 36);
		break;
	case 6:
		sbtB_mono((float *) m_sample, pcm, 36);
		break;
	case 7:
		sbtB_dual((float *) m_sample, pcm, 36);
		break;
	case 8:
		sbtB16_mono((float *) m_sample, pcm, 36);
		break;
	case 9:
		sbtB16_dual((float *) m_sample, pcm, 36);
		break;
	case 10:
		sbtB8_mono((float *) m_sample, pcm, 36);
		break;
	case 11:
		sbtB8_dual((float *) m_sample, pcm, 36);
		break;
	}
}
