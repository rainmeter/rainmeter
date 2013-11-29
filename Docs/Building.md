## Building Rainmeter

Rainmeter can be built using any (including free) version of Visual Stuido 2013. The free "Visual Studio Express 2013 for Windows Desktop" version can be downloaded [here](http://www.visualstudio.com/downloads/download-visual-studio-vs).

After Visual Studio has been installed and updated, open Rainmeter.sln to build.


### Building the installer

To build the full Rainmeter installer, run Build.bat. If you receive "not found" errors, open Build.bat and change the `set=` lines at the top to match your system.

To sign the installer and the Rainmeter executables, create a Certificate.bat file alongside Build.bat with the following contents:

    set CERTFILE=/path/to/PFXcert.p12
    set CERTKEY=certpassword

Use `Build.bat RELEASE` to build a non-beta installer.


### Building old versions

The Rainmeter GitHub repository does not contain the full source code history required to build the installer for versions prior to r1249. To obtain the full source from r1 to r1248, use the old Google Code SVN repository located at:

    http://rainmeter.googlecode.com/svn

The build instructions above are applicable to r1130 - r1248. Build instructions for r27 - r1129 can be found in svn/wiki/.

The language files for r963 - r1214 were in a separate repository (svn:externals), which is not available any longer. As a result, the language .dll's for those revisions cannot be built.