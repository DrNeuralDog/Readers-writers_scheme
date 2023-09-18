#include <windows.h>
#include <iostream>
#include <string>


#define NUM_READERS 5 
#define NUM_WRITERS 2 
#define REPEATS 5 
#define READER_DELAY 100
#define WRITER_DELAY 200


HANDLE memory_mutex;
HANDLE data_ready_event;
HANDLE readers_sem;
HANDLE writers_sem;


LONG readers_count = 0;
LONG writers_count = 0;
LONG writers_waiting = 0;
int shared_memory = 0;

HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);


void print_message(char* actor, int id, int shared_message)
{
	wchar_t message[50];

	wsprintf(message, L"%s %d is writing : %d\n", actor, id, shared_message);
	DWORD length = wcslen(message);

	BOOL result = WriteConsole(console, message, length, NULL, NULL);
}


DWORD WINAPI reader(LPVOID lpParam)
{
	char actor[] = "Reader";
	int id_reader = (int)lpParam;

	for (int i = 0; i < REPEATS; i++)
	{
		WaitForSingleObject(memory_mutex, INFINITE);

		while (writers_count > 0 || writers_waiting > 0)  //Обеспечиваем приоритет писателя
		{
			ReleaseMutex(memory_mutex);
			WaitForSingleObject(readers_sem, INFINITE);
			WaitForSingleObject(memory_mutex, INFINITE);
		}

		InterlockedIncrement(&readers_count);

		ReleaseMutex(memory_mutex);


		print_message(actor, id_reader, shared_memory);


		Sleep(READER_DELAY); //Имитируем затраты времени на обработку чтения

		WaitForSingleObject(memory_mutex, INFINITE);

		InterlockedDecrement(&readers_count);

		if (readers_count == 0)
			ReleaseSemaphore(writers_sem, 1, NULL);

		ReleaseMutex(memory_mutex);

		Sleep(READER_DELAY); //Имитируем какую-либо дополнительную деятельность потока чтения
	}
	return 0;
}


DWORD WINAPI writer(LPVOID lpParam)
{
	char actor[] = "Writer";
	int id_writer = (int)lpParam;

	for (int i = 0; i < REPEATS; i++)
	{
		WaitForSingleObject(memory_mutex, INFINITE);

		InterlockedIncrement(&writers_waiting);

		while (writers_count > 0)
		{
			ReleaseMutex(memory_mutex);
			WaitForSingleObject(writers_sem, INFINITE);
			WaitForSingleObject(memory_mutex, INFINITE);
		}

		InterlockedDecrement(&writers_waiting);
		InterlockedIncrement(&writers_count);
		ReleaseMutex(memory_mutex);

		shared_memory = id_writer * 10 + i;


		print_message(actor, id_writer, shared_memory);


		Sleep(WRITER_DELAY);

		WaitForSingleObject(memory_mutex, INFINITE);
		InterlockedDecrement(&writers_count);

		if (writers_waiting > 0)
			ReleaseSemaphore(writers_sem, 1, NULL);
		else
			ReleaseSemaphore(readers_sem, NUM_READERS, NULL);

		ReleaseMutex(memory_mutex);

		Sleep(WRITER_DELAY);
	}
	return 0;
}

int main()
{
	HANDLE readers[NUM_READERS];
	HANDLE writers[NUM_WRITERS];

	memory_mutex = CreateMutex(NULL, FALSE, NULL);

	readers_sem = CreateSemaphore(NULL, 0, NUM_READERS, NULL);
	writers_sem = CreateSemaphore(NULL, 0, 1, NULL);

	for (int i = 0; i < NUM_READERS; i++)
	{
		readers[i] = CreateThread(NULL, 0, reader, (LPVOID)(i + 1), 0, NULL);
	}
	for (int i = 0; i < NUM_WRITERS; i++)
	{
		writers[i] = CreateThread(NULL, 0, writer, (LPVOID)(i + 1), 0, NULL);
	}

	WaitForMultipleObjects(NUM_READERS, readers, TRUE, INFINITE);
	WaitForMultipleObjects(NUM_WRITERS, writers, TRUE, INFINITE);

	for (int i = 0; i < NUM_READERS; i++)
	{
		CloseHandle(readers[i]);
	}
	for (int i = 0; i < NUM_WRITERS; i++)
	{
		CloseHandle(writers[i]);
	}

	CloseHandle(memory_mutex);
	CloseHandle(readers_sem);
	CloseHandle(writers_sem);

	return 0;
}

