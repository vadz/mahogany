# Microsoft Developer Studio Generated NMAKE File, Based on M.DSP
!IF "$(CFG)" == ""
CFG=M - Win32 Debug
!MESSAGE No configuration specified. Defaulting to M - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "M - Win32 Release" && "$(CFG)" != "M - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "M.MAK" CFG="M - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "M - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "M - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "M - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\M.exe"

!ELSE 

ALL : "wxWindows - Win32 Release" "$(OUTDIR)\M.exe"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"wxWindows - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\AdbEntry.obj"
	-@erase "$(INTDIR)\AdbFrame.obj"
	-@erase "$(INTDIR)\AdbManager.obj"
	-@erase "$(INTDIR)\AdbProvider.obj"
	-@erase "$(INTDIR)\InitPython.obj"
	-@erase "$(INTDIR)\kbList.obj"
	-@erase "$(INTDIR)\M.res"
	-@erase "$(INTDIR)\MailCollector.obj"
	-@erase "$(INTDIR)\MailFolder.obj"
	-@erase "$(INTDIR)\MailFolder_swig.obj"
	-@erase "$(INTDIR)\MailFolderCC.obj"
	-@erase "$(INTDIR)\MAppBase_swig.obj"
	-@erase "$(INTDIR)\MApplication.obj"
	-@erase "$(INTDIR)\Message.obj"
	-@erase "$(INTDIR)\Message_swig.obj"
	-@erase "$(INTDIR)\MessageCC.obj"
	-@erase "$(INTDIR)\MEvent.obj"
	-@erase "$(INTDIR)\MFolder.obj"
	-@erase "$(INTDIR)\miscutil.obj"
	-@erase "$(INTDIR)\MObject.obj"
	-@erase "$(INTDIR)\MObject_swig.obj"
	-@erase "$(INTDIR)\PathFinder.obj"
	-@erase "$(INTDIR)\persctrl.obj"
	-@erase "$(INTDIR)\Profile.obj"
	-@erase "$(INTDIR)\Profile_swig.obj"
	-@erase "$(INTDIR)\ProvDummy.obj"
	-@erase "$(INTDIR)\ProvFC.obj"
	-@erase "$(INTDIR)\PythonHelp.obj"
	-@erase "$(INTDIR)\SendMessageCC.obj"
	-@erase "$(INTDIR)\String_swig.obj"
	-@erase "$(INTDIR)\strutil.obj"
	-@erase "$(INTDIR)\sysutil.obj"
	-@erase "$(INTDIR)\upgrade.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\wxBrowseButton.obj"
	-@erase "$(INTDIR)\wxComposeView.obj"
	-@erase "$(INTDIR)\wxDialogLayout.obj"
	-@erase "$(INTDIR)\wxFolderTree.obj"
	-@erase "$(INTDIR)\wxFolderView.obj"
	-@erase "$(INTDIR)\wxFontManager.obj"
	-@erase "$(INTDIR)\wxHeadersDialogs.obj"
	-@erase "$(INTDIR)\wxIconManager.obj"
	-@erase "$(INTDIR)\wxllist.obj"
	-@erase "$(INTDIR)\wxlparser.obj"
	-@erase "$(INTDIR)\wxlwindow.obj"
	-@erase "$(INTDIR)\wxMainFrame.obj"
	-@erase "$(INTDIR)\wxMApp.obj"
	-@erase "$(INTDIR)\wxMDialogs.obj"
	-@erase "$(INTDIR)\wxMenuDefs.obj"
	-@erase "$(INTDIR)\wxMessageView.obj"
	-@erase "$(INTDIR)\wxMFolderDialogs.obj"
	-@erase "$(INTDIR)\wxMFrame.obj"
	-@erase "$(INTDIR)\wxOptionsDlg.obj"
	-@erase "$(INTDIR)\XFace.obj"
	-@erase "$(OUTDIR)\M.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "include" /I "\libs\wxwindows\include" /I "D:\Progs\imap-4.2\cclient" /I "\python\include" /D "NDEBUG" /D "USE_PCH" /D "_WINDOWS" /D WIN95=1 /D _X86_=1 /D "__WINDOWS__" /D "USE_NEW_IOSTREAM" /D "WX_DRAG_DROP" /D "WX_OWNER_DRAWN" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /Fp"$(INTDIR)\M.pch" /Yu"Mpch.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\M.res" /i "\libs\wxwin2\include" /i "include" /i "res" /i "$(wx)\include" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"Debug/M.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=D:\Libs\wxWindows\Release\wxWindows.lib D:\Libs\wxWindows\contrib\WXXPM\Release\wxXPM.lib D:\Progs\imap-4.2\cclient\Release\cclient.lib D:\Python\PCbuild\Release\python15.lib rpcrt4.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib winmm.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\M.pdb" /machine:I386 /out:"$(OUTDIR)\M.exe" 
