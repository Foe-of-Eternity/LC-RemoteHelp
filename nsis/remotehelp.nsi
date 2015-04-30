!define S_NAME "RemoteHelp"
AutoCloseWindow True
SilentInstall silent
WindowIcon off
XPStyle on
Icon "../tvnserver/res/default.ico"
Name "${S_NAME}"
OutFile "${S_NAME}.exe"
RequestExecutionLevel user
SetCompressor /solid lzma

!include UAC.nsh
!include nsProcess.nsh
!include x64.nsh
!include WinVer.nsh
!include macros.nsh

Section
${IfNot} ${UAC_IsInnerInstance}
 MessageBox MB_ICONQUESTION|MB_YESNO "This will allow Lacon Computer to see your screen$\nand interact with your computer.$\n$\nAre you sure you want to do this?" IDNO skip_all
${EndIf} 
!insertmacro Init "application"
InitPluginsDir
SetOutPath "$PLUGINSDIR"
${!defineifexist} HAVE_DFMIRAGE_1_1 dfmirage_1.1.68.0
${!defineifexist} HAVE_DFMIRAGE_2_0 dfmirage_2.0.105.0
!ifdef HAVE_DFMIRAGE_1_1
 File /r "dfmirage_1.1.68.0"
!endif
!ifdef HAVE_DFMIRAGE_2_0
 File /r "dfmirage_2.0.105.0"
!endif

File "..\Release\rhserver.exe"
#File "..\Release\screenhooks32.dll"
File "rhserver.ini"

!ifdef HAVE_DFMIRAGE_1_1 | HAVE_DFMIRAGE_2_0
 File "devcon.exe"
!endif
!ifdef HAVE_DFMIRAGE_2_0
 File "devcon64.exe"
!endif

!ifdef HAVE_DFMIRAGE_1_1 | HAVE_DFMIRAGE_2_0
 var /global devcon
 ${If} ${RunningX64}
  StrCpy $devcon "$PLUGINSDIR\devcon64.exe"
 ${Else}
  StrCpy $devcon "$PLUGINSDIR\devcon.exe"
 ${EndIf}

 nsExec::ExecToStack '"$devcon" find dfmirage'
 Pop $0
 Pop $1
 ${StrContains} $0 "Mirage Driver" $1
 StrCmp $0 "Mirage Driver" skip_driver_install
 ${If} ${AtLeastWinVista}
  !ifdef HAVE_DFMIRAGE_2_0
   nsExec::ExecToStack '"$devcon" install dfmirage_2.0.105.0/dfmirage.inf dfmirage'
  !endif
 ${Else}
  ${If} ${RunningX64}
   !ifdef HAVE_DFMIRAGE_2_0
    nsExec::Exec '"$devcon" install dfmirage_2.0.105.0/dfmirage.inf dfmirage'
   !endif
  ${Else}
   !ifdef HAVE_DFMIRAGE_1_1
    nsExec::Exec '"$devcon" install dfmirage_1.1.68.0/dfmirage.inf dfmirage'
   !endif
  ${EndIf}
 ${EndIf}
 skip_driver_install:
!endif

ExecWait "rhserver.exe"

skip_all:
SectionEnd