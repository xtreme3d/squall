//-----------------------------------------------------------------------------
//	Класс для работы с Ogg файлами
//	Копонент звукового двигателя Шквал
//	команда		: AntiTank
//	разработчик	: Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------
#ifndef _OGG_FILE_H_INCLUDED_
#define _OGG_FILE_H_INCLUDED_

// включения
#include <windows.h>
#include <mmreg.h>
#include <mmsystem.h>

#include "../Reader.h"
#include "../AbstractSoundFile.h"

class CAbstractSoundFile;

//-----------------------------------------------------------------------------
// Класс для работы с данными Ogg файла
//-----------------------------------------------------------------------------
class COggFile : public CAbstractSoundFile {
protected:

	char fmt[256];					// место под контейнер с форматом звука

public:
	// конструктор деструктор
	COggFile(CReader* reader);
	~COggFile();

	// получение указателя на формат данных
	WAVEFORMATEX* GetFmt(void)
	{
		return (WAVEFORMATEX *) &fmt;
	}
};
#endif
