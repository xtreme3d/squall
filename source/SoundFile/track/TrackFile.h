//-----------------------------------------------------------------------------
//	Класс для работы с Ogg файлами
//	Копонент звукового двигателя Шквал
//	команда		: AntiTank
//	разработчик	: Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------
#ifndef _TRACK_FILE_H_INCLUDED_
#define _TRACK_FILE_H_INCLUDED_

// включения
#include <windows.h>
#include <mmreg.h>
#include <mmsystem.h>

#include "Reader.h"
#include "AbstractSoundFile.h"

class CAbstractSoundFile;

//-----------------------------------------------------------------------------
// Класс для работы с данными Ogg файла
//-----------------------------------------------------------------------------
class CTrackFile : public CAbstractSoundFile {
protected:

	char fmt[256];					// место под контейнер с форматом звука

public:
	// конструктор деструктор
	CTrackFile(CReader* reader);
	~CTrackFile();

	// получение указателя на формат данных
	WAVEFORMATEX* GetFmt(void)
	{
		return (WAVEFORMATEX *) &fmt;
	}
};
#endif