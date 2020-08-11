"../../bin/cgen"  diamond.hpp diamond.cgen.hpp
if %ERRORLEVEL% NEQ 0 exit /B
copy diamond.hpp diamond.bak
"../../bin/skel" -i diamond.hpp -s diamond.cgen.hpp
if %ERRORLEVEL% NEQ 0 echo skel.exe failed
del diamond.cgen.hpp