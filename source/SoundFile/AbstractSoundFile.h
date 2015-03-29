//-----------------------------------------------------------------------------
//	Класс абстрактных данных звука
//	Копонент звукового двигателя Шквал
//	команда		: AntiTank
//	разработчик	: Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------
#ifndef _ABSTRACT_SOUND_FILE_H_INCLUDED_
#define _ABSTRACT_SOUND_FILE_H_INCLUDED_

// включения
#include <windows.h>
#include <mmreg.h>
#include <mmsystem.h>

#include "Reader.h"

//-----------------------------------------------------------------------------
//	Класс абстрактных данных звука
//-----------------------------------------------------------------------------
class CAbstractSoundFile {
protected:
	CReader* reader;			  // указатель на обработчик данных звукового файла

	DWORD start_position;      // смещение в файле на начало звуковых данных
	DWORD end_position; 	   // смещение в файле на конец звуковых данных
	DWORD cur_position; 	   // текущая позиция извлечения данных

public:
	DWORD size; 			   // размер звуковых данных

	// конструктор деструктор
	CAbstractSoundFile()
	{
	};
	virtual ~CAbstractSoundFile()
	{
	};

	// получение указателя на формат звука
	virtual WAVEFORMATEX* GetFmt(void) = 0;

	//-----------------------------------------------------------------------------
	//	Прочитать данные из файла со сдвигом указателя в файле
	//	на входе	:	buffer	- указатель на буфер куда читать данные
	//					length	- количество читаемых данных, в байтах
	//	на выходе	:	количество прочитанных байт
	//-----------------------------------------------------------------------------
	DWORD read(void* buffer, DWORD length)
	{
		// проверка на выход за пределы файла
		if ((cur_position + length) > size)
			length = size - cur_position;

		// чтение данных
		length = reader->read(buffer, length);
		cur_position += length;
		return length;
	};

	//-----------------------------------------------------------------------------
	//	Прочитать данные из файла без сдвига указателя в файле
	//	на входе	:	buffer	- указатель на буфер куда читать данные
	//					length	- количество читаемых данных, в байтах
	//	на выходе	:	количество прочитанных байт
	//-----------------------------------------------------------------------------
	DWORD peek(void* buffer, DWORD length)
	{
		// проверка на выход за пределы файла
		if ((cur_position + length) > size)
			length = size - cur_position;

		// чтение данных
		return reader->peek(buffer, length);
	};

	//-----------------------------------------------------------------------------
	//	Установить позицию чтения из файла
	//	на входе	:	new_position	- новоя позиция в файле
	//					type			- тип точки отсчета
	//	на выходе	:	текущая позиция в файле
	//-----------------------------------------------------------------------------
	DWORD seek(DWORD new_position, DWORD type)
	{
		switch (type) {
		case FILE_BEGIN:
			cur_position = new_position;
			break;

		case FILE_CURRENT:
			cur_position += new_position;
			break;

		case FILE_END:
			cur_position = size;
			break;
		}
		// проверка на выход за пределы файла
		if (cur_position > size)
			cur_position = size;

		// установка новой позиции чтения
		reader->seek(start_position + cur_position);
		return cur_position;
	};

	//-----------------------------------------------------------------------------
	//	Получить позицию в файле
	//	на входе	:	*
	//	на выходе	:	текущая позиция в файле
	//-----------------------------------------------------------------------------
	DWORD tell(void)
	{
		cur_position = (reader->tell() - start_position);
		return cur_position;
	};

	//-----------------------------------------------------------------------------
	//	Проверка на конец файла
	//	на входе	:	*
	//	на выходе	:	true	- файл кончился
	//					false	- файл не кончился
	//-----------------------------------------------------------------------------
	int eof()
	{
		return cur_position == size;
	};

	//-----------------------------------------------------------------------------
	//	Получение размера файла
	//	на входе	:	*
	//	на выходе	:	размер файла
	//-----------------------------------------------------------------------------
	int length()
	{
		return size;
	}

	char* get_file_name(void)
	{
		return reader->get_file_name();
	}

	void free_file_resource(void)
	{
		reader->free_file_resource();
	}
};
#endif