LINK32_OBJS= \
	"$(INTDIR)\kbList.obj" \
	"$(INTDIR)\MApplication.obj" \
	"$(INTDIR)\MEvent.obj" \
	"$(INTDIR)\MFolder.obj" \
	"$(INTDIR)\miscutil.obj" \
	"$(INTDIR)\MObject.obj" \
	"$(INTDIR)\PathFinder.obj" \
	"$(INTDIR)\Profile.obj" \
	"$(INTDIR)\strutil.obj" \
	"$(INTDIR)\sysutil.obj" \
	"$(INTDIR)\upgrade.obj" \
	"$(INTDIR)\XFace.obj" \
	"$(INTDIR)\MailFolder.obj" \
	"$(INTDIR)\MailFolderCC.obj" \
	"$(INTDIR)\Message.obj" \
	"$(INTDIR)\MessageCC.obj" \
	"$(INTDIR)\SendMessageCC.obj" \
	"$(INTDIR)\wxllist.obj" \
	"$(INTDIR)\wxlparser.obj" \
	"$(INTDIR)\wxlwindow.obj" \
	"$(INTDIR)\wxBrowseButton.obj" \
	"$(INTDIR)\wxComposeView.obj" \
	"$(INTDIR)\wxDialogLayout.obj" \
	"$(INTDIR)\wxFolderTree.obj" \
	"$(INTDIR)\wxFolderView.obj" \
	"$(INTDIR)\wxFontManager.obj" \
	"$(INTDIR)\wxHeadersDialogs.obj" \
	"$(INTDIR)\wxIconManager.obj" \
	"$(INTDIR)\wxMainFrame.obj" \
	"$(INTDIR)\wxMApp.obj" \
	"$(INTDIR)\wxMDialogs.obj" \
	"$(INTDIR)\wxMenuDefs.obj" \
	"$(INTDIR)\wxMessageView.obj" \
	"$(INTDIR)\wxMFolderDialogs.obj" \
	"$(INTDIR)\wxMFrame.obj" \
	"$(INTDIR)\wxOptionsDlg.obj" \
	"$(INTDIR)\AdbEntry.obj" \
	"$(INTDIR)\AdbFrame.obj" \
	"$(INTDIR)\AdbManager.obj" \
	"$(INTDIR)\AdbProvider.obj" \
	"$(INTDIR)\ProvDummy.obj" \
	"$(INTDIR)\ProvFC.obj" \
	"$(INTDIR)\MailFolder_swig.obj" \
	"$(INTDIR)\MAppBase_swig.obj" \
	"$(INTDIR)\Message_swig.obj" \
	"$(INTDIR)\MObject_swig.obj" \
	"$(INTDIR)\Profile_swig.obj" \
	"$(INTDIR)\String_swig.obj" \
	"$(INTDIR)\InitPython.obj" \
	"$(INTDIR)\PythonHelp.obj" \
	"$(INTDIR)\persctrl.obj" \
	"$(INTDIR)\M.res" \
	"$(INTDIR)\MailCollector.obj" \
	"..\..\Libs\wxWindows\Release\wxWindows.lib"

"$(OUTDIR)\M.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\M.exe" "$(OUTDIR)\M.bsc"

!ELSE 

