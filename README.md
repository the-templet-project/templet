![](LOGO.png)  
##  **TEMPLET** -- a framework for actor-task computing 
*******************************************************

[The Templet project](http://templet.ssau.ru) exists to take full advantage of computing resources in scientific, educational and industrial applications through the implementation of *actor-task computing*, a generic approach that adapts [actor model](https://en.wikipedia.org/wiki/Actor_model) for [many-task computing](https://en.wikipedia.org/wiki/Many-task_computing) across multiple cores, clusters, and the Internet.

The current implementation supports the [Everest platform](http://everest.distcomp.org) for managing the execution of parallel tasks on distributed computing resources over the Internet. It can also model logic and measure the performance of a complex many-task workflow on a single computer.

You can try Templet
* interactively with Binder [![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gh/the-templet-project/templet.git/master?urlpath=lab), 
* or use the provided [MS Visual Studio](https://visualstudio.microsoft.com) project files,
* or compile the source files in any C ++ programming environment.

### Repository structure
************************
The software package consists of three components.
1. Code generator driven by lightweight actor specification language.
2. The C ++ library that implements the actor engine and several task engines for integration with [Everest](http://everest.distcomp.org), behavior and performance modeling, etc.
3. Gallery of small examples of many-task applications.

The components are arranged as described below.
* **~/bin/** – code generator executables, curl binaries for Windows, quick info on MS VC++ redistributable packages
* **~/binder/** – Binder configuration files
* **~/lib/** – Templet C++ library source files
* **~/lib/opt/** – external packages including libcurl and JSON for Modern C++
* **~/lib/speclang.txt** – technical overview of the lightweight actor specification language
* **~/lib/templet.hpp** – core actor engine and base task engine
* **~/lib/everest.hpp** – Everest task engine
* **~/lib/basesim.hpp** – task engine for the paracomputer simulation;
* **~/cgen/** – code generator sources
* **~/cgen/acta** - integrated actor-task model processor
* **~/cgen/cgen** - code generator for actor-task model
* **~/cgen/skel** - skeleton processor
* **~/samples/** – gallery of small examples
* **~/samples/brickmov/** – model of transferring bricks from the original heap to the new one along the chain of workers
* **~/samples/diamond/** – demonstration of the paracomputer simulation engine
* **~/samples/evrstagnt/** - installing and verifying the Everest agent on the Binder virtual machine
* **~/samples/heatomp/** - solving the heat equation using multicore systems with OpenMP 2.0 or higher
* **~/samples/parsweep/** - parameter sweep application (calculation of the Lyapunov spectrum for the Lorentz equation with different values of rho, sigma, beta)
* **~/samples/pingpong/** – demonstration of the Everest task engine
* **~/samples/primempi/** - distributed prime search algorithm
* **~/samples/pythagor/** – demonstration of the lightweight actor specification language, core actor engine and base task engine APIs

### Templet developers
**********************
This project is a part of learning and research activities of [Samara University](http://www.ssau.ru/english), Institute of IT, Mathematics and Electronics. For more details feel free to contact the author: Dr. Sergei Vostokin (sergey.vostokin@gmail.com).

#### Thank you for your interest in the Templet project!




