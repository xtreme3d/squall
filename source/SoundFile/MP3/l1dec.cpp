#include <math.h>
#include "MpegDecoder.h"

// unpack samples
#define UNPACKL1_N(n) \
	s[k] = m_cs_factorL1[k]*(bitget(n)-((1 << (n-1)) -1));	 \
	goto dispatch;
#define UNPACKL1J_N(n) \
	tmp 	   =  (bitget(n)-((1 << (n-1)) -1));  		   \
	s[k]	   =  m_cs_factorL1[k]*tmp; 					 \
	s[k+1]     =  m_cs_factorL1[k+1]*tmp;   				 \
	k++;												   \
	goto dispatch;

/* lookup stereo sb's by mode+ext */
int CDecompressMpeg::look_joint[16] = {
	64, 64, 64, 64,		/* stereo */
	2 * 4, 2 * 8, 2 * 12, 2 * 16,	/* joint */
	64, 64, 64, 64,		/* dual */
	32, 32, 32, 32,		/* mono */
};

int CDecompressMpeg::bat_bit_masterL1[18] = {
	0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
};

// инициализация первого слоя
void CDecompressMpeg::L1table_init()
{
	int i, stepL1;

	for (stepL1 = 4, i = 1; i < 16; i++, stepL1 <<= 1) {
		m_look_c_valueL1[i] = (float) (2.0 / (stepL1 - 1));
	}
}

// запуск первого слоя
int CDecompressMpeg::L1decode_start(MPEG_HEADER* h)
{
	int i, k, bit_code, limit;

	/*- caller limit -*/
	m_nbatL1 = 32;
	m_max_sb = m_nbatL1;
	m_nsb_limit = (m_option.freqLimit * 64L + m_frequency / 2) / m_frequency;

	/*---- limit = 0.94*(32>>reduction_code);  ----*/
	limit = (32 >> m_option.reduction);
	if (limit > 8)
		limit--;
	if (m_nsb_limit > limit)
		m_nsb_limit = limit;
	if (m_nsb_limit > m_max_sb)
		m_nsb_limit = m_max_sb;

	if (h->mode != 3) {
		/* adjust for 2 channel modes */
		m_nbatL1 *= 2;
		m_max_sb *= 2;
		m_nsb_limit *= 2;
	}
	/* set sbt function */
	bit_code = (m_option.convert & 8) ? 1 : 0;
	k = (h->mode == 3) ? 0 : (1 + m_option.convert);

	//!!!todo!!m_sbt_proc = sbt_table[bit_code][m_option.reduction][k];//[2][3][2]

	m_sbt_wrk = bit_code * 3 * 2 + m_option.reduction * 2 + k;

	for (i = 0; i < 768; i++)
		((float *) m_sample)[i] = 0.0F;

	return 1;
}

void CDecompressMpeg::unpack_baL1()
{
	int j;
	int nstereo;

	m_bit_skip = 0;
	nstereo = m_stereo_sb;

	for (j = 0; j < m_nbatL1; j++) {
		bitget_check(4);
		m_ballo[j] = m_samp_dispatch[j] = mac_bitget(4);
		if (j >= m_nsb_limit)
			m_bit_skip += bat_bit_masterL1[m_samp_dispatch[j]];
		m_c_value[j] = m_look_c_valueL1[m_samp_dispatch[j]];
		if (--nstereo < 0) {
			m_ballo[j + 1] = m_ballo[j];
			// flag as joint
			m_samp_dispatch[j] += 15;
			// flag for sf
			m_samp_dispatch[j + 1] = m_samp_dispatch[j];
			m_c_value[j + 1] = m_c_value[j];
			j++;
		}
	}
	// terminate with bit skip and end
	m_samp_dispatch[m_nsb_limit] = 31;
	m_samp_dispatch[j] = 30;
}

// unpack scale factor
// combine dequant and scale factors
void CDecompressMpeg::unpack_sfL1()
{
	int i;

	for (i = 0; i < m_nbatL1; i++) {
		if (m_ballo[i]) {
			bitget_check(6);
			m_cs_factorL1[i] = m_c_value[i] * m_sf_table[mac_bitget(6)];
		}
	}
}

