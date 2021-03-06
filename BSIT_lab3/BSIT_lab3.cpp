#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <shlwapi.h>
#include <tchar.h>
#include <stdio.h>
#include <regex>
#include <locale.h>
#pragma comment (lib, "advapi32.lib")
#pragma comment (lib, "Shlwapi.lib")
#pragma comment (lib, "rpcrt4.lib")
#pragma comment (lib, "User32.lib")

LPSTR serviceName = (LPSTR)"BSIT3";

SERVICE_STATUS serviceStatus;
SERVICE_STATUS_HANDLE serviceStatusHandle = 0;
HANDLE stopServiceEvent = NULL;
char folder[500] = { 0 };
char archive_path[500] = { 0 };
char archive_name[500] = { 0 };

char **masks = NULL;
char **filenames = NULL;
int num_of_masks = 0;
int num_of_filenames = 0;

void freeNames(char **names, int cnt)
{
	for (int i = 0; i < cnt; i++)
	{
		free(names[i]);
	}
	free(names);
}

void parseIniFile(char *path)
{
	int size;
	size = GetPrivateProfileStringA("Paths", "Folder", NULL, folder, sizeof(folder), path);
	if (size == 0 && folder[0] == '\0')
	{
		exit(0);
	}
	size = GetPrivateProfileStringA("Paths", "ArchivePath", NULL, archive_path, sizeof(archive_path), path);
	if (size == 0 && folder[0] == '\0')
	{
		exit(0);
	}
	strcpy(archive_name, PathFindFileNameA(archive_path));
	int cnt = 0;
	char keyname[15] = { 0 };
	masks = (char**)malloc(sizeof(char*)*(cnt + 1));
	do
	{
		sprintf(keyname, "f%d", cnt + 1);
		char tmp[500] = { 0 };
		size = GetPrivateProfileStringA("Masks", keyname, NULL, tmp, sizeof(tmp), path);
		if (size == 0 && tmp[0] == '\0')
		{
			break;
		}
		masks = (char**)realloc(masks, sizeof(char*)*(cnt + 1));
		masks[cnt] = (char*)calloc(size + 1, sizeof(char));
		memcpy(masks[cnt], tmp, size);
		cnt++;

	} while (1);
	if (cnt == 0)
	{
		free(masks);
	}
	num_of_masks = cnt;
	cnt = 0;
	filenames = (char**)malloc(sizeof(char*)*(cnt + 1));
	do
	{
		sprintf(keyname, "f%d", cnt + 1);
		char tmp[500] = { 0 };
		size = GetPrivateProfileStringA("Files", keyname, NULL, tmp, sizeof(tmp), path);
		if (size == 0 && tmp[0] == '\0')
		{
			break;
		}
		filenames = (char**)realloc(filenames, sizeof(char*)*(cnt + 1));
		filenames[cnt] = (char*)calloc(size + 1, sizeof(char));
		memcpy(filenames[cnt], tmp, size);
		cnt++;

	} while (1);
	if (cnt == 0)
	{
		free(filenames);
	}
	num_of_filenames = cnt;
}

char **parseFilesInFolder(char *folder_path, int &cnt)
{
	while(1);
	char *search_path = (char*)malloc(strlen(folder_path) + 3);
	strcpy(search_path, folder_path);
	strcat(search_path, "\\*");
	char **names;
	cnt = 0;

	HANDLE search;
	_WIN32_FIND_DATAA info;
	search = FindFirstFileA(search_path, &info);
	if (search == INVALID_HANDLE_VALUE)
	{
		return NULL;
	}
	bool found = false;
	names = (char**)malloc(sizeof(char*));
	do
	{
		if (!strcmp(info.cFileName, archive_name))
		{
			continue;
		}
		for (int i = 0; i < num_of_masks; i++)
		{
			std::regex format(masks[i]);
			if (std::regex_match(info.cFileName, format))
			{
				names[cnt] = (char*)malloc(strlen(info.cFileName) + 1);
				strcpy(names[cnt], info.cFileName);
				cnt++;
				names = (char**)realloc(names, sizeof(char*)*(cnt + 1));
				found = true;
				break;
			}
		}
		if (found)
		{
			found = false;
			continue;
		}
		for (int i = 0; i < num_of_filenames; i++)
		{
			if (!strcmp(info.cFileName, filenames[i]))
			{
				names[cnt] = (char*)malloc(strlen(info.cFileName) + 1);
				strcpy(names[cnt], info.cFileName);
				cnt++;
				names = (char**)realloc(names, sizeof(char*)*(cnt + 1));
				break;
			}
		}

	} while (FindNextFileA(search, &info));
	if (cnt == 0)
	{
		free(names);
		return NULL;
	}
	return names;
}

