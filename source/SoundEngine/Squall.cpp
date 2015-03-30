//-----------------------------------------------------------------------------
// Основная программа
// Копонент звукового двигателя Шквал
// команда     : AntiTank
// разработчик : Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------

// включения
#include <stdio.h>
#include <math.h>
#include "../Squall.h"
#include "ext/Eax.h"
#include "ext/ZoomFX.h"
#include "../SoundFile/SoundFile.h"
#include "Context.h"
#include "Channels.h"
#include "../SoundFile/Reader.h"

#include "SquallApi.h"
#include "HiddenWindow.h"
#include "DirectSound.h"

#include "CDXOutput.h"

// подключаемые библиотеки
//#pragma comment(lib, "dxguid")
//#pragma comment(lib, "dsound")
//#pragma comment(lib, "winmm")

// список ошибок, работает только в отладочной версии двигателя
#if SQUALL_DEBUG
static const char* ErrorTable[] = {
	"", "SQUALL_ERROR_NO_SOUND", "SQUALL_ERROR_MEMORY",
	"SQUALL_ERROR_UNINITIALIZED", "SQUALL_ERROR_INVALID_PARAM",
	"SQUALL_ERROR_CREATE_WINDOW", "SQUALL_ERROR_CREATE_DIRECT_SOUND",
	"SQUALL_ERROR_CREATE_THREAD", "SQUALL_ERROR_SET_LISTENER_PARAM",
	"SQUALL_ERROR_GET_LISTENER_PARAM", "SQUALL_ERROR_NO_FREE_CHANNEL",
	"SQUALL_ERROR_CREATE_CHANNEL", "SQUALL_ERROR_CHANNEL_NOT_FOUND",
	"SQUALL_ERROR_SET_CHANNEL_PARAM", "SQUALL_ERROR_GET_CHANNEL_PARAM",
	"SQUALL_ERROR_METHOD", "SQUALL_ERROR_ALGORITHM", "SQUALL_ERROR_NO_EAX",
	"SQUALL_ERROR_EAX_VERSION", "SQUALL_ERROR_SET_EAX_PARAM",
	"SQUALL_ERROR_GET_EAX_PARAM", "SQUALL_ERROR_NO_ZOOMFX",
	"SQUALL_ERROR_SET_ZOOMFX_PARAM", "SQUALL_ERROR_GET_ZOOMFX_PARAM",
	"SQUALL_ERROR_UNKNOWN", "SQUALL_ERROR_SAMPLE_INIT",
	"SQUALL_ERROR_SAMPLE_BAD", "SQUALL_ERROR_SET_MIXER_PARAM",
	"SQUALL_ERROR_GET_MIXER_PARAM", 
};
#endif

// таблица для преобразования громкости
int volume_table[100];

int naturalvolume_to_dxvolume( float volume )
{
	if( volume == 0 ) // andrewp: log10(0) is undefined
		return -10000;

	return (int)floorf(10000.0f * .5f * log10f(volume));
}

int dxvolume_to_squallvolume( int dxvolume )
{
	int result = (int)floorf(100 * expf(dxvolume * (2.303f / (10000.0f * .5f)))); // andrewp: this must correlate with 'volume_table'

	if( result < 0 )
		result = 0;
	else
	if( result > 100 )
		result = 100;
	
	return result;
}

//-----------------------------------------------------------------------------
// Проверка параметров 
// на входе    :  in	- указатель на структуру с входными параметрами
//  			  out   - указатель на структуру куда нужно поместить
//  					  результирующие значения
// на выходе   :  *
//-----------------------------------------------------------------------------
void InitParameters(squall_parameters_t* in, squall_parameters_t* out)
{
	// проверка наличия дескриптора
	if (IsBadReadPtr(in, sizeof(squall_parameters_t))) {
		// заполним значениями по умолчанию
		memset(out, 0, sizeof(squall_parameters_t));
	} else {
		// скопируем для себя
		memcpy(out, in, sizeof(squall_parameters_t));
	}

	// проверка частоты звука
	if (out->SampleRate <= 0)
		out->SampleRate = 44100;
	else if (out->SampleRate > 48000)
		out->SampleRate = 48000;

	// проверка количества бит
	if ((out->BitPerSample != 8) && (out->BitPerSample != 16))
		out->BitPerSample = 16;

	// проверка наличия максимального количества каналов
	if (out->Channels <= 0)
		out->Channels = 16;
	else if (out->Channels > 256)
		out->Channels = 256;

	// проверка размера звукового буфера
	if (out->BufferSize <= 0)
		out->BufferSize = 200;
	else if (out->BufferSize > 5000)
		out->BufferSize = 5000;

	// проверка алгоритма обработки трехмерного пространства
	if ((out->UseAlg < 0) || (out->UseAlg > SQUALL_ALG_3D_LIGHT))
		out->UseAlg = SQUALL_ALG_3D_DEFAULT;

	// проверка режима работы слушателя
	if ((out->ListenerMode != 0) && (out->ListenerMode != 1))
		out->ListenerMode = SQUALL_LISTENER_MODE_IMMEDIATE;

	// фактор дистанции
	if (out->DistanceFactor <= 0.0f) // дистанция не может быть отрицательной или равной 0
		out->DistanceFactor = 1.0f;

	// фактор затухания звука в зависимости от растояния
	if (out->RolloffFactor <= 0.0f)
		out->RolloffFactor = 1.0f;
	else if (out->RolloffFactor > 10.0f)
		out->RolloffFactor = 10.0f;

	// Допплер фактор
	if (out->DopplerFactor <= 0.0f)
		out->DopplerFactor = 1;
	else if (out->DopplerFactor > 10.0f)
		out->DopplerFactor = 10.0f;

	// данные об аппаратной акселерации 2D
	if ((out->UseHW2D != 0) && (out->UseHW2D != 1))
		out->UseHW2D = true;

	// данные об аппаратной акселерации 3D
	if ((out->UseHW3D != 0) && (out->UseHW3D != 1))
		out->UseHW3D = true;

	// проверка окна
	if (!IsWindow((HWND) out->Window))
		out->Window = 0;

	// проверка устройства воспроизведения
	if ((out->Device <= 0) || (out->Device >= api_GetNumDevice()))
		out->Device = 0;
}

//-----------------------------------------------------------------------------
// Блокирование доступа к контексту
// на входе    :  context  - указатель на блокируемый контекст
// на выходе   :  *
//-----------------------------------------------------------------------------
void LockContext(context_t* context)
{
	// проверка наличия контекста
	if (!context)
		return;

	// поток текущий ?
	if (context->_threadID != GetCurrentThreadId())
		WaitForSingleObject(context->_mutex, INFINITE);
}

