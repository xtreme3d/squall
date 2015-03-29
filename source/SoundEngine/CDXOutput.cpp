#include "CDXOutput.h"
#include "HiddenWindow.h"

/////////////////////////////////////////////////
// макросы
/////////////////////////////////////////////////
#define DX_DEVICE_GROWBY	  4

// статические переменные
int CDXOutput::_init = 0;
dx_device_list_t CDXOutput::_deviceList;

//-----------------------------------------------------------------------------
// Процедура обратного вызова списка драйверов
// на входе    :  Id	- идентификатор устройства
//  			  Desc  - имя устройства
//  			  Mod   - имя файла с драйвером
//  			  Data  - указатель на данные звукового двигателя
// на выходе   :  TRUE  - продолжить перечисление
//  			  FALSE - закончить перечисление
//-----------------------------------------------------------------------------
BOOL CALLBACK dx_DirectSoundEnumerate(LPGUID Id, LPSTR Desc, LPSTR Mod,
	HWND hWnd)
{
	int i;
	LPDIRECTSOUND direct;
	LPDIRECTSOUNDBUFFER primary;
	LPDIRECTSOUND3DLISTENER listener;
	dx_device_t* device;

	// подготовка переменных
	direct = 0;
	primary = 0;
	device = 0;

	// создание Direct Sound объекта
	direct = ds_Create(hWnd, CLSID_DirectSound, Id, DSSCL_EXCLUSIVE);

	// создание первичного буфера
	primary = ds_CreatePrimary(direct);

	// создание слушателя
	listener = ds_CreateListener(primary);

	// конфигурирование первичного буфера прошло ?
	if (ds_ConfigurePrimary(primary, 44100, 16, 2)) {
		// новое устройство есть где разместить ?
		if (CDXOutput::_deviceList._size == CDXOutput::_deviceList._capacity)
			CDXOutput::DeviceGrowArray();

		// создадим новую память под новое устройство
		device = (dx_device_t *) malloc(sizeof(dx_device_t));
		memset(device, 0, sizeof(dx_device_t));

		// запомним идентификатор
		device->_id = Id;

		// скопируем имя драйвера
		i = (strlen(Desc) > (DEVICE_NAME_LEN - 1)) ?
			DEVICE_NAME_LEN -
			1 :
			strlen(Desc);
		memcpy(device->_name, Desc, i);
		device->_name[i] = 0;

		// получение параметров устройства воспроизведения
		if (ds_GetCaps(direct, & device->_caps)) {
			// проверка наличия алгоритмов расчета 3D звука
			for (i = 0; i < 4; i++)
				device->_alg[i] = ds_TestAlgorithm(direct, i);

			// проверка наличия EAX интерфейсов
			for (i = 0; i <= EAX_NUM; i++)
				device->_eax[i] = eax_TestSupport(direct, i);

			// проверка наличия ZOOMFX
			device->_zoomfx = zoomfx_TestSupport(direct);

			// включение нового элемента в массив
			i = CDXOutput::_deviceList._size++;
			CDXOutput::_deviceList._array[i] = device;
		} else {
			// освободим память
			free(device);
		}
	}

	// освобождение сдушателя
	ds_ReleaseListener(listener);

	// освобождение первичного буфера
	ds_ReleasePrimary(primary);

	// освобождение устройства воспроизведения
	ds_Release(direct);

	// продолжаем поиск
	return TRUE;
}

//-----------------------------------------------------------------------------
// Cоздание списка устройств воспроизведения
// на входе    :
// на выходе   :
//-----------------------------------------------------------------------------
int CDXOutput::CreateDeviceList(void)
{
	HWND hWnd = 0;

	// проверка наличия устройств воспроизведения
	if (waveOutGetNumDevs() == 0)
		return false;

	// создание скрытого окна
	hWnd = CreateHiddenWindow();
	if (!hWnd)
		return false;

	// перечисление всех устройств воспроизведения
	DirectSoundEnumerate((LPDSENUMCALLBACK) dx_DirectSoundEnumerate, hWnd);

	// удаление скрытого окна
	ReleaseHiddenWindow(hWnd);

	return true;
}

//-----------------------------------------------------------------------------
// Cоздание списка устройств воспроизведения
// на входе    :
// на выходе   :
//-----------------------------------------------------------------------------
void CDXOutput::DeviceGrowArray(void)
{
	dx_device_t** newlist;

	// расширение массива
	_deviceList._capacity += DX_DEVICE_GROWBY;

	// выделение нового массива
	newlist = (dx_device_t * *)
		malloc(_deviceList._capacity * sizeof(dx_device_t *));

	// подготовка нового массива
	memset(newlist, 0, _deviceList._capacity * sizeof(dx_device_t *));

	// копирование старого массива в новый
	if (_deviceList._size > 0)
		memcpy(newlist,
			_deviceList._array,
			_deviceList._size * sizeof(dx_device_t *));

	// освобождение старого массива
	free(_deviceList._array);

	// рапишем новый массив в ядро
	_deviceList._array = newlist;
}

