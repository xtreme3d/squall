//-----------------------------------------------------------------------------
//	Класс для работы с Wav файлами
//	Копонент звукового двигателя Шквал
//	команда		: AntiTank
//	разработчик	: Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------

// включаемые файлы
#include "WavFile.h"

//-----------------------------------------------------------------------------
//	Конструктор класса
//	на входе	:	reader	- указатель низкоуровневый обработчик файла
//	на выходе	:	*
//-----------------------------------------------------------------------------
CWavFile::CWavFile(CReader* reader_ptr)
	: CAbstractSoundFile()
{
	memset(fmt, 0, sizeof(WAVEFORMATEX));
	wav_header head;
	wav_block block;
	DWORD cur_pos;

	// запишем указатель на низкоуровневые данные файла
	reader = reader_ptr;

	// прочитаем заголовок файла
	reader->read(&head, sizeof(head));

	// проверка идентификатора файла
	if ((head.Id1 == RIFF_ID) && (head.Id2 == WAVE_ID)) {
		// ищем контейнер с форматом и данными
		do {
			// прочитаем первый контейнер
			cur_pos = reader->tell();
			reader->read(&block, sizeof(wav_block));

			// контейнер формата
			if (block.Id == FORMAT_ID) {
				if (block.Size > 256)
					return;
				reader->read(&fmt, block.Size);
			}

			// контейнер данных
			if (block.Id == DATA_ID) {
				start_position = cur_pos + sizeof(wav_block);
				end_position = start_position + block.Size;
				size = block.Size;
			}
			// получим указатель на следующий блок
			reader->seek(cur_pos + block.Size + sizeof(wav_block));
		} while (!reader->eof());

		// встанем в начало файла
		reader->seek(start_position);
		cur_position = 0;
		return;
	}
}

//-----------------------------------------------------------------------------
//	Деструктор класса
//	на входе	:	*
//	на выходе	:	*
//-----------------------------------------------------------------------------
CWavFile::~CWavFile()
{
}