void zipFiles(char *archiveName)
{
	while (1);
	DeleteFileA(archive_path);
	char **names;
	int num = 0;
	names = parseFilesInFolder(folder, num);
	char command[10000];
	int pos = 0;
	pos += sprintf(command, " Compress-Archive -LiteralPath \"\"\"%s\"\"\"", names[0]);
	num--;
	while (num)
	{
		pos += sprintf(command + pos, ", \"\"\"%s\"\"\"", names[num--]);
	}
	pos += sprintf(command + pos, " -DestinationPath %s", archiveName);
	_PROCESS_INFORMATION info;
	STARTUPINFOA startinfo = { 0 };
	startinfo.cb = sizeof(STARTUPINFOA);
	if (!CreateProcessA("C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe", command, NULL, NULL, FALSE, REALTIME_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, /*"C:\\test"*/folder/*"C:\\Users\\Elvin\\source\\repos\\BSIT_lab3\\"*/, &startinfo, &info))//это фолдер
	{

	}
	freeNames(names, num);
	WaitForSingleObject(info.hThread, INFINITE);
}


bool parseFileChanges(FILE_NOTIFY_INFORMATION *info)
{
	FILE_NOTIFY_INFORMATION *next = info;
	do
	{
		for (int i = 0; i < num_of_masks; i++)
		{
			std::regex format(masks[i]);

			wchar_t name[500] = { 0 };
			memcpy(name, next->FileName, next->FileNameLength);
			char nameA[1000];
			sprintf(nameA, "%S", name);
			if (!strcmp(nameA, archive_name))
			{
				if (next->NextEntryOffset == 0)
				{
					break;
				}
				next = (FILE_NOTIFY_INFORMATION *)((char*)next + next->NextEntryOffset);
				continue;
			}
			if (std::regex_match(nameA, format))
			{
				return true;
			}

		}
		for (int i = 0; i < num_of_filenames; i++)
		{

			wchar_t name[500] = { 0 };
			memcpy(name, next->FileName, next->FileNameLength);
			char nameA[1000];
			sprintf(nameA, "%S", name);
			if (!strcmp(nameA, archive_name))
			{
				if (next->NextEntryOffset == 0)
				{
					break;
				}
				next = (FILE_NOTIFY_INFORMATION *)((char*)next + next->NextEntryOffset);
				continue;
			}
			if (!strcmp(nameA, filenames[i]))
			{
				return true;
			}
		}
		if (next->NextEntryOffset == 0)
		{
			break;
		}
		next = (FILE_NOTIFY_INFORMATION *)((char*)next + next->NextEntryOffset);
	} while (1);
	return false;
}


