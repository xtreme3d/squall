//-----------------------------------------------------------------------------
//	Класс декодера IMA ADPCM формата версия от MS
//	Копонент звукового двигателя Шквал
//	команда		: AntiTank
//	разработчик	: Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------

#include "ImaAdpcmDecoder.h"

// таблица доступных значений размера шага для IMA ADPCM
static const int _stepSizeTable[89] = {
	7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41,
	45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190,
	209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
	876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499,
	2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845,
	8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385,
	24623, 27086, 29794, 32767
};

// таблица коррекции индекса IMA ADPCM
static const int indexAdjustTable[16] = {
	-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8, 
};

// информация состояния IMA ADPCM канала
struct SImaState {
	int index;
	int previousValue;
};

//-----------------------------------------------------------------------------
//	Декодирование одного полубайта
//	на входе	:	deltaCode	- значение полубайта
//					state		- состояние для декодируемого канала
//	на выходе	:	раскодированное значение
//-----------------------------------------------------------------------------
WORD ImaAdpcmDecode(BYTE deltaCode, SImaState& state)
{
	// получаем значение текущей величины шага
	int step = _stepSizeTable[state.index];

	// создаем приращение, масштабируя текущую величину шага
	// приблизительно так: приращение = (deltaCode+0.5)*шаг/4
	int difference = step >> 3;
	if (deltaCode & 1)
		difference += step >> 2;
	if (deltaCode & 2)
		difference += step >> 1;
	if (deltaCode & 4)
		difference += step;
	if (deltaCode & 8)
		difference = -difference;

	// вычисляем новую выборку
	state.previousValue += difference;
	if (state.previousValue > 32767)
		state.previousValue = 32767;
	else if (state.previousValue < -32768)
		state.previousValue = -32768;

	// обновляем величину шага для обработки следующей выборки
	state.index += indexAdjustTable[deltaCode];
	if (state.index < 0)
		state.index = 0;
	else if (state.index > 88)
		state.index = 88;
	return state.previousValue;
}

//-----------------------------------------------------------------------------
//	Декодирование пакета
//	на входе	:	*
//	на выходе	:	количество декодированных семплов
//-----------------------------------------------------------------------------
DWORD CDecompressImaAdpcmMs::DecodePacket(void)
{
	BYTE b;
	DWORD i, remaining;
	short* left_ptr;
	short* rigth_ptr;

	// состояния правого и левого канала
	SImaState state[2];

	// читаем заголовок
	if (SourceData->read(_packetBuffer, _bytesPerPacket) != _bytesPerPacket)
		return 0;

	char* bytePtr = _packetBuffer;

	// получим указатели на буфера
	left_ptr = _left;
	rigth_ptr = _rigth;

	// проверка типа записи
	if (_channels == 1)	// моно запись
	{
		// инициализация левого канала
		state[0].previousValue = bytePtr[1] * 0x100 + bytePtr[0];
		bytePtr += 2;
		state[0].index = *bytePtr++;
		if ((*bytePtr++) || (state[0].index > 88))
			return 0;
		*left_ptr++ = state[0].previousValue;

		remaining = _samplesPerPacket - 1;

		while (remaining) {
			for (i = 0; i < 4; i++) {
				b = *bytePtr++;
				*left_ptr++ = ImaAdpcmDecode(b & 0xf, state[0]);
				*left_ptr++ = ImaAdpcmDecode((b >> 4) & 0xf, state[0]);
			}
			remaining -= 8;
		}
	} else {
		// стерео запись

		// инициализация левого канала
		state[0].previousValue = bytePtr[1] * 0x100 + bytePtr[0];
		bytePtr += 2;
		state[0].index = *bytePtr++;
		if ((*bytePtr++) || (state[0].index > 88))
			return 0;
		*left_ptr++ = state[0].previousValue;

		// инициализация правого канала
		state[1].previousValue = bytePtr[1] * 0x100 + bytePtr[0];
		bytePtr += 2;
		state[1].index = *bytePtr++;
		if ((*bytePtr++) || (state[1].index > 88))
			return 0;
		*rigth_ptr++ = state[1].previousValue;

		remaining = _samplesPerPacket - 1;

		while (remaining) {
			for (i = 0; i < 4; i++) {
				b = *bytePtr++;
				*left_ptr++ = ImaAdpcmDecode(b & 0xf, state[0]);
				*left_ptr++ = ImaAdpcmDecode((b >> 4) & 0xf, state[0]);
			}

			for (i = 0; i < 4; i++) {
				b = *bytePtr++;
				*rigth_ptr++ = ImaAdpcmDecode(b & 0xf, state[1]);
				*rigth_ptr++ = ImaAdpcmDecode((b >> 4) & 0xf, state[1]);
			}
			remaining -= 8;
		}
	}
	return _samplesPerPacket;
}

