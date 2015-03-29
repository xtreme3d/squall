//-----------------------------------------------------------------------------
//	Класс для работы с данными Mp3 файлов
//	Копонент звукового двигателя Шквал
//	команда		: AntiTank
//	разработчик	: Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------
#include "MpegFile.h"

//-----------------------------------------------------------------------------
//	Конструктор класса
//	на входе :	reader_ptr	- указатель на низкоуровневые данные файла
//	на выходе :	*
//-----------------------------------------------------------------------------
CMpegFile::CMpegFile(CReader* reader_ptr)
	: CAbstractSoundFile()
{
	// структура для идентификации тега второй версии
	struct ID3Header {
		char Tag[3];		// заголовок "ID3"
		BYTE VerHi;		// номер версии старшая часть
		BYTE VerLow;		// номер версии младшая часть
		BYTE Flags;		// флаги
		BYTE Size[4];	// закодированный размер заголовка
	};

	BYTE temp[256];
	DWORD start_pos;
	DWORD end_pos;

	reader = reader_ptr;			// указатель на данные файла

	start_pos = 0;
	end_pos = reader->get_size();

	// проверка наличия тега второй версии
	reader->seek(start_pos);
	if (reader->read(&temp, sizeof(temp)) != sizeof(temp))
		return;

	// проверим наличие тега второй версии
	if (!memcmp(((ID3Header *) temp)->Tag, "ID3", 3)) {
		start_pos += (((ID3Header *) temp)->Size[3] & 0x7F) |
			((((ID3Header *) temp)->Size[2] & 0x7F) << 7) |
			((((ID3Header *) temp)->Size[1] & 0x7F) << 14) |
			((((ID3Header *) temp)->Size[0] & 0x7F) << 21);
		start_pos += 10;
	}

	// встанем в начало файла
	reader->seek(start_pos);
	if (reader->read(&temp, sizeof(temp)) != sizeof(temp))
		return;

	// найдем начало если таковое имеется
	while (1) {
		DWORD i = 0;
		// конец файла ?
		if (reader->eof())
			break;

		// поиск маркера
		for (i = 0; i < sizeof(temp); i ++) {
			if ((temp[i] == 0xFF) && ((temp[i + 1] & 0xF0) == 0xF0)) {
				break;
			} else {
				if ((temp[i] == 0xFF) && ((temp[i + 1] & 0xF0) == 0xE0))
					break;
			}
		}
		if (i == sizeof(temp)) {
			start_pos += sizeof(temp) - 1;
			reader->seek(start_pos);
			reader->read(temp, sizeof(temp));
		} else {
			start_pos += i;
			break;
		}
	}

	// проверка наличия тега первой версии
	reader->seek(end_pos - 128);
	reader->read(temp, 3);
	if (!memcmp(temp, "TAG", 3))
		end_pos -= 128;

	// встанем в начало файла
	reader->seek(start_pos);

	start_position = start_pos;						// начальная позиция в файле
	end_position = end_pos;							// конечная позиция в файле

	cur_position = 0;								// внутренняя текущая позиция
	size = end_position - start_position;	// размер внутренних данных
}

//-----------------------------------------------------------------------------
//	Деструктор класса
//	на входе :	*
//	на выходе :	*
//-----------------------------------------------------------------------------
CMpegFile::~CMpegFile()
{
}
