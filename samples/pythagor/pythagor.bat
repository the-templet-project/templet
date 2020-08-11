"../../bin/cgen"  pythagor.cpp pythagor.cgen.cpp
if %ERRORLEVEL% NEQ 0 exit /B
copy pythagor.cpp pythagor.bak
"../../bin/skel" -i pythagor.cpp -s pythagor.cgen.cpp
if %ERRORLEVEL% NEQ 0 echo skel.exe failed
del pythagor.cgen.cpp