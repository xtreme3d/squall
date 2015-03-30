//-----------------------------------------------------------------------------
//	Работа со звуковыми каналами
//	Копонент звукового двигателя Шквал
//	команда		: AntiTank
//	разработчик	: Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------

// включения
#include <stdio.h>
#include <math.h>
#include "../Squall.h"
#include "Context.h"
#include "SquallApi.h"
#include "DirectSound.h"

//-----------------------------------------------------------------------------
// Инициализация DirectSound
// на входе		:	hWnd			- хендлер окна к которому привязан обьект
//					Deсive			- идентификатор устройства воспроизведения
//					SampleRate		- частота дискретизации звука
//					BitsPerSample	- количество бит на выборку
//					Channels		- количество каналов
// на выходе	:	успешность инициализации DirectSound
//-----------------------------------------------------------------------------
int context_s::InitAudio(HWND window, LPGUID device, int rate, int bits,
	int channels)
{
	int ok = false;
	// очистка переменных
	_directSound = 0;
	_primary = 0;
	_listener = 0;

	// создание Direct Sound объекта
	_directSound = ds_Create(window,
				   	CLSID_DirectSound,
				   	device,
				   	DSSCL_EXCLUSIVE);
	ok = _directSound ? true : false;

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
		ok = ds_ConfigurePrimary(_primary, rate, bits, channels);

	// в случае ошибки освободим все что заняли
	if (!ok)
		FreeAudio();

	// все прошло успешно
	return ok;
}

//-----------------------------------------------------------------------------
// Освобождение DirectSound обьекта
// на входе		:	*
// на выходе	:	успешность деинициализации DirectSound
//-----------------------------------------------------------------------------
void context_s::FreeAudio(void)
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
// Процедура поиска канала по идентификатору
// на входе    :  ChannelID   - идентификатор искомого канала
// на выходе   :  номер канала, в случае неудачи -1
//-----------------------------------------------------------------------------
int context_s::GetChannelIndex(int ChannelID)
{
	int index;
	int count;

	index = ChannelID & 0xFF;
	count = ChannelID >> 8;

	// проверка индекса
	if (index >= _channels)
		return -1;

	// проверка счетчика
	if (_channelsArray[index]._count != count)
		return -1;

	// проверка наполнености канала
	if (_channelsArray[index].Status.STAGE == EMPTY)
		return -1;

	return index;
}

//-----------------------------------------------------------------------------
//	Процедура поиска канала по идентификатору
//	на входе	:	Priority	- приоритет создаваемого канала
//	на выходе	:	номер свободного канала, если значение равно -1
//					нет свободных каналов.
//-----------------------------------------------------------------------------
int context_s::GetFreeChannel(int Priority)
{
	// переменные
	int index;
	float Dist = 0.0f;
	float direct = 0.0f;

	// проверка наличия свободного канала
	for (int i = 0; i < _channels; i++) {
		// канал свободен ?
		if (_channelsArray[i].Status.STAGE == EMPTY) {
			index = i;
			break;
		} else {
			// поиск наименьшего приоритета
			if (Priority >= _channelsArray[i].Priority) {
				Priority = _channelsArray[i].Priority;
				index = i;
				/*  		  // вычисление растояния до источника звука
								 if( _channelsArray[i].DS3D_Buffer != 0 )
								 {
									direct = sqrtf(   powf((_listenerParameters.vPosition.x - _channelsArray[i].Param.ds3d.vPosition.x), 2) +
													  powf((_listenerParameters.vPosition.y - _channelsArray[i].Param.ds3d.vPosition.y), 2) + 
													  powf((_listenerParameters.vPosition.z - _channelsArray[i].Param.ds3d.vPosition.z), 2));
									if( Dist <= direct )
									{
										   index = i;
									   Dist = direct;
									}
								 }*/
			}
		}
	}

	// канал занят ?
	if (_channelsArray[index].Status.STAGE != EMPTY)
		_channelsArray[index].DeleteSoundBuffer();


	return index;
}

