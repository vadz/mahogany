# Microsoft Developer Studio Project File - Name="M" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=M - Win32 Debug DLL
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "M.MAK".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "M.MAK" CFG="M - Win32 Debug DLL"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "M - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "M - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "M - Win32 Debug DLL" (based on "Win32 (x86) Application")
!MESSAGE "M - Win32 Release DLL" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "M - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /O2 /I "..\imap-4.2\cclient" /I "include" /I "extra\src\compface" /I "extra\src\c-client" /I "$(wx)\include" /I "e:\python20\include" /D "NDEBUG" /D "USE_PCH" /D "_WINDOWS" /D WIN95=1 /D _X86_=1 /D "__WINDOWS__" /D "USE_NEW_IOSTREAM" /D "WX_DRAG_DROP" /D "WX_OWNER_DRAWN" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /D wxUSE_GUI=1 /D "_MT" /D "USE_MODULES_STATIC" /Yu"Mpch.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "include" /i "res" /i "$(wx)\include" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Debug/M.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 $(wx)\Release\wxWindows.lib extra\src\cclient\Release\cclient.lib rpcrt4.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib winmm.lib extra\src\compface\Release\compface.lib src\wx\vcard\release\versit.lib $(WX)\src\xpm\Release\xpm.lib $(WX)\src\zlib\Release\zlib.lib $(WX)\src\png\Release\png.lib $(WX)\src\jpeg\Release\jpeg.lib $(WX)\src\tiff\Release\tiff.lib /nologo /subsystem:windows /machine:I386 /libpath:"e:\python20\libs"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /ZI /Od /I "include" /I "extra\src\compface" /I "extra\src\c-client" /I "$(wx)\include" /I "e:\python20\include" /D "_DEBUG" /D "DEBUG" /D WXDEBUG=1 /D "__WXDEBUG__" /D "__WIN95__" /D "_WIN32" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /D wxUSE_GUI=1 /D "_MT" /D "USE_PCH" /D "USE_MODULES_STATIC" /Yu"Mpch.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "include" /i "res" /i "$(wx)\include" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 rpcrt4.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib winmm.lib $(wx)\debug\wxWindows.lib extra\src\c-client\Debug\cclient.lib extra\src\compface\Debug\compface.lib src\wx\vcard\debug\versit.lib $(WX)\src\xpm\Debug\xpm.lib $(WX)\src\zlib\Debug\zlib.lib $(WX)\src\png\Debug\png.lib $(WX)\src\jpeg\Debug\jpeg.lib $(WX)\src\tiff\Debug\tiff.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"e:\python20\libs"

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "M___Win32_Debug_DLL"
# PROP BASE Intermediate_Dir "M___Win32_Debug_DLL"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /ZI /Od /I "include" /I "extra\src\compface" /I "extra\src\c-client" /I "$(wx)\include" /I "\python\include" /D "_DEBUG" /D "DEBUG" /D WXDEBUG=1 /D "__WXDEBUG__" /D "__WIN95__" /D "_WIN32" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /D wxUSE_GUI=1 /D "_MT" /Yu"Mpch.h" /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MDd /W3 /Gm /ZI /Od /I "include" /I "extra\src\compface" /I "extra\src\c-client" /I "$(wx)\include" /I "e:\python20\include" /D "_DEBUG" /D "DEBUG" /D WXDEBUG=1 /D "__WXDEBUG__" /D "__WIN95__" /D "_WIN32" /D "WXUSINGDLL" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /D wxUSE_GUI=1 /D "_MT" /D "USE_PCH" /D "USE_MODULES_STATIC" /Yu"Mpch.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /i "include" /i "res" /i "$(wx)\include" /d "_DEBUG"
# ADD RSC /l 0x409 /i "include" /i "res" /i "$(wx)\include" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 rpcrt4.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib winmm.lib $(wx)\debug\wxWindows.lib $(wx)\src\xpm\debug\xpm.lib extra\src\c-client\Debug\cclient.lib extra\src\compface\Debug\compface.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 rpcrt4.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib winmm.lib $(wx)\DebugDll\wxWinDll.lib $(wx)\src\xpm\debug\xpm.lib extra\src\c-client\Debug\cclient.lib extra\src\compface\Debug\compface.lib src\wx\vcard\debug\versit.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"e:\python20\libs"

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "M___Win32_Release_DLL"
# PROP BASE Intermediate_Dir "M___Win32_Release_DLL"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /O2 /I "..\imap-4.2\cclient" /I "include" /I "extra\src\compface" /I "extra\src\c-client" /I "$(wx)\include" /I "\python\include" /D "NDEBUG" /D "USE_PCH" /D "_WINDOWS" /D WIN95=1 /D _X86_=1 /D "__WINDOWS__" /D "USE_NEW_IOSTREAM" /D "WX_DRAG_DROP" /D "WX_OWNER_DRAWN" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /D wxUSE_GUI=1 /D "_MT" /Yu"Mpch.h" /FD /c
# ADD CPP /nologo /MD /W3 /O2 /I "..\imap-4.2\cclient" /I "include" /I "extra\src\compface" /I "extra\src\c-client" /I "$(wx)\include" /I "e:\python20\include" /D "NDEBUG" /D "USE_PCH" /D "_WINDOWS" /D WIN95=1 /D _X86_=1 /D "__WINDOWS__" /D "USE_NEW_IOSTREAM" /D "WX_DRAG_DROP" /D "WX_OWNER_DRAWN" /D "WXUSINGDLL" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /D wxUSE_GUI=1 /D "_MT" /D "USE_MODULES_STATIC" /Yu"Mpch.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /i "include" /i "res" /i "$(wx)\include" /d "NDEBUG"
# ADD RSC /l 0x409 /i "include" /i "res" /i "$(wx)\include" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"Debug/M.bsc"
# ADD BSC32 /nologo /o"Debug/M.bsc"
LINK32=link.exe
# ADD BASE LINK32 $(wx)\Release\wxWindows.lib $(wx)\src\xpm\Release\xpm.lib extra\src\cclient\Release\cclient.lib rpcrt4.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib winmm.lib extra\src\compface\Release\compface.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT BASE LINK32 /debug
# ADD LINK32 $(wx)\ReleaseDll\wxWinDll.lib $(wx)\src\xpm\Release\xpm.lib extra\src\cclient\Release\cclient.lib rpcrt4.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib winmm.lib extra\src\compface\Release\compface.lib src\wx\vcard\release\versit.lib /nologo /subsystem:windows /machine:I386 /libpath:"e:\python20\libs"
# SUBTRACT LINK32 /debug

