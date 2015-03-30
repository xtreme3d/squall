//-----------------------------------------------------------------------------
// Работа с EAX расширением
// Компонент звукового двигателя "Шквал"
// команда     : AntiTank
// разработчик : Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------
#ifndef _EAX_H_INCLUDED_
#define _EAX_H_INCLUDED_

// включения
#include <dsound.h>

////////////////////////////////////////////////
// Макросы
////////////////////////////////////////////////
#define EAX_NUM      3     // максимальный номер версии EAX интерфейса

#define EAX_LISTENER 0     // тип слушатель
#define EAX_BUFFER   1     // тип буфер

////////////////////////////////////////////////
// Рабочие методы
////////////////////////////////////////////////
// получение eax интерфейса для звукового буфера
LPKSPROPERTYSET   eax_GetBufferInterface(LPDIRECTSOUNDBUFFER buffer);

// получение eax интерфейса для трехмерного звукового буфера
LPKSPROPERTYSET   eax_GetBuffer3DInterface(LPDIRECTSOUND3DBUFFER buffer3d);
/*
// проверка доступности интерфейса
int               eax_TestQuerySupport(
                  LPKSPROPERTYSET prop,
                  int type,
                  int version);
*/
// установка eax параметров 
int               eax_Set(
                  LPKSPROPERTYSET prop,
                  int type,
                  int version,
                  void* data);

// получение eax параметров
int               eax_Get(
                  LPKSPROPERTYSET prop,
                  int type,
                  int version,
                  void* data);

// установка пресета
int               eax_Preset(
                  LPKSPROPERTYSET prop,
                  int version,
                  int preset);

// проверка поддержки интерфейса
int               eax_TestSupport(
                  LPDIRECTSOUND direct,
                  int version);

#endif
