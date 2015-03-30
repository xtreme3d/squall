//-----------------------------------------------------------------------------
//	Класс для работы со звуковыми файлами
//	Копонент звукового двигателя Шквал
//	команда		: AntiTank
//	разработчик	: Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------

#include "../SoundEngine/SquallApi.h"
#include "SoundFile.h"
#include "Reader.h"

// форматы файлов
#include "WAV/WavFile.h"
#include "MP3/MpegFile.h"
#include "Ogg/OggFile.h"
//#include "Wma/WmaFile.h"
//#include "TrackFile.h"

// декодеры звуковых потоков
#include "WAV/PcmDecoder.h"
#include "WAV/ImaAdpcmDecoder.h"
#include "WAV/MsAdpcmDecoder.h"
#include "MP3/MpegDecoder.h"
#include "Ogg/OggDecoder.h"
//#include "WmaDecoder.h"
//#include "TrackDecoder.h"

//-----------------------------------------------------------------------------
//	Конструктор класса
//	на входе	:	*
//	на выходе	:	*
//-----------------------------------------------------------------------------
CSoundFile::CSoundFile()
{
	memset(&pcm_format, 0, sizeof(WAVEFORMATEX));
	class_id = 0x53536746;  					 // идентификатор данных
	decoder = 0;								// декодера нет
	sound_data = 0;									// обработчика данных нет
	reader = 0;									// обработчик данных звукового файла
	_ref = 0;   							 // количество ссылок
}

//-----------------------------------------------------------------------------
//	Деструктор класса
//	на входе	:	*
//	на выходе	:	*
//-----------------------------------------------------------------------------
CSoundFile::~CSoundFile()
{
	// защита от дурака
	if (this) {
		DeInit();
		class_id = 0;
	}
}

//-----------------------------------------------------------------------------
// Добавление ссылки
// на входе    :  *
// на выходе   :  количество ссылок к данным звука
//-----------------------------------------------------------------------------
int CSoundFile::AddRef(void)
{
	_ref++;
	return _ref;
}

//-----------------------------------------------------------------------------
// Удаление ссылки
// на входе    :  *
// на выходе   :  количество ссылок к данным звука
//-----------------------------------------------------------------------------
int CSoundFile::DecRef(void)
{
	if (_ref <= 0)
		_ref = 0;
	else
		_ref--;

	return _ref;
}
//-----------------------------------------------------------------------------
//	Инициализировать файл
//	на входе	:	*
//	на выходе	:	успешность инициализации
//-----------------------------------------------------------------------------
bool CSoundFile::Init(void)
{
	bool format_init;
	// инициализация загруженных данных
	WAVEFORMATEX* format;

	// попробуем открыть файл как wav
	sound_data = new CWavFile(reader);

	// проверка инициализации файла
	format = sound_data->GetFmt();
	if (format) {
		// проверка на формат PCM
		if (format->wFormatTag == 1) {
			if (format->wBitsPerSample == 8)
				decoder = new CDecompressPcm8Unsigned(&pcm_format,
							  	format_init,
							  	sound_data);
			else
				decoder = new CDecompressPcm16Signed(&pcm_format,
							  	format_init,
							  	sound_data);
			return format_init;
		}

		// проверка на формат MS ADPCM
		if (format->wFormatTag == 2) {
			decoder = new CDecompressMsAdpcm(&pcm_format,
						  	format_init,
						  	sound_data);
			return format_init;
		}

		// проверка на формат IMA ADPCM версии от MS
		if (format->wFormatTag == 17) {
			decoder = new CDecompressImaAdpcmMs(&pcm_format,
						  	format_init,
						  	sound_data);
			return format_init;
		}

		// проверка на формат Mpeg Layer 3
		if (format->wFormatTag == 85) {
			decoder = new CDecompressMpeg(&pcm_format, format_init, sound_data);
			return format_init;
		}
	}
	// удалим тип
	delete sound_data;
	sound_data = 0;
	delete decoder;
	decoder = 0;


	char* test = reader->get_file_ext(); 

	// проверка расширения файла
        /*
	if (reader->get_file_ext() != 0 && !memcmp(reader->get_file_ext(),
											"wma",
											3)) {
		// открываем файл как wma
		sound_data = new CWmaFile(reader);
		decoder = new CDecompressWma(&pcm_format, format_init, sound_data);
		if (format_init)
			return format_init;

		delete sound_data;
		sound_data = 0;
		delete decoder;
		decoder = 0;
	} else {
        */
		// поробуем открыть файл как Mpeg
		sound_data = new CMpegFile(reader);
		decoder = new CDecompressMpeg(&pcm_format, format_init, sound_data);
		if (format_init)
			return format_init;

		// удалим тип
		delete sound_data;
		sound_data = 0;
		delete decoder;
		decoder = 0;

		// попробуем открыть как Ogg
		sound_data = new COggFile(reader);
		decoder = new CDecompressOgg(&pcm_format, format_init, sound_data);
		if (format_init)
			return format_init;

		delete sound_data;
		sound_data = 0;
		delete decoder;
		decoder = 0;
		/*  	
				  // попробуем открыть как трекерный формат
				  sound_data	= new CTrackFile(reader);
				  decoder		= new CDecompressTrack(&pcm_format, format_init, sound_data);
				  if (format_init)
					 return format_init;
					delete sound_data;
					sound_data = 0;
					delete decoder;
					decoder = 0;
			*/
	//}
	// ошибка инициализации данных 
	return false;
}