ALL : "wxWindows - Win32 Debug" "$(OUTDIR)\M.exe" "$(OUTDIR)\M.bsc"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"wxWindows - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\AdbEntry.obj"
	-@erase "$(INTDIR)\AdbEntry.sbr"
	-@erase "$(INTDIR)\AdbFrame.obj"
	-@erase "$(INTDIR)\AdbFrame.sbr"
	-@erase "$(INTDIR)\AdbManager.obj"
	-@erase "$(INTDIR)\AdbManager.sbr"
	-@erase "$(INTDIR)\AdbProvider.obj"
	-@erase "$(INTDIR)\AdbProvider.sbr"
	-@erase "$(INTDIR)\kbList.obj"
	-@erase "$(INTDIR)\kbList.sbr"
	-@erase "$(INTDIR)\M.pch"
	-@erase "$(INTDIR)\M.res"
	-@erase "$(INTDIR)\MailCollector.obj"
	-@erase "$(INTDIR)\MailCollector.sbr"
	-@erase "$(INTDIR)\MailFolder.obj"
	-@erase "$(INTDIR)\MailFolder.sbr"
	-@erase "$(INTDIR)\MailFolderCC.obj"
	-@erase "$(INTDIR)\MailFolderCC.sbr"
	-@erase "$(INTDIR)\MApplication.obj"
	-@erase "$(INTDIR)\MApplication.sbr"
	-@erase "$(INTDIR)\Message.obj"
	-@erase "$(INTDIR)\Message.sbr"
	-@erase "$(INTDIR)\MessageCC.obj"
	-@erase "$(INTDIR)\MessageCC.sbr"
	-@erase "$(INTDIR)\MEvent.obj"
	-@erase "$(INTDIR)\MEvent.sbr"
	-@erase "$(INTDIR)\MFolder.obj"
	-@erase "$(INTDIR)\MFolder.sbr"
	-@erase "$(INTDIR)\miscutil.obj"
	-@erase "$(INTDIR)\miscutil.sbr"
	-@erase "$(INTDIR)\MObject.obj"
	-@erase "$(INTDIR)\MObject.sbr"
	-@erase "$(INTDIR)\PathFinder.obj"
	-@erase "$(INTDIR)\PathFinder.sbr"
	-@erase "$(INTDIR)\persctrl.obj"
	-@erase "$(INTDIR)\persctrl.sbr"
	-@erase "$(INTDIR)\Profile.obj"
	-@erase "$(INTDIR)\Profile.sbr"
	-@erase "$(INTDIR)\ProvDummy.obj"
	-@erase "$(INTDIR)\ProvDummy.sbr"
	-@erase "$(INTDIR)\ProvFC.obj"
	-@erase "$(INTDIR)\ProvFC.sbr"
	-@erase "$(INTDIR)\SendMessageCC.obj"
	-@erase "$(INTDIR)\SendMessageCC.sbr"
	-@erase "$(INTDIR)\strutil.obj"
	-@erase "$(INTDIR)\strutil.sbr"
	-@erase "$(INTDIR)\sysutil.obj"
	-@erase "$(INTDIR)\sysutil.sbr"
	-@erase "$(INTDIR)\upgrade.obj"
	-@erase "$(INTDIR)\upgrade.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\wxBrowseButton.obj"
	-@erase "$(INTDIR)\wxBrowseButton.sbr"
	-@erase "$(INTDIR)\wxComposeView.obj"
	-@erase "$(INTDIR)\wxComposeView.sbr"
	-@erase "$(INTDIR)\wxDialogLayout.obj"
	-@erase "$(INTDIR)\wxDialogLayout.sbr"
	-@erase "$(INTDIR)\wxFolderTree.obj"
	-@erase "$(INTDIR)\wxFolderTree.sbr"
	-@erase "$(INTDIR)\wxFolderView.obj"
	-@erase "$(INTDIR)\wxFolderView.sbr"
	-@erase "$(INTDIR)\wxFontManager.obj"
	-@erase "$(INTDIR)\wxFontManager.sbr"
	-@erase "$(INTDIR)\wxHeadersDialogs.obj"
	-@erase "$(INTDIR)\wxHeadersDialogs.sbr"
	-@erase "$(INTDIR)\wxIconManager.obj"
	-@erase "$(INTDIR)\wxIconManager.sbr"
	-@erase "$(INTDIR)\wxllist.obj"
	-@erase "$(INTDIR)\wxllist.sbr"
	-@erase "$(INTDIR)\wxlparser.obj"
	-@erase "$(INTDIR)\wxlparser.sbr"
	-@erase "$(INTDIR)\wxlwindow.obj"
	-@erase "$(INTDIR)\wxlwindow.sbr"
	-@erase "$(INTDIR)\wxMainFrame.obj"
	-@erase "$(INTDIR)\wxMainFrame.sbr"
	-@erase "$(INTDIR)\wxMApp.obj"
	-@erase "$(INTDIR)\wxMApp.sbr"
	-@erase "$(INTDIR)\wxMDialogs.obj"
	-@erase "$(INTDIR)\wxMDialogs.sbr"
	-@erase "$(INTDIR)\wxMenuDefs.obj"
	-@erase "$(INTDIR)\wxMenuDefs.sbr"
	-@erase "$(INTDIR)\wxMessageView.obj"
	-@erase "$(INTDIR)\wxMessageView.sbr"
	-@erase "$(INTDIR)\wxMFolderDialogs.obj"
	-@erase "$(INTDIR)\wxMFolderDialogs.sbr"
	-@erase "$(INTDIR)\wxMFrame.obj"
	-@erase "$(INTDIR)\wxMFrame.sbr"
	-@erase "$(INTDIR)\wxOptionsDlg.obj"
	-@erase "$(INTDIR)\wxOptionsDlg.sbr"
	-@erase "$(INTDIR)\XFace.obj"
	-@erase "$(INTDIR)\XFace.sbr"
	-@erase "$(OUTDIR)\M.bsc"
	-@erase "$(OUTDIR)\M.exe"
	-@erase "$(OUTDIR)\M.ilk"
	-@erase "$(OUTDIR)\M.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MLd /W3 /Gm /ZI /Od /I "include" /I "\libs\wxwindows\include" /I "\Progs\imap-4.2\cclient" /I "\python\include" /D "_DEBUG" /D "DEBUG" /D WXDEBUG=1 /D "__WXDEBUG__" /D "__WIN95__" /D "_WIN32" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\M.pch" /Yu"Mpch.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\M.res" /i "\libs\wxwindows\include" /i "include" /i "res" /i "$(wx)\include" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\M.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\kbList.sbr" \
	"$(INTDIR)\MApplication.sbr" \
	"$(INTDIR)\MEvent.sbr" \
	"$(INTDIR)\MFolder.sbr" \
	"$(INTDIR)\miscutil.sbr" \
	"$(INTDIR)\MObject.sbr" \
	"$(INTDIR)\PathFinder.sbr" \
	"$(INTDIR)\Profile.sbr" \
	"$(INTDIR)\strutil.sbr" \
	"$(INTDIR)\sysutil.sbr" \
	"$(INTDIR)\upgrade.sbr" \
	"$(INTDIR)\XFace.sbr" \
	"$(INTDIR)\MailFolder.sbr" \
	"$(INTDIR)\MailFolderCC.sbr" \
	"$(INTDIR)\Message.sbr" \
	"$(INTDIR)\MessageCC.sbr" \
	"$(INTDIR)\SendMessageCC.sbr" \
	"$(INTDIR)\wxllist.sbr" \
	"$(INTDIR)\wxlparser.sbr" \
	"$(INTDIR)\wxlwindow.sbr" \
	"$(INTDIR)\wxBrowseButton.sbr" \
	"$(INTDIR)\wxComposeView.sbr" \
	"$(INTDIR)\wxDialogLayout.sbr" \
	"$(INTDIR)\wxFolderTree.sbr" \
	"$(INTDIR)\wxFolderView.sbr" \
	"$(INTDIR)\wxFontManager.sbr" \
	"$(INTDIR)\wxHeadersDialogs.sbr" \
	"$(INTDIR)\wxIconManager.sbr" \
	"$(INTDIR)\wxMainFrame.sbr" \
	"$(INTDIR)\wxMApp.sbr" \
	"$(INTDIR)\wxMDialogs.sbr" \
	"$(INTDIR)\wxMenuDefs.sbr" \
	"$(INTDIR)\wxMessageView.sbr" \
	"$(INTDIR)\wxMFolderDialogs.sbr" \
	"$(INTDIR)\wxMFrame.sbr" \
	"$(INTDIR)\wxOptionsDlg.sbr" \
	"$(INTDIR)\AdbEntry.sbr" \
	"$(INTDIR)\AdbFrame.sbr" \
	"$(INTDIR)\AdbManager.sbr" \
	"$(INTDIR)\AdbProvider.sbr" \
	"$(INTDIR)\ProvDummy.sbr" \
	"$(INTDIR)\ProvFC.sbr" \
	"$(INTDIR)\persctrl.sbr" \
	"$(INTDIR)\MailCollector.sbr"

