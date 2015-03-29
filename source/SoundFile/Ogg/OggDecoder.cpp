//-----------------------------------------------------------------------------
//	Класс декодера Ogg формата
//	Копонент звукового двигателя Шквал
//	команда		: AntiTank
//	разработчик	: Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------
#include "OggDecoder.h"
// чтение данных
size_t OggRead(void* ptr, size_t size, size_t nmemb, void* datasource)
{
	CAbstractSoundFile* SourceData = (CAbstractSoundFile*) datasource;

	// проверка на конец файла
	if (SourceData->eof())
		return 0;

	// чтение данных
	return SourceData->read(ptr, size * nmemb);
}

// позиционирование в файле
int OggSeek(void* datasource, ogg_int64_t offset, int whence)
{
	CAbstractSoundFile* SourceData = (CAbstractSoundFile*) datasource;
	// чтение данных
	return SourceData->seek((int) offset, whence);
}

// закрытие файла
int OggClose(void* datasource)
{
	return 0;
}

// получение длинны файла
long OggTell(void* datasource)
{
	CAbstractSoundFile* SourceData = (CAbstractSoundFile*) datasource;
	return SourceData->tell();
}

#define TEMP_BUFFER_SIZE 1024

int CDecompressOgg::GetSamples(void* buffer, int remaining, int out_channels)
{
	int j, in_channels, val, read_bytes;

	int need_read = 0;
	int temp;

	int buffer_offset = 0;
	int read_samples = 0;

	// указатель на буфер
	short* out = (short*) buffer;
	short* in = 0;

	// получим исходное количество каналов
	in_channels = ov_info(_vf, -1)->channels;

	// определим нужное количество данных в байтах
	int remaining_bytes = remaining* in_channels * 2;

	while (remaining_bytes) {
		// ограничение данных по размеру буфера
		need_read = (remaining_bytes > _tempBufferSize) ?
			_tempBufferSize :
			remaining_bytes;

		read_bytes = ov_read(_vf,
					 	(char *) _tempBuffer,
					 	need_read,
					 	0,
					 	2,
					 	1,
					 	& temp);

		if (in_channels == out_channels) {
			// количество каналов входящего и исходного совпадают
			// копирование буфера
			memcpy(out, _tempBuffer, read_bytes);
			out = (short *) ((char *) out + read_bytes);
		} else {
			// количество каналов входящего и исходного не совпадают

			// получение указателя на буфер
			in = (short *) _tempBuffer;
			// вычислим количество прочитаных семплов
			read_samples = read_bytes / (ov_info(_vf, -1)->channels * 2);

			// конвертация
			if (in_channels == 1) {
				// моно в стерео
				for (j = 0; j < read_samples; j++) {
					val = (int) * in;

					if (val > 32767)
						val = 32767;
					else if (val < -32768)
						val = -32768;
					*out++ = val;
					*out++ = val;
					in++;
				}
			} else {
				// стерео в моно
				for (j = 0; j < read_samples; j++) {
					val = ((int) in[0] + (int) in[1]) >> 1;

					if (val > 32767)
						val = 32767;
					else if (val < -32768)
						val = -32768;
					*out++ = val;
					in += 2;
				}
			}
		}

		remaining_bytes -= read_bytes;
	}
	return (int) out - (int) buffer;
}

//-----------------------------------------------------------------------------
//	Конструктор декодера
//	на входе	:	pcm_format	- указатель на формат куда поместить параметры
//								  звука
//					flag		- указатель на переменную в которую поместить
//								  флаг успешности инициализации
//					a			- указатель на абстрактный звуковой файл
//								  родитель
//	на выходе	:	*
//-----------------------------------------------------------------------------
CDecompressOgg::CDecompressOgg(WAVEFORMATEX* pcm_format, bool& flag,
	CAbstractSoundFile* a)
	: CAbstractDecompressor(pcm_format, flag, a)
{
	ov_callbacks Callback;

	flag = false;

	// внешние методы чтения данных
	Callback.read_func = OggRead;
	Callback.seek_func = OggSeek;
	Callback.close_func = OggClose;
	Callback.tell_func = OggTell;

	// выделение памяти под структуру Ogg
#if AGSS_USE_MALLOC
	_vf = (OggVorbis_File *) malloc(sizeof(OggVorbis_File));
#else
	_vf = (OggVorbis_File *) GlobalAlloc(GPTR, sizeof(OggVorbis_File));
#endif

	// память выделена ?
	if (_vf) {
		// откроем файл
		if (!ov_open_callbacks(SourceData, _vf, NULL, 0, Callback)) {
			// получим информацию о файле
			vorbis_info* vi = ov_info(_vf, -1);

			// преобразуем данные для Direct X (иначе Direct X не сможет создать буфер)
			pcm_format->wFormatTag = 1;
			pcm_format->wBitsPerSample = 16;
			pcm_format->nSamplesPerSec = vi->rate;
			pcm_format->nChannels = vi->channels;
			pcm_format->nBlockAlign = (pcm_format->nChannels * pcm_format->wBitsPerSample) >>
				3;
			pcm_format->nAvgBytesPerSec = pcm_format->nBlockAlign * pcm_format->nSamplesPerSec;

			// выделение памяти под промежуточный буфер
#if AGSS_USE_MALLOC
			_tempBuffer = malloc(SAMPLES_IN_TEMP_BUFFER * 4);
#else
			_tempBuffer = GlobalAlloc(GPTR, SAMPLES_IN_TEMP_BUFFER * 4);
#endif
			// проверка наличия памяти
			if (_tempBuffer == 0)
				return;

			// размер промежуточного буфера
			_tempBufferSize = SAMPLES_IN_TEMP_BUFFER * 4;

			// все прошло 
			flag = true;
		}
		// проверка инициализации
		if (!flag) {
#if AGSS_USE_MALLOC
			free(_vf);
#else
			GlobalFree(_vf);
#endif
			_vf = 0;
		}
	}
}

