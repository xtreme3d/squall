//-----------------------------------------------------------------------------
//	Абстрактный вывод звука
//	Копонент звукового двигателя Шквал
//	разработчик	: Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------

#ifndef _CABSTRACT_OUTPUT_H_INCLUDED_
#define _CABSTRACT_OUTPUT_H_INCLUDED_

#include <windows.h>

class CAbstractOutput {
protected:
	unsigned int _channels; 						// количество каналов
	//   channel_t* _channelsArray;

public:
	// конструктор деструктор
	CAbstractOutput()
	{
	};
	virtual ~CAbstractOutput()
	{
	};

	//Система
	//Функции инициализации и освобождения
	virtual int Init(HWND iWindow,  			   //  Инициализация звукового двигателя.
		unsigned int iDevice, int iRate, int iBits, int iChannels) = 0;
	virtual void Free() = 0;						//  Освобождение системных ресурсов занятых звуковым двигателем. 
	/*
	   //Функции управления звуковым двигателем
	   virtual void Pause() = 0;					   // Включение/выключение паузы воспроизведения всех звуковых каналов.
	   virtual void Stop() = 0; 					   // Остановка воспроизведения всех звуковых каналов. 
	   //Функции для настройки параметров звукового двигателя
	   virtual void SetDevice() = 0;//  Установка нового устройства воспроизведения.
	   virtual void GetDevice() = 0;//  Получение текущего устройства воспроизведения.
	   virtual void SetHardwareAcceleration() = 0;// Включение/выключение аппаратной поддержки звуковых каналов.
	   virtual void GetHardwareAcceleration () = 0;//Получение текущих настроек аппаратной поддержки звуковых каналов.
	   virtual void SetSpeakerMode() = 0;// Установка режима акустики.
	   virtual void GetSpeakerMode() = 0;// Получение текущего режима акустики.
	   virtual void Set3DAlgorithm() = 0;// Установка алгоритма обработки трехмерного звука, в режиме отсутствия аппаратной поддержки трехмерных звуковых каналов.
	   virtual void Get3DAlgorithm () = 0;//Получение текущего номера алгоритма обработки трехмерного звука, в режиме отсутствия аппаратной поддержки трехмерных звуковых каналов.
	   virtual void SetBufferSize() = 0;// Установка размера звукового канала.
	   virtual void GetBufferSize() = 0;// Получение текущего размера звукового канала.
	   virtual void SetMemoryCallbacks () = 0;//Установка внешних функций работы с памятью.
	   virtual void SetFileCallbacks() = 0;// Установка внешних функций работы с файлами. 
	*/
	//Функции для получения информации
	virtual int GetNumDevice() = 0; 				// Получение количества устройств воспроизведения в системе.
	virtual char* GetDeviceName() = 0;  			// Получение имени устройства воспроизведения.
	//   virtual void GetDeviceCaps() = 0;// Получение информации о возможностях устройства воспроизведения.
	//   virtual void GetEAXVersion() = 0;// Получение максимально доступной версии EAX.
	//   virtual void GetChannelsInfo() = 0;// Получение информации о состоянии всех звуковых каналов. 
	/*
	   // Слушатель
	   virtual void Listener_SetParameters() = 0;// Установка основных параметров слушателя.
	   virtual void Listener_GetParameters() = 0;//  Получение текущих основных параметров слушателя.
	   virtual void Listener_SetVelocity() = 0;// Установка новой скорости перемещения слушателя.
	   virtual void Listener_GetVelocity() = 0;// Получение текущей скорости перемещения слушателя.
	   virtual void Listener_SetPosition() = 0;// Установка нового положения слушателя в пространстве.
	   virtual void Listener_GetPosition () = 0;//Получение текущего положения слушателя в пространстве.
	   virtual void Listener_SetDistanceFactor() = 0;// Установка нового коэффициента преобразования дистанции.
	   virtual void Listener_GetDistanceFactor () = 0;//Получение текущего коэффициента преобразования дистанции.
	   virtual void Listener_SetRolloffFactor () = 0;//Установка нового коэффициента фактора удаления.
	   virtual void Listener_GetRolloffFactor () = 0;//Получение текущего коэффициента фактора удаления.
	   virtual void Listener_SetDopplerFactor() = 0;// Установка нового коэффициента эффекта Допплера.
	   virtual void Listener_GetDopplerFactor() = 0;// Получение текущего коэффициента эффекта Допплера.
	   virtual void Listener_Update() = 0;// Обновление трехмерных настроек слушателя.
	   virtual void Listener_EAX_SetPreset() = 0;// Установка предустановленного EAX окружения.
	   virtual void Listener_EAX_SetProperties() = 0;// Установка новых EAX параметров слушателя.
	   virtual void Listener_EAX_GetProperties() = 0;// Получение текущих EAX параметров слушателя.
	   virtual void Listener_SetWorker() = 0;// Установка обработчика слушателя. 
	   //Каналы
	   //Общие функции для работы с каналами.
	   virtual void Channel_Start() = 0;// Начало воспроизведения отложенного канала. 
	   virtual void Channel_Pause () = 0;// Включение/выключение паузы звукового канала.
	   virtual void Channel_Stop () = 0;//Остановка воспроизведения звукового канала.
	   virtual void Channel_Status () = 0;//Получение текущего состояния звукового канала.
	   virtual void Channel_SetVolume() = 0;// Установка новой громкости звукового канала.
	   virtual void Channel_GetVolume() = 0;// Получение текущей громкости звукового канала.
	   virtual void Channel_SetFrequency() = 0;// Установка новой частоты дискретизации звукового канала.
	   virtual void Channel_GetFrequency() = 0;// Получение текущей частоты дискретизации звукового канала.
	   virtual void Channel_SetPlayPosition() = 0;// Установка новой позиции воспроизведения звукового канала, в отсчетах.
	   virtual void Channel_GetPlayPosition () = 0;//Получение текущей позиции воспроизведения звукового канала, в отсчетах.
	   virtual void Channel_SetPlayPositionMs() = 0;// Установка новой позиции воспроизведения звукового канала, в миллисекундах.
	   virtual void Channel_GetPlayPositionMs() = 0;// Получение текущей позиции воспроизведения звукового канала, в миллисекундах.
	   virtual void Channel_SetWorker() = 0;// Установка обработчика звукового канала.
	   virtual void Channel_SetFragment () = 0;//Установка новых границ воспроизведения семпла, звуковым каналом, в отсчетах.
	   virtual void Channel_GetFragment () = 0;//Получение текущих границ воспроизведения семпла, звуковым каналом, в отсчетах.
	   virtual void Channel_SetFragmentMs () = 0;//Установка новых границ воспроизведения семпла, звуковым каналом, в миллисекундах.
	   virtual void Channel_GetFragmentMs () = 0;//Получение текущих границ воспроизведения семпла, звуковым каналом, в миллисекундах.
	   virtual void Channel_GetLength () = 0;//Получение исходной продолжительности семпла, звукового канала, в отсчетах.
	   virtual void Channel_GetLengthMs() = 0;// Получение исходной продолжительности семпла, звукового канала, в миллисекундах.
	   virtual void Channel_SetPriority () = 0;//Установка нового приоритета звукового канала.
	   virtual void Channel_GetPriority() = 0;// Получение текущего приоритета звукового канала.
	   virtual void Channel_SetLoop() = 0;// Установка цикличности воспроизведения звукового канала.
	   virtual void Channel_GetLoop() = 0;// Получение текущей цикличности воспроизведения звукового канала. 
	   //Функции работы с рассеянными каналами
	   virtual void Channel_SetPan() = 0;// Установка новой панорамы звукового канала.
	   virtual void Channel_GetPan () = 0;// Получение текущей панорамы звукового канала. 
	   //Функции работы с пространственными (трехмерными) каналами
	   virtual void Channel_Set3DPosition() = 0;// Установка новой позиции звукового канала в пространстве.
	   virtual void Channel_Get3DPosition() = 0;//  Получение текущей позиции звукового канала в пространстве.
	   virtual void Channel_SetVelocity() = 0;// Установка новой скорости перемещения звукового канала в пространстве.
	   virtual void Channel_GetVelocity() = 0;// Получение текущей скорости перемещения звукового канала в пространстве.
	   virtual void Channel_SetMinMaxDistance() = 0;// Установка новых значений минимального и максимального расстояния слышимости звукового канала.
	   virtual void Channel_GetMinMaxDistance () = 0;//Получение текущих значений минимального и максимального расстояния слышимости звукового канала.
	   virtual void Channel_SetConeParameters() = 0;// Установка новых параметров конуса распространения звукового канала.
	   virtual void Channel_GetConeParameters () = 0;//Получение текущих параметров конуса распространения звукового канала.
	   virtual void Channel_EAX_SetProperties () = 0;//Установка новых EAX параметров звукового канала.
	   virtual void Channel_EAX_GetProperties () = 0;//Получение текущих EAX параметров звукового канала.
	   virtual void Channel_ZOOMFX_SetProperties() = 0;// Установка новых ZOOMFX параметров звукового канала.
	   virtual void Channel_ZOOMFX_GetProperties() = 0;// Получение текущих ZOOMFX параметров звукового канала. 
	   /*
	   //Группы каналов
	   ChannelGroup_Pause () = 0;//Включение/выключение воспроизведения группы звуковых каналов.
	   ChannelGroup_Stop () = 0;// Остановка воспроизведения группы звуковых каналов.
	   ChannelGroup_SetVolume() = 0;// Установка новой громкости группе звуковых каналов.
	   ChannelGroup_SetFrequency() = 0;// Установка новой частоты дискретизации группе звуковых каналов. 
	   //Семплы
	   SQUALL_Sample_LoadFile () = 0;//Создание семпла из файла.
	   SQUALL_Sample_UnloadAll () = 0;// Выгрузка всех семплов из звукового двигателя.
	   SQUALL_Sample_Unload() = 0;// Выгрузка указанного семпла из звукового двигателя.
	   SQUALL_Sample_GetFileLength () = 0;//Получение исходной продолжительности семпла, в отсчетах.
	   SQUALL_Sample_GetFileLengthMs() = 0;// Получение исходной продолжительности семпла, в миллисекундах.
	   SQUALL_Sample_GetFileFrequency() = 0;// Получение исходной частоты дискретизации семпла.
	   SQUALL_Sample_SetDefault() = 0;// Установка новых параметров семпла по умолчанию.
	   SQUALL_Sample_GetDefault() = 0;// Получение текущих параметров семпла по умолчанию.
	   SQUALL_Sample_Play () = 0;//Создание рассеянного звукового канала из указанного семпла, опираясь на параметры семпла по умолчанию.
	   SQUALL_Sample_PlayEx() = 0;// Создание рассеянного звукового канала из указанного семпла.
	   SQUALL_Sample_Play3D () = 0;//Создание пространственного (трехмерного) звукового канала из указанного семпла, опираясь на параметры семпла по умолчанию.
	   SQUALL_Sample_Play3DEx () = 0;//Создание пространственного (трехмерного) звукового канала из указанного семпла.
	   SQUALL_Sample_Pause () = 0;//Включение/выключение воспроизведения всех звуковых каналов использующих указанный семпл.
	   SQUALL_Sample_Stop() = 0;// Остановка всех звуковых каналов использующих указанный семпл. 
	   //Группы семплов
	   SQUALL_SampleGroup_Play() = 0;// Создание рассеянного звукового канала из группы семплов, опираясь на параметры семпла по умолчанию.
	   SQUALL_SampleGroup_PlayEx () = 0;// Создание рассеянного звукового канала из группы семплов.
	   SQUALL_SampleGroup_Play3D() = 0;// Создание пространственного (трехмерного) звукового канала из группы семплов, опираясь на параметры семпла по умолчанию.
	   SQUALL_SampleGroup_Play3DEx () = 0;//Создание пространственного (трехмерного) звукового канала из группы семплов. 
	   */
};
#endif