"$(OUTDIR)\M.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=rpcrt4.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib winmm.lib \Libs\wxWindows\debug\wxWindows.lib \Libs\wxWindows\contrib\WXXPM\debug\xpm.lib \Progs\imap-4.2\cclient\Debug\cclient.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\M.pdb" /debug /machine:I386 /nodefaultlib:"libcmt.lib" /out:"$(OUTDIR)\M.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\kbList.obj" \
	"$(INTDIR)\MApplication.obj" \
	"$(INTDIR)\MEvent.obj" \
	"$(INTDIR)\MFolder.obj" \
	"$(INTDIR)\miscutil.obj" \
	"$(INTDIR)\MObject.obj" \
	"$(INTDIR)\PathFinder.obj" \
	"$(INTDIR)\Profile.obj" \
	"$(INTDIR)\strutil.obj" \
	"$(INTDIR)\sysutil.obj" \
	"$(INTDIR)\upgrade.obj" \
	"$(INTDIR)\XFace.obj" \
	"$(INTDIR)\MailFolder.obj" \
	"$(INTDIR)\MailFolderCC.obj" \
	"$(INTDIR)\Message.obj" \
	"$(INTDIR)\MessageCC.obj" \
	"$(INTDIR)\SendMessageCC.obj" \
	"$(INTDIR)\wxllist.obj" \
	"$(INTDIR)\wxlparser.obj" \
	"$(INTDIR)\wxlwindow.obj" \
	"$(INTDIR)\wxBrowseButton.obj" \
	"$(INTDIR)\wxComposeView.obj" \
	"$(INTDIR)\wxDialogLayout.obj" \
	"$(INTDIR)\wxFolderTree.obj" \
	"$(INTDIR)\wxFolderView.obj" \
	"$(INTDIR)\wxFontManager.obj" \
	"$(INTDIR)\wxHeadersDialogs.obj" \
	"$(INTDIR)\wxIconManager.obj" \
	"$(INTDIR)\wxMainFrame.obj" \
	"$(INTDIR)\wxMApp.obj" \
	"$(INTDIR)\wxMDialogs.obj" \
	"$(INTDIR)\wxMenuDefs.obj" \
	"$(INTDIR)\wxMessageView.obj" \
	"$(INTDIR)\wxMFolderDialogs.obj" \
	"$(INTDIR)\wxMFrame.obj" \
	"$(INTDIR)\wxOptionsDlg.obj" \
	"$(INTDIR)\AdbEntry.obj" \
	"$(INTDIR)\AdbFrame.obj" \
	"$(INTDIR)\AdbManager.obj" \
	"$(INTDIR)\AdbProvider.obj" \
	"$(INTDIR)\ProvDummy.obj" \
	"$(INTDIR)\ProvFC.obj" \
	"$(INTDIR)\persctrl.obj" \
	"$(INTDIR)\M.res" \
	"$(INTDIR)\MailCollector.obj" \
	"..\..\Libs\wxWindows\Debug\wxWindows.lib"

