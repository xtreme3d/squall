//-----------------------------------------------------------------------------
// Работа с Direct Sound
// Копонент звукового двигателя Squall
// команда     : AntiTank
// разработчик : Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------
#ifndef _DIRECT_SOUND_H_INCLUDED_
#define _DIRECT_SOUND_H_INCLUDED_

#include <dsound.h>

// создание Direct Sound объекта
LPDIRECTSOUND ds_Create(HWND window, REFCLSID clsid_irect, LPGUID device,
	int level);

// удаление Direct Sound объекта
void ds_Release(LPDIRECTSOUND direct);

// создание первичного буфера
LPDIRECTSOUNDBUFFER ds_CreatePrimary(LPDIRECTSOUND direct);

void ds_ReleasePrimary(LPDIRECTSOUNDBUFFER primary);

// конфигурирование первичного буфера
int ds_ConfigurePrimary(LPDIRECTSOUNDBUFFER primary, int rate, int bits,
	int channels);

// проверка поддержки алгоритма расчета 3D
int ds_TestAlgorithm(LPDIRECTSOUND direct, int alg);

// получение параметров устройства воспроизведения
int ds_GetCaps(LPDIRECTSOUND direct, LPDSCAPS caps);

// получение интерфейса слушателя
LPDIRECTSOUND3DLISTENER ds_CreateListener(LPDIRECTSOUNDBUFFER primary);

// освобождение интерфейса слушателя
void ds_ReleaseListener(LPDIRECTSOUND3DLISTENER listener);

// получение всех параметров слушателя
int ds_GetAllListenerParameters(LPDIRECTSOUND3DLISTENER listener,
	LPDS3DLISTENER data);

// установка всех параметров слушателя
int ds_SetAllListenerParameters(LPDIRECTSOUND3DLISTENER listener,
	LPDS3DLISTENER data, DWORD def);

// создание звукового буфера
LPDIRECTSOUNDBUFFER ds_CreateBuffer(LPDIRECTSOUND direct, LPDSBUFFERDESC desc);

// освобождение звукового буфера
void ds_ReleaseBuffer(LPDIRECTSOUNDBUFFER buffer);

// получение 3д интерфейса
LPDIRECTSOUND3DBUFFER ds_Get3DInterface(LPDIRECTSOUNDBUFFER buffer);

// освобождение 3д интерфейса
void ds_Release3DInterface(LPDIRECTSOUND3DBUFFER buffer3d);
#endif
