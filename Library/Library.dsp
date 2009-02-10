# Microsoft Developer Studio Project File - Name="Library" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Library - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Library.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Library.mak" CFG="Library - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Library - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Library - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Library - Win32 Release 64" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "Library"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Library - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "x32/Release"
# PROP Intermediate_Dir "x32/Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBRARY_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_USRDLL" /D "LIBRARY_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40b /d "NDEBUG"
# ADD RSC /l 0x40b /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 comctl32.lib Wininet.lib Winmm.lib gdiplus.lib Iphlpapi.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"../TestBench/x32/Release/Rainmeter.dll"

!ELSEIF  "$(CFG)" == "Library - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "x32/Debug"
# PROP Intermediate_Dir "x32/Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBRARY_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_USRDLL" /D "LIBRARY_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40b /d "_DEBUG"
# ADD RSC /l 0x40b /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 comctl32.lib Wininet.lib Winmm.lib gdiplus.lib Iphlpapi.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"../TestBench/x32/Debug/Rainmeter.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "Library - Win32 Release 64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Library___Win32_Release_64"
# PROP BASE Intermediate_Dir "Library___Win32_Release_64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "x64/Release"
# PROP Intermediate_Dir "x64/Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GR /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_USRDLL" /D "LIBRARY_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GR /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_USRDLL" /D "LIBRARY_EXPORTS" /FD /GL /EHsc /Wp64 /GA /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40b /d "NDEBUG"
# ADD RSC /l 0x40b /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 comctl32.lib Wininet.lib Winmm.lib gdiplus.lib Iphlpapi.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"../TestBench/x32/Release/Rainmeter.dll"
# ADD LINK32 bufferoverflowU.lib comctl32.lib Wininet.lib Winmm.lib gdiplus.lib Iphlpapi.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:IX86 /out:"../TestBench/x64/Release/Rainmeter.dll" /machine:AMD64 /LTCG
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "Library - Win32 Release"
# Name "Library - Win32 Debug"
# Name "Library - Win32 Release 64"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AboutDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\ConfigParser.cpp
# End Source File
# Begin Source File

SOURCE=.\Error.cpp
# End Source File
# Begin Source File

SOURCE=.\Library.rc
# End Source File
# Begin Source File

SOURCE=.\Litestep.cpp
# End Source File
# Begin Source File

SOURCE=.\Measure.cpp
# End Source File
# Begin Source File

SOURCE=.\MeasureCalc.cpp
# End Source File
# Begin Source File

SOURCE=.\MeasureCPU.cpp
# End Source File
# Begin Source File

SOURCE=.\MeasureDiskSpace.cpp
# End Source File
# Begin Source File

SOURCE=.\MeasureMemory.cpp
# End Source File
# Begin Source File

SOURCE=.\MeasureNet.cpp
# End Source File
# Begin Source File

SOURCE=.\MeasureNetIn.cpp
# End Source File
# Begin Source File

SOURCE=.\MeasureNetOut.cpp
# End Source File
# Begin Source File

SOURCE=.\MeasureNetTotal.cpp
# End Source File
# Begin Source File

SOURCE=.\MeasurePhysicalMemory.cpp
# End Source File
# Begin Source File

SOURCE=.\MeasurePlugin.cpp
# End Source File
# Begin Source File

SOURCE=.\MeasureRegistry.cpp
# End Source File
# Begin Source File

SOURCE=.\MeasureTime.cpp
# End Source File
# Begin Source File

SOURCE=.\MeasureUptime.cpp
# End Source File
# Begin Source File

SOURCE=.\MeasureVirtualMemory.cpp
# End Source File
# Begin Source File

SOURCE=.\Meter.cpp
# End Source File
# Begin Source File

SOURCE=.\MeterBar.cpp
# End Source File
# Begin Source File

SOURCE=.\MeterBitmap.cpp
# End Source File
# Begin Source File

SOURCE=.\MeterButton.cpp
# End Source File
# Begin Source File

SOURCE=.\MeterHistogram.cpp
# End Source File
# Begin Source File