"$(OUTDIR)\M.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("M.DEP")
!INCLUDE "M.DEP"
!ELSE 
!MESSAGE Warning: cannot find "M.DEP"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "M - Win32 Release" || "$(CFG)" == "M - Win32 Debug"
SOURCE=.\src\classes\kbList.cpp

!IF  "$(CFG)" == "M - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "include" /I "\libs\wxwindows\include" /I "D:\Progs\imap-4.2\cclient" /I "\python\include" /D "NDEBUG" /D "USE_PCH" /D "_WINDOWS" /D WIN95=1 /D _X86_=1 /D "__WINDOWS__" /D "USE_NEW_IOSTREAM" /D "WX_DRAG_DROP" /D "WX_OWNER_DRAWN" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\kbList.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "M - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /ZI /Od /I "include" /I "\libs\wxwindows\include" /I "\Progs\imap-4.2\cclient" /I "\python\include" /D "_DEBUG" /D "DEBUG" /D WXDEBUG=1 /D "__WXDEBUG__" /D "__WIN95__" /D "_WIN32" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\kbList.obj"	"$(INTDIR)\kbList.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\src\classes\MailCollector.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\MailCollector.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\MailCollector.obj"	"$(INTDIR)\MailCollector.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\classes\MApplication.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\MApplication.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\MApplication.obj"	"$(INTDIR)\MApplication.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\classes\MEvent.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\MEvent.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\MEvent.obj"	"$(INTDIR)\MEvent.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\classes\MFolder.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\MFolder.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\MFolder.obj"	"$(INTDIR)\MFolder.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\util\miscutil.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\miscutil.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\miscutil.obj"	"$(INTDIR)\miscutil.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\classes\MObject.cpp

!IF  "$(CFG)" == "M - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "include" /I "\libs\wxwindows\include" /I "D:\Progs\imap-4.2\cclient" /I "\python\include" /D "NDEBUG" /D "USE_PCH" /D "_WINDOWS" /D WIN95=1 /D _X86_=1 /D "__WINDOWS__" /D "USE_NEW_IOSTREAM" /D "WX_DRAG_DROP" /D "WX_OWNER_DRAWN" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /Fp"$(INTDIR)\M.pch" /Yu"Mpch.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\MObject.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "M - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /ZI /Od /I "include" /I "\libs\wxwindows\include" /I "\Progs\imap-4.2\cclient" /I "\python\include" /D "_DEBUG" /D "DEBUG" /D WXDEBUG=1 /D "__WXDEBUG__" /D "__WIN95__" /D "_WIN32" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\M.pch" /Yc"Mpch.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\MObject.obj"	"$(INTDIR)\MObject.sbr"	"$(INTDIR)\M.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\src\classes\PathFinder.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\PathFinder.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\PathFinder.obj"	"$(INTDIR)\PathFinder.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\classes\Profile.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\Profile.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\Profile.obj"	"$(INTDIR)\Profile.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\util\strutil.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\strutil.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\strutil.obj"	"$(INTDIR)\strutil.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\util\sysutil.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\sysutil.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\sysutil.obj"	"$(INTDIR)\sysutil.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\util\upgrade.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\upgrade.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\upgrade.obj"	"$(INTDIR)\upgrade.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\classes\XFace.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\XFace.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\XFace.obj"	"$(INTDIR)\XFace.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\mail\MailFolder.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\MailFolder.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\MailFolder.obj"	"$(INTDIR)\MailFolder.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\mail\MailFolderCC.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\MailFolderCC.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\MailFolderCC.obj"	"$(INTDIR)\MailFolderCC.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\mail\Message.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\Message.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\Message.obj"	"$(INTDIR)\Message.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\mail\MessageCC.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\MessageCC.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\MessageCC.obj"	"$(INTDIR)\MessageCC.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\mail\SendMessage.cpp
SOURCE=.\src\mail\SendMessageCC.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\SendMessageCC.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\SendMessageCC.obj"	"$(INTDIR)\SendMessageCC.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\gui\wxllist.cpp