int context_s::GetChannelID(int ChannelIndex)
{
	// увеличение ссылки
	if (_channelsArray[ChannelIndex]._count > 0xFFFF)
		_channelsArray[ChannelIndex]._count = 1;
	else
		_channelsArray[ChannelIndex]._count++;

	return _channelsArray[ChannelIndex]._count << 8 | ChannelIndex;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//				Процедуры для работы со звуковым потоком
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
//	Обработчик звуковых каналов в игре
//	на входе	:	указатель на объект звукового двигателя
//	на выходе	:	0 - конец потока
//-----------------------------------------------------------------------------
DWORD WINAPI SoundThreadProc(LPVOID param)
{
	DWORD result;
	int i;
	context_s* Data = (context_s*) param;
	SSample* cur;

	// бесконечный цикл
	while (Data->_event) {
		// небольшая пауза
		if (WaitForSingleObject(Data->_event, 5) == WAIT_TIMEOUT) {
			// проверим возможность обработки
			result = WaitForSingleObject(Data->_mutex, 5);

			// ошибка ожидания
			if (result == WAIT_FAILED) {
				return 0;
			}

			// поток заблокирован
			if (result == WAIT_OBJECT_0) {
				// обработка всех звуков
				for (i = 0, cur = (SSample*) Data->_channelsArray;
					i < Data->_channels;
					i++, cur++) {
					// канал пуст ?
					if (cur->Status.STAGE != EMPTY) {
						// обновление канала
						if (!cur->Update(Data->_listenerParameters.vPosition.x,
								  	Data->_listenerParameters.vPosition.y,
								  	Data->_listenerParameters.vPosition.z)) {
							// удаление канала в случае ошибки
							cur->Stop();
							cur->DeleteSoundBuffer();
						}
					}
				}

				// обработка всех звуков
				for (i = 0, cur = (SSample*) Data->_channelsArray;
					i < Data->_channels;
					i++, cur++) {
					if (cur->Status.STAGE != EMPTY) // канал пуст ?
						cur->UpdateWorker();
				}

				// вызовем обработчик слушателя
				if (Data->_worker) {
					if ((timeGetTime() - Data->_prevWorkerTime) >
						Data->_workerUpdateTime) {
						Data->_prevWorkerTime = timeGetTime();
						// вызов обработчика
						Data->_worker(Data->_workerUserData);
					}
				}

				// освободим мутекс
				ReleaseMutex(Data->_mutex);
			}
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
//	Процедура создания обработчика каналов в игре
//	на входе	:	*
//	на выходе	:	успешность создания обработчика звука
//-----------------------------------------------------------------------------
bool context_s::CreateSoundThread(void)
{
	// создание события
	if (!(_event = CreateEvent(0, TRUE, 0, 0))) {
		return false;
	}

	// создание обработчика 
	if (!(_thread = CreateThread(NULL,
						0,
						(LPTHREAD_START_ROUTINE) SoundThreadProc,
						(LPVOID)this,
						0,
						& _threadID))) {
		return false;
	}

	// установка приоритета
	if (SetThreadPriority(_thread, THREAD_PRIORITY_TIME_CRITICAL) == NULL) {
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
//	Удаление обработчика звуковых каналов
//	на входе	:	*
//	на выходе	:	успешность удаления обработчика звука
//-----------------------------------------------------------------------------
void context_s::DeleteSoundThread(void)
{
	// если есть поток, то удалим его
	if (_thread) {
		TerminateThread(_thread, 0);
		_thread = 0;
	}

	// если есть синхронизирующее событие то удалим его
	if (_event) {
		CloseHandle(_event);
		_event = 0;
	}
}

//-----------------------------------------------------------------------------
//	Инициализация EAX слушателя
//	на входе	:	указатель на переменную в которую нужно поместить EAX
//					интерфейс
//	на выходе	:	успешность
//-----------------------------------------------------------------------------
int context_s::InitAudioExtensions(void)
{
	// объявление переменных
	int i, ok;
	WAVEFORMATEX wfx;		// формат звука
	DSBUFFERDESC dsbd;		// дескриптор
	device_t* device;  // указатель на параметры устройства воспроизведения


	// подготовка параметров
	for (i = 0; i <= EAX_NUM; i++)
		_eaxSupport[i] = 0;
	_useEax = 0;
	_zoomfxSupport = 0;

	_eaxBuffer = 0;
	_eaxListener = 0;

	// получение информации о звуковых расширениях
	device = api_GetDevice(_curDevice);
	if (!device)
		return false;

	// скопируем параметры о EAX расширениях
	for (i = 0; i <= EAX_NUM; i++) {
		_eaxSupport[i] = device->_eax[i];
		if (device->_eax[i])
			_useEax = i;
	}

	// скопируем параметры о ZOOMFX расширении
	_zoomfxSupport = device->_zoomfx;

	// заполним структуру с форматом
	wfx.wFormatTag = 1;
	wfx.nChannels = 2;
	wfx.nSamplesPerSec = 44100;
	wfx.wBitsPerSample = 16;
	wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) >> 3;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	wfx.cbSize = 0;

	// занесение данных вторичного буфера
	ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
	dsbd.dwSize = sizeof(DSBUFFERDESC);
	dsbd.dwFlags = DSBCAPS_STATIC | DSBCAPS_CTRL3D;
	dsbd.dwBufferBytes = 128;
	dsbd.lpwfxFormat = &wfx;

	// проверка наличия устройства воспроизведения
	ok = _directSound ? true : false;

	// Создание вторичного буфера
	if (ok) {
		_eaxBuffer = ds_CreateBuffer(_directSound, & dsbd);
		ok = _eaxBuffer ? true : false;
	}

	// получение EAX интерфейса
	if (ok) {
		_eaxListener = eax_GetBufferInterface(_eaxBuffer);
		ok = _eaxListener ? true : false;
	}

	// в случае ошибки освободим занятые ресурсы
	if (!ok)
		FreeAudioExtensions();

	return ok;
}

//-----------------------------------------------------------------------------
//	Освобождение EAX слушателя
//	на входе	:	*
//	на выходе	:	*
//-----------------------------------------------------------------------------
void context_s::FreeAudioExtensions(void)
{
	// освободим интерфейс EAX слушателя
	if (_eaxListener) {
		_eaxListener->Release();
		_eaxListener = 0;
	}

	// освободим EAX буфер
	ds_ReleaseBuffer(_eaxBuffer);
	_eaxBuffer = 0;
}

//-----------------------------------------------------------------------------
//	Установка нового текущего устройства воспроизведения
// на входе    :  cur_device  - номер текущего устройства воспроизведения 
//  			  new_device  - номер нового устройства воспроизведения
// на выходе   :	успешность установки нового устройства воспроизведения
//-----------------------------------------------------------------------------
int context_s::SetDevice(DWORD cur_device, DWORD new_device)
{
	int result = true;

	// внутренная структура для хранения промежуточных данных
	struct SSaveBuffer {
		SChannelParam Param;
		int PlayPosition;
	};

	int i;
	int ret;		// возвращаемое значение
	int channel = 0;				 // текущий обрабатываемый канал
	LPGUID NewDevice = 0;   			  // идентификатор нового устройства воспроизведения
	LPGUID CurDevice = 0;   			  // идентификатор текущего устройства воспроизведения
	DS3DLISTENER temp_list;  // буфер с промежуточными параметрами слушателя
	SSaveBuffer* temp;  	 // буфер с данными всех каналов

	temp = 0;
	i = 0;

	CurDevice = api_GetDeviceID(cur_device);
	NewDevice = api_GetDeviceID(new_device);

	// сохраним параметры системы
	if (IsTrue(result)) {
		// выделим память под промежуточный буфер
		temp = (SSaveBuffer *) malloc(sizeof(SSaveBuffer) * _channels);

		// удалим звуковые буфера
		for (channel = 0; channel < _channels; channel++) {
			if (_channelsArray[channel].Status.STAGE != EMPTY) {
				// сохраним параметры канала
				memcpy(&temp[channel].Param,
					& _channelsArray[channel].Param,
					sizeof(SChannelParam));
				_channelsArray[channel].GetPlayPosition(&temp[channel].PlayPosition);

				// удалим буфер 3D
				if (_channelsArray[channel].DS3D_Buffer) {
					_channelsArray[channel].DS3D_Buffer->Release();
					_channelsArray[channel].DS3D_Buffer = 0;
				}
				// удалим буфер
				if (_channelsArray[channel].DS_Buffer) {
					_channelsArray[channel].DS_Buffer->Release();
					_channelsArray[channel].DS_Buffer = 0;
				}
			}
		}
		// сохраним параметры слушателя
		memcpy(&temp_list, & _listenerParameters, sizeof(DS3DLISTENER));

		// удалим интерфейс EAX слушателя
		FreeAudioExtensions();

		// удалим текущее устройство воспроизведения
		FreeAudio();
	}

	// установка нового устройства воспроизведения
	if (IsTrue(result))
		if (!InitAudio(_window, NewDevice, _sampleRate, _bitPerSample, 2))
			result = SQUALL_ERROR_CREATE_DIRECT_SOUND;

	// востановление звуковых расширений
	if (IsTrue(result)) {
		InitAudioExtensions();

		// востановим параметры слушателя
		memcpy(&_listenerParameters, & temp_list, sizeof(DS3DLISTENER));
		if (!ds_SetAllListenerParameters(_listener,
			 	& _listenerParameters,
			 	DS3D_IMMEDIATE))
			result = SQUALL_ERROR_SET_LISTENER_PARAM;
	}


	// востановим звуковые буфера
	if (IsTrue(result)) {
		for (channel = 0; channel < _channels; channel++) {
			if (_channelsArray[channel].Status.STAGE != EMPTY) {
				if (_channelsArray[channel].Status.SOUND_TYPE ==
					TYPE_POSITIONED) {
					// создание буфера					   
					ret = _channelsArray[channel].Create3DSoundBuffer(_directSound,
												  	_channelsArray[channel].SoundFile->GetFormat(),
												  	_channelsArray[channel].SoundFile->GetRealMonoDataSize(),
												  	_bufferSize,
												  	_used3DAlgorithm,
												  	_useEax,
												  	_useHW3D);
					// подготовка и установка параметров
					if (ret) {
						memcpy(&_channelsArray[channel].Param,
							& temp[channel].Param,
							sizeof(SChannelParam));
						ret = (_channelsArray[channel].DS3D_Buffer->SetAllParameters(&_channelsArray[channel].Param.ds3d,
																		DS3D_IMMEDIATE) ==
							DS_OK) ?
							true :
							false;
					}
					// установка громкости
					if (ret)
						ret = _channelsArray[channel].SetVolume(temp[channel].Param.Volume);
					// установка частоты
					if (ret)
						ret = _channelsArray[channel].SetFrequency(temp[channel].Param.Frequency);
					// установка позиции воспроизведения
					if (ret)
						ret = _channelsArray[channel].SetPlayPosition(temp[channel].PlayPosition);
				} else {
					// создание буфера
					ret = _channelsArray[channel].CreateSoundBuffer(_directSound,
												  	_channelsArray[channel].SoundFile->GetFormat(),
												  	_channelsArray[channel].SoundFile->GetRealStereoDataSize(),
												  	_bufferSize,
												  	_useHW2D);
					// установка громкости
					if (ret)
						ret = _channelsArray[channel].SetVolume(temp[channel].Param.Volume);
					// установка частоты
					if (ret)
						ret = _channelsArray[channel].SetFrequency(temp[channel].Param.Frequency);
					// установка панорамы
					if (ret)
						ret = _channelsArray[channel].SetPan(temp[channel].Param.Pan);
					// установка позиции воспроизведения
					if (ret)
						ret = _channelsArray[channel].SetPlayPosition(temp[channel].PlayPosition);
				}

				// проверка наличия ошибки
				if (!ret) {
					break;
				}
			}
		}
	}
	/*
	   // в случае неудачи вернем все обратно
	   if (error)
	   {
		  // удалим интерфейс EAX слушателя
		   FreeAudioExtensions();
		   // удалим текущее устройство воспроизведения
		   FreeAudio();
		   // создадим новое устройство воспроизведения
		   error = (InitAudio(hWnd, CurDevice, SampleRate, BitPerSample, 2)) ? AGSS_NO_ERROR : AGSS_ERROR_CREATE_DIRECT_SOUND;
		   if (!error)
		  {
			   // востановим параметры слушателя
			   memcpy(&_listenerParameters, &temp_list, sizeof(DS3DLISTENER));
			   error = (ds_SetAllListenerParameters(_listener, &_listenerParameters, DS3D_IMMEDIATE)) ? AGSS_NO_ERROR : AGSS_ERROR_SET_LISTENER_PARAMETERS;
		   }
		   // востановим EAX сдушателя
		   InitAudioExtensions();
		   // востановим звуковые буфера
		   if (!error)
		  {
			   for (channel = 0; channel < _channels; channel++)
			 {
				   if (_channelsArray[channel].Status.STAGE != EMPTY)
				{
					   if (_channelsArray[channel].Status.SOUND_TYPE == TYPE_POSITIONED)
				   {
					  // создание буфера					   
					  ret = _channelsArray[channel].Create3DSoundBuffer(_directSound, _channelsArray[channel].SoundFile->GetFormat(), _channelsArray[channel].SoundFile->GetRealMonoDataSize(), BufferSize, Used3DAlgorithm, _useEax, HardwareAcceleration3D);
					  // подготовка и установка параметров
					  if (ret)
					  {
							  memcpy(&_channelsArray[channel].Param, &temp[channel].Param, sizeof(SChannelParam));
							  ret = (_channelsArray[channel].DS3D_Buffer->SetAllParameters(&_channelsArray[channel].Param.ds3d, DS3D_IMMEDIATE) == DS_OK) ? true : false;
					  }
					  // установка громкости
					  if (ret)
							  ret = _channelsArray[channel].SetVolume(temp[channel].Param.Volume);
					  // установка частоты
						   if (ret)
							  ret = _channelsArray[channel].SetFrequency(temp[channel].Param.Frequency);
					  // установка позиции воспроизведения
						   if (ret)
							  ret = _channelsArray[channel].SetPlayPosition(temp[channel].PlayPosition);
					   } else
				   {
					  // создание буфера
						   ret = _channelsArray[channel].CreateSoundBuffer(_directSound, _channelsArray[channel].SoundFile->GetFormat(), _channelsArray[channel].SoundFile->GetRealStereoDataSize(), BufferSize, HardwareAcceleration2D);
					  // установка громкости
						   if (ret)
							  ret = _channelsArray[channel].SetVolume(temp[channel].Param.Volume);
					  // установка частоты
						   if (ret)
							   ret = _channelsArray[channel].SetFrequency(temp[channel].Param.Frequency);
					  // установка панорамы
						   if (ret)
							  ret = _channelsArray[channel].SetPan(temp[channel].Param.Pan);
					  // установка позиции воспроизведения
						   if (ret)
							  ret = _channelsArray[channel].SetPlayPosition(temp[channel].PlayPosition);
					   }
				   // проверка наличия ошибки
				   if (!ret)
					  break;
				   }
			   }
		   }
	   }
	*/
	if (temp)
		free(temp);

	return ret;
}
