//-----------------------------------------------------------------------------
//	Класс для низкоуровневой работы с файлами
//	Копонент звукового двигателя Шквал
//	команда		: AntiTank
//	разработчик	: Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------
#include "Reader.h"
#include "string.h"

//-----------------------------------------------------------------------------
//	Конструктор класса
//	Создает звуковые данные из указанного файла
//	на входе	:	file		- указатель на имя звукового файла
//					stream_flag	- флаг определяет тип хранения файла
//								  true	- хранить в памяти
//								  false	- файл с докачкой
//					init		- указатель на переменную в которую поместить
//								  успешность инициализации файла
//					open		- указатель на внешнюю функцию открытия файла
//					read		- указатель на внешнюю функцию чтения из файла
//					seek		- указатель на внешнюю функцию позиционирования в файле
//					close		- указатель на внешнюю функцию закрытия файла
//	на выходе	:	*
//-----------------------------------------------------------------------------
CReader::CReader(const char* file, int stream_flag, bool& init,
	extern_open_t ext_open, extern_read_t ext_read, extern_seek_t ext_seek,
	extern_close_t ext_close, extern_malloc_t ext_malloc,
	extern_free_t ext_free)
{
	unsigned int r;

	init = false;		// ошибки нет
	file_handler = 0xffffffff;	// хендлер файла
	file_size = 0;			// размер файла
	file_ptr = 0;			// указатель на загруженные данные файла
	file_type = 0;			// тип файла
	file_mem = 0;			// флаг расположения файла в памяти
	file_position = 0;			// текущее смещение в файле
	file_name = 0;			// указатель на имя файла
	file_ext = 0;			// указатель на расширение файла

	// заполнение указателей на внешние методы чтения файла
	_open = ext_open;
	_read = ext_read;
	_seek = ext_seek;
	_close = ext_close;

	_malloc = ext_malloc;
	_free = ext_free;

	if (file == 0)
		return;

	// определим длинну стринга
	unsigned int len = strlen(file);

	// выделим память под имя файла
	file_name = (char *) _malloc(len + 1);

	// память под файл выделена ?
	if (!file_name)
		return;

	// сохраним имя файла
	memcpy(file_name, file, len + 1);

	// определим расширение
	for (int i = strlen(file_name); i > 0; --i) {
		if (file_name[i] == 0x2e) {
			file_ext = file_name + i + 1;
			break;
		}
	}

	// открытие файла
	file_handler = _open(file);
	if (file_handler != 0xffffffff) {
		// определим размер файла
		file_size = _seek(file_handler, 0, 2);
		if (file_size != 0xffffffff) {
			_seek(file_handler, 0, 0);

			// определим тип файла
			if (stream_flag) {
				// файл в памяти

				// выделим память под файл
				file_ptr = (void *) _malloc(file_size);
				if (file_ptr) {
					// загрузка файла в память
					r = _read(file_handler, file_ptr, file_size);
					if (r == file_size) {
						_close(file_handler);
						file_handler = 0xffffffff;
						init = true;
						file_type = 1;
						file_mem = true;
						return;
					}
				}
			} else {
				// файл с докачкой с диска
				init = true;
				file_type = 2;
				file_mem = false;
				return;
			}
		}
	}

	// файл открыли ?
	if (file_handler != 0xffffffff) {
		_close(file_handler);
		file_handler = 0xffffffff;
	}

	// память под файл выделена ?
	if (file_ptr) {
		_free(file_ptr);
		file_ptr = 0;
	}

	file_size = 0;
	file_type = 0;
}

//-----------------------------------------------------------------------------
//	Конструктор класса
//	Создает звуковой файл из указанных данных
//	на входе	:	memory		- указатель память со звуковыми данными
//					memory_size	- размер звуковых данных
//					new_memory	- флаг выделения новой памяти под звуковые данные
//								  true	- выделить и скопировать звуковые данные
//								  false	- использовать уже выделенную память
//					init		- указатель на переменную в которую поместить
//								  успешность инициализации файла
//	на выходе	:	*
//-----------------------------------------------------------------------------
CReader::CReader(void* memory, unsigned int memory_size, int new_memory,
	bool& init, extern_malloc_t ext_malloc, extern_free_t ext_free)
{
	init = false;		// ошибки нет
	file_handler = 0xffffffff;	// хендлер файла
	file_size = memory_size;	// размер файла
	file_ptr = 0;			// указатель на загруженные данные файла
	file_type = 1;			// тип файла
	file_mem = 0;			// флаг расположения файла в памяти
	file_position = 0;			// текущее смещение в файле
	file_name = 0;			// указатель на имя файла
	file_ext = 0;			// указатель на расширение файла

	// внешние методы
	_malloc = ext_malloc;
	_free = ext_free;

	// проверка типа создания данных
	if (new_memory) {
		// с выделением памяти
		// выделим память под файл
		file_ptr = (void *) _malloc(memory_size);
		if (file_ptr) {
			// скопируем данные
			memcpy(file_ptr, memory, memory_size);
			file_mem = true;
			init = true;
			return;
		}
	} else {
		// без выделения памяти
		file_ptr = memory;
		file_mem = false;
		init = true;
		return;
	}

	// память под файл выделена ?
	if (file_ptr) {
		_free(file_ptr);
		file_ptr = 0;
		file_mem = 0;
	}

	file_size = 0;
	file_type = 0;
}

