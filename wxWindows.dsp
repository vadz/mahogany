# Microsoft Developer Studio Project File - Name="wxWindows" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=wxWindows - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "wxWindows.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "wxWindows.mak" CFG="wxWindows - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "wxWindows - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "wxWindows - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "wxWindows - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MT" /YX /FD /c
# ADD CPP /nologo /MD /W4 /Zi /O2 /I "$(wx)\include" /I "$(wx)\src\zlib" /I "$(wx)\src\jpeg" /I "$(wx)\src\png" /I "$(wx)\src\tiff" /D "NDEBUG" /D "_MT" /D wxUSE_GUI=1 /D WIN95=1 /D "__WIN95__" /D "WIN32" /D "_WIN32" /D WINVER=0x400 /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN32__" /Yu"wx/wxprec.h" /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "wxWindows - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" -D "_MT" /YX /FD /c
# ADD CPP /nologo /MDd /W4 /Zi /Od /I "$(wx)\include" /I "$(wx)\src\zlib" /I "$(wx)\src\jpeg" /I "$(wx)\src\png" /I "$(wx)\src\tiff" /D "_DEBUG" /D DEBUG=1 /D WXDEBUG=1 /D "__WXDEBUG__" /D "_MT" /D wxUSE_GUI=1 /D WIN95=1 /D "__WIN95__" /D "WIN32" /D "_WIN32" /D WINVER=0x400 /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN32__" /Yu"wx/wxprec.h" /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"lib/wxWindows.bsc"
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "wxWindows - Win32 Release"
# Name "wxWindows - Win32 Debug"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\msw\dummy.cpp
# ADD CPP /Yc"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=.\src\msw\accel.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\app.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\bitmap.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\bmpbuttn.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\brush.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\button.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\caret.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\checkbox.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\checklst.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\choice.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\clipbrd.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\colordlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\colour.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\combobox.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\control.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\curico.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\cursor.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\data.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\dc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\dcclient.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\dcmemory.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\dcprint.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\dcscreen.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\dde.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\dialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\dialup.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\dib.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\dibutils.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\dir.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\dirdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\dragimag.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\enhmeta.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\filedlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\font.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\fontdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\fontenum.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\fontutil.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\frame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\gauge95.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\gdiimage.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\gdiobj.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\glcanvas.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\helpchm.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\helpwin.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\icon.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\imaglist.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\joystick.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\listbox.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\listctrl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\main.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\mdi.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\menu.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\menuitem.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\metafile.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\mimetype.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\minifram.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\msgdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\nativdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\notebook.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\ownerdrw.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\palette.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\pen.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\penwin.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\printdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\printwin.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\radiobox.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\radiobut.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\regconf.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\region.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\registry.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\scrolbar.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\settings.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\slider95.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\spinbutt.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\spinctrl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\statbmp.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\statbox.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\statbr95.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\statline.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\stattext.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\tabctrl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\taskbar.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\tbar95.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\textctrl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\thread.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\timer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\tooltip.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\treectrl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\utils.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\utilsexc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\wave.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\window.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\xpmhand.cpp
# End Source File

# Begin Source File

SOURCE=.\src\msw\ole\automtn.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\ole\dataobj.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\ole\dropsrc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\ole\droptgt.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\ole\oleutils.cpp
# End Source File
# Begin Source File

SOURCE=.\src\msw\ole\uuid.cpp
# End Source File

# Begin Source File

SOURCE=.\src\generic\busyinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\src\generic\calctrl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\generic\choicdgg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\generic\dragimgg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\generic\grid.cpp
# End Source File
# Begin Source File

SOURCE=.\src\generic\gridsel.cpp
# End Source File
# Begin Source File

SOURCE=.\src\generic\laywin.cpp
# End Source File
# Begin Source File

SOURCE=.\src\generic\logg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\generic\numdlgg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\generic\panelg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\generic\plot.cpp
# End Source File
# Begin Source File

SOURCE=.\src\generic\progdlgg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\generic\prop.cpp
# End Source File
# Begin Source File

SOURCE=.\src\generic\propform.cpp
# End Source File
# Begin Source File

SOURCE=.\src\generic\proplist.cpp
# End Source File
# Begin Source File

