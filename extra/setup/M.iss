;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Project:     M
;; File name:   M.iss - Mahogany installation script for Inno Setup
;; Purpose:     Inno Setup Script for M
;; Author:      Vadim Zeitlin
;; Modified by: VZ at 20.06.99 for InnoSetup 1.1
;;              VZ at 09.03.01 for InnoSetup 2.0
;; Created:     23.08.98
;; CVS-ID:      $Id$
;; Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
;; Licence:     M license
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[Setup]
; --- app info
AppName=Mahogany
AppVerName=Mahogany alpha 0.62 'Mars'

; --- setup compiler params
OutputBaseFilename=Mahogany-0.62a
DefaultDirName={pf}\Mahogany
DefaultGroupName=Mahogany
AllowRootDirectory=1
AllowNoIcons=1

SourceDir=f:\Progs\M
OutputDir=extra\setup\output

; TODO: use AppMutex to check whether the program is running

; --- app publisher info (for W2K only)
AppPublisher=Mahogany Dev-Team
AppPublisherURL=http://mahogany.sourceforge.net/
AppVersion=0.62

; --- appearance parameters

; hmm... what's RGB value of mahogany?
BackColor=$037ebd
AppCopyright=Copyright © 1997-2001 Karsten Ballüder and Vadim Zeitlin
WizardImageFile=res\install1.bmp
WizardSmallImageFile=res\install_small.bmp

; --- doc files
LicenseFile=doc\license.txt
InfoBeforeFile=doc\preread.txt
InfoAfterFile=doc\postread.txt

[Components]
Name: "main"; Description: "Required Program Files"; Types: full compact custom; Flags: fixed
Name: "help"; Description: "Help Files"; Types: full compact
Name: "python"; Description: "Python Support (Requires Python 2.0)"; Types: full
Name: "misc"; Description: "Miscellaneous Helper Files"; Types: full

[Files]
; --- EXEs and DLLs
Source: "Release\M.EXE"; DestDir: "{app}"
Source: "src\wx\vcard\Release\versit.dll"; DestDir: "{app}"
Source: "doc\Mahogany.url"; DestDir: "{app}"; Components: misc
Source: "doc\Bug.url"; DestDir: "{app}"; Components: misc

; --- docs
Source: "doc\Tips\tips.txt"; DestDir: "{app}\doc"
Source: "doc\bugs.txt"; DestDir: "{app}\doc"; Components: misc
Source: "doc\License*.txt"; DestDir: "{app}\doc"; Components: misc
Source: "doc\Mannounce.txt"; DestDir: "{app}\doc"; DestName: "announce.txt"; Components: misc
Source: "doc\readme_win.txt"; DestDir: "{app}\doc"; DestName: "readme.txt"; Flags: "isreadme"

; --- help
Source: "doc\HtmlHlp\Manual.*"; DestDir: "{app}\help"; Components: help

; icons
Source: "src\icons\tb*.xpm"; DestDir: "{app}\icons"

; --- python support

; we always need Python DLL as the app wouldn't start without it :-(
Source: "w:\winnt40\system32\Python20.dll"; DestDir: "{sys}"

Source: "src\Python\MailFolder.py"; DestDir: "{app}\Python"; Components: python
Source: "src\Python\MAppBase.py"; DestDir: "{app}\Python"; Components: python
Source: "src\Python\Message.py"; DestDir: "{app}\Python"; Components: python
Source: "src\Python\MObject.py"; DestDir: "{app}\Python"; Components: python
Source: "src\Python\MProfile.py"; DestDir: "{app}\Python"; Components: python
Source: "src\Python\MString.py"; DestDir: "{app}\Python"; Components: python
Source: "src\Python\MTest.py"; DestDir: "{app}\Python"; Components: python

; case is important (should be Minit) or Python complains!!
Source: "src\Python\Scripts\MInit.py"; DestDir: "{app}\Python"; DestName: "Minit.py"; Components: python

[UninstallDelete]
; delete all precompiled python files
Type: files; Name: "{app}\Python\*.pyc"; Components: python

[Icons]
Name: "{group}\Mahogany"; Filename: "{app}\M.EXE"; WorkingDir: "{app}"
Name: "{group}\Mahogany Help"; Filename: "{app}\help\Manual.chm"; Components: help
Name: "{group}\Visit Mahogany Home Page"; Filename: "{app}\Mahogany.url"; Components: misc
Name: "{group}\Report a Bug"; Filename: "{app}\Bug.url"; Components: misc

[Registry]
Root: HKCU; Subkey: "Software\Mahogany-Team"; ValueType: none; Flags: uninsdeletekeyifempty
Root: HKCU; Subkey: "Software\Mahogany-Team\M"; ValueType: none; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Mahogany-Team\M\Profiles"; ValueType: string; ValueName: "GlobalDir"; ValueData: "{app}"
Root: HKCU; Subkey: "Software\Mahogany-Team\M\Profiles"; ValueType: dword; ValueName: "UsePython"; ValueData: 1; Components: python
