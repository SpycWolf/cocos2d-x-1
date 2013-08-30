@echo off

echo./*
echo. * Check VC++ environment...
echo. */
echo.

if defined VS110COMNTOOLS (
    set VSTOOLS="%VS110COMNTOOLS%"
    set VC_VER=110
) 


set VSTOOLS=%VSTOOLS:"=%
set "VSTOOLS=%VSTOOLS:\=/%"

set VSVARS="%VSTOOLS%vsvars32.bat"

if not defined VSVARS (
    echo Can't find VC2012 installed!
    goto ERROR
)

echo./*
echo. * Building cocos2d-x library binary, please wait a while...
echo. */
echo.

call %VSVARS%
if %VC_VER%==110 (
    msbuild cocos2d-winrt.vc2012.sln /t:Clean
    msbuild cocos2d-winrt.vc2012.sln /p:Configuration="Debug" /p:Platform="ARM" /m
    msbuild cocos2d-winrt.vc2012.sln /p:Configuration="Release" /p:Platform="ARM" /m
    msbuild cocos2d-winrt.vc2012.sln /p:Configuration="Debug" /p:Platform="Win32" /m
    msbuild cocos2d-winrt.vc2012.sln /p:Configuration="Release" /p:Platform="Win32" /m
) else (
    echo Script error.
    goto ERROR
)

echo./*
echo. * Check the cocos2d-winrt application "TestCpp.exe" ...
echo. */
echo.

pushd ".\Win32\Release\TestCpp"

set CC_TEST_BIN=TestCpp.exe




echo./*
echo. * Run cocos2d-winrt tests.exe and view Cocos2d-x Application Wizard for Visual Studio User Guide.
echo. */
echo.
.

if not exist "%CC_TEST_BIN%" (
    echo Can't find the binary "TestCpp.exe", is there build error?
    goto ERROR
)

call "%CC_TEST_BIN%"
popd
start http://www.cocos2d-x.org/projects/cocos2d-x/wiki/Cocos2d-x_Application_Wizard_for_Visual_Studio_User_Guide
goto EOF

:ERROR
pause

:EOF
