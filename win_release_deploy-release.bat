SET QtDir=C:\Qt\5.15.2\mingw81_64\bin\
SET buildDir=.\build-XTPlayer-Desktop_Qt_5_15_2_MinGW_64_bit-Release\
SET deployDir=.\bin\release\

%QtDir%windeployqt %buildDir%release\XTPlayer.exe -dir %deployDir%
xcopy %buildDir%release\XTPlayer.exe %deployDir%
xcopy %buildDir%release\themes %deployDir%themes /s /i
xcopy %QtDir%QtAV1.dll %deployDir%
xcopy %QtDir%QtAVWidgets1.dll %deployDir%
xcopy %QtDir%avcodec-56.dll %deployDir%
xcopy %QtDir%avdevice-55.dll %deployDir%
xcopy %QtDir%avfilter-5.dll %deployDir%
xcopy %QtDir%avformat-56.dll %deployDir%
xcopy %QtDir%avresample-2.dll %deployDir%
xcopy %QtDir%avutil-54.dll %deployDir%
xcopy %QtDir%swscale-3.dll %deployDir%
xcopy %QtDir%zlib1.dll %deployDir%
xcopy %QtDir%libvo-aacenc-0.dll %deployDir%
xcopy %QtDir%libmp3lame-0.dll %deployDir%
xcopy %QtDir%libvorbis-0.dll %deployDir%
xcopy %QtDir%libopus-0.dll %deployDir%
xcopy %QtDir%libx264-142.dll %deployDir%
xcopy %QtDir%libvorbisenc-2.dll %deployDir%
xcopy %QtDir%libogg-0.dll %deployDir%
xcopy %QtDir%Qt5OpenGL.dll %deployDir%
xcopy %QtDir%Qt5Compress.dll %deployDir%
xcopy .\src\rename_me_for_local_settings.ini %deployDir%
xcopy "XTP and DeoVR guide.pdf" %deployDir%
xcopy .\src\XTPlayerDebug.bat %deployDir%
xcopy .\src\README.txt %deployDir%

pause