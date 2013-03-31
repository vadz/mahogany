;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Project:     M
;; File name:   M.iss - Mahogany installation script for Inno Setup
;; Purpose:     Inno Setup Script for M
;; Author:      Vadim Zeitlin
;; Modified by: 20.06.99 for InnoSetup 1.1
;;              09.03.01 for InnoSetup 2.0
;;              02.03.03 for InnoSetup 3.0
;; Created:     23.08.98
;; CVS-ID:      $Id$
;; Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
;; Licence:     M license
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[Setup]
; --- app info
AppName=Mahogany
AppVerName=Mahogany 0.68 "Cynthia"

; --- setup compiler params
OutputBaseFilename=Mahogany-0.68.0
DefaultDirName={pf}\Mahogany
DefaultGroupName=Mahogany
AllowRootDirectory=1
AllowNoIcons=1

SourceDir=..\..
OutputDir=extra\setup

; TODO: use AppMutex to check whether the program is running

; --- app publisher info (for W2K only)
AppPublisher=Mahogany Dev-Team
AppPublisherURL=http://mahogany.sourceforge.net/
AppVersion=0.68.0

; --- appearance parameters

; hmm... what's RGB value of mahogany?
BackColor=$037ebd
AppCopyright=Copyright © 1997-2013 Vadim Zeitlin and Karsten Ballüder
WizardImageFile=res\wizard.bmp
WizardSmallImageFile=res\install_small.bmp

; --- doc files
LicenseFile=doc\license.txt
InfoBeforeFile=extra\setup\preread.txt
InfoAfterFile=extra\setup\postread.txt

[Components]
Name: "main"; Description: "Required Program Files"; Types: full compact custom; Flags: fixed
Name: "help"; Description: "Help Files"; Types: full
Name: "python"; Description: "Python Scripting Support"; Types: full
Name: "misc"; Description: "Miscellaneous Helper Files"; Types: full
Name: "i18n"; Description: "Translations to other languages"; Types: full

[Files]

; --- EXEs and DLLs
Source: "Release\M.exe"; DestDir: "{app}";
Source: "src\wx\vcard\Release\versit.dll"; DestDir: "{app}"

Source: "{#env SystemRoot}\system32\msvcp100.dll"; DestDir: "{app}"
Source: "{#env SystemRoot}\system32\msvcr100.dll"; DestDir: "{app}"

Source: "{#env wxwin}\lib\vc100_dll\wxbase294u_vc100.dll"; DestDir: "{app}"
Source: "{#env wxwin}\lib\vc100_dll\wxbase294u_net_vc100.dll"; DestDir: "{app}"
Source: "{#env wxwin}\lib\vc100_dll\wxbase294u_xml_vc100.dll"; DestDir: "{app}"
Source: "{#env wxwin}\lib\vc100_dll\wxmsw294u_adv_vc100.dll"; DestDir: "{app}"
Source: "{#env wxwin}\lib\vc100_dll\wxmsw294u_core_vc100.dll"; DestDir: "{app}"
Source: "{#env wxwin}\lib\vc100_dll\wxmsw294u_html_vc100.dll"; DestDir: "{app}"
Source: "{#env wxwin}\lib\vc100_dll\wxmsw294u_qa_vc100.dll"; DestDir: "{app}"

; --- misc stuff
Source: "extra\setup\autocollect.adb"; DestDir: "{userappdata}\Mahogany"; Flags: onlyifdoesntexist
Source: "extra\setup\Mahogany.url"; DestDir: "{app}"; Components: misc
Source: "extra\setup\Bug.url"; DestDir: "{app}"; Components: misc

; --- docs
Source: "doc\Tips\tips.txt"; DestDir: "{app}\doc"
Source: "doc\bugs.txt"; DestDir: "{app}\doc"; Components: misc
Source: "doc\License*.txt"; DestDir: "{app}\doc"; Components: misc
Source: "doc\Mannounce.txt"; DestDir: "{app}\doc"; DestName: "announce.txt"; Components: misc
Source: "doc\readme_win.txt"; DestDir: "{app}\doc"; DestName: "readme.txt"; Flags: "isreadme"

; --- help
Source: "doc\HtmlHlp\Manual.chm"; DestDir: "{app}\help"; Components: help

; icons (not included in the resources)
Source: "res\Msplash.png"; DestDir: "{app}\icons"

; --- python support

Source: "src\Python\HeaderInfo.py-swig"; DestName: "HeaderInfo.py"; DestDir: "{app}\Python"; Components: python
Source: "src\Python\MailFolder.py-swig"; DestName: "MailFolder.py"; DestDir: "{app}\Python"; Components: python
Source: "src\Python\MDialogs.py-swig"; DestName: "MDialogs.py"; DestDir: "{app}\Python"; Components: python
Source: "src\Python\Message.py-swig"; DestName: "Message.py"; DestDir: "{app}\Python"; Components: python
Source: "src\Python\MimePart.py-swig"; DestName: "MimePart.py"; DestDir: "{app}\Python"; Components: python
Source: "src\Python\SendMessage.py-swig"; DestName: "SendMessage.py"; DestDir: "{app}\Python"; Components: python

; case is important (should be Minit) or Python complains!!
Source: "src\Python\Scripts\MInit.py"; DestDir: "{app}\Python"; DestName: "Minit.py"; Components: python

; this is just an example but install it nevertheless, can be useful
Source: "src\Python\Scripts\spam.py"; DestDir: "{app}\Python"; Components: python

; --- translations
Source: "locale\de.mo"; DestDir: "{app}\locale\de"; DestName: "M.mo"; Components: i18n
Source: "locale\es.mo"; DestDir: "{app}\locale\es"; DestName: "M.mo"; Components: i18n
Source: "locale\fi.mo"; DestDir: "{app}\locale\fi"; DestName: "M.mo"; Components: i18n
Source: "locale\fr.mo"; DestDir: "{app}\locale\fr"; DestName: "M.mo"; Components: i18n
Source: "locale\it.mo"; DestDir: "{app}\locale\it"; DestName: "M.mo"; Components: i18n
Source: "locale\nl.mo"; DestDir: "{app}\locale\nl"; DestName: "M.mo"; Components: i18n
Source: "locale\pt.mo"; DestDir: "{app}\locale\pt"; DestName: "M.mo"; Components: i18n
Source: "locale\pt_br.mo"; DestDir: "{app}\locale\pt_BR"; DestName: "M.mo"; Components: i18n

[UninstallDelete]
; delete all precompiled python files
Type: files; Name: "{app}\Python\*.pyc"; Components: python

[Tasks]
Name: "run"; Description: "Run the program"

[Run]
Filename: {app}\M.exe; Tasks: run; Flags: nowait

[Icons]
Name: "{group}\Mahogany"; Filename: "{app}\M.EXE"; WorkingDir: "{app}"
Name: "{group}\Mahogany Help"; Filename: "{app}\help\Manual.chm"; Components: help
Name: "{group}\Visit Mahogany Home Page"; Filename: "{app}\Mahogany.url"; Components: misc
Name: "{group}\Report a Bug"; Filename: "{app}\Bug.url"; Components: misc

[Registry]
Root: HKCU; Subkey: "Software\Mahogany-Team"; ValueType: none; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Mahogany-Team\M"; ValueType: none; Flags: uninsdeletekeyifempty
Root: HKCU; Subkey: "Software\Mahogany-Team\M\Profiles"; ValueType: string; ValueName: "GlobalDir"; ValueData: "{app}"
Root: HKCU; Subkey: "Software\Mahogany-Team\M\Profiles"; ValueType: dword; ValueName: "UsePython"; ValueData: 1; Components: python

