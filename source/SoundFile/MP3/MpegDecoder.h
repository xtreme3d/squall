//-----------------------------------------------------------------------------
//	Класс декодера Audio - Mpeg Layer 1,2,3
//	Копонент звукового двигателя Шквал
//	команда		: AntiTank
//	разработчик	: Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------
#ifndef _MPEG_DECOMPRESSOR_H_INCLUDED_
#define _MPEG_DECOMPRESSOR_H_INCLUDED_

// включения
#include <windows.h>
#include "../AbstractDecoder.h"
#include "../AbstractSoundFile.h"
#include "MpegHuff.h"

// коды ошибок
#define MP3_ERROR_UNKNOWN				1
#define MP3_ERROR_INVALID_PARAMETER		2
#define MP3_ERROR_INVALID_SYNC			3
#define MP3_ERROR_INVALID_HEADER		4
#define MP3_ERROR_OUT_OF_BUFFER			5

#define no_bits							0
#define one_shot						1
#define no_linbits						2
#define have_linbits					3
#define quad_a							4
#define quad_b							5

#define NBUF (8*1024)
#define BUF_TRIGGER (NBUF-1500)
#define ISMAX 32
#define MAXBITS 9
#define GLOBAL_GAIN_SCALE (4*15)

#ifndef min
#define max(a,b)	(((a) > (b)) ? (a) : (b))
#define min(a,b)	(((a) < (b)) ? (a) : (b))
#endif

// определение структур
typedef struct {
	DWORD part2_3_length;
	DWORD big_values;
	DWORD global_gain;
	DWORD scalefac_compress;
	DWORD window_switching_flag;
	DWORD block_type;
	DWORD mixed_block_flag;
	DWORD table_select[3];
	DWORD subblock_gain[3];
	DWORD region0_count;
	DWORD region1_count;
	DWORD preflag;
	DWORD scalefac_scale;
	DWORD count1table_select;
} GR_INFO;

typedef struct {
	DWORD main_data_begin;
	DWORD private_bits;

	DWORD scfsi[2];	/* 4 bit flags [ch] */
	GR_INFO gr[2][2];	/* [gr][ch] */
} SIDE_INFO;

typedef struct {
	int l[23];			/* [cb] */
	int s[3][13];		/* [window][cb] */
} SCALE_FACTOR;

typedef struct {
	int cbtype;			/* long=0 short=1 */
	int cbmax;			/* max crit band */
	int lb_type;			/* long block type 0 1 3 */
	int cbs0;			/* short band start index 0 3 12 (12=no shorts */
	int ncbl;			/* number long cb's 0 8 21 */
	int cbmax_s[3];		/* cbmax by individual short blocks */
} CB_INFO;

typedef struct {
	int nr[3];
	int slen[3];
	int intensity_scale;
} IS_SF_INFO;

typedef union {
	int s;
	float x;
} SAMPLE;

typedef struct {
	int version;		//1:MPEG-1, 2:MPEG-2, 3:MPEG-2.5
	int layer;			//1:Layer1, 2:Layer2, 3:Layer3
	int error_prot;		//1:CRC on, 0:CRC off
	int br_index;
	int fr_index;
	int padding;
	int extension;
	int mode;
	int mode_ext;
	int copyright;
	int original;
	int emphasis;
} MPEG_HEADER;

typedef struct {
	int reduction;
	int convert;
	int freqLimit;
} MPEG_DECODE_OPTION;

typedef struct {
	MPEG_HEADER header;
	int channels;
	int bitsPerSample;
	int frequency;
	int bitRate;
	int HeadBitRate;

	int frames;
	int skipSize;
	int dataSize;

	int minInputSize;
	int maxInputSize;
	int outputSize;
} MPEG_DECODE_INFO;

typedef struct {
	MPEG_HEADER header;
	int bitRate;

	void* inputBuf;
	int inputSize;
	void* outputBuf;
	int outputSize;
} MPEG_DECODE_PARAM;

