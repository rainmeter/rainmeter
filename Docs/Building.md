## Building Rainmeter

### Get the source code

Use <a href="http://git-scm.com">Git</a> to clone the repository:

    git clone https://github.com/rainmeter/rainmeter.git

Alternatively, download the repository contents as a [ZIP archive](https://github.com/rainmeter/rainmeter/archive/master.zip).


### Building with Visual Studio

Rainmeter can be built using any version of Visual Studio 2013. If you don't already have VS2013, you can download the free "Visual Studio Express 2013 for Windows Desktop" version [here](http://www.visualstudio.com/downloads/download-visual-studio-vs).

After Visual Studio has been installed and updated, open Rainmeter.sln at the root of the repository to build.


### Building the installer

First, download and install <a href="https://code.google.com/p/unsis/downloads/list">NSIS Unicode</a> (download the Unicode version, not the ANSI version).

Now you can simply run the <b>Build.bat</b> batch file in the Build folder of your local repository. If you see any "not found" errors, check that the paths in the `set` commands at the top of the file match your environment. To build the release (non-beta) installer, use `Build.bat RELEASE`.

To digitally sign the installer and the Rainmeter executables, obtain a Windows code signing certificate and create a Certificate.bat file alongside Build.bat with the following contents:

    set CERTFILE=/path/to/PFXcert.p12
    set CERTKEY=certpassword


### Building old versions

The Rainmeter GitHub repository does not contain the full source code history required to build the installer for versions prior to r1249. To obtain the full source from r1 to r1248, use the old Google Code SVN repository located at:

    http://rainmeter.googlecode.com/svn

The build instructions above are applicable to r1130 - r1248. Build instructions for r27 - r1129 can be found in svn/wiki/.

The language files for r963 - r1214 were in a separate repository (svn:externals), which is not available any longer. As a result, the language .dll's for those revisions cannot be built.