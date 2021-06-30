"../../bin/cgen"  phase-0.cpp phase.cgen.cpp
if %ERRORLEVEL% NEQ 0 exit /B
copy phase-0.cpp phase-0.bak
"../../bin/skel" -i phase-0.cpp -s phase.cgen.cpp
if %ERRORLEVEL% NEQ 0 echo skel.exe failed
del phase.cgen.cpp

"../../bin/cgen"  phase-1.cpp phase.cgen.cpp
if %ERRORLEVEL% NEQ 0 exit /B
copy phase-1.cpp phase-1.bak
"../../bin/skel" -i phase-1.cpp -s phase.cgen.cpp
if %ERRORLEVEL% NEQ 0 echo skel.exe failed
del phase.cgen.cpp

"../../bin/cgen"  phase-2.cpp phase.cgen.cpp
if %ERRORLEVEL% NEQ 0 exit /B
copy phase-2.cpp phase-2.bak
"../../bin/skel" -i phase-2.cpp -s phase.cgen.cpp
if %ERRORLEVEL% NEQ 0 echo skel.exe failed
del phase.cgen.cpp

"../../bin/cgen"  phase-3.cpp phase.cgen.cpp
if %ERRORLEVEL% NEQ 0 exit /B
copy phase-3.cpp phase-3.bak
"../../bin/skel" -i phase-3.cpp -s phase.cgen.cpp
if %ERRORLEVEL% NEQ 0 echo skel.exe failed
del phase.cgen.cpp