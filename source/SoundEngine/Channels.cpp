//-----------------------------------------------------------------------------
//	Работа со звуковыми каналами
//	Копонент звукового двигателя Шквал
//	команда		: AntiTank
//	разработчик	: Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------

// включения
#include <stdio.h>
#include <math.h>
#include <dsound.h>
#include "../SoundFile/SoundFile.h"
#include "Channels.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//				Процедуры для работы со звуковыми каналами
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
// Создание вторичного звукового буфера для рассеянного звука
//	на входе	:	DirectSound	-	указатель на объект DirectSound
//					format		-	указатель на характеристики создаваемого буфера
//					size		-	размер звуковых данных
//	на выходе	:	успешность создания вторичного звукового буфера
//	примечание	:	1.	Процедура ВСЕГДА создает СТЕРЕОФОНИЧЕСКИЙ звуковой буфер
//					2.	Процедура определяет размер данных, если звуковые данные
//						по продолжительности менее 2х секунд, то процедура создает
//						буфер равный по размеры данных, иначе создает звуковой 
//						буфер с докачкой.
//-----------------------------------------------------------------------------
int SSample::CreateSoundBuffer(LPDIRECTSOUND DirectSound,
	WAVEFORMATEX* format, DWORD size, DWORD BufferSize, int UseAcceleration)
{
	// объявление переменных
	DSBUFFERDESC dsbd;
	WAVEFORMATEX wfx;
	DWORD SoundSize;

	// проверка наличия объекта воспроизведения
	if (DirectSound) {
		// скопируем формат
		memcpy(&wfx, format, sizeof(WAVEFORMATEX));
		wfx.nChannels = 2;						// всегда стерео
		wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) >> 3;
		wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
		wfx.cbSize = 0;

		BufferSize = (wfx.nSamplesPerSec * BufferSize) / 1000;
		// вычисление размера секундного звукового буфера
		SoundSize = (BufferSize * wfx.nBlockAlign) << 1;

		// занесение данных вторичного буфера
		ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
		dsbd.dwSize = sizeof(DSBUFFERDESC);
		dsbd.dwFlags = DSBCAPS_CTRLPAN |
			DSBCAPS_CTRLFREQUENCY |
			DSBCAPS_CTRLVOLUME |
			DSBCAPS_GLOBALFOCUS;
		dsbd.lpwfxFormat = &wfx;

		// флаг использования аппаратной акселерации
		if (!UseAcceleration) {
			dsbd.dwFlags |= DSBCAPS_LOCSOFTWARE;
		}

		// звук менее чем звуковой буфер ?
		if ((BufferSize << 1) > size) {
			// вычислим размер трека
			SoundSize = size * wfx.nBlockAlign;
			// создадим статический буфер
			Status.DOUBLE_BUFFERING = 0;
			Status.BUFFER_LOOP = Status.SAMPLE_LOOP;
			dsbd.dwFlags |= DSBCAPS_STATIC;
		} else {
			// создадим потоковый буфер
			Status.DOUBLE_BUFFERING = 1;
			Status.BUFFER_LOOP = 1;
		}
		// размер звукового буфера
		dsbd.dwBufferBytes = SoundSize;

		// Создание вторичного буфера
		if (DirectSound->CreateSoundBuffer(&dsbd, & DS_Buffer, NULL) != DS_OK)
			return false;

		// количество байт в буфере
		BytesInBuffer = SoundSize;

		// количество семплов в буфере
		SamplesInBuffer = SoundSize / wfx.nBlockAlign;

		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
//	Создание вторичного звукового буфера для позиционного звука
//	на входе	:	DirectSound	-	указатель на объект DirectSound
//					format		-	указатель на характеристики создаваемого буфера
//					size		-	размер звуковых данных
//	на выходе	:	успешность создания вторичного звукового буфера
//	примечание	:	1.	Процедура ВСЕГДА создает МОНОФОНИЧЕСКИЙ звуковой буфер
//					2.	Процедура определяет размер данных, если звуковые данные
//						по продолжительности менее 2х секунд, то процедура создает
//						буфер равный по размеры данных, иначе создает звуковой 
//						буфер с докачкой.
//-----------------------------------------------------------------------------
int SSample::Create3DSoundBuffer(LPDIRECTSOUND DirectSound,
	WAVEFORMATEX* format, DWORD size, DWORD BufferSize, DWORD Algorithm,
	DWORD UseEAX, int UseAcceleration)
{
	// объявление переменных
	DSBUFFERDESC dsbd;
	WAVEFORMATEX wfx;
	DWORD SoundSize;

	// проверка наличия объекта воспроизведения
	if (DirectSound) {
		// скопируем формат
		memcpy(&wfx, format, sizeof(WAVEFORMATEX));
		wfx.nChannels = 1;						// всегда моно
		wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) >> 3;
		wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
		wfx.cbSize = 0;

		// вычисление размера буфера в семплах
		BufferSize = (wfx.nSamplesPerSec * BufferSize) / 1000;
		// вычисление размера буфера в байтах
		SoundSize = (BufferSize * wfx.nBlockAlign) << 1;

		// подготовка к занесению данных для вторичного буфера
		ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
		dsbd.dwSize = sizeof(DSBUFFERDESC);
		dsbd.dwFlags = DSBCAPS_CTRLFREQUENCY |
			DSBCAPS_CTRLVOLUME |
			DSBCAPS_GLOBALFOCUS |
			DSBCAPS_CTRL3D |
			DSBCAPS_MUTE3DATMAXDISTANCE;
		dsbd.lpwfxFormat = &wfx;

		// использование аппаратной акселерации
		if (!UseAcceleration) {
			dsbd.dwFlags |= DSBCAPS_LOCSOFTWARE;
		}

		// звук менее чем звуковой буфер ?
		if ((BufferSize << 1) > size) {
			// вычислим размер трека
			SoundSize = size * wfx.nBlockAlign;
			// создаем статичный буфер
			Status.DOUBLE_BUFFERING = 0;
			Status.BUFFER_LOOP = Status.SAMPLE_LOOP;
			dsbd.dwFlags |= DSBCAPS_STATIC;
		} else {
			// создаем потоковый буфер
			Status.DOUBLE_BUFFERING = 1;
			Status.BUFFER_LOOP = 1;
		}
		// размер звукового буфера
		dsbd.dwBufferBytes = SoundSize;

		// подбор способа общета трехмерного звука
		switch (Algorithm) {
		case 0x1:
			dsbd.guid3DAlgorithm = DS3DALG_NO_VIRTUALIZATION;
			break;

		case 0x2:
			dsbd.guid3DAlgorithm = DS3DALG_HRTF_FULL;
			break;

		case 0x3:
			dsbd.guid3DAlgorithm = DS3DALG_HRTF_LIGHT;
			break;

		default:
			dsbd.guid3DAlgorithm = DS3DALG_DEFAULT;
			break;
		}

		// Создание вторичного буфера
		if (DirectSound->CreateSoundBuffer(&dsbd, & DS_Buffer, NULL) != DS_OK)
			return false;

		// Создание интерфейса для трехмерного звука
		if (DS_Buffer->QueryInterface(IID_IDirectSound3DBuffer,
					   	(VOID * *) &DS3D_Buffer) != DS_OK)
			return false;

		// получим дополнительный интерфейс трехмерного буфера
		DS_PropSet = 0;
		if (DS_Buffer->QueryInterface(IID_IKsPropertySet,
					   	(void * *) &DS_PropSet) != DS_OK)
			DS_PropSet = 0;

		// получим дополнительный интерфейс трехмерного буфера
		DS3D_PropSet = 0;
		if (UseEAX) {
			if (DS3D_Buffer->QueryInterface(IID_IKsPropertySet,
							 	(void * *) &DS3D_PropSet) != DS_OK)
				DS3D_PropSet = 0;
		}

		Param.ds3d.dwSize = sizeof(DS3DBUFFER);
		if (DS3D_Buffer->GetAllParameters(&Param.ds3d) != DS_OK)
			return false;

		// количество байт в буфере
		BytesInBuffer = SoundSize;

		// количество семплов в буфере
		SamplesInBuffer = SoundSize / wfx.nBlockAlign;

		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
//	Деинициализация вторичного звукового буфера
//	на входе	:	*
//	на выходе	:	успешность удаления звукового буфера
//-----------------------------------------------------------------------------
int SSample::DeleteSoundBuffer(void)
{
	// освобождение дополнительного интерфейса трехмерного буфера
	if (DS3D_PropSet) {
		DS3D_PropSet->Release();
		DS3D_PropSet = 0;
	}

	// освобождение интерфейса трехмерного звука
	if (DS3D_Buffer) {
		DS3D_Buffer->Release();
		DS3D_Buffer = 0;
	}

	// освобождение дополнительного интерфейса
	if (DS_PropSet) {
		DS_PropSet->Release();
		DS_PropSet = 0;
	}

	// освободим буфер
	if (DS_Buffer) {
		DS_Buffer->Release();
		DS_Buffer = 0;
	}

	// пометим буфер как пустой
	Status.STAGE = EMPTY;
	return true;
}

//-----------------------------------------------------------------------------
//	Заполнение звукового буфера
//	на входе	:	half	- номер заполняемой половины
//	на выходе	:	успешность заполнения звуковой половины
//-----------------------------------------------------------------------------
int SSample::FillSoundBuffer(char half)
{
	// обьявление переменных
	bool ok;
	DWORD cur_size, fill_size, block_size, lock_len, write_pos, write_len, read;
	void* lock_ptr;

	// проверка наличия звукового буфера
	ok = (DS_Buffer) ? true : false;
	if (ok) {
		// определим размер оставшихся данных
		cur_size = EndPtr - CurPtr;
		// если звук закончался удалим его
		if (!cur_size && !Status.SAMPLE_LOOP) {
			// остановим звук
			if (EndFlag == 1) {
				Stop();
				return true;
			}
			// установка флага, в следующее обновление удалить буфер
			EndFlag = 1;
		}

		// вычисление данных блокирования буфера звука
		write_pos = 0;
		write_len = BytesInBuffer;
		fill_size = SamplesInBuffer;

		// звук с двойной буферизацией ?
		if (Status.DOUBLE_BUFFERING) {
			write_len >>= 1;
			fill_size >>= 1;
			if (half)
				write_pos = write_len;
		} 

		// блокирование звукового буфера
		ok = (DS_Buffer->Lock(write_pos,
						 	write_len,
						 	& lock_ptr,
						 	& lock_len,
						 	NULL,
						 	NULL,
						 	0) == DS_OK) ?
			true :
			false;
		if (ok) {
			// подкачка буфера данными
			char* ptr = (char*) lock_ptr;

			if (Status.SOUND_TYPE == TYPE_AMBIENT) {
				do {
					// размер оставшихся данных
					cur_size = EndPtr - CurPtr;
					// вычисление количества заполнения
					block_size = (cur_size < fill_size) ?
						cur_size :
						fill_size;

					// данные еще есть ?
					if (cur_size) {
						// заполним данными 
						read = SoundFile->GetStereoData(ptr,
										  	CurPtr,
										  	block_size,
										  	Status.SAMPLE_LOOP);
						if (read) {
							ptr += read;
							CurPtr += block_size;
							fill_size -= block_size;
						} else {
							ok = false;
							break;
						}
					} else {
						// звук зациклен ?
						if (Status.SAMPLE_LOOP) {
							CurPtr = StartPtr;
						} else {
							// заполнение буфера тишиной
							read = SoundFile->GetStereoMute(ptr, fill_size);
							if (read) {
								ptr += read;
								fill_size = 0;
							} else {
								ok = false;
								break;
							}
						}
					}
				} while (fill_size);
			} else {
				do {
					// размер оставшихся данных
					cur_size = EndPtr - CurPtr;
					// вычисление количества заполнения
					block_size = (cur_size < fill_size) ?
						cur_size :
						fill_size;

					// данные еще есть ?
					if (cur_size) {
						// заполним данными 
						read = SoundFile->GetMonoData(ptr,
										  	CurPtr,
										  	block_size,
										  	Status.SAMPLE_LOOP);
						if (read) {
							ptr += read;
							CurPtr += block_size;
							fill_size -= block_size;
						} else {
							ok = false;
							break;
						}
					} else {
						// звук зациклен ?
						if (Status.SAMPLE_LOOP) {
							CurPtr = StartPtr;
						} else {
							// заполнение буфера тишиной
							read = SoundFile->GetMonoMute(ptr, fill_size);
							if (read) {
								ptr += read;
								fill_size = 0;
							} else {
								ok = false;
								break;
							}
						}
					}
				} while (fill_size);
			}
		}
		if (ok) {
			// разблокирование звукового буфера
			ok = (DS_Buffer->Unlock(lock_ptr, lock_len, NULL, NULL) == DS_OK) ?
				true :
				false;
		}
	}
	// вернем результат работы
	return ok;
}

//-----------------------------------------------------------------------------
//	Установка координат звука в пространстве
//	на входе	:	указатель на структуру с координатами звука
//	на выходе	:	успешность установки координат звука
//-----------------------------------------------------------------------------
int SSample::Set3DParameters(D3DVECTOR* Position, D3DVECTOR* Velocity,
	D3DVECTOR* ConeOrientation, unsigned int InsideConeAngle,
	unsigned int OutsideConeAngle, long ConeOutsideVolume, float MinDist,
	float MaxDist, DWORD Deferred)
{
	bool change = false;
	// защита от дурака
	if (!DS3D_Buffer)
		return false;

	// проверка наличия параметра
	if (Position) {
		// проверка изменений параметра
		if ((Param.ds3d.vPosition.x != Position->x) ||
			(Param.ds3d.vPosition.y != Position->y) ||
			(Param.ds3d.vPosition.z != Position->z)) {
			// запишем новое значение
			Param.ds3d.vPosition.x = Position->x;
			Param.ds3d.vPosition.y = Position->y;
			Param.ds3d.vPosition.z = Position->z;
			change = true;
		}
	}

	// проверка наличия параметра
	if (Velocity) {
		// проверка изменений параметра
		if ((Param.ds3d.vVelocity.x != Velocity->x) ||
			(Param.ds3d.vVelocity.y != Velocity->y) ||
			(Param.ds3d.vVelocity.z != Velocity->z)) {
			// запишем новое значение
			Param.ds3d.vVelocity.x = Velocity->x;
			Param.ds3d.vVelocity.y = Velocity->y;
			Param.ds3d.vVelocity.z = Velocity->z;
			change = true;
		}
	}

	// проверка наличия параметра
	if (ConeOrientation) {
		// проверка изменений параметра
		if ((Param.ds3d.vConeOrientation.x != ConeOrientation->x) ||
			(Param.ds3d.vConeOrientation.y != ConeOrientation->y) ||
			(Param.ds3d.vConeOrientation.z != ConeOrientation->z)) {
			// запишем новое значение
			Param.ds3d.vConeOrientation.x = ConeOrientation->x;
			Param.ds3d.vConeOrientation.y = ConeOrientation->y;
			Param.ds3d.vConeOrientation.z = ConeOrientation->z;
			change = true;
		}
	}

	// проверка наличия параметра
	if (InsideConeAngle) {
		// проверка изменений параметра
		if (Param.ds3d.dwInsideConeAngle != InsideConeAngle) {
			// запишем новое значение
			Param.ds3d.dwInsideConeAngle = InsideConeAngle;
			change = true;
		}
	}

	// проверка наличия параметра
	if (OutsideConeAngle) {
		// проверка изменений параметра
		if (Param.ds3d.dwOutsideConeAngle != OutsideConeAngle) {
			// запишем новое значение
			Param.ds3d.dwOutsideConeAngle = OutsideConeAngle;
			change = true;
		}
	}

	// проверка наличия параметра
	if (ConeOutsideVolume) {
		// проверка изменений параметра
		if (Param.ds3d.lConeOutsideVolume != ConeOutsideVolume) {
			// запишем новое значение
			Param.ds3d.lConeOutsideVolume = ConeOutsideVolume;
			change = true;
		}
	}

	// проверка наличия параметра
	if (MinDist) {
		// проверка изменений параметра
		if (Param.ds3d.flMinDistance != MinDist) {
			// запишем новое значение
			Param.ds3d.flMinDistance = MinDist;
			change = true;
		}
	}

	// проверка наличия параметра
	if (MaxDist) {
		// проверка изменений параметра
		if (Param.ds3d.flMaxDistance != MaxDist) {
			// запишем новое значение
			Param.ds3d.flMaxDistance = MaxDist;
			change = true;
		}
	}

	// проверка наличия изменений
	if (change) {
		// установка координат звука в пространстве
		if (DS3D_Buffer->SetAllParameters(&Param.ds3d, Deferred) != DS_OK)
			return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
//	Получение координат звука в пространстве
//	на входе	:	указатель на структуру в которую поместить координаты звука
//	на выходе	:	успешность получения координат звука
//-----------------------------------------------------------------------------
int SSample::Get3DParameters(D3DVECTOR* Position, D3DVECTOR* Velocity,
	D3DVECTOR* ConeOrientation, int* InsideConeAngle, int* OutsideConeAngle,
	long* ConeOutsideVolume, float* MinDist, float* MaxDist)
{
	// защита от дурака
	if (!DS3D_Buffer)
		return false;

	// получение координат источника звука
	if (Position) {
		Position->x = Param.ds3d.vPosition.x;
		Position->y = Param.ds3d.vPosition.y;
		Position->z = Param.ds3d.vPosition.z;
	}

	// получение скорости источника звука
	if (Velocity) {
		Velocity->x = Param.ds3d.vVelocity.x;
		Velocity->y = Param.ds3d.vVelocity.y;
		Velocity->z = Param.ds3d.vVelocity.z;
	}

	// получения направления источника звука
	if (ConeOrientation) {
		ConeOrientation->x = Param.ds3d.vConeOrientation.x;
		ConeOrientation->y = Param.ds3d.vConeOrientation.y;
		ConeOrientation->z = Param.ds3d.vConeOrientation.z;
	}

	// получения угла внутреннего конуса
	if (InsideConeAngle)
		*InsideConeAngle = Param.ds3d.dwInsideConeAngle;

	// получения угла внешнего конуса
	if (OutsideConeAngle)
		*OutsideConeAngle = Param.ds3d.dwOutsideConeAngle;

	// получение соотношения громкости за пределами внешнего конуса
	if (ConeOutsideVolume)
		*ConeOutsideVolume = Param.ds3d.lConeOutsideVolume;

	// получение минимальной дистанции слышимости
	if (MinDist)
		*MinDist = Param.ds3d.flMinDistance;

	// получение максимальной дистанции слышимости
	if (MaxDist)
		*MaxDist = Param.ds3d.flMaxDistance;

	return true;
}

//-----------------------------------------------------------------------------
//	Получение статуса звукового канала
//	на входе	:	*
//	на выходе	:	*
//	примечание	:	Процедура заполняет структуру состояния канала
//-----------------------------------------------------------------------------
int SSample::GetStatus(void)
{
	// установка переменных
	bool Status_flag = false;
	DWORD Temp_Status = 0;

	// очистим данные статуса
	Status.STATUS_ERROR = 0;
	Status.BUFFER_PRESENT = 0;

	// проверка наличия звукового буфера
	if (!DS_Buffer) {
		Status.STATUS_ERROR = 1;
		return false;
	}

	// статус получен ?
	while (!Status_flag) {
		// получение статуса
		if (DS_Buffer->GetStatus(&Temp_Status) != DS_OK) {
			Status.STATUS_ERROR = 1;
			return false;
		}

		// буфер потерян ?
		if (Temp_Status & DSBSTATUS_BUFFERLOST) {
			// востановление звукового буфера
			while (DS_Buffer->Restore() == DSERR_BUFFERLOST) {
				Sleep(10);
			}

			// востановим звуковые данные буфера
		} else
			Status_flag = true;
	}
	// звуковой буфер есть
	Status.BUFFER_PRESENT = 1;

	// буфер воспроизводится ?
	if ((Status.STAGE != PAUSE) && (Status.STAGE != PREPARE)) {
		if (Temp_Status & DSBSTATUS_PLAYING)
			Status.STAGE = PLAY;
		else
			Status.STAGE = STOP;
	}
	return true;
}

//-----------------------------------------------------------------------------
//	Обновление звука
//	на входе	   :	x
//  			  y
//  			  z
//	на выходе   :	*
//	примечание	:	Данная процедура вызываеться из обработчика звуковых каналов
//-----------------------------------------------------------------------------
int SSample::Update(float x, float y, float z)
{
	int ok;
	float direct;
	int Position;
	int Bank;
	// получение статуса семпла
	if (!GetStatus())
		return false;

	// если буфер простаивает
	ok = true;
	if (Status.STAGE == STOP) {
		DeleteSoundBuffer();
		return ok;
	}

	// проверка наличия буфера
	if (Status.BUFFER_PRESENT) {
		// канал с двойной буферизацией ?
		if (Status.DOUBLE_BUFFERING) {
			// канал воспроизводиться ?
			if (Status.STAGE == PLAY) {
				ok = GetDXPosition(&Position);
				if (ok) {
					Bank = (Position > BytesInBuffer / 2) ? 0 : 1;
					if (OldBank != Bank) {
						OldBank = Bank;
						ok = FillSoundBuffer(Bank);
					}
				}
			}
		}
	}

	if (DS3D_Buffer) {
		// вычисление растояния до источника звука
		direct = sqrtf(powf((x - Param.ds3d.vPosition.x), 2) +
				 	powf((y - Param.ds3d.vPosition.y), 2) +
				 	powf((z - Param.ds3d.vPosition.z), 2));

		// в случае если растояние до источника больше чем максимальное растояние установким громкость в 0
		if (direct > Param.ds3d.flMaxDistance)
			DS_Buffer->SetVolume(-10000);
		else
			DS_Buffer->SetVolume(Param.Volume);
	}
	return ok;
}

//-----------------------------------------------------------------------------
//	Обновление обработчика
//	на входе	:	*
//	на выходе	:	*
//	примечание	:	Данная процедура вызываеться из обработчика звуковых каналов
//-----------------------------------------------------------------------------
void SSample::UpdateWorker(void)
{
	int CurTime;
	CurTime = timeGetTime();
	// период прошел ?
	if ((CurTime - OldChannelWorkerTime) > ChannelWorkerTime) {
		OldChannelWorkerTime = CurTime;
		// вызов обработчика канала
		if (ChannelWorker) {
			if (!ChannelWorker(ChannelID, UserData)) {
				DeleteSoundBuffer();
			}
		}
	}
}

//-----------------------------------------------------------------------------
//	Начало воспроизведения звукового канала
//	на входе	:	*
//	на выходе	:	успешность запуска воспроизведения
//-----------------------------------------------------------------------------
int SSample::Play(void)
{
	if (DS_Buffer) {
		// проверка наличия буфера
		if (DS_Buffer->SetCurrentPosition(0) == DS_OK) {
			if (DS_Buffer->Play(0, 0, Status.BUFFER_LOOP) == DS_OK) {
				Status.STAGE = PLAY;
				return true;
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
//	Остановка воспроизведения звука
//	на входе	:	*
//	на выходе	:	успешность остановки звукового канала
//-----------------------------------------------------------------------------
int SSample::Stop(void)
{
	if (DS_Buffer) {
		// проверка наличия буфера
		DS_Buffer->SetVolume(-10000);

		if (DS_Buffer->Stop() == DS_OK) {
			// вызов обработчика окончания воспроизведения
			if (ChannelEndWorker) {
				ChannelEndWorker(ChannelID, ChannelEndUserData);
			}

			Status.STAGE = STOP;
			ChannelID = 0;
			return true;
		}
	}


	return false;
}

//-----------------------------------------------------------------------------
//	Установка уровня громкости канала
//	на входе	:	volume	- значение устанавливаемого уровня громкости
//	на выходе	:	успешность установки уровня громкости
//-----------------------------------------------------------------------------
int SSample::SetVolume(long volume)
{
	if (DS_Buffer) {
		// проверка наличия буфера
		Param.Volume = volume;
		if (DS_Buffer->SetVolume(volume) == DS_OK)
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
//	Получение уровня громкости канала
//	на входе	:	volume	- указатель на переменную в которую поместить
//					значение уровня громкости
//	на выходе	:	успешность получения уровня громкости
//-----------------------------------------------------------------------------
int SSample::GetVolume(long* volume)
{
	if (DS_Buffer) {
		// проверка наличия буфера
		if (DS_Buffer->GetVolume(volume) == DS_OK)
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
//	Установка новой частоты передачи данных
//	на входе	:	Frequency	- новая частота
//	на выходе	:	успешность установки новой частоты передачи данных
//-----------------------------------------------------------------------------
int SSample::SetFrequency(int Frequency)
{
	if (DS_Buffer) {
		// защита от дурака
		Param.Frequency = Frequency;
		if (DS_Buffer->SetFrequency(Frequency) == DS_OK)
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
//	Получение текущей частоты передачи данных
//	на входе	:	Frequency	- указатель на переменную в которую нужно
//								  поместить текущее значение передачи данных
//	на выходе	:	успешность получения текущей частоты
//-----------------------------------------------------------------------------
int SSample::GetFrequency(int* Frequency)
{
	DWORD fr;
	if (DS_Buffer) {
		// защита от дурака
		if (DS_Buffer->GetFrequency(&fr) == DS_OK) {
			*Frequency = fr;
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
//	Установка панорамы звука
//	на входе	:	Pan		- новое значение панорамы
//	на выходе	:	успешность установки панорамы
//-----------------------------------------------------------------------------
int SSample::SetPan(int Pan)
{
	if (DS_Buffer) {
		// защита от дурака
		// ограничитель
		if (Pan > 100)
			Pan = 100;
		Param.Pan = Pan;
		if (DS_Buffer->SetPan((Pan * 200) - 10000) == DS_OK)
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
//	Получение текущей панорамы звука
//	на входе	:	Pan		- указатель на переменную в которую нужно поместить
//							  текущее значение панорамы звука
//	на выходе	:	успешность получения текущей панорамы
//-----------------------------------------------------------------------------
int SSample::GetPan(int* Pan)
{
	long temp = 0;
	if (DS_Buffer) {
		if (DS_Buffer->GetPan(&temp) == DS_OK) {
			*Pan = (temp + 10000) / 200;
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
//	Установка новой позиции воспроизведения звука
//	на входе	:	Position	- новое значение позиции воспроизведения звука
//	на выходе	:	успешность установки новой позиции воспроизведения звука
//-----------------------------------------------------------------------------
int SSample::SetPlayPosition(int Position)
{
	int ret;
	ret = (DS_Buffer) ? true : false;
	if (ret) {
		// звук воспроизводится ?
		if (Status.STAGE == PLAY) {
			ret = (DS_Buffer->Stop() == DS_OK) ? true : false;
			if (ret)
				ret = (DS_Buffer->SetCurrentPosition(0) == DS_OK) ?
					true :
					false;
		} else {
			if (Status.STAGE == PAUSE)
				PausePtr = 0;
		}

		if (ret) {
			CurPtr = StartPtr + Position;
			ret = FillSoundBuffer(0);
			OldBank = 0;
		}

		if (ret) {
			if (Status.STAGE == PLAY) {
				ret = (DS_Buffer->Play(0, 0, Status.BUFFER_LOOP) == DS_OK) ?
					true :
					false;
			}
		}
	}
	return ret;
}

//-----------------------------------------------------------------------------
//	Получение текущей позиции воспроизведения звука
//	на входе	:	Position	- указатель на переменную в которую нужно
//								  поместить текущее значение позиции
//								  воспроизведения звука
//	на выходе	:	успешность получения текущей позиции воспроизведения
//-----------------------------------------------------------------------------
int SSample::GetPlayPosition(int* Position)
{
	DWORD DXPosition;
	if (DS_Buffer) {
		if (Status.STAGE == PLAY) {
			if (DS_Buffer->GetCurrentPosition(&DXPosition, NULL) != DS_OK)
				return false;
		} else {
			if (Status.STAGE == PAUSE)
				DXPosition = PausePtr;
		}
		if (Status.DOUBLE_BUFFERING) {
			*Position = (CurPtr / SamplesInBuffer) * SamplesInBuffer +
				DXPosition /
				(BytesInBuffer / SamplesInBuffer);
			if (*Position >= MasterSize)
				*Position %= MasterSize;
		} else {
			*Position = DXPosition / (BytesInBuffer / SamplesInBuffer);
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
//	Получение текущей позиции воспроизведения звука
//	на входе	:	Position	- указатель на переменную в которую нужно
//								  поместить текущее значение позиции
//								  воспроизведения звука
//	на выходе	:	успешность получения текущей позиции воспроизведения
//-----------------------------------------------------------------------------
int SSample::GetDXPosition(int* Position)
{
	if (DS_Buffer) {
		if (DS_Buffer->GetCurrentPosition((unsigned long *) Position, NULL) ==
			DS_OK)
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
//	Включение/выключение паузы канала
//	на входе	:	Flag	- флаг включения/выключения воспроизведения
//	на выходе	:	успешность установки состояния паузы
//-----------------------------------------------------------------------------
int SSample::Pause(int Flag)
{
	if (DS_Buffer) {
		if (Flag) {
			// включение паузы
			if (Status.STAGE == PLAY) {
				if (DS_Buffer->GetCurrentPosition((unsigned long *) &PausePtr,
							   	NULL) == DS_OK) {
					if (DS_Buffer->Stop() == DS_OK) {
						Status.STAGE = PAUSE;
						return true;
					}
				}
			}
		} else {
			// выключение паузы
			if (Status.STAGE == PAUSE) {
				if (DS_Buffer->SetCurrentPosition(PausePtr) == DS_OK) {
					if (DS_Buffer->Play(0, 0, Status.BUFFER_LOOP) == DS_OK) {
						Status.STAGE = PLAY;
						return true;
					}
				}
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
//	Установка границ фрагмента
//	на входе	:	Start	- начальная позиция
//					End		- конечная позиция
//	на выходе	:	успешность установки
//-----------------------------------------------------------------------------
int SSample::SetFragment(int Start, int End)
{
	int ret;
	ret = (DS_Buffer) ? true : false;
	if (ret) {
		// проверка выхода за пределы
		if (Start > MasterSize)
			Start = MasterSize;
		if (End > MasterSize)
			End = MasterSize;

		// если фрагмент равен 0
		ret = (Start != End) ? true : false;
		if (ret) {
			// проверка параметров
			if (Start > End) {
				// поменяем параметры местами
				StartPtr = End;
				EndPtr = Start;
			} else {
				StartPtr = Start;
				EndPtr = End;
			}
			// проверка выхода текущего указателя за пределы нового фрагмента
			if ((CurPtr <= StartPtr) || (CurPtr >= EndPtr))
				ret = SetPlayPosition(0);
		}
	}
	return ret;
}





























