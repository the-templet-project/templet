"../../bin/cgen"  primempi.cpp primempi.cgen.cpp
if %ERRORLEVEL% NEQ 0 exit /B
copy primempi.cpp primempi.bak
"../../bin/skel" -i primempi.cpp -s primempi.cgen.cpp
if %ERRORLEVEL% NEQ 0 echo skel.exe failed
del primempi.cgen.cpp