!IF  "$(CFG)" == "M - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "include" /I "\libs\wxwindows\include" /I "D:\Progs\imap-4.2\cclient" /I "\python\include" /D "NDEBUG" /D "USE_PCH" /D "_WINDOWS" /D WIN95=1 /D _X86_=1 /D "__WINDOWS__" /D "USE_NEW_IOSTREAM" /D "WX_DRAG_DROP" /D "WX_OWNER_DRAWN" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /Fp"$(INTDIR)\M.pch" /Yu"Mpch.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\wxllist.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "M - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /ZI /Od /I "include" /I "\libs\wxwindows\include" /I "\Progs\imap-4.2\cclient" /I "\python\include" /D "_DEBUG" /D "DEBUG" /D WXDEBUG=1 /D "__WXDEBUG__" /D "__WIN95__" /D "_WIN32" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\M.pch" /Yu"Mpch.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\wxllist.obj"	"$(INTDIR)\wxllist.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\src\gui\wxlparser.cpp

!IF  "$(CFG)" == "M - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "include" /I "\libs\wxwindows\include" /I "D:\Progs\imap-4.2\cclient" /I "\python\include" /D "NDEBUG" /D "USE_PCH" /D "_WINDOWS" /D WIN95=1 /D _X86_=1 /D "__WINDOWS__" /D "USE_NEW_IOSTREAM" /D "WX_DRAG_DROP" /D "WX_OWNER_DRAWN" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /Fp"$(INTDIR)\M.pch" /Yu"Mpch.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\wxlparser.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "M - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /ZI /Od /I "include" /I "\libs\wxwindows\include" /I "\Progs\imap-4.2\cclient" /I "\python\include" /D "_DEBUG" /D "DEBUG" /D WXDEBUG=1 /D "__WXDEBUG__" /D "__WIN95__" /D "_WIN32" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\M.pch" /Yu"Mpch.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\wxlparser.obj"	"$(INTDIR)\wxlparser.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\src\gui\wxlwindow.cpp

!IF  "$(CFG)" == "M - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "include" /I "\libs\wxwindows\include" /I "D:\Progs\imap-4.2\cclient" /I "\python\include" /D "NDEBUG" /D "USE_PCH" /D "_WINDOWS" /D WIN95=1 /D _X86_=1 /D "__WINDOWS__" /D "USE_NEW_IOSTREAM" /D "WX_DRAG_DROP" /D "WX_OWNER_DRAWN" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /Fp"$(INTDIR)\M.pch" /Yu"Mpch.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\wxlwindow.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "M - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /ZI /Od /I "include" /I "\libs\wxwindows\include" /I "\Progs\imap-4.2\cclient" /I "\python\include" /D "_DEBUG" /D "DEBUG" /D WXDEBUG=1 /D "__WXDEBUG__" /D "__WIN95__" /D "_WIN32" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\M.pch" /Yu"Mpch.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\wxlwindow.obj"	"$(INTDIR)\wxlwindow.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\src\gui\wxBrowseButton.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\wxBrowseButton.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\wxBrowseButton.obj"	"$(INTDIR)\wxBrowseButton.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\gui\wxComposeView.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\wxComposeView.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\wxComposeView.obj"	"$(INTDIR)\wxComposeView.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\gui\wxDialogLayout.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\wxDialogLayout.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\wxDialogLayout.obj"	"$(INTDIR)\wxDialogLayout.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\gui\wxFolderTree.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\wxFolderTree.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\wxFolderTree.obj"	"$(INTDIR)\wxFolderTree.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\gui\wxFolderView.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\wxFolderView.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\wxFolderView.obj"	"$(INTDIR)\wxFolderView.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\gui\wxFontManager.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\wxFontManager.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\wxFontManager.obj"	"$(INTDIR)\wxFontManager.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\gui\wxHeadersDialogs.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\wxHeadersDialogs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\wxHeadersDialogs.obj"	"$(INTDIR)\wxHeadersDialogs.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\gui\wxIconManager.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\wxIconManager.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\wxIconManager.obj"	"$(INTDIR)\wxIconManager.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\gui\wxMainFrame.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\wxMainFrame.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\wxMainFrame.obj"	"$(INTDIR)\wxMainFrame.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\gui\wxMApp.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\wxMApp.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\wxMApp.obj"	"$(INTDIR)\wxMApp.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\gui\wxMDialogs.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\wxMDialogs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\wxMDialogs.obj"	"$(INTDIR)\wxMDialogs.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\gui\wxMenuDefs.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\wxMenuDefs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\wxMenuDefs.obj"	"$(INTDIR)\wxMenuDefs.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\gui\wxMessageView.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\wxMessageView.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\wxMessageView.obj"	"$(INTDIR)\wxMessageView.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\gui\wxMFolderDialogs.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\wxMFolderDialogs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\wxMFolderDialogs.obj"	"$(INTDIR)\wxMFolderDialogs.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\gui\wxMFrame.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\wxMFrame.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\wxMFrame.obj"	"$(INTDIR)\wxMFrame.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\gui\wxOptionsDlg.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\wxOptionsDlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\wxOptionsDlg.obj"	"$(INTDIR)\wxOptionsDlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\res\M.rc

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\M.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) /l 0x409 /fo"$(INTDIR)\M.res" /i "include" /i "res" /i "$(wx)\include" /d "NDEBUG" $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\M.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) /l 0x409 /fo"$(INTDIR)\M.res" /i "include" /i "res" /i "$(wx)\include" /d "_DEBUG" $(SOURCE)


