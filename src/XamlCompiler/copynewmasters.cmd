@echo off

REM Copyright (c) Microsoft Corporation.
REM Licensed under the MIT License. See LICENSE in the project root for license information.

SETLOCAL
call csc tools\fixmasters\fixmasters.cs /out:%temp%\fixmasters.exe
if NOT %ERRORLEVEL%==0 goto failedCopy

del /q /s TestMasters
if NOT %ERRORLEVEL%==0 goto failedCopy

CALL :copyProject "RegressionProjects\Basic\CSharp\Simple\obj\x86\Debug" "RegressionProjects\Basic\CSharp\Simple" 
CALL :copyProject "RegressionProjects\Basic\CppWinRT\Simple\Generated Files" "RegressionProjects\Basic\CppWinRT\Simple\generated"
CALL :copyProject "RegressionProjects\Basic\CppWinRT\EventHandling_968976\Generated Files" "RegressionProjects\Basic\CppWinRT\EventHandling_968976\generated"
CALL :copyProject "RegressionProjects\Basic\References\CSharpExe\obj\x86\Debug" "RegressionProjects\Basic\References\CSharpExe"
CALL :copyProject "RegressionProjects\Basic\References\CSharpLib\obj\x86\Debug" "RegressionProjects\Basic\References\CSharpLib"
CALL :copyProject "RegressionProjects\Basic\References\CSharpWinRTComponent\obj\x86\Debug" "RegressionProjects\Basic\References\CSharpWinRTComponent"
CALL :copyProject "RegressionProjects\Basic\References\CppWinRTComponent\Generated Files" "RegressionProjects\Basic\References\CppWinRTComponent\generated"
CALL :copyProject "RegressionProjects\Basic\References\CppWinRTExe\Generated Files" "RegressionProjects\Basic\References\CppWinRTExe\generated"

CALL :copyProject "RegressionProjects\NonStandard\NonStandardCppWinRT\NonStandardCppWinRT\Generated Files" "RegressionProjects\NonStandard\NonStandardCppWinRT\NonStandardCppWinRT\generated"

CALL :copyProject "RegressionProjects\Features\BindPhasingTestBedCppWinRT\BindPhasingTestBedCppWinRT\Generated Files" "RegressionProjects\Features\BindPhasingTestBedCppWinRT\BindPhasingTestBedCppWinRT\generated"


CALL :copyProject "RegressionProjects\Features\CompiledBinding\BindTestbedCppWinRT\Generated Files" "RegressionProjects\Features\CompiledBinding\BindTestbedCppWinRT\generated"
CALL :copyProject "RegressionProjects\Features\CompiledBinding\BindTestbedCppWinRT\Incremental\Generated Files" "RegressionProjects\Features\CompiledBinding\BindTestbedCppWinRT\BindTestbedCppWinRTIncremental\generated"
CALL :copyProject "RegressionProjects\Features\CompiledBinding\BindTestbedModel\obj\x86\Debug" "RegressionProjects\Features\CompiledBinding\BindTestbedModel"
CALL :copyProject "RegressionProjects\Features\CompiledBinding\BindTestbedCS\obj\x86\Debug" "RegressionProjects\Features\CompiledBinding\BindTestbedCS"


CALL :copyProject "RegressionProjects\Features\Conditionals\ConditionalControls\obj\x86\Debug" "RegressionProjects\Features\Conditionals\ConditionalControls"
CALL :copyProject "RegressionProjects\Features\Conditionals\ConditionalsCppWinRT\Generated Files" "RegressionProjects\Features\Conditionals\ConditionalsCppWinRT\generated"
CALL :copyProject "RegressionProjects\Features\Conditionals\ConditionalsCS\obj\x86\Debug" "RegressionProjects\Features\Conditionals\ConditionalsCS"
CALL :copyProject "RegressionProjects\Features\Conditionals\ConditionalsModel\obj\x86\Debug" "RegressionProjects\Features\Conditionals\ConditionalsModel"

CALL :copyProject "RegressionProjects\Features\Conditionals\Platform Conditionals\PlatformConditionalsCS\obj\x86\Debug" "RegressionProjects\Features\Conditionals\Platform Conditionals\PlatformConditionalsCS"
CALL :copyProject "RegressionProjects\Features\Conditionals\Platform Conditionals\PlatformConditionalsModel\obj\x86\Debug" "RegressionProjects\Features\Conditionals\Platform Conditionals\PlatformConditionalsModel"

CALL :copyProject "RegressionProjects\Features\DeferLoadStrategy\CSharp\obj\x86\Debug" "RegressionProjects\Features\DeferLoadStrategy\CSharp"
CALL :copyProject "RegressionProjects\Features\DeferLoadStrategy\CppWinRT\Generated Files" "RegressionProjects\Features\DeferLoadStrategy\CppWinRT\generated"

