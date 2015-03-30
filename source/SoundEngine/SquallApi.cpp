//-----------------------------------------------------------------------------
// Ядро звукового двигателя "Шквал"
// команда     : AntiTank
// разработчик : Гилязетдинов Марат (Марыч)
//-----------------------------------------------------------------------------
// включения
#include <stdio.h>
#include "ext/ZoomFX.h"
#include "SquallApi.h"
#include "DirectSound.h"
#include "HiddenWindow.h"
#include "Context.h"

/////////////////////////////////////////////////
// макросы
/////////////////////////////////////////////////
#define DEVICE_GROWBY      4
#define CONTEXT_GROWBY     8
#define SAMPLE_GROWBY      128
#define SAMPLES_IN_GROUP   256

/////////////////////////////////////////////////
// глобальные переменные
/////////////////////////////////////////////////
int api_init = 0;   		   // флаг инициализации ядра
squall_api_t api;   					// ядро Шквала
int api_temp[256];  		   // промежуточный буфер для поиска файлов группы

//-----------------------------------------------------------------------------
// Открыти файла
// на входе    :  filename - указатель на имя файла
// на выходе   :  хендлер файла, если значение рано -1 значит произошла ошибка
//-----------------------------------------------------------------------------
unsigned int api_open(const char* filename)
{
	return (unsigned int) CreateFile(filename,
						  	GENERIC_READ,
						  	FILE_SHARE_READ,
						  	NULL,
						  	OPEN_EXISTING,
						  	0,
						  	NULL);
}

//-----------------------------------------------------------------------------
// Чтение данных из файла
// на входе    :  handler  -  хендлер файла
//  			  buffer   -  указатель на буфер в который нужно читать данные
//  			  size     -  количество читаемых данных
// на выходе   :  количество прочитанных данных
//-----------------------------------------------------------------------------
int api_read(unsigned int handler, void* buffer, int size)
{
	unsigned long read;
	ReadFile((void *) handler, buffer, size, & read, 0);
	return read;
}

//-----------------------------------------------------------------------------
// Позиционирование в файле
// на входе    :  handler  -  хендлер файла
//  			  position -  новая позиция в файле
//  			  mode     -  режим позиционирования
// на выходе   :  новая позиция в файле
//-----------------------------------------------------------------------------
int api_seek(unsigned int handler, int position, int mode)
{
	return SetFilePointer((void *) handler, position, 0, mode);
}

//-----------------------------------------------------------------------------
// Закрыти открытого файла
// на входе    :  handler  -  хендлер файла
// на выходе   :  *
//-----------------------------------------------------------------------------
void api_close(unsigned int handler)
{
	CloseHandle((void *) handler);
}

//-----------------------------------------------------------------------------
// Выделение памяти
// на входе    :  size  - количество выделяемой памяти
// на выходе   :  указатель на выделенную память, если значение равно 0 значит
//  			  выделение памяти не состоялось
//-----------------------------------------------------------------------------
void* api_malloc(unsigned int size)
{
	// проверка входных параметров
	if (size == 0)
		return 0;

	// выделение памяти
	return GlobalAlloc(GPTR, size);
}

//-----------------------------------------------------------------------------
// Освобождение памяти
// на входе    :  ptr   - указатель на выделеннй блок памяти
// на выходе   :  *
//-----------------------------------------------------------------------------
void api_free(void* ptr)
{
	// проверка параметров
	if (ptr != 0)
		GlobalFree(ptr);
}


//-----------------------------------------------------------------------------
// Cоздание списка устройств воспроизведения
// на входе    :
// на выходе   :
//-----------------------------------------------------------------------------
void device_GrowArray(void)
{
	device_t** newlist;

	// расширение массива
	api._deviceList._capacity += DEVICE_GROWBY;

	// выделение нового массива
	newlist = (device_t * *)
		malloc(api._deviceList._capacity * sizeof(device_t *));

	// подготовка нового массива
	memset(newlist, 0, api._deviceList._capacity * sizeof(device_t *));

	// копирование старого массива в новый
	if (api._deviceList._size > 0)
		memcpy(newlist,
			api._deviceList._array,
			api._deviceList._size * sizeof(device_t *));

	// освобождение старого массива
	free(api._deviceList._array);

	// рапишем новый массив в ядро
	api._deviceList._array = newlist;
}