int other_main()
{
	//while (1);
	setlocale(LC_ALL, "Russian");
	//parseIniFile((char*)"C:\\test\\conf.ini");
	parseIniFile((char*)"C:\\Users\\Elvin\\source\\repos\\BSIT_lab3\\test.ini");
	zipFiles(archive_path);
	HANDLE f;
	f = CreateFileA(folder, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
					NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
	if (f == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	OVERLAPPED asynch = { 0 };
	asynch.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
	HANDLE events[2] = { stopServiceEvent,asynch.hEvent };
	DWORD res;
	BOOL success;
	BYTE buf[10000] = { 0 };
	DWORD size = sizeof(buf);
	DWORD flags = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES
		| FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION;

	while (1)
	{
		success = ReadDirectoryChangesW(f, buf, sizeof(buf), FALSE, flags, &size, &asynch, NULL);
		if (!success)
		{
			break;
		}
		res = WaitForMultipleObjects(2, events, FALSE, INFINITE);
		if (res != WAIT_OBJECT_0 + 1)
		{
			break;
		}
		if (parseFileChanges((FILE_NOTIFY_INFORMATION*)buf))
		{

			zipFiles(archive_path);
		}
	}
	CloseHandle(f);
	CloseHandle(events[1]);
	return 0;
}
void WINAPI ServiceControlHandler(DWORD controlCode)// функция обработки запросов (описание корректного завершения работы сервиса)
{
	switch (controlCode)
	{
	case SERVICE_CONTROL_INTERROGATE:
	{
		break;
	}
	case SERVICE_CONTROL_SHUTDOWN:
	{
	}
	case SERVICE_CONTROL_STOP:// остановка сервиса
	{
		serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;//Служба находится в процессе остановки
		SetEvent(stopServiceEvent);// сигнальное состояние
		SetServiceStatus(serviceStatusHandle, &serviceStatus);// обновляем статус service control manager
		return;
	}
	case SERVICE_CONTROL_PAUSE:
	{
		break;
	}
	case SERVICE_CONTROL_CONTINUE:
	{
		break;
	}
	}
	SetServiceStatus(serviceStatusHandle, &serviceStatus);
}

void WINAPI ServiceMain(DWORD argc, TCHAR* argv[])// оповещаем SCM о текущем статусе сервиса
{
	
	serviceStatus.dwServiceType = SERVICE_WIN32;
	serviceStatus.dwCurrentState = SERVICE_STOPPED;  //shutdown , служба не работает
	serviceStatus.dwControlsAccepted = 0;//вынесенный shutdown case
	serviceStatus.dwWin32ExitCode = NO_ERROR;
	serviceStatus.dwServiceSpecificExitCode = NO_ERROR;
	serviceStatus.dwCheckPoint = 0;
	serviceStatus.dwWaitHint = 0;

	serviceStatusHandle = RegisterServiceCtrlHandlerA(serviceName, ServiceControlHandler);//обработка управляющих запросов регистрацией RegisterServiceCtrlHandlerA
	if (serviceStatusHandle)
	{

		serviceStatus.dwCurrentState = SERVICE_START_PENDING;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);

		stopServiceEvent = CreateEventA(0, FALSE, FALSE, 0);

		serviceStatus.dwControlsAccepted |= (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
		serviceStatus.dwCurrentState = SERVICE_RUNNING;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);
		other_main();

		serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);

		CloseHandle(stopServiceEvent);
		stopServiceEvent = 0;

		serviceStatus.dwControlsAccepted &= ~(SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
		serviceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);
	}
	freeNames(masks, num_of_masks);
	freeNames(filenames, num_of_filenames);

}
void Service()
{

	SERVICE_TABLE_ENTRY serviceTable[] = {
											{ serviceName, ServiceMain },
											{ 0, 0 } };

	StartServiceCtrlDispatcherA(serviceTable);//связывает сервис с SCM
}
void RunService()
{
	SC_HANDLE serviceControlManager = OpenSCManagerA(0, 0, SC_MANAGER_CONNECT);
	if (serviceControlManager)
	{

		SC_HANDLE hService = OpenServiceA(serviceControlManager, serviceName, SERVICE_ALL_ACCESS);
		if (hService)
		{
			if (!StartServiceA(hService, NULL, NULL))
			{
				printf("ERROR: Start service\n");
				return;
			}
			else
			{
				printf("OK: Start service\n");
			}
		}
	}
	CloseServiceHandle(serviceControlManager);
}
void StopService(void)
{
	SC_HANDLE serviceControlManager = OpenSCManagerA(0, 0, SC_MANAGER_CONNECT);
	if (serviceControlManager)
	{
		SC_HANDLE hService = OpenServiceA(serviceControlManager, serviceName, SERVICE_STOP);
		if (hService)
		{
			SERVICE_STATUS ss;
			if (!ControlService(hService, SERVICE_CONTROL_STOP, &ss))
			{
				printf("ERROR: Stop service\n");
				return;
			}
			else
			{
				printf("OK: Stop service\n");
			}
		}
	}
	CloseServiceHandle(serviceControlManager);
}
void InstallService()
{
	SC_HANDLE serviceControlManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (serviceControlManager)
	{
		TCHAR path[_MAX_PATH + 1];
		if (GetModuleFileNameA(0, path, sizeof(path) / sizeof(path[0])) > 0)
		{
			SC_HANDLE service = CreateService(serviceControlManager,
											  serviceName,
											  serviceName,
											  SERVICE_ALL_ACCESS,
											  SERVICE_WIN32_OWN_PROCESS,
											  SERVICE_AUTO_START,
											  SERVICE_ERROR_IGNORE,
											  path,
											  0, 0, 0, 0, 0);
			if (service)
			{
				printf("OK: Install service\n");
				CloseServiceHandle(service);
			}
			else
			{
				printf("ERROR: Install service\n");
			}
		}
		CloseServiceHandle(serviceControlManager);
	}
}

void UninstallService()
{
	SC_HANDLE serviceControlManager = OpenSCManagerA(0, 0, SC_MANAGER_ALL_ACCESS);
	if (serviceControlManager)
	{
		SC_HANDLE service = OpenServiceA(serviceControlManager,
										 serviceName,
										 SERVICE_QUERY_STATUS | DELETE);
		if (service)
		{
			SERVICE_STATUS serviceStatus;
			if (QueryServiceStatus(service, &serviceStatus))
			{
				if (serviceStatus.dwCurrentState == SERVICE_STOPPED)
				{
					if (!DeleteService(service))
					{
						printf("ERROR: Delete service\n");
						return;
					}
					else
					{
						printf("OK: Delete service\n");
					}
				}
			}
			CloseServiceHandle(service);
		}
		else
		{
			printf("ERROR: OpenService\n");
		}
		CloseServiceHandle(serviceControlManager);
	}
}
int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "Russian");

	if (argc > 1 && _strcmpi(argv[1], "install") == 0)
	{
		InstallService();
	}
	else if (argc > 1 && _strcmpi(argv[1], "uninstall") == 0)
	{
		StopService();
		UninstallService();
	}
	else if (argc > 1 && _strcmpi(argv[1], "stop") == 0)
	{
		StopService();
	}
	else if (argc > 1 && _strcmpi(argv[1], "start") == 0)
	{
		RunService();
	}
	else
	{
		Service();
	}
	return 0;
}