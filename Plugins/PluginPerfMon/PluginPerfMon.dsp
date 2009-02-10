# Microsoft Developer Studio Project File - Name="PluginPerfMon" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=PluginPerfMon - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "PluginPerfMon.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "PluginPerfMon.mak" CFG="PluginPerfMon - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PluginPerfMon - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "PluginPerfMon - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "PluginPerfMon - Win32 Release 64" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "PluginPerfMon"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "PluginPerfMon - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PluginPerfMon_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_USRDLL" /D "PluginPerfMon_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40b /d "NDEBUG"
# ADD RSC /l 0x40b /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"../../TestBench/x32/Release/Plugins/PerfMon.dll"

!ELSEIF  "$(CFG)" == "PluginPerfMon - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PluginPerfMon_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_USRDLL" /D "PluginPerfMon_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40b /d "_DEBUG"
# ADD RSC /l 0x40b /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"../../TestBench/x32/Debug/Plugins/PerfMon.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "PluginPerfMon - Win32 Release 64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "PluginPerfMon___Win32_Release_64"
# PROP BASE Intermediate_Dir "PluginPerfMon___Win32_Release_64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "x64/Release"
# PROP Intermediate_Dir "x64/Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_USRDLL" /D "PluginPerfMon_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_USRDLL" /D "PluginPerfMon_EXPORTS" /FD /GL /EHsc /Wp64 /GA /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40b /d "NDEBUG"
# ADD RSC /l 0x40b /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"../../TestBench/x32/Release/Plugins/PerfMon.dll"
# ADD LINK32 bufferoverflowU.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:IX86 /out:"../../TestBench/x64/Release/Plugins/PerfMon.dll" /machine:AMD64 /LTCG
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "PluginPerfMon - Win32 Release"
# Name "PluginPerfMon - Win32 Debug"
# Name "PluginPerfMon - Win32 Release 64"
# Begin Source File

SOURCE=.\MakePtr.h
# End Source File
# Begin Source File

SOURCE=.\ObjInst.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjInst.h
# End Source File
# Begin Source File

SOURCE=.\ObjList.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjList.h
# End Source File
# Begin Source File

SOURCE=.\PerfCntr.cpp
# End Source File
# Begin Source File

SOURCE=.\PerfCntr.h
# End Source File
# Begin Source File

SOURCE=.\PerfData.cpp
# End Source File
# Begin Source File

SOURCE=.\PerfData.h
# End Source File
# Begin Source File

SOURCE=.\PerfObj.cpp
# End Source File
# Begin Source File

SOURCE=.\PerfObj.h
# End Source File
# Begin Source File

SOURCE=.\PerfSnap.cpp
# End Source File
# Begin Source File

SOURCE=.\PerfSnap.h
# End Source File
# Begin Source File

SOURCE=.\Titledb.cpp
# End Source File
# Begin Source File

SOURCE=.\Titledb.h
# End Source File
# End Target
# End Project