//-----------------------------------------------------------------------------
//	Конструктор декодера
//	на входе	:	pcm_format	- указатель на структуру формата, куда 
//								  поместить параметры звука
//					flag		- указатель на переменную в которую поместить
//								  флаг успешности инициализации
//					a			- указатель на абстрактный звуковой файл
//								  родитель
//	на выходе	:	*
//-----------------------------------------------------------------------------
CDecompressImaAdpcmMs::CDecompressImaAdpcmMs(WAVEFORMATEX* pcm_format,
	bool& flag, CAbstractSoundFile* a)
	: CAbstractDecompressor(pcm_format, flag, a)
{
	flag = false;
	// получение указателя на формат
	SIMA_ADPCM_Format* format = (SIMA_ADPCM_Format*) SourceData->GetFmt();

	// заполнение данными полей структуры
	_channels = format->fmt.nChannels;					// количество звуковых каналов
	_samplesPerPacket = format->SamplesPerPacket;					// количество выборок в пакете
	_bytesPerPacket = ((_samplesPerPacket + 7) / 2) * _channels;// вычислим количество байт на пакет
	_packetof = SourceData->size / _bytesPerPacket;		// определение количества пакетов в файле
	_curPacket = 0;

	// проверка на количество каналов в файле (зацита от дурака)
	if ((!_channels) || (_channels > 2))
		return;

	// выделим буфер для распакованных данных левого, правого канала и буфера под пакет
#if AGSS_USE_MALLOC
	_packetBuffer = (char *) malloc(_samplesPerPacket * _channels * 2 +
							 	_bytesPerPacket);
#else
	_packetBuffer = (char *) GlobalAlloc(GPTR,
							 	_samplesPerPacket * _channels * 2 +
							 	_bytesPerPacket);
#endif

	if (_packetBuffer != 0) {
		// пропишем указатель на правый канал
		_left = (short *) (_packetBuffer + _bytesPerPacket);

		// пропишем указатель на буфер нераспакованных пакетов
		_rigth = _left + _samplesPerPacket;

		// преобразуем данные для Direct X (иначе Direct X не сможет создать буфер)
		pcm_format->wFormatTag = 1;
		pcm_format->wBitsPerSample = 16;
		pcm_format->nSamplesPerSec = format->fmt.nSamplesPerSec;
		pcm_format->nChannels = format->fmt.nChannels;
		pcm_format->nBlockAlign = (pcm_format->nChannels * pcm_format->wBitsPerSample) >>
			3;
		pcm_format->nAvgBytesPerSec = pcm_format->nBlockAlign * pcm_format->nSamplesPerSec;

		// декодируем один пакет
		DecodePacket();
		flag = true;
		return;
	}

	// очистим данные в случае неудачи
	_left = 0;
	_rigth = 0;
}

//-----------------------------------------------------------------------------
//	Деструктор декодера
//	на входе	:	*
//	на выходе	:	*
//-----------------------------------------------------------------------------
CDecompressImaAdpcmMs::~CDecompressImaAdpcmMs()
{
	// освободим занятую память
	if (_packetBuffer) {
#if AGSS_USE_MALLOC
		free(_packetBuffer);
#else
		GlobalFree(_packetBuffer);
#endif
	}

	// обнулим указатели
	_packetBuffer = 0;
	_left = 0;
	_rigth = 0;
}

