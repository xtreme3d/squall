//-----------------------------------------------------------------------------
//	Класс абстрактного декодера
//	Копонент звукового двигателя Шквал
//	команда		: AntiTank
//	разработчик	: Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------
#ifndef _ABSTRACT_DECOMPRESSOR_H_INCLUDED_
#define _ABSTRACT_DECOMPRESSOR_H_INCLUDED_

// включения
#include <windows.h>
#include "AbstractSoundFile.h"

class CAbstractSoundFile;

//-----------------------------------------------------------------------------
// Класс абстрактного декодера
//-----------------------------------------------------------------------------
class CAbstractDecompressor {
protected:
	// указатель на объект донор
	CAbstractSoundFile* SourceData;

public:
	// конструктор/деструктор
	CAbstractDecompressor(WAVEFORMATEX* pcm_format, bool& flag,
		CAbstractSoundFile* a)
	{
		SourceData = a;
	};

	virtual ~CAbstractDecompressor()
	{
	};

	// получение данных
	virtual DWORD GetMonoSamples(void* buffer, DWORD start, DWORD length,
		bool loop) = 0;
	virtual DWORD GetStereoSamples(void* buffer, DWORD start, DWORD length,
		bool loop) = 0;

	// получение тишины
	virtual DWORD GetMonoMute(void* buffer, DWORD length) = 0;
	virtual DWORD GetStereoMute(void* buffer, DWORD length) = 0;

	// получение длинны файла в семплах
	virtual DWORD GetSamplesInFile(void) = 0;

	// получение длинны трека в байтах
	virtual DWORD GetRealMonoDataSize(void) = 0;
	virtual DWORD GetRealStereoDataSize(void) = 0;
};

#endif