!ENDIF 

SOURCE=.\src\adb\AdbEntry.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\AdbEntry.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\AdbEntry.obj"	"$(INTDIR)\AdbEntry.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\adb\AdbFrame.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\AdbFrame.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\AdbFrame.obj"	"$(INTDIR)\AdbFrame.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\adb\AdbManager.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\AdbManager.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\AdbManager.obj"	"$(INTDIR)\AdbManager.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\adb\AdbProvider.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\AdbProvider.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\AdbProvider.obj"	"$(INTDIR)\AdbProvider.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\adb\ProvDummy.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\ProvDummy.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\ProvDummy.obj"	"$(INTDIR)\ProvDummy.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\adb\ProvFC.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\ProvFC.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"


"$(INTDIR)\ProvFC.obj"	"$(INTDIR)\ProvFC.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\Python\MailFolder_swig.cpp

!IF  "$(CFG)" == "M - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "include" /I "\libs\wxwindows\include" /I "D:\Progs\imap-4.2\cclient" /I "\python\include" /D "NDEBUG" /D "USE_PCH" /D "_WINDOWS" /D WIN95=1 /D _X86_=1 /D "__WINDOWS__" /D "USE_NEW_IOSTREAM" /D "WX_DRAG_DROP" /D "WX_OWNER_DRAWN" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\MailFolder_swig.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "M - Win32 Debug"

!ENDIF 

SOURCE=.\src\Python\MAppBase_swig.cpp

!IF  "$(CFG)" == "M - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "include" /I "\libs\wxwindows\include" /I "D:\Progs\imap-4.2\cclient" /I "\python\include" /D "NDEBUG" /D "USE_PCH" /D "_WINDOWS" /D WIN95=1 /D _X86_=1 /D "__WINDOWS__" /D "USE_NEW_IOSTREAM" /D "WX_DRAG_DROP" /D "WX_OWNER_DRAWN" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\MAppBase_swig.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "M - Win32 Debug"

!ENDIF 

SOURCE=.\src\Python\Message_swig.cpp

!IF  "$(CFG)" == "M - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "include" /I "\libs\wxwindows\include" /I "D:\Progs\imap-4.2\cclient" /I "\python\include" /D "NDEBUG" /D "USE_PCH" /D "_WINDOWS" /D WIN95=1 /D _X86_=1 /D "__WINDOWS__" /D "USE_NEW_IOSTREAM" /D "WX_DRAG_DROP" /D "WX_OWNER_DRAWN" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Message_swig.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "M - Win32 Debug"

!ENDIF 

SOURCE=.\src\Python\MObject_swig.cpp

!IF  "$(CFG)" == "M - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "include" /I "\libs\wxwindows\include" /I "D:\Progs\imap-4.2\cclient" /I "\python\include" /D "NDEBUG" /D "USE_PCH" /D "_WINDOWS" /D WIN95=1 /D _X86_=1 /D "__WINDOWS__" /D "USE_NEW_IOSTREAM" /D "WX_DRAG_DROP" /D "WX_OWNER_DRAWN" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /Fp"$(INTDIR)\M.pch" /Yu"Mpch.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\MObject_swig.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "M - Win32 Debug"

!ENDIF 

SOURCE=.\src\Python\Profile_swig.cpp

!IF  "$(CFG)" == "M - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "include" /I "\libs\wxwindows\include" /I "D:\Progs\imap-4.2\cclient" /I "\python\include" /D "NDEBUG" /D "USE_PCH" /D "_WINDOWS" /D WIN95=1 /D _X86_=1 /D "__WINDOWS__" /D "USE_NEW_IOSTREAM" /D "WX_DRAG_DROP" /D "WX_OWNER_DRAWN" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Profile_swig.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "M - Win32 Debug"

!ENDIF 

SOURCE=.\src\Python\String_swig.cpp

!IF  "$(CFG)" == "M - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "include" /I "\libs\wxwindows\include" /I "D:\Progs\imap-4.2\cclient" /I "\python\include" /D "NDEBUG" /D "USE_PCH" /D "_WINDOWS" /D WIN95=1 /D _X86_=1 /D "__WINDOWS__" /D "USE_NEW_IOSTREAM" /D "WX_DRAG_DROP" /D "WX_OWNER_DRAWN" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\String_swig.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "M - Win32 Debug"

!ENDIF 

SOURCE=.\src\Python\InitPython.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\InitPython.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"

!ENDIF 

SOURCE=.\src\Python\MailFolder.i

!IF  "$(CFG)" == "M - Win32 Release"

ProjDir=.
InputPath=.\src\Python\MailFolder.i
InputName=MailFolder