typedef struct {
	MPEG_HEADER header;
	DWORD curBitRate;
	DWORD curFrameSize;
	DWORD samplesInFrame;
	DWORD outputSize;
} MPEG_HEADER_INFO;

typedef struct {
	HUFF_ELEMENT* table;
	int linbits;
	int ncase;
} HUFF_SETUP;

typedef struct {
	int l[23];
	int s[14];
} SFBT;

class CAbstractSoundFile;
//-----------------------------------------------------------------------------
//	Класс декодера
//-----------------------------------------------------------------------------
class CDecompressMpeg : public CAbstractDecompressor {
protected:
	static int look_joint[16];  	   // исп I1dec, I2Dec
	static int bat_bit_masterL1[18];   // исп I1dec
	static int bat_bit_masterL2[18];   // исп I2dec
	static BYTE look_nbat[5][4];		// исп I3dec
	static BYTE look_bat[5][4][16]; 	// исп I2dec
	static char lookqt[4][3][16];   	// исп I2dec
	static BYTE quad_table_a[64][2];	// исп I3dec
	static HUFF_ELEMENT huff_table_0[4];		// исп I3dec
	static float wincoef[264];  		 // mpeg_decoder
	static int br_tbl[3][3][16];	   // mpeg_decoder
	static int fr_tbl[3][4];		   // mpeg_decoder
	static SFBT sfBandTable[3][3];  	// I3dec
	static int slen_table[16][2];      // I3dec
	static int nr_table[6][3][4];      // I3dec
	static HUFF_SETUP table_look[34];   	  // I3dec
	static int pretab[2][22];   	   // I3dec

	int m_stereo_sb;							// i1dec, i2dec
	int m_max_sb;								// i1dec, i2dec

	float m_sf_table[64];						// инициализируемое в L2table_init
	float m_look_c_valueL2[18];					// инициализируемое в L2table_init
	char m_group3_table[32][3];					// инициализируемое в L2table_init
	char m_group5_table[128][3];				// инициализируемое в L2table_init
	short m_group9_table[1024][3];				// инициализируемое в L2table_init
	int m_nbat[4];								// l1 l2
	int m_bat[4][16];							// l2

	int m_ballo[64];							// l1, l2
	DWORD m_samp_dispatch[66];					// l1, l2
	float m_c_value[64];						// l1, l2
	DWORD m_sf_dispatch[66];					// l2
	float m_cs_factor[3][64];					// l1, l2

	int m_bit_skip;								// l1, l2

	float csa[8][2];							// l3
	float lr[2][8][2];							// l3
	float lr2[2][2][64][2];						// l3

	SAMPLE m_sample[2][2][576];		//- sample union of int/float  sample[ch][gr][576]
	int m_nsb_limit;
	int m_channels;					//(mode == 3) ? 1 : 2
	int m_ms_mode;					// l3
	int m_is_mode;					// l3
	int m_sfBandIndex[2][22];		// [long/short][cb]
	int m_nBand[2][22];
	int m_band_limit;
	int m_band_limit21;				// limit for sf band 21
	int m_band_limit12;				// limit for sf band 12 short
	int m_band_limit_nsb;
	int m_ncbl_mixed;

	SIDE_INFO m_side_info;
	SCALE_FACTOR m_scale_fac[2][2];	// [gr][ch]
	CB_INFO m_cb_info[2][2];	// [gr][ch]
	IS_SF_INFO m_is_sf_info;

	float win[4][36];
	float look_global[256 + 2 + 4];
	float look_scale[2][4][32];
	float look_pow[2 * ISMAX];
	float look_subblock[8];
	float re_buf[192][3];

	int m_gr;
	int m_main_pos_bit;
	BYTE m_buf[NBUF];
	int m_buf_ptr0;								// ?
	int m_buf_ptr1;								// ?
	int m_nsamp[2][2];							// must start = 0, for m_nsamp[igr_prev]
	float m_yout[576];							// hybrid out, sbt in