#ifndef _USRDLL
//-----------------------------------------------------------------------------
// Cоздание списка устройств воспроизведения
// на входе    :
// на выходе   :
//-----------------------------------------------------------------------------
void context_GrowArray(void)
{
	context_t** newlist;

	// расширение массива
	api._contextList._capacity += CONTEXT_GROWBY;

	// выделение нового массива
	newlist = (context_t * *)
		malloc(api._contextList._capacity * sizeof(context_t *));

	// подготовка нового массива
	memset(newlist, 0, api._contextList._capacity * sizeof(context_t *));

	// копирование старого массива в новый
	if (api._contextList._size > 0)
		memcpy(newlist,
			api._contextList._array,
			api._contextList._size * sizeof(context_t *));

	// освобождение старого массива
	free(api._contextList._array);

	// рапишем новый массив в ядро
	api._contextList._array = newlist;
}
#endif
//-----------------------------------------------------------------------------
// Cоздание списка семплов
// на входе    :
// на выходе   :
//-----------------------------------------------------------------------------
void sample_GrowArray(void)
{
	sample_t** newlist;

	// расширение массива
	api._sampleList._capacity += SAMPLE_GROWBY;

	// выделение нового массива
	newlist = (sample_t * *)
		malloc(api._sampleList._capacity * sizeof(sample_t *));

	// подготовка нового массива
	memset(newlist, 0, api._sampleList._capacity * sizeof(sample_t *));

	// копирование старого массива в новый
	if (api._sampleList._size > 0)
		memcpy(newlist,
			api._sampleList._array,
			api._sampleList._size * sizeof(sample_t *));

	// освобождение старого массива
	free(api._sampleList._array);

	// рапишем новый массив в ядро
	api._sampleList._array = newlist;
}

//-----------------------------------------------------------------------------
// Процедура обратного вызова списка драйверов
// на входе    :  Id	- идентификатор устройства
//  			  Desc  - имя устройства
//  			  Mod      - имя файла с драйвером
//  			  Data  - указатель на данные звукового двигателя
// на выходе   :  TRUE  - продолжить перечисление
//  			  FALSE - закончить перечисление
//-----------------------------------------------------------------------------
BOOL CALLBACK api_DirectSoundEnumerate(LPGUID Id, LPSTR Desc, LPSTR Mod,
	HWND hWnd)
{
	int i;
	LPDIRECTSOUND direct;
	LPDIRECTSOUNDBUFFER primary;
	LPDIRECTSOUND3DLISTENER listener;
	device_t* device;

	// подготовка переменных
	direct = 0;
	primary = 0;
	device = 0;

	// создание Direct Sound объекта
	direct = ds_Create(hWnd, CLSID_DirectSound, Id, DSSCL_EXCLUSIVE);

	// создание первичного буфера
	primary = ds_CreatePrimary(direct);

	// создание слушателя
	listener = ds_CreateListener(primary);

	// конфигурирование первичного буфера прошло ?
	if (ds_ConfigurePrimary(primary, 44100, 16, 2)) {
		// новое устройство есть где разместить ?
		if (api._deviceList._size == api._deviceList._capacity)
			device_GrowArray();

		// создадим новую память под новое устройство
		device = (device_t *) malloc(sizeof(device_t));
		memset(device, 0, sizeof(device_t));

		// запомним идентификатор
		device->_id = Id;

		// скопируем имя драйвера
		i = (strlen(Desc) > (DEVICE_NAME_LEN - 1)) ?
			DEVICE_NAME_LEN -
			1 :
			strlen(Desc);
		memcpy(device->_name, Desc, i);
		device->_name[i] = 0;

		// получение параметров устройства воспроизведения
		if (ds_GetCaps(direct, & device->_caps)) {
			// проверка наличия алгоритмов расчета 3D звука
			for (i = 0; i < 4; i++)
				device->_alg[i] = ds_TestAlgorithm(direct, i);

			// проверка наличия EAX интерфейсов
			for (i = 0; i <= EAX_NUM; i++)
				device->_eax[i] = eax_TestSupport(direct, i);

			// проверка наличия ZOOMFX
			device->_zoomfx = zoomfx_TestSupport(direct);

			// включение нового элемента в массив
			i = api._deviceList._size++;
			api._deviceList._array[i] = device;
		} else {
			// освободим память
			free(device);
		}
	}

	// освобождение сдушателя
	ds_ReleaseListener(listener);

	// освобождение первичного буфера
	ds_ReleasePrimary(primary);

	// освобождение устройства воспроизведения
	ds_Release(direct);

	// продолжаем поиск
	return TRUE;
}

//-----------------------------------------------------------------------------
// Cоздание списка устройств воспроизведения
// на входе    :
// на выходе   :
//-----------------------------------------------------------------------------
int api_CreateDeviceList(void)
{
	HWND hWnd = 0;

	// проверка наличия устройств воспроизведения
	if (waveOutGetNumDevs() == 0)
		return false;

	// создание скрытого окна
	hWnd = CreateHiddenWindow();
	if (!hWnd)
		return false;

	// перечисление всех устройств воспроизведения
	DirectSoundEnumerate((LPDSENUMCALLBACK) api_DirectSoundEnumerate, hWnd);

	// удаление скрытого окна
	ReleaseHiddenWindow(hWnd);

	return true;
}

