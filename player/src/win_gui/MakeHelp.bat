@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by WMP4PLAYER.HPJ. >"hlp\wmp4player.hm"
echo. >>"hlp\wmp4player.hm"
echo // Commands (ID_* and IDM_*) >>"hlp\wmp4player.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\wmp4player.hm"
echo. >>"hlp\wmp4player.hm"
echo // Prompts (IDP_*) >>"hlp\wmp4player.hm"
makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\wmp4player.hm"
echo. >>"hlp\wmp4player.hm"
echo // Resources (IDR_*) >>"hlp\wmp4player.hm"
makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\wmp4player.hm"
echo. >>"hlp\wmp4player.hm"
echo // Dialogs (IDD_*) >>"hlp\wmp4player.hm"
makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\wmp4player.hm"
echo. >>"hlp\wmp4player.hm"
echo // Frame Controls (IDW_*) >>"hlp\wmp4player.hm"
makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\wmp4player.hm"
REM -- Make help for Project WMP4PLAYER


echo Building Win32 Help files
start /wait hcw /C /E /M "hlp\wmp4player.hpj"
if errorlevel 1 goto :Error
if not exist "hlp\wmp4player.hlp" goto :Error
if not exist "hlp\wmp4player.cnt" goto :Error
echo.
if exist Debug\nul copy "hlp\wmp4player.hlp" Debug
if exist Debug\nul copy "hlp\wmp4player.cnt" Debug
if exist Release\nul copy "hlp\wmp4player.hlp" Release
if exist Release\nul copy "hlp\wmp4player.cnt" Release
echo.
goto :done

:Error
echo hlp\wmp4player.hpj(1) : error: Problem encountered creating help file

:done
echo.