//-----------------------------------------------------------------------------
// Разблокирование доступа к контексту
// на входе    :  context  - указатель на разблокируемый контекст
// на выходе   :  *
//-----------------------------------------------------------------------------
void UnlockContext(context_t* context)
{
	// проверка наличия контекста
	if (!context)
		return;

	// поток текущий ?
	if (context->_threadID != GetCurrentThreadId())
		ReleaseMutex(context->_mutex);
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//  			   Методы инициализация / освобождения
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
// Инициализация звуковой системы Шквал
// на входе    :  
// SystemParam -  указатель на структуру с параметрами
//  		звукового двигателя для инициализации.
//  		Если параметр будет равен 0 то звуковой
//  		двигатель создаст экземпляр объекта с
//  		следующими параметрами:
// Window 		= 0 	(создать свое окно)
// Device 		= 0 	(устройство по умолчанию)
// SampleRate 		= 44100
// BitPerSample		= 16
// Channels   		= 16
// UseHW2D		= true  (использовать)
// UseHW3D		= true  (использовать)
// UseAlg 		= 0 	(алгоритм по умолчанию)
// BufferSize 		= 200
// ListenerMode		= 0 	(немедленное применение)
// DistanceFactor	= 1.0f
// RolloffFactor	= 1.0f
// DopplerFactor	= 1.0f
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Init(squall_parameters_t* SystemParam)
#else
DLL_API int SQUALL_Init(squall_parameters_t* SystemParam)
#endif
{
	// переменные
	int i = 0;
	context_t* context = 0;
	LPGUID device = 0;
	HWND hidden = 0;
	int result = true;
	bool init = false;
	squall_parameters_t sys;

	// проверка идентификатора класса
#ifndef _USRDLL
	if (IsBadReadPtr(this, sizeof(this)))
		result = SQUALL_ERROR_UNINITIALIZED;
#endif

	// проверка наличия устройства воспроизведения
	if (IsTrue(result))
		if (api_GetNumDevice() == 0)
			result = SQUALL_ERROR_NO_SOUND;


	// создание таблицы громкостей
	for (i = 0; i < 100; ++i)
		volume_table[i] = naturalvolume_to_dxvolume(i * (1 / 99.0f));

	// проверка входных параметров
	InitParameters(SystemParam, & sys);

	// проверка наличия окна к которому нужно привязывать двигатель
	if (IsTrue(result))
		if (!sys.Window) {
			// создание скрытого окна
			hidden = CreateHiddenWindow();
			if (!hidden)
				result = SQUALL_ERROR_CREATE_WINDOW;
			else
				sys.Window = hidden;
		}

#ifndef _USRDLL
	// выделение памяти под данные двигателя
	if (IsTrue(result)) {
		context = api_CreateContext(this);
		if (!context)
			result = SQUALL_ERROR_MEMORY;
	}
#else
	context = &api._context;
#endif

	// заблокируем доступ к контексту
	LockContext(context);

	// занесение параметров
	if (IsTrue(result)) {
		// создадим мутекс
		while (context->_mutex == 0)
			context->_mutex = CreateMutex(0, 0, 0);

		// копирование параметров
		context->_bitPerSample = sys.BitPerSample;
		context->_sampleRate = sys.SampleRate;
		context->_channels = sys.Channels;
		context->_bufferSize = sys.BufferSize;
		context->_used3DAlgorithm = sys.UseAlg;
		context->_deferred = sys.ListenerMode ?
			DS3D_DEFERRED :
			DS3D_IMMEDIATE;
		context->_useHW2D = sys.UseHW2D;
		context->_useHW3D = sys.UseHW3D;
		context->_curDevice = sys.Device;

		// заполнение данными
		context->_prevWorkerTime = timeGetTime();
		context->_hiddenWindow = hidden;
		context->_window = (HWND) sys.Window;

		// отладочная информация
#if SQUALL_DEBUG
		// выведем настройки с которыми стартовал двигатель
		api_LogTime("SQUALL_Init(0x%X)", (void *) SystemParam);
		api_Log("\t\t[out]Window                   = 0x%X", sys.Window);
		api_Log("\t\t[out]Device                   = %d", sys.Device);
		api_Log("\t\t[out]SampleRate               = %d", sys.SampleRate);
		api_Log("\t\t[out]BitPerSample             = %d", sys.BitPerSample);
		api_Log("\t\t[out]Channels                 = %d", sys.Channels);
		api_Log("\t\t[out]UseAlg                   = %d", sys.UseAlg);
		api_Log("\t\t[out]BufferSize               = %d", sys.BufferSize);
		api_Log("\t\t[out]ListenerMode             = %d", sys.ListenerMode);
		api_Log("\t\t[out]DistanceFactor           = %f", sys.DistanceFactor);
		api_Log("\t\t[out]RolloffFactor            = %f", sys.RolloffFactor);
		api_Log("\t\t[out]DopplerFactor            = %f", sys.DopplerFactor);
		api_Log("\t\t[out]UseHW2D                  = %d", sys.UseHW2D);
		api_Log("\t\t[out]UseHW3D                  = %d", sys.UseHW3D);
#endif

		// выделим память под массив звуков
		context->_channelsArray = (SSample *)
			api_malloc(sizeof(SSample) * context->_channels);
		if (context->_channelsArray) {
			// очистим структуру
			memset(context->_channelsArray,
				0,
				sizeof(SSample) * context->_channels);
		} else {
			// ошибка выделения памяти
			result = SQUALL_ERROR_MEMORY;
		}
	}

	if (IsTrue(result)) {
		// получение указателя на идентификатор устройства воспроизведения
		device = api_GetDeviceID(sys.Device);

		// инициализация звуковой системы
		if (!context->InitAudio(context->_window,
					  	device,
					  	context->_sampleRate,
					  	context->_bitPerSample,
					  	2))
			result = SQUALL_ERROR_CREATE_DIRECT_SOUND;
	}

	// инициализация звуковых расширений
	if (IsTrue(result))
		context->InitAudioExtensions();

	// установим параметры слушателя
	if (IsTrue(result)) {
		context->_listenerParameters.flRolloffFactor = sys.RolloffFactor;
		context->_listenerParameters.flDopplerFactor = sys.DopplerFactor;
		context->_listenerParameters.flDistanceFactor = sys.DistanceFactor;
		if (!ds_SetAllListenerParameters(context->_listener,
			 	& context->_listenerParameters,
			 	DS3D_IMMEDIATE))
			result = SQUALL_ERROR_SET_LISTENER_PARAM;
	}

	// создание обработчика звуковых каналов
	if (IsTrue(result))
		if (!context->CreateSoundThread())
			result = SQUALL_ERROR_CREATE_THREAD;

	// проверим наличие ошибок и удалим все ненужное
	if (IsFalse(result)) {
		// удаление обработчика звуковых каналов
		context->DeleteSoundThread();

		// освобождение звуковых расширений
		context->FreeAudioExtensions();

		// освобождение звуковой системы
		context->FreeAudio();

		// удалим массив со звуками
		api_free(context->_channelsArray);
		context->_channelsArray = 0;

		// удалим скрытое окно
		ReleaseHiddenWindow(context->_hiddenWindow);
		context->_hiddenWindow = 0;
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем работу потока
	UnlockContext(context);
	return result;
}
//-----------------------------------------------------------------------------
// Освобождение занятых звуковой системой ресурсов
// на входе    :  *
// на выходе   :  *
//-----------------------------------------------------------------------------
#ifndef _USRDLL
void Squall::Free(void)
#else
DLL_API void SQUALL_Free(void)
#endif
{
	context_t* context = 0;
	int result = true;

#ifndef _USRDLL
	// получение контекста
	context = api_GetContext(this);
	if (!context)
		result = SQUALL_ERROR_UNINITIALIZED;
#else
	context = &api._context;
#endif

#if SQUALL_DEBUG
	api_LogTime("SQUALL_Free()");
#endif

	// заблокируем работу потока
	LockContext(context);

	if (context->_channelsArray) {
		// остановим и удалим все каналы
		for (int i = 0; i < context->_channels; i++) {
			context->_channelsArray[i].Stop();
			context->_channelsArray[i].DeleteSoundBuffer();
		}
	}

	// небольшая пауза
	Sleep(500);

	// удаление обработчика звуковых каналов
	context->DeleteSoundThread();

	// освобождение звуковых расширений
	context->FreeAudioExtensions();

	// освобождение звуковой системы
	context->FreeAudio();

	// удалим массив со звуками
	if (context->_channelsArray) {
		api_free(context->_channelsArray);
		context->_channelsArray = 0;
	}

	// удалим скрытое окно
	if (context->_hiddenWindow) {
		DestroyWindow(context->_hiddenWindow);
		context->_hiddenWindow = 0;
	}

	// разблокируем работу потока
	UnlockContext(context);

	// удалм мутекс
	if (context->_mutex) {
		CloseHandle(context->_mutex);
		context->_mutex = 0;
	}

#ifndef _USRDLL
	// удалим дескриптор
	if (context)
		api_ReleaseContext(this);
#else
	memset(context, 0, sizeof(context_t));
#endif

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif
	// освобождение ядра
	api_Shutdown();
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//  				  Методы управление двигателем
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
// Включение/выключение паузы всех звуковых каналов
// на входе    :  Pause -  флаг включения/выключения паузы
//  					   параметр может принимать слудующие значения
//  					   true  - включить паузу
//  					   false - выключить паузу
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Pause(int Pause)
#else
DLL_API int SQUALL_Pause(int Pause)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Pause(%d)", Pause);
#endif

	// приостановим все звуки в списке
	if (IsTrue(result))
		for (int i = 0; i < context->_channels; i++)
			if (context->_channelsArray[i].Status.STAGE != EMPTY)
				context->_channelsArray[i].Pause(Pause);

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}
//-----------------------------------------------------------------------------
// Остановка всех звуковых каналов
// на входе    :  *
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Stop(void)
#else
DLL_API int SQUALL_Stop(void)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Stop()");
#endif

	// остановим воспроизведение всех каналов
	if (IsTrue(result))
		for (int i = 0; i < context->_channels; i++)
			if (context->_channelsArray[i].Status.STAGE != EMPTY)
				context->_channelsArray[i].Stop();

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//  					Методы настройки двигателя
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
// Установка нового устройства воспроизведения
// на входе    :  Num   -  номер нового устройства воспроизведения, значение 
//  					   параметра должно быть в пределах от 0 до значения
#ifndef _USRDLL
//  					   полученного с помощью метода GetNumDevice.
#else
//  					   полученного с помощью метода SQUALL_GetNumDevice.
#endif
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::SetDevice(int Num)
#else
DLL_API int SQUALL_SetDevice(int Num)
#endif
{
	// защита от дурака
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_SetDevice(%d)", Num);
#endif

	// проверка параметров
	if (IsTrue(result))
		if ((Num < 0) || (Num >= api_GetNumDevice()))
			result = SQUALL_ERROR_INVALID_PARAM;

	// установка нового устройства воспроизведения
	if (IsTrue(result))
		context->SetDevice(context->_curDevice, Num);

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("error = %s", ErrorTable[-result]);
#endif

	// разблокируем работу потока
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение номера текущего устройства воспроизведения
// на входе    :  *
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//  			  в случае успешного вызова результат содержит номер текущего
//  			  устройства воспроизведения
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::GetDevice(void)
#else
DLL_API int SQUALL_GetDevice(void)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_GetDevice()");
#endif

	// вернем номер текущего устройства
	if (IsTrue(result))
		result = context->_curDevice;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("error = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = %d", result);
#endif

	// разблокируем работу потока
	UnlockContext(context);
	return result;
}
//-----------------------------------------------------------------------------
// Включение/выключение использования аппаратной акселерации звука
// на входе    :  UseHW2D  -  флаг определяющий использование аппаратной
//  						  акселерации рассеянных звуковых каналов,
//  						  параметр может принимать следующие значения:
//  						  true   - использовать аппаратную акселерацию
//  						  false  - не использовать аппаратную акселерацию
//  			  UseHW3D  -  флаг определяющий использование аппаратной
//  						  акселерации позиционных звуковых каналов
//  						  параметр может принимать следующие значения:
//  						  true   - использовать аппаратную акселерацию
//  						  false  - не использовать аппаратную акселерацию
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::SetHardwareAcceleration(int UseHW2D, int UseHW3D)
#else
DLL_API int SQUALL_SetHardwareAcceleration(int UseHW2D, int UseHW3D)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_SetHardwareAcceleration(%d, %d)", UseHW2D, UseHW3D);
#endif

	// установка нового устройства воспроизведения
	if (IsTrue(result)) {
		context->_useHW2D = UseHW2D;
		context->_useHW3D = UseHW3D;
		context->SetDevice(context->_curDevice, context->_curDevice);
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("error = %s", ErrorTable[-result]);
#endif

	// разблокируем работу потока
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение текущих настроек использования аппаратной акселерации звука
// на входе    :  UseHW2D  -  указатель на переменную в которую нужно поместить
//  						  текущее значение использования аппаратной
//  						  акселерации для рассеянных звуковых каналов
//  			  UseHW3D  -  указатель на переменную в которую нужно поместить
//  						  текущее значение использования аппаратной
//  						  акселерации для позиционных звуковых каналов
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::GetHardwareAcceleration(int* UseHW2D, int* UseHW3D)
#else
DLL_API int SQUALL_GetHardwareAcceleration(int* UseHW2D, int* UseHW3D)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_GetHardwareAcceleration(0x%X, 0x%X)", UseHW2D, UseHW3D);
#endif

	if (IsBadWritePtr(UseHW2D, sizeof(int)))
		UseHW2D = 0;

	if (IsBadWritePtr(UseHW3D, sizeof(int)))
		UseHW3D = 0;

	// проверка параметров
	if (IsTrue(result))
		if ((UseHW2D == 0) && (UseHW3D == 0))
			result = SQUALL_ERROR_INVALID_PARAM;

	// установка нового устройства воспроизведения
	if (IsTrue(result)) {
		if (UseHW2D)
			*UseHW2D = context->_useHW2D;
		if (UseHW3D)
			*UseHW3D = context->_useHW3D;
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("error = %s", ErrorTable[-result]);
	else {
		if (UseHW2D)
			api_Log("UseHW2D = %d", * UseHW2D);
		if (UseHW3D)
			api_Log("UseHW3D = %d", * UseHW3D);
	}
#endif

	// разблокируем работу потока
	UnlockContext(context);
	return result;
}
//-----------------------------------------------------------------------------
// Установка режима аккустики
// на входе    :  Mode  -  аккустическая модель, параметр может принимать
//  					   следуюшие значения:
//  					   SQUALL_SPEAKER_DEFAULT   - аккустика по умолчанию
//  					   SQUALL_SPEAKER_HEADPHONE - наушники (головные телефоны)
//  					   SQUALL_SPEAKER_MONO  	- моно колонка (1.0)
//  					   SQUALL_SPEAKER_STEREO	- стерео колонки (2.0)
//  					   SQUALL_SPEAKER_QUAD  	- квадро колонки (4.0)
//  					   SQUALL_SPEAKER_SURROUND  - квадро система с буфером
//  												  низких эффектов (4.1)
//  					   SQUALL_SPEAKER_5POINT1   - пяти канальная система с
//  												  буфером низких эффектов (5.1)
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::SetSpeakerMode(int Mode)
#else
DLL_API int SQUALL_SetSpeakerMode(int Mode)
#endif
{
	unsigned int   mode;
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_SetSpeakerMode(%d)", Mode);
#endif

	// проверка параметров
	if (IsTrue(result))
		if ((Mode < 0) || (Mode > SQUALL_SPEAKER_5POINT1))
			result = SQUALL_ERROR_INVALID_PARAM;

	// определение типа аккустики
	if (IsTrue(result)) {
		mode = 0;
		switch (Mode) {
		case SQUALL_SPEAKER_HEADPHONE:
			mode = DSSPEAKER_HEADPHONE;
			break;
		case SQUALL_SPEAKER_MONO:
			mode = DSSPEAKER_MONO;
			break;
		case SQUALL_SPEAKER_STEREO:
			mode = DSSPEAKER_STEREO;
			break;
		case SQUALL_SPEAKER_QUAD:
			mode = DSSPEAKER_QUAD;
			break;
		case SQUALL_SPEAKER_SURROUND:
			mode = DSSPEAKER_SURROUND;
			break;
			//  	   case SQUALL_SPEAKER_5POINT1:
			//  		  mode = DSSPEAKER_5POINT1;
			//  		  break;
			//  	   case SQUALL_SPEAKER_7POINT1:
			//  		  mode = DSSPEAKER_7POINT1;
			//  		break;
		}
	}

	// установка типа аккустики
	if (IsTrue(result))
		if ((!context->_directSound) ||
			(context->_directSound->SetSpeakerConfig(mode) != DS_OK))
			result = SQUALL_ERROR_UNKNOWN;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("error = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
//	Получение текущего режима аккустики
//	на входе	:  *
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//  			  в случае успешного вызова результат содержит номер текущего
//  			  режима аккустики
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::GetSpeakerMode(void)
#else
DLL_API int SQUALL_GetSpeakerMode(void)
#endif
{
	unsigned long  mode = 0;
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_GetSpeakerMode()");
#endif

	// получение типа аккустики
	if (IsTrue(result))
		if ((!context->_directSound) ||
			(context->_directSound->GetSpeakerConfig(&mode) != DS_OK))
			result = SQUALL_ERROR_UNKNOWN;

	// определение типа аккустики
	if (IsTrue(result)) {
		switch (DSSPEAKER_CONFIG(mode)) {
		case DSSPEAKER_HEADPHONE:
			result = SQUALL_SPEAKER_HEADPHONE;
			break;
		case DSSPEAKER_MONO:
			result = SQUALL_SPEAKER_MONO;
			break;
		case DSSPEAKER_STEREO:
			result = SQUALL_SPEAKER_STEREO;
			break;
		case DSSPEAKER_QUAD:
			result = SQUALL_SPEAKER_QUAD;
			break;
		case DSSPEAKER_SURROUND:
			result = SQUALL_SPEAKER_SURROUND;
			break;
			//  	   case DSSPEAKER_5POINT1:
			//  		  result = SQUALL_SPEAKER_5POINT1;
			//  		  break;
			//  	   case DSSPEAKER_7POINT1:
			//  		  result = SQUALL_SPEAKER_7POINT1;
			//  		break;
		}
	}


	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = %d", result);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}
//-----------------------------------------------------------------------------
// Установка алгоритма расчета трехмерного звука
// на входе    :  Algoritm -  код применяемого алгоритма расчета звука
//  						  параметр может принимать следуюшие значения:
//  						  SQUALL_ALG_3D_DEFAULT - алгоритм по умолчанию
//  						  SQUALL_ALG_3D_OFF 	- 2D алгоритм
//  						  SQUALL_ALG_3D_FULL	- полноценный 3D алгоритм
//  						  SQUALL_ALG_3D_LIGTH   - облегченный 3D алгоритм
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Set3DAlgorithm(int Algorithm)
#else
DLL_API int SQUALL_Set3DAlgorithm(int Algorithm)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Set3DAlgorithm(%d)", Algorithm);
#endif

	// проверка параметров
	if (IsTrue(result))
		if ((Algorithm < 0) || (Algorithm > SQUALL_ALG_3D_LIGHT))
			result = SQUALL_ERROR_INVALID_PARAM;

	// проверка поддержки алгоритма
	if (IsTrue(result))
		if (context->_support3DAlgTab[Algorithm])
			context->_support3DAlgTab[Algorithm] = Algorithm;
		else
			result = SQUALL_ERROR_ALGORITHM;

		// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получения текущего алгоритма расчета трехмерного звука
// на входе    :  *
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//  			  в случае успешного вызова результат содержит номер текущего
//  			  алгоритма расчета трехмерного звука, результат может
//  			  принимать слудующие значения:
//  			  SQUALL_ALG_3D_DEFAULT - алгоритм по умолчанию
//  			  SQUALL_ALG_3D_OFF 	- 2D алгоритм
//  			  SQUALL_ALG_3D_FULL	- полноценный 3D алгоритм
//  			  SQUALL_ALG_3D_LIGTH   - облегченный 3D алгоритм
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Get3DAlgorithm(void)
#else
DLL_API int SQUALL_Get3DAlgorithm(void)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Get3DAlgorithm()");
#endif

	// получение алгоритмя
	if (IsTrue(result))
		result = context->_used3DAlgorithm;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = %d", result);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}
//-----------------------------------------------------------------------------
// Установка нового размера звукового буфера в милисекундах
// на входе    :  BufferSize  -  новый размер звукового буфера, в милисекундах
//  							 параметр должен лежать в пределах от 200
//  							 до 5000
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::SetBufferSize(int BufferSize)
#else
DLL_API int SQUALL_SetBufferSize(int BufferSize)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_SetBufferSize(%d)", BufferSize);
#endif

	// проверка параметров
	if ((BufferSize <= 0) || (BufferSize > 5000))
		result = SQUALL_ERROR_INVALID_PARAM;

	// получение алгоритмя
	if (IsTrue(result))
		context->_bufferSize = BufferSize;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение текущего размера звукового буфера в милисекундах
// на входе    :  *
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//  			  в случае успешного вызова результат содержит текущий размер
//  			  звукового буфера в милисекундах
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::GetBufferSize(void)
#else
DLL_API int SQUALL_GetBufferSize(void)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_GetBufferSize()");
#endif

	// получение алгоритмя
	if (IsTrue(result))
		result = context->_bufferSize;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = %d", result);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}
//-----------------------------------------------------------------------------
// Установка внешней системы работы с памятью
// на входе    :  UserAlloc   -  указатель на внешний метод выделения памяти
//  			  UserFree    -  указатель на внешний метод освобождения памяти
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::SetMemoryCallbacks(squall_callback_alloc_t UserAlloc,
	squall_callback_free_t UserFree)
#else
DLL_API int SQUALL_SetMemoryCallbacks(squall_callback_alloc_t UserAlloc,
	squall_callback_free_t UserFree)
#endif
{
	int result = true;

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_SetMemoryCallbacks(0x%X, 0x%X)", UserAlloc, UserFree);
#endif

	// проверка параметров
	if (IsBadCodePtr((FARPROC) UserAlloc) || IsBadCodePtr((FARPROC) UserFree))
		result = SQUALL_ERROR_INVALID_PARAM;

	// установка внешней системы работы с памятью
	api_SetMemoryCallbacks(UserAlloc, UserFree);

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	return result;
}

//-----------------------------------------------------------------------------
// Установка внешних методов для работы с файлами
// на входе    :  UserOpen    -  указатель на внешний метод открытия файлов
//  			  UserSeek    -  указатель на внешний метод позиционирования
//  							 в открытом файле
//  			  UserRead    -  указатель на внешний метод чтения данных из
//  							 открытого файла
//  			  UserClose   -  указатель на внешний метод закрытия открытого
//  							 файла
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::SetFileCallbacks(squall_callback_open_t UserOpen,
	squall_callback_seek_t UserSeek, squall_callback_read_t UserRead,
	squall_callback_close_t UserClose)
#else
DLL_API int SQUALL_SetFileCallbacks(squall_callback_open_t UserOpen,
	squall_callback_seek_t UserSeek, squall_callback_read_t UserRead,
	squall_callback_close_t UserClose)
#endif
{
	int result = true;

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_SetFileCallbacks(0x%X, 0x%X, 0x%X, 0x%X)",
		UserOpen,
		UserSeek,
		UserRead,
		UserClose);
#endif

	// проверка параметров
	if (IsBadCodePtr((FARPROC) UserOpen) ||
		IsBadCodePtr((FARPROC) UserRead) ||
		IsBadCodePtr((FARPROC)
			UserSeek) ||
		IsBadCodePtr((FARPROC)
			UserClose))
		result = SQUALL_ERROR_INVALID_PARAM;

	// установка внешней системы работы с памятью
	api_SetFileCallbacks(UserOpen, UserRead, UserSeek, UserClose);

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	return result;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//  			  Методы для получения информация о системе
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
// Получение количества устройств воспроизведения звука
// на входе    :  *
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки.
//  			  в случае успешного вызова результат содержит количество
//  			  устройств воспроизведения в системе. Если в системе нет
//  			  устройств воспроизведения результат будет равен 0.
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::GetNumDevice(void)
#else
DLL_API int SQUALL_GetNumDevice(void)
#endif
{
	int result = true;

	CDXOutput* dx = 0;//new CDXOutput();

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_GetNumDevice()");
#endif

	// запишем количество устройств воспроизведения
	//result = api_GetNumDevice();
	result = dx->GetNumDevice();

	// отладочная информация
#if SQUALL_DEBUG
	api_Log("\t\tresult = %d", result);
#endif

	// вернем результат работы
	return result;
}

//-----------------------------------------------------------------------------
// Получение имени устройства по указаному номеру
// на входе    :  Num      -  номер устройства, значение параметра должно быть
//  						  в пределах от 0 до значения полученного с помощью
#ifndef _USRDLL
//  						  метода GetNumDevice.
#else
//  						  метода SQUALL_GetNumDevice.
#endif
//  			  Buffer   -  указатель на буфер куда нужно поместить имя
//  						  устройства воспроизведения
//  			  Size     -  размер принимающего буфера в байтах
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::GetDeviceName(int Num, char* Name, int Len)
#else
DLL_API int SQUALL_GetDeviceName(int Num, char* Name, int Len)
#endif
{
	int length;
	char* name;
	int result = true;

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_GetDeviceName(%d, 0x%X, %d)", Num, Name, Len);
#endif

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

	// проверка параметров
	if (IsTrue(result))
		if ((Len <= 0) ||
			(Num < 0) ||
			(Num >= api_GetNumDevice()) ||
			IsBadWritePtr(Name,
																	  	Len))
			result = SQUALL_ERROR_INVALID_PARAM;

	// получение указателя на имя устройства воспроизведения
	if (IsTrue(result)) {
		name = api_GetDeviceName(Num);
		if (!name)
			result = SQUALL_ERROR_UNKNOWN;
	}

	// копирование имени в указанный буфер
	if (IsTrue(result)) {
		length = ((int) strlen(name) > (Len - 1)) ? Len - 1 : strlen(name);
		memcpy(Name, name, length);
		Name[length] = 0;
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("error = %s", ErrorTable[-result]);
	else
		api_Log("\t\t[out]*Name = %s", Name);
#endif

	// результат работы
	return result;
}
//-----------------------------------------------------------------------------
// Получение свойств устройства воспроизведения по указаному номеру
// на входе    :  Num   -  номер устройства, значение параметра должно быть в
//  					   пределах от 0 до значения полученного с помощью
#ifndef _USRDLL
//  					   метода GetNumDevice.
#else
//  					   метода SQUALL_GetNumDevice.
#endif
//  			  Caps  -  указатель на структуру в которую нужно поместить
//  					   свойства устройства воспроизведения
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::GetDeviceCaps(int Num, squall_device_caps_t* Caps)
#else
DLL_API int SQUALL_GetDeviceCaps(int Num, squall_device_caps_t* Caps)
#endif
{
	device_t* dev;
	int result = true;

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_GetDeviceCaps(%d, 0x%X)", Num, Caps);
#endif

	// проверка наличия устройства воспроизведения
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

	// проверка параметров
	if (IsTrue(result))
		if ((Num < 0) ||
			(Num >= api_GetNumDevice()) ||
			IsBadWritePtr(Caps,
				sizeof(squall_device_caps_t)))
			result = SQUALL_ERROR_INVALID_PARAM;

	// Получение праметров устройства
	if (IsTrue(result)) {
		// подготовка струкруты перед заполнением
		memset(Caps, 0, sizeof(squall_device_caps_t));

		// получение указателя на данные устройства воспроизведения
		dev = api_GetDevice(Num);
		if (dev) {
			// проверка наличия аппаратного смешивания каналов
			if (dev->_caps.dwFreeHwMixingAllBuffers != 0) {
				Caps->HardwareChannels = dev->_caps.dwFreeHwMixingAllBuffers;
				Caps->Flags |= SQUALL_DEVICE_CAPS_HARDWARE;
			}

			// проверка наличия аппараного смешивания 3D каналов
			if (dev->_caps.dwFreeHw3DAllBuffers != 0) {
				Caps->Hardware3DChannels = dev->_caps.dwFreeHw3DAllBuffers;
				Caps->Flags |= SQUALL_DEVICE_CAPS_HARDWARE_3D;
			}
			// проверка наличия поддержки EAX 1.0
			if (dev->_eax[1] != 0)
				Caps->Flags |= SQUALL_DEVICE_CAPS_EAX10;

			// проверка наличия поддержки EAX 2.0
			if (dev->_eax[2] != 0)
				Caps->Flags |= SQUALL_DEVICE_CAPS_EAX20;

			// проверка наличия поддержки EAX 3.0
			if (dev->_eax[3] != 0)
				Caps->Flags |= SQUALL_DEVICE_CAPS_EAX30;

			// проверка наличия поддержки ZOOMFX
			if (dev->_zoomfx != 0)
				Caps->Flags |= SQUALL_DEVICE_CAPS_ZOOMFX;
		} else
			result = SQUALL_ERROR_UNKNOWN;
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("error = %s", ErrorTable[-result]);
	else {
		api_Log("\t\t[out]Caps->Flags              = %d", Caps->Flags);
		api_Log("\t\t[out]Caps->HardwareChannels   = %d",
			Caps->HardwareChannels);
		api_Log("\t\t[out]Caps->Hardware3DChannels = %d",
			Caps->Hardware3DChannels);
	}
#endif

	return result;
}
//-----------------------------------------------------------------------------
// Получение используемой версии EAX интерфейса
// на входе    :  *
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//  			  в случае успешного вызова результат содержит максимально
//  			  доступную версию EAX
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::GetEAXVersion(void)
#else
DLL_API int SQUALL_GetEAXVersion(void)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_GetEAXVersion()");
#endif

	// получение громкости
	if (IsTrue(result))
		result = context->_useEax;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = %d", result);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}
//-----------------------------------------------------------------------------
// Получение информации о каналах
// на входе    :  Info  -  указатель на структуру в которую нужно поместить
//  					   информацию о состоянии каналов
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::GetChannelsInfo(squall_channels_t* Info)
#else
DLL_API int SQUALL_GetChannelsInfo(squall_channels_t* Info)
#endif
{
	// переменные
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_GetChannelsInfo(0x%X)", Info);
#endif

	// проверка праметров
	if (IsTrue(result))
		if (IsBadWritePtr(Info, sizeof(squall_channels_t)))
			result = SQUALL_ERROR_INVALID_PARAM;
		else
			memset(Info, 0, sizeof(squall_channels_t));

		// сбор информации
	if (IsTrue(result)) {
		for (int i = 0; i < context->_channels; i++) {
			if (context->_channelsArray[i].Status.STAGE != EMPTY) {
				// канал рассеянный ?
				if (context->_channelsArray[i].Status.SOUND_TYPE ==
					TYPE_AMBIENT) {
					if (context->_channelsArray[i].Status.STAGE == PLAY)
						Info->Play++;    // количество воспроизводимых рассеянных каналов
					if (context->_channelsArray[i].Status.STAGE == PAUSE)
						Info->Pause++;  	// количество стоящих в паузе рассеянных каналов
					if (context->_channelsArray[i].Status.STAGE == PREPARE)
						Info->Prepare++;	// количество подготовленныз рассеянных каналов
				}

				// канал позиционный ?
				if (context->_channelsArray[i].Status.SOUND_TYPE ==
					TYPE_POSITIONED) {
					if (context->_channelsArray[i].Status.STAGE == PLAY)
						Info->Play3D++;    // количество воспроизводимых позиционных каналов
					if (context->_channelsArray[i].Status.STAGE == PAUSE)
						Info->Pause3D++;	  // количество стоящих в паузе позиционных каналов
					if (context->_channelsArray[i].Status.STAGE == PREPARE)
						Info->Prepare3D++;    // количество подготовленных позиционных каналов
				}
			}
		}
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else {
		api_Log("\t\t[out]Info->Play        = %d", Info->Play);
		api_Log("\t\t[out]Info->Pause       = %d", Info->Pause);
		api_Log("\t\t[out]Info->Prepare     = %d", Info->Prepare);
		api_Log("\t\t[out]Info->Play3D      = %d", Info->Play3D);
		api_Log("\t\t[out]Info->Pause3D     = %d", Info->Pause3D);
		api_Log("\t\t[out]Info->Prepare3D   = %d", Info->Prepare3D);
	}
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//  				  Методы для работы со слушателем
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
// Установка новых параметров слушателя
// на входе    :  Position -  указатель на структуру с новыми координатами
//  						  позиции слушателя. В случае если позицию слушателя
//  						  изменять не нужно, то данный параметр должен
//  						  содержать 0 
//  			  Front    -  указатель на структуру с новым вектором 
//  						  фронтального направления слушателя. В случае еслт
//  						  вектор фронтального направления слушателя изменять
//  						  не нужно, то данный парамерт должен содержать 0
//  			  Top      -  указатель на структуру с новым вектором
//  						  вертикального направления слушателя. В случае если
//  						  вектор вертикального направления изменять не нужно,
//  						  то данный параметр должен содержать 0
//  			  Velocity -  указатель на структуру с новым вектором
//  						  скорости перемещения слушателя. В случае если
//  						  скорость перемещения слушателя изменять не нужно,
//  						  то данный параметр должен содержать 0.
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Listener_SetParameters(float* Position, float* Front, float* Top,
	float* Velocity)
#else
DLL_API int SQUALL_Listener_SetParameters(float* Position, float* Front,
	float* Top, float* Velocity)
#endif
{
	int change;
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Listener_SetParameters(0x%X, 0x%X, 0x%X, 0x%X)",
		Position,
		Front,
		Top,
		Velocity);
#endif

	// проверка параметров
	if (IsBadReadPtr(Position, sizeof(float) * 3))
		Position = 0;
	if (IsBadReadPtr(Front, sizeof(float) * 3))
		Front = 0;
	if (IsBadReadPtr(Top, sizeof(float) * 3))
		Top = 0;
	if (IsBadReadPtr(Velocity, sizeof(float) * 3))
		Velocity = 0;

	// отладочная информация
#if SQUALL_DEBUG
	if (Position) {
		api_Log("\t\t[in]Position->x = %f", Position[0]);
		api_Log("\t\t[in]Position->y = %f", Position[1]);
		api_Log("\t\t[in]Position->z = %f", Position[2]);
	}
	if (Front) {
		api_Log("\t\t[in]Front->x    = %f", Front[0]);
		api_Log("\t\t[in]Front->y    = %f", Front[1]);
		api_Log("\t\t[in]Front->z    = %f", Front[2]);
	}
	if (Top) {
		api_Log("\t\t[in]Top->x      = %f", Top[0]);
		api_Log("\t\t[in]Top->y      = %f", Top[1]);
		api_Log("\t\t[in]Top->z      = %f", Top[2]);
	}
	if (Velocity) {
		api_Log("\t\t[in]Velocity->x = %f", Velocity[0]);
		api_Log("\t\t[in]Velocity->y = %f", Velocity[1]);
		api_Log("\t\t[in]Velocity->z = %f", Velocity[2]);
	}
#endif

	// проверим наличие слушателя
	if (IsTrue(result))
		if (!context->_listener)
			result = SQUALL_ERROR_UNKNOWN;

	// установка параметров
	if (IsTrue(result)) {
		change = false;
		// параметр опущен ?
		if (Position) {
			// проверка изменений
			if ((Position[0] != context->_listenerParameters.vPosition.x) ||
				(Position[1] != context->_listenerParameters.vPosition.y) ||
				(Position[2] != context->_listenerParameters.vPosition.z)) {
				context->_listenerParameters.vPosition.x = Position[0];
				context->_listenerParameters.vPosition.y = Position[1];
				context->_listenerParameters.vPosition.z = Position[2];
				change = true; // параметры изменились
			}
		}

		// параметр опущен ?
		if (Velocity) {
			// проверка изменений
			if ((Velocity[0] != context->_listenerParameters.vVelocity.x) ||
				(Velocity[1] != context->_listenerParameters.vVelocity.y) ||
				(Velocity[2] != context->_listenerParameters.vVelocity.z)) {
				context->_listenerParameters.vVelocity.x = Velocity[0];
				context->_listenerParameters.vVelocity.y = Velocity[1];
				context->_listenerParameters.vVelocity.z = Velocity[2];
				change = true; // параметры изменились
			}
		}

		// параметр опущен ?
		if (Front) {
			// проверка изменений
			if ((Front[0] != context->_listenerParameters.vOrientFront.x) ||
				(Front[1] != context->_listenerParameters.vOrientFront.y) ||
				(Front[2] != context->_listenerParameters.vOrientFront.z)) {
				context->_listenerParameters.vOrientFront.x = Front[0];
				context->_listenerParameters.vOrientFront.y = Front[1];
				context->_listenerParameters.vOrientFront.z = Front[2];
				change = true; // параметры изменились
			}
		}

		// параметр опущен ?
		if (Top) {
			// проверка изменений
			if ((Top[0] != context->_listenerParameters.vOrientTop.x) ||
				(Top[1] != context->_listenerParameters.vOrientTop.y) ||
				(Top[2] != context->_listenerParameters.vOrientTop.z)) {
				context->_listenerParameters.vOrientTop.x = Top[0];
				context->_listenerParameters.vOrientTop.y = Top[1];
				context->_listenerParameters.vOrientTop.z = Top[2];
				change = true; // параметры изменились
			}
		}

		// установка новых параметров
		if (change)
			if (!ds_SetAllListenerParameters(context->_listener,
				 	& context->_listenerParameters,
				 	context->_deferred))
				result = SQUALL_ERROR_SET_LISTENER_PARAM;
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение текущих параметров слушателя
// на входе    :  Position -  указатель на структуру в которую нужно 
//  						  поместить текущие координаты слушателя. В случае
//  						  если значение координат слушателя не требуется
//  						  определять, то данный параметр должен содержать 0.
//  			  Front    -  указатель на структуру в которую нужно
//  						  поместить текущий вектор фронтального 
//  						  направления слушателя. В случае если вектор
//  						  фронтального направления слушателя не требуется
//  						  определять, то данный параметр должен содержать 0.
//  			  Top      -  указатель на структуру в которую нужно
//  						  поместить текущий вектор вертикального 
//  						  направления слушателя. В случае если вектор
//  						  вертикального направления слушателя не требуется
//  						  определять, то данный параметр должен содержать 0.
//  			  Velocity -  указатель на структуру в которую нужно
//  						  поместить текущий вектор скорости перемещения
//  						  слушателя. В случае если скорость перемещения
//  						  слушателя не требуется определять, то данный
//  						  параметр должен содержать 0.
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Listener_GetParameters(float* Position, float* Front, float* Top,
	float* Velocity)
#else
DLL_API int SQUALL_Listener_GetParameters(float* Position, float* Front,
	float* Top, float* Velocity)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Listener_GetParameters(0x%X, 0x%X, 0x%X, 0x%X)",
		Position,
		Front,
		Top,
		Velocity);
#endif

	// проверка параметров
	if (IsBadWritePtr(Position, sizeof(float) * 3))
		Position = 0;
	if (IsBadWritePtr(Front, sizeof(float) * 3))
		Front = 0;
	if (IsBadWritePtr(Top, sizeof(float) * 3))
		Top = 0;
	if (IsBadWritePtr(Velocity, sizeof(float) * 3))
		Velocity = 0;

	// проверим наличие слушателя
	if (IsTrue(result))
		if (!context->_listener)
			result = SQUALL_ERROR_UNKNOWN;

	// защита от дурака
	if (IsTrue(result)) {
		// параметр опущен ?
		if (Position) {
			Position[0] = context->_listenerParameters.vPosition.x;
			Position[1] = context->_listenerParameters.vPosition.y;
			Position[2] = context->_listenerParameters.vPosition.z;
		}

		// Параметр опущен ?
		if (Front) {
			Front[0] = context->_listenerParameters.vOrientFront.x;
			Front[1] = context->_listenerParameters.vOrientFront.y;
			Front[2] = context->_listenerParameters.vOrientFront.z;
		}

		// Параметр опущен ?
		if (Top) {
			Top[0] = context->_listenerParameters.vOrientTop.x;
			Top[1] = context->_listenerParameters.vOrientTop.y;
			Top[2] = context->_listenerParameters.vOrientTop.z;
		}

		// Параметр опущен ?
		if (Velocity) {
			Velocity[0] = context->_listenerParameters.vVelocity.x;
			Velocity[1] = context->_listenerParameters.vVelocity.y;
			Velocity[2] = context->_listenerParameters.vVelocity.z;
		}
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else {
		if (Position) {
			api_Log("\t\t[out]Position->x = %f", Position[0]);
			api_Log("\t\t[out]Position->y = %f", Position[1]);
			api_Log("\t\t[out]Position->z = %f", Position[2]);
		}
		if (Front) {
			api_Log("\t\t[out]Front->x    = %f", Front[0]);
			api_Log("\t\t[out]Front->y    = %f", Front[1]);
			api_Log("\t\t[out]Front->z    = %f", Front[2]);
		}
		if (Top) {
			api_Log("\t\t[out]Top->x      = %f", Top[0]);
			api_Log("\t\t[out]Top->y      = %f", Top[1]);
			api_Log("\t\t[out]Top->z      = %f", Top[2]);
		}
		if (Velocity) {
			api_Log("\t\t[out]Velocity->x = %f", Velocity[0]);
			api_Log("\t\t[out]Velocity->y = %f", Velocity[1]);
			api_Log("\t\t[out]Velocity->z = %f", Velocity[2]);
		}
	}
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Установка новой скорости перемещения слушателя
// на входе    :  Velocity -  указатель на структуру с новым вектором 
//  						  скорости перемещения слушателя.
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Listener_SetVelocity(float* Velocity)
#else
DLL_API int SQUALL_Listener_SetVelocity(float* Velocity)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Listener_SetVelocity(0x%X)", Velocity);
#endif

	// проверка параметров
	if (IsBadReadPtr(Velocity, sizeof(float) * 3)) {
		Velocity = 0;
		result = SQUALL_ERROR_INVALID_PARAM;
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (Velocity) {
		api_Log("\t\t[in]Velocity->x = %f", Velocity[0]);
		api_Log("\t\t[in]Velocity->y = %f", Velocity[1]);
		api_Log("\t\t[in]Velocity->z = %f", Velocity[2]);
	}
#endif

	// проверка наличия слушателя
	if (IsTrue(result))
		if (!context->_listener)
			result = SQUALL_ERROR_UNKNOWN;

	// установка параметров
	if (IsTrue(result)) {
		// проверка изменений
		if ((Velocity[0] != context->_listenerParameters.vVelocity.x) ||
			(Velocity[1] != context->_listenerParameters.vVelocity.y) ||
			(Velocity[2] != context->_listenerParameters.vVelocity.z)) {
			// запишем новые параметры скорости слушателя
			context->_listenerParameters.vVelocity.x = Velocity[0];
			context->_listenerParameters.vVelocity.y = Velocity[1];
			context->_listenerParameters.vVelocity.z = Velocity[2];
			// установка новых параметров скорости слушателя
			if (context->_listener->SetVelocity(context->_listenerParameters.vVelocity.x,
										context->_listenerParameters.vVelocity.y,
										context->_listenerParameters.vVelocity.z,
										context->_deferred) !=
				DS_OK)
				result = SQUALL_ERROR_SET_LISTENER_PARAM;
		}
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение текущей скорости перемещения слушателя
// на входе    :  Velocity - указатель на структуру в которую нужно
//  						 поместить текущий вектор скорости перемещения
//  						 слушателя.
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Listener_GetVelocity(float* Velocity)
#else
DLL_API int SQUALL_Listener_GetVelocity(float* Velocity)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Listener_GetVelocity(0x%X)", Velocity);
#endif

	// проверка параметров
	if (IsBadWritePtr(Velocity, sizeof(float) * 3)) {
		Velocity = 0;
		result = SQUALL_ERROR_INVALID_PARAM;
	}

	// проверка наличия слушателя
	if (IsTrue(result))
		if (!context->_listener)
			result = SQUALL_ERROR_UNKNOWN;

	// получение параметров
	if (IsTrue(result)) {
		Velocity[0] = context->_listenerParameters.vVelocity.x;
		Velocity[1] = context->_listenerParameters.vVelocity.y;
		Velocity[2] = context->_listenerParameters.vVelocity.z;
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else {
		api_Log("\t\t[out]Velocity->x = %f", Velocity[0]);
		api_Log("\t\t[out]Velocity->y = %f", Velocity[1]);
		api_Log("\t\t[out]Velocity->z = %f", Velocity[2]);
	}
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Установка новой позиции слушателя в пространстве
// на входе    :  Position -  указатель на структуру с новым вектором
//  						  скорости перемещения слушателя.
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Listener_SetPosition(float* Position)
#else
DLL_API int SQUALL_Listener_SetPosition(float* Position)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Listener_SetPosition(0x%X)", Position);
#endif

	// проверка параметров
	if (IsBadReadPtr(Position, sizeof(float) * 3)) {
		Position = 0;
		result = SQUALL_ERROR_INVALID_PARAM;
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (Position) {
		api_Log("\t\t[in]Position->x = %f", Position[0]);
		api_Log("\t\t[in]Position->y = %f", Position[1]);
		api_Log("\t\t[in]Position->z = %f", Position[2]);
	}
#endif

	// проверка наличия слушателя
	if (IsTrue(result))
		if (!context->_listener)
			result = SQUALL_ERROR_UNKNOWN;

	// установка параметров
	if (IsTrue(result)) {
		// проверка изменений
		if ((Position[0] != context->_listenerParameters.vPosition.x) ||
			(Position[1] != context->_listenerParameters.vPosition.y) ||
			(Position[2] != context->_listenerParameters.vPosition.z)) {
			// запомним новые координаты слушателя
			context->_listenerParameters.vPosition.x = Position[0];
			context->_listenerParameters.vPosition.y = Position[1];
			context->_listenerParameters.vPosition.z = Position[2];
			// установка новых координат слушателя
			if (context->_listener->SetPosition(Position[0],
										Position[1],
										Position[2],
										DS3D_IMMEDIATE) != DS_OK)
				result = SQUALL_ERROR_SET_LISTENER_PARAM;
		}
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение текущей позиции слушателя в пространстве
// на входе    :  Position -  указатель на структуру в которую нужно
//  						  поместить текущий вектор скорости перемещения
//  						  слушателя.
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Listener_GetPosition(float* Position)
#else
DLL_API int SQUALL_Listener_GetPosition(float* Position)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Listener_GetPosition(0x%X)", Position);
#endif

	// проверка параметров
	if (IsBadWritePtr(Position, sizeof(float) * 3)) {
		Position = 0;
		result = SQUALL_ERROR_INVALID_PARAM;
	}

	// проверка наличия слушателя
	if (IsTrue(result))
		if (!context->_listener)
			result = SQUALL_ERROR_UNKNOWN;

	// получение параметров
	if (IsTrue(result)) {
		Position[0] = context->_listenerParameters.vPosition.x;
		Position[1] = context->_listenerParameters.vPosition.y;
		Position[2] = context->_listenerParameters.vPosition.z;
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else {
		// выведем информацию о позиции слушателя
		api_Log("\t\t[out]Position->x = %f", Position[0]);
		api_Log("\t\t[out]Position->y = %f", Position[1]);
		api_Log("\t\t[out]Position->z = %f", Position[2]);
	}
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}
//-----------------------------------------------------------------------------
// Установка нового коэффициента преобразования дистанции
// на входе    :  DistanceFactor -  новый коэффициент преобразования дистанции
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Listener_SetDistanceFactor(float DistanceFactor)
#else
DLL_API int SQUALL_Listener_SetDistanceFactor(float DistanceFactor)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Listener_SetDistanceFactor(%f)", DistanceFactor);
#endif

	// проверка параметров
	if ((DistanceFactor <= 0.0f) || (DistanceFactor > 10.0f))
		result = SQUALL_ERROR_INVALID_PARAM;

	// проверка наличия слушателя
	if (IsTrue(result))
		if (!context->_listener)
			result = SQUALL_ERROR_UNKNOWN;

	// установка параметров
	if (IsTrue(result)) {
		// проверка изменений
		if (context->_listenerParameters.flDistanceFactor != DistanceFactor) {
			// установка нового коэфициента
			context->_listenerParameters.flDistanceFactor = DistanceFactor;
			if (context->_listener->SetDistanceFactor(DistanceFactor,
										context->_deferred) != DS_OK)
				result = SQUALL_ERROR_SET_LISTENER_PARAM;
		}
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}
//-----------------------------------------------------------------------------
// Получение текущего коэффициента преобразования дистанции
// на входе    :  DistanceFactor -  указатель на переменную в которую нужно
//  								поместить текущий коэффициент 
//  								преобразования дистанции
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Listener_GetDistanceFactor(float* DistanceFactor)
#else
DLL_API int SQUALL_Listener_GetDistanceFactor(float* DistanceFactor)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Listener_GetDistanceFactor(0x%X)", DistanceFactor);
#endif

	// проверка параметров
	if (IsTrue(result))
		if (IsBadWritePtr(DistanceFactor, sizeof(float)))
			result = SQUALL_ERROR_INVALID_PARAM;

	// проверка наличия слушателя
	if (IsTrue(result))
		if (!context->_listener)
			result = SQUALL_ERROR_UNKNOWN;

	// получение параметров
	if (IsTrue(result))
		*DistanceFactor = context->_listenerParameters.flDistanceFactor;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\t[out]*DistanceFactor = %f", * DistanceFactor);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}
//-----------------------------------------------------------------------------
// Установка нового коэффициента затухания звука в зависимости от растояния
// на входе    :  RolloffFactor  -  новый коэффициент преобразования затухания
//  								звука,  значение параметра должно быть
//  								в пределах от 0.1f до 10.0f
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Listener_SetRolloffFactor(float RolloffFactor)
#else
DLL_API int SQUALL_Listener_SetRolloffFactor(float RolloffFactor)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Listener_SetRolloffFactor(%f)", RolloffFactor);
#endif

	// проверка параметров
	if ((RolloffFactor <= 0.0f) || (RolloffFactor > 10.0f))
		result = SQUALL_ERROR_INVALID_PARAM;

	// проверка наличия слушателя
	if (IsTrue(result))
		if (!context->_listener)
			result = SQUALL_ERROR_UNKNOWN;

	if (IsTrue(result)) {
		// проверка изменений
		if (context->_listenerParameters.flRolloffFactor != RolloffFactor) {
			// установка нового коэфициента
			context->_listenerParameters.flRolloffFactor = RolloffFactor;
			if (context->_listener->SetRolloffFactor(RolloffFactor,
										context->_deferred) != DS_OK)
				result = SQUALL_ERROR_SET_LISTENER_PARAM;
		}
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}
//-----------------------------------------------------------------------------
// Получение текущего коэффициента затухания звука в зависимости от растояния
// на входе    :  RolloffFactor  -  указатель на переменную в которую нужно
//  								поместить текущий коэффициент преобразования
//  								затухания звука
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Listener_GetRolloffFactor(float* RolloffFactor)
#else
DLL_API int SQUALL_Listener_GetRolloffFactor(float* RolloffFactor)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Listener_GetRolloffFactor(%f)", RolloffFactor);
#endif

	// проверка параметров
	if (IsTrue(result))
		if (IsBadWritePtr(RolloffFactor, sizeof(float)))
			result = SQUALL_ERROR_INVALID_PARAM;

	// проверка наличия слушателя
	if (IsTrue(result))
		if (!context->_listener)
			result = SQUALL_ERROR_UNKNOWN;

	// получение параметров
	if (IsTrue(result))
		*RolloffFactor = context->_listenerParameters.flRolloffFactor;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\t[out]*RolloffFactor = %f", * RolloffFactor);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Установка нового коэффициента эффекта Допплера
// на входе    :  DopplerFactor  -  новый коэффициент эффекта Допплера, значение
//  								параметра должно быть в пределах от 0.1f
//  								до 10.0f
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Listener_SetDopplerFactor(float DopplerFactor)
#else
DLL_API int SQUALL_Listener_SetDopplerFactor(float DopplerFactor)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Listener_SetDopplerFactor(%f)", DopplerFactor);
#endif

	// проверка параметров
	if ((DopplerFactor <= 0.0f) || (DopplerFactor > 10.0f))
		result = SQUALL_ERROR_INVALID_PARAM;

	// проверка наличия слушателя
	if (IsTrue(result))
		if (!context->_listener)
			result = SQUALL_ERROR_UNKNOWN;

	if (IsTrue(result)) {
		// проверка изменений
		if (context->_listenerParameters.flDopplerFactor != DopplerFactor) {
			// установка нового коэфициента
			context->_listenerParameters.flDopplerFactor = DopplerFactor;
			if (context->_listener->SetDopplerFactor(DopplerFactor,
										context->_deferred) != DS_OK)
				result = SQUALL_ERROR_SET_LISTENER_PARAM;
		}
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение текущего коэффициента эффекта Допплера
// на входе    :  DopplerFactor  -  указатель на переменную в которую нужно
//  								поместить текущий коэффициент эффекта
//  								Допплера
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Listener_GetDopplerFactor(float* DopplerFactor)
#else
DLL_API int SQUALL_Listener_GetDopplerFactor(float* DopplerFactor)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Listener_GetDopplerFactor(0x%X)", DopplerFactor);
#endif

	// проверка параметров
	if (IsTrue(result))
		if (IsBadWritePtr(DopplerFactor, sizeof(float)))
			result = SQUALL_ERROR_INVALID_PARAM;

	// проверка наличия слушателя
	if (IsTrue(result))
		if (!context->_listener)
			result = SQUALL_ERROR_UNKNOWN;

	// получение параметров
	if (IsTrue(result))
		*DopplerFactor = context->_listenerParameters.flDopplerFactor;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\t[out]*DopplerFactor = %f", * DopplerFactor);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Обновление трехмерных настроек
// на входе    :  *
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
// примечание  :  данный метод имеет смысл применять только в том случае когда
//  			  слушатель настроен на принудительное обновление трехмерных
//  			  настроек. По есть при инициализации двигателя слушатель
//  			  переведен в режим SQUALL_LISTENER_MODE_DEFERRED
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Listener_Update(void)
#else
DLL_API int SQUALL_Listener_Update(void)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Listener_Update()");
#endif

	// проверка наличия слушателя
	if (IsTrue(result))
		if (!context->_listener)
			result = SQUALL_ERROR_UNKNOWN;
		else if (context->_listener->CommitDeferredSettings() != DS_OK)
			result = SQUALL_ERROR_UNKNOWN;

		// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Установка предустановленных значения окружения
// на входе    :  Preset   -  номер предустановленного значения
//  						  параметр может принимать следуюшие значения:
//  						  SQUALL_EAX_OFF
//  						  SQUALL_EAX_GENERIC
//  						  SQUALL_EAX_PADDEDCELL
//  						  SQUALL_EAX_ROOM
//  						  SQUALL_EAX_BATHROOM
//  						  SQUALL_EAX_LIVINGROOM
//  						  SQUALL_EAX_STONEROOM
//  						  SQUALL_EAX_AUDITORIUM
//  						  SQUALL_EAX_CONCERTHALL
//  						  SQUALL_EAX_CAVE
//  						  SQUALL_EAX_ARENA
//  						  SQUALL_EAX_HANGAR
//  						  SQUALL_EAX_CARPETEDHALLWAY
//  						  SQUALL_EAX_HALLWAY
//  						  SQUALL_EAX_STONECORRIDOR
//  						  SQUALL_EAX_ALLEY
//  						  SQUALL_EAX_FOREST
//  						  SQUALL_EAX_CITY
//  						  SQUALL_EAX_MOUNTAINS
//  						  SQUALL_EAX_QUARRY
//  						  SQUALL_EAX_PLAIN
//  						  SQUALL_EAX_PARKINGLOT
//  						  SQUALL_EAX_SEWERPIPE
//  						  SQUALL_EAX_UNDERWATER
//  						  SQUALL_EAX_DRUGGED
//  						  SQUALL_EAX_DIZZY
//  						  SQUALL_EAX_PSYCHOTIC
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Listener_EAX_SetProperties(int Version,
	squall_eax_listener_t* Properties)
#else
DLL_API int SQUALL_Listener_EAX_SetProperties(int Version,
	squall_eax_listener_t* Properties)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Listener_EAX_SetProperties(%d, 0x%X)",
		Version,
		Properties);
#endif

	// проверка наличия EAX
	if (IsTrue(result))
		if (!context->_useEax)
			result = SQUALL_ERROR_NO_EAX;

	// проверка параметров
	if (IsTrue(result))
		if ((Version <= 0) ||
			(Version > EAX_NUM) ||
			IsBadReadPtr(Properties,
				sizeof(squall_eax_listener_t))) {
			Properties = 0;
			result = SQUALL_ERROR_INVALID_PARAM;
		}

	// отладочная информация
#if SQUALL_DEBUG
	if (Properties) {
		// в зависимости от версии различные параметры
		switch (Version) {
			// EAX 1.0
		case 1:
			api_Log("\t\t[in]Properties->Environment       = %d",
				Properties->eax1.Environment);
			api_Log("\t\t[in]Properties->Volume            = %f",
				Properties->eax1.Volume);
			api_Log("\t\t[in]Properties->DecayTime_sec     = %f",
				Properties->eax1.DecayTime_sec);
			api_Log("\t\t[in]Properties->Damping           = %f",
				Properties->eax1.Damping);
			break;

			// EAX 2.0
		case 2:
			api_Log("\t\t[in]Properties->Room              = %d",
				Properties->eax2.Room);
			api_Log("\t\t[in]Properties->RoomHF            = %d",
				Properties->eax2.RoomHF);
			api_Log("\t\t[in]Properties->RoomRolloffFactor = %f",
				Properties->eax2.RoomRolloffFactor);
			api_Log("\t\t[in]Properties->DecayTime         = %f",
				Properties->eax2.DecayTime);
			api_Log("\t\t[in]Properties->DecayHFRatio      = %f",
				Properties->eax2.DecayHFRatio);
			api_Log("\t\t[in]Properties->Reflections       = %d",
				Properties->eax2.Reflections);
			api_Log("\t\t[in]Properties->ReflectionsDelay  = %f",
				Properties->eax2.ReflectionsDelay);
			api_Log("\t\t[in]Properties->Reverb            = %d",
				Properties->eax2.Reverb);
			api_Log("\t\t[in]Properties->ReverbDelay       = %f",
				Properties->eax2.ReverbDelay);
			api_Log("\t\t[in]Properties->Environment       = %d",
				Properties->eax2.Environment);
			api_Log("\t\t[in]Properties->EnvSize           = %f",
				Properties->eax2.EnvironmentSize);
			api_Log("\t\t[in]Properties->EnvDiffusion      = %f",
				Properties->eax2.EnvironmentDiffusion);
			api_Log("\t\t[in]Properties->AirAbsorptionHF   = %f",
				Properties->eax2.AirAbsorptionHF);
			api_Log("\t\t[in]Properties->Flags             = %d",
				Properties->eax2.Flags);
			break;

			// EAX 3.0
		case 3:
			api_Log("\t\t[in]Properties->Environment          = %d",
				Properties->eax3.Environment);
			api_Log("\t\t[in]Properties->EnvironmentSize      = %f",
				Properties->eax3.EnvironmentSize);
			api_Log("\t\t[in]Properties->EnvironmentDiffusion = %f",
				Properties->eax3.EnvironmentDiffusion);
			api_Log("\t\t[in]Properties->Room                 = %d",
				Properties->eax3.Room);
			api_Log("\t\t[in]Properties->RoomHF               = %d",
				Properties->eax3.RoomHF);
			api_Log("\t\t[in]Properties->RoomLF               = %d",
				Properties->eax3.RoomLF);
			api_Log("\t\t[in]Properties->DecayTime            = %f",
				Properties->eax3.DecayTime);
			api_Log("\t\t[in]Properties->DecayHFRatio         = %f",
				Properties->eax3.DecayHFRatio);
			api_Log("\t\t[in]Properties->DecayLFRatio         = %f",
				Properties->eax3.DecayLFRatio);
			api_Log("\t\t[in]Properties->Reflections          = %d",
				Properties->eax3.Reflections);
			api_Log("\t\t[in]Properties->ReflectionsDelay     = %f",
				Properties->eax3.ReflectionsDelay);
			api_Log("\t\t[in]Properties->ReflectionsPan[x]    = %f",
				Properties->eax3.ReflectionsPan[0]);
			api_Log("\t\t[in]Properties->ReflectionsPan[y]    = %f",
				Properties->eax3.ReflectionsPan[1]);
			api_Log("\t\t[in]Properties->ReflectionsPan[z]    = %f",
				Properties->eax3.ReflectionsPan[2]);
			api_Log("\t\t[in]Properties->Reverb               = %d",
				Properties->eax3.Reverb);
			api_Log("\t\t[in]Properties->ReverbDelay          = %f",
				Properties->eax3.ReverbDelay);
			api_Log("\t\t[in]Properties->ReverbPan[x]         = %f",
				Properties->eax3.ReverbPan[0]);
			api_Log("\t\t[in]Properties->ReverbPan[y]         = %f",
				Properties->eax3.ReverbPan[1]);
			api_Log("\t\t[in]Properties->ReverbPan[z]         = %f",
				Properties->eax3.ReverbPan[2]);
			api_Log("\t\t[in]Properties->EchoTime             = %f",
				Properties->eax3.EchoTime);
			api_Log("\t\t[in]Properties->EchoDepth            = %f",
				Properties->eax3.EchoDepth);
			api_Log("\t\t[in]Properties->ModulationTime       = %f",
				Properties->eax3.ModulationTime);
			api_Log("\t\t[in]Properties->ModulationDepth      = %f",
				Properties->eax3.ModulationDepth);
			api_Log("\t\t[in]Properties->AirAbsorptionHF      = %f",
				Properties->eax3.AirAbsorptionHF);
			api_Log("\t\t[in]Properties->HFReference          = %f",
				Properties->eax3.HFReference);
			api_Log("\t\t[in]Properties->LFReference          = %f",
				Properties->eax3.LFReference);
			api_Log("\t\t[in]Properties->RoomRolloffFactor    = %f",
				Properties->eax3.RoomRolloffFactor);
			api_Log("\t\t[in]Properties->Flags                = %d",
				Properties->eax3.Flags);
			break;
		}
	}
#endif

	// проверка доступности интерфейса
	if (IsTrue(result)) {
		// скопируем параметры
		memcpy(&context->_eaxLP, Properties, sizeof(squall_eax_listener_t));
		if (context->_eaxSupport[Version])
			result = SQUALL_ERROR_EAX_VERSION;
	}

	// установка параметров
	if (IsTrue(result))
		if (!eax_Set(context->_eaxListener,
			 	EAX_LISTENER,
			 	Version,
			 	& context->_eaxLP))
			result = SQUALL_ERROR_SET_EAX_PARAM;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Установка новых EAX параметров слушателя
// на входе    :  Version     -  номер версии EAX интерфейса
//  			  Properties  -  указатель на структуру с новыми EAX параметрами
//  							 слушателя
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Listener_EAX_GetProperties(int Version,
	squall_eax_listener_t* Properties)
#else
DLL_API int SQUALL_Listener_EAX_GetProperties(int Version,
	squall_eax_listener_t* Properties)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Listener_EAX_GetProperties(%d, 0x%X)",
		Version,
		Properties);
#endif

	// проверка наличия EAX
	if (IsTrue(result))
		if (!context->_useEax)
			result = SQUALL_ERROR_NO_EAX;

	// проверка параметров
	if (IsTrue(result))
		if ((Version <= 0) ||
			(Version > EAX_NUM) ||
			IsBadWritePtr(Properties,
				sizeof(squall_eax_listener_t))) {
			Properties = 0;
			result = SQUALL_ERROR_INVALID_PARAM;
		}


	// проверка версии
	if (IsTrue(result))
		if (context->_eaxSupport[Version])
			result = SQUALL_ERROR_EAX_VERSION;

	// получение параметров
	if (IsTrue(result))
		if (eax_Get(context->_eaxListener,
				EAX_LISTENER,
				Version,
				& context->_eaxLP))
			memcpy(Properties,
				& context->_eaxLP,
				sizeof(squall_eax_listener_t));
		else
			result = SQUALL_ERROR_GET_EAX_PARAM;

		// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else {
		if (Properties) {
			switch (Version) {
				// EAX 1.0
			case 1:
				api_Log("\t\t[out]Properties->Environment       = %d",
					Properties->eax1.Environment);
				api_Log("\t\t[out]Properties->Volume            = %f",
					Properties->eax1.Volume);
				api_Log("\t\t[out]Properties->DecayTime_sec     = %f",
					Properties->eax1.DecayTime_sec);
				api_Log("\t\t[out]Properties->Damping           = %f",
					Properties->eax1.Damping);
				break;
				// EAX 2.0
			case 2:
				api_Log("\t\t[out]Properties->Room              = %d",
					Properties->eax2.Room);
				api_Log("\t\t[out]Properties->RoomHF            = %d",
					Properties->eax2.RoomHF);
				api_Log("\t\t[out]Properties->RoomRolloffFactor = %f",
					Properties->eax2.RoomRolloffFactor);
				api_Log("\t\t[out]Properties->DecayTime         = %f",
					Properties->eax2.DecayTime);
				api_Log("\t\t[out]Properties->DecayHFRatio      = %f",
					Properties->eax2.DecayHFRatio);
				api_Log("\t\t[out]Properties->Reflections       = %d",
					Properties->eax2.Reflections);
				api_Log("\t\t[out]Properties->ReflectionsDelay  = %f",
					Properties->eax2.ReflectionsDelay);
				api_Log("\t\t[out]Properties->Reverb            = %d",
					Properties->eax2.Reverb);
				api_Log("\t\t[out]Properties->ReverbDelay       = %f",
					Properties->eax2.ReverbDelay);
				api_Log("\t\t[out]Properties->Environment       = %d",
					Properties->eax2.Environment);
				api_Log("\t\t[out]Properties->EnvSize           = %f",
					Properties->eax2.EnvironmentSize);
				api_Log("\t\t[out]Properties->EnvDiffusion      = %f",
					Properties->eax2.EnvironmentDiffusion);
				api_Log("\t\t[out]Properties->AirAbsorptionHF   = %f",
					Properties->eax2.AirAbsorptionHF);
				api_Log("\t\t[out]Properties->Flags             = %d",
					Properties->eax2.Flags);
				break;
				// EAX 3.0
			case 3:
				api_Log("\t\t[out]Properties->Environment          = %d",
					Properties->eax3.Environment);
				api_Log("\t\t[out]Properties->EnvironmentSize      = %f",
					Properties->eax3.EnvironmentSize);
				api_Log("\t\t[out]Properties->EnvironmentDiffusion = %f",
					Properties->eax3.EnvironmentDiffusion);
				api_Log("\t\t[out]Properties->Room                 = %d",
					Properties->eax3.Room);
				api_Log("\t\t[out]Properties->RoomHF               = %d",
					Properties->eax3.RoomHF);
				api_Log("\t\t[out]Properties->RoomLF               = %d",
					Properties->eax3.RoomLF);
				api_Log("\t\t[out]Properties->DecayTime            = %f",
					Properties->eax3.DecayTime);
				api_Log("\t\t[out]Properties->DecayHFRatio         = %f",
					Properties->eax3.DecayHFRatio);
				api_Log("\t\t[out]Properties->DecayLFRatio         = %f",
					Properties->eax3.DecayLFRatio);
				api_Log("\t\t[out]Properties->Reflections          = %d",
					Properties->eax3.Reflections);
				api_Log("\t\t[out]Properties->ReflectionsDelay     = %f",
					Properties->eax3.ReflectionsDelay);
				api_Log("\t\t[out]Properties->ReflectionsPan[x]    = %f",
					Properties->eax3.ReflectionsPan[0]);
				api_Log("\t\t[out]Properties->ReflectionsPan[y]    = %f",
					Properties->eax3.ReflectionsPan[1]);
				api_Log("\t\t[out]Properties->ReflectionsPan[z]    = %f",
					Properties->eax3.ReflectionsPan[2]);
				api_Log("\t\t[out]Properties->Reverb               = %d",
					Properties->eax3.Reverb);
				api_Log("\t\t[out]Properties->ReverbDelay          = %f",
					Properties->eax3.ReverbDelay);
				api_Log("\t\t[out]Properties->ReverbPan[x]         = %f",
					Properties->eax3.ReverbPan[0]);
				api_Log("\t\t[out]Properties->ReverbPan[y]         = %f",
					Properties->eax3.ReverbPan[1]);
				api_Log("\t\t[out]Properties->ReverbPan[z]         = %f",
					Properties->eax3.ReverbPan[2]);
				api_Log("\t\t[out]Properties->EchoTime             = %f",
					Properties->eax3.EchoTime);
				api_Log("\t\t[out]Properties->EchoDepth            = %f",
					Properties->eax3.EchoDepth);
				api_Log("\t\t[out]Properties->ModulationTime       = %f",
					Properties->eax3.ModulationTime);
				api_Log("\t\t[out]Properties->ModulationDepth      = %f",
					Properties->eax3.ModulationDepth);
				api_Log("\t\t[out]Properties->AirAbsorptionHF      = %f",
					Properties->eax3.AirAbsorptionHF);
				api_Log("\t\t[out]Properties->HFReference          = %f",
					Properties->eax3.HFReference);
				api_Log("\t\t[out]Properties->LFReference          = %f",
					Properties->eax3.LFReference);
				api_Log("\t\t[out]Properties->RoomRolloffFactor    = %f",
					Properties->eax3.RoomRolloffFactor);
				api_Log("\t\t[out]Properties->Flags                = %d",
					Properties->eax3.Flags);
				break;
			}
		}
	}
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}
//-----------------------------------------------------------------------------
// Получение текущих EAX параметров слушателя
// на входе    :  Version     -  номер версии EAX интерфейса
//  			  Properties  -  указатель на структуру куда надо поместить
//  							 текущие EAX параметры слушателя
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Listener_EAX_SetPreset(int Preset)
#else
DLL_API int SQUALL_Listener_EAX_SetPreset(int Preset)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Listener_EAX_SetPreset(%d)", Preset);
#endif

	// проверка наличия EAX
	if (IsTrue(result))
		if (!context->_useEax)
			result = SQUALL_ERROR_NO_EAX;

	// преобразование номера окружения
	Preset++;

	// проверка параметров
	if (IsTrue(result))
		if ((Preset < 0) || (Preset > 26))
			result = SQUALL_ERROR_INVALID_PARAM;

	// установка окружения
	if (IsTrue(result))
		if (!eax_Preset(context->_eaxListener, context->_useEax, Preset))
			result = SQUALL_ERROR_SET_EAX_PARAM;

	// получение текущих параметров
	if (IsTrue(result))
		if (!eax_Get(context->_eaxListener,
			 	EAX_LISTENER,
			 	context->_useEax,
			 	& context->_eaxLP))
			result = SQUALL_ERROR_GET_EAX_PARAM;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Установка обработчика слушателя
// на входе    :  Worker	  -  указатель на обработчик слушателя, в случае
//  							 если параметр равен 0, предедущий обработчик
//  							 будет удален.
//  			  Param 	  -  указатель на данные пользователя, в случае
//  							 данных пользователя нет, то данный параметр
//  							 может содержать 0
//  			  UpdateTime  -  промежуток времени через который нужно
//  							 вызывать обработчик параметр должен лежать
//  							 в пределах от 1 до 5000
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Listener_SetWorker(squall_callback_listener_t Worker, void* Param,
	unsigned int UpdateTime)
#else
DLL_API int SQUALL_Listener_SetWorker(squall_callback_listener_t Worker,
	void* Param, unsigned int UpdateTime)
#endif
{
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Listener_SetWorker(0x%X, 0x%X, %d)",
		Worker,
		Param,
		UpdateTime);
#endif

	if (IsTrue(result)) {
		if (Worker == 0) {
			// сброс обработчика
			context->_worker = 0;
			context->_workerUserData = 0;
			context->_workerUpdateTime = 0;
		} else {
			// проверка параметров
			if (IsBadCodePtr((FARPROC) Worker) || (UpdateTime <= 0))
				result = SQUALL_ERROR_INVALID_PARAM;

			if (IsTrue(result)) {
				// занесем параметры обработчика
				context->_worker = Worker;
				context->_workerUserData = Param;
				context->_workerUpdateTime = UpdateTime;
			}
		}
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Процедура воспроизведения рассеяного звука, расширенная версия
// на входе :  SampleID 	- идентификатор загруженого звука
//  		   Loop 	- флаг бесконечного воспроизведения
//  					  true   - воспроизводить звук в цикле бесконечно
//  					  false  - воспроизвести звук один раз
//  		   Group	- принадлежность создаваемого канала к группе
//  		   Start	- флаг запуска звука по окончанию создания канала
//  					  true   - канал начнет проигрываться сразу же
//  					  false  - канал будет только подготовлен, для того
//  							чтобы начать воспроизведение нужно
//  							вызвать метод CHANNEL_Start()
//  		   Priority - приоритет создаваемого звукового канала,
//  					  значение должно лежать в пределах от 0 до 255
//  		   Volume      - громкость создаваемого звукового канала,
//  					  значение должно лежать в пределах от 0 до 100
//  		   Frequency   - частота создаваемого звукового канала
//  					  значение должно лежать в пределах от 100 до 100000
//  		   Pan  	   - панорама создаваемого звукового канала
//  					  значение должно лежать в пределах от 0 до 100
// на выходе   :  идентификатор звукового канала, если выходное значение равно
//  		   0, значит произошла ошибка, какая именно можно узнать
//  		   вызвав метод SYSTEM_GetError()
// примечание  :  1. Воспроизведение производиться с преобразованием звука в 
//  		   стерео режим. По сему лучше использовать заранее стерео
//  		   записи
//-----------------------------------------------------------------------------
int CreateChannel(context_t* context, int SampleID, int Loop,
	int ChannelGroupID, int Start, int Priority, int Volume, int Frequency,
	int Pan)
{
	// переменные
	int channel = - 1;
	int count = 0;
	CSoundFile* SoundData = 0;
	int result = true;

	// получение указателя на данные семпла
	SoundData = api_SampleGetFile(SampleID);
	if (!SoundData)
		result = SQUALL_ERROR_SAMPLE_BAD;

	// поиск свободного места под звук
	if (IsTrue(result)) {
		channel = context->GetFreeChannel(Priority);
		if (channel < 0)
			result = SQUALL_ERROR_NO_FREE_CHANNEL;
	}

	// заполним данными канал и создадим звуковой буфер
	if (IsTrue(result)) {
		// подготовим канал
		count = context->_channelsArray[channel]._count;
		memset(&context->_channelsArray[channel], 0, sizeof(SSample));
		context->_channelsArray[channel]._count = count;

		// заполнение данными звуковой структуры
		context->_channelsArray[channel].ChannelID = context->GetChannelID(channel);
		context->_channelsArray[channel].Status.SOUND_TYPE = TYPE_AMBIENT;
		context->_channelsArray[channel].GroupID = ChannelGroupID;
		context->_channelsArray[channel].Status.SAMPLE_LOOP = Loop;
		context->_channelsArray[channel].SoundFile = SoundData;
		context->_channelsArray[channel].SampleID = SampleID;
		context->_channelsArray[channel].EndPtr = SoundData->GetSamplesInFile();
		context->_channelsArray[channel].MasterSize = context->_channelsArray[channel].EndPtr;
		context->_channelsArray[channel].SamplesPerSec = SoundData->GetFormat()->nSamplesPerSec;
		context->_channelsArray[channel].Priority = Priority;
		context->_channelsArray[channel].Status.STAGE = PREPARE;

		// создадим звуковой буфер
		if (!context->_channelsArray[channel].CreateSoundBuffer(context->_directSound,
											  	SoundData->GetFormat(),
											  	SoundData->GetSamplesInFile(),
											  	context->_bufferSize,
											  	context->_useHW2D))
			result = SQUALL_ERROR_CREATE_CHANNEL;
	}

	// заполнение данными первой половины буфера
	if (IsTrue(result))
		if (!context->_channelsArray[channel].FillSoundBuffer(0))
			result = SQUALL_ERROR_SET_CHANNEL_PARAM;

	// установка уровня громкости канала
	if (IsTrue(result))
		if (!context->_channelsArray[channel].SetVolume(volume_table[Volume]))
			result = SQUALL_ERROR_SET_CHANNEL_PARAM;

	// установка частоты
	if (IsTrue(result))
		if (Frequency)
			if (!context->_channelsArray[channel].SetFrequency(Frequency))
				result = SQUALL_ERROR_SET_CHANNEL_PARAM;

	// установка панорамы
	if (IsTrue(result))
		if (!context->_channelsArray[channel].SetPan(Pan))
			result = SQUALL_ERROR_SET_CHANNEL_PARAM;

	// поставим на воспроизведение
	if (IsTrue(result))
		if (Start)
			if (!context->_channelsArray[channel].Play())
				result = SQUALL_ERROR_SET_CHANNEL_PARAM;

	// если была ошибка удалим все ненужное
	if (IsFalse(result)) {
		if (channel != -1) {
			context->_channelsArray[channel].DeleteSoundBuffer();
			context->_channelsArray[channel].Status.STAGE = EMPTY;
		}
	}

	// если ошибки не было вернем идентификатор канала
	if (IsTrue(result))
		result = context->_channelsArray[channel].ChannelID;

	// вернем результат работы
	return result;
}

//-----------------------------------------------------------------------------
// Процедура воспроизведения точечного звука, расширенная версия
// на входе :  SampleID 	- идентификатор загруженого звука
//  		   Loop 	- флаг бесконечного воспроизведения
//  					  true   - воспроизводить звук в цикле бесконечно
//  					  false  - воспроизвести звук один раз
//  		   Group	- принадлежность создаваемого канала к группе
//  		   Start	- флаг запуска звука по окончанию создания канала
//  					  true   - канал начнет проигрываться сразу же
//  					  false  - канал будет только подготовлен, для того
//  							чтобы начать воспроизведение нужно
//  							вызвать метод CHANNEL_Start()
//  		   Position - указатель на структуру координат звукового
//  					  источника
//  		   Velocity - указатель на вектор скорости источника звука,
//  					  если значение скорости не важно, то данный
//  					  параметр может быть равен 0
//  		   Priority - приоритет создаваемого звукового канала,
//  					  значение должно лежать в пределах от 0 до 255
//  		   Volume      - громкость создаваемого звукового канала,
//  					  значение должно лежать в пределах от 0 до 100
//  		   Frequency   - частота создаваемого звукового канала
//  					  значение должно лежать в пределах от 100 до 100000
//  		   MinDist     - минимальное растояние слышимости
//  		   MaxDist     - максимальное растояние слышимости
// на выходе   :  идентификатор звукового канала, если выходное значение равно
//  		   0, значит произошла ошибка, какая именно можно узнать
//  		   вызвав метод SYSTEM_GetError()
// примечание  :  1. Воспроизведение производиться с преобразованием звука в 
//  		   моно режим. По сему лучше использовать заранее моно записи
//-----------------------------------------------------------------------------
int CreateChannel3D(context_t* context, int SampleID, int Loop,
	int ChannelGroupID, int Start, float* Position, float* Velocity,
	int Priority, int Volume, int Frequency, float MinDist, float MaxDist)
{
	// переменные
	int channel = - 1;
	int count = 0;
	CSoundFile* SoundData = 0;
	int result = true;


	// вычисление растояния до источника звука
	float dx = context->_listenerParameters.vPosition.x - Position[0];
	float dy = context->_listenerParameters.vPosition.y - Position[1];
	float dz = context->_listenerParameters.vPosition.z - Position[2];

	float direct = sqrtf(dx * dx + dy * dy + dz * dz);

	// в случае если растояние до источника больше чем максимальное растояние установким громкость в 0
	if (direct > MaxDist)
		return SQUALL_ERROR_CREATE_CHANNEL;

	// получение указателя на данные семпла
	SoundData = api_SampleGetFile(SampleID);
	if (!SoundData)
		result = SQUALL_ERROR_SAMPLE_BAD;

	// поиск свободного места под звук
	if (IsTrue(result)) {
		channel = context->GetFreeChannel(Priority);
		if (channel < 0)
			result = SQUALL_ERROR_NO_FREE_CHANNEL;
	}

	// заполним данными канал и создадим буфер
	if (IsTrue(result)) {
		// подготовим канал
		count = context->_channelsArray[channel]._count;
		memset(&context->_channelsArray[channel], 0, sizeof(SSample));
		context->_channelsArray[channel]._count = count;

		// заполнение данными звуковой структуры
		context->_channelsArray[channel].ChannelID = context->GetChannelID(channel);
		context->_channelsArray[channel].Status.SOUND_TYPE = TYPE_POSITIONED;
		context->_channelsArray[channel].GroupID = ChannelGroupID;
		context->_channelsArray[channel].Status.SAMPLE_LOOP = Loop;
		context->_channelsArray[channel].SoundFile = SoundData;
		context->_channelsArray[channel].SampleID = SampleID;
		context->_channelsArray[channel].EndPtr = SoundData->GetSamplesInFile();
		context->_channelsArray[channel].MasterSize = context->_channelsArray[channel].EndPtr;
		context->_channelsArray[channel].SamplesPerSec = SoundData->GetFormat()->nSamplesPerSec;
		context->_channelsArray[channel].Priority = Priority;
		context->_channelsArray[channel].Status.STAGE = PREPARE;

		// создадим звуковой буфер
		if (!context->_channelsArray[channel].Create3DSoundBuffer(context->_directSound,
											  	SoundData->GetFormat(),
											  	SoundData->GetSamplesInFile(),
											  	context->_bufferSize,
											  	context->_used3DAlgorithm,
											  	context->_useEax,
											  	context->_useHW3D))
			result = SQUALL_ERROR_CREATE_CHANNEL;
	}

	// установим трехмерные параметры
	if (IsTrue(result))
		if (!context->_channelsArray[channel].Set3DParameters((D3DVECTOR *) Position,
										  	(D3DVECTOR *)Velocity,
										  	0,
										  	0,
										  	0,
										  	0,
										  	MinDist,
										  	MaxDist,
										  	context->_deferred))
			result = SQUALL_ERROR_SET_CHANNEL_PARAM;

	// заполнение данными первой половины буфера
	if (IsTrue(result))
		if (!context->_channelsArray[channel].FillSoundBuffer(0))
			result = SQUALL_ERROR_SET_CHANNEL_PARAM;

	// установка уровня громкости канала
	if (IsTrue(result))
		if (!context->_channelsArray[channel].SetVolume(volume_table[Volume]))
			result = SQUALL_ERROR_SET_CHANNEL_PARAM;

	// установка частоты
	if (IsTrue(result))
		if (Frequency)
			if (!context->_channelsArray[channel].SetFrequency(Frequency))
				result = SQUALL_ERROR_SET_CHANNEL_PARAM;

	// поставим на воспроизведение
	if (IsTrue(result))
		if (Start)
			if (!context->_channelsArray[channel].Play())
				result = SQUALL_ERROR_SET_CHANNEL_PARAM;

	// если была ошибка удалим все ненужное
	if (IsFalse(result)) {
		if (channel != -1) {
			context->_channelsArray[channel].DeleteSoundBuffer();
			context->_channelsArray[channel].Status.STAGE = EMPTY;
		}
	}

	// если ошибки не было вернем идентификатор канала
	if (IsTrue(result))
		result = context->_channelsArray[channel].ChannelID;

	// вернем результат работы
	return result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//  				 Общие методы для работы с каналами
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
// Начало воспроизведения подготовленного звукового канала
// на входе    :  ChannelID   -  идентификатор подготовленного канала
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_Start(int ChannelID)
#else
DLL_API int SQUALL_Channel_Start(int ChannelID)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_Start(0x%X)", ChannelID);
#endif

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// запуск воспроизведения
	if (IsTrue(result))
		if (context->_channelsArray[channel].Status.STAGE == PREPARE)
			if (!context->_channelsArray[channel].Play())
				result = SQUALL_ERROR_SET_CHANNEL_PARAM;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Включение/выключение паузы звукового канала
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  Pause 	  -  флаг включения/выключения паузы, параметр может
//  							 принимать следующие значения:
//  							 true  - включить паузу
//  							 false - выключить паузу
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_Pause(int ChannelID, int PauseFlag)
#else
DLL_API int SQUALL_Channel_Pause(int ChannelID, int PauseFlag)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_Pause(0x%X, %d)", ChannelID, PauseFlag);
#endif

	// поиск канала по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// поставим на паузу канал
	if (IsTrue(result))
		if (!context->_channelsArray[channel].Pause(PauseFlag))
			result = SQUALL_ERROR_SET_CHANNEL_PARAM;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Остановка звукового канала по идентификатору
// на входе    :  ChannelID   -  идентификатор звукового канала
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_Stop(int ChannelID)
#else
DLL_API int SQUALL_Channel_Stop(int ChannelID)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_Stop(0x%X)", ChannelID);
#endif

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// поиск звука по идентификатору
	if (IsTrue(result))
		if (!context->_channelsArray[channel].Stop())
			result = SQUALL_ERROR_SET_CHANNEL_PARAM;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение статуса звукового канала
// на входе    :  ChannelID   -  идентификатор звукового канала
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//  			  в случае успешного вызова результат содержит статус канала,
//  			  результат может принимать следующие значения:
//  			  SQUALL_CHANNEL_STATUS_NONE	   -  звукового канала с таким
//  												  идентификатором нет
//  			  SQUALL_CHANNEL_STATUS_PLAY	   -  звуковой канал
//  												  воспроизводится
//  			  SQUALL_CHANNEL_STATUS_PAUSE      -  звуковой канал находится
//  												  в режиме паузы
//  			  SQUALL_CHANNEL_STATUS_PREPARED   -  звуковой канал
//  												  подготовлен
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_Status(int ChannelID)
#else
DLL_API int SQUALL_Channel_Status(int ChannelID)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_Status(0x%X)", ChannelID);
#endif

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_CHANNEL_STATUS_NONE;
		else if (context->_channelsArray[channel].Status.STAGE == PLAY)
			result = SQUALL_CHANNEL_STATUS_PLAY;
		else if (context->_channelsArray[channel].Status.STAGE == PAUSE)
			result = SQUALL_CHANNEL_STATUS_PAUSE;
		else if (context->_channelsArray[channel].Status.STAGE == PREPARE)
			result = SQUALL_CHANNEL_STATUS_PREPARED;
		else
			result = SQUALL_ERROR_UNKNOWN;
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = %d", result);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Установка нового уровня громкости звукового канала в процентах
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  Volume	  -  значение уровня громкости в провентах,
//  							 значение ппараметра должно быть в пределах
//  							 от 0 до 100
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_SetVolume(int ChannelID, int Volume)
#else
DLL_API int SQUALL_Channel_SetVolume(int ChannelID, int Volume)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_SetVolume(0x%X, %d)", ChannelID, Volume);
#endif

	// проверка параметров
	if (IsTrue(result))
		if ((Volume < 0) || (Volume > 100))
			result = SQUALL_ERROR_INVALID_PARAM;

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// установим новую громкость
	if (IsTrue(result))
		if (!context->_channelsArray[channel].SetVolume(volume_table[Volume]))
			result = SQUALL_ERROR_SET_CHANNEL_PARAM;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение текущего уровня громкости звукового канала в процентах
// на входе    :  ChannelID   -  идентификатор канала
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//  			  в случае успешного вызова результат содержит текущее значение
//  			  громкости канала в процентах
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_GetVolume(int ChannelID)
#else
DLL_API int SQUALL_Channel_GetVolume(int ChannelID)
#endif
{
	// переменные
	long DxVolume = 0;
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

#if SQUALL_DEBUG
	// отладочная информация
	api_LogTime("SQUALL_Channel_GetVolume(0x%X)", ChannelID);
#endif

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// получение громкости   
	if (IsTrue(result))
		if (!context->_channelsArray[channel].GetVolume(&DxVolume))
			result = SQUALL_ERROR_GET_CHANNEL_PARAM;
		else
			result = dxvolume_to_squallvolume(DxVolume);

		// отладочная информация
#if SQUALL_DEBUG 
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = %d", result);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}
//-----------------------------------------------------------------------------
// Устанока новой частоты дискретизации звукового канала
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  Frequency   -  новое значение частоты дискретизации, значение
//  							 параметра должно быть в пределах от 100 Герц
//  							 до 1000000 Герц
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_SetFrequency(int ChannelID, int Frequency)
#else
DLL_API int SQUALL_Channel_SetFrequency(int ChannelID, int Frequency)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_SetFrequency(0x%X, %d)", ChannelID, Frequency);
#endif

	// проверка параметров
	if (IsTrue(result))
		if ((Frequency < 100) || (Frequency > 100000))
			result = SQUALL_ERROR_INVALID_PARAM;

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// установим новую частоту
	if (IsTrue(result))
		if (!context->_channelsArray[channel].SetFrequency(Frequency))
			result = SQUALL_ERROR_SET_CHANNEL_PARAM;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение текущей частоты дискретизации звукового канала
// на входе    :  ChannelID   -  идентификатор звукового канала
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//  			  в случае успешного вызова результат содержит текущее значение
//  			  частоты дискретизации звукового канала
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_GetFrequency(int ChannelID)
#else
DLL_API int SQUALL_Channel_GetFrequency(int ChannelID)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_GetFrequency(0x%X)", ChannelID);
#endif

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// получим частоту дискретизации
	if (IsTrue(result))
		if (!context->_channelsArray[channel].GetFrequency(&result))
			result = SQUALL_ERROR_GET_CHANNEL_PARAM;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = %d", result);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Установка новой позиции воспроизведения звукового канала в семплах
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  Position    -  новое значение позиции воспроизведения
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_SetPlayPosition(int ChannelID, int PlayPosition)
#else
DLL_API int SQUALL_Channel_SetPlayPosition(int ChannelID, int PlayPosition)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_SetPlayPosition(0x%X, %d)",
		ChannelID,
		PlayPosition);
#endif

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// установим новые координаты звука в пространстве
	if (IsTrue(result))
		if (!context->_channelsArray[channel].SetPlayPosition(PlayPosition))
			result = SQUALL_ERROR_SET_CHANNEL_PARAM;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}
//-----------------------------------------------------------------------------
// Получение текущей позиции воспроизведения звукового канала в семплах
// на входе    :  ChannelID   -  идентификатор звукового канала
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//  			  в случае успешного вызова результат содержит текущую позицию
//  			  воспроизведения
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_GetPlayPosition(int ChannelID)
#else
DLL_API int SQUALL_Channel_GetPlayPosition(int ChannelID)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_GetPlayPosition(0x%X)", ChannelID);
#endif

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// получение текущей позиции воспроизведения
	if (IsTrue(result))
		if (!context->_channelsArray[channel].GetPlayPosition(&result))
			result = SQUALL_ERROR_SET_CHANNEL_PARAM;

#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = %d", result);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Установка новой позиции воспроизведения звукового канала в миллисекундах
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  Position    -  новое значение позиции воспроизведения,
//  							 в миллисекундах
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_SetPlayPositionMs(int ChannelID, int PlayPosition)
#else
DLL_API int SQUALL_Channel_SetPlayPositionMs(int ChannelID, int PlayPosition)
#endif
{
	// переменные
	int position = 0;   
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_SetPlayPositionMs(0x%X, %d)",
		ChannelID,
		PlayPosition);
#endif

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// установка новой позиции воспроизведения
	if (IsTrue(result)) {
		// сделаем преобразование времени в семплы
		position = (int)
			(((__int64)
			PlayPosition * (__int64)
			context->_channelsArray[channel].SamplesPerSec) /
			(__int64)
			1000);
		if (context->_channelsArray[channel].MasterSize < position)
			result = SQUALL_ERROR_INVALID_PARAM;
	}
	// установим новую позицию воспроизведения
	if (IsTrue(result))
		if (context->_channelsArray[channel].SetPlayPosition(position))
			result = SQUALL_ERROR_SET_CHANNEL_PARAM;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение текущей позиции воспроизведения звукового канала в миллисекундах
// на входе    :  ChannelID   -  идентификатор звукового канала
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//  			  в случае успешного вызова результат содержит текущую позицию
//  			  воспроизведения
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_GetPlayPositionMs(int ChannelID)
#else
DLL_API int SQUALL_Channel_GetPlayPositionMs(int ChannelID)
#endif
{
	// переменные
	int position = 0;   
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_GetPlayPositionMs(0x%X)", ChannelID);
#endif

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// получение текущей позиции воспроизведения
	if (IsTrue(result))
		if (!context->_channelsArray[channel].GetPlayPosition(&position))
			result = SQUALL_ERROR_GET_CHANNEL_PARAM;
		else
			result = (int)
				((__int64) (position * (__int64) 1000) /
				(__int64) context->_channelsArray[channel].SamplesPerSec);

		// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = %d", result);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Установка обработчика звукового канала
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  Worker	  -  указатель на обработчик звукового канала
//  			  Param 	  -  указатель на данные пользователя, в случае
//  							 если данных пользователя нет, параметр может
//  							 содержать 0
//  			  UpdateTime  -  промежуток времени в миллисекундах через
//  							 который нужно вызывать обработчик, значение
//  							 параметра должно быть в пределах от 1 до 5000
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_SetWorker(int ChannelID, squall_callback_channel_t Worker,
	void* Param, unsigned int UpdateTime)
#else
DLL_API int SQUALL_Channel_SetWorker(int ChannelID,
	squall_callback_channel_t Worker, void* Param, unsigned int UpdateTime)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_SetWorker(0x%X, %0x%X)", ChannelID, Worker);
#endif

	// поиск канала по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// установка параметров
	if (IsTrue(result))
		if (!Worker) {
			// удаление обработчика
			context->_channelsArray[channel].ChannelWorker = 0;
			context->_channelsArray[channel].UserData = 0;
			context->_channelsArray[channel].ChannelWorkerTime = 0;
		} else {
			if (IsBadCodePtr((FARPROC) Worker) || (UpdateTime <= 0))
				result = SQUALL_ERROR_INVALID_PARAM;
			else {
				// установка обработчика
				context->_channelsArray[channel].ChannelWorker = Worker;
				context->_channelsArray[channel].UserData = Param;
				context->_channelsArray[channel].ChannelWorkerTime = UpdateTime;
			}
		}

		// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Установка обработчика окончания звукового канала
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  Worker	  -  указатель на обработчик звукового канала
//  			  Param 	  -  указатель на данные пользователя, в случае
//  							 если данных пользователя нет, параметр может
//  							 содержать 0
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_SetEndWorker(int ChannelID,
	squall_callback_end_channel_t Worker, void* Param)
#else
DLL_API int SQUALL_Channel_SetEndWorker(int ChannelID,
	squall_callback_end_channel_t Worker, void* Param)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_SetEndWorker(0x%X, %0x%X)", ChannelID, Worker);
#endif

	// поиск канала по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// установка параметров
	if (IsTrue(result))
		if (!Worker) {
			// удаление обработчика
			context->_channelsArray[channel].ChannelEndWorker = 0;
			context->_channelsArray[channel].ChannelEndUserData = 0;
		} else {
			if (IsBadCodePtr((FARPROC) Worker))
				result = SQUALL_ERROR_INVALID_PARAM;
			else {
				// установка обработчика
				context->_channelsArray[channel].ChannelEndWorker = Worker;
				context->_channelsArray[channel].ChannelEndUserData = Param;
			}
		}

		// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Установка новых границ фрагмента звукового канала в семплах
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  Start 	  -  начальная позиция фрагмента, в отсчетах
//  			  End   	  -  конечная позиция фрагмента, в отсчетах
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_SetFragment(int ChannelID, int Start, int End)
#else
DLL_API int SQUALL_Channel_SetFragment(int ChannelID, int Start, int End)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_SetFragment(0x%X, %d, %d)",
		ChannelID,
		Start,
		End);
#endif

	// проверка параметров
	if (IsTrue(result))
		if ((Start < 0) || (End < 0) || (Start == End))
			result = SQUALL_ERROR_INVALID_PARAM;

	// поиск канала по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// установим границы
	if (IsTrue(result))
		if (!context->_channelsArray[channel].SetFragment(Start, End))
			result = SQUALL_ERROR_SET_CHANNEL_PARAM;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение текущих границ фрагмента звукового канала в семплах
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  Start 	  -  указатель на переменную в которую нужно
//  							 поместить начальную позицию фрагмента в
//  							 отсчетах
//  			  End   	  -  указатель на переменную в которую нужно
//  							 поместить конечную позицию фрагмента
//  							 в отсчетах
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_GetFragment(int ChannelID, int* Start, int* End)
#else
DLL_API int SQUALL_Channel_GetFragment(int ChannelID, int* Start, int* End)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_GetFragment(0x%X, 0x%X, 0x%X)",
		ChannelID,
		Start,
		End);
#endif

	// проверка параметров
	if (IsBadWritePtr(Start, sizeof(int)))
		Start = 0;
	if (IsBadWritePtr(End, sizeof(int)))
		End = 0;

	// проверка параметров
	if (IsTrue(result))
		if ((Start == 0) && (End == 0))
			result = SQUALL_ERROR_INVALID_PARAM;

	// поиск канала по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// получение границ
	if (IsTrue(result)) {
		if (Start)
			*Start = context->_channelsArray[channel].StartPtr;
		if (End)
			*End = context->_channelsArray[channel].EndPtr;
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else {
		if (Start)
			api_Log("\t\t[out]*Start = %d", * Start);
		if (End)
			api_Log("\t\t[out]*End = %d", * End);
	}
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Установка новых границ фрагмента звукового канала в миллисекундах
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  Start 	  -  начальная позиция фрагмента, позиция в
//  							 миллисекундах
//  			  End   	  -  конечная позиция фрагмента, позиция в
//  							 миллисекундах
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_SetFragmentMs(int ChannelID, int Start, int End)
#else
DLL_API int SQUALL_Channel_SetFragmentMs(int ChannelID, int Start, int End)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_SetFragmentMs(0x%X, %d, %d)",
		ChannelID,
		Start,
		End);
#endif

	// проверка параметров
	if (IsTrue(result))
		if ((Start < 0) || (End < 0) || (Start == End))
			result = SQUALL_ERROR_INVALID_PARAM;

	// поиск канала по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// установим границы
	if (IsTrue(result)) {
		// преобразование время в семплы
		int StartPtr = (int)
			((__int64)
			((__int64) Start * (__int64) context->_channelsArray[channel].SamplesPerSec) /
			(__int64)
			1000);
		int EndPtr = (int)
			(((__int64)
			End * (__int64)
			context->_channelsArray[channel].SamplesPerSec) /
			(__int64)
			1000);
		if (!context->_channelsArray[channel].SetFragment(StartPtr, EndPtr))
			result = SQUALL_ERROR_SET_CHANNEL_PARAM;
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение текущих границ фрагмента звукового канала в миллисекундах
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  Start 	  -  указатель на переменную в которую нужно
//  							 поместить начальную позицию фрагмента
//  							 в миллисекундах
//  			  End   	  -  указатель на переменную в которую нужно
//  							 поместить конечную позицию фрагмента
//  							 в миллисекундах
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_GetFragmentMs(int ChannelID, int* Start, int* End)
#else
DLL_API int SQUALL_Channel_GetFragmentMs(int ChannelID, int* Start, int* End)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_GetFragmentMs(0x%X, 0x%X, 0x%X)",
		ChannelID,
		Start,
		End);
#endif

	// проверка параметров
	if (IsBadWritePtr(Start, sizeof(int)))
		Start = 0;
	if (IsBadWritePtr(End, sizeof(int)))
		End = 0;

	// проверка параметров
	if (IsTrue(result))
		if ((Start == 0) && (End == 0))
			result = SQUALL_ERROR_INVALID_PARAM;

	// поиск канала по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// получение границ
	if (IsTrue(result)) {
		if (Start)
			*Start = (int)
				((__int64)
				(context->_channelsArray[channel].StartPtr * (__int64) 1000) /
				(__int64)
				context->_channelsArray[channel].SamplesPerSec);
		if (End)
			*End = (int)
				((__int64)
				(context->_channelsArray[channel].EndPtr * (__int64) 1000) /
				(__int64)
				context->_channelsArray[channel].SamplesPerSec);
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else {
		if (Start)
			api_Log("\t\t[out]*Start = %d", * Start);
		if (End)
			api_Log("\t\t[out]*End = %d", * End);
	}
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение продолжительности исходных звуковых данных в семплах
// на входе    :  ChannelID   -  идентификатор звукового канала
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//  			  в случае успешного вызова результат содержит продолжительность
//  			  исходных данных в семплах
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_GetLength(int ChannelID)
#else
DLL_API int SQUALL_Channel_GetLength(int ChannelID)
#endif
{
	// переменные
	int samples = 0;   
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_GetLength(0x%X)", ChannelID);
#endif

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// получение количества семплов
	if (IsTrue(result)) {
		samples = context->_channelsArray[channel].SoundFile->GetSamplesInFile();
		if (!samples)
			result = SQUALL_ERROR_SAMPLE_BAD;
		else
			result = samples;
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = %d", result);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение продолжительности исходных звуковых данных в миллисекундах
// на входе    :  ChannelID   -  идентификатор звукового канала
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//  			  в случае успешного вызова результат содержит продолжительность
//  			  исходных данных в милисекундах
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_GetLengthMs(int ChannelID)
#else
DLL_API int SQUALL_Channel_GetLengthMs(int ChannelID)
#endif
{
	// переменные
	int samples = 0;   
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_GetLengthMs(0x%X)", ChannelID);
#endif

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// получение количества семплов
	if (IsTrue(result)) {
		samples = context->_channelsArray[channel].SoundFile->GetSamplesInFile();
		if (!samples)
			result = SQUALL_ERROR_SAMPLE_BAD;
		else
			result = (int)
				((__int64)
				(samples * (__int64) 1000) /
				(__int64)
				context->_channelsArray[channel].SoundFile->GetFormat()->nSamplesPerSec);
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = %d", result);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Установка нового приоритета звукового канала
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  Priority    -  новый приоритет канала, значение параметра
//  							 должно быть в пределах от 0 (самый низший
//  							 приоритет) до 65535 (самый высший приоритет)
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_SetPriority(int ChannelID, int Priority)
#else
DLL_API int SQUALL_Channel_SetPriority(int ChannelID, int Priority)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_SetPriority(0x%X, %d)", ChannelID, Priority);
#endif

	// проверка параметров
	if (IsTrue(result))
		if (Priority < 0)
			result = SQUALL_ERROR_INVALID_PARAM;

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
		else
			context->_channelsArray[channel].Priority = Priority;
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение текущего приоритета звукового канала
// на входе    :  ChannelID   -  идентификатор звукового канала
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//  			  в случае успешного вызова результат содержит приоритет
//  			  звукового канала
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_GetPriority(int ChannelID)
#else
DLL_API int SQUALL_Channel_GetPriority(int ChannelID)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_GetPriority(0x%X)", ChannelID);
#endif

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
		else
			result = context->_channelsArray[channel].Priority;
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = %d", result);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение текущего значения флага зацикленности воспроизведения звукового
// канала
// на входе    :  ChannelID   -  идентификатор звукового канала
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//  			  в случае успешного вызова результат содержит текущее значение
//  			  флага зацикленности воспроизведения канала, результат может
//  			  принимать следующие значения:
//  			  true  -  звуковой канал воспроизводится бесконечно
//  			  false -  звуковой канал воспроизводится один раз
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_SetLoop(int ChannelID, int Loop)
#else
DLL_API int SQUALL_Channel_SetLoop(int ChannelID, int Loop)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_SetLoop(0x%X, %d)", ChannelID, Loop);
#endif

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
		else {
			context->_channelsArray[channel].Pause(true);
			context->_channelsArray[channel].Status.SAMPLE_LOOP = Loop;
			// если звук полностью в буфере включаем зацикливание буфера
			if (context->_channelsArray[channel].Status.DOUBLE_BUFFERING == 0)
				context->_channelsArray[channel].Status.BUFFER_LOOP = Loop;
			context->_channelsArray[channel].Pause(false);
		}
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Установка нового значения флага зацикленности воспроизведения звукового
// канала
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  Loop  	  -  флаг зацикленности канала, значение праметра
//  							 может принимать следующие значения:
//  							 true  -  бесконечное воспроизведение звукового
//  									  канала
//  							 false -  воспроизведение звукового канала один
//  									  раз
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_GetLoop(int ChannelID)
#else
DLL_API int SQUALL_Channel_GetLoop(int ChannelID)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_GetLoop(0x%X)", ChannelID);
#endif

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
		else
			result = context->_channelsArray[channel].Status.SAMPLE_LOOP;
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = %d", result);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//  			  Методы для работы с рассеянными каналами
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
// Установка новой панорамы звукового канала
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  Pan   	  -  новое значение панорамы, значение параметра
//  							 должено быть в пределах от 0 (максимальное
//  							 смещение стерео баланса влево) до 100
//  							 (максимальное смещение стерео баланса вправо)
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
// примечание  :  данный метод не работает с позиционными каналами
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_SetPan(int ChannelID, int Pan)
#else
DLL_API int SQUALL_Channel_SetPan(int ChannelID, int Pan)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_SetPan(0x%X, %d)", ChannelID, Pan);
#endif

	// проверка параметров
	if (IsTrue(result))
		if ((Pan < 0) || (Pan > 100))
			result = SQUALL_ERROR_INVALID_PARAM;

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// проверка возможности метода
	if (IsTrue(result))
		if (context->_channelsArray[channel].Status.SOUND_TYPE != TYPE_AMBIENT)
			result = SQUALL_ERROR_METHOD;

	// установим новую панораму
	if (IsTrue(result))
		if (!context->_channelsArray[channel].SetPan(Pan))
			result = SQUALL_ERROR_SET_CHANNEL_PARAM;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение текущей панорамы звукового канала
// на входе    :  ChannelID   -  идентификатор звукового канала
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//  			  в случае успешного вызова результат содержит текущее значение
//  			  панорамы канала
// примечание  :  данный метод не работает с позиционными каналами
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_GetPan(int ChannelID)
#else
DLL_API int SQUALL_Channel_GetPan(int ChannelID)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_GetPan(0x%X)", ChannelID);
#endif

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// проверка возможности метода
	if (IsTrue(result))
		if (context->_channelsArray[channel].Status.SOUND_TYPE != TYPE_AMBIENT)
			result = SQUALL_ERROR_METHOD;

	// получим текущую панораму
	if (IsTrue(result))
		if (!context->_channelsArray[channel].GetPan(&result))
			result = SQUALL_ERROR_SET_CHANNEL_PARAM;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = %d", result);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//  			  Методы для работы с позиционными каналами
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
// Установка новой трехмерной позиции звукового канала в пространстве
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  Position    -  указатель на структуру с новыми координатами
//  							 канала в пространстве
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
// примечание  :  данный метод не работает с рассеянными каналами
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_Set3DPosition(int ChannelID, float* Position)
#else
DLL_API int SQUALL_Channel_Set3DPosition(int ChannelID, float* Position)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_Set3DPosition(0x%X, 0x%X)",
		ChannelID,
		Position);
#endif

	// проверка параметров
	if (IsBadReadPtr(Position, sizeof(float) * 3))
		Position = 0;

	// отладочная информация
#if SQUALL_DEBUG
	// позиция звука
	if (Position) {
		api_Log("\t\t[in]Position->x = %f", Position[0]);
		api_Log("\t\t[in]Position->y = %f", Position[1]);
		api_Log("\t\t[in]Position->z = %f", Position[2]);
	}
#endif

	// проверка параметров
	if (IsTrue(result))
		if (!Position)
			result = SQUALL_ERROR_INVALID_PARAM;

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// проверка возможности метода
	if (IsTrue(result))
		if (context->_channelsArray[channel].Status.SOUND_TYPE !=
			TYPE_POSITIONED)
			result = SQUALL_ERROR_METHOD;

	// установим новые координаты звука в пространстве
	if (IsTrue(result))
		if (!context->_channelsArray[channel].Set3DParameters((D3DVECTOR *)
											  	Position,
											  	0,
											  	0,
											  	0,
											  	0,
											  	0,
											  	0,
											  	0,
											  	context->_deferred))
			result = SQUALL_ERROR_SET_CHANNEL_PARAM;

	// отладочная информация
#if SQUALL_DEBUG 
	if (IsFalse(result))
		api_Log("error = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение текущей трехмерной позиции звукового канала в пространстве
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  Position    -  указатель на структуру в которую нужно
//  							 поместить текущую трехмерную позицию звукового
//  							 канала в пространстве
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
// примечание  :  данный метод не работает с рассеянными каналами
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_Get3DPosition(int ChannelID, float* Position)
#else
DLL_API int SQUALL_Channel_Get3DPosition(int ChannelID, float* Position)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_Get3DPosition(0x%X, 0x%X)",
		ChannelID,
		Position);
#endif

	// проверка параметров
	if (IsTrue(result))
		if (IsBadWritePtr(Position, sizeof(float) * 3))
			result = SQUALL_ERROR_INVALID_PARAM;

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// проверка возможности метода
	if (IsTrue(result))
		if (context->_channelsArray[channel].Status.SOUND_TYPE !=
			TYPE_POSITIONED)
			result = SQUALL_ERROR_METHOD;

	// установим новые координаты звука в пространстве
	if (IsTrue(result))
		if (!context->_channelsArray[channel].Get3DParameters((D3DVECTOR *)
											  	Position,
											  	0,
											  	0,
											  	0,
											  	0,
											  	0,
											  	0,
											  	0))
			result = SQUALL_ERROR_SET_CHANNEL_PARAM;

	// отладочная информация
#if SQUALL_DEBUG 
	if (IsFalse(result))
		api_Log("error = %s", ErrorTable[-result]);
	else {
		api_Log("\t\t[in]Position->x = %f", Position[0]);
		api_Log("\t\t[in]Position->y = %f", Position[1]);
		api_Log("\t\t[in]Position->z = %f", Position[2]);
	}
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Установка новой скорости перемещения звукового канала
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  Velocity    -  указатель на структуру с новым вектором
//  							 скорости перемещения звукового канала
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
// примечание  :  данный метод не работает с рассеянными каналами
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_SetVelocity(int ChannelID, float* Velocity)
#else
DLL_API int SQUALL_Channel_SetVelocity(int ChannelID, float* Velocity)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_SetVelocity(0x%X, 0x%X)", ChannelID, Velocity);
#endif

	// проверка параметров
	if (IsBadReadPtr(Velocity, sizeof(float) * 3))
		Velocity = 0;

	// отладочная информация
#if SQUALL_DEBUG
	// скорость перемещения источника звука
	if (Velocity) {
		api_Log("\t\t[in]Velocity->x = %f", Velocity[0]);
		api_Log("\t\t[in]Velocity->y = %f", Velocity[1]);
		api_Log("\t\t[in]Velocity->z = %f", Velocity[2]);
	}
#endif

	// проверка параметров
	if (IsTrue(result))
		if (!Velocity)
			result = SQUALL_ERROR_INVALID_PARAM;

	// поиск канала по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// проверка возможности установки параметров
	if (IsTrue(result))
		if (context->_channelsArray[channel].Status.SOUND_TYPE !=
			TYPE_POSITIONED)
			result = SQUALL_ERROR_METHOD;

	// установка параметров
	if (IsTrue(result))
		if (!context->_channelsArray[channel].Set3DParameters(0,
											  	(D3DVECTOR *) Velocity,
											  	0,
											  	0,
											  	0,
											  	0,
											  	0,
											  	0,
											  	context->_deferred))
			result = SQUALL_ERROR_SET_CHANNEL_PARAM;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение текущей скорости перемещения звукового канала
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  Velocity    -  указатель на структуру в которую нужно
//  							 поместить текущее значение вектора скорости
//  							 звукового канала
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
// примечание  :  данный метод не работает с рассеянными каналами
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_GetVelocity(int ChannelID, float* Velocity)
#else
DLL_API int SQUALL_Channel_GetVelocity(int ChannelID, float* Velocity)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_GetVelocity(0x%X, 0x%X)", ChannelID, Velocity);
#endif

	// проверка параметров
	if (IsTrue(result))
		if (IsBadWritePtr(Velocity, sizeof(float) * 3))
			result = SQUALL_ERROR_INVALID_PARAM;

	// поиск канала по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// проверка возможности установки параметров
	if (IsTrue(result))
		if (context->_channelsArray[channel].Status.SOUND_TYPE !=
			TYPE_POSITIONED)
			result = SQUALL_ERROR_METHOD;

	// установка параметров
	if (IsTrue(result))
		if (!context->_channelsArray[channel].Get3DParameters(0,
											  	(D3DVECTOR *) Velocity,
											  	0,
											  	0,
											  	0,
											  	0,
											  	0,
											  	0))
			result = SQUALL_ERROR_GET_CHANNEL_PARAM;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else {
		api_Log("\t\t[out]Velocity->x = %f", Velocity[0]);
		api_Log("\t\t[out]Velocity->y = %f", Velocity[1]);
		api_Log("\t\t[out]Velocity->z = %f", Velocity[2]);
	}
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Установка нового минимального и максимального расстояния слышимости
// звукового канала
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  MinDist     -  новое минимальное расстояние слышимости
//  							 значение параметра должно быть в пределах
//  							 от 0.01f до 1000000000.0f
//  			  MaxDist     -  новое максимальное расстояние слышимости
//  							 значение параметра должно быть в пределах
//  							 от 0.01f до 1000000000.0f
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
// примечание  :  данный метод не работает с рассеянными каналами
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_SetMinMaxDistance(int ChannelID, float MinDist,
	float MaxDist)
#else
DLL_API int SQUALL_Channel_SetMinMaxDistance(int ChannelID, float MinDist,
	float MaxDist)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_SetMinMaxDistance(0x%X, %f, %f)",
		ChannelID,
		MinDist,
		MaxDist);
#endif

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
		else if (!context->_channelsArray[channel].Set3DParameters(0,
												   	0,
												   	0,
												   	0,
												   	0,
												   	0,
												   	MinDist,
												   	MaxDist,
												   	context->_deferred))
			result = SQUALL_ERROR_SET_CHANNEL_PARAM;
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение текущего минимального растояния слышимости звукового канала
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  MinDist     -  указатель на переменную в которую нужно
//  							 поместить текущее минимальное растояние
//  							 слышимости
//  			  MinDist     -  указатель на переменную в которую нужно
//  							 поместить текущее максимальное растояние
//  							 слышимости
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
// примечание  :  данный метод не работает с рассеянными каналами
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_GetMinMaxDistance(int ChannelID, float* MinDist,
	float* MaxDist)
#else
DLL_API int SQUALL_Channel_GetMinMaxDistance(int ChannelID, float* MinDist,
	float* MaxDist)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_GetMinMaxDistance(0x%X, %d, %d)",
		ChannelID,
		MinDist,
		MaxDist);
#endif

	// проверка адресов
	if (IsBadWritePtr(MinDist, sizeof(float)))
		MinDist = 0;
	if (IsBadWritePtr(MaxDist, sizeof(float)))
		MaxDist = 0;

	// проверка параметров
	if (IsTrue(result))
		if ((MinDist == 0) && (MaxDist == 0))
			result = SQUALL_ERROR_INVALID_PARAM;

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
		else {
			// минимальная дистанция
			if (MinDist)
				*MinDist = context->_channelsArray[channel].Param.ds3d.flMinDistance;
			// максимальная дистанция
			if (MaxDist)
				*MaxDist = context->_channelsArray[channel].Param.ds3d.flMaxDistance;
		}
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else {
		// вывод параметров
		if (MinDist)
			api_Log("\t\t*MinDist = %d", * MinDist);
		if (MaxDist)
			api_Log("\t\t*MaxDist = %d", * MinDist);
	}
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}
//-----------------------------------------------------------------------------
// Установка параметров конуса распространения звукового канала
// на входе    :  ChannelID 		-  идентификатор звукового канала
//  			  Orientation   	-  указатель на струкруру с вектором
//  								   направления внутреннего и внешнего конуса,
//  								   в случае если значение вектора направления
//  								   внешнего и внутреннего конуса изменять
//  								   не нужно, то данный параметр должен
//  								   содержать 0
//  			  InsideConeAngle   -  угол внутреннего звукового конуса, значение
//  								   параметра должно быть в пределах от 1 до
//  								   360 градусов, в случае если значение
//  								   угла внутреннего звукового конуса изненять
//  								   не нужно, то данный параметр должен
//  								   содержать 0
//  			  OutsideConeAngle  -  угол внешнего звукового конуса, значение
//  								   параметра должно быть в пределах от 1 до
//  								   360 градусов, в случае если значение
//  								   угла внешнего звукового конуса изменять
//  								   не нужно, то данный параметр должен
//  								   содержать 0
//  			  OutsideVolume 	-  уровень громкости источника за пределами
//  								   внешнего конуса, в процентах значение
//  								   праметра должно быть в пределах от 0
//  								   до 100
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
// примечание  :  данный метод не работает с рассеянными каналами
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_SetConeParameters(int ChannelID, float* Orientation,
	int InsideConeAngle, int OutsideConeAngle, int OutsideVolume)
#else
DLL_API int SQUALL_Channel_SetConeParameters(int ChannelID,
	float* Orientation, int InsideConeAngle, int OutsideConeAngle,
	int OutsideVolume)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_SetConeParameters(%X, %X, %d, %d, %d)",
		ChannelID,
		Orientation,
		InsideConeAngle,
		OutsideConeAngle,
		OutsideVolume);
#endif

	// проверка параметров
	if (IsBadWritePtr(Orientation, sizeof(float) * 3))
		Orientation = 0;

#if SQUALL_DEBUG
	if (Orientation) {
		api_Log("\t\t[in]Orientation->x = %f", Orientation[0]);
		api_Log("\t\t[in]Orientation->y = %f", Orientation[1]);
		api_Log("\t\t[in]Orientation->z = %f", Orientation[2]);
	}
#endif
	// проверка параметров
	if (IsTrue(result))
		if ((OutsideVolume < 0) ||
			(OutsideVolume > 100) ||
			(InsideConeAngle < 0) ||
			(InsideConeAngle > 360) ||
			(OutsideConeAngle < 0) ||
			(OutsideConeAngle > 360) ||
			(!Orientation))
			result = SQUALL_ERROR_INVALID_PARAM;

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
		else if (!context->_channelsArray[channel].Set3DParameters(0,
									0,
									(D3DVECTOR *)
									Orientation,
									InsideConeAngle,
									OutsideConeAngle,
									volume_table[OutsideVolume],
									0,
									0,
									context->_deferred))
			result = SQUALL_ERROR_SET_CHANNEL_PARAM;
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение параметров конуса распространения звукового канала
// на входе    :  ChannelID 		-  идентификатор звукового канала
//  			  Orientation   	-  указатель на струкруру в которую нужно
//  								   поместить текущий вектор направления
//  								   внешнего и внутреннего конуса, в случае
//  								   если значение вектора внутреннего
//  								   и внешнего конуса, получать не нужно,
//  								   то данный параметр должен содержать 0
//  			  InsideConeAngle   -  указатель на переменную в которую нужно
//  								   поместить текущее значение угола внутреннего
//  								   конуса в градусах, в случае если значение
//  								   угла внутреннего конуса получать не нужно,
//  								   то данный параметр должен содержать 0
//  			  OutsideConeAngle  -  указатель на переменную в которую нужно
//  								   поместить текущее значение угола внешнего
//  								   конуса в градусах, в случае если значение
//  								   угла внешнего конуса получать не нужно,
//  								   то данный параметр должен содержать 0
//  			  OutsideVolume 	-  указатель на переменную в которую нужно
//  								   поместить текущее значение уровеня громкости
//  								   источника за пределами внешнего конуса, в
//  								   процентах, в случае если значение уровня
//  								   громкости за пределами внешего конуса
//  								   получать не нужно, то данный параметр
//  								   должен содержать 0
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
// примечание  :  данный метод не работает с рассеянными каналами
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_GetConeParameters(int ChannelID, float* Orientation,
	int* InsideConeAngle, int* OutsideConeAngle, int* OutsideVolume)
#else
DLL_API int SQUALL_Channel_GetConeParameters(int ChannelID,
	float* Orientation, int* InsideConeAngle, int* OutsideConeAngle,
	int* OutsideVolume)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_GetConeParameters(0x%X, 0x%X, 0x%X, 0x%X, 0x%X)",
		ChannelID,
		Orientation,
		InsideConeAngle,
		OutsideConeAngle,
		OutsideVolume);
#endif

	// проверка параметров
	if (IsBadWritePtr(Orientation, sizeof(float) * 3))
		Orientation = 0;
	if (IsBadWritePtr(InsideConeAngle, sizeof(int)))
		InsideConeAngle = 0;
	if (IsBadWritePtr(OutsideConeAngle, sizeof(int)))
		OutsideConeAngle = 0;
	if (IsBadWritePtr(OutsideVolume, sizeof(int)))
		OutsideVolume = 0;

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		long ocv = 0;
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
		else 
		if (!context->_channelsArray[channel].Get3DParameters(0,
								   	0,
								   	(D3DVECTOR *) Orientation,
								   	InsideConeAngle,
								   	OutsideConeAngle,
								   	& ocv,
								   	0,
								   	0))
			result = SQUALL_ERROR_SET_CHANNEL_PARAM;
		else
			*OutsideVolume = dxvolume_to_squallvolume(ocv);
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else {
		if (Orientation) {
			api_Log("\t\t[out]Orientation->x    = %f", Orientation[0]);
			api_Log("\t\t[out]Orientation->y    = %f", Orientation[1]);
			api_Log("\t\t[out]Orientation->z    = %f", Orientation[2]);
		}
		if (InsideConeAngle)
			api_Log("\t\t[out]*InsideConeAngle  = %d", * InsideConeAngle);
		if (OutsideConeAngle)
			api_Log("\t\t[out]*OutsideConeAngle = %d", * OutsideConeAngle);
		if (OutsideVolume)
			api_Log("\t\t[out]*OutsideVolume    = %d", * OutsideVolume);
	}
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Установка новых EAX параметров звукового канала
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  Version     -  номер версии EAX, параметр определяет в каком
//  							 формате передаются EAX параметры канала.
//  			  Properties  -  указатель на структуру описывающую параметры
//  							 EAX канала, параметры должны быть в формате
//  							 указанном параметром Version
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
// примечание  :  данный метод не работает с рассеянными каналами
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_EAX_SetProperties(int ChannelID, int Version,
	squall_eax_channel_t* Properties)
#else
DLL_API int SQUALL_Channel_EAX_SetProperties(int ChannelID, int Version,
	squall_eax_channel_t* Properties)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_EAX_SetProperties(0x%X, %d, 0x%X)",
		ChannelID,
		Version,
		Properties);
#endif

	// проверка параметров
	if (IsBadReadPtr(Properties, sizeof(squall_eax_channel_t)))
		Properties = 0;

	// отладочная информация
#if SQUALL_DEBUG
	// выведем параметры
	if (Properties) {
		switch (Version) {
			// EAX 1.0
		case 1:
			api_Log("\t\t[in]Properties->Mix                 = %d",
				Properties->eax1.Mix);
			break;
			// EAX 2.0
		case 2:
			api_Log("\t\t[in]Properties->Direct              = %d",
				Properties->eax2.Direct);
			api_Log("\t\t[in]Properties->DirectHF            = %d",
				Properties->eax2.DirectHF);
			api_Log("\t\t[in]Properties->Room                = %d",
				Properties->eax2.Room);
			api_Log("\t\t[in]Properties->RoomHF              = %d",
				Properties->eax2.RoomHF);
			api_Log("\t\t[in]Properties->RoomRolloffFactor   = %f",
				Properties->eax2.RoomRolloffFactor);
			api_Log("\t\t[in]Properties->Obstruction         = %d",
				Properties->eax2.Obstruction);
			api_Log("\t\t[in]Properties->ObstructionLFRatio  = %f",
				Properties->eax2.ObstructionLFRatio);
			api_Log("\t\t[in]Properties->Occlusion           = %d",
				Properties->eax2.Occlusion);
			api_Log("\t\t[in]Properties->OcclusionLFRatio    = %f",
				Properties->eax2.OcclusionLFRatio);
			api_Log("\t\t[in]Properties->OcclusionRoomRatio  = %f",
				Properties->eax2.OcclusionRoomRatio);
			api_Log("\t\t[in]Properties->OutsideVolumeHF     = %d",
				Properties->eax2.OutsideVolumeHF);
			api_Log("\t\t[in]Properties->AirAbsorptionFactor = %f",
				Properties->eax2.AirAbsorptionFactor);
			api_Log("\t\t[in]Properties->Flags               = %d",
				Properties->eax2.Flags);
			break;
			// EAX 3.0
		case 3:
			api_Log("\t\t[in]Properties->Direct               = %d",
				Properties->eax3.Direct);
			api_Log("\t\t[in]Properties->DirectHF             = %d",
				Properties->eax3.DirectHF);
			api_Log("\t\t[in]Properties->Room                 = %d",
				Properties->eax3.Room);
			api_Log("\t\t[in]Properties->RoomHF               = %d",
				Properties->eax3.RoomHF);
			api_Log("\t\t[in]Properties->Obstruction          = %d",
				Properties->eax3.Obstruction);
			api_Log("\t\t[in]Properties->ObstructionLFRatio   = %f",
				Properties->eax3.ObstructionLFRatio);
			api_Log("\t\t[in]Properties->Occlusion            = %d",
				Properties->eax3.Occlusion);
			api_Log("\t\t[in]Properties->OcclusionLFRatio     = %f",
				Properties->eax3.OcclusionLFRatio);
			api_Log("\t\t[in]Properties->OcclusionRoomRatio   = %f",
				Properties->eax3.OcclusionRoomRatio);
			api_Log("\t\t[in]Properties->OcclusionDirectRatio = %f",
				Properties->eax3.OcclusionDirectRatio);
			api_Log("\t\t[in]Properties->Exclusion            = %d",
				Properties->eax3.Exclusion);
			api_Log("\t\t[in]Properties->ExclusionLFRatio     = %f",
				Properties->eax3.ExclusionLFRatio);
			api_Log("\t\t[in]Properties->OutsideVolumeHF      = %d",
				Properties->eax3.OutsideVolumeHF);
			api_Log("\t\t[in]Properties->DopplerFactor        = %f",
				Properties->eax3.DopplerFactor);
			api_Log("\t\t[in]Properties->RolloffFactor        = %f",
				Properties->eax3.RolloffFactor);
			api_Log("\t\t[in]Properties->RoomRolloffFactor    = %f",
				Properties->eax3.RoomRolloffFactor);
			api_Log("\t\t[in]Properties->AirAbsorptionFactor  = %f",
				Properties->eax3.AirAbsorptionFactor);
			api_Log("\t\t[in]Properties->Flags                = %d",
				Properties->eax3.Flags);
			break;
		}
	}
#endif

	// проверка параметров
	if (IsTrue(result))
		if ((!Properties) || (Version < 0) || (Version > EAX_NUM))
			result = SQUALL_ERROR_INVALID_PARAM;

	// проверим взможность работы
	if (IsTrue(result))
		if (!context->_eaxSupport[Version])
			result = SQUALL_ERROR_EAX_VERSION;

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// подготовим и запишем данные   
	if (IsTrue(result)) {
		memcpy(&context->_channelsArray[channel].Param.EAXBP,
			Properties,
			sizeof(squall_eax_channel_t));
		if (!eax_Set(context->_channelsArray[channel].DS3D_PropSet,
			 	EAX_BUFFER,
			 	Version,
			 	& context->_channelsArray[channel].Param.EAXBP))
			result = SQUALL_ERROR_SET_EAX_PARAM;
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение текущих EAX параметров звукового канала
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  Version     -  номер версии EAX, параметр определяет в каком
//  							 формате получать EAX параметры канала.
//  			  Properties  -  указатель на структуру куда нужно поместить
//  							 текущие параметры EAX канала, структура будет
//  							 заполнена параметрами в формате указанном
//  							 параметром Version
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
// примечание  :  данный метод не работает с рассеянными каналами
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_EAX_GetProperties(int ChannelID, int Version,
	squall_eax_channel_t* Properties)
#else
DLL_API int SQUALL_Channel_EAX_GetProperties(int ChannelID, int Version,
	squall_eax_channel_t* Properties)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_EAX_GetProperties(0x%X, %d, 0x%X)",
		ChannelID,
		Version,
		Properties);
#endif

	// проверка параметров
	if (IsTrue(result))
		if ((Version < 0) ||
			(Version > EAX_NUM) ||
			IsBadWritePtr(Properties,
				sizeof(squall_eax_channel_t)))
			result = SQUALL_ERROR_INVALID_PARAM;

	// проверим взможность работы
	if (IsTrue(result))
		if (!context->_eaxSupport[Version])
			result = SQUALL_ERROR_EAX_VERSION;

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// получим данные
	if (IsTrue(result))
		if (!eax_Get(context->_channelsArray[channel].DS3D_PropSet,
			 	EAX_BUFFER,
			 	Version,
			 	& context->_channelsArray[channel].Param.EAXBP))
			result = SQUALL_ERROR_GET_EAX_PARAM;
		else
			memcpy(Properties,
				& context->_channelsArray[channel].Param.EAXBP,
				sizeof(squall_eax_channel_t));

		// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else {
		switch (Version) {
			// EAX 1.0
		case 1:
			api_Log("\t\t[out]Properties->Mix                 = %d",
				Properties->eax1.Mix);
			break;
			// EAX 2.0
		case 2:
			api_Log("\t\t[out]Properties->Direct              = %d",
				Properties->eax2.Direct);
			api_Log("\t\t[out]Properties->DirectHF            = %d",
				Properties->eax2.DirectHF);
			api_Log("\t\t[out]Properties->Room                = %d",
				Properties->eax2.Room);
			api_Log("\t\t[out]Properties->RoomHF              = %d",
				Properties->eax2.RoomHF);
			api_Log("\t\t[out]Properties->RoomRolloffFactor   = %f",
				Properties->eax2.RoomRolloffFactor);
			api_Log("\t\t[out]Properties->Obstruction         = %d",
				Properties->eax2.Obstruction);
			api_Log("\t\t[out]Properties->ObstructionLFRatio  = %f",
				Properties->eax2.ObstructionLFRatio);
			api_Log("\t\t[out]Properties->Occlusion           = %d",
				Properties->eax2.Occlusion);
			api_Log("\t\t[out]Properties->OcclusionLFRatio    = %f",
				Properties->eax2.OcclusionLFRatio);
			api_Log("\t\t[out]Properties->OcclusionRoomRatio  = %f",
				Properties->eax2.OcclusionRoomRatio);
			api_Log("\t\t[out]Properties->OutsideVolumeHF     = %d",
				Properties->eax2.OutsideVolumeHF);
			api_Log("\t\t[out]Properties->AirAbsorptionFactor = %f",
				Properties->eax2.AirAbsorptionFactor);
			api_Log("\t\t[out]Properties->Flags               = %d",
				Properties->eax2.Flags);
			break;
			// EAX 3.0
		case 3:
			api_Log("\t\t[out]Properties->Direct               = %d",
				Properties->eax3.Direct);
			api_Log("\t\t[out]Properties->DirectHF             = %d",
				Properties->eax3.DirectHF);
			api_Log("\t\t[out]Properties->Room                 = %d",
				Properties->eax3.Room);
			api_Log("\t\t[out]Properties->RoomHF               = %d",
				Properties->eax3.RoomHF);
			api_Log("\t\t[out]Properties->Obstruction          = %d",
				Properties->eax3.Obstruction);
			api_Log("\t\t[out]Properties->ObstructionLFRatio   = %f",
				Properties->eax3.ObstructionLFRatio);
			api_Log("\t\t[out]Properties->Occlusion            = %d",
				Properties->eax3.Occlusion);
			api_Log("\t\t[out]Properties->OcclusionLFRatio     = %f",
				Properties->eax3.OcclusionLFRatio);
			api_Log("\t\t[out]Properties->OcclusionRoomRatio   = %f",
				Properties->eax3.OcclusionRoomRatio);
			api_Log("\t\t[out]Properties->OcclusionDirectRatio = %f",
				Properties->eax3.OcclusionDirectRatio);
			api_Log("\t\t[out]Properties->Exclusion            = %d",
				Properties->eax3.Exclusion);
			api_Log("\t\t[out]Properties->ExclusionLFRatio     = %f",
				Properties->eax3.ExclusionLFRatio);
			api_Log("\t\t[out]Properties->OutsideVolumeHF      = %d",
				Properties->eax3.OutsideVolumeHF);
			api_Log("\t\t[out]Properties->DopplerFactor        = %f",
				Properties->eax3.DopplerFactor);
			api_Log("\t\t[out]Properties->RolloffFactor        = %f",
				Properties->eax3.RolloffFactor);
			api_Log("\t\t[out]Properties->RoomRolloffFactor    = %f",
				Properties->eax3.RoomRolloffFactor);
			api_Log("\t\t[out]Properties->AirAbsorptionFactor  = %f",
				Properties->eax3.AirAbsorptionFactor);
			api_Log("\t\t[out]Properties->Flags                = %d",
				Properties->eax3.Flags);
			break;
		}
	}
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Установка новых ZOOMFX параметров звукового канала
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  Properties  -  указатель на структуру описывающую параметры
//  							 ZOOMFX канала
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
// примечание  :  данный метод не работает с рассеянными каналами
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_ZOOMFX_SetProperties(int ChannelID,
	squall_zoomfx_channel_t* Properties)
#else
DLL_API int SQUALL_Channel_ZOOMFX_SetProperties(int ChannelID,
	squall_zoomfx_channel_t* Properties)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_ZOOMFX_SetProperties(0x%X, 0x%X)",
		ChannelID,
		Properties);
#endif
	// проверка параметров
	if (IsBadReadPtr(Properties, sizeof(squall_zoomfx_channel_t)))
		Properties = 0;

// отладочная информация
#if SQUALL_DEBUG
	// выведем параметры
	if (Properties) {
		api_Log("\t\t[in]Properties->Min     = [%f, %f, %f]",
			Properties->Min[0],
			Properties->Min[1],
			Properties->Min[2]);
		api_Log("\t\t[in]Properties->Max     = [%f, %f, %f]",
			Properties->Max[0],
			Properties->Max[1],
			Properties->Max[2]);
		api_Log("\t\t[in]Properties->Front   = [%f, %f, %f]",
			Properties->Front[0],
			Properties->Front[1],
			Properties->Front[2]);
		api_Log("\t\t[in]Properties->Top     = [%f, %f, %f]",
			Properties->Top[0],
			Properties->Top[1],
			Properties->Top[2]);
		api_Log("\t\t[in]Properties->MacroFX = %d", Properties->MacroFX);
	}
#endif

	// проверка параметров
	if (IsTrue(result))
		if (!Properties)
			result = SQUALL_ERROR_INVALID_PARAM;

	// проверим взможность работы
	if (IsTrue(result))
		if (!context->_zoomfxSupport)
			result = SQUALL_ERROR_NO_ZOOMFX;

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// получим данные
	if (IsTrue(result)) {
		memcpy(&context->_channelsArray[channel].Param.ZOOMFXBP,
			Properties,
			sizeof(squall_zoomfx_channel_t));
		if (!zoomfx_Set(context->_channelsArray[channel].DS_PropSet,
			 	& context->_channelsArray[channel].Param.ZOOMFXBP))
			result = SQUALL_ERROR_SET_ZOOMFX_PARAM;
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Получение текущих ZOOMFX параметров звукового канала
// на входе    :  ChannelID   -  идентификатор звукового канала
//  			  Properties  -  указатель на структуру куда нужно поместить
//  							 текущие параметры ZOOMFX канала
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
// примечание  :  данный метод не работает с рассеянными каналами
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Channel_ZOOMFX_GetProperties(int ChannelID,
	squall_zoomfx_channel_t* Properties)
#else
DLL_API int SQUALL_Channel_ZOOMFX_GetProperties(int ChannelID,
	squall_zoomfx_channel_t* Properties)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Channel_ZOOMFX_GetProperties(0x%X, 0x%X)",
		ChannelID,
		Properties);
#endif

	// проверка параметров
	if (IsTrue(result))
		if (IsBadWritePtr(Properties, sizeof(squall_zoomfx_channel_t)))
			result = SQUALL_ERROR_INVALID_PARAM;

	// проверим взможность работы
	if (IsTrue(result))
		if (!context->_zoomfxSupport)
			result = SQUALL_ERROR_NO_ZOOMFX;

	// поиск звука по идентификатору
	if (IsTrue(result)) {
		channel = context->GetChannelIndex(ChannelID);
		if (channel < 0)
			result = SQUALL_ERROR_CHANNEL_NOT_FOUND;
	}

	// получим данные
	if (IsTrue(result))
		if (!zoomfx_Get(context->_channelsArray[channel].DS_PropSet,
			 	& context->_channelsArray[channel].Param.ZOOMFXBP))
			result = SQUALL_ERROR_GET_ZOOMFX_PARAM;
		else
			memcpy(Properties,
				& context->_channelsArray[channel].Param.ZOOMFXBP,
				sizeof(squall_zoomfx_channel_t));

		// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else {
		api_Log("\t\t[out]Properties->Min     = [%f, %f, %f]",
			Properties->Min[0],
			Properties->Min[1],
			Properties->Min[2]);
		api_Log("\t\t[out]Properties->Max     = [%f, %f, %f]",
			Properties->Max[0],
			Properties->Max[1],
			Properties->Max[2]);
		api_Log("\t\t[out]Properties->Front   = [%f, %f, %f]",
			Properties->Front[0],
			Properties->Front[1],
			Properties->Front[2]);
		api_Log("\t\t[out]Properties->Top     = [%f, %f, %f]",
			Properties->Top[0],
			Properties->Top[1],
			Properties->Top[2]);
		api_Log("\t\t[out]Properties->MacroFX = %d", Properties->MacroFX);
	}
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//  			  Методы для работы с группами каналов
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
// Включение/выключение паузы группы каналов
// на входе    :  ChannelGroupID -  идентификатор группы каналов
//  			  Pause 		 -  флаг включения/выключения паузы, параметр
//  								может принимать слудующие значения:
//  								true  -  включить паузу
//  								false -  выключить паузу
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::ChannelGroup_Pause(int GroupID, int Pause)
#else
DLL_API int SQUALL_ChannelGroup_Pause(int GroupID, int Pause)
#endif
{
	// переменные
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_ChannelGroup_Pause(0x%X, %d)", GroupID, Pause);
#endif

	// остановим все звуки принадлежащие указаной группе
	if (IsTrue(result))
		for (int i = 0; i < context->_channels; i++)
			if (context->_channelsArray[i].GroupID == GroupID)
				if (context->_channelsArray[i].Status.STAGE != EMPTY)
					context->_channelsArray[i].Pause(Pause);

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Остановка группы каналов
// на входе    :  ChannelGroupID -  идентификатор группы каналов
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::ChannelGroup_Stop(int GroupID)
#else
DLL_API int SQUALL_ChannelGroup_Stop(int GroupID)
#endif
{
	// переменные
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_ChannelGroup_Stop(0x%X)", GroupID);
#endif

	// остановка всех звуков группы
	if (IsTrue(result))
		for (int i = 0; i < context->_channels; i++)
			if (context->_channelsArray[i].GroupID == GroupID)
				if (context->_channelsArray[i].Status.STAGE != EMPTY)
					context->_channelsArray[i].Stop();

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Установка уровня громкости группы каналов в процентах
// на входе    :  ChannelGroupID -  идентификатор группу каналов
//  			  Volume		 -  значение урокня громкости, значение должно
//  								лежать в пределах от 0 до 100
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::ChannelGroup_SetVolume(int GroupID, int Volume)
#else
DLL_API int SQUALL_ChannelGroup_SetVolume(int GroupID, int Volume)
#endif
{
	// переменные
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_ChannelGroup_SetVolume(0x%X, %d)", GroupID, Volume);
#endif

	// установка громкости всем каналам принадлежащим к указаной группе
	if (IsTrue(result))
		for (int i = 0; i < context->_channels; i++)
			if (context->_channelsArray[i].GroupID == GroupID)
				if (context->_channelsArray[i].Status.STAGE != EMPTY)
					context->_channelsArray[i].SetVolume(volume_table[Volume]);

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Установка новой частоты дискретизации группы каналов
// на входе    :  ChannelGroupID -  номер группы каналов
//  			  Frequency 	 -  новое значение частоты дискретизации,
//  								значение параметра должно быть в пределах
//  								от 100 до 1000000 Герц
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::ChannelGroup_SetFrequency(int GroupID, int Frequency)
#else
DLL_API int SQUALL_ChannelGroup_SetFrequency(int GroupID, int Frequency)
#endif
{
	// переменные
	context_t* context = 0;
	int channel = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_ChannelGroup_SetFrequency(0x%X, %d)",
		GroupID,
		Frequency);
#endif

	// проверка параметров
	if (IsTrue(result))
		if ((Frequency < 100) || (Frequency > 100000))
			result = SQUALL_ERROR_INVALID_PARAM;

	// остановка всех звуков группы
	if (IsTrue(result))
		for (int i = 0; i < context->_channels; i++)
			if (context->_channelsArray[i].GroupID == GroupID)
				if (context->_channelsArray[i].Status.STAGE != EMPTY)
					context->_channelsArray[i].SetFrequency(Frequency);

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//  			  Методы для работы с семплами
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
// Создание семпла из файла
// на входе    :  FileName - указатель на имя файла
//  			  MemFlag  - флаг определяющий расположение файла, параметр
//  						 может принимать следующие значения:
//  						 true  -   размещать данные файла в памяти
//  						 false -   разместить данные файла на диске
//  			  Default  - указатель на структуру параметров семпла по
//  						 умолчанию, если параметр равен 0, загрузчик
//  						 установит следующие параметры семпла по умолчанию:
//  						 SampleGroupID - 0
//  						 Priority      - 0
//  						 Frequency     - 0
//  						 Volume 	   - 100
//  						 Pan		   - 50
//  						 MinDist	   - 1.0f
//  						 MaxDist	   - 1000000000.0f
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//  			  в случае успешного вызова результат содержит идентификатор
//  			  созданного семпла
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Sample_LoadFile(char* FileName, int MemFlag,
	squall_sample_default_t* Default)
#else
DLL_API int SQUALL_Sample_LoadFile(char* FileName, int MemFlag,
	squall_sample_default_t* Default)
#endif
{
	// переменные
	int result = true;

	// проверка параметров
	if (IsBadReadPtr(FileName, sizeof(char)))
		FileName = 0;
	if (IsBadReadPtr(Default, sizeof(squall_sample_default_t)))
		Default = 0;

	// отладочная информация
#if SQUALL_DEBUG
	if (FileName)
		api_LogTime("SQUALL_Sample_LoadFile(%s, %d, 0x%X)",
			FileName,
			MemFlag,
			Default);
	else
		api_LogTime("SQUALL_Sample_LoadFile(%s, %d, 0x%X)",
			"error FileName",
			MemFlag,
			Default);

	// параметры по умолчанию
	if (Default) {
		api_Log("\t\t[in]Default->SampleGroupID = %d", Default->SampleGroupID);
		api_Log("\t\t[in]Default->Frequency     = %d", Default->Frequency);
		api_Log("\t\t[in]Default->Volume        = %d", Default->Volume);
		api_Log("\t\t[in]Default->Pan           = %d", Default->Pan);
		api_Log("\t\t[in]Default->Priority      = %d", Default->Priority);
		api_Log("\t\t[in]Default->MinDist       = %f", Default->MinDist);
		api_Log("\t\t[in]Default->MaxDist       = %f", Default->MaxDist);
	}
#endif

	// проверка параметров
	if (!FileName)
		result = SQUALL_ERROR_INVALID_PARAM;

	// загрузим файл в ядро
	if (IsTrue(result))
		result = api_SampleLoadFile(FileName, MemFlag, Default);

	// проверка успешности загрузки
	if (result <= 0)
		result = SQUALL_ERROR_SAMPLE_BAD;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = 0x%X", result);
#endif

	return result;
}
//-----------------------------------------------------------------------------
// Создание семпла из памяти
// на входе    :  FileName - указатель на имя файла
//  			  MemFlag  - флаг определяющий расположение файла, параметр
//  						 может принимать следующие значения:
//  						 true  -   размещать данные файла в памяти
//  						 false -   разместить данные файла на диске
//  			  Default  - указатель на структуру параметров семпла по
//  						 умолчанию, если параметр равен 0, загрузчик
//  						 установит следующие параметры семпла по умолчанию:
//  						 SampleGroupID - 0
//  						 Priority      - 0
//  						 Frequency     - 0
//  						 Volume 	   - 100
//  						 Pan		   - 50
//  						 MinDist	   - 1.0f
//  						 MaxDist	   - 1000000000.0f
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//  			  в случае успешного вызова результат содержит идентификатор
//  			  созданного семпла
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Sample_LoadFromMemory(void* Ptr, unsigned int Size,
	int NewMemFlag, squall_sample_default_t* Default)
#else
DLL_API int SQUALL_Sample_LoadFromMemory(void* Ptr, unsigned int Size,
	int NewMemFlag, squall_sample_default_t* Default)
#endif
{
	// переменные
	int result = true;

	// проверка параметров
	if (IsBadReadPtr(Ptr, Size))
		Ptr = 0;
	if (IsBadReadPtr(Default, sizeof(squall_sample_default_t)))
		Default = 0;

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Sample_LoadFromMemory(0x%X, %d, %d, 0x%X)",
		Ptr,
		Size,
		NewMemFlag,
		Default);

	// параметры по умолчанию
	if (Default) {
		api_Log("\t\t[in]Default->SampleGroupID = %d", Default->SampleGroupID);
		api_Log("\t\t[in]Default->Frequency     = %d", Default->Frequency);
		api_Log("\t\t[in]Default->Volume        = %d", Default->Volume);
		api_Log("\t\t[in]Default->Pan           = %d", Default->Pan);
		api_Log("\t\t[in]Default->Priority      = %d", Default->Priority);
		api_Log("\t\t[in]Default->MinDist       = %f", Default->MinDist);
		api_Log("\t\t[in]Default->MaxDist       = %f", Default->MaxDist);
	}
#endif

	// проверка параметров
	if (!Ptr)
		result = SQUALL_ERROR_INVALID_PARAM;

	// загрузим файл в ядро
	if (IsTrue(result))
		result = api_CreateSampleFromMemory(Ptr, Size, NewMemFlag, Default);

	// проверка успешности загрузки
	if (result <= 0)
		result = SQUALL_ERROR_SAMPLE_BAD;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = 0x%X", result);
#endif

	return result;
}

//-----------------------------------------------------------------------------
// Освобождение всех семплов
// на входе    :  *
// на выходе   :  *
//-----------------------------------------------------------------------------
#ifndef _USRDLL
void Squall::Sample_UnloadAll(void)
#else
DLL_API void SQUALL_Sample_UnloadAll(void)
#endif
{
	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Sample_UnloadAll()");
#endif

	// выгрузка всех звуков
	api_SampleUnloadAll();
}

//-----------------------------------------------------------------------------
// Освобождение указанного семпла
// на входе    :  SampleID -  идентификатор семпла
// на выходе   :  *
//-----------------------------------------------------------------------------
#ifndef _USRDLL
void Squall::Sample_Unload(int SampleID)
#else
DLL_API void SQUALL_Sample_Unload(int SampleID)
#endif
{
	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Sample_Unload(0x%X)", SampleID);
#endif

	// удаление семпла
	api_SampleUnload(SampleID);
}

//-----------------------------------------------------------------------------
// Получение продолжительности данных звукового файла в отсчетах
// на входе    :  SampleID -  идентификатор семпла
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//  			  в случае успешного вызова результат содержит продолжительность
//  			  данных в отсчетах
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Sample_GetFileLength(int SampleID)
#else
DLL_API int SQUALL_Sample_GetFileLength(int SampleID)
#endif
{
	CSoundFile* file = 0;
	int result = true;

#if SQUALL_DEBUG
	api_LogTime("SQUALL_Sample_GetFileLength(0x%X)", SampleID);
#endif

	// получение указателя на семпл
	if (IsTrue(result)) {
		file = api_SampleGetFile(SampleID);
		if (!file)
			result = SQUALL_ERROR_INVALID_PARAM;
	}

	// получим количество семплов в файле
	if (IsTrue(result)) {
		result = file->GetSamplesInFile();
		if (!result)
			result = SQUALL_ERROR_SAMPLE_BAD;
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = %d", result);
#endif

	// вернем результат работы
	return result;
}

//-----------------------------------------------------------------------------
// Получение продолжительности данных звукового файла в миллисекундах
// на входе    :  SampleID -  идентификатор семпла
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//  			  в случае успешного вызова результат содержит продолжительность
//  			  данных в милисекундах
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Sample_GetFileLengthMs(int SampleID)
#else
DLL_API int SQUALL_Sample_GetFileLengthMs(int SampleID)
#endif
{
	CSoundFile* file = 0;
	int result = true;

#if SQUALL_DEBUG
	api_LogTime("SQUALL_Sample_GetFileLengthMs(0x%X)", SampleID);
#endif

	// получение указателя на семпл
	if (IsTrue(result)) {
		file = api_SampleGetFile(SampleID);
		if (!file)
			result = SQUALL_ERROR_INVALID_PARAM;
	}

	// получим количество семплов в файле
	if (IsTrue(result)) {
		result = file->GetSamplesInFile();
		if (!result)
			result = SQUALL_ERROR_SAMPLE_BAD;
		else
			result = (int)
				((__int64) (result * (__int64) 1000) /
				(__int64) file->GetFormat()->nSamplesPerSec);
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = %d", result);
#endif

	// вернем результат работы
	return result;
}

//-----------------------------------------------------------------------------
// Получение частоты дискретизации данных звукового файла
// на входе    :  SampleID -  идентификатор семпла
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//  			  в случае успешного вызова результат содержит частоту
//  			  дискретизации
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Sample_GetFileFrequency(int SampleID)
#else
DLL_API int SQUALL_Sample_GetFileFrequency(int SampleID)
#endif
{
	CSoundFile* file = 0;
	int result = true;

#if SQUALL_DEBUG
	api_LogTime("SQUALL_Sample_GetFileFrequency(0x%X)", SampleID);
#endif

	// получение указателя на семпл
	if (IsTrue(result)) {
		file = api_SampleGetFile(SampleID);
		if (!file)
			result = SQUALL_ERROR_INVALID_PARAM;
	}

	// получим количество семплов в файле
	if (IsTrue(result)) {
		result = file->GetFormat()->nSamplesPerSec;
		if (!result)
			result = SQUALL_ERROR_SAMPLE_BAD;
	}

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = %d", result);
#endif

	// вернем результат работы
	return result;
}
//-----------------------------------------------------------------------------
// Установка новых параметров семпла по умолчанию
// на входе    :  SampleID -  идентификатор семпла
//  			  Default  -  указатель на структуру с новыми параметрами семпла
//  						  по умолчанию
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Sample_SetDefault(int SampleID, squall_sample_default_t* Default)
#else
DLL_API int SQUALL_Sample_SetDefault(int SampleID,
	squall_sample_default_t* Default)
#endif
{
	int result = true;

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Sample_SetDefault(0x%X, 0x%X)", SampleID, Default);
#endif

	// проверка параметров
	if (IsBadWritePtr(Default, sizeof(squall_sample_default_t)))
		Default = 0;

	// отладочная информация
#if SQUALL_DEBUG
	if (Default) {
		api_Log("\t\t[in]Default->SampleGroupID = %d", Default->SampleGroupID);
		api_Log("\t\t[in]Default->Frequency     = %d", Default->Frequency);
		api_Log("\t\t[in]Default->Volume        = %d", Default->Volume);
		api_Log("\t\t[in]Default->Pan           = %d", Default->Pan);
		api_Log("\t\t[in]Default->Priority      = %d", Default->Priority);
		api_Log("\t\t[in]Default->MinDist       = %f", Default->MinDist);
		api_Log("\t\t[in]Default->MaxDist       = %f", Default->MaxDist);
	}
#endif

	// проверка параметров
	if (!Default)
		result = SQUALL_ERROR_INVALID_PARAM;

	// установка параметров
	if (IsTrue(result))
		if (!api_SampleSetDefault(SampleID, Default))
			result = SQUALL_ERROR_INVALID_PARAM;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// вернем результат работы
	return result;
}

//-----------------------------------------------------------------------------
// Получение текущих параметров семпла по умолчанию
// на входе    :  SampleID -  идентификатор семпла
//  			  Default  -  указатель на структуру в которую нужно поместить
//  						  текущие параметры семпла по умолчанию
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Sample_GetDefault(int SampleID, squall_sample_default_t* Default)
#else
DLL_API int SQUALL_Sample_GetDefault(int SampleID,
	squall_sample_default_t* Default)
#endif
{
	// переменные
	int result = true;

#if SQUALL_DEBUG
	api_LogTime("SQUALL_Sample_GetDefault(0x%X, 0x%X)", SampleID, Default);
#endif

	// проверка параметров
	if (IsBadWritePtr(Default, sizeof(squall_sample_default_t)))
		result = SQUALL_ERROR_INVALID_PARAM;

	// получение параметров
	if (IsTrue(result))
		if (!api_SampleGetDefault(SampleID, Default))
			result = SQUALL_ERROR_INVALID_PARAM;

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else {
		api_Log("\t\t[out]Default->SampleGroupID = %d", Default->SampleGroupID);
		api_Log("\t\t[out]Default->Frequency     = %d", Default->Frequency);
		api_Log("\t\t[out]Default->Volume        = %d", Default->Volume);
		api_Log("\t\t[out]Default->Pan           = %d", Default->Pan);
		api_Log("\t\t[out]Default->Priority      = %d", Default->Priority);
		api_Log("\t\t[out]Default->MinDist       = %f", Default->MinDist);
		api_Log("\t\t[out]Default->MaxDist       = %f", Default->MaxDist);
	}
#endif

	// вернем результат  
	return result;
}
//-----------------------------------------------------------------------------
// Процедура воспроизведения рассеяного звука
// на входе :  SampleID  - идентификатор загруженого звука
//  		   Loop  - флаг бесконечного воспроизведения
//  				   true   - воспроизводить звук в цикле бесконечно
//  				   false  - воспроизвести звук один раз
//  		   Group - принадлежность создаваемого канала к группе
//  		   Start - флаг запуска звука по окончанию создания канала
//  				   true   - канал начнет проигрываться сразу же
//  				   false  - канал будет только подготовлен, для того
//  						 чтобы начать воспроизведение нужно
//  						 вызвать метод CHANNEL_Start()
// на выходе   :  идентификатор звукового канала, если выходное значение равно
//  		   0, значит произошла ошибка, какая именно можно узнать
//  		   вызвав метод SYSTEM_GetError()
// примечание  :  1. Остальные параметры как громкость, панорама, частота,
//  		   приоритет берутся из параметров звука по умолчанию
//  		   2. Воспроизведение производиться с преобразованием звука в 
//  		   стерео режим. По сему лучше использовать заранее стерео
//  		   записи
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Sample_Play(int SampleID, int Loop, int ChannelGroupID, int Start)
#else
DLL_API int SQUALL_Sample_Play(int SampleID, int Loop, int ChannelGroupID,
	int Start)
#endif
{
	// переменные
	squall_sample_default_t def;
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Sample_Play(0x%X, %d, %d, %d)",
		SampleID,
		Loop,
		ChannelGroupID,
		Start);
#endif

	// получение параметров звука по умолчанию
	if (IsTrue(result))
		if (!api_SampleGetDefault(SampleID, & def))
			result = SQUALL_ERROR_SAMPLE_BAD;

	// создание канала
	result = CreateChannel(context,
			 	SampleID,
			 	Loop,
			 	ChannelGroupID,
			 	Start,
			 	def.Priority,
			 	def.Volume,
			 	def.Frequency,
			 	def.Pan);

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = 0x%X", result);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Процедура воспроизведения рассеяного звука, расширенная версия
// на входе :  SampleID 	- идентификатор загруженого звука
//  		   Loop 	- флаг бесконечного воспроизведения
//  					  true   - воспроизводить звук в цикле бесконечно
//  					  false  - воспроизвести звук один раз
//  		   Group	- принадлежность создаваемого канала к группе
//  		   Start	- флаг запуска звука по окончанию создания канала
//  					  true   - канал начнет проигрываться сразу же
//  					  false  - канал будет только подготовлен, для того
//  							чтобы начать воспроизведение нужно
//  							вызвать метод CHANNEL_Start()
//  		   Priority - приоритет создаваемого звукового канала,
//  					  значение должно лежать в пределах от 0 до 255
//  		   Volume      - громкость создаваемого звукового канала,
//  					  значение должно лежать в пределах от 0 до 100
//  		   Frequency   - частота создаваемого звукового канала
//  					  значение должно лежать в пределах от 100 до 100000
//  		   Pan  	   - панорама создаваемого звукового канала
//  					  значение должно лежать в пределах от 0 до 100
// на выходе   :  идентификатор звукового канала, если выходное значение равно
//  		   0, значит произошла ошибка, какая именно можно узнать
//  		   вызвав метод SYSTEM_GetError()
// примечание  :  1. Воспроизведение производиться с преобразованием звука в 
//  		   стерео режим. По сему лучше использовать заранее стерео
//  		   записи
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Sample_PlayEx(int SampleID, int Loop, int ChannelGroupID,
	int Start, int Priority, int Volume, int Frequency, int Pan)
#else
DLL_API int SQUALL_Sample_PlayEx(int SampleID, int Loop, int ChannelGroupID,
	int Start, int Priority, int Volume, int Frequency, int Pan)
#endif
{
	// переменные
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Sample_PlayEx(0x%X, %d, %d, %d, %d, %d, %d, %d)",
		SampleID,
		Loop,
		ChannelGroupID,
		Start,
		Priority,
		Volume,
		Frequency,
		Pan);
#endif

	// создание канала
	result = CreateChannel(context,
			 	SampleID,
			 	Loop,
			 	ChannelGroupID,
			 	Start,
			 	Priority,
			 	Volume,
			 	Frequency,
			 	Pan);

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\tresult = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = 0x%X", result);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Процедура воспроизведения точечного звука
// на входе :  SampleID 	- идентификатор загруженого звука
//  		   Loop 	- флаг бесконечного воспроизведения
//  					  true   - воспроизводить звук в цикле бесконечно
//  					  false  - воспроизвести звук один раз
//  		   Group	- принадлежность создаваемого канала к группе
//  		   Start	- флаг запуска звука по окончанию создания канала
//  					  true   - канал начнет проигрываться сразу же
//  					  false  - канал будет только подготовлен, для того
//  							чтобы начать воспроизведение нужно
//  							вызвать метод CHANNEL_Start()
//  		   Position - указатель на структуру координат звукового
//  					  источника
//  		   Velocity - указатель на вектор скорости источника звука,
//  					  если значение скорости не важно, то данный
//  					  параметр может быть равен 0
// на выходе   :  идентификатор звукового канала, если выходное значение равно
//  		   0, значит произошла ошибка, какая именно можно узнать
//  		   вызвав метод SYSTEM_GetError()
// примечание  :  1. Остальные параметры как громкость, частота, приоритет,
//  		   минимальное и максимальное растояние слышимости, берутся
//  		   из параметров звука по умолчанию
//  		   2. Воспроизведение производиться с преобразованием звука в 
//  		   моно режим. По сему лучше использовать заранее моно записи
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Sample_Play3D(int SampleID, int Loop, int ChannelGroupID,
	int Start, float* Position, float* Velocity)
#else
DLL_API int SQUALL_Sample_Play3D(int SampleID, int Loop, int ChannelGroupID,
	int Start, float* Position, float* Velocity)
#endif
{
	// переменные
	squall_sample_default_t def;
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Sample_Play3D(0x%X, %d, %d, %d, 0x%X, 0x%X)",
		SampleID,
		Loop,
		ChannelGroupID,
		Start,
		Position,
		Velocity);
#endif

	// проверка параметров
	if (IsBadReadPtr(Position, sizeof(float) * 3))
		Position = 0;
	if (IsBadReadPtr(Velocity, sizeof(float) * 3))
		Velocity = 0;

	// отладочная информация
#if SQUALL_DEBUG
	// позиция звука
	if (Position) {
		api_Log("\t\t[in]Position->x = %f", Position[0]);
		api_Log("\t\t[in]Position->y = %f", Position[1]);
		api_Log("\t\t[in]Position->z = %f", Position[2]);
	}
	// скорость звука
	if (Velocity) {
		api_Log("\t\t[in]Velocity->x = %f", Velocity[0]);
		api_Log("\t\t[in]Velocity->y = %f", Velocity[1]);
		api_Log("\t\t[in]Velocity->z = %f", Velocity[2]);
	}
#endif

	// проверка параметров
	if (IsTrue(result))
		if (!Position)
			result = SQUALL_ERROR_INVALID_PARAM;

	// получение параметров звука по умолчанию
	if (IsTrue(result))
		if (!api_SampleGetDefault(SampleID, & def))
			result = SQUALL_ERROR_SAMPLE_BAD;

	// создание канала
	result = CreateChannel3D(context,
			 	SampleID,
			 	Loop,
			 	ChannelGroupID,
			 	Start,
			 	Position,
			 	Velocity,
			 	def.Priority,
			 	def.Volume,
			 	def.Frequency,
			 	def.MinDist,
			 	def.MaxDist);

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = 0x%X", result);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Процедура воспроизведения точечного звука, расширенная версия
// на входе :  SampleID 	- идентификатор загруженого звука
//  		   Loop 	- флаг бесконечного воспроизведения
//  					  true   - воспроизводить звук в цикле бесконечно
//  					  false  - воспроизвести звук один раз
//  		   Group	- принадлежность создаваемого канала к группе
//  		   Start	- флаг запуска звука по окончанию создания канала
//  					  true   - канал начнет проигрываться сразу же
//  					  false  - канал будет только подготовлен, для того
//  							чтобы начать воспроизведение нужно
//  							вызвать метод CHANNEL_Start()
//  		   Position - указатель на структуру координат звукового
//  					  источника
//  		   Velocity - указатель на вектор скорости источника звука,
//  					  если значение скорости не важно, то данный
//  					  параметр может быть равен 0
//  		   Priority - приоритет создаваемого звукового канала,
//  					  значение должно лежать в пределах от 0 до 255
//  		   Volume      - громкость создаваемого звукового канала,
//  					  значение должно лежать в пределах от 0 до 100
//  		   Frequency   - частота создаваемого звукового канала
//  					  значение должно лежать в пределах от 100 до 100000
//  		   MinDist     - минимальное растояние слышимости
//  		   MaxDist     - максимальное растояние слышимости
// на выходе   :  идентификатор звукового канала, если выходное значение равно
//  		   0, значит произошла ошибка, какая именно можно узнать
//  		   вызвав метод SYSTEM_GetError()
// примечание  :  1. Воспроизведение производиться с преобразованием звука в 
//  		   моно режим. По сему лучше использовать заранее моно записи
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Sample_Play3DEx(int SampleID, int Loop, int ChannelGroupID,
	int Start, float* Position, float* Velocity, int Priority, int Volume,
	int Frequency, float MinDist, float MaxDist)
#else
DLL_API int SQUALL_Sample_Play3DEx(int SampleID, int Loop, int ChannelGroupID,
	int Start, float* Position, float* Velocity, int Priority, int Volume,
	int Frequency, float MinDist, float MaxDist)
#endif
{
	// переменные
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Sample_Play3DEx(0x%X, %d, %d, %d, 0x%X, 0x%X, %d, %d, %d, %f, %f)",
		SampleID,
		Loop,
		ChannelGroupID,
		Start,
		Position,
		Velocity,
		Priority,
		Volume,
		Frequency,
		MinDist,
		MaxDist);
#endif

	// проверка параметров
	if (IsBadReadPtr(Position, sizeof(float) * 3))
		Position = 0;
	if (IsBadReadPtr(Velocity, sizeof(float) * 3))
		Velocity = 0;

	// отладочная информация
#if SQUALL_DEBUG
	// позиция звука
	if (Position) {
		api_Log("\t\t[in]Position->x = %f", Position[0]);
		api_Log("\t\t[in]Position->y = %f", Position[1]);
		api_Log("\t\t[in]Position->z = %f", Position[2]);
	}
	// скорость звука
	if (Velocity) {
		api_Log("\t\t[in]Velocity->x = %f", Velocity[0]);
		api_Log("\t\t[in]Velocity->y = %f", Velocity[1]);
		api_Log("\t\t[in]Velocity->z = %f", Velocity[2]);
	}
#endif

	// проверка параметров
	if (IsTrue(result))
		if (!Position)
			result = SQUALL_ERROR_INVALID_PARAM;

	// создание канала
	result = CreateChannel3D(context,
			 	SampleID,
			 	Loop,
			 	ChannelGroupID,
			 	Start,
			 	Position,
			 	Velocity,
			 	Priority,
			 	Volume,
			 	Frequency,
			 	MinDist,
			 	MaxDist);

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = 0x%X", result);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Включение/выключение паузы всех каналов использующих указаный семпл
// на входе    :  SampleID -  указатель на данные звука
//  			  Pause    -  флаг включения/выключения паузы, параметр может
//  						  принимать следующие значения:
//  						  true   - включить паузу
//  						  false  - выключить паузу
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Sample_Pause(int SampleID, int Pause)
#else
DLL_API int SQUALL_Sample_Pause(int SampleID, int Pause)
#endif
{
	// переменные
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Sample_Pause(0x%X, %d)", SampleID, Pause);
#endif

	if (IsTrue(result))
		for (int i = 0; i < context->_channels; i++)
			if (context->_channelsArray[i].Status.STAGE != EMPTY)
				if (context->_channelsArray[i].SampleID == SampleID)
					context->_channelsArray[i].Pause(Pause);

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Остановка всех звуковых каналов использующих указанный семпл
// на входе    :  SampleID -  идентификатор семпла
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::Sample_Stop(int SampleID)
#else
DLL_API int SQUALL_Sample_Stop(int SampleID)
#endif
{
	// переменные
	context_t* context = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_Sample_Stop(0x%X)", SampleID);
#endif

	if (IsTrue(result))
		for (int i = 0; i < context->_channels; i++)
			if (context->_channelsArray[i].Status.STAGE != EMPTY)
				if (context->_channelsArray[i].SampleID == SampleID)
					context->_channelsArray[i].Stop();

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//  			  Методы для работы с группами семплов
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
// Процедура воспроизведения рассеяного звука из группы
// на входе :  SoundGroupID   - идентификатор загруженой группы
//  		   Loop 	   - флаг бесконечного воспроизведения
//  						 true   - воспроизводить звук в цикле бесконечно
//  						 false  - воспроизвести звук один раз
//  		   Group	   - принадлежность создаваемого канала к группе
//  		   Start	   - флаг запуска звука по окончанию создания канала
//  						 true   - канал начнет проигрываться сразу же
//  						 false  - канал будет только подготовлен, для того
//  							   чтобы начать воспроизведение нужно
//  							   вызвать метод CHANNEL_Start()
// на выходе   :  идентификатор звукового канала, если выходное значение равно
//  		   0, значит произошла ошибка, какая именно можно узнать
//  		   вызвав метод SYSTEM_GetError()
// примечание  :  1. Остальные параметры как громкость, панорама, частота,
//  		   приоритет берутся из параметров звука по умолчанию
//  		   2. Воспроизведение производиться с преобразованием звука в 
//  		   стерео режим. По сему лучше использовать заранее стерео
//  		   записи
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::SampleGroup_Play(int SampleGroupID, int Loop, int ChannelGroupID,
	int Start)
#else
DLL_API int SQUALL_SampleGroup_Play(int SampleGroupID, int Loop,
	int ChannelGroupID, int Start)
#endif
{
	// переменные
	squall_sample_default_t def;
	context_t* context = 0;
	int SampleID = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_SampleGroup_Play(0x%X, %d, %d, %d)",
		SampleGroupID,
		Loop,
		ChannelGroupID,
		Start);
#endif

	// получим указатель на данные
	if (IsTrue(result)) {
		SampleID = api_SampleGetFileGroup(SampleGroupID, 0);
		if (SampleID == 0)
			result = SQUALL_ERROR_SAMPLE_BAD;
	}

	// получение параметров звука по умолчанию
	if (IsTrue(result))
		if (!api_SampleGetDefault(SampleID, & def))
			result = SQUALL_ERROR_SAMPLE_BAD;

	// создание канала
	result = CreateChannel(context,
			 	SampleID,
			 	Loop,
			 	ChannelGroupID,
			 	Start,
			 	def.Priority,
			 	def.Volume,
			 	def.Frequency,
			 	def.Pan);

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = 0x%X", result);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Процедура воспроизведения рассеяного звука из группы, расширенная версия
// на входе :  SoundGroupID   - идентификатор загруженой группы
//  		   Loop 	   - флаг бесконечного воспроизведения
//  						 true   - воспроизводить звук в цикле бесконечно
//  						 false  - воспроизвести звук один раз
//  		   Group	   - принадлежность к группе
//  		   Start	   - флаг запуска звука по окончанию создания канала
//  						 true   - канал начнет проигрываться сразу же
//  						 false  - канал будет только подготовлен, для того
//  							   чтобы начать воспроизведение нужно
//  							   вызвать метод CHANNEL_Start()
//  		   Priority    - приоритет создаваемого звукового канала,
//  						 значение должно лежать в пределах от 0 до 255
//  		   Volume   	  - громкость создаваемого звукового канала,
//  						 значение должно лежать в пределах от 0 до 100
//  		   Frequency	  - частота создаваемого звукового канала
//  						 значение должно лежать в пределах от 100 до 100000
//  		   Pan  		  - панорама создаваемого звукового канала
//  						 значение должно лежать в пределах от 0 до 100
// на выходе   :  идентификатор звукового канала, если выходное значение равно
//  		   0, значит произошла ошибка, какая именно можно узнать
//  		   вызвав метод SYSTEM_GetError()
// примечание  :  1. Воспроизведение производиться с преобразованием звука в
//  		   стерео режим. По сему лучше использовать заранее стерео
//  		   записи
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::SampleGroup_PlayEx(int SampleGroupID, int Loop,
	int ChannelGroupID, int Start, int Priority, int Volume, int Frequency,
	int Pan)
#else
DLL_API int SQUALL_SampleGroup_PlayEx(int SampleGroupID, int Loop,
	int ChannelGroupID, int Start, int Priority, int Volume, int Frequency,
	int Pan)
#endif
{
	// переменные
	context_t* context = 0;
	int SampleID = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_SampleGroup_PlayEx(0x%X, %d, %d, %d, %d, %d, %d, %d)",
		SampleGroupID,
		Loop,
		ChannelGroupID,
		Priority,
		Start,
		Volume,
		Frequency,
		Pan);
#endif

	// получим указатель на данные
	if (IsTrue(result)) {
		SampleID = api_SampleGetFileGroup(SampleGroupID, 0);
		if (SampleID == 0)
			result = SQUALL_ERROR_SAMPLE_BAD;
	}

	// создание канала
	result = CreateChannel(context,
			 	SampleID,
			 	Loop,
			 	ChannelGroupID,
			 	Start,
			 	Priority,
			 	Volume,
			 	Frequency,
			 	Pan);

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = 0x%X", result);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Процедура воспроизведения точечного звука из группы
// на входе :  SoundGroupID   - идентификатор загруженого звука
//  		   Loop 	   - флаг бесконечного воспроизведения
//  						 true   - воспроизводить звук в цикле бесконечно
//  						 false  - воспроизвести звук один раз
//  		   Group	   - принадлежность создаваемого канала к группе
//  		   Start	   - флаг запуска звука по окончанию создания канала
//  						 true   - канал начнет проигрываться сразу же
//  						 false  - канал будет только подготовлен, для того
//  							   чтобы начать воспроизведение нужно
//  							   вызвать метод CHANNEL_Start()
//  		   Position    - указатель на структуру координат звукового
//  						 источника
//  		   Velocity    - указатель на вектор скорости источника звука,
//  						 если значение скорости не важно, то данный
//  						 параметр может быть равен 0
// на выходе   :  идентификатор звукового канала, если выходное значение равно
//  		   0, значит произошла ошибка, какая именно можно узнать
//  		   вызвав метод SYSTEM_GetError()
// примечание  :  1. Остальные параметры как громкость, частота, приоритет,
//  		   минимальное и максимальное растояние слышимости, берутся
//  		   из параметров звука по умолчанию
//  		   2. Воспроизведение производиться с преобразованием звука в 
//  		   моно режим. По сему лучше использовать заранее моно записи
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::SampleGroup_Play3D(int SampleGroupID, int Loop,
	int ChannelGroupID, int Start, float* Position, float* Velocity)
#else
DLL_API int SQUALL_SampleGroup_Play3D(int SampleGroupID, int Loop,
	int ChannelGroupID, int Start, float* Position, float* Velocity)
#endif
{
	// переменные
	squall_sample_default_t def;
	context_t* context = 0;
	int SampleID = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_SampleGroup_Play3D(0x%X, %d, %d, %d, 0x%X, 0x%X)",
		SampleGroupID,
		Loop,
		ChannelGroupID,
		Start,
		Position,
		Velocity);
#endif

	// проверка параметров
	if (IsBadReadPtr(Position, sizeof(float) * 3))
		Position = 0;
	if (IsBadReadPtr(Velocity, sizeof(float) * 3))
		Velocity = 0;

	// отладочная информация
#if SQUALL_DEBUG
	// позиция звука
	if (Position) {
		api_Log("\t\t[in]Position->x = %f", Position[0]);
		api_Log("\t\t[in]Position->y = %f", Position[1]);
		api_Log("\t\t[in]Position->z = %f", Position[2]);
	}
	// скорость звука
	if (Velocity) {
		api_Log("\t\t[in]Velocity->x = %f", Velocity[0]);
		api_Log("\t\t[in]Velocity->y = %f", Velocity[1]);
		api_Log("\t\t[in]Velocity->z = %f", Velocity[2]);
	}
#endif

	// проверка параметров
	if (IsTrue(result))
		if (!Position)
			result = SQUALL_ERROR_INVALID_PARAM;

	// получим указатель на данные
	if (IsTrue(result)) {
		SampleID = api_SampleGetFileGroup(SampleGroupID, 0);
		if (SampleID == 0)
			result = SQUALL_ERROR_SAMPLE_BAD;
	}

	// получение параметров звука по умолчанию
	if (IsTrue(result))
		if (!api_SampleGetDefault(SampleID, & def))
			result = SQUALL_ERROR_SAMPLE_BAD;

	// создание канала
	result = CreateChannel3D(context,
			 	SampleID,
			 	Loop,
			 	ChannelGroupID,
			 	Start,
			 	Position,
			 	Velocity,
			 	def.Priority,
			 	def.Volume,
			 	def.Frequency,
			 	def.MinDist,
			 	def.MaxDist);

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = 0x%X", result);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}

//-----------------------------------------------------------------------------
// Создание и воспроизведение позиционного (трехмерного) звукового канала из
// указаной группы семплов
// на входе    :  SampleGroupID  -  идентификатор группы семплов
//  			  Loop  		 -  флаг зацикленности воспроизведения, параметр
//  								может принимать следующие значения:
//  								true  -  воспроизводить канал в цикле
//  										 бесконечно
//  								false -  воспроизвести канал один раз
//  			  ChannelGroupID -  принадлежность создаваемого канала к группе
//  								каналов, если значение параметра равно 0
//  								значит звуковой канал не принадлежит группе
//  								каналов.
//  			  Start 		 -  флаг запуска звука по окончанию создания
//  								канала, параметр может принимать следующие
//  								значения:
//  								true  -  канал начнет воспроизводится сразу
//  										 после создания
//  								false -  канал будет только подготовлен,
//  										 для того чтобы начать воспроизведение
#ifndef _USRDLL
//  										 нужно вызвать метод Channel_Start()
#else
//  										 нужно вызвать метод
//  										 SQUALL_Channel_Start()
#endif
//  			  Position  	 -  указатель на структуру c координатами
//  								источника звукового канала
//  			  Velocity  	 -  указатель на вектор скорости источника
//  								звукового канала, в случае если значение
//  								вектора скорости устанавливать не надо,
//  								то данный параметр должен быть равен 0
//  			  Priority  	 -  приоритет создаваемого звукового канала,
//  								значение параметра должно лежать в пределах
//  								от 0 до 65535
//  			  Volume		 -  громкость создаваемого звукового канала,
//  								в процентах, значение параметра должно
//  								лежать в пределах от 0 до 100
//  			  Frequency 	 -  частота дискретизации звукового канала,
//  								значение параметра должно лежать в пределах
//  								от 100 до 1000000000
//  			  MinDist   	 -  минимальное растояние слышимости звукового
//  								канала, значение параметра должно быть в
//  								пределах от 0.01f до 1000000000.0f
//  			  MaxDist   	 -  максимальное растояние слышимости звукового
//  								канала, значение параметра должно быть в
//  								пределах от 0.01f до 1000000000.0f
// на выходе   :  успешность, если возвращаемый результат больше либо равен 0,
//  			  вызов состоялся, иначе результат содержит код ошибки
//  			  в случае успешного вызова результат содержит идентификатор
//  			  созданного звукового канала
//-----------------------------------------------------------------------------
#ifndef _USRDLL
int Squall::SampleGroup_Play3DEx(int SampleGroupID, int Loop,
	int ChannelGroupID, int Start, float* Position, float* Velocity,
	int Priority, int Volume, int Frequency, float MinDist, float MaxDist)
#else
DLL_API int SQUALL_SampleGroup_Play3DEx(int SampleGroupID, int Loop,
	int ChannelGroupID, int Start, float* Position, float* Velocity,
	int Priority, int Volume, int Frequency, float MinDist, float MaxDist)
#endif
{
	// переменные
	context_t* context = 0;
	int SampleID = 0;
	int result = true;

	// проверка наличия звука в системе
	if (api_GetNumDevice() == 0)
		result = SQUALL_ERROR_NO_SOUND;

#ifndef _USRDLL
	// получение контекста
	if (IsTrue(result)) {
		context = api_GetContext(this);
		if (!context)
			result = SQUALL_ERROR_UNINITIALIZED;
	}
#else
	context = &api._context;
#endif

	// блокирование контекста
	LockContext(context);

	// отладочная информация
#if SQUALL_DEBUG
	api_LogTime("SQUALL_SampleGroup_Play3DEx(0x%X, %d, %d, %d, 0x%X, 0x%X, %d, %d, %d, %f, %f)",
		SampleGroupID,
		Loop,
		ChannelGroupID,
		Start,
		Position,
		Velocity,
		Priority,
		Volume,
		Frequency,
		MinDist,
		MaxDist);
#endif

	// проверка параметров
	if (IsBadReadPtr(Position, sizeof(float) * 3))
		Position = 0;
	if (IsBadReadPtr(Velocity, sizeof(float) * 3))
		Velocity = 0;

	// отладочная информация
#if SQUALL_DEBUG
	// позиция звука
	if (Position) {
		api_Log("\t\t[in]Position->x = %f", Position[0]);
		api_Log("\t\t[in]Position->y = %f", Position[1]);
		api_Log("\t\t[in]Position->z = %f", Position[2]);
	}
	// скорость звука
	if (Velocity) {
		api_Log("\t\t[in]Velocity->x = %f", Velocity[0]);
		api_Log("\t\t[in]Velocity->y = %f", Velocity[1]);
		api_Log("\t\t[in]Velocity->z = %f", Velocity[2]);
	}
#endif

	// проверка параметров
	if (IsTrue(result))
		if (!Position)
			result = SQUALL_ERROR_INVALID_PARAM;

	// получим указатель на данные
	if (IsTrue(result)) {
		SampleID = api_SampleGetFileGroup(SampleGroupID, 0);
		if (SampleID == 0)
			result = SQUALL_ERROR_SAMPLE_BAD;
	}

	// создание канала
	result = CreateChannel3D(context,
			 	SampleID,
			 	Loop,
			 	ChannelGroupID,
			 	Start,
			 	Position,
			 	Velocity,
			 	Priority,
			 	Volume,
			 	Frequency,
			 	MinDist,
			 	MaxDist);

	// отладочная информация
#if SQUALL_DEBUG
	if (IsFalse(result))
		api_Log("\t\terror = %s", ErrorTable[-result]);
	else
		api_Log("\t\tresult = 0x%X", result);
#endif

	// разблокируем контекст
	UnlockContext(context);
	return result;
}