//-----------------------------------------------------------------------------
//	Создать звуковые данные из файла
//	на входе	:	filename	- указатель на имя файла
//					stream_flag	- флаг поточного использования файла
//	на выходе	:	успешность создания звуковых данных
//-----------------------------------------------------------------------------
bool CSoundFile::CreateSoundDataFromFile(const char* filename, int stream_flag)
{
	bool ret = false;

	// защита от дурака
	if (!this)
		return ret;

	// инициализируем низкоуровневый обработчик файла
	reader = new CReader(filename,
				 	stream_flag,
				 	ret,
				 	api._open,
				 	api._read,
				 	api._seek,
				 	api._close,
				 	api._malloc,
				 	api._free);
	if (!ret) {
		DeInit();
		return ret;
	}

	// инициализация данных
	ret = Init();
	if (!ret)
		DeInit();

	return ret;
}

//-----------------------------------------------------------------------------
//	Создать звуковые данные из памяти
//	на входе	:	memory_ptr	- указатель на имя файла
//					memory_size	- флаг поточного использования файла
//					new_memory	- флаг создания копии передаваемых данных
//	на выходе	:	успешность создания звуковых данных
//-----------------------------------------------------------------------------
bool CSoundFile::CreateSoundDataFromMemory(void* memory,
	unsigned int memory_size, int new_memory)
{
	bool ret = false;

	// защита от дурака
	if (!this)
		return ret;

	// инициализируем низкоуровневый обработчик файла
	reader = new CReader(memory,
				 	memory_size,
				 	new_memory,
				 	ret,
				 	api._malloc,
				 	api._free);
	if (!ret) {
		DeInit();
		return ret;
	}

	// инициализация данных
	ret = Init();
	if (!ret)
		DeInit();

	return ret;
}

//-----------------------------------------------------------------------------
//	Процедура освобождения занятых классом данных
//	на входе	:	*
//	на выходе	:	*
//-----------------------------------------------------------------------------
void CSoundFile::DeInit(void)
{
	// декодер есть ?
	if (decoder) {
		delete decoder;
		decoder = 0;
	}

	// класс работы с конкретным звуковым типом есть ?
	if (sound_data) {
		delete sound_data;
		sound_data = 0;
	}
	// класс для работы с файлом есть ?
	if (reader) {
		delete reader;
		reader = 0;
	}
}

