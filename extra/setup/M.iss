;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Project:     M
;; File name:   M.iss - Mahogany installation script for Inno Setup
;; Purpose:     Inno Setup Script for M
;; Author:      Vadim Zeitlin
;; Modified by: VZ at 20.06.99 for InnoSetup 1.1
;; Created:     23.08.98
;; CVS-ID:      $Id$
;; Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
;; Licence:     M license
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[Setup]
Bits=32
AppName=Mahogany
AppVerName=Mahogany pre-alpha 0.23 'Polwarth III'
AppCopyright=Copyright © 1998 Karsten Ballüder and Vadim Zeitlin
DefaultDirName={pf}\Mahogany
DefaultGroupName=Mahogany
AllowNoIcons=1

; hmm... what's RGB value of mahogany?
BackColor=$037ebd

SourceDir=f:\Progs\M
LicenseFile=doc\license.txt

InfoBeforeFile=doc\preread.txt
InfoAfterFile=doc\postread.txt

[Dirs]
Name: "{app}\doc"
Name: "{app}\locale"

;Name: "{app}\lib"

[Files]
; --- program files
Source: "Release\M.EXE"; DestDir: "{app}"

; --- help
Source: "doc\Hlp\Mahogany.hlp"; DestDir: "{app}\doc"
Source: "doc\Hlp\Mahogany.cnt"; DestDir: "{app}\doc"

; --- python support
; "python15.dll";   "{app}\python15.dll"; copy_normal; 

; Python modules: the corresponding pyc files should be specified
; in the next section for deletion

; M python support
;"Minit.py";    "{app}\lib\Minit.py"; copy_normal; 
;"MailFolder.py";  "{app}\lib\MailFolder.py"; copy_normal; 
;"MAppBase.py";    "{app}\lib\MAppBase.py"; copy_normal; 
;"Message.py";    "{app}\lib\Message.py"; copy_normal; 
;"MString.py";    "{app}\lib\MString.py"; copy_normal; 
;"MProfile.py";    "{app}\lib\MProfile.py"; copy_normal; 

; standard Python modules used
;"exceptions.py";  "{app}\lib\exceptions.py"; copy_normal; 
;"ntpath.py";    "{app}\lib\ntpath.py"; copy_normal; 
;"os.py";    "{app}\lib\os.py"; copy_normal; 
;"stat.py";    "{app}\lib\stat.py"; copy_normal; 
;"string.py";    "{app}\lib\string.py"; copy_normal; 
;"UserDict.py";    "{app}\lib\UserDict.py"; copy_normal; 

; --- documentation
Source: "doc\readme_win.txt"; DestDir: "{app}"; Flags: "isreadme"
Source: "doc\bugs.txt"; DestDir: "{app}\doc"
Source: "doc\copying"; DestDir: "{app}\doc"

;Source: "doc\readme.txt"; DestDir: "{app}\doc"
;Source: "information.txt"; DestDir: "{app}\doc"
;Source: "license.html"; DestDir: "{app}\doc"

[DeleteFiles]
; delete all precompiled python files
{app}\lib\Minit.pyc
{app}\lib\MailFolder.pyc
{app}\lib\MAppBase.pyc
{app}\lib\Message.pyc
{app}\lib\MString.pyc
{app}\lib\MProfile.pyc
{app}\lib\exceptions.pyc
{app}\lib\ntpath.pyc
{app}\lib\os.pyc
{app}\lib\stat.pyc
{app}\lib\string.pyc
{app}\lib\UserDict.pyc

[Icons]
Name: "{group}\Mahogany"; Filename: "{app}\M.EXE"; WorkingDir: "{app}"
Name: "{group}\Mahogany Help"; Filename: "{app}\doc\Mahogany.hlp"

[Registry]
Root: HKCU; Subkey: "Software\Mahogany-Team"; ValueType: none; Flags: uninsdeletekey
