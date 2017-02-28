## Building Rainmeter

### Get the source code

Use <a href="http://git-scm.com">Git</a> to clone the repository:

    git clone https://github.com/rainmeter/rainmeter.git

Alternatively, download the repository contents as a [ZIP archive](https://github.com/rainmeter/rainmeter/archive/master.zip).


### Building with Visual Studio

Rainmeter can be built using any version of Visual Studio 2015. If you don't already have VS2015, you can download [Visual Studio Community 2015](https://www.visualstudio.com/en-us/downloads/download-visual-studio-vs.aspx) for free. When installing, be sure to select the following components:

* Programming Languages -> Visual C++ -> Common Tools for Visual C++ 2015
* Programming Languages -> Visual C++ -> Windows XP Support for C++

After Visual Studio has been installed and updated, open Rainmeter.sln at the root of the repository to build. You may have to install [.NET Framework 3.5](http://www.microsoft.com/en-us/download/details.aspx?id=21).


### Building the installer

First, download and install [NSIS 3](http://nsis.sourceforge.net) or later.

Now you can simply run the <b>Build.bat</b> batch file in the Build folder of your local repository. If you see any "not found" errors, check that the paths in the `set` commands at the top of the file match your environment. To build the release (non-beta) installer, use `Build.bat RELEASE`.

To digitally sign the installer and the Rainmeter executables, obtain a Windows code signing certificate and create a Certificate.bat file alongside Build.bat with the following contents:

    set CERTFILE=/path/to/PFXcert.p12
    set CERTKEY=certpassword

