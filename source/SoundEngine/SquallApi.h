//-----------------------------------------------------------------------------
// Описание ядра звукового двигателя "Шквал"
// команда     : AntiTank
// разработчик : Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------
#ifndef _SQUALL_API_H_INCLUDED_
#define _SQUALL_API_H_INCLUDED_

#include <dsound.h>
#include "ext/Eax.h"
#include "../SoundFile/ExternFunction.h"
#include "../Squall.h"
#include "../SoundFile/SoundFile.h"
#include "Context.h"

/////////////////////////////////////////////////
// макросы
/////////////////////////////////////////////////
#define DEVICE_NAME_LEN 256 					   // максимальная длинна строки с именем устройства воспроизведения

/////////////////////////////////////////////////
// структура устройства воспроизведения
/////////////////////////////////////////////////
typedef struct device_s {
	LPGUID _id; 				   // идентификатор устройства воспроизведения
	DSCAPS _caps;   			   // информация об устройстве
	char _name[DEVICE_NAME_LEN]; // имя устройства воспроизведения
	int _alg[4];				// список поддерживаемых 3D алгоритмов
	int _eax[EAX_NUM + 1];  	  // информация о поддержке EAX интерфейсов
	int _zoomfx;				// информация о поддержке ZOOMFX
} device_t;

////////////////////////////////////////////////
// Cтруктура списка устройств воспроизведения
////////////////////////////////////////////////
typedef struct device_list_s {
	int _capacity;  			// текущий размер массива
	int _size;  				// текущее количество элементов в массиве
	device_s** _array;  			   // указатель на массив
} device_list_t;

////////////////////////////////////////////////
// Структура для описания звуковых данных
////////////////////////////////////////////////
typedef struct sample_s {
	unsigned char _count;   			  // сколько раз занималась ячейка
	CSoundFile* _file;  				// указатель на звуковые данные
	squall_sample_default_t _def;   				// установки по умолчанию
} sample_t;

////////////////////////////////////////////////
// Cтруктура списка контекстов
////////////////////////////////////////////////
typedef struct context_list_s {
	int _capacity;  			// текущий размер массива
	int _size;  				// количество элементов в списке
	context_t** _array; 				// указатель на массив с данными
} context_list_t;

////////////////////////////////////////////////
// Cтруктура списка звуковых файлов
////////////////////////////////////////////////
typedef struct sample_list_s {
	int _capacity;  			// текущий размер массива
	int _size;  				// количество элементов в списке
	sample_t** _array;  			   // указатель на массив с данными
} sample_list_t;

////////////////////////////////////////////////
// Cтруктура ядра Шквала
////////////////////////////////////////////////
typedef struct squall_api_s {
	device_list_t _deviceList;  		  // список устройств воспроизведения
#ifndef _USRDLL
	context_list_t _contextList;		   // список контекстов
#else
	context_t _context;
#endif
	sample_list_t _sampleList;  		  // список семплов
	HANDLE _sample_mutex;   	   // мутекс для списка семплов
	int _log_init;  			// флаг инициализации ядра
	HANDLE _log_mutex;  		   // мутекс лог файла
	unsigned int _log_start_time;   	 // время запуска лога
	char _log_file_name[1024];   // имя лог файла

	// указатели на внешние функций
	extern_open_t _open;				  // внешная функция открытия файла
	extern_read_t _read;				  // внешняя функция чтения файла
	extern_seek_t _seek;				  // внешняя функция позиционирования в файле
	extern_close_t _close;  			   // внешняя функция закрытия файла
	extern_malloc_t _malloc;				// внешняя функция выделения памяти
	extern_free_t _free;				  // внешняя функция освобождение памяти
} squall_api_t;

extern squall_api_s api;

////////////////////////////////////////////////
// Рабочие методы
////////////////////////////////////////////////
void* api_malloc(   						 // выделение памяти
unsigned int size);

void api_free(  							// освобождение памяти
void* ptr);

void api_Startup(); 						// инициализация ядра

void api_Shutdown();						// деинициализация ядра

// работа с устройствами воспроизведения
LPGUID api_GetDeviceID( 					  // получение идентификатора устройства воспроизведения
int device);

int api_GetNumDevice(); 				   // получение количества устройств воспроизведения

char* api_GetDeviceName(					 // получение имени устройства воспроизведения
int device);

device_t* api_GetDevice(						 // получение свойств устройства воспроизведения
int device);

// работа с контекстами
context_t* api_CreateContext(   				  // создание контекста
void* id);

context_t* api_GetContext(  					  // получение контекста
void* id);

void api_ReleaseContext(					// освобождение контекста
void* id);

// работа с семплами
int api_SampleLoadFile( 				   // создание семпла из файла
const char* filename, int mem_flag, squall_sample_default_t* def);

int api_CreateSampleFromMemory( 		   // создание семпла из памяти
void* ptr, unsigned int size, int new_mem, squall_sample_default_t* def);

void api_SampleUnload(  					// удаление семпла
int id);

void api_SampleUnloadAll(); 				// удаление всех семплов

CSoundFile* api_SampleGetFile(  				   // получение указателя на звуковой файл
int id);

int api_SampleGetFileGroup( 			   // получение указателя на данные семпла
int id, unsigned int num);

int api_SampleSetDefault(   			   // установка параметров семпла по умолчанию
int id, squall_sample_default_t* def);

int api_SampleGetDefault(   			   // получение параметров семпла по умолчанию
int id, squall_sample_default_t* def);

// работа с лог файлом
void api_Log(   							// запись строки в отчет
const char* string, ...);

void api_LogTime(   						// запись строки в отчет с временем
const char* string, ...);

// работа с функциями обратного вызова
void api_SetFileCallbacks(  				// установка внешних функций работы с файлами
extern_open_t ext_open, extern_read_t ext_read, extern_seek_t ext_seek,
	extern_close_t ext_close);

void api_SetMemoryCallbacks(				// установка внешних функций работы с памятью
extern_malloc_t ext_malloc, extern_free_t ext_free);
#endif
