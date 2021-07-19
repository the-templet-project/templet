"../../bin/cgen"  heatomp.cpp heatomp.cgen.cpp
if %ERRORLEVEL% NEQ 0 exit /B
copy heatomp.cpp heatomp.bak
"../../bin/skel" -i heatomp.cpp -s heatomp.cgen.cpp
if %ERRORLEVEL% NEQ 0 echo skel.exe failed
del heatomp.cgen.cpp