//-----------------------------------------------------------------------------
// Подготовка класса к работе
// на входе    :  *
// на выходе   :  *
//-----------------------------------------------------------------------------
void CDXOutput::Startup()
{
	if (_init == 0) {
		// подготовк памяти
		memset(&_deviceList, 0, sizeof(dx_device_list_t));

		// создание структуры для хранения списка устройств
		DeviceGrowArray();

		// создание списка устройств
		CreateDeviceList();

		// флаг инициализации
		_init = 1;
	}
}

//-----------------------------------------------------------------------------
// Завершение работы с классом
// на входе    :  *
// на выходе   :  *
//-----------------------------------------------------------------------------
void CDXOutput::Shutdown()
{
	_init = 0;
}


//-----------------------------------------------------------------------------
// Конструктор класса
// на входе    :  *
// на выходе   :  *
//-----------------------------------------------------------------------------
CDXOutput::CDXOutput()
{
	Startup();
}

//-----------------------------------------------------------------------------
// Деструктор класса
// на входе    :  *
// на выходе   :  *
//-----------------------------------------------------------------------------
CDXOutput::~CDXOutput()
{
	Shutdown();
}

//-----------------------------------------------------------------------------
// Инициализация DirectSound
// на входе    :  iWindow     - хендлер окна к которому привязан обьект
//  			  iDeсive     - идентификатор устройства воспроизведения
//  			  iRate 	  - частота дискретизации звука
//  			  iBits 	  - количество бит на выборку
//  			  iChannels   - количество каналов
// на выходе	:	успешность инициализации DirectSound
//-----------------------------------------------------------------------------
int CDXOutput::Init(HWND iWindow, unsigned int iDevice, int iRate, int iBits,
	int iChannels)
{
	int ok = false;
	// очистка переменных
	_directSound = 0;
	_primary = 0;
	_listener = 0;

	// проверка номера устройства
	ok = (iDevice < (unsigned int)_deviceList._size) ? true : false;

	// создание Direct Sound объекта
	if (ok) {
		_directSound = ds_Create(iWindow,
					   	CLSID_DirectSound,
					   	_deviceList._array[iDevice]->_id,
					   	DSSCL_EXCLUSIVE);
		ok = _directSound ? true : false;
	}

	// создание первичного буфера
	if (ok) {
		_primary = ds_CreatePrimary(_directSound);
		ok = _primary ? true : false;
	}

	// создание слушателя
	if (ok) {
		_listener = ds_CreateListener(_primary);
		ok = _listener ? true : false;
	}

	// получение параметров слушателя
	if (ok)
		ok = ds_GetAllListenerParameters(_listener, & _listenerParameters);

	// конфигурирование первичного буфера
	if (ok)
		ok = ds_ConfigurePrimary(_primary, iRate, iBits, iChannels);

	// в случае ошибки освободим все что заняли
	if (!ok)
		Free();

	// все прошло успешно
	return ok;
}

//-----------------------------------------------------------------------------
// Освобождение DirectSound обьекта
// на входе		:	*
// на выходе	:	успешность деинициализации DirectSound
//-----------------------------------------------------------------------------
void CDXOutput::Free(void)
{
	// освобождение интерфейса слушателя
	ds_ReleaseListener(_listener);

	// освобождение первичного буфера
	ds_ReleasePrimary(_primary);

	// Освобождение DirectSound интерфейса
	ds_Release(_directSound);

	// очистка переменных
	_listener = 0;
	_primary = 0;
	_directSound = 0;
}

//-----------------------------------------------------------------------------
// получение количества устройств воспроизведения
// на входе    :  
// на выходе   :  
//-----------------------------------------------------------------------------
int CDXOutput::GetNumDevice(void)
{
	// инициализация ядра
	Startup();

	// вернем количество устройств воспроизведения
	return _deviceList._size;
}

//-----------------------------------------------------------------------------
// получение имени устройства воспроизведения
// на входе    :  iDevice  - порядковый номер устройства в списке
// на выходе   :  указатель на строку с именем устройства
//-----------------------------------------------------------------------------
char* CDXOutput::GetDeviceName(int iDevice)
{
	// инициализация ядра
	Startup();

	// такое устройство есть ?
	if (iDevice >= _deviceList._size)
		return 0;

	// вернем имя
	return _deviceList._array[iDevice]->_name;
}
/*
//-----------------------------------------------------------------------------
// Получение идентификатора устройства воспроизведения
// на входе    :  iDevice  - порядковый номер устройства в списке
// на выходе   :  идентификатор устройства
//-----------------------------------------------------------------------------
LPGUID CDXOutput::GetDeviceID(int iDevice)
{
   // инициализация ядра
   Startup();

   // такое устройство есть ?
   if (iDevice >= _deviceList._size)
	  return NULL;

   // вернем идентификатор
   return _deviceList._array[iDevice]->_id;
}
*/
//-----------------------------------------------------------------------------
// Получение указателя на данные устройства
// на входе    :  iDevice  - порядковый номер устройства в списке
// на выходе   :  указатель на данные устройства
//-----------------------------------------------------------------------------
dx_device_t* CDXOutput::GetDevice(int device)
{
	// инициализация ядра
	Startup();

	// такое устройство есть ?
	if (device >= _deviceList._size)
		return 0;

	// вернем идентификатор
	return _deviceList._array[device];
}
