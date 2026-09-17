rem Makes the repo use the appropriate build files for .NET
rem Any files populated in this way should have their destination file added to the root .gitignore

@echo off

echo Configuring test host runtime for target framework %_DotNetMoniker% ^(separate from the build SDK^)...
copy /y %1\dxaml\test\infra\taefhostappnetcore\WinRT.Host.runtimeconfig.json.%_DotNetMoniker% %1\dxaml\test\infra\taefhostappnetcore\WinRT.Host.runtimeconfig.json >NUL