CALL :copyProject "RegressionProjects\Features\LinkedMD\LinkedMDAppCppWinRT\Generated Files" "RegressionProjects\Features\LinkedMD\LinkedMDAppCppWinRT\generated"
CALL :copyProject "RegressionProjects\Features\LinkedMD\LinkedMDAppCS\obj\x86\Debug" "RegressionProjects\Features\LinkedMD\LinkedMDAppCS"
CALL :copyProject "RegressionProjects\Features\LinkedMD\LinkedMDControlsCppWinRT\Generated Files" "RegressionProjects\Features\LinkedMD\LinkedMDControlsCppWinRT\generated"
CALL :copyProject "RegressionProjects\Features\LinkedMD\LinkedMDControlsCS\obj\x86\Debug" "RegressionProjects\Features\LinkedMD\LinkedMDControlsCS"
CALL :copyProject "RegressionProjects\Features\LinkedMD\LinkedMDSubControlsCppWinRT\Generated Files" "RegressionProjects\Features\LinkedMD\LinkedMDSubControlsCppWinRT\generated"
CALL :copyProject "RegressionProjects\Features\LinkedMD\LinkedMDSubControlsCS\obj\x86\Debug" "RegressionProjects\Features\LinkedMD\LinkedMDSubControlsCS"

CALL :copyProject "RegressionProjects\Features\MarkupExtensions\MarkupExtensionsCppWinRT\Generated Files" "RegressionProjects\Features\MarkupExtensions\MarkupExtensionsCppWinRT\generated"
CALL :copyProject "RegressionProjects\Features\MarkupExtensions\MarkupExtensionsCS\obj\x86\Debug" "RegressionProjects\Features\MarkupExtensions\MarkupExtensionsCS"

CALL :copyProject "RegressionProjects\Features\Metadata\MetadataTestbedCS\obj\x86\Debug" "RegressionProjects\Features\Metadata\MetadataTestbedCS"
CALL :copyProject "RegressionProjects\Features\Metadata\MetadataTestbedCppWinRT\Generated Files" "RegressionProjects\Features\Metadata\MetadataTestbedCppWinRT\generated"

CALL :copyProject "RegressionProjects\Features\MultiXamlFiles\MultipleViewsTestbedCppWinRT\Generated Files" "RegressionProjects\Features\MultiXamlFiles\MultipleViewsTestbedCppWinRT\generated"
CALL :copyProject "RegressionProjects\Features\MultiXamlFiles\MultipleViewsTestbed\obj\x86\Debug" "RegressionProjects\Features\MultiXamlFiles\MultipleViewsTestbed"

CALL :copyProject "RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ConsumerCs\obj\x86\Debug" "RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ConsumerCs"
CALL :copyProject "RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ProviderCs\obj\x86\Debug" "RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ProviderCs"
CALL :copyProject "RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ConsumerCppWinRT\Generated Files" "RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ConsumerCppWinRT\generated"
CALL :copyProject "RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ProviderCppWinRT\Generated Files" "RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ProviderCppWinRT\generated"

CALL :copyProject "RegressionProjects\Features\StaticLibs\RuntimeComponentWithStaticLibInApp\Generated Files" "RegressionProjects\Features\StaticLibs\RuntimeComponentWithStaticLibInApp\generated"
CALL :copyProject "RegressionProjects\Features\StaticLibs\StaticControlsLib\Generated Files" "RegressionProjects\Features\StaticLibs\StaticControlsLib\generated"
CALL :copyProject "RegressionProjects\Features\StaticLibs\StaticLibInApp\Generated Files" "RegressionProjects\Features\StaticLibs\StaticLibInApp\generated"
CALL :copyProject "RegressionProjects\Features\StaticLibs\StaticLibInRuntimeComponent\Generated Files" "RegressionProjects\Features\StaticLibs\StaticLibInRuntimeComponent\generated"

CALL :copyProject "RegressionProjects\Features\CustomAppXaml\CustomAppXaml\obj\x86\Debug" "RegressionProjects\Features\CustomAppXaml\CustomAppXaml"

call %temp%\fixmasters.exe
if NOT %ERRORLEVEL%==0 goto failedCopy

echo.
echo Done.
goto :EOF

:failedCopy
echo.
@echo ERROR: Failed to copy! You may be in a half-state if some items copied and some other didn't.
()
goto :EOF

:copyProject
echo ## Updating %~2 to %~1
echo.

robocopy "%BuildOutputRoot%\%_BuildArch%%_BuildType%\src\XamlCompiler\Tests\%~2" "TestMasters\%~1" *.g.* /XF *.g.obj /XF *.nuget.g.* /XF *.backup /s /r:0 /z /ndl
if %ERRORLEVEL% GTR 1 goto :failedCopy
EXIT /B 0
