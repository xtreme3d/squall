//-----------------------------------------------------------------------------
//	Класс для работы с Wav файлами
//	Копонент звукового двигателя Шквал
//	команда		: AntiTank
//	разработчик	: Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------
#ifndef _WAV_FILE_H_INCLUDED_
#define _WAV_FILE_H_INCLUDED_

// включения
#include <windows.h>
#include <mmreg.h>
#include <mmsystem.h>

#include "../Reader.h"
#include "../AbstractSoundFile.h"

// идентификаторы блоков
#define RIFF_ID		0x46464952			// идентификатор файла
#define WAVE_ID		0x45564157			// идентификатор wav блока
#define FORMAT_ID	0x20746d66			// идентификатор блока формата
#define DATA_ID		0x61746164			// идентификатор блока данных звука

class CAbstractSoundFile;

//-----------------------------------------------------------------------------
// Класс для работы с данными Wav файла
//-----------------------------------------------------------------------------
class CWavFile : public CAbstractSoundFile {
protected:
	// структура заголовка файла
	struct wav_header {
		DWORD Id1;						// идентификатор (должен быть RIFF)
		DWORD Size;						// размер блока
		DWORD Id2;						// идентификатар (должен быть WAVE)
	};

	// структура блока описателя данных
	struct wav_block {
		DWORD Id;						// идентификатор блока
		DWORD Size;						// размер блока
	};

	char fmt[256];					// место под контейнер с форматом звука

public:
	// конструктор деструктор
	CWavFile(CReader* reader);
	~CWavFile();

	// получение указателя на формат данных
	WAVEFORMATEX* GetFmt(void)
	{
		return (WAVEFORMATEX *) &fmt;
	}
};
#endif
