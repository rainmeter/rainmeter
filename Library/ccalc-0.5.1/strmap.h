#pragma once

#include "pack.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push,1)

typedef struct {
    int   FCount, FCapacity;
    int   FExtraLen, FRecordLen;
    int	  FDoDuplicate;
    char *FList;
} hqStrMap;

#pragma pack(pop)

/* API */

hqStrMap* Strmap_Create( int extrabytes, int dup );
hqStrMap* Strmap_CreateFromChain( int extrabytes, char *strchain, void *data );
void StrMap_Destroy( hqStrMap* strmap );

void StrMap_AddString( hqStrMap* strmap, const char *str, void *data );
void StrMap_AddStrLen( hqStrMap* strmap, const char *str, size_t len, void *data );
void StrMap_ShrinkMem( hqStrMap* strmap );
void StrMap_Trim( hqStrMap* strmap, int NewCount );
void StrMap_TrimClear( hqStrMap* strmap, int NewCount );
void StrMap_SetCapacity( hqStrMap* strmap, int NewCapacity );
int StrMap_IndexOf( hqStrMap* strmap, const char *str, void **data );
int StrMap_LenIndexOf( hqStrMap* strmap, const char *str, size_t len, void **data );
char* StrMap_GetString( hqStrMap* strmap, int index, int *len, void **data );
void Strmap_FillFromChain( hqStrMap* strmap, char *strchain, void *data );

#ifdef __cplusplus
}
#endif