//-----------------------------------------------------------------------------
// получение количества устройств воспроизведения
// на входе    :  
// на выходе   :  
//-----------------------------------------------------------------------------
int api_GetNumDevice(void)
{
	// инициализация ядра
	api_Startup();

	// вернем количество устройств воспроизведения
	return api._deviceList._size;
}

//-----------------------------------------------------------------------------
// получение имени устройства воспроизведения
// на входе    :  
// на выходе   :  
//-----------------------------------------------------------------------------
char* api_GetDeviceName(int device)
{
	// инициализация ядра
	api_Startup();

	// такое устройство есть ?
	if (device >= api._deviceList._size)
		return 0;

	// вернем имя
	return api._deviceList._array[device]->_name;
}

//-----------------------------------------------------------------------------
// Получение идентификатора устройства воспроизведения
// на входе    :  
// на выходе   :  
//-----------------------------------------------------------------------------
LPGUID api_GetDeviceID(int device)
{
	// инициализация ядра
	api_Startup();

	// такое устройство есть ?
	if (device >= api._deviceList._size)
		return NULL;

	// вернем идентификатор
	return api._deviceList._array[device]->_id;
}

//-----------------------------------------------------------------------------
// Получение идентификатора устройства воспроизведения
// на входе    :  
// на выходе   :  
//-----------------------------------------------------------------------------
device_t* api_GetDevice(int device)
{
	// инициализация ядра
	api_Startup();

	// такое устройство есть ?
	if (device >= api._deviceList._size)
		return 0;

	// вернем идентификатор
	return api._deviceList._array[device];
}
#ifndef _USRDLL
//-----------------------------------------------------------------------------
// получение контекста
// на входе    :  
// на выходе   :  
//-----------------------------------------------------------------------------
context_t* api_GetContext(void* id)
{
	// инициализация ядра
	api_Startup();

	for (int i = 0; i < api._contextList._size; i++)
		if (api._contextList._array[i] != 0)
			if (api._contextList._array[i]->_id == id)
				return api._contextList._array[i];
	return 0;
}
//-----------------------------------------------------------------------------
// создание контекста
// на входе    :  
// на выходе   :  
//-----------------------------------------------------------------------------
context_t* api_CreateContext(void* id)
{
	int i;
	context_t* context;

	// инициализация ядра
	api_Startup();

	// новое устройство есть где разместить ?
	if (api._contextList._size == api._contextList._capacity)
		context_GrowArray();

	// создадим новую память под новое устройство
	context = (context_t *) malloc(sizeof(context_t));
	memset(context, 0, sizeof(context_t));

	// запомним идентификатор
	context->_id = id;

	// включение нового элемента в массив
	i = api._contextList._size++;
	api._contextList._array[i] = context;
	return context;
}

