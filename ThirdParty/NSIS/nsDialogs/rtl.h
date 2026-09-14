#ifndef __NS_DIALOGS__RTL_H__
#define __NS_DIALOGS__RTL_H__

#include "defs.h"

void NSDFUNC ConvertStyleToRTL(enum nsControlType type, LPDWORD style, LPDWORD exStyle);
void NSDFUNC ConvertPosToRTL(int *x, int width, int dialogWidth);

#endif//__NS_DIALOGS__RTL_H__
