#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_NON_CONFORMING_SWPRINTFS

#ifdef _MSC_VER
#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )
#endif

#include <windows.h> 
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <locale.h> 
#include <Tlhelp32.h>
#include <string.h>
#define SystemBasicInformation 0 
#define SystemPerformanceInformation 2 
#define SystemTimeInformation 3
#define SystemProcessorPerformanceInformation 8

#define Time 60
 
#define Li2Double(x) ((double)((x).HighPart) * 4.294967296E9 + (double)((x).LowPart))
 
typedef struct 
{ 
	DWORD dwUnknown1; 
	ULONG uKeMaximumIncrement; 
	ULONG uPageSize; 
	ULONG uMmNumberOfPhysicalPages; 
	ULONG uMmLowestPhysicalPage; 
	ULONG uMmHighestPhysicalPage; 
	ULONG uAllocationGranularity; 
	PVOID pLowestUserAddress; 
	PVOID pMmHighestUserAddress; 
	ULONG uKeActiveProcessors; 
	BYTE bKeNumberProcessors; 
	BYTE bUnknown2; 
	WORD wUnknown3; 
} SYSTEM_BASIC_INFORMATION;
 
typedef struct 
{ 
	LARGE_INTEGER liIdleTime;
	LARGE_INTEGER ReadTransferCount;
	LARGE_INTEGER WriteTransferCount;
	LARGE_INTEGER OtherTransferCount;
	ULONG ReadOperationCount;
	ULONG WriteOperationCount;
	ULONG OtherOperationCount;
	ULONG AvailablePages;
	ULONG TotalCommittedPages;
	ULONG TotalCommitLimit;
	ULONG PeakCommitment;
	ULONG PageFaults;    // total soft or hard Page Faults since boot (wraps at 32-bits)
	ULONG Reserved[74]; // unknown

	/*LARGE_INTEGER liIdleTime; 
	DWORD dwSpare[76]; */
} SYSTEM_PERFORMANCE_INFORMATION;
 
typedef struct 
{ 
	LARGE_INTEGER liKeBootTime; 
	LARGE_INTEGER liKeSystemTime; 
	LARGE_INTEGER liExpTimeZoneBias; 
	ULONG uCurrentTimeZoneId; 
	DWORD dwReserved; 
} SYSTEM_TIME_INFORMATION;
 