!ENDIF 

# Begin Target

# Name "M - Win32 Release"
# Name "M - Win32 Debug"
# Name "M - Win32 Debug DLL"
# Name "M - Win32 Release DLL"
# Begin Group "classes"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=.\src\classes\FolderView.cpp
# End Source File
# Begin Source File

SOURCE=.\src\classes\kbList.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\src\classes\MailCollector.cpp
# End Source File
# Begin Source File

SOURCE=.\src\classes\MApplication.cpp
# End Source File
# Begin Source File

SOURCE=.\src\classes\MessageTemplate.cpp
# End Source File
# Begin Source File

SOURCE=.\src\classes\MEvent.cpp
# End Source File
# Begin Source File

SOURCE=.\src\classes\MFilter.cpp
# End Source File
# Begin Source File

SOURCE=.\src\classes\MFolder.cpp
# End Source File
# Begin Source File

SOURCE=.\src\classes\MModule.cpp
# End Source File
# Begin Source File

SOURCE=.\src\classes\MObject.cpp
# ADD CPP /Yc"Mpch.h"
# End Source File
# Begin Source File

SOURCE=.\src\classes\PathFinder.cpp
# End Source File
# Begin Source File

SOURCE=.\src\classes\Profile.cpp
# End Source File
# Begin Source File