//-----------------------------------------------------------------------------
//	Декомпрессия IMA ADPCM формата версия от MS в моно данные
//	на входе	:	buffer	- указатель на буфер
//					start	- смещение в данных звука, в семплах
//					length	- количество семплов для декодирования
//	на выходе	:	На сколько байт сдвинулся буфер в который
//					читали семплы
//-----------------------------------------------------------------------------
DWORD CDecompressImaAdpcmMs::GetMonoSamples(void* buffer, DWORD start,
	DWORD length, bool loop)
{
	int s;
	short* Out;
	short* LeftPtr;
	short* RigthPtr;
	DWORD Offset;
	DWORD NeedPacket;
	DWORD Samples;

	// вычислим в каком пакете находиться начало
	NeedPacket = (start / _samplesPerPacket) * _bytesPerPacket;
	// вычислим смещение в распакованом пакете
	Offset = start % _samplesPerPacket;
	// инициализация переменных
	Out = (short *) buffer;

	while (length) {
		// проверка на нахождение в нужном пакете
		if (_curPacket != NeedPacket) {
			if ((_curPacket + _bytesPerPacket) != NeedPacket)
				SourceData->seek(NeedPacket, FILE_BEGIN);
			// декодирование нового пакета
			_curPacket = NeedPacket;
			DecodePacket();
		}

		// расчет смещений в буфере
		LeftPtr = _left + Offset;
		RigthPtr = _rigth + Offset;
		Samples = _samplesPerPacket - Offset;
		Offset = 0;

		// определим количество семплов для извлечения из данного пакета
		if (Samples > length)
			Samples = length;
		length -= Samples;

		// сборка семплов
		if (_channels == 1) {
			// моно данные
			do {
				*Out++ = *LeftPtr++;
			} while (--Samples);
		} else {
			// стерео данные
			do {
				s = ((int) * LeftPtr + (int) * RigthPtr) >> 1;
				if (s < -32768)
					s = -32768;
				else if (s > 32767)
					s = 32767;
				*Out++ = (short) s;
				LeftPtr++;
				RigthPtr++;
			} while (--Samples);
		}
		if (length)
			NeedPacket += _bytesPerPacket;
	}

	// вернем сдвиг в принимающем буфере
	return (DWORD) Out - (DWORD) buffer;
}

//-----------------------------------------------------------------------------
//	Декомпрессия IMA ADPCM формата версия от MS в стерео данные
//	на входе	:	buffer	- указатель на буфер
//					start	- смещение в данных звука, в семплах
//					length	- количество семплов для декодирования
//	на выходе	:	На сколько байт сдвинулся буфер в который
//					читали семплы
//-----------------------------------------------------------------------------
DWORD CDecompressImaAdpcmMs::GetStereoSamples(void* buffer, DWORD start,
	DWORD length, bool loop)
{
	short* Out;
	short* LeftPtr;
	short* RigthPtr;
	DWORD Offset;
	DWORD NeedPacket;
	DWORD Samples;

	// вычислим в каком пакете находиться начало
	NeedPacket = (start / _samplesPerPacket) * _bytesPerPacket;
	// вычислим смещение в распакованом пакете
	Offset = start % _samplesPerPacket;
	// инициализация переменных
	Out = (short *) buffer;

	while (length) {
		// проверка на нахождение в нужном пакете
		if (_curPacket != NeedPacket) {
			if ((_curPacket + _bytesPerPacket) != NeedPacket)
				SourceData->seek(NeedPacket, FILE_BEGIN);
			// декодирование нового пакета
			_curPacket = NeedPacket;
			DecodePacket();
		}

		// расчет смещений в буфере
		LeftPtr = _left + Offset;
		RigthPtr = _rigth + Offset;
		Samples = _samplesPerPacket - Offset;
		Offset = 0;

		// определим количество семплов для извлечения из данного пакета
		if (Samples > length)
			Samples = length;
		length -= Samples;

		// сборка потока
		if (_channels == 1) {
			// моно данные
			do {
				*Out++ = *LeftPtr;
				*Out++ = *LeftPtr;
				LeftPtr++;
			} while (--Samples);
		} else {
			// стерео данные
			do {
				*Out++ = *LeftPtr++;
				*Out++ = *RigthPtr++;
			} while (--Samples);
		}
		if (length)
			NeedPacket += _bytesPerPacket;
	}

	// вернем сдвиг в принимающем буфере
	return (DWORD) Out - (DWORD) buffer;
}
//-----------------------------------------------------------------------------
//	Создание тишины на заданом отрезке буфера
//	на входе	:	buffer	- указатель на буфер
//					length	- количество семплов
//	на выходе	:	На сколько байт сдвинулся буфер
//-----------------------------------------------------------------------------
DWORD CDecompressImaAdpcmMs::GetMonoMute(void* buffer, DWORD length)
{
	// так как данные 16 битные -> samples*16/8*1
	length <<= 1;

	// очистим буфер
	memset(buffer, 0, length);

	return length;
}

//-----------------------------------------------------------------------------
//	Создание тишины на заданом отрезке буфера
//	на входе	:	buffer	- указатель на буфер
//					length	- количество семплов
//	на выходе	:	На сколько байт сдвинулся буфер
//-----------------------------------------------------------------------------
DWORD CDecompressImaAdpcmMs::GetStereoMute(void* buffer, DWORD length)
{
	// так как данные 16 битные -> samples*16/8*2
	length <<= 2;

	// очистим буфер
	memset(buffer, 0, length);

	return length;
}