//-----------------------------------------------------------------------------
//	Деструктор декодера
//	на входе	:	*
//	на выходе	:	*
//-----------------------------------------------------------------------------
CDecompressOgg::~CDecompressOgg()
{
	if (_vf) {
		// освободим данные декодера
		ov_clear(_vf);
		// освободим структуру
#if AGSS_USE_MALLOC
		free(_vf);
#else
		GlobalFree(_vf);
#endif
		_vf = 0;
	}

	// освободим промежуточный буфер
	if (_tempBuffer) {
#if AGSS_USE_MALLOC
		free(_tempBuffer);
#else
		GlobalFree(_tempBuffer);
#endif
	}
}

//-----------------------------------------------------------------------------
//	Декомпрессия OGG формата в стерео данные
//	на входе	:	buffer	- указатель на буфер
//					start	- смещение в данных звука, в семплах
//					length	- количество семплов для декодирования
//	на выходе	:	На сколько байт сдвинулся буфер в который
//					читали семплы
//-----------------------------------------------------------------------------
DWORD CDecompressOgg::GetStereoSamples(void* buffer, DWORD start,
	DWORD length, bool loop)
{
	if (start != 0xFFFFFFFF)
		ov_pcm_seek(_vf, start);
	return GetSamples(buffer, length, 2);
}

//-----------------------------------------------------------------------------
//	Декомпрессия OGG формата в моно данные
//	на входе	:	buffer	- указатель на буфер
//					start	- смещение в данных звука, в семплах
//					length	- количество семплов для декодирования
//	на выходе	:	На сколько байт сдвинулся буфер в который
//					читали семплы
//-----------------------------------------------------------------------------
DWORD CDecompressOgg::GetMonoSamples(void* buffer, DWORD start, DWORD length,
	bool loop)
{
	if (start != 0xFFFFFFFF)
		ov_pcm_seek(_vf, start);
	return GetSamples(buffer, length, 1);
}

//-----------------------------------------------------------------------------
//	Создание тишины на заданом отрезке буфера стерео режим
//	на входе	:	buffer	- указатель на буфер
//					length	- количество семплов
//	на выходе	:	На сколько байт сдвинулся буфер
//-----------------------------------------------------------------------------
DWORD CDecompressOgg::GetStereoMute(void* buffer, DWORD length)
{
	length <<= 2;
	memset(buffer, 0, length);
	return length;
}

//-----------------------------------------------------------------------------
//	Создание тишины на заданом отрезке буфера моно режим
//	на входе	:	buffer	- указатель на буфер
//					length	- количество семплов
//	на выходе	:	На сколько байт сдвинулся буфер
//-----------------------------------------------------------------------------
DWORD CDecompressOgg::GetMonoMute(void* buffer, DWORD length)
{
	length <<= 1;
	memset(buffer, 0, length);
	return length;
}

//-----------------------------------------------------------------------------
//	Получение количества семплов в файле
//	на входе	:	*
//	на выходе	:	Количество семплов в файла
//-----------------------------------------------------------------------------
DWORD CDecompressOgg::GetSamplesInFile(void)
{
	return (DWORD) ov_pcm_total(_vf, -1);
}

//-----------------------------------------------------------------------------
//	Получение количества байт в треке моно режим
//	на входе	:	*
//	на выходе	:	Количество баит в треке
//-----------------------------------------------------------------------------
DWORD CDecompressOgg::GetRealMonoDataSize(void)
{
	return GetSamplesInFile() * 2;
}

//-----------------------------------------------------------------------------
//	Получение количества байт в треке стерео режим
//	на входе	:	*
//	на выходе	:	Количество баит в треке
//-----------------------------------------------------------------------------
DWORD CDecompressOgg::GetRealStereoDataSize(void)
{
	return GetSamplesInFile() * 4;
}