//-----------------------------------------------------------------------------
// удаление контекста
// на входе    :  
// на выходе   :  
//-----------------------------------------------------------------------------
void api_ReleaseContext(void* id)
{
	// поиск нужного контекста
	for (int i = 0; i < api._contextList._size; i++) {
		// нужный контекст ?
		if (api._contextList._array[i]->_id == id) {
			// освободим контекст
			free(api._contextList._array[i]);
			// очистим ячейку
			api._contextList._array[i] = 0;
			api._contextList._size--;
			return;
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Инициализация файла отчета
//	на входе :	filename - указатель на имя файла
//	на выходе:	*
//-----------------------------------------------------------------------------
void LogInit(char* filename)
{
	// переменные
	FILE* file = 0;

	// проверка на инициализацию
	if (api._log_init)
		return;

	// создание мутекса
	while (api._log_mutex == 0)
		api._log_mutex = CreateMutex(0, 0, 0);

	// блокировка доступа к лог файла
	WaitForSingleObject(api._log_mutex, INFINITE);

	// получение стартового времени
	api._log_start_time = timeGetTime();

	// копирование имени файла
	strcpy(api._log_file_name, filename);

	// открытие лог файла
	file = fopen(api._log_file_name, "wb");

	// закроем файл
	fclose(file);

	// лог файл инициализирован
	api._log_init = true;

	// разблоктруем доступ
	ReleaseMutex(api._log_mutex);
}

//-----------------------------------------------------------------------------
// Освобождение файла отчета
//	на входе :	*
//	на выходе:	*
//-----------------------------------------------------------------------------
void LogFree(void)
{
	// если инициализации не было выйдем
	if (!api._log_init)
		return;

	// блокировка доступа к лог файла
	WaitForSingleObject(api._log_mutex, INFINITE);

	// лог закрыт
	api._log_init = false;

	// очистка имени файла
	memset(api._log_file_name, 0, sizeof(api._log_file_name));

	// разблоктруем доступ
	ReleaseMutex(api._log_mutex);

	// освобождение мутекса
	CloseHandle(api._log_mutex);
}

//-----------------------------------------------------------------------------
// Запись строки в отчет
//	на входе :	указатель на строку
//	на выходе:	*
//-----------------------------------------------------------------------------
void api_Log(const char* string, ...)
{
	va_list ap;
	FILE* file;
	char temp[1024];

	// инициализация ядра
	api_Startup();

	// проверка параметров
	if ((!api._log_init) || (!string))
		return;

	// блокировка доступа
	WaitForSingleObject(api._log_mutex, INFINITE);

	temp[0] = 0;

	// формирование строки
	va_start(ap, string);
	vsprintf(temp, string, ap);
	va_end(ap);

	file = fopen(api._log_file_name, "ab+");
	fprintf(file, "%s\n", temp);
	fflush(file);
	fclose(file);

	// разблокирование доступа
	ReleaseMutex(api._log_mutex);
}

//-----------------------------------------------------------------------------
// Запись строки в отчет с добавлением времени
//	на входе :	указатель на строку
//	на выходе:	*
//-----------------------------------------------------------------------------
void api_LogTime(const char* string, ...)
{
	va_list ap;
	FILE* file;
	char temp[1024];
	unsigned int   hour, min, sec, millisec;

	// инициализация ядра
	api_Startup();

	// проверка параметров
	if ((!api._log_init) || (!string))
		return;

	// блокировка доступа
	WaitForSingleObject(api._log_mutex, INFINITE);

	// создание данных для строки с временем
	millisec = timeGetTime() - api._log_start_time;
	hour = millisec / 3600000;
	millisec -= hour * 3600000;
	min = millisec / 60000;
	millisec -= min * 60000;
	sec = millisec / 1000;
	millisec -= sec * 1000;

	temp[0] = 0;

	// формирование строки
	va_start(ap, string);
	vsprintf(temp, string, ap);
	va_end(ap);

	file = fopen(api._log_file_name, "ab+");
	fprintf(file,
		"[%.2d:%.2d:%.2d.%.3d]\t%s\n",
		hour,
		min,
		sec,
		millisec,
		temp);
	fflush(file);
	fclose(file);

	// разблокирование доступа
	ReleaseMutex(api._log_mutex);
}

//-----------------------------------------------------------------------------
// Инициализация параметров звука по умолчанию
// на входе    :  in	- указатель на входные параметры звука по умолчанию
//  			  out   - указатель на выходные параметры звука по умолчанию
// на выходе   :  *
//-----------------------------------------------------------------------------
void InitSoundDefault(squall_sample_default_t* in,
	squall_sample_default_t* out, unsigned int frequency)
{
	// проверка наличия входных параметров
	if (in) {
		// скопируем параметры
		memcpy(out, in, sizeof(squall_sample_default_t));

		// проверим частоту дискретизации
		if (out->Frequency == 0)
			out->Frequency = frequency;
		else if (out->Frequency < 100)
			out->Frequency = 100;
		else if (out->Frequency > 100000)
			out->Frequency = 100000;

		// проверим громкость
		if (out->Volume > 100)
			out->Volume = 100;

		// проверим панораму
		if (out->Pan > 100)
			out->Pan = 50;

		// проверим минимальное расстояние слышимости
		if (out->MinDist <= 0.0f)
			out->MinDist = DS3D_DEFAULTMINDISTANCE;

		// проверим максимальное расстояние слышимости
		if (out->MaxDist <= 0.0f)
			out->MaxDist = DS3D_DEFAULTMAXDISTANCE;
	} else {
		// заполним поля структуры по умолчанию
		out->SampleGroupID = 0;
		out->Frequency = frequency;
		out->Volume = 100;
		out->Pan = 50;
		out->Priority = 0;
		out->MinDist = DS3D_DEFAULTMINDISTANCE;
		out->MaxDist = DS3D_DEFAULTMAXDISTANCE;
	}
}

//-----------------------------------------------------------------------------
// Блокирование доступа к данным семпла
// на входе    :  *
// на выходе   :  *
//-----------------------------------------------------------------------------
void SampleLock(void)
{
	WaitForSingleObject(api._sample_mutex, INFINITE);
}

//-----------------------------------------------------------------------------
// Разблокирование доступа к данным семпла
// на входе    :  *
// на выходе   :  *
//-----------------------------------------------------------------------------
void SampleUnlock(void)
{
	ReleaseMutex(api._sample_mutex);
}

//-----------------------------------------------------------------------------
// Получение указателя на данные семпла
// на входе    :  *
// на выходе   :  *
//-----------------------------------------------------------------------------
sample_t* GetSample(int id)
{
	unsigned char  count;
	int index;

	count = id >> 16;
	index = id & 0xFFFF;

	if (index >= api._sampleList._size)
		return 0;

	if (api._sampleList._array[index]->_count != count)
		return 0;

	if (!api._sampleList._array[index]->_file)
		return 0;

	return api._sampleList._array[index];
}

//-----------------------------------------------------------------------------
// Создание семпла из файла
// на входе    :  *
// на выходе   :  *
//-----------------------------------------------------------------------------
int api_SampleLoadFile(const char* filename, int mem_flag,
	squall_sample_default_t* def)
{
	int index, hole, equal;
	sample_t* sample;
	CSoundFile* new_sample;

	// инициализация ядра
	api_Startup();

	// поиск дырки и подобных файлов
	for (equal = -1, hole = -1, index = 0;
		index < api._sampleList._size;
		index++) {
		// элемент пуст ?
		if (api._sampleList._array[index]->_file != 0) {
			// подобный найден ?
			if (equal == -1) {
				// файлы подобны ?
				if (strcmp(filename,
						api._sampleList._array[index]->_file->GetFileName()) ==
					0) {
					equal = index;
					if (hole != -1)
						break;
				}
			}
		} else {
			hole = index;
			if (equal != -1)
				break;
		}
	}

	// если количество элементов больше либо равно 65535 и дырки нет превышен прелел
	if ((hole == -1) && (api._sampleList._size >= 65535))
		return 0;

	// новый семпл есть где разместить ?
	if (api._sampleList._size == api._sampleList._capacity)
		sample_GrowArray();

	// дырка есть ?
	if (hole == -1) {
		// создадим новую память под новый семпл
		sample = (sample_t *) malloc(sizeof(sample_t));
		memset(sample, 0, sizeof(sample_t));
		index = api._sampleList._size++;
		// включение нового элемента в массив
		api._sampleList._array[index] = sample;
	} else {
		index = hole;
	}


	// подобный файл есть ?
	if (equal == -1) {
		// создание нового файла
		new_sample = new CSoundFile();

		// попытаемся создать звуковые данные
		if (!new_sample->CreateSoundDataFromFile(filename, mem_flag)) {
			// ошибка создания данных освободим все занятые ресурсы
			delete new_sample;  				   // удаление звукового файла
			return 0;
		}
	} else {
		// создание ссылки
		new_sample = api._sampleList._array[equal]->_file;
	}

	// запишем новый элемент
	api._sampleList._array[index]->_file = new_sample;
	// добавление ссылки на данные
	api._sampleList._array[index]->_file->AddRef();

	// скопируем параметры по умолчанию
	InitSoundDefault(def,
		& api._sampleList._array[index]->_def,
		new_sample->GetFormat()->nSamplesPerSec);

	// счетчик использования ячейки
	api._sampleList._array[index]->_count++;

	// вернем индекс созданного элемента
	return index | api._sampleList._array[index]->_count << 16;
}

//-----------------------------------------------------------------------------
// Создание семпла из памяти
// на входе    :  *
// на выходе   :  *
//-----------------------------------------------------------------------------
int api_CreateSampleFromMemory(void* ptr, unsigned int size, int new_mem,
	squall_sample_default_t* def)
{
	int index, hole;
	sample_t* sample;
	CSoundFile* new_sample;

	// инициализация ядра
	api_Startup();

	// поиск дырки и подобных файлов
	for (hole = -1, index = 0; index < api._sampleList._size; index++) {
		// элемент пуст ?
		if (api._sampleList._array[index]->_file == 0) {
			hole = index;
			break;
		}
	}

	// если количество элементов больше либо равно 65535 и дырки нет превышен прелел
	if ((hole == -1) && (api._sampleList._size >= 65535))
		return 0;

	// новый семпл есть где разместить ?
	if (api._sampleList._size == api._sampleList._capacity)
		sample_GrowArray();

	// дырка есть ?
	if (hole == -1) {
		// создадим новую память под новый семпл
		sample = (sample_t *) malloc(sizeof(sample_t));
		memset(sample, 0, sizeof(sample_t));
		index = api._sampleList._size++;
		// включение нового элемента в массив
		api._sampleList._array[index] = sample;
	} else {
		index = hole;
	}


	// создание нового файла
	new_sample = new CSoundFile();

	// попытаемся создать звуковые данные
	if (!new_sample->CreateSoundDataFromMemory(ptr, size, new_mem)) {
		// ошибка создания данных освободим все занятые ресурсы
		delete new_sample;  				   // удаление звукового файла
		return 0;
	}

	// запишем новый элемент
	api._sampleList._array[index]->_file = new_sample;
	// добавление ссылки на данные
	api._sampleList._array[index]->_file->AddRef();

	// скопируем параметры по умолчанию
	InitSoundDefault(def,
		& api._sampleList._array[index]->_def,
		new_sample->GetFormat()->nSamplesPerSec);

	// счетчик использования ячейки
	api._sampleList._array[index]->_count++;

	// вернем индекс созданного элемента
	return index | api._sampleList._array[index]->_count << 16;
}

//-----------------------------------------------------------------------------
// Освобождение семпла
// на входе    :  id - идентификатор освобождаемого семпла
// на выходе   :  *
//-----------------------------------------------------------------------------
void api_SampleUnload(int id)
{
	sample_t* sample;

	// блокируем доступ
	SampleLock();

	// проверка наличия семпла
	sample = GetSample(id);
	if (sample) {
		// удалим ссылку на файл
		if (sample->_file->DecRef() == 0)
			delete sample->_file;

		// создание дырки
		sample->_file = 0;
	}

	// разблокируем доступ
	SampleUnlock();
}

//-----------------------------------------------------------------------------
// Освобождение всех семплов
// на входе    :  *
// на выходе   :  *
//-----------------------------------------------------------------------------
void api_SampleUnloadAll(void)
{
	// блокируем доступ
	SampleLock();

	// удаление всех файлов из хранилища
	for (int i = 0; i < api._sampleList._size; i++) {
		if (api._sampleList._array[i]->_file != 0)
			if (api._sampleList._array[i]->_file->DecRef() == 0)
				delete api._sampleList._array[i]->_file;
		api._sampleList._array[i]->_file = 0;
	}

	// разблокируем доступ
	SampleUnlock();
}

//-----------------------------------------------------------------------------
// Получение указателя на данные семпла
// на входе    :  id - идентификатор файла
// на выходе   :  указатель на данные файла, в случае ошибки 0
//-----------------------------------------------------------------------------
CSoundFile* api_SampleGetFile(int id)
{
	sample_t* sample;
	CSoundFile* file;

	// блокируем доступ
	SampleLock();

	// проверка наличия семпла
	sample = GetSample(id);
	if (!sample)
		file = 0;
	else
		file = sample->_file;

	// разблокируем доступ
	SampleUnlock();

	return file;
}

//-----------------------------------------------------------------------------
// Получение указателя на данные семпла из группы
// на входе    :  id	-  идентификатор группы
//  			  num   -  номер файла в группе
// на выходе   :  идентификатор семпла
//-----------------------------------------------------------------------------
int api_SampleGetFileGroup(int id, unsigned int num)
{
	// переменные
	int index = 0;
	int count = 0;

	// блокируем доступ
	SampleLock();

	// составление списка
	for (count = 0, index = 0; index < api._sampleList._size; index++) {
		// нужный элемент
		if ((api._sampleList._array[index]->_file != 0) &&
			(api._sampleList._array[index]->_def.SampleGroupID == id)) {
			// проверка на переполнение
			if (count >= SAMPLES_IN_GROUP)
				break;
			api_temp[count] = index;
			count++;
		}
	}

	// выберем случайный семпл
	if (count != 0) {
		index = api_temp[rand() % count];
		index |= api._sampleList._array[index]->_count << 16;
	} else
		index = 0;

	// разблокируем доступ
	SampleUnlock();

	// вернем индекс найденного элемента
	return index;
}

//-----------------------------------------------------------------------------
// Установка параметров семпла по умолчанию
// на входе    :  id	-  идентификатор семпла
//  			  def   -  указатель на данные семпла по умолчанию
// на выходе   :  успешность выполнения
//-----------------------------------------------------------------------------
int api_SampleSetDefault(int id, squall_sample_default_t* def)
{
	sample_t* sample;
	int ret = false;

	// блокирование доступа
	SampleLock();

	// получение указателя на семпл
	sample = GetSample(id);
	if (sample) {
		// скопируем параметры по умолчанию
		InitSoundDefault(def,
			& sample->_def,
			sample->_file->GetFormat()->nSamplesPerSec);
		ret = true;
	}

	// разблокируем доступ
	SampleUnlock();

	return ret;
}

//-----------------------------------------------------------------------------
// Получение параметров семпла по умолчанию
// на входе    :  id	-  идентификатор семпла
//  			  def   -  указатель на данные семпла по умолчанию
// на выходе   :  успешность выполнения
//-----------------------------------------------------------------------------
int api_SampleGetDefault(int id, squall_sample_default_t* def)
{
	sample_t* sample;
	int ret = false;

	// блокирование доступа
	SampleLock();

	// получение указателя на семпл
	sample = GetSample(id);
	if (sample) {
		// скопируем параметры по умолчанию
		memcpy(def, & sample->_def, sizeof(squall_sample_default_t));
		ret = true;
	}

	// разблокируем доступ
	SampleUnlock();

	return ret;
}

//-----------------------------------------------------------------------------
// Инициализация данных ядра
// на входе    :  *
// на выходе   :  *
//-----------------------------------------------------------------------------
void api_Startup(void)
{
	// ядро инициализировано ?
	if (api_init == 0) {
		// подготовка ядра к работе
		memset(&api, 0, sizeof(api));

		// установим функции для работы с памятью и файлами
		api._open = api_open;
		api._read = api_read;
		api._seek = api_seek;
		api._close = api_close;
		api._malloc = api_malloc;
		api._free = api_free;

		// отладочная информация
#if SQUALL_DEBUG

		// получение текущей директории
		char LogFileName[4096];
		GetCurrentDirectory(sizeof(LogFileName), LogFileName);

		// создадим файл отчета
		strcat(LogFileName, "\\Squall.log");

		// инициализация файла отчета
		LogInit(LogFileName);
#endif
		// создание списка устройств воспроизведения
		device_GrowArray();

		// создание списка контекстов
#ifndef _USRDLL
		context_GrowArray();
#endif

		// создание мутекса для системы хранилища
		while (api._sample_mutex == 0)
			api._sample_mutex = CreateMutex(0, 0, 0);

		// заблокируем доступ к хранилищу
		SampleLock();

		// создание списка семплов
		sample_GrowArray();

		// разблоктруем доступ к хранилищу
		SampleUnlock();

		// создание списка устройств воспроизведения
		api_CreateDeviceList();

		// ядро инициализировано
		api_init = 1;

		// отладочная информация
#if SQUALL_DEBUG

		// переменные
		device_t* dev;

		// создание отчета по устройствам воспроизведения
		for (int i = 0; i < api_GetNumDevice(); i++) {
			dev = api_GetDevice(i);
			api_Log("[Устройство №%d] %s", i, dev->_name);

			if (dev->_caps.dwFlags & DSCAPS_CERTIFIED)
				api_Log("Устройство протестировано и сертифицировано Microsoft");
			if (dev->_caps.dwFlags & DSCAPS_EMULDRIVER)
				api_Log("Не имеет DirectX драйвера");

			api_Log("\t[Аппаратные возможности]");
			if (dev->_caps.dwFlags & DSCAPS_CONTINUOUSRATE) {
				api_Log("\t- Аппаратно поддерживаются частоты дискретизации, для вторичных буферов,\n\t  от %d до %d",
					dev->_caps.dwMinSecondarySampleRate,
					dev->_caps.dwMaxSecondarySampleRate);
			}

			api_Log("\t- Аппаратно поддерживаются следующие типы сигналов:");
			api_Log("\t\tТип буфера  8 Бит 16 Бит Моно Стерео ");
			api_Log("\t\tПервичный   %s   %s    %s  %s",
				(dev->_caps.dwFlags & DSCAPS_PRIMARY8BIT) ? "да " : "нет",
				(dev->_caps.dwFlags & DSCAPS_PRIMARY16BIT) ? "да " : "нет",
				(dev->_caps.dwFlags & DSCAPS_PRIMARYMONO) ? "да " : "нет",
				(dev->_caps.dwFlags & DSCAPS_PRIMARYSTEREO) ? "да " : "нет");
			api_Log("\t\tВторичный   %s   %s    %s  %s",
				(dev->_caps.dwFlags & DSCAPS_SECONDARY8BIT) ? "да " : "нет",
				(dev->_caps.dwFlags & DSCAPS_SECONDARY16BIT) ? "да " : "нет",
				(dev->_caps.dwFlags & DSCAPS_SECONDARYMONO) ? "да " : "нет",
				(dev->_caps.dwFlags & DSCAPS_SECONDARYSTEREO) ? "да " : "нет");

			api_Log("\t- Аппаратно поддерживаются следующее количество буферов:");
			api_Log("\t\t          2D смешивание [из них свободно] 3D смешивание [из них свобоно]");
			api_Log("\t\tВсего     %.2d            [%.2d]              %.2d            [%.2d]",
				dev->_caps.dwMaxHwMixingAllBuffers,
				dev->_caps.dwFreeHwMixingAllBuffers,
				dev->_caps.dwMaxHw3DAllBuffers,
				dev->_caps.dwFreeHw3DAllBuffers);
			api_Log("\t\tСтатичных %.2d            [%.2d]              %.2d            [%.2d]",
				dev->_caps.dwMaxHwMixingStaticBuffers,
				dev->_caps.dwFreeHwMixingStaticBuffers,
				dev->_caps.dwMaxHw3DStaticBuffers,
				dev->_caps.dwFreeHw3DStaticBuffers);
			api_Log("\t\tПотоковых %.2d            [%.2d]              %.2d            [%.2d]",
				dev->_caps.dwMaxHwMixingStreamingBuffers,
				dev->_caps.dwFreeHwMixingStreamingBuffers,
				dev->_caps.dwMaxHw3DStreamingBuffers,
				dev->_caps.dwFreeHw3DStreamingBuffers);

			api_Log("\t- Количество первичных буферов         = %d",
				dev->_caps.dwPrimaryBuffers);
			api_Log("\t- Объем всей памяти на устройстве      = %d",
				dev->_caps.dwTotalHwMemBytes);
			api_Log("\t- Объем свободной памяти на устройстве = %d",
				dev->_caps.dwFreeHwMemBytes);
			//  	 api_Log("\tdwMaxContigFreeHwMemBytes      = %d", cur->DeviceInfo.dwMaxContigFreeHwMemBytes);
			//  	 api_Log("\tdwUnlockTransferRateHwBuffers  = %d", cur->DeviceInfo.dwUnlockTransferRateHwBuffers);
			//  	 api_Log("\tdwPlayCpuOverheadSwBuffers     = %d", cur->DeviceInfo.dwPlayCpuOverheadSwBuffers);

			// выведем информацию о звуковых расширениях карты
			api_Log("\t- Звуковые расширения");
			for (int eax = 1; eax <= EAX_NUM; eax++)
				api_Log("\t\tEAX %d.0 %s",
					eax,
					dev->_eax[eax] ? "Поддерживается" : "Не поддерживается");

			// ZOOMFX
			api_Log("\t\tZOOMFX %s",
				dev->_zoomfx ? "Поддерживается" : "Не поддерживается");

			// выведем список поддерживаемых 3D агроритмов
			api_Log("\t- Поддерживаемые 3D алгоритмы");
			if (dev->_alg[1])
				api_Log("\t\tALG_3D_OFF");
			if (dev->_alg[2])
				api_Log("\t\tALG_3D_FULL");
			if (dev->_alg[3])
				api_Log("\t\tALG_3D_LIGTH");
		}
#endif
	}
}

//-----------------------------------------------------------------------------
// Освобождение данных ядра
// на входе    :  *
// на выходе   :  *
//-----------------------------------------------------------------------------
void api_Shutdown(void)
{
	int i;
	// ядро инициализировано ?
	if (api_init == 1) {
#ifndef _USRDLL
		// контексты есть ?
		if (api._contextList._size == 0) {
#endif
			// выгрузка всех семплов
			api_SampleUnloadAll();

			// удаление мутекса для блокировки семплов
			CloseHandle(api._sample_mutex);
			api._sample_mutex = 0;

			// удаление семплов
			for (i = 0; i < api._sampleList._size; i++)
				free(api._sampleList._array[i]);

			// удаление массива семплов
			free(api._sampleList._array);

#ifndef _USRDLL
			// удаление элементов списка контекстов
			for (i = 0; i < api._contextList._size; i++)
				free(api._contextList._array[i]);
			// удаление массива контекстов
			free(api._contextList._array);
#endif

			// удаление элементов списка устройств
			for (i = 0; i < api._deviceList._size; i++)
				free(api._deviceList._array[i]);

			// удаление массива устройств
			free(api._deviceList._array);

			// освобождение файла отчета
			LogFree();

			// очистка ядра
			memset(&api, 0, sizeof(api));

			// ядро инициализировано
			api_init = 0;
#ifndef _USRDLL
		}
#endif
	}
}

//-----------------------------------------------------------------------------
// Установка внешних функций работы с файлом
// на входе    :  ext_open    - указатель на функцию открытия файла
//  			  ext_read    - указатель на функцию чтения файла
//  			  ext_seek    - указатель на функцию позиционирования в файле
//  			  ext_close   - указатель на функцию закрытия файла
// на выходе   :  успешность установки
//-----------------------------------------------------------------------------
void api_SetFileCallbacks(extern_open_t ext_open, extern_read_t ext_read,
	extern_seek_t ext_seek, extern_close_t ext_close)
{
	// инициализация ядра
	api_Startup();

	// блокируем доступ к данным
	SampleLock();

	// запишем указатели на методы работы с файлами
	api._open = ext_open;
	api._read = ext_read;
	api._seek = ext_seek;
	api._close = ext_close;

	// разблокируем доступ к данным
	SampleUnlock();
}

//-----------------------------------------------------------------------------
// Установка внешних функций работы с памятью
// на входе    :  ext_malloc  -  указатель на функцию выделения памяти
//  			  ext_free    -  указатель на функцию освобождения памяти
// на выходе   :  успешность установки
//-----------------------------------------------------------------------------
void api_SetMemoryCallbacks(extern_malloc_t ext_malloc, extern_free_t ext_free)
{
	// инициализация ядра
	api_Startup();

	// блокируем доступ к данным
	SampleLock();

	// установим методы для работы с памятью
	api._malloc = ext_malloc;
	api._free = ext_free;

	// разблокируем доступ к данным
	SampleUnlock();
}