SOURCE=.\MeterImage.cpp
# End Source File
# Begin Source File

SOURCE=.\MeterLine.cpp
# End Source File
# Begin Source File

SOURCE=.\MeterRotator.cpp
# End Source File
# Begin Source File

SOURCE=.\MeterRoundLine.cpp
# End Source File
# Begin Source File

SOURCE=.\MeterString.cpp
# End Source File
# Begin Source File

SOURCE=.\MeterWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\Rainmeter.cpp
# End Source File
# Begin Source File

SOURCE=.\TrayWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\UpdateCheck.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AboutDialog.h
# End Source File
# Begin Source File

SOURCE=.\ConfigParser.h
# End Source File
# Begin Source File

SOURCE=.\Error.h
# End Source File
# Begin Source File

SOURCE=.\Export.h
# End Source File
# Begin Source File

SOURCE=.\Litestep.h
# End Source File
# Begin Source File

SOURCE=.\Measure.h
# End Source File
# Begin Source File

SOURCE=.\MeasureCalc.h
# End Source File
# Begin Source File

SOURCE=.\MeasureCPU.h
# End Source File
# Begin Source File

SOURCE=.\MeasureDiskSpace.h
# End Source File
# Begin Source File

SOURCE=.\MeasureMemory.h
# End Source File
# Begin Source File

SOURCE=.\MeasureNet.h
# End Source File
# Begin Source File

SOURCE=.\MeasureNetIn.h
# End Source File
# Begin Source File

SOURCE=.\MeasureNetOut.h
# End Source File
# Begin Source File

SOURCE=.\MeasureNetTotal.h
# End Source File
# Begin Source File

SOURCE=.\MeasurePhysicalMemory.h
# End Source File
# Begin Source File

SOURCE=.\MeasurePlugin.h
# End Source File
# Begin Source File

SOURCE=.\MeasureRegistry.h
# End Source File
# Begin Source File

SOURCE=.\MeasureTime.h
# End Source File
# Begin Source File

SOURCE=.\MeasureUptime.h
# End Source File
# Begin Source File

SOURCE=.\MeasureVirtualMemory.h
# End Source File
# Begin Source File

SOURCE=.\Meter.h
# End Source File
# Begin Source File

SOURCE=.\MeterBar.h
# End Source File
# Begin Source File

SOURCE=.\MeterBitmap.h
# End Source File
# Begin Source File

SOURCE=.\MeterButton.h
# End Source File
# Begin Source File

SOURCE=.\MeterHistogram.h
# End Source File
# Begin Source File

SOURCE=.\MeterImage.h
# End Source File
# Begin Source File

SOURCE=.\MeterLine.h
# End Source File
# Begin Source File

SOURCE=.\MeterRotator.h
# End Source File
# Begin Source File

SOURCE=.\MeterRoundLine.h
# End Source File
# Begin Source File

SOURCE=.\MeterString.h
# End Source File
# Begin Source File

SOURCE=.\MeterWindow.h
# End Source File
# Begin Source File

SOURCE=.\Rainmeter.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\TrayWindow.h
# End Source File
# Begin Source File

SOURCE=.\UpdateCheck.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\Res\tray.ico
# End Source File
# End Group
# Begin Group "CCalc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\ccalc-0.5.1\lexer.c"
# End Source File
# Begin Source File

SOURCE=".\ccalc-0.5.1\lexer.h"
# End Source File
# Begin Source File

SOURCE=".\ccalc-0.5.1\mparser.c"
# End Source File
# Begin Source File

SOURCE=".\ccalc-0.5.1\mparser.h"
# End Source File
# Begin Source File

SOURCE=".\ccalc-0.5.1\pack.h"
# End Source File
# Begin Source File

SOURCE=".\ccalc-0.5.1\strmap.c"
# End Source File
# Begin Source File

SOURCE=".\ccalc-0.5.1\strmap.h"
# End Source File
# Begin Source File

SOURCE=".\ccalc-0.5.1\wininit.c"
# End Source File
# End Group
# End Target
# End Project
