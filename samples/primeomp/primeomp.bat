"../../bin/cgen"  primeomp.cpp primeomp.cgen.cpp
if %ERRORLEVEL% NEQ 0 exit /B
copy primeomp.cpp primeomp.bak
"../../bin/skel" -i primeomp.cpp -s primeomp.cgen.cpp
if %ERRORLEVEL% NEQ 0 echo skel.exe failed
del primeomp.cgen.cpp