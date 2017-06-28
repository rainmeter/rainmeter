// Common.h: 
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_COMMON_H__B302F7F1_E8D6_4EF2_9D89_A634D14922BF__INCLUDED_)
#define AFX_COMMON_H__B302F7F1_E8D6_4EF2_9D89_A634D14922BF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define CORE_TEMP_MAPPING_OBJECT_EX L"CoreTempMappingObjectEx"
#define CORE_TEMP_MUTEX_OBJECT L"CoreTempMutexObject"

//Plugin types
#define General_Type 1

#define Plugin_Interface_Version 2
#define CoreTempSharedData_Structure_Version 2

//Errors
#define USER					0x20000000
#define CONFIGURE_UNSUPPORTED	0x20000001

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(4)     /* set alignment to 4 byte boundary */

typedef struct core_temp_shared_data
{
	unsigned int	uiLoad[256];
	unsigned int	uiTjMax[128];
	unsigned int	uiCoreCnt;
	unsigned int	uiCPUCnt;
	float			fTemp[256];
	float			fVID;
	float			fCPUSpeed;
	float			fFSBSpeed;
	float			fMultiplier;
	char			sCPUName[100];
	unsigned char	ucFahrenheit;
	unsigned char	ucDeltaToTjMax;
	unsigned char	reserved[2];
} CoreTempSharedData, *LPCoreTempSharedData, **PPCoreTempSharedData;

typedef struct core_temp_shared_data_ex
{
	// Original structure (CoreTempSharedData)
	unsigned int	uiLoad[256];
	unsigned int	uiTjMax[128];
	unsigned int	uiCoreCnt;
	unsigned int	uiCPUCnt;
	float			fTemp[256];
	float			fVID;
	float			fCPUSpeed;
	float			fFSBSpeed;
	float			fMultiplier;
	char			sCPUName[100];
	unsigned char	ucFahrenheit;
	unsigned char	ucDeltaToTjMax;
	// uiStructVersion = 2
	unsigned char	ucTdpSupported;
	unsigned char	ucPowerSupported;
	unsigned int	uiStructVersion;
	unsigned int	uiTdp[128];
	float			fPower[128];
	float			fMultipliers[256];
} CoreTempSharedDataEx, *LPCoreTempSharedDataEx, **PPCoreTempSharedDataEx;

#pragma pack(pop)   /* restore original alignment from stack */

typedef struct CoreTempPluginInfo CoreTempPluginInfo, *LPCoreTempPluginInfo;
typedef struct CoreTempPlugin CoreTempPlugin, *LPCoreTempPlugin;
typedef void (*RemotePluginStop)(LPCoreTempPlugin);
struct CoreTempPluginInfo
{
	wchar_t *version;
	wchar_t *name;
	wchar_t *description;
	HWND hwndParent;
	HINSTANCE dllInstance;
	RemotePluginStop remoteStopProc;
};

struct CoreTempPlugin
{
	int interfaceVersion;
	int type;
	LPCoreTempPluginInfo pluginInfo;

	// interfaceVersion = 2
	int (*Start)();
	void (*Update)(const LPCoreTempSharedData data);
	void (*Stop)();
	int (*Configure)();
	void (*Remove)(const wchar_t *path);
	void (*UpdateEx)(const LPCoreTempSharedDataEx data);
};

typedef LPCoreTempPlugin (WINAPI *fnGetPlugin)(HMODULE hModule);
typedef void (WINAPI *fnReleasePlugin)();

class CSharedMemClient
{
// Construction
public:
	CSharedMemClient(void);	// standard constructor
	virtual ~CSharedMemClient(void);

	bool ReadSharedMem(LPCoreTempSharedDataEx i_SharedData);
};

#endif // !defined(AFX_COMMON_H__B302F7F1_E8D6_4EF2_9D89_A634D14922BF__INCLUDED_)
