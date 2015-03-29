//-----------------------------------------------------------------------------
//	Описание внешних функций для работы с файлами и памятью
//	Копонент звукового двигателя Шквал
//	команда		: AntiTank
//	разработчик	: Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------

#ifndef _EXTERN_FUNCRION_H_INCLUDED_
#define _EXTERN_FUNCRION_H_INCLUDED_

// прототипы внешних функций чтения данных
typedef unsigned int (__cdecl* extern_open_t)   (const char* filename);
typedef int 		 (__cdecl* extern_read_t)   (unsigned int handle,
	void* buffer, int size);
typedef int 		 (__cdecl* extern_seek_t)   (unsigned int handle,
	int position, int mode);
typedef void		 (__cdecl* extern_close_t)  (unsigned int handle);

// прототипы внешних функций для работы с памятью
typedef void*(__cdecl* extern_malloc_t) (unsigned int size);
typedef void		 (__cdecl* extern_free_t)   (void* memory);

#endif