	MPEG_DECODE_OPTION m_option;

	int m_frequency;

	// layer 1
	float m_look_c_valueL1[18];
	float* m_cs_factorL1;
	int m_nbatL1;

	int m_last_error;
	int m_frame_size;
	int m_pcm_size;
	//   int			m_enableEQ;
	//   float  		m_equalizer[32];


	int m_xform_wrk;
	int m_sbt_wrk;

	/* circular window buffers */
	int vb_ptr;
	int vb2_ptr;
	float vbuf[512];
	float vbuf2[512];

	/*------ 18 point xform -------*/
	float w[18];
	float w2[9];
	float coef[9][4];

	float v[6];
	float v2[3];
	float coef87;

	float coef32[31];
	unsigned int bitbuf;
	int bits;
	unsigned char* bs_ptr;
	unsigned char* bs_ptr0;
	unsigned char* bs_ptr_end;

	void alias_init();
	void antialias(float x[], int n);
	void msis_init();
	void msis_init1();
	void msis_init2();
	void ms_process(float x[], int n);
	void is_process1(float x[], SCALE_FACTOR* sf, CB_INFO cb_info[2],
		int nsamp);
	void is_process2(float x[], SCALE_FACTOR* sf, CB_INFO cb_info[2],
		int nsamp);
	void hwin_init();
	void huffman(int xy[], int n, int ntable);
	int huffman_quad(int vwxy[], int n, int nbits, int ntable);
	int hybrid(float xin[], float xprev[], float y[], int btype, int nlong,
		int ntot, int nprev);
	int hybrid_sum(float xin[], float xin_left[], float y[], int btype,
		int nlong, int ntot);
	void sum_f_bands(float a[], float b[], int n);
	void freq_invert(float y[], int n);
	void quant_init();
	void dequant(SAMPLE Sample[], int gr, int ch);
	int L3get_side_info1();
	int L3get_side_info2(int gr);
	void L3get_scale_factor1(int gr, int ch);
	void L3get_scale_factor2(int gr, int ch);
	void L3decode_reset();
	void L3decode_main(MPEG_HEADER* h, byte* pcm, int gr);
	void imdct_init(void);
	void imdct18(float f[]);
	void imdct6_3(float f[]);
	void forward_bf(int m, int n, float x[], float f[], float coef[]);
	void back_bf(int m, int n, float x[], float f[]);
	void fdct_init(void);
	void fdct32(float*, float*);
	void fdct32_dual(float*, float*);
	void fdct32_dual_mono(float*, float*);
	void fdct16(float*, float*);
	void fdct16_dual(float*, float*);
	void fdct16_dual_mono(float*, float*);
	void fdct8(float*, float*);
	void fdct8_dual(float*, float*);
	void fdct8_dual_mono(float*, float*);
	void window(float* vbuf, int vb_ptr, short* pcm);
	void window_dual(float* vbuf, int vb_ptr, short* pcm);
	void window16(float* vbuf, int vb_ptr, short* pcm);
	void window16_dual(float* vbuf, int vb_ptr, short* pcm);
	void window8(float* vbuf, int vb_ptr, short* pcm);
	void window8_dual(float* vbuf, int vb_ptr, short* pcm);
	void windowB(float* vbuf, int vb_ptr, unsigned char* pcm);
	void windowB_dual(float* vbuf, int vb_ptr, unsigned char* pcm);
	void windowB16(float* vbuf, int vb_ptr, unsigned char* pcm);
	void windowB16_dual(float* vbuf, int vb_ptr, unsigned char* pcm);
	void windowB8(float* vbuf, int vb_ptr, unsigned char* pcm);
	void windowB8_dual(float* vbuf, int vb_ptr, unsigned char* pcm);
	void bitget_init(BYTE* buf);
	void bitget_init_end(BYTE* buf_end);
	int bitget_overrun();
	int bitget_bits_used();
	void bitget_check(int n);
	int bitget(int n);
	void bitget_skip(int n);
	int bitget_lb(int n);
	int bitget2(int n);
	void bitget_purge(int n);
	void mac_bitget_check(int n);
	int mac_bitget(int n);
	int mac_bitget_1bit();
	int mac_bitget2(int n);
	void mac_bitget_purge(int n);
	int extractInt4(BYTE* buf);
	void sbt_init(void);
	void sbt_mono(float* sample, void* pcm, int ch);
	void sbt_mono_L3(float* sample, void* pcm, int ch);
	void sbt_dual(float* sample, void* pcm, int ch);
	void sbt_dual_L3(float* sample, void* pcm, int ch);
	void sbt_dual_mono(float* sample, void* in_pcm, int n);
	void sbt_dual_left(float* sample, void* in_pcm, int n);
	void sbt_dual_right(float* sample, void* in_pcm, int n);
	void sbt16_mono(float* sample, void* pcm, int ch);
	void sbt16_mono_L3(float* sample, void* pcm, int ch);
	void sbt16_dual(float* sample, void* pcm, int ch);
	void sbt16_dual_L3(float* sample, void* pcm, int ch);
	void sbt16_dual_mono(float* sample, void* in_pcm, int n);
	void sbt16_dual_left(float* sample, void* in_pcm, int n);
	void sbt16_dual_right(float* sample, void* in_pcm, int n);
	void sbt8_mono(float* sample, void* pcm, int ch);
	void sbt8_mono_L3(float* sample, void* pcm, int ch);
	void sbt8_dual(float* sample, void* pcm, int ch);
	void sbt8_dual_L3(float* sample, void* pcm, int ch);
	void sbt8_dual_mono(float* sample, void* in_pcm, int n);
	void sbt8_dual_left(float* sample, void* in_pcm, int n);
	void sbt8_dual_right(float* sample, void* in_pcm, int n);
	void sbtB_mono(float* sample, void* pcm, int ch);
	void sbtB_mono_L3(float* sample, void* pcm, int ch);
	void sbtB_dual(float* sample, void* pcm, int ch);
	void sbtB_dual_L3(float* sample, void* pcm, int ch);
	void sbtB_dual_mono(float* sample, void* in_pcm, int n);
	void sbtB_dual_left(float* sample, void* in_pcm, int n);
	void sbtB_dual_right(float* sample, void* in_pcm, int n);
	void sbtB16_mono(float* sample, void* pcm, int ch);
	void sbtB16_mono_L3(float* sample, void* pcm, int ch);
	void sbtB16_dual(float* sample, void* pcm, int ch);
	void sbtB16_dual_L3(float* sample, void* pcm, int ch);
	void sbtB16_dual_mono(float* sample, void* in_pcm, int n);
	void sbtB16_dual_left(float* sample, void* in_pcm, int n);
	void sbtB16_dual_right(float* sample, void* in_pcm, int n);
	void sbtB8_mono(float* sample, void* pcm, int ch);
	void sbtB8_mono_L3(float* sample, void* pcm, int ch);
	void sbtB8_dual(float* sample, void* pcm, int ch);
	void sbtB8_dual_L3(float* sample, void* pcm, int ch);
	void sbtB8_dual_mono(float* sample, void* in_pcm, int n);
	void sbtB8_dual_left(float* sample, void* in_pcm, int n);
	void sbtB8_dual_right(float* sample, void* in_pcm, int n);
	void xform_mono(void* pcm, int igr);
	void xform_dual(void* pcm, int igr);
	void xform_dual_mono(void* pcm, int igr);
	void xform_dual_right(void* pcm, int igr);
	void unpack_ba();
	void unpack_sfs();
	void unpack_sf();
	void unpack_samp();
	void unpack_baL1();
	void unpack_sfL1();
	void unpack_sampL1();
	void L1table_init();
	void L2table_init();
	void L3table_init();
	int L1decode_start(MPEG_HEADER* h);
	int L2decode_start(MPEG_HEADER* h);
	int L3decode_start(MPEG_HEADER* h);
	void L1decode_frame(MPEG_HEADER* h, BYTE* mpeg, BYTE* pcm);
	void L2decode_frame(MPEG_HEADER* h, BYTE* mpeg, BYTE* pcm);
	void L3decode_frame(MPEG_HEADER* h, BYTE* mpeg, BYTE* pcm);
	void mp3DecodeInit();
	int mp3GetLastError();
	int mp3GetHeader(BYTE* buf, MPEG_HEADER* h);
	bool mp3GetHeaderInfo(BYTE* buffer, MPEG_HEADER_INFO* info);
	int mp3SetDecodeOption(MPEG_DECODE_OPTION* option);
	void mp3GetDecodeOption(MPEG_DECODE_OPTION* option);
	int mp3SetEqualizer(int* value);
	int mp3FindSync(BYTE* buf, int size, int* sync);
	int mp3GetDecodeInfo(BYTE* mpeg, int size, MPEG_DECODE_INFO* info,
		int decFlag);
	int mp3DecodeStart(BYTE* buf, int size);
	int mp3DecodeFrame(MPEG_DECODE_PARAM* param);
	void mp3Reset(void);
	int mp3seek(DWORD pos);

