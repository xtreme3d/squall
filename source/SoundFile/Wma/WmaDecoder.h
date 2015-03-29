//-----------------------------------------------------------------------------
//	Класс декодера WMA формата
//	Копонент звукового двигателя Argentum
//	разработчик : Гилязетдинов Марат
//-----------------------------------------------------------------------------
#ifndef _WMA_DECOMPRESSOR_H_INCLUDED_
#define _WMA_DECOMPRESSOR_H_INCLUDED_

// включения
#include <windows.h>
#include <mmsystem.h>
#include <amstream.h>

#include "AbstractDecoder.h"
#include "AbstractSoundFile.h"

class CAbstractSoundFile;

//-----------------------------------------------------------------------------
//	Класс декодера WMA формата
//-----------------------------------------------------------------------------
class CDecompressWma : public CAbstractDecompressor {
protected:
	IMultiMediaStream* MMStream_ptr;
	IMediaStream* MStream_ptr;
	IAudioMediaStream* AudioStream_ptr;
	IAudioData* AudioData_ptr;		// контейнер со звуком
	WORD* temp_wma_buffer;	// промежуточный семпловый буфер
	WAVEFORMATEX WmaFormat;			// формат файла

public:
	// конструктор/деструктор
	CDecompressWma(WAVEFORMATEX* pcm_format, bool& flag, CAbstractSoundFile* a);
	~CDecompressWma();

	// получение моно данных из декодера
	DWORD GetMonoSamples(void* buffer, DWORD start, DWORD length, bool loop);

	// получение стерео данных из декодера
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