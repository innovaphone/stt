@echo off

set PROJECT=stt

set BUILD=1001
set RELEASE_STATE=sr1
set MANUF=innovaphone

if not [%1]==[] (set BUILD=%1)
if not [%2]==[] (set RELEASE_STATE=%2)
if not [%3]==[] (set MANUF=%3)

if not exist "release" mkdir "release"
if not exist "release\%BUILD%" mkdir "release\%BUILD%"

echo|set /p=%MANUF%-%PROJECT%,%MANUF%-%PROJECT%.debug,%MANUF%-%PROJECT%.png> "release\%BUILD%\%MANUF%-%PROJECT%_files"

set PATH=%PATH%;%INNOVAPHONE-SDK%\app-platform-buildtls;%INNOVAPHONE-SDK%\arm-7.2.0-linux-gnu\bin;%INNOVAPHONE-SDK%\x86_64-7.2.0-linux-gnu\bin;%INNOVAPHONE-SDK%\aarch64-7.2.0-linux-gnu\bin;

set INCLUDE=sdk;common\lap;%INNOVAPHONE-SDK%\arm-7.2.0-linux-gnu\arm-linux-gnueabi\include\c++\7.2.0;%INNOVAPHONE-SDK%\app-platform-libs\8\armel\usr\include;%INNOVAPHONE-SDK%\app-platform-libs\8\armel\usr\include\postgresql;%INNOVAPHONE-SDK%\arm-7.2.0-linux-gnu\lib\gcc\arm-linux-gnueabi\7.2.0\include;%INNOVAPHONE-SDK%\arm-7.2.0-linux-gnu\arm-linux-gnueabi\include\c++\7.2.0\arm-linux-gnueabi
make -j5 -f %PROJECT%.mak arm

set INCLUDE=sdk;.;common\lap;%INNOVAPHONE-SDK%\x86_64-7.2.0-linux-gnu\x86_64-linux-gnu\include\c++\7.2.0;%INNOVAPHONE-SDK%\app-platform-libs\8\x86_64\usr\include;%INNOVAPHONE-SDK%\app-platform-libs\8\x86_64\usr\include\postgresql;%INNOVAPHONE-SDK%\x86_64-7.2.0-linux-gnu\lib\gcc\x86_64-linux-gnu\7.2.0\include;%INNOVAPHONE-SDK%\x86_64-7.2.0-linux-gnu\x86_64-linux-gnu\include\c++\7.2.0\x86_64-linux-gnu
make -j5 -f %PROJECT%.mak x86_64

set INCLUDE=sdk;.;common\lap;%INNOVAPHONE-SDK%\aarch64-7.2.0-linux-gnu\aarch64-linux-gnu\include\c++\7.2.0;%INNOVAPHONE-SDK%\app-platform-libs\10\arm64\usr\include;%INNOVAPHONE-SDK%\app-platform-libs\10\arm64\usr\include\postgresql;%INNOVAPHONE-SDK%\aarch64-7.2.0-linux-gnu\lib\gcc\aarch64-linux-gnu\7.2.0\include;%INNOVAPHONE-SDK%\aarch64-7.2.0-linux-gnu\aarch64-linux-gnu\include\c++\7.2.0\aarch64-linux-gnu
make -j5 -f %PROJECT%.mak arm64

if not exist "release\%BUILD%\arm" mkdir "release\%BUILD%\arm"
if not exist "release\%BUILD%\arm\%MANUF%-%PROJECT%" mkdir "release\%BUILD%\arm\%MANUF%-%PROJECT%"
copy "arm\%PROJECT%\%PROJECT%" "release\%BUILD%\arm\%MANUF%-%PROJECT%\%MANUF%-%PROJECT%"
copy "arm\%PROJECT%\%PROJECT%.debug" "release\%BUILD%\arm\%MANUF%-%PROJECT%\%MANUF%-%PROJECT%.debug"
copy "arm\%PROJECT%\%PROJECT%.png" "release\%BUILD%\arm\%MANUF%-%PROJECT%\%MANUF%-%PROJECT%.png"

if not exist "release\%BUILD%\arm64" mkdir "release\%BUILD%\arm64"
if not exist "release\%BUILD%\arm64\%MANUF%-%PROJECT%" mkdir "release\%BUILD%\arm64\%MANUF%-%PROJECT%"
copy "arm64\%PROJECT%\%PROJECT%" "release\%BUILD%\arm64\%MANUF%-%PROJECT%\%MANUF%-%PROJECT%"
copy "arm64\%PROJECT%\%PROJECT%.debug" "release\%BUILD%\arm64\%MANUF%-%PROJECT%\%MANUF%-%PROJECT%.debug"
copy "arm64\%PROJECT%\%PROJECT%.png" "release\%BUILD%\arm64\%MANUF%-%PROJECT%\%MANUF%-%PROJECT%.png"

if not exist "release\%BUILD%\x86_64" mkdir "release\%BUILD%\x86_64"
if not exist "release\%BUILD%\x86_64\%MANUF%-%PROJECT%" mkdir "release\%BUILD%\x86_64\%MANUF%-%PROJECT%"
copy "x86_64\%PROJECT%\%PROJECT%" "release\%BUILD%\x86_64\%MANUF%-%PROJECT%\%MANUF%-%PROJECT%"
copy "x86_64\%PROJECT%\%PROJECT%.debug" "release\%BUILD%\x86_64\%MANUF%-%PROJECT%\%MANUF%-%PROJECT%.debug"
copy "x86_64\%PROJECT%\%PROJECT%.png" "release\%BUILD%\x86_64\%MANUF%-%PROJECT%\%MANUF%-%PROJECT%.png"