void CDecompressMpeg::unpack_sampL1()
{
	int j, k;
	float* s;
	long tmp;

	s = (float *) m_sample;
	for (j = 0; j < 12; j++) {
		k = -1;
		dispatch:
		switch (m_samp_dispatch[++k]) {
		case 0:
			s[k] = 0.0F;
			goto dispatch;
		case 1:
			UNPACKL1_N(2)	/*  3 levels */
		case 2:
			UNPACKL1_N(3)	/*  7 levels */
		case 3:
			UNPACKL1_N(4)	/* 15 levels */
		case 4:
			UNPACKL1_N(5)	/* 31 levels */
		case 5:
			UNPACKL1_N(6)	/* 63 levels */
		case 6:
			UNPACKL1_N(7)	/* 127 levels */
		case 7:
			UNPACKL1_N(8)	/* 255 levels */
		case 8:
			UNPACKL1_N(9)	/* 511 levels */
		case 9:
			UNPACKL1_N(10)	/* 1023 levels */
		case 10:
			UNPACKL1_N(11)	/* 2047 levels */
		case 11:
			UNPACKL1_N(12)	/* 4095 levels */
		case 12:
			UNPACKL1_N(13)	/* 8191 levels */
		case 13:
			UNPACKL1_N(14)	/* 16383 levels */
		case 14:
			UNPACKL1_N(15)	/* 32767 levels */
			/* -- joint ---- */
		case 15 + 0:
			s[k + 1] = s[k] = 0.0F;
			k++;		/* skip right chan dispatch */
			goto dispatch;
			/* -- joint ---- */
		case 15 + 1:
			UNPACKL1J_N(2)	/*  3 levels */
		case 15 + 2:
			UNPACKL1J_N(3)	/*  7 levels */
		case 15 + 3:
			UNPACKL1J_N(4)	/* 15 levels */
		case 15 + 4:
			UNPACKL1J_N(5)	/* 31 levels */
		case 15 + 5:
			UNPACKL1J_N(6)	/* 63 levels */
		case 15 + 6:
			UNPACKL1J_N(7)	/* 127 levels */
		case 15 + 7:
			UNPACKL1J_N(8)	/* 255 levels */
		case 15 + 8:
			UNPACKL1J_N(9)	/* 511 levels */
		case 15 + 9:
			UNPACKL1J_N(10)	/* 1023 levels */
		case 15 + 10:
			UNPACKL1J_N(11)	/* 2047 levels */
		case 15 + 11:
			UNPACKL1J_N(12)	/* 4095 levels */
		case 15 + 12:
			UNPACKL1J_N(13)	/* 8191 levels */
		case 15 + 13:
			UNPACKL1J_N(14)	/* 16383 levels */
		case 15 + 14:
			UNPACKL1J_N(15)	/* 32767 levels */

			/* -- end of dispatch -- */
		case 31:
			bitget_skip(m_bit_skip);
		case 30:
			s += 64;
		}				/* end switch */
	}				/* end j loop */
}

void CDecompressMpeg::L1decode_frame(MPEG_HEADER* h, byte* mpeg, byte* pcm)
{
	int crc_size;

	crc_size = (h->error_prot) ? 2 : 0;
	bitget_init(mpeg + 4 + crc_size);

	m_stereo_sb = look_joint[(h->mode << 2) + h->mode_ext];
	unpack_baL1();		/* unpack bit allocation */
	unpack_sfL1();		/* unpack scale factor */
	unpack_sampL1();	/* unpack samples */

	switch (m_sbt_wrk) {
	case 0:
		sbt_mono((float *) m_sample, pcm, 12);
		break;
	case 1:
		sbt_dual((float *) m_sample, pcm, 12);
		break;
	case 2:
		sbt16_mono((float *) m_sample, pcm, 12);
		break;
	case 3:
		sbt16_dual((float *) m_sample, pcm, 12);
		break;
	case 4:
		sbt8_mono((float *) m_sample, pcm, 12);
		break;
	case 5:
		sbt8_dual((float *) m_sample, pcm, 12);
		break;
	case 6:
		sbtB_mono((float *) m_sample, pcm, 12);
		break;
	case 7:
		sbtB_dual((float *) m_sample, pcm, 12);
		break;
	case 8:
		sbtB16_mono((float *) m_sample, pcm, 12);
		break;
	case 9:
		sbtB16_dual((float *) m_sample, pcm, 12);
		break;
	case 10:
		sbtB8_mono((float *) m_sample, pcm, 12);
		break;
	case 11:
		sbtB8_dual((float *) m_sample, pcm, 12);
		break;
	}
}
