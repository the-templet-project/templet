"../../bin/cgen"  parsweep.hpp parsweep.cgen.hpp
if %ERRORLEVEL% NEQ 0 exit /B
copy parsweep.hpp parsweep.bak
"../../bin/skel" -i parsweep.hpp -s parsweep.cgen.hpp
if %ERRORLEVEL% NEQ 0 echo skel.exe failed
del parsweep.cgen.hpp