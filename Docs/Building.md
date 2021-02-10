## Building Rainmeter

### Get the source code

Use <a href="http://git-scm.com">Git</a> to clone the repository:

    git clone https://github.com/rainmeter/rainmeter.git

Alternatively, download the repository contents as a [ZIP archive](https://github.com/rainmeter/rainmeter/archive/master.zip).


### Building with Visual Studio

Rainmeter can be built using any version of Visual Studio 2019. If you don't already have VS2019, you can download [Visual Studio Community 2019](https://www.visualstudio.com/downloads/) for free.


### Building the installer manually

First, download and install [NSIS 3](http://nsis.sourceforge.net) or later.

Then, in the Build directory, run e.g. `Build.bat beta 1 2 3 4` to build the beta 1.2.3 r4.

If you see any "not found" errors, check that the paths in the `set` commands at the top of the file match your environment.

To build the final (non-beta) installer, use `Build.bat final`.

To digitally sign the installer and the Rainmeter executables, obtain a Windows code signing certificate and create a Certificate.bat file alongside Build.bat with the following contents:

    set CERTFILE=/path/to/PFXcert.p12
    set CERTKEY=certpassword
