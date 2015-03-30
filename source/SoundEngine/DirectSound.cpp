//-----------------------------------------------------------------------------
// Работа с Direct Sound
// Копонент звукового двигателя Squall
// команда     : AntiTank
// разработчик : Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------
#include "DirectSound.h"

//-----------------------------------------------------------------------------
//	Создание Direct Sound объекта
// на входе    :  window		 - идентификатор окна
//  			  clsid_direct   - идентификатор Direct Sound
//  			  device		 - идентификатор устройства воспроизведения
//  			  level 		 - уровень кооперации
// на выходе   :	указатель на созданный Direct Sound объект, если значение
//  			  равно 0 значит создание не состоялось
//-----------------------------------------------------------------------------
LPDIRECTSOUND ds_Create(HWND window, REFCLSID clsid_direct, LPGUID device,
	int level)
{
	// установка переменных
	LPDIRECTSOUND direct = 0;

	// Инициализация COM
	CoInitialize(NULL);

	// Создание Direct Sound обьекта
	if (CoCreateInstance(clsid_direct,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IDirectSound,
			(void * *) &direct) != DS_OK)
		return false;

	// инициализация устройства воспроизведения
	if (direct->Initialize(device) != DS_OK)
		return false;

	// установка приоритетного режима
	if (direct->SetCooperativeLevel(window, level) != DS_OK)
		return false;

	return direct;
}

//-----------------------------------------------------------------------------
//	Освобождение Direct Sound объекта
// на входе    :  direct   - указатель на Direct Sound объект
// на выходе   :	*
//-----------------------------------------------------------------------------
void ds_Release(LPDIRECTSOUND direct)
{
	// удаление объекта воспроизведения
	if (direct)
		direct->Release();

	// Освобождение COM
	CoUninitialize();
}

//-----------------------------------------------------------------------------
//	Создание первичного буфера
// на входе    :  direct   - указатель на Direct Sound объект
// на выходе   :	указатель на первичный буфер, в случае если значение равно 0
//  			  значит создание не состоялось
//-----------------------------------------------------------------------------
LPDIRECTSOUNDBUFFER ds_CreatePrimary(LPDIRECTSOUND direct)
{
	DSBUFFERDESC dsbd;
	LPDIRECTSOUNDBUFFER primary;

	// защита от дурака
	if (!direct)
		return 0;

	// создание данных для получения доступа к первичному буферу
	ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
	dsbd.dwSize = sizeof(DSBUFFERDESC);
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER |
		DSBCAPS_CTRL3D |
		DSBCAPS_CTRLVOLUME;
	dsbd.dwBufferBytes = 0;
	dsbd.lpwfxFormat = NULL;

	// получение доступа к первичному буферу
	if (direct->CreateSoundBuffer(&dsbd, & primary, NULL) != DS_OK)
		return 0;

	return primary;
}

//-----------------------------------------------------------------------------
//	Освобождение первичного буфера
// на входе    :  primary  - указатель на первичный буфер
// на выходе   :	*
//-----------------------------------------------------------------------------
void ds_ReleasePrimary(LPDIRECTSOUNDBUFFER primary)
{
	if (primary)
		primary->Release();
}

//-----------------------------------------------------------------------------
//	Создание слушателя
// на входе    :  primary  - указатель на первичный буфер
// на выходе   :	указатель на интерфейс слушателя, в случае если значение
//  			  равно 0 значит создание не состоялось
//-----------------------------------------------------------------------------
LPDIRECTSOUND3DLISTENER ds_CreateListener(LPDIRECTSOUNDBUFFER primary)
{
	LPDIRECTSOUND3DLISTENER listener;

	if (!primary)
		return 0;

	// получение указателя на интерфейс слушателя
	if (primary->QueryInterface(IID_IDirectSound3DListener,
				 	(void * *) &listener) == DS_OK)
		return listener;

	return 0;
}

//-----------------------------------------------------------------------------
//	Освобождение интерфейса слушателя
// на входе    :  listener  - указатель на интерфейс слушателя
// на выходе   :	*
//-----------------------------------------------------------------------------
void ds_ReleaseListener(LPDIRECTSOUND3DLISTENER listener)
{
	if (listener)
		listener->Release();
}

