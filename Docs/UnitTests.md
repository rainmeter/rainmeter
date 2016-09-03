Rainmeter uses the [Visual Studio Native Unit Testing framework](http://msdn.microsoft.com/en-us/library/hh598953.aspx) for unit testing.


### General notes

All unit test files should have the `<ExcludedFromBuild>$(ExcludeTests)</ExcludedFromBuild>` property set. The easiest way to ensure this is to manually edit the `ClCompile` entry of the project .vcxproj file. Make sure it looks something like this:

        <ClCompile Include="ConfigParser_Test.cpp">
          <ExcludedFromBuild>$(ExcludeTests)</ExcludedFromBuild>
        </ClCompile>

When building within Visual Studio, the macro `$(ExcludeTests)` evaluates to `false` (i.e. the test files are **not** excluded). When building using Build.bat, it evaluates to `true`, and, consequently, the test files are excluded from the build. This is done in order to both reduce the actual build size and remove any kind of dependency on *Microsoft.VisualStudio.TestTools.CppUnitTestFramework.dll* (see below).


### Project type specific notes

* Static library projects:

    Unit testing static library project requires the use of a separate DLL project. The static library project contains the actual code while the DLL project references the static library and includes the testing code.

    See `Common\Common.vcxproj` and `Common\Common_Test.vcxproj` for an example.

* Dynamic library projects:

    A unit tested dynamic library project should contain both the actual code and the testing code.

    In addition, the `$(DelayLoadTestDLL)` macro should be included in the linker *Delay Loaded Dlls* option. The VS unit testing framework introduces a dependency on *Microsoft.VisualStudio.TestTools.CppUnitTestFramework.dll* for dynamic library projects, which means that omitting the `$(DelayLoadTestDLL)` macro will result in the dynamic library failing to load.


### Running tests

When you build the test project, the tests appear in **Test Explorer**. If Test Explorer is not visible, choose **Test** on the Visual Studio menu, choose **Windows**, and then choose **Test Explorer**. From there, you can run all or a subset of the tests.