SOURCE=.\src\classes\XFace.cpp
# ADD CPP /D "SYSV32" /D "__STDC__"
# End Source File
# End Group
# Begin Group "mail"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\mail\ASMailFolder.cpp
# End Source File
# Begin Source File

SOURCE=.\src\mail\HeaderInfoImpl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\mail\MailFolder.cpp
# End Source File
# Begin Source File

SOURCE=.\src\mail\MailFolderCC.cpp
# End Source File
# Begin Source File

SOURCE=.\src\mail\MailMH.cpp
# End Source File
# Begin Source File

SOURCE=.\src\mail\Message.cpp
# End Source File
# Begin Source File

SOURCE=.\src\mail\MessageCC.cpp
# End Source File
# Begin Source File

SOURCE=.\src\mail\SendMessageCC.cpp
# End Source File
# End Group
# Begin Group "gui"

# PROP Default_Filter "*.cpp"
# Begin Group "layout"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\gui\wxllist.cpp

!IF  "$(CFG)" == "M - Win32 Release"

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# ADD BASE CPP /Yu
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gui\wxlparser.cpp

!IF  "$(CFG)" == "M - Win32 Release"

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# ADD BASE CPP /Yu
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gui\wxlwindow.cpp

!IF  "$(CFG)" == "M - Win32 Release"

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# ADD BASE CPP /Yu
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\src\gui\Mdnd.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\MImport.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\wxBrowseButton.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\wxComposeView.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\wxDialogLayout.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\wxFiltersDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\wxFolderMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\wxFolderTree.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\wxFolderView.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\wxHeadersDialogs.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\wxIconManager.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\wxMainFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\wxMApp.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\wxMDialogs.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\wxMenuDefs.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\wxMessageView.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\wxMFolderDialogs.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\wxMFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\wxModulesDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\wxOptionsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\wxSubfoldersDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\wxTemplateDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gui\wxWizards.cpp
# End Source File
# End Group
# Begin Group "Resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\res\M.rc
# End Source File
# End Group
# Begin Group "adb"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=.\src\adb\AdbDialogs.cpp
# End Source File
# Begin Source File

SOURCE=.\src\adb\AdbEntry.cpp
# End Source File
# Begin Source File

SOURCE=.\src\adb\AdbExport.cpp
# End Source File
# Begin Source File

SOURCE=.\src\adb\AdbFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\adb\AdbImport.cpp
# End Source File
# Begin Source File

SOURCE=.\src\adb\AdbManager.cpp
# End Source File
# Begin Source File

SOURCE=.\src\adb\AdbModule.cpp
# End Source File
# Begin Source File

SOURCE=.\src\adb\AdbProvider.cpp
# End Source File
# Begin Source File

SOURCE=.\src\adb\Collect.cpp
# End Source File
# Begin Source File

SOURCE=.\src\adb\ExportPalm.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\adb\ExportText.cpp
# End Source File
# Begin Source File

SOURCE=.\src\adb\ExportVCard.cpp
# End Source File
# Begin Source File

SOURCE=.\src\adb\ImportEudora.cpp

!IF  "$(CFG)" == "M - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\adb\ImportMailrc.cpp

!IF  "$(CFG)" == "M - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\adb\ImportPine.cpp

!IF  "$(CFG)" == "M - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\adb\ImportText.cpp

!IF  "$(CFG)" == "M - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\adb\ImportVCard.cpp
# End Source File
# Begin Source File

SOURCE=.\src\adb\ImportXFMail.cpp

!IF  "$(CFG)" == "M - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\adb\ProvBbdb.cpp

!IF  "$(CFG)" == "M - Win32 Release"

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\adb\ProvDummy.cpp
# End Source File
# Begin Source File

SOURCE=.\src\adb\ProvFC.cpp
# End Source File
# Begin Source File

SOURCE=.\src\adb\ProvPalm.cpp
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "python"

# PROP Default_Filter ""
# Begin Group "SWIG-IN"

# PROP Default_Filter "*.i"
# Begin Source File

SOURCE=.\src\Python\MailFolder.i

!IF  "$(CFG)" == "M - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Python\MAppBase.i

