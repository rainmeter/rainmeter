## Building Rainmeter

Rainmeter can be built using the free Visual Studio Express 2012 for Windows Desktop or any paid version of VS2012 (e.g. Professional). Note some projects (such as the NowPlaying plugin) cannot be built with the free Express version.

The free VS2012 Express version can be downloaded [here](http://microsoft.com/visualstudio/eng/products/visual-studio-express-for-windows-desktop).

VS2012 Update 2 (or later) is also required for Windows XP targeting support. It is available [here](http://microsoft.com/visualstudio/eng/downloads#d-visual-studio-2012-update).

After Visual Studio has been installed and updated, open Rainmeter.sln to build.


### Building the installer

To build the full Rainmeter distribution, run Build.bat. If you receive "not found" errors, open Build.bat and change the variables at the top to match your system.

To sign the installer and the Rainmeter executables, create a Certificate.bat file alongside Build.bat with the following contents:

    set CERTFILE=/path/to/PFXcert.p12
    set CERTKEY=certpassword


### Building old versions

The Rainmeter GitHub repository does not contain the full source code history required to build the installer for versions prior to r1249. To obtain the full source from r1 to r1248, use the old Google Code SVN repository located at:

    http://rainmeter.googlecode.com/svn

The build instructions above are applicable to r1130 - r1248. Build instructions for r27 - r1129 can be found in svn/wiki/.

The language files for r963 - r1214 were in a separate repository (svn:externals), which is not available any longer. As a result, the language .dll's for those revisions cannot be built.