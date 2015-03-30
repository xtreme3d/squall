//-----------------------------------------------------------------------------
//	Класс декодера Ogg
//	Копонент звукового двигателя Шквал
//	команда		: AntiTank
//	разработчик	: Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------
#ifndef _OGG_DECOMPRESSOR_H_INCLUDED_
#define _OGG_DECOMPRESSOR_H_INCLUDED_

// включения
#include "../AbstractDecoder.h"
#include "../AbstractSoundFile.h"
#include <vorbis/vorbisfile.h>

class CAbstractSoundFile;

#define SAMPLES_IN_TEMP_BUFFER 1024

//-----------------------------------------------------------------------------
//	Класс декодера Ogg
//-----------------------------------------------------------------------------
class CDecompressOgg : public CAbstractDecompressor {
protected:
	OggVorbis_File* _vf;
	void* _tempBuffer;
	int _tempBufferSize;

	int GetSamples(void* buffer, int remaining, int out_channels);

public:
	// конструктор/деструктор
	CDecompressOgg(WAVEFORMATEX* pcm_format, bool& flag, CAbstractSoundFile* a);
	~CDecompressOgg();

	// получение данных из декодера в моно режиме
	DWORD GetMonoSamples(void* buffer, DWORD start, DWORD length, bool loop);

	// получение данных из декодера в стерео режиме
	DWORD GetStereoSamples(void* buffer, DWORD start, DWORD length, bool loop);

	// получение тишины в моно режиме
	DWORD GetMonoMute(void* buffer, DWORD length);

	// получение тишины в стерео режиме
	DWORD GetStereoMute(void* buffer, DWORD length);

	// получение количества семплов в файле
	DWORD GetSamplesInFile(void);

	// получение количества байт в треке моно режим
	DWORD GetRealMonoDataSize(void);

	// получение количества байт в треке стерео режим
	DWORD GetRealStereoDataSize(void);
};

#endif