//-----------------------------------------------------------------------------
//	Проверка целосности данных
//	на входе	:	*
//	на выходе	:	целосность данных
//					true - данные впорядке
//					false - данные утеряны или повреждены
//-----------------------------------------------------------------------------
bool CSoundFile::Check(void)
{
	return (this &&
		decoder &&
		sound_data &&
		reader &&
		(class_id == 0x53536746)) ?
		true :
		false;
}

//-----------------------------------------------------------------------------
//	Процедура получение формата звука
//	на входе	:	*
//	на выходе	:	указатель на формат звука (0 - файл не идентифицирован)
//-----------------------------------------------------------------------------
WAVEFORMATEX* CSoundFile::GetFormat(void)
{
	return Check() ? &pcm_format : 0;
}
//-----------------------------------------------------------------------------
//	Заполнение данными буфера
//	на входе	:	buffer	- указатель на буфер заполнения
//					start	- начальная позиция в файле относительно 0
//					length	- длинна получаемых данных
//	на выходе	:	на сколько нужно сдвинуть принимающий буфер
//-----------------------------------------------------------------------------
unsigned int CSoundFile::GetMonoData(void* buffer, unsigned int start,
	unsigned int length, bool loop)
{
	return Check() ? decoder->GetMonoSamples(buffer, start, length, loop) : 0;
}
//-----------------------------------------------------------------------------
//	Заполнение данными буфера
// на входе    :  buffer   - указатель на буфер заполнения
//  			  start    - начальная позиция в файле относительно 0
//  			  length   - длинна получаемых данных
//	на выходе	:	на сколько нужно сдвинуть принимающий буфер
//-----------------------------------------------------------------------------
unsigned int CSoundFile::GetStereoData(void* buffer, unsigned int start,
	unsigned int length, bool loop)
{
	return Check() ?
		decoder->GetStereoSamples(buffer, start, length, loop) :
		0;
}
//-----------------------------------------------------------------------------
// Заполнение "монофонической тишиной" буфера
// на входе    :  buffer   - указатель на буфер заполнения
//  			  length   - длинна тишины в семплах
// на выходе   :  на сколько нужно сдвинуть принимающий буфер
//-----------------------------------------------------------------------------
unsigned int CSoundFile::GetMonoMute(void* buffer, unsigned int length)
{
	return Check() ? decoder->GetMonoMute(buffer, length) : 0;
}
//-----------------------------------------------------------------------------
// Заполнение "стерефонической тишиной" буфера
// на входе    :  buffer	- указатель на буфер заполнения
//  			  length	- длинна тишины в семплах
//	на выходе   :  на сколько нужно сдвинуть принимающий буфер
//-----------------------------------------------------------------------------
unsigned int CSoundFile::GetStereoMute(void* buffer, unsigned int length)
{
	return Check() ? decoder->GetStereoMute(buffer, length) : 0;
}
//-----------------------------------------------------------------------------
// Получение длинны моно трека в байтах
// на входе    :  *
// на выходе   :  длинна трека в байтах
//-----------------------------------------------------------------------------
unsigned int CSoundFile::GetRealMonoDataSize(void)
{
	return Check() ? decoder->GetRealMonoDataSize() : 0;
}
//-----------------------------------------------------------------------------
//	Получение длинны стерео трека в байтах
//	на входе	:	*
//	на выходе	:	длинна трека в байтах
//-----------------------------------------------------------------------------
unsigned int CSoundFile::GetRealStereoDataSize(void)
{
	return Check() ? decoder->GetRealStereoDataSize() : 0;
}
//-----------------------------------------------------------------------------
// Получение длинны трека с семплах
// на входе    :  *
// на выходе   :  длинна трека в семплах
//-----------------------------------------------------------------------------
unsigned int CSoundFile::GetSamplesInFile(void)
{
	return Check() ? decoder->GetSamplesInFile() : 0;
}

//-----------------------------------------------------------------------------
// Получение длинны трека с семплах
// на входе    :	*
// на выходе   :	получение указателя на имя файла
//-----------------------------------------------------------------------------
char* CSoundFile::GetFileName(void)
{
	return Check() ? sound_data->get_file_name() : 0;
}
