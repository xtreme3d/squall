//-----------------------------------------------------------------------------
//	Класс для работы с данными звукового файла на низком уровне
//	Копонент звукового двигателя Шквал
//	команда		: AntiTank
//	разработчик	: Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------
#ifndef _CREADER_H_INCLUDED_
#define _CREADER_H_INCLUDED_

// включения
#include "ExternFunction.h"

//-----------------------------------------------------------------------------
//	Класс для работы с данными звукового файла на низком уровне
//-----------------------------------------------------------------------------
class CReader {
protected:
	// обратные вызовы для работы с файлами
	extern_open_t _open;						// указатель на внешнюю функцию открытия файла
	extern_read_t _read;						// указатель на внешнюю функцию чтения файла
	extern_seek_t _seek;						// указатель на внешнюю функцию сдвига позиции в файле
	extern_close_t _close;  					 // указатель на внешнюю функцию закрытия файла
	extern_malloc_t _malloc;					  // указатель на внешнюю функцию выделения памяти
	extern_free_t _free;						// указатель на внешнюю функцию освобождения памяти

	void* file_ptr; 					// указатель на данные файла
	char* file_name;					// указатель на имя файла
	char* file_ext; 					// указатель на расширение файла
	unsigned int file_size; 				   // размер файла
	unsigned int file_type; 				   // тип файла
	bool file_mem;  				   // флаг выделения памяти
	unsigned int file_position; 			   // текущая позиция в файле
	unsigned int file_handler;  			   // хендлер файла

public:

	CReader(						 // конструктор для файлов с диска
	const char* file, int stream_flag, bool& init, extern_open_t ext_open,
		extern_read_t ext_read, extern_seek_t ext_seek,
		extern_close_t ext_close, extern_malloc_t ext_malloc,
		extern_free_t ext_free);

	CReader(						 // конструктор для файла из памяти
	void* memory, unsigned int memory_size, int new_memory, bool& init,
		extern_malloc_t ext_malloc, extern_free_t ext_free);

	~CReader(); 					 // деструктор

	void free_file_resource(			  // освободить занимаемые файлом данные
	void);

	unsigned int read(  						  // чтение даных из файла со сдвигом позиции чтения
	void* buffer, unsigned int size);


	unsigned int peek(  						  // чтение данных из файла без сдвига позиции чтения
	void* buffer, unsigned int size);

	unsigned int seek(  						  // установка новой позиции в файле относитель начала файла
	unsigned int new_position);

	unsigned int tell(  						  // получить позицию в файле
	void);


	unsigned int get_size(  					  // получение размера файла
	void)
	{
		return file_size;
	}


	bool eof(   						  // проверка на конец файла
	void)
	{
		return file_size == file_position;
	}

	char* get_file_name(				   // получить указатель на имя используемого файла
	void)
	{
		return file_name;
	}

	char* get_file_ext( 				   // получить указатель на расширение используемого файла
	void)
	{
		return file_ext;
	}
};

#endif