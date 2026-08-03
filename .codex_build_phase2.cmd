@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 || exit /b 1
"C:\Program Files\CMake\bin\cmake.exe" --build D:\tc\third_party\express\out\build\x64-Debug --target express_unittest express_benchmark
