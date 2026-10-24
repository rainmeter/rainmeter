# RainLexer release workflow

When the user asks for a "new build" of RainLexer, treat that request as authorization to perform the entire release workflow below, including committing, tagging, pushing, and building. Do not ask for separate confirmation of those steps.

1. Read `RainLexer/Version.h` relative to this folder (full path: `C:\Rainmeter4\RainLexer\RainLexer\Version.h`). Increment the minor version and set the patch version to zero: `5.x.0` becomes `5.(x+1).0`.
2. Update all version definitions consistently: resource tuple `5,x,0,0`, version string `5.x.0.0`, and display title `RainLexer 5.x.0`. The display title must match what `Build/Build.ps1` generates.
3. Review the pending RainLexer changes and commit the version update together with all release changes. Do not push yet. Leave unrelated parent-repository changes untouched.
4. Create an annotated tag named `rainlexer-v5.x.0`, using the new version number, at the release commit. Never overwrite an existing tag.
5. Push the release commit and new tag, preferably together with an atomic push. Include all RainLexer release changes.
6. Run the full release build with `Build/Build.ps1 full 5.x.0`, using the new version. In a noninteractive session, set `CI=true` to suppress the final prompt. This builds and runs the lexer tests, builds Win32 and x64 release DLLs, generates theme configurations, and creates the installer.
7. Verify the build result and embedded versions, and report the commit, tag, test results, and installer path. Check for build-generated source changes before declaring completion.

The Git repository is the parent `C:\Rainmeter4` repository. RainLexer currently uses Visual Studio platform toolset `v145`. The release installer is written to `Build/RainLexer-5.x.0.exe` within this folder.

If a build requires a source fix after the tag has already been pushed, do not silently move the published tag. Report the discrepancy and resolve the release state with the user.
