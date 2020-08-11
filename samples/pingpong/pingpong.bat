"../../bin/cgen"  pingpong.cpp pingpong.cgen.cpp
if %ERRORLEVEL% NEQ 0 exit /B
copy pingpong.cpp pingpong.bak
"../../bin/skel" -i pingpong.cpp -s pingpong.cgen.cpp
if %ERRORLEVEL% NEQ 0 echo skel.exe failed
del pingpong.cgen.cpp