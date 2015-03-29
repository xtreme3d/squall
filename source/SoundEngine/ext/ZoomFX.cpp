//-----------------------------------------------------------------------------
// Работа с ZOOMFX расширением
// Компонент звукового двигателя "Шквал"
// команда     : AntiTank
// разработчик : Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------

// включения
#include "ZoomFX.h"
#include "ZoomFX_api.h"

// идентификатор интерфейса
GUID ZOOMFX_Guid = {
	0xCD5368E0, 0x3450, 0x11D3,
	{ 0x8B, 0x6E, 0x00, 0x10, 0x5A, 0x9B, 0x7B, 0xBC }
};

//-----------------------------------------------------------------------------
// Получение интерфейса для звукового буфера
// на входе    :  buffer   - указатель на звуковой буфер для которого нужно
//  						 получить интерфейс
// на выходе   :  указатель на полученый интерфейс
//-----------------------------------------------------------------------------
LPKSPROPERTYSET zoomfx_GetBufferInterface(LPDIRECTSOUNDBUFFER buffer)
{
	LPKSPROPERTYSET prop;

	// проверка наличия буфера
	if (!buffer)
		return 0;

	// получение интерфейса
	if (buffer->QueryInterface(IID_IKsPropertySet, (void * *) &prop) == DS_OK)
		return prop;

	// ошибка
	return 0;
}

//-----------------------------------------------------------------------------
// Проверка возможности чтения и записи
// на входе    :  prop  - указатель на интерфейс
// на выходе   :  успешнось чтения и записи
//-----------------------------------------------------------------------------
int zoomfx_TestQuerySupport(LPKSPROPERTYSET prop)
{
	unsigned long support;

	// проверка параметров
	if (!prop)
		return false;

	// проверим поддержку слушателя
	if (prop->QuerySupport(ZOOMFX_Guid, DSPROPERTY_ZOOMFXBUFFER_ALL, & support) ==
		DS_OK) {
		// проверка возможности чтения и записи
		if ((support & KSPROPERTY_SUPPORT_GET) &&
			(support & KSPROPERTY_SUPPORT_SET))
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Установка параметров
// на входе    :  prop  - указатель на интерфейс
//  			  data  - указатель на устанавливаемые данные
// на выходе   :  успешнось установки
//-----------------------------------------------------------------------------
int zoomfx_Set(LPKSPROPERTYSET prop, void* data)
{
	// проверка наличия интерфейса
	if (prop) {
		if (prop->Set(ZOOMFX_Guid,
				  	DSPROPERTY_ZOOMFXBUFFER_ALL,
				  	NULL,
				  	0,
				  	data,
				  	sizeof(ZOOMFX_BUFFERPROPERTIES)) == DS_OK)
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Получение параметров
// на входе    :  prop  - указатель на интерфейс
//  			  data  - указатель куда нужно поместить параметры
// на выходе   :  успешнось получения
//-----------------------------------------------------------------------------
int zoomfx_Get(LPKSPROPERTYSET prop, void* data)
{
	unsigned long read;

	// проверка наличия интерфейса
	if (prop) {
		if (prop->Get(ZOOMFX_Guid,
				  	DSPROPERTY_ZOOMFXBUFFER_ALL,
				  	NULL,
				  	0,
				  	data,
				  	sizeof(ZOOMFX_BUFFERPROPERTIES),
				  	& read) == DS_OK)
			return true;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Проверка наличия ZOOMFX интерфейса
// на входе    :  direct   - указатель на объект воспроизведения
// на выходе   :  наличие ZOOMFX интерфейса
//-----------------------------------------------------------------------------
int zoomfx_TestSupport(LPDIRECTSOUND direct)
{
	// объявление переменных
	WAVEFORMATEX wfx;   	 // формат звука
	DSBUFFERDESC dsbd;  	 // дескриптор
	int ok; 		// флаг инициализации ZOOMFX интерфейса

	// промежуточные буфера
	LPDIRECTSOUNDBUFFER Buffer;
	LPKSPROPERTYSET KsProp;

	// заполним структуру с форматом
	wfx.wFormatTag = 1;
	wfx.nChannels = 2;
	wfx.nSamplesPerSec = 44100;
	wfx.wBitsPerSample = 16;
	wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) >> 3;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	wfx.cbSize = 0;

	// обнулим параметры
	ok = false;
	Buffer = 0;
	KsProp = 0;

	// проверка наличия объекта воспроизведения
	if (direct) {
		// занесение данных вторичного буфера
		ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
		dsbd.dwSize = sizeof(DSBUFFERDESC);
		dsbd.dwFlags = DSBCAPS_STATIC | DSBCAPS_CTRL3D;
		dsbd.dwBufferBytes = 128;
		dsbd.lpwfxFormat = &wfx;

		// Создание вторичного буфера
		ok = (direct->CreateSoundBuffer(&dsbd, & Buffer, NULL) == DS_OK) ?
			true :
			false;

		// получим интерфейс для работы с ZOOMFX
		KsProp = zoomfx_GetBufferInterface(Buffer);

		// проверка возможности чтения и записи
		ok = zoomfx_TestQuerySupport(KsProp);

		// освободим EAX слушателя
		if (KsProp) {
			KsProp->Release();
			KsProp = 0;
		}

		// освободим буфер
		if (Buffer) {
			Buffer->Release();
			Buffer = 0;
		}
	}
	return ok;
}