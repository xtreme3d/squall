//-----------------------------------------------------------------------------
//	Классы декодеров ИКМ форматов
//	Копонент звукового двигателя Шквал
//	команда		: AntiTank
//	разработчик	: Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------
#ifndef _PCM_DECOMPRESSOR_H_INCLUDED_
#define _PCM_DECOMPRESSOR_H_INCLUDED_

// включения
#include <windows.h>
#include "../AbstractDecoder.h"
#include "../AbstractSoundFile.h"

class CAbstractSoundFile;

//-----------------------------------------------------------------------------
// Класс для беззнакового 8 битного ИКМ
//-----------------------------------------------------------------------------
class CDecompressPcm8Unsigned : public CAbstractDecompressor {
public:
	// конструктор/деструктор
	CDecompressPcm8Unsigned(WAVEFORMATEX* pcm_format, bool& flag,
		CAbstractSoundFile* a)
		: CAbstractDecompressor(pcm_format, flag, a)
	{
		memcpy(pcm_format, a->GetFmt(), sizeof(WAVEFORMATEX));
		flag = true;
	};
	~CDecompressPcm8Unsigned()
	{
	};

	// получение данных из декодера в моно режиме
	DWORD GetMonoSamples(void* buffer, DWORD start, DWORD length, bool loop);

	// получение данных из декодера в стерео режиме
	DWORD GetStereoSamples(void* buffer, DWORD start, DWORD length, bool loop);

	// получение тишины в моно режиме
	DWORD GetMonoMute(void* buffer, DWORD length);

	// получение тишины в стерео режиме
	DWORD GetStereoMute(void* buffer, DWORD length);

	// получение количества семплов в файле
	DWORD GetSamplesInFile(void)
	{
		return SourceData->size / SourceData->GetFmt()->nBlockAlign;
	}

	// получение количества байт в треке моно режим
	DWORD GetRealMonoDataSize(void)
	{
		return GetSamplesInFile();
	}

	// получение количества байт в треке стерео режим
	DWORD GetRealStereoDataSize(void)
	{
		return GetSamplesInFile() << 1;
	}
};

//-----------------------------------------------------------------------------
// Класс для знакового 16 битного ИКМ
//-----------------------------------------------------------------------------
class CDecompressPcm16Signed : public CAbstractDecompressor {
protected:
	DWORD _channels;
	DWORD _curPosition;
public:
	// конструктор/деструктор
	CDecompressPcm16Signed(WAVEFORMATEX* pcm_format, bool& flag,
		CAbstractSoundFile* a)
		: CAbstractDecompressor(pcm_format, flag, a)
	{
		memcpy(pcm_format, a->GetFmt(), sizeof(WAVEFORMATEX));
		_channels = a->GetFmt()->nChannels;
		_curPosition = 0;
		flag = true;
	};

	~CDecompressPcm16Signed()
	{
	};

	// получение данных из декодера в моно режиме
	DWORD GetMonoSamples(void* buffer, DWORD start, DWORD length, bool loop);

	// получение данных из декодера в стерео режиме
	DWORD GetStereoSamples(void* buffer, DWORD start, DWORD length, bool loop);

	// получение тишины в моно режиме
	DWORD GetMonoMute(void* buffer, DWORD length);

	// получение тишины в стерео режиме
	DWORD GetStereoMute(void* buffer, DWORD length);

	// получение количества семплов в файле
	DWORD GetSamplesInFile(void)
	{
		return SourceData->size / SourceData->GetFmt()->nBlockAlign;
	}

	// получение количества байт в треке моно режим
	DWORD GetRealMonoDataSize(void)
	{
		return GetSamplesInFile() << 1;
	}

	// получение количества байт в треке стерео режим
	DWORD GetRealStereoDataSize(void)
	{
		return GetSamplesInFile() << 2;
	}
};

#endif