!IF  "$(CFG)" == "M - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Python\Message.i

!IF  "$(CFG)" == "M - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Python\MObject.i

!IF  "$(CFG)" == "M - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Python\MProfile.i
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\Python\MString.i
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "SWIG-OUT"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=".\src\Python\MailFolder.cpp-swig"

!IF  "$(CFG)" == "M - Win32 Release"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\src\Python\MAppBase.cpp-swig"

!IF  "$(CFG)" == "M - Win32 Release"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\src\Python\Message.cpp-swig"

!IF  "$(CFG)" == "M - Win32 Release"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\src\Python\MObject.cpp-swig"

!IF  "$(CFG)" == "M - Win32 Release"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\src\Python\MProfile.cpp-swig"

!IF  "$(CFG)" == "M - Win32 Release"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\src\Python\MString.cpp-swig"

!IF  "$(CFG)" == "M - Win32 Release"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\src\Python\MTest.cpp-swig"

!IF  "$(CFG)" == "M - Win32 Release"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\src\Python\swiglib.cpp-swig"

!IF  "$(CFG)" == "M - Win32 Release"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\src\Python\InitPython.cpp

!IF  "$(CFG)" == "M - Win32 Release"

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP BASE Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP BASE Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Python\MailFolder_swig.cpp

!IF  "$(CFG)" == "M - Win32 Release"

# ADD CPP /U "USE_PCH"

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# ADD CPP /U "USE_PCH"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP BASE Exclude_From_Build 1
# ADD CPP /U "USE_PCH"

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP BASE Exclude_From_Build 1
# ADD CPP /U "USE_PCH"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Python\MAppBase_swig.cpp

!IF  "$(CFG)" == "M - Win32 Release"

# ADD CPP /U "USE_PCH"

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# ADD CPP /U "USE_PCH"
# SUBTRACT CPP /D "USE_PCH" /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP BASE Exclude_From_Build 1
# ADD CPP /U "USE_PCH"

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP BASE Exclude_From_Build 1
# ADD CPP /U "USE_PCH"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Python\Message_swig.cpp

!IF  "$(CFG)" == "M - Win32 Release"

# ADD CPP /U "USE_PCH"

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# ADD CPP /U "USE_PCH"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP BASE Exclude_From_Build 1
# ADD CPP /U "USE_PCH"

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP BASE Exclude_From_Build 1
# ADD CPP /U "USE_PCH"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Python\MObject_swig.cpp

!IF  "$(CFG)" == "M - Win32 Release"

# ADD CPP /U "USE_PCH"

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# ADD CPP /U "USE_PCH"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP BASE Exclude_From_Build 1
# ADD CPP /U "USE_PCH"

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP BASE Exclude_From_Build 1
# ADD CPP /U "USE_PCH"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Python\MProfile_swig.cpp
# ADD CPP /U "USE_PCH"
# SUBTRACT CPP /D "USE_PCH" /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\src\Python\MString_swig.cpp
# ADD CPP /U "USE_PCH"
# SUBTRACT CPP /D "USE_PCH" /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\src\Python\PythonHelp.cpp

!IF  "$(CFG)" == "M - Win32 Release"

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP BASE Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP BASE Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Python\swiglib_swig.cpp
# ADD CPP /U "USE_PCH"
# SUBTRACT CPP /D "USE_PCH" /YX /Yc /Yu
# End Source File
# End Group
# Begin Group "wx"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\wx\generic\persctrl.cpp
# ADD CPP /Yu"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=.\src\wx\common\vcard.cpp
# ADD CPP /Yu"wx/wxprec.h"
# End Source File
# End Group
# Begin Group "interfaces"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\include\MInterface.mid

!IF  "$(CFG)" == "M - Win32 Release"

# Begin Custom Build
InputDir=.\include
InputPath=.\include\MInterface.mid

