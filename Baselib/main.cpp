
#include "ServiceInstance.h"
#include <WinSvc.h>

using namespace wt;

#ifndef _WT_NO_MAIN

void Init();
bool Install();
bool UnInstall();
void WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpszArg);
void WINAPI ServiceCtrl(DWORD dwOpcode);

SERVICE_STATUS_HANDLE    g_hServiceStatus;
SERVICE_STATUS           g_Status;

int main(int argc, char* pArgv[])
{
	Init();

	CServiceInstance* pRun = CServiceInstance::GetInstance();
	if (pRun == NULL)
	{
		printf("system exception, exit!\n");
		return 0;
	}
	if (!pRun->InitInstance())
	{
		printf("Init instance failed, exit!\n");
		return 0;
	}

	if (argc > 1)
	{
		std::string strArg = pArgv[1];
		if (strArg.compare("-d") == 0)
		{
			pRun->RunInstance();
		}
		else if (strArg.compare("-i") == 0)
		{
			Install();
		}
		else if (strArg.compare("-u") == 0)
		{
			// 卸载服务，调用ControlService执行SERVICE_CONTROL_STOP来通知
			UnInstall();
		}
		else
		{
			printf("Invalid argument, exit!\n");
		}

		return 0;
	}
	
	char szServiceName[256] = {0};
	memcpy(szServiceName, pRun->GetServiceName(), min(sizeof(szServiceName) - 1, strlen(pRun->GetServiceName())));

	SERVICE_TABLE_ENTRY st[] =  
	{  
		{ szServiceName, (LPSERVICE_MAIN_FUNCTION)ServiceMain },  
		{ NULL, NULL }  
	};

	if (!StartServiceCtrlDispatcher(st))
	{
		printf("Service start up failed, error: %d!\n", GetLastError());
	}

	return 0;
}

void Init()
{
	g_hServiceStatus = NULL;
	g_Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;  
	g_Status.dwCurrentState = SERVICE_START_PENDING;  
	g_Status.dwControlsAccepted = SERVICE_ACCEPT_STOP;  
	g_Status.dwWin32ExitCode = 0;  
	g_Status.dwServiceSpecificExitCode = 0;  
	g_Status.dwCheckPoint = 0;  
	g_Status.dwWaitHint = 0; 
}

bool Install()
{
	// 获取服务名称
	CServiceInstance* pRun = CServiceInstance::GetInstance();
	if (pRun == NULL)
	{
		return false;
	}
	if (strlen(pRun->GetServiceName()) == 0)
	{
		printf("not set service name\n");
		return false;
	}

	// 创建服务
	SC_HANDLE hSCManager;
	SC_HANDLE hService;
	char szBinPath[MAX_PATH + 1] = {0};
	if (!GetModuleFileName(NULL, szBinPath, MAX_PATH))
		return false;

	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hSCManager == NULL)
		return false;

	char szServiceName[256] = {0};
	char szDisplayName[256] = {0};
	char szDescription[256] = {0};
	memcpy(szServiceName, pRun->GetServiceName(), min(sizeof(szServiceName) - 1, strlen(pRun->GetServiceName())));
	memcpy(szDisplayName, pRun->GetDisplayName(), min(sizeof(szDisplayName) - 1, strlen(pRun->GetDisplayName())));
	memcpy(szDescription, pRun->GetDescription(), min(sizeof(szDescription) - 1, strlen(pRun->GetDescription())));

	hService = CreateService(hSCManager, szServiceName, szDisplayName, SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
		szBinPath, NULL, NULL, NULL, NULL, NULL);
	if (hService == NULL)
	{
		if (GetLastError() == ERROR_INVALID_NAME || 
			GetLastError() == ERROR_SERVICE_EXISTS)
		{
			printf("Service name invalid or exist!\n");
		}
		CloseServiceHandle(hSCManager);
		return false;
	}
	printf("Service is installed successfully!\n");

	// 修改服务描述
	SERVICE_DESCRIPTION description = {0};
	description.lpDescription = szDescription;
	ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, &description);

	if (!StartService(hService, 0, NULL))
	{
		printf("Service start failed");
	}
	else
	{
		printf("Service start successfully!\n");
	}

	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);

	return true;
}

void WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpszArg)
{
	CServiceInstance* pRun = CServiceInstance::GetInstance();
	if (pRun == NULL)
		return;

	g_hServiceStatus = RegisterServiceCtrlHandler(pRun->GetServiceName(), ServiceCtrl);
	if (g_hServiceStatus == NULL)
		return;

	g_Status.dwWin32ExitCode = S_OK;
	g_Status.dwCurrentState = SERVICE_RUNNING;
	if (!SetServiceStatus(g_hServiceStatus, &g_Status))
		return;

	pRun->RunInstance();

	g_Status.dwCurrentState = SERVICE_STOPPED;
	SetServiceStatus(g_hServiceStatus, &g_Status);
}

void WINAPI ServiceCtrl(DWORD dwOpcode)
{
	CServiceInstance* pRun = CServiceInstance::GetInstance();
	if (pRun == NULL)
		return;

	switch(dwOpcode)
	{
	case SERVICE_CONTROL_STOP:
		{
			pRun->StopInstance();
		}
		break;
	case SERVICE_CONTROL_PAUSE:
	case SERVICE_CONTROL_CONTINUE:
	case SERVICE_CONTROL_INTERROGATE:
	case SERVICE_CONTROL_SHUTDOWN:
		break;
	default:
		break;
	}
}

bool UnInstall()
{
	CServiceInstance* pRun = CServiceInstance::GetInstance();
	if (pRun == NULL)
	{
		printf("Not find instance name!\n");
		return false;
	}

	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hSCManager == NULL)
		return false;

	char szServiceName[256] = {0};
	memcpy(szServiceName, pRun->GetServiceName(), min(sizeof(szServiceName) - 1, strlen(pRun->GetServiceName())));
	SC_HANDLE hService = OpenService(hSCManager, szServiceName, SERVICE_ALL_ACCESS);
	if (hService == NULL)
	{
		CloseServiceHandle(hSCManager);
		return false;
	}


	SERVICE_STATUS srvStatus = {0};
	if (ControlService(hService, SERVICE_CONTROL_STOP, &srvStatus))
	{
		if (srvStatus.dwCurrentState != SERVICE_STOPPED)
		{
		    Sleep(1000);
		}
	}

	if (!DeleteService(hService))
		printf("Remove service from service list failed, error: %d\n!", GetLastError());
	else
		printf("Remove service successfully!\n");

	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);

	return true;
}

#endif

