## Building Rainmeter

### Get the source code

Use <a href="http://git-scm.com">Git</a> to clone the repository:

    git clone https://github.com/rainmeter/rainmeter.git

Alternatively, download the repository contents as a [ZIP archive](https://github.com/rainmeter/rainmeter/archive/master.zip).


### Building with Visual Studio

Install the required tools:

- [Visual Studio Community 2026](https://www.visualstudio.com/downloads/) (select "Desktop development with C++" in the installer)
- [.NET Framework 4.7.1 Developer Pack](https://dotnet.microsoft.com/en-us/download/dotnet-framework/thank-you/net471-developer-pack-offline-installer)
- [NSIS 3](http://nsis.sourceforge.net)

Then open `Rainmeter.sln` to build and run Rainmeter.


### Building the installer

From the repository root, run e.g. `.\Build\Build.ps1 full 1.2.3.4` to build all components with the version 1.2.3.4.

Run `Get-Help .\Build\Build.ps1 -Detailed` to see the available parameters.

If you see any "not found" errors, check that the tool paths near the top of the script match your environment.
