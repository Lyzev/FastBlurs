@ECHO off
adb shell am force-stop com.intel.cput.IntelFastBlurs
ECHO.
ECHO.
ECHO calling ndk-build------------------------------------------------------------------------
ECHO.
call ndk-build
ECHO -----------------------------------------------------------------------------------------
ECHO Call to "ndk-build" returns error level: %errorlevel%
if %errorlevel% neq 0 (
 pause
 exit /b %errorlevel%
)
ECHO -----------------------------------------------------------------------------------------

ECHO.
ECHO.
ECHO calling ant debug------------------------------------------------------------------------
ECHO.
call ant debug
ECHO -----------------------------------------------------------------------------------------
ECHO Call to "ant debug" returns error level: %errorlevel%
if %errorlevel% neq 0 (
 pause
 exit /b %errorlevel%
)
ECHO -----------------------------------------------------------------------------------------

ECHO.
ECHO.
ECHO calling adb install -r ./bin/IntelFastBlurs-debug.apk------------------------------------
ECHO.
call adb install -r ./bin/IntelFastBlurs-debug.apk
ECHO -----------------------------------------------------------------------------------------
ECHO Call to "adb install -r ./bin/IntelFastBlurs-debug.apk" returns error level: %errorlevel%
ECHO -----------------------------------------------------------------------------------------
ECHO .
ECHO -----------------------------------------------------------------------------------------
ECHO Running the app...
adb shell am start -n com.intel.cput.IntelFastBlurs/android.app.NativeActivity
pause