typedef struct
_SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION {
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER DpcTime;
    LARGE_INTEGER InterruptTime;
    ULONG Reserved2;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;
 
typedef struct
_m_PROCESSORS_USE_TIME {
    double dbOldIdleTime;	// save old total time
	double dbOldCurrentTime;
	double dbIdleTime;		// save time after calc
	double dbCurrentTime;
	float fUse;
}m_PROCESSORS_USE_TIME;
 
m_PROCESSORS_USE_TIME * m_PUT;
 
SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION * m_pSPPI = NULL;
 
int m_iNumberProcessors;
 
typedef LONG (WINAPI *PROCNTQSI)(UINT,PVOID,ULONG,PULONG);
 
PROCNTQSI NtQuerySystemInformation;
 
static LARGE_INTEGER liOldIdleTime = {0,0}; 
static LARGE_INTEGER liOldSystemTime = {0,0};
 
double dbIdleTime = 0; 
double dbSystemTime = 0; 
double alldbIdleTime = 0;
 
double getCpuUsage()
{ 
	SYSTEM_PERFORMANCE_INFORMATION SysPerfInfo; 
	SYSTEM_TIME_INFORMATION SysTimeInfo; 
	SYSTEM_BASIC_INFORMATION SysBaseInfo; 
 
	LONG status; 

	//change char** to lpcwstr
	char a[] = "ntdll";
	WCHAR wsz[64]; 
	swprintf(wsz, L"%S", a);
	LPCWSTR ntdll = wsz;

	NtQuerySystemInformation = (PROCNTQSI)GetProcAddress(GetModuleHandle(ntdll),"NtQuerySystemInformation");
	// get number of processors in the system 
	status = NtQuerySystemInformation(SystemBasicInformation,&SysBaseInfo,sizeof(SysBaseInfo),NULL); 
	if (status != NO_ERROR) 
		return -1;
 
	if (!NtQuerySystemInformation) 
		return -1;
 
	// get number of processors in the system 
	status = NtQuerySystemInformation(SystemBasicInformation,&SysBaseInfo,sizeof(SysBaseInfo),NULL); 
	if (status != NO_ERROR) 
		return -1;
 
	// get new system time 
	status = NtQuerySystemInformation(SystemTimeInformation,&SysTimeInfo,sizeof(SysTimeInfo),0); 
	if (status!=NO_ERROR) 
		return -1;
 
	// get new CPU's idle time 
	status =NtQuerySystemInformation(SystemPerformanceInformation,&SysPerfInfo,sizeof(SysPerfInfo),NULL); 
	if (status != NO_ERROR) 
		return -1;
 
	if ( m_iNumberProcessors != SysBaseInfo.bKeNumberProcessors)
	{
		//save
		m_iNumberProcessors = SysBaseInfo.bKeNumberProcessors;
		//if sppi not null clear
		if (m_pSPPI != NULL) delete []m_pSPPI;
		if (m_PUT != NULL) delete []m_PUT;
		//malloc and point
		m_pSPPI = new SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION[m_iNumberProcessors];
		m_PUT = new m_PROCESSORS_USE_TIME[m_iNumberProcessors];
	}
 
	// get ProcessorPer time 
	status =NtQuerySystemInformation(SystemProcessorPerformanceInformation, m_pSPPI, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * m_iNumberProcessors, NULL); 
	if (status != NO_ERROR) 
		return -1;
 
	// if it's a first call - skip it 
	if (liOldIdleTime.QuadPart != 0) 
	{ 
		// CurrentValue = NewValue - OldValue 
		dbIdleTime = Li2Double(SysPerfInfo.liIdleTime) - Li2Double(liOldIdleTime); 
 
		dbSystemTime = Li2Double(SysTimeInfo.liKeSystemTime) - Li2Double(liOldSystemTime);
 
		// CurrentCpuIdle = IdleTime / SystemTime 
		dbIdleTime = dbIdleTime / dbSystemTime;
 
		// CurrentCpuUsage% = 100 - (CurrentCpuIdle * 100) / NumberOfProcessors 
		dbIdleTime = 100.0 - dbIdleTime * 100.0 / (double)SysBaseInfo.bKeNumberProcessors;
 
		//calc Processors
		for (int i = 0; i < m_iNumberProcessors; i++)
		{
			m_PUT[i].dbCurrentTime = Li2Double(m_pSPPI[i].KernelTime) + Li2Double(m_pSPPI[i].UserTime) + 
										Li2Double(m_pSPPI[i].DpcTime) + Li2Double(m_pSPPI[i].InterruptTime) - m_PUT[i].dbOldCurrentTime;
			m_PUT[i].dbIdleTime = Li2Double(m_pSPPI[i].IdleTime) - m_PUT[i].dbOldIdleTime;
 
			// CurrentCpuIdle = IdleTime / SystemTime 
			m_PUT[i].dbIdleTime = m_PUT[i].dbIdleTime / m_PUT[i].dbCurrentTime;
 
			// CurrentCpuUsage% = 100 - (CurrentCpuIdle * 100) / NumberOfProcessors 
			m_PUT[i].dbIdleTime = 100.0 - m_PUT[i].dbIdleTime * 100.0;
 
		}
	}
 
	// store new CPU's idle and system time 
	liOldIdleTime = SysPerfInfo.liIdleTime; 
	liOldSystemTime = SysTimeInfo.liKeSystemTime;
 
	for (int i = 0; i < m_iNumberProcessors; i++)
	{
		m_PUT[i].dbOldCurrentTime = Li2Double(m_pSPPI[i].KernelTime) + Li2Double(m_pSPPI[i].UserTime) + 
										Li2Double(m_pSPPI[i].DpcTime) + Li2Double(m_pSPPI[i].InterruptTime);
 
		m_PUT[i].dbOldIdleTime = Li2Double(m_pSPPI[i].IdleTime);
	}
 
	return dbIdleTime;
}

int getAverageCpuUsage(double  cpuUsage[17]) 
{

	for(int n=0;n<Time;n++) { //求和
		Sleep(1000);
		cpuUsage[0] += getCpuUsage();
		for (int i = 0; i < m_iNumberProcessors; i++)
		{
			cpuUsage[i+1] += m_PUT[i].dbIdleTime;
		}
		
	}
	//求平均
	for (int i = 0; i < m_iNumberProcessors+1; i++)
	{
		cpuUsage[i] /= Time;
	}
	return 1;
}

int fileInit() {
	FILE *fp = NULL;
	getCpuUsage();//获取CPU数，并舍弃第一组数据
	if (_access("CpuUsage.log",0) == -1)//判断是不是新文件
	{
		if(((fp = fopen("CpuUsage.log", "wt")) == NULL)) 
		{
			printf("Can not creat log file");
			exit(0);
		}

		fprintf(fp, "This compute has %d core!\n", m_iNumberProcessors);
		fputs("Sum\t", fp);

		for (int i = 1; i <= m_iNumberProcessors; i++)
			fprintf(fp, "Core%d\t", i);
		fputs("\n",fp);

		fclose(fp);
	} else {
		if(((fp = fopen("CpuUsage.log", "r+")) == NULL)) 
		{
			printf("Can not read log file");
			exit(0);
		}

		int pos;
		for(pos = 0;pos > -1000;pos--)//判断记录是否出错
		{
			fseek( fp, pos, SEEK_END );
			if(fgetc(fp) == '\n') break;
		}

		if(pos != 0)
		{
			//重写出错记录
			fseek(fp, pos+1, SEEK_END );
			double cpuUsage[33] = {0};//最大支持32核;
			getAverageCpuUsage(cpuUsage);
 
			for (int i = 0; i < m_iNumberProcessors+1; i++)
				fprintf(fp, "%.3f\t", cpuUsage[i]);

			fputs("\n",fp);
		}
		
		fclose(fp);
	}
    return 0;
}

int exist()
{
	char a[] = "CpuUsageAnalyser";
	WCHAR wsz[64]; 
	swprintf(wsz, L"%S", a);
	LPCWSTR sin = wsz;
	//创建互斥信号量
	
	HANDLE m_hMutex = CreateMutex(NULL, FALSE, sin);
	
	if (GetLastError() == ERROR_ALREADY_EXISTS) //存在则释放句柄，并复位互斥量
	{
		CloseHandle(m_hMutex);
		m_hMutex = NULL;
		//返回1
		
		return 1;
	
	}
	return 0;
}

int main()
{
	FILE *fp = NULL;
	if(exist()) return 0;//判断是否有相同程序运行
	fileInit();

	while(1) 
	{
		double cpuUsage[33] = {0};//最大支持32核;
		getAverageCpuUsage(cpuUsage);
			//写文件
		if(((fp = fopen("CpuUsage.log", "at")) == NULL)) 
		{
			printf("File open faild");
			exit(0);
		}

		for (int i = 0; i < m_iNumberProcessors + 1; i++)
			fprintf(fp, "%.3f\t", cpuUsage[i]);

		fputs("\n",fp);
	
		fclose(fp);
	}
	return 1;
}