".\src\Python\MailFolder_swig.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	\tools\swig\swig.exe -python -c++ -shadow -I$(ProjDir)\src\python -o               $(ProjDir)\src\Python\$(InputName)_swig.cpp $(InputPath)
<< 
	

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

!ENDIF 

SOURCE=.\src\Python\MAppBase.i

!IF  "$(CFG)" == "M - Win32 Release"

ProjDir=.
InputPath=.\src\Python\MAppBase.i
InputName=MAppBase

".\src\Python\MAppBase_swig.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	\tools\swig\swig.exe -python -c++ -shadow -I$(ProjDir)\src\python -o               $(ProjDir)\src\Python\$(InputName)_swig.cpp $(InputPath)
<< 
	

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

!ENDIF 

SOURCE=.\src\Python\Message.i

!IF  "$(CFG)" == "M - Win32 Release"

ProjDir=.
InputPath=.\src\Python\Message.i
InputName=Message

".\src\Python\Message_swig.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	\tools\swig\swig.exe -python -c++ -shadow -I$(ProjDir)\src\python -o               $(ProjDir)\src\Python\$(InputName)_swig.cpp $(InputPath)
<< 
	

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

!ENDIF 

SOURCE=.\src\Python\Profile.i

!IF  "$(CFG)" == "M - Win32 Release"

ProjDir=.
InputPath=.\src\Python\Profile.i
InputName=Profile

".\src\Python\Profile_swig.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	\tools\swig\swig.exe -python -c++ -shadow -I$(ProjDir)\src\python -o               $(ProjDir)\src\Python\$(InputName)_swig.cpp $(InputPath)
<< 
	

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

!ENDIF 

SOURCE=.\src\Python\PythonHelp.cpp

!IF  "$(CFG)" == "M - Win32 Release"


"$(INTDIR)\PythonHelp.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "M - Win32 Debug"

!ENDIF 

SOURCE=.\src\Python\String.i

!IF  "$(CFG)" == "M - Win32 Release"

ProjDir=.
InputPath=.\src\Python\String.i
InputName=String

".\src\Python\String_swig.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	\tools\swig\swig.exe -python -c++ -shadow -I$(ProjDir)\src\python -o               $(ProjDir)\src\Python\$(InputName)_swig.cpp $(InputPath)
<< 
	

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

!ENDIF 

SOURCE=.\src\wx\generic\persctrl.cpp

!IF  "$(CFG)" == "M - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "include" /I "\libs\wxwindows\include" /I "D:\Progs\imap-4.2\cclient" /I "\python\include" /D "NDEBUG" /D "USE_PCH" /D "_WINDOWS" /D WIN95=1 /D _X86_=1 /D "__WINDOWS__" /D "USE_NEW_IOSTREAM" /D "WX_DRAG_DROP" /D "WX_OWNER_DRAWN" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /Fp"$(INTDIR)\M.pch" /Yu"Mpch.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\persctrl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "M - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /ZI /Od /I "include" /I "\libs\wxwindows\include" /I "\Progs\imap-4.2\cclient" /I "\python\include" /D "_DEBUG" /D "DEBUG" /D WXDEBUG=1 /D "__WXDEBUG__" /D "__WIN95__" /D "_WIN32" /D "WIN32" /D WINVER=0x400 /D "__WXMSW__" /D "__WIN32__" /D M_PREFIX="" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\M.pch" /Yu"wx/wxprec.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\persctrl.obj"	"$(INTDIR)\persctrl.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\M.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

!IF  "$(CFG)" == "M - Win32 Release"

"wxWindows - Win32 Release" : 
   cd "\Libs\wxWindows"
   $(MAKE) /$(MAKEFLAGS) /F .\wxWindows.mak CFG="wxWindows - Win32 Release" 
   cd "..\..\Progs\M"

"wxWindows - Win32 ReleaseCLEAN" : 
   cd "\Libs\wxWindows"
   $(MAKE) /$(MAKEFLAGS) /F .\wxWindows.mak CFG="wxWindows - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\Progs\M"

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

"wxWindows - Win32 Debug" : 
   cd "\Libs\wxWindows"
   $(MAKE) /$(MAKEFLAGS) /F .\wxWindows.mak CFG="wxWindows - Win32 Debug" 
   cd "..\..\Progs\M"

"wxWindows - Win32 DebugCLEAN" : 
   cd "\Libs\wxWindows"
   $(MAKE) /$(MAKEFLAGS) /F .\wxWindows.mak CFG="wxWindows - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\Progs\M"

!ENDIF 

!IF  "$(CFG)" == "M - Win32 Release"

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

!ENDIF 

!IF  "$(CFG)" == "M - Win32 Release"

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

!ENDIF 

!IF  "$(CFG)" == "M - Win32 Release"

!ELSEIF  "$(CFG)" == "M - Win32 Debug"

!ENDIF 


!ENDIF 

