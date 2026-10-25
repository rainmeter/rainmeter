# RainLexer release workflow

When the user asks for a "new build" or "full build" of RainLexer, treat that request as authorization to prepare the release changes, update the version, commit, tag, and push using the workflow below. Do not ask for separate confirmation of those steps. The workflow ends after verifying the push; do not build the release executable locally.

1. Read `RainLexer/Version.h` relative to this folder (full path: `C:\Rainmeter4\RainLexer\RainLexer\Version.h`). Increment the minor version and set the patch version to zero: `5.x.0` becomes `5.(x+1).0`.
2. Update all version definitions consistently: resource tuple `5,x,0,0`, version string `5.x.0.0`, and display title `RainLexer 5.x.0`. The display title must match what `Build/Build.ps1` generates.
3. Review the pending RainLexer changes and commit the version update together with all release changes. Do not push yet. Leave unrelated parent-repository changes untouched.
4. Create an annotated tag named `rainlexer-v5.x.0`, using the new version number, at the release commit. Never overwrite an existing tag.
5. Push the release commit and new tag, preferably together with an atomic push. Include all RainLexer release changes.
6. Verify that the release commit and tag were pushed successfully and that the tag points to the intended release commit. Report the new version, commit, and tag.

Pushing the new tag triggers a GitHub Actions workflow that builds the executable and has it digitally signed through SignPath. The user downloads the signed artifact from SignPath and creates the GitHub release. Do not download the signed artifact or publish a GitHub release unless the user separately requests it.

The Git repository is the parent `C:\Rainmeter4` repository. RainLexer currently uses Visual Studio platform toolset `v145`. Run appropriate source validation or tests when needed for the changes, but a release request does not require running `Build/Build.ps1 full`, producing a local installer, or waiting for the remote build and signing process.

If a source fix is needed after the tag has already been pushed, do not silently move the published tag. Report the discrepancy and resolve the release state with the user.