//-----------------------------------------------------------------------------
//	Деструктор класса
//	на входе	:	*
//	на выходе	:	*
//-----------------------------------------------------------------------------
CReader::~CReader()
{
	// файл открыли ?
	if (file_handler != 0xffffffff) {
		_close(file_handler);
		file_handler = 0xffffffff;
	}

	// память под файл выделена ?
	if (file_mem) {
		if (file_ptr) {
			_free(file_ptr);
			file_ptr = 0;
		}
	}

	if (file_name) {
		_free(file_name);
		file_name = 0;
	}

	file_size = 0;	// размер файла
	file_type = 0;	// тип файла
	file_position = 0;	// текущее смещение в файле
}

//-----------------------------------------------------------------------------
//	Освобождение загруженных данных (внутренний метод AgSS)
//	на входе	:	*
//	на выходе	:	*
//-----------------------------------------------------------------------------
void CReader::free_file_resource(void)
{
	// файл открыли ?
	if (file_handler != 0xffffffff) {
		_close(file_handler);
		file_handler = 0xffffffff;
	}

	// память под файл выделена ?
	if (file_mem) {
		if (file_ptr) {
			_free(file_ptr);
			file_ptr = 0;
		}
	}

	file_size = 0;	// размер файла
	file_type = 0;	// тип файла
	file_position = 0;	// текущее смещение в файле
}

//-----------------------------------------------------------------------------
//	Низкоуровневое чтение данных из файла со сдвигом позиции в файле
//	на входе	:	buffer	- указатель на буфер в который поместить данные
//					length	- размер читаемых данных
//	на выходе	:	количество прочитанных данных, в байтах
//-----------------------------------------------------------------------------
unsigned int CReader::read(void* buffer, unsigned int length)
{
	unsigned int read_size;
	// проверин на выход за пределы файла
	if (file_position > file_size)
		file_position = file_size;

	// проверим что размер чтения не выходит за пределы файла
	if ((file_position + length) > file_size)
		length = file_size - file_position;

	// проверим что есть что читать
	if (length) {
		// проверка типа файла
		switch (file_type) {
			// обработка файла загруженного в память
		case 1:
			// защита от дурака
			if ((file_ptr) && (file_size)) {
				// скопируем данные в буфер
				memcpy(buffer, (char *) file_ptr + file_position, length);
				file_position += length;
				return length;
			}
			break;

			// обработка файла с докачкой
		case 2:
			// защита от дурака
			if ((file_handler != 0xffffffff) && (file_size)) {
				// прочитаем данные из файла
				read_size = _read(file_handler, buffer, length);
				if (read_size == length) {
					file_position += length;
					return length;
				}
			}
			break;
		};
	}
	return 0;
}

//-----------------------------------------------------------------------------
//	Низкоуровневое чтение данных из файла без сдвига позиции в файле
//	на входе	:	buffer	- указатель на буфер в который поместить данные
//					length	- размер читаемых данных
//	на выходе	:	количество прочитанных данных, в байтах
//-----------------------------------------------------------------------------
unsigned int CReader::peek(void* buffer, unsigned int length)
{
	unsigned int read_size;
	// проверин на выход за пределы файла
	if (file_position > file_size)
		file_position = file_size;

	// проверим что размер чтения не выходит за пределы файла
	if ((file_position + length) > file_size)
		length = file_size - file_position;

	// проверим что есть что читать
	if (length) {
		// проверка типа файла
		switch (file_type) {
			// обработка файла загруженного в память
		case 1:
			// защита от дурака
			if ((file_ptr) && (file_size)) {
				// скопируем данные в буфер
				memcpy(buffer, (char *) file_ptr + file_position, length);
				return length;
			}
			break;

			// обработка файла с докачкой
		case 2:
			// защита от дурака
			if ((file_handler != 0xffffffff) && (file_size)) {
				// прочитаем данные из файла
				read_size = _read(file_handler, buffer, length);
				if (read_size == length) {
					if (_seek(file_handler, file_position, 0) != 0xFFFFFFFF)
						return length;
					else
						return 0;
				}
			}
			break;
		};
	}
	return 0;
}

//-----------------------------------------------------------------------------
//	Установка позиции чтения из файла относительно начала файла
//	на входе	:	new_position	- новая позиция чтения из файла
//					type			- тип позиционирования (относительно чего)
//	на выходе	:	новое значение позиции чтения из файла
//-----------------------------------------------------------------------------
unsigned int CReader::seek(unsigned int new_position)
{
	file_position = new_position;
	// проверка на выход за пределы файла
	if (file_position > file_size)
		file_position = file_size;

	switch (file_type) {
		// файл в памяти
	case 1:
		return file_position;

		// файл с докачкой
	case 2:
		if (_seek(file_handler, file_position, 0) != 0xFFFFFFFF) {
			return file_position;
		} else {
			return -1;
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
//	Получение текущей позиции в файле
//	на входе	:	*
//	на выходе	:	позиции в файле
//-----------------------------------------------------------------------------
unsigned int CReader::tell(void)
{
	return file_position;
}
