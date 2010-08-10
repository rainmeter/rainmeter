#include <stdlib.h>
#include <string.h>
#include "strmap.h"

hqStrMap* create_instance( int extrabytes, int dup )
{
    hqStrMap* strmap = calloc( 1, sizeof(hqStrMap) );
    strmap->FDoDuplicate = dup;
    strmap->FExtraLen = extrabytes;
    strmap->FRecordLen = sizeof(char*) + sizeof(int) + extrabytes;
	strmap->FList = NULL;
    return strmap;
}

hqStrMap* Strmap_Create( int extrabytes, int dup )
{
    return create_instance( extrabytes, dup );
}

void Strmap_FillFromChain( hqStrMap* strmap, char *strchain, void *data )
{
    while ( *strchain ) {
		size_t len = strlen( strchain );
		StrMap_AddStrLen( strmap, strchain, len, data );
		strchain += len+1;
		data = (char*)data + strmap->FExtraLen;
    }
}

hqStrMap* Strmap_CreateFromChain( int extrabytes, char *strchain, void *data )
{
    hqStrMap* strmap = create_instance( extrabytes, 0 );
    Strmap_FillFromChain( strmap, strchain, data );
    StrMap_ShrinkMem( strmap );
    return strmap;
}

void StrMap_Destroy( hqStrMap* strmap )
{
    if ( strmap->FDoDuplicate )
		StrMap_TrimClear( strmap, 0 );
    if ( strmap->FList )
		free( strmap->FList );
	
	free(strmap);
}

void StrMap_AddString( hqStrMap* strmap, const char *str, void *data )
{
	StrMap_AddStrLen( strmap, str, strlen(str), data );
}

void StrMap_AddStrLen( hqStrMap* strmap, const char *str, size_t len, void *data )
{
    const char *Rec;
    if ( strmap->FCount >= strmap->FCapacity ) {
		int delta = (strmap->FCapacity > 64) ? strmap->FCapacity / 4 : 16;
		StrMap_SetCapacity( strmap, strmap->FCapacity + delta );
    }
    Rec = strmap->FList + strmap->FCount * strmap->FRecordLen;
    *(const char**)Rec = str;
    *(int*)(Rec + sizeof(char*)) = len;
    if (data) {
		void *recdata = (void*)(Rec + sizeof(char*) + sizeof(int));
		memcpy( recdata, data, strmap->FExtraLen );
    }
    ++ strmap->FCount;
}

void StrMap_ShrinkMem( hqStrMap* strmap )
{
    StrMap_SetCapacity( strmap, strmap->FCount );
}

void StrMap_Trim( hqStrMap* strmap, int NewCount )
{
    strmap->FCount = NewCount;
}

void StrMap_TrimClear( hqStrMap* strmap, int NewCount )
{
    int i;
    char *Rec = strmap->FList + NewCount * strmap->FRecordLen;
    for (i=NewCount; i < strmap->FCount; i++) {
		free( *(char**)Rec );
		Rec += strmap->FRecordLen;
    }
    strmap->FCount = NewCount;
}

void StrMap_SetCapacity( hqStrMap* strmap, int NewCapacity )
{
    strmap->FCapacity = NewCapacity;
    if ( strmap->FCount > strmap->FCapacity )
        strmap->FCount = strmap->FCapacity;
    strmap->FList = (char*) realloc( strmap->FList,
		strmap->FCapacity * strmap->FRecordLen );
}

int StrMap_IndexOf( hqStrMap* strmap, const char *str, void **data )
{
    return StrMap_LenIndexOf( strmap, str, strlen(str), data );
}

int StrMap_LenIndexOf( hqStrMap* strmap, const char *str, size_t len, void **data )
{
    int i;
    char *Rec = strmap->FList;
    for (i=0; i<strmap->FCount; i++) {
        int recLen = *(int*)(Rec + sizeof(char*));
		if (recLen==len && strnicmp( str, *(char**)Rec, recLen )==0 ) {
			*data = (Rec + sizeof(char*) + sizeof(int));
			return i;
		}
		Rec += strmap->FRecordLen;
    }
    *data = NULL;
    return -1;
}

char* StrMap_GetString( hqStrMap* strmap, int index, int *len, void **data )
{
    char *Rec = strmap->FList + index * strmap->FRecordLen;
    *len =  *(int*)(Rec + sizeof(char*));
    if (data!=NULL && strmap->FExtraLen>0)
        *data = (Rec + sizeof(char*) + sizeof(int));
    return *(char**)Rec;
}

