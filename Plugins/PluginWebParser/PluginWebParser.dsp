# Microsoft Developer Studio Project File - Name="PluginWebParser" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=PluginWebParser - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "PluginWebParser.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "PluginWebParser.mak" CFG="PluginWebParser - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PluginWebParser - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "PluginWebParser - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "PluginWebParser - Win32 Release 64" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "PluginWebParser"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "PluginWebParser - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PluginWebParser_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_USRDLL" /D "PluginWebParser_EXPORTS" /D "SUPPORT_UTF8" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40b /d "NDEBUG"
# ADD RSC /l 0x40b /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 Urlmon.lib Wininet.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"../../TestBench/x32/Release/Plugins/WebParser.dll"

!ELSEIF  "$(CFG)" == "PluginWebParser - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PluginWebParser_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_USRDLL" /D "PluginWebParser_EXPORTS" /D "SUPPORT_UTF8" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40b /d "_DEBUG"
# ADD RSC /l 0x40b /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Urlmon.lib Wininet.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"../../TestBench/x32/Debug/Plugins/WebParser.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "PluginWebParser - Win32 Release 64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "PluginWebParser___Win32_Release_64"
# PROP BASE Intermediate_Dir "PluginWebParser___Win32_Release_64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "x64/Release"
# PROP Intermediate_Dir "x64/Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_USRDLL" /D "PluginWebParser_EXPORTS" /D "SUPPORT_UTF8" /YX /FD /c
# ADD CPP /nologo /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_USRDLL" /D "PluginWebParser_EXPORTS" /D "SUPPORT_UTF8" /FD /GL /EHsc /Wp64 /GA /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40b /d "NDEBUG"
# ADD RSC /l 0x40b /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 Urlmon.lib Wininet.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"../../TestBench/x32/Release/Plugins/WebParser.dll"
# ADD LINK32 bufferoverflowU.lib Urlmon.lib Wininet.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:IX86 /out:"../../TestBench/x64/Release/Plugins/WebParser.dll" /machine:AMD64 /LTCG
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "PluginWebParser - Win32 Release"
# Name "PluginWebParser - Win32 Debug"
# Name "PluginWebParser - Win32 Release 64"
# Begin Group "pcre"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\pcre-6.4\chartables.c"
# End Source File
# Begin Source File

SOURCE=".\pcre-6.4\pcre_compile.c"
# End Source File
# Begin Source File

SOURCE=".\pcre-6.4\pcre_config.c"
# End Source File
# Begin Source File

SOURCE=".\pcre-6.4\pcre_dfa_exec.c"
# End Source File
# Begin Source File

SOURCE=".\pcre-6.4\pcre_exec.c"
# End Source File
# Begin Source File

SOURCE=".\pcre-6.4\pcre_fullinfo.c"
# End Source File
# Begin Source File

SOURCE=".\pcre-6.4\pcre_get.c"
# End Source File
# Begin Source File

SOURCE=".\pcre-6.4\pcre_globals.c"
# End Source File
# Begin Source File

SOURCE=".\pcre-6.4\pcre_info.c"
# End Source File
# Begin Source File

SOURCE=".\pcre-6.4\pcre_internal.h"
# End Source File
# Begin Source File

SOURCE=".\pcre-6.4\pcre_maketables.c"
# End Source File
# Begin Source File

SOURCE=".\pcre-6.4\pcre_ord2utf8.c"
# End Source File
# Begin Source File

SOURCE=".\pcre-6.4\pcre_refcount.c"
# End Source File
# Begin Source File

SOURCE=".\pcre-6.4\pcre_scanner.h"
# End Source File
# Begin Source File

SOURCE=".\pcre-6.4\pcre_study.c"
# End Source File
# Begin Source File

SOURCE=".\pcre-6.4\pcre_tables.c"
# End Source File
# Begin Source File

SOURCE=".\pcre-6.4\pcre_try_flipped.c"
# End Source File
# Begin Source File

SOURCE=".\pcre-6.4\pcre_ucp_findchar.c"
# End Source File
# Begin Source File

SOURCE=".\pcre-6.4\pcre_valid_utf8.c"
# End Source File
# Begin Source File

SOURCE=".\pcre-6.4\pcre_version.c"
# End Source File
# Begin Source File

SOURCE=".\pcre-6.4\pcre_xclass.c"
# End Source File
# Begin Source File

SOURCE=".\pcre-6.4\pcreposix.c"
# End Source File
# End Group
# Begin Source File

SOURCE=.\WebParser.cpp
# End Source File
# End Target
# End Project
