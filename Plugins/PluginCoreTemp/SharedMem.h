// Common.h: 
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_COMMON_H__B302F7F1_E8D6_4EF2_9D89_A634D14922BF__INCLUDED_)
#define AFX_COMMON_H__B302F7F1_E8D6_4EF2_9D89_A634D14922BF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define CORE_TEMP_MAPPING_OBJECT L"CoreTempMappingObject"
#define CORE_TEMP_MUTEX_OBJECT L"CoreTempMutexObject"

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
	float			fMultipier;	
	char			sCPUName[100];
	unsigned char	ucFahrenheit;
	unsigned char	ucDeltaToTjMax;
}CORE_TEMP_SHARED_DATA,*PCORE_TEMP_SHARED_DATA,**PPCORE_TEMP_SHARED_DATA;

class CSharedMemClient
{
// Construction
public:
	CSharedMemClient(void);	// standard constructor
	virtual ~CSharedMemClient(void);

	bool ReadSharedMem(PCORE_TEMP_SHARED_DATA i_SharedData);
};

#endif // !defined(AFX_COMMON_H__B302F7F1_E8D6_4EF2_9D89_A634D14922BF__INCLUDED_)