BuildCmds= \
	N:\Unix\Cygwin\cygwin-b20\H-i586-cygwin32\bin\M4.exe -DM4FILE=$(InputDir)/mid2cpp.m4 $(InputPath) > $(InputDir)/MInterface.cpp \
	N:\Unix\Cygwin\cygwin-b20\H-i586-cygwin32\bin\M4.exe -DM4FILE=$(InputDir)/mid2h.m4 $(InputPath) > $(InputDir)/MInterface.h \
	

"$(InputDir)/MInterface.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)/MInterface.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# Begin Custom Build - Creating MInterface.h and .cpp
InputDir=.\include
InputPath=.\include\MInterface.mid

BuildCmds= \
	N:\Unix\Cygwin\cygwin-b20\H-i586-cygwin32\bin\M4.exe -DM4FILE=$(InputDir)/mid2cpp.m4 $(InputPath) > $(InputDir)/MInterface.cpp \
	N:\Unix\Cygwin\cygwin-b20\H-i586-cygwin32\bin\M4.exe -DM4FILE=$(InputDir)/mid2h.m4 $(InputPath) > $(InputDir)/MInterface.h \
	

"$(InputDir)/MInterface.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)/MInterface.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# Begin Custom Build - Creating MInterface.h and .cpp
InputDir=.\include
InputPath=.\include\MInterface.mid

BuildCmds= \
	N:\Unix\Cygwin\cygwin-b20\H-i586-cygwin32\bin\M4.exe -DM4FILE=$(InputDir)/mid2cpp.m4 $(InputPath) > $(InputDir)/MInterface.cpp \
	N:\Unix\Cygwin\cygwin-b20\H-i586-cygwin32\bin\M4.exe -DM4FILE=$(InputDir)/mid2h.m4 $(InputPath) > $(InputDir)/MInterface.h \
	

"$(InputDir)/MInterface.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)/MInterface.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# Begin Custom Build
InputDir=.\include
InputPath=.\include\MInterface.mid

BuildCmds= \
	N:\Unix\Cygwin\cygwin-b20\H-i586-cygwin32\bin\M4.exe -DM4FILE=$(InputDir)/mid2cpp.m4 $(InputPath) > $(InputDir)/MInterface.cpp \
	N:\Unix\Cygwin\cygwin-b20\H-i586-cygwin32\bin\M4.exe -DM4FILE=$(InputDir)/mid2h.m4 $(InputPath) > $(InputDir)/MInterface.h \
	

"$(InputDir)/MInterface.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)/MInterface.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "import"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\modules\PineImport.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\modules\XFMailImport.cpp
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "modules"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\modules\Calendar.cpp
# End Source File
# Begin Source File

SOURCE=.\src\modules\Filters.cpp
# End Source File
# End Group
# Begin Group "util"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\util\miscutil.cpp
# End Source File
# Begin Source File

SOURCE=.\src\util\strutil.cpp
# End Source File
# Begin Source File

SOURCE=.\src\util\sysutil.cpp
# End Source File
# Begin Source File

SOURCE=.\src\util\twofish2.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\src\util\upgrade.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\include\config_nt.h

!IF  "$(CFG)" == "M - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Copying $(InputPath) to config.h...
InputDir=.\include
InputPath=.\include\config_nt.h

"$(InputDir)\config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy $(InputPath) $(InputDir)\config.h

# End Custom Build

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Copying $(InputPath) to config.h...
InputDir=.\include
InputPath=.\include\config_nt.h

"$(InputDir)\config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy $(InputPath) $(InputDir)\config.h

# End Custom Build

!ELSEIF  "$(CFG)" == "M - Win32 Debug DLL"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Copying $(InputPath) to config.h...
InputDir=.\include
InputPath=.\include\config_nt.h

"$(InputDir)\config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy $(InputPath) $(InputDir)\config.h

# End Custom Build

!ELSEIF  "$(CFG)" == "M - Win32 Release DLL"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Copying $(InputPath) to config.h...
InputDir=.\include
InputPath=.\include\config_nt.h

"$(InputDir)\config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy $(InputPath) $(InputDir)\config.h

# End Custom Build

!ENDIF 

# End Source File
# End Target
# End Project
