//-----------------------------------------------------------------------------
//	Работа со звуковыми каналами
//	Копонент звукового двигателя Шквал
//	команда		: AntiTank
//	разработчик	: Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------
#ifndef _SSAMPLE_H_INCLUDED_
#define _SSAMPLE_H_INCLUDED_

// включения
#include "../Squall.h"
#include "DirectSound.h"	
#include "../SoundFile/SoundFile.h"

//-----------------------------------------------------------------------------
//	Макросы
//-----------------------------------------------------------------------------
// флаги состояния звука
enum { EMPTY = 0,   								   // канал пуст
PLAY,   										// канал воспроизводится
STOP,   										// канал остановлен
PAUSE,  										// канал в паузе
PREPARE,
// канал подготовлен для воспроизведения
};

// коды ошибок
enum { ERROR_NO_ERROR = 0,  						   // нет ошибок
ERROR_BUFFER_LOST,  							// буфер утерян
ERROR_READ_STATUS,  							// ошибка чтения статуса звука
};

// типы звуков
enum { TYPE_POSITIONED = 1, 						   // звук в заданых координатах
TYPE_AMBIENT									// рассеянный звук
};

//-----------------------------------------------------------------------------
//	Структура описывающая состояние звукового канала
//-----------------------------------------------------------------------------
struct SSoundStatus {
	unsigned STAGE : 3; 				// состояние звука смотри таблицу состояний звука
	unsigned STATUS_ERROR : 3;  			   // флаг ошибки смотри таблицу кодов ошибок
	unsigned SOUND_TYPE : 2;				 // тип звука смотри таблицу типов звуков
	unsigned SAMPLE_LOOP : 1;				   //	флаг бесконечного воспроизведения звука
	// 0 - звук играется 1 раз
	//	1 - звук воспроизводится бесконечно
	unsigned BUFFER_LOOP : 1;   			  // флаг зацикленного звуковой буфера
	// 0 - звук не зациклен
	// 1 - звук зациклен
	unsigned DOUBLE_BUFFERING : 1;  			   // флаг двойной буферизации
	// 0 - буферизация отсутствует
	// 1 - буферизация присутствует
	unsigned BUFFER_PRESENT : 1;				 //	флаг наличия буфера Direct X
	//	0 - буфера нет
	//	1 - буфер есть 
};

// структура для хранения данных канала
struct SChannelParam {
	DS3DBUFFER ds3d;				   // параметры трехмерного звука
	squall_eax_channel_t EAXBP; 				 // EAX параметры буфера
	squall_zoomfx_channel_t ZOOMFXBP;   			// ZOOMFX параметры буфера
	unsigned int Volume;				 // громкость канала
	unsigned int Pan;   				 // панорама канала
	unsigned int Frequency; 			 // чатота канала
};

//-----------------------------------------------------------------------------
//	Структура звукового канала
//-----------------------------------------------------------------------------
struct SSample {
	unsigned char _count;
	// данные DirectSound
	LPDIRECTSOUNDBUFFER DS_Buffer;  			// указатель на буфер воспроизведения
	LPKSPROPERTYSET DS_PropSet; 			// интерфейс дополнительных параметров
	LPDIRECTSOUND3DBUFFER DS3D_Buffer;  		  // указатель на интерфейс трехмерного звука
	LPKSPROPERTYSET DS3D_PropSet;   		// интерфейс дополнительных параметров 3D буфера
	int SamplesInBuffer;		// количество семплов в буфере
	int BytesInBuffer;  		// количество байт в буфере
	int SamplesPerSec;  		// частота семплов в секунду
	int OldBank;				// предедущая обновляемая половина звукового буфера
	int EndFlag;				// переменная для определения удаления звука из очереди
	int StartPtr;   			// начало фрагмента
	int EndPtr; 				// конец фрагмента
	int CurPtr; 				// текущая позиция воспроизведения
	int MasterSize; 			// настоящий размер звуковых данных в отсчетах
	int PausePtr;   			// позиция при паузе звука
	int ChannelID;  			// идентификатор звукового канала
	int GroupID;				// группа канала
	int Priority;   			// приоритет канала
	SSoundStatus Status;				 // статус звукового канала
	int SampleID;   			// идентификатор файла
	CSoundFile* SoundFile;  			// указатель на файл со звуком
	SChannelParam Param;				  // параметры канала

	// данные для обработчика
	int ChannelWorkerTime;  	// период времени через который нужно опрашивать обработчик
	int OldChannelWorkerTime;   // предедущее время когда был обработан канал
	int (*ChannelWorker)	(int, void*);   		// указатель на обработчик звукового канала
	void* UserData; 			  // данные пользователя

	// данные для обработчика конца воспроизведения
	int (*ChannelEndWorker) (int, void*);   		// указатель на обработчик звукового канала
	void* ChannelEndUserData;     // данные пользователя


	// обновление звукового канала
	int Update(float x, float y, float z);
	// обновление обработчика
	void UpdateWorker(void);
	// запуск звука
	int Play(void);
	// остановка звука
	int Stop(void);
	// установка всех трехмерных параметров буфера
	int Set3DParameters(D3DVECTOR* Position, D3DVECTOR* Velocity,
		D3DVECTOR* ConeOrientation, unsigned int InsideConeAngle,
		unsigned int OutsideConeAngle, long ConeOutsideVolume, float MinDist,
		float MaxDist, DWORD Deferred);
	// получение всех трехмерных параметров буфера
	int Get3DParameters(D3DVECTOR* Position, D3DVECTOR* Velocity,
		D3DVECTOR* ConeOrientation, int* InsideConeAngle,
		int* OutsideConeAngle, long* ConeOutsideVolume, float* MinDist,
		float* MaxDist);
	// заполнить весь звуковой буфер
	int FillSoundBuffer(char BankNum);
	// получение информции о статусе звука
	int GetStatus(void);
	// востановление утеряного звукового буфера
	int RestoreBuffer(void);
	// создание обычного звукового буфера
	int CreateSoundBuffer(LPDIRECTSOUND DirectSound, WAVEFORMATEX* format,
		DWORD Size, DWORD SamplesPerBuffer, int UseAcceleration);
	// создание трехмерного звукового буфера
	int Create3DSoundBuffer(LPDIRECTSOUND DirectSound, WAVEFORMATEX* format,
		DWORD Size, DWORD SamplesPerBuffer, DWORD Algorithm, DWORD UseEAX,
		int UseAcceleration);
	// удаление звукового буфера
	int DeleteSoundBuffer(void);
	// установка уровня громкости для канала
	int SetVolume(long Volume);
	// получение уровня громкости для канала
	int GetVolume(long* Volume);
	// установка новой скорости передачи данных
	int SetFrequency(int Frequency);
	// получение текущей скорости передачи
	int GetFrequency(int* Frequency);
	// установка новой панорамы
	int SetPan(int Pan);
	// получение текущей панорамы
	int GetPan(int* Pan);
	// установка новой позиции воспроизведения
	int SetPlayPosition(int Position);	
	// получение текущей позиции воспроизведения
	int GetPlayPosition(int* Position);
	// получение текущей позиции в локальном буфере
	int GetDXPosition(int* Position);
	// включение/выключение паузы
	int Pause(int Flag);
	// установка новых границ фрагмента
	int SetFragment(int Start, int End);
};
#endif
