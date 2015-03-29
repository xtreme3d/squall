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
LPKSPROPERTYSET   eax_GetBufferInterface(          // получение eax интерфейса для звукового буфера
                  LPDIRECTSOUNDBUFFER buffer);

LPKSPROPERTYSET   eax_GetBuffer3DInterface(        // получение eax интерфейса для трехмерного звукового буфера
                  LPDIRECTSOUND3DBUFFER buffer3d);
/*
int               eax_TestQuerySupport(            // проверка доступности интерфейса
                  LPKSPROPERTYSET prop,
                  int type,
                  int version);
*/
int               eax_Set(                         // установка eax параметров 
                  LPKSPROPERTYSET prop,
                  int type,
                  int version,
                  void* data);

int               eax_Get(                         // получение eax параметров
                  LPKSPROPERTYSET prop,
                  int type,
                  int version,
                  void* data);

int               eax_Preset(                      // установка пресета
                  LPKSPROPERTYSET prop,
                  int version,
                  int preset);

int               eax_TestSupport(                 // проверка поддержки интерфейса
                  LPDIRECTSOUND direct,
                  int version);

#endif