//-----------------------------------------------------------------------------
//	Получение всех параметров слушателя
// на входе    :  listener - указатель на интерфейс слушателя
//  			  data     - указатель на структуру в которую нужно поместить
//  						 параметры слушателя
// на выходе   :	успешность получения параметров слушателя
//-----------------------------------------------------------------------------
int ds_GetAllListenerParameters(LPDIRECTSOUND3DLISTENER listener,
	LPDS3DLISTENER data)
{
	// проверка наличия слушателя
	if (!listener)
		return false;

	// пропишем размер структуры
	data->dwSize = sizeof(DS3DLISTENER);

	// получение параметров слушателя
	if (listener->GetAllParameters(data) == DS_OK)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
//	Установка всех параметров слушателя
// на входе    :  listener - указатель на интерфейс слушателя
//  			  data     - указатель на структуру в которой содержатся
//  						 параметры слушателя
// на выходе   :	успешность установки параметров слушателя
//-----------------------------------------------------------------------------
int ds_SetAllListenerParameters(LPDIRECTSOUND3DLISTENER listener,
	LPDS3DLISTENER data, DWORD def)
{
	int ret = false;

	// проверка наличия слушателя
	if (!listener)
		return ret;

	// установка позиции
	ret = (listener->SetPosition(data->vPosition.x,
					 	data->vPosition.y,
					 	data->vPosition.z,
					 	def) == DS_OK) ?
		true :
		false;

	// установка скорости слушателя
	if (ret)
		ret = (listener->SetVelocity(data->vVelocity.x,
						 	data->vVelocity.y,
						 	data->vVelocity.z,
						 	def) == DS_OK) ?
			true :
			false;

	// установка ориентации
	if (ret)
		ret = (listener->SetOrientation(data->vOrientFront.x,
						 	data->vOrientFront.y,
						 	data->vOrientFront.z,
						 	data->vOrientTop.x,
						 	data->vOrientTop.y,
						 	data->vOrientTop.z,
						 	def) == DS_OK) ?
			true :
			false;

	// установка дистанции
	if (ret)
		ret = (listener->SetDistanceFactor(data->flDistanceFactor, def) ==
			DS_OK) ?
			true :
			false;

	// установка фактора удаления
	if (ret)
		ret = (listener->SetRolloffFactor(data->flRolloffFactor, def) == DS_OK) ?
			true :
			false;

	// установка эффекта допплера
	if (ret)
		ret = (listener->SetDopplerFactor(data->flDopplerFactor, def) == DS_OK) ?
			true :
			false;

	return ret;
}

//-----------------------------------------------------------------------------
//	Конфигурирование первичного буфера
// на входе    :  primary  - указатель на первичный буфер
//  			  rate     - частота дискретизации
//  			  bits     - количество бит на выборку
//  			  channels - количество каналов
// на выходе   :	успешность конфигурирования первичного буфер
//-----------------------------------------------------------------------------
int ds_ConfigurePrimary(LPDIRECTSOUNDBUFFER primary, int rate, int bits,
	int channels)
{
	WAVEFORMATEX wfx;

	if (!primary)
		return false;

	// занесение данных о формате первичного звукового буфера
	ZeroMemory(&wfx, sizeof(WAVEFORMATEX)); 
	wfx.wFormatTag = WAVE_FORMAT_PCM; 
	wfx.nChannels = channels;
	wfx.nSamplesPerSec = rate;
	wfx.wBitsPerSample = bits;
	wfx.nBlockAlign = wfx.wBitsPerSample / 8 * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

	// форматирование первичного буфера
	if (primary->SetFormat(&wfx) != DS_OK)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
//	Проверка возможности использования алгоритма трехмерного звука
// на входе    :  direct   - указатель интерфейс Direct Sound
//  			  alg      - тестируемый алгорим
// на выходе   :	успешность использования
//-----------------------------------------------------------------------------
int ds_TestAlgorithm(LPDIRECTSOUND direct, int alg)
{
	// объявление переменных
	int ret;
	DSBUFFERDESC dsbd;
	WAVEFORMATEX wfx;
	LPDIRECTSOUNDBUFFER buffer;

	// заполним структуру с форматом
	wfx.wFormatTag = 1;
	wfx.nChannels = 2;
	wfx.nSamplesPerSec = 44100;
	wfx.wBitsPerSample = 16;
	wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) >> 3;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	wfx.cbSize = 0;

	// проверка наличия объекта воспроизведения
	if (direct) {
		// занесение данных вторичного буфера
		ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
		dsbd.dwSize = sizeof(DSBUFFERDESC);
		dsbd.dwFlags = DSBCAPS_STATIC |
			DSBCAPS_CTRLFREQUENCY |
			DSBCAPS_CTRLVOLUME |
			DSBCAPS_GLOBALFOCUS |
			DSBCAPS_CTRL3D |
			DSBCAPS_MUTE3DATMAXDISTANCE;
		dsbd.dwBufferBytes = 128;
		dsbd.lpwfxFormat = &wfx;
		/* !!! убрать !!!
					// подбор способа общета трехмерного звука
					switch(alg)
				  {
						case 0x0:
						dsbd.guid3DAlgorithm = DS3DALG_DEFAULT;
						break;
						case 0x1:
						dsbd.guid3DAlgorithm = DS3DALG_NO_VIRTUALIZATION;
						break;
						case 0x2:
						dsbd.guid3DAlgorithm = DS3DALG_HRTF_FULL;
						break;
						case 0x3:
						dsbd.guid3DAlgorithm = DS3DALG_HRTF_LIGHT;
						break;
					}*/

		// Создание вторичного буфера
		ret = (direct->CreateSoundBuffer(&dsbd, & buffer, NULL) == DS_OK) ?
			true :
			false;
		if (ret)
			buffer->Release();
	}

	// вернем результат
	return ret;
}

//-----------------------------------------------------------------------------
//	Получение параметров устройства воспроизведения
// на входе    :  direct   - указатель интерфейс Direct Sound
//  			  caps     - указатель на структуру описывающую параметры
//  						 устройства воспроизведения
// на выходе   :	успешность получения информации
//-----------------------------------------------------------------------------
int ds_GetCaps(LPDIRECTSOUND direct, LPDSCAPS caps)
{
	// проверка наличия объекта
	if (direct) {
		caps->dwSize = sizeof(DSCAPS);
		// получение свойств объекта
		if (direct->GetCaps(caps) == DS_OK)
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
//	Создание звукового буфера
// на входе    :  direct   - указатель интерфейс Direct Sound
//  			  desc     - указатель на структуру описывающую параметры
//  						 создаваемого буфера
// на выходе   :	указатель на созданный звуковой буфер, если значение равно 0
//  			  значит создание не состоялось
//-----------------------------------------------------------------------------
LPDIRECTSOUNDBUFFER ds_CreateBuffer(LPDIRECTSOUND direct, LPDSBUFFERDESC desc)
{
	LPDIRECTSOUNDBUFFER buffer = 0;

	// проверка наличия Direct Sound объекта
	if (!direct)
		return 0;

	// Создание вторичного буфера
	if (direct->CreateSoundBuffer(desc, & buffer, NULL) == DS_OK)
		return buffer;

	// создание не состоялось
	return 0;
}

//-----------------------------------------------------------------------------
//	Освобождение звукового буфера
// на входе    :  buffer   - указатель на звуковой буфер
// на выходе   :	*
//-----------------------------------------------------------------------------
void ds_ReleaseBuffer(LPDIRECTSOUNDBUFFER buffer)
{
	if (buffer)
		buffer->Release();
}

//-----------------------------------------------------------------------------
//	Получение трехмерного интерфейса для звукового буфера
// на входе    :  buffer   - указатель на звуковой буфер для которого нужно
//  						 получить трехмерный интерфейс
// на выходе   :	указатель на созданный трехмерный интерфейс, если значение
//  			  равно 0 значит получение не состоялось
//-----------------------------------------------------------------------------
LPDIRECTSOUND3DBUFFER ds_Get3DInterface(LPDIRECTSOUNDBUFFER buffer)
{
	LPDIRECTSOUND3DBUFFER buffer3d = 0;

	// проверка наличия звукового буфера
	if (!buffer)
		return 0;

	// получение трехмерного интерфейса
	if (buffer->QueryInterface(IID_IDirectSound3DBuffer, (VOID * *) buffer3d) ==
		DS_OK)
		return buffer3d;

	// получение не состоялось
	return 0;
}

//-----------------------------------------------------------------------------
//	Освобождение трехмерного интерфейса звукового буфера
// на входе    :  buffer   - указатель на трехмерный интерфейс звукового буфера
// на выходе   :	*
//-----------------------------------------------------------------------------
void ds_Release3DInterface(LPDIRECTSOUND3DBUFFER buffer3d)
{
	if (buffer3d)
		buffer3d->Release();
}