	int _version;
	int _layer;
	int _br_index;
	int _fr_index;
	int _mode;

	WORD _channels;					// количество каналов в исходном файле
	DWORD _frequency;					// частота звука
	DWORD _bitrate;					// скорость передачи
	bool _vbr;						// флаг наличия переменной скорости передачи
	DWORD* _vbrFrameOffTable;			// таблица смещений
	DWORD _frames;					// количество фреймов в файле
	DWORD _minFrameSize;				// минимальная длинна одного фрейма в байтах
	DWORD _maxFrameSize;				// максимальная длинна одного фрейма в байтах
	DWORD _curFrameSize;				// текущий размер фрейма
	DWORD _samplesInFrame;			// количество семплов в фрейме
	DWORD _samplesInFile;				// количество семплов в файле

	DWORD _slotSize;
	double _bitPerFrame;				// количество бит в фрейме
	DWORD _curFrame;					// текущий фрейм
	DWORD _curSampleOffset;			// текущее смещение в семплах

	MPEG_HEADER _mpegH;			// структура заголовка фрейма
	MPEG_HEADER_INFO _mpegHI;		// структура расширенного заголовка
	MPEG_DECODE_INFO _mpegDI;		// структура для получения информации о фрейме
	MPEG_DECODE_PARAM _mpegDP;		// структура для декодирования фрейма

	short _sampleBuffer[1152 * 2];		// промежуточный исходящий буфер 
	BYTE* _frameBuffer;				// промежуточный входящий буфер

public:
	// конструктор/деструктор
	CDecompressMpeg(WAVEFORMATEX* pcm_format, bool& flag,
		CAbstractSoundFile* a);
	~CDecompressMpeg();

	// получение семплов в моно режиме
	DWORD GetMonoSamples(void* buffer, DWORD start, DWORD length, bool loop);

	// получение семплов в стерео режиме
	DWORD GetStereoSamples(void* buffer, DWORD start, DWORD length, bool loop);

	// заполнение тишиной в моно режиме
	DWORD GetMonoMute(void* buffer, DWORD length);

	// заполнение тишиной в стерео режиме
	DWORD GetStereoMute(void* buffer, DWORD length);

	// получение количества семплов в файле
	DWORD GetSamplesInFile(void);

	// получение длинны трека в байтах моно режим
	DWORD GetRealMonoDataSize();

	// получение длинны трека в байтах стерео режим
	DWORD GetRealStereoDataSize();
};
#endif
