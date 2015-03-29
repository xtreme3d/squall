//-----------------------------------------------------------------------------
//	Класс декодера ADPCM формата, версия от MicroSoft
//	Копонент звукового двигателя Шквал
//	команда		: AntiTank
//	разработчик	: Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------

// включения
#include "MsAdpcmDecoder.h"

// таблица коррекции индекса IMA ADPCM
static const int IndexTable[16] = {
	230, 230, 230, 230, 307, 409, 512, 614, 768, 614, 512, 409, 307, 230, 230,
	230, 
};

//-----------------------------------------------------------------------------
// структуры
//-----------------------------------------------------------------------------
#pragma pack(1)

// структура монофонического заголовка
struct SMonoHeader {
	char table_index;
	short step;
	short cur_value;
	short prev_value;
};

// структура стереофонического заголовка
struct SStereoHeader {
	char l_table_index;
	char r_table_index;
	short l_step;
	short r_step;
	short l_cur_value;
	short r_cur_value;
	short l_prev_value;
	short r_prev_value;
};

// структура состояния канала
struct SChannelState {
	int coef_1;
	int coef_2;
	int step;
	int previous_value;
	int current_value;
};

//-----------------------------------------------------------------------------
//	Декодирование одной выборки
//	на входе	:	delta_code	- код смещения
//					state		- состояник декодируемого канала
//	на выходе	:	декодированое значение
//-----------------------------------------------------------------------------
WORD DecodeMsAdpcm(char delta_code, SChannelState& state)
{
	int data;
	delta_code >>= 4;

	data = (state.step * delta_code) +
		(((state.coef_2 * state.previous_value) +
		(state.coef_1 * state.current_value)) >>
		8);
	if (data > 32767)
		data = 32767;
	else if (data < -32768)
		data = -32768;

	state.step = (state.step * IndexTable[delta_code & 0xF]) >> 8;
	if (state.step < 16)
		state.step = 16;

	state.previous_value = state.current_value;
	state.current_value = data;
	return data;
}

//-----------------------------------------------------------------------------
//	Декодирование монофонического пакета
//	на входе	:	*
//	на выходе	:	количество декодированных семплов
//-----------------------------------------------------------------------------
DWORD CDecompressMsAdpcm::DecodeMONOPacket(void)
{
	SChannelState state;
	int bytes;
	// читаем заголовок
	if (SourceData->read(_packetBuffer, _bytesPerPacket) != _bytesPerPacket)
		return 0;

	char* in_buffer = (char*) _packetBuffer;
	short* out_buffer = _left;

	bytes = _bytesPerPacket;

	char value =* in_buffer;
	if (value > _numCoef)
		return 0;

	state.coef_1 = _coefTable[value].Coef1;
	state.coef_2 = _coefTable[value].Coef2;
	state.step = ((SMonoHeader *) in_buffer)->step;
	state.current_value = ((SMonoHeader *) in_buffer)->cur_value;
	state.previous_value = ((SMonoHeader *) in_buffer)->prev_value;

	*out_buffer++ = state.previous_value;
	*out_buffer++ = state.current_value;

	bytes -= sizeof(SMonoHeader);
	in_buffer += sizeof(SMonoHeader);

	while (bytes) {
		value = *in_buffer++;
		*out_buffer++ = DecodeMsAdpcm(value, state);

		value <<= 4;
		*out_buffer++ = DecodeMsAdpcm(value, state);

		bytes--;
	}
	return _samplesPerPacket;
}

//-----------------------------------------------------------------------------
//	Декодирование пакета
//	на входе	:	*
//	на выходе	:	количество декодированных семплов
//-----------------------------------------------------------------------------
DWORD CDecompressMsAdpcm::DecodeSTEREOPacket(void)
{
	SChannelState state[2];
	int bytes;
	char value;
	// читаем заголовок
	if (SourceData->read(_packetBuffer, _bytesPerPacket) != _bytesPerPacket)
		return 0;

	char* in_buffer = (char*) _packetBuffer;
	short* l_out_buffer = _left;
	short* r_out_buffer = _rigth;

	bytes = _bytesPerPacket;

	value = ((SStereoHeader *) in_buffer)->l_table_index;
	if (value > _numCoef)
		return 0;

	state[0].coef_1 = _coefTable[value].Coef1;
	state[0].coef_2 = _coefTable[value].Coef2;
	state[0].step = ((SStereoHeader *) in_buffer)->l_step;
	state[0].current_value = ((SStereoHeader *) in_buffer)->l_cur_value;
	state[0].previous_value = ((SStereoHeader *) in_buffer)->l_prev_value;

	value = ((SStereoHeader *) in_buffer)->r_table_index;
	if (value > _numCoef)
		return 0;

	state[1].coef_1 = _coefTable[value].Coef1;
	state[1].coef_2 = _coefTable[value].Coef2;
	state[1].step = ((SStereoHeader *) in_buffer)->r_step;
	state[1].current_value = ((SStereoHeader *) in_buffer)->r_cur_value;
	state[1].previous_value = ((SStereoHeader *) in_buffer)->r_prev_value;

	*l_out_buffer++ = state[0].previous_value;
	*l_out_buffer++ = state[0].current_value;
	*r_out_buffer++ = state[1].previous_value;
	*r_out_buffer++ = state[1].current_value;

	bytes -= sizeof(SStereoHeader);
	in_buffer += sizeof(SStereoHeader);

	while (bytes) {
		value = *in_buffer++;
		*l_out_buffer++ = DecodeMsAdpcm(value, state[0]);

		value <<= 4;
		*r_out_buffer++ = DecodeMsAdpcm(value, state[1]);

		bytes--;
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
CDecompressMsAdpcm::CDecompressMsAdpcm(WAVEFORMATEX* pcm_format, bool& flag,
	CAbstractSoundFile* a)
	: CAbstractDecompressor(pcm_format, flag, a)
{
	flag = false;
	// получение указателя на формат
	SMS_ADPCM_Format* format = (SMS_ADPCM_Format*) SourceData->GetFmt();

	// заполнение данными полей структуры
	_channels = format->fmt.nChannels;					// количество звуковых каналов
	_samplesPerPacket = format->SamplesPerPacket;					// количество выборок в пакете
	_bytesPerPacket = (((_samplesPerPacket - 2) / 2) + 7) * _channels;// вычислим количество байт на пакет
	_packetof = SourceData->size / _bytesPerPacket;		// определение количества пакетов в файле
	_curPacket = 0;

	_numCoef = format->NumCoef;
	_coefTable = (SCoef *) ((char *) format + sizeof(SMS_ADPCM_Format));

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
		if (_channels == 1)
			DecodeMONOPacket();
		else
			DecodeSTEREOPacket();
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
CDecompressMsAdpcm::~CDecompressMsAdpcm()
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
DWORD CDecompressMsAdpcm::GetMonoSamples(void* buffer, DWORD start,
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
			if (_channels == 1)
				DecodeMONOPacket();
			else
				DecodeSTEREOPacket();
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
DWORD CDecompressMsAdpcm::GetStereoSamples(void* buffer, DWORD start,
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
			if (_channels == 1)
				DecodeMONOPacket();
			else
				DecodeSTEREOPacket();
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
DWORD CDecompressMsAdpcm::GetMonoMute(void* buffer, DWORD length)
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
DWORD CDecompressMsAdpcm::GetStereoMute(void* buffer, DWORD length)
{
	// так как данные 16 битные -> samples*16/8*2
	length <<= 2;

	// очистим буфер
	memset(buffer, 0, length);

	return length;
}