SOURCE=.\src\generic\sashwin.cpp
# End Source File
# Begin Source File

SOURCE=.\src\generic\scrolwin.cpp
# End Source File
# Begin Source File

SOURCE=.\src\generic\splitter.cpp
# End Source File
# Begin Source File

SOURCE=.\src\generic\statusbr.cpp
# End Source File
# Begin Source File

SOURCE=.\src\generic\tbarsmpl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\generic\textdlgg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\generic\tipdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\generic\treelay.cpp
# End Source File
# Begin Source File

SOURCE=.\src\generic\wizard.cpp
# End Source File

# Begin Source File

SOURCE=.\src\common\appcmn.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\choiccmn.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\clipcmn.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\cmdline.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\cmndata.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\config.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\ctrlcmn.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\ctrlsub.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\datetime.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\datstrm.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\db.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\dbtable.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\dcbase.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\dlgcmn.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\dndcmn.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\dobjcmn.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\docmdi.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\docview.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\dynarray.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\dynlib.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\encconv.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\event.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\ffile.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\file.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\fileconf.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\filefn.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\filesys.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\fontcmn.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\fontmap.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\framecmn.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\fs_inet.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\fs_mem.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\fs_zip.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\ftp.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\gdicmn.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\geometry.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\gifdecod.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\hash.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\helpbase.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\http.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\imagall.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\imagbmp.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\image.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\imaggif.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\imagjpeg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\imagpcx.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\imagpng.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\imagpnm.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\imagtiff.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\intl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\ipcbase.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\layout.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\lboxcmn.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\list.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\log.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\longlong.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\memory.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\menucmn.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\mimecmn.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\module.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\mstream.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\object.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\objstrm.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\odbc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\paper.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\prntbase.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\process.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\protocol.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\resource.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\sckaddr.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\sckfile.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\sckipc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\sckstrm.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\serbase.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\sizer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\socket.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\strconv.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\stream.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\string.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\tbarbase.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\textcmn.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\textfile.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\timercmn.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\tokenzr.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\txtstrm.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\url.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\utilscmn.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\valgen.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\validate.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\valtext.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\variant.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\wfstream.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\wincmn.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\wxchar.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\wxexpr.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\zipstrm.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\zstream.cpp
# End Source File

# Begin Source File

SOURCE=.\src\common\extended.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\src\common\unzip.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File

# Begin Source File

SOURCE=.\src\msw\gsocket.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\src\msw\gsockmsw.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File

# Begin Source File

SOURCE=.\src\html\helpctrl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\html\helpdata.cpp
# End Source File
# Begin Source File

SOURCE=.\src\html\helpfrm.cpp
# End Source File
# Begin Source File

SOURCE=.\src\html\htmlcell.cpp
# End Source File
# Begin Source File

SOURCE=.\src\html\htmlfilt.cpp
# End Source File
# Begin Source File

SOURCE=.\src\html\htmlpars.cpp
# End Source File
# Begin Source File

SOURCE=.\src\html\htmltag.cpp
# End Source File
# Begin Source File

SOURCE=.\src\html\htmlwin.cpp
# End Source File
# Begin Source File

SOURCE=.\src\html\htmprint.cpp
# End Source File
# Begin Source File

SOURCE=.\src\html\m_dflist.cpp
# End Source File
# Begin Source File

SOURCE=.\src\html\m_fonts.cpp
# End Source File
# Begin Source File

SOURCE=.\src\html\m_hline.cpp
# End Source File
# Begin Source File

SOURCE=.\src\html\m_image.cpp
# End Source File
# Begin Source File

SOURCE=.\src\html\m_layout.cpp
# End Source File
# Begin Source File

SOURCE=.\src\html\m_links.cpp
# End Source File
# Begin Source File

SOURCE=.\src\html\m_list.cpp
# End Source File
# Begin Source File

SOURCE=.\src\html\m_meta.cpp
# End Source File
# Begin Source File

SOURCE=.\src\html\m_pre.cpp
# End Source File
# Begin Source File

SOURCE=.\src\html\m_tables.cpp
# End Source File
# Begin Source File

SOURCE=.\src\html\winpars.cpp
# End Source File


# Begin Source File

SOURCE=.\src\common\y_tab.c

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Target
# End Project
