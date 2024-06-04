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
CALL :copyProject "RegressionProjects\Basic\References\CSharpExe\obj\x86\Debug" "RegressionProjects\Basic\References\CSharpExe"
CALL :copyProject "RegressionProjects\Basic\References\CSharpLib\obj\x86\Debug" "RegressionProjects\Basic\References\CSharpLib"
CALL :copyProject "RegressionProjects\Basic\References\CSharpWinRTComponent\obj\x86\Debug" "RegressionProjects\Basic\References\CSharpWinRTComponent"
CALL :copyProject "RegressionProjects\Basic\References\VBExe\obj\x86\Debug" "RegressionProjects\Basic\References\VBExe"
CALL :copyProject "RegressionProjects\Basic\References\VBLib\obj\x86\Debug" "RegressionProjects\Basic\References\VBLib"
CALL :copyProject "RegressionProjects\Basic\References\VBWinRTComponent\obj\x86\Debug" "RegressionProjects\Basic\References\VBWinRTComponent"
CALL :copyProject "RegressionProjects\Basic\References\VCExe\Generated Files" "RegressionProjects\Basic\References\VCExe\generated"
CALL :copyProject "RegressionProjects\Basic\References\VCWinRTComponent\Generated Files" "RegressionProjects\Basic\References\VCWinRTComponent\generated"
CALL :copyProject "RegressionProjects\Basic\VC\EventHandling_968976\Generated Files" "RegressionProjects\Basic\VC\EventHandling_968976\generated"
CALL :copyProject "RegressionProjects\Basic\VC\Simple\Generated Files" "RegressionProjects\Basic\VC\Simple\generated"
CALL :copyProject "RegressionProjects\Basic\VisualBasic\Simple\obj\x86\Debug" "RegressionProjects\Basic\VisualBasic\Simple"

CALL :copyProject "RegressionProjects\NonStandard\NonStandardCX\NonStandardCX\Generated Files" "RegressionProjects\NonStandard\NonStandardCX\NonStandardCX\generated"

CALL :copyProject "RegressionProjects\Features\BindPhasingTestBedCpp\BindPhasingTestBedCpp\Generated Files" "RegressionProjects\Features\BindPhasingTestBedCpp\BindPhasingTestBedCpp\generated"

CALL :copyProject "RegressionProjects\Features\CompiledBinding\BindTestbedCppWinRT\Generated Files" "RegressionProjects\Features\CompiledBinding\BindTestbedCppWinRT\generated"
CALL :copyProject "RegressionProjects\Features\CompiledBinding\BindTestbedCppWinRT\Incremental\Generated Files" "RegressionProjects\Features\CompiledBinding\BindTestbedCppWinRT\BindTestbedCppWinRTIncremental\generated"
CALL :copyProject "RegressionProjects\Features\CompiledBinding\BindTestbedCX\Generated Files" "RegressionProjects\Features\CompiledBinding\BindTestbedCX\generated"
CALL :copyProject "RegressionProjects\Features\CompiledBinding\BindTestbedCX\Incremental\Generated Files" "RegressionProjects\Features\CompiledBinding\BindTestbedCX\BindTestbedCXIncremental\generated"
CALL :copyProject "RegressionProjects\Features\CompiledBinding\BindTestbedModel\obj\x86\Debug" "RegressionProjects\Features\CompiledBinding\BindTestbedModel"
CALL :copyProject "RegressionProjects\Features\CompiledBinding\BindTestbedCS\obj\x86\Debug" "RegressionProjects\Features\CompiledBinding\BindTestbedCS"
CALL :copyProject "RegressionProjects\Features\CompiledBinding\BindTestbedVB\obj\x86\Debug" "RegressionProjects\Features\CompiledBinding\BindTestbedVB"

CALL :copyProject "RegressionProjects\Features\CompiledBinding\BindTestbedBackcompat\RS1\BindTestbedCppWinRT\Generated Files" "RegressionProjects\Features\CompiledBinding\BindTestbedBackcompat\RS1\BindTestbedCppWinRT\generated"
CALL :copyProject "RegressionProjects\Features\CompiledBinding\BindTestbedBackcompat\RS1\BindTestbedCX\Generated Files" "RegressionProjects\Features\CompiledBinding\BindTestbedBackcompat\RS1\BindTestbedCX\generated"
CALL :copyProject "RegressionProjects\Features\CompiledBinding\BindTestbedBackcompat\RS1\BindTestbedModel\obj\x86\Debug" "RegressionProjects\Features\CompiledBinding\BindTestbedBackcompat\RS1\BindTestbedModel"
CALL :copyProject "RegressionProjects\Features\CompiledBinding\BindTestbedBackcompat\RS1\BindTestbedCS\obj\x86\Debug" "RegressionProjects\Features\CompiledBinding\BindTestbedBackcompat\RS1\BindTestbedCS"
CALL :copyProject "RegressionProjects\Features\CompiledBinding\BindTestbedBackcompat\RS1\BindTestbedVB\obj\x86\Debug" "RegressionProjects\Features\CompiledBinding\BindTestbedBackcompat\RS1\BindTestbedVB"

CALL :copyProject "RegressionProjects\Features\Conditionals\ConditionalControls\obj\x86\Debug" "RegressionProjects\Features\Conditionals\ConditionalControls"
CALL :copyProject "RegressionProjects\Features\Conditionals\ConditionalsCppWinRT\Generated Files" "RegressionProjects\Features\Conditionals\ConditionalsCppWinRT\generated"
CALL :copyProject "RegressionProjects\Features\Conditionals\ConditionalsCX\Generated Files" "RegressionProjects\Features\Conditionals\ConditionalsCX\generated"
CALL :copyProject "RegressionProjects\Features\Conditionals\ConditionalsCS\obj\x86\Debug" "RegressionProjects\Features\Conditionals\ConditionalsCS"
CALL :copyProject "RegressionProjects\Features\Conditionals\ConditionalsModel\obj\x86\Debug" "RegressionProjects\Features\Conditionals\ConditionalsModel"
CALL :copyProject "RegressionProjects\Features\Conditionals\ConditionalsVB\obj\x86\Debug" "RegressionProjects\Features\Conditionals\ConditionalsVB"

CALL :copyProject "RegressionProjects\Features\Conditionals\Platform Conditionals\PlatformConditionalsCS\obj\x86\Debug" "RegressionProjects\Features\Conditionals\Platform Conditionals\PlatformConditionalsCS"
CALL :copyProject "RegressionProjects\Features\Conditionals\Platform Conditionals\PlatformConditionalsModel\obj\x86\Debug" "RegressionProjects\Features\Conditionals\Platform Conditionals\PlatformConditionalsModel"

CALL :copyProject "RegressionProjects\Features\DeferLoadStrategy\VC\Generated Files" "RegressionProjects\Features\DeferLoadStrategy\VC\generated"
CALL :copyProject "RegressionProjects\Features\DeferLoadStrategy\CSharp\obj\x86\Debug" "RegressionProjects\Features\DeferLoadStrategy\CSharp"
CALL :copyProject "RegressionProjects\Features\DeferLoadStrategy\VisualBasic\obj\x86\Debug" "RegressionProjects\Features\DeferLoadStrategy\VisualBasic"

CALL :copyProject "RegressionProjects\Features\LinkedMD\AppCX\Generated Files" "RegressionProjects\Features\LinkedMD\AppCX\generated"
CALL :copyProject "RegressionProjects\Features\LinkedMD\ControlsCX\Generated Files" "RegressionProjects\Features\LinkedMD\ControlsCX\generated"
CALL :copyProject "RegressionProjects\Features\LinkedMD\LinkedMDAppBV\obj\x86\Debug" "RegressionProjects\Features\LinkedMD\LinkedMDAppBV"
CALL :copyProject "RegressionProjects\Features\LinkedMD\LinkedMDAppCppWinRT\Generated Files" "RegressionProjects\Features\LinkedMD\LinkedMDAppCppWinRT\generated"
CALL :copyProject "RegressionProjects\Features\LinkedMD\LinkedMDAppCS\obj\x86\Debug" "RegressionProjects\Features\LinkedMD\LinkedMDAppCS"
CALL :copyProject "RegressionProjects\Features\LinkedMD\LinkedMDAppBV\obj\x86\Debug" "RegressionProjects\Features\LinkedMD\LinkedMDAppBV"
CALL :copyProject "RegressionProjects\Features\LinkedMD\LinkedMDControlsCppWinRT\Generated Files" "RegressionProjects\Features\LinkedMD\LinkedMDControlsCppWinRT\generated"
CALL :copyProject "RegressionProjects\Features\LinkedMD\LinkedMDControlsCS\obj\x86\Debug" "RegressionProjects\Features\LinkedMD\LinkedMDControlsCS"
CALL :copyProject "RegressionProjects\Features\LinkedMD\LinkedMDControlsVB\obj\x86\Debug" "RegressionProjects\Features\LinkedMD\LinkedMDControlsVB"
CALL :copyProject "RegressionProjects\Features\LinkedMD\LinkedMDSubControlsCppWinRT\Generated Files" "RegressionProjects\Features\LinkedMD\LinkedMDSubControlsCppWinRT\generated"
CALL :copyProject "RegressionProjects\Features\LinkedMD\LinkedMDSubControlsCS\obj\x86\Debug" "RegressionProjects\Features\LinkedMD\LinkedMDSubControlsCS"
CALL :copyProject "RegressionProjects\Features\LinkedMD\LinkedMDSubControlsVB\obj\x86\Debug" "RegressionProjects\Features\LinkedMD\LinkedMDSubControlsVB"
CALL :copyProject "RegressionProjects\Features\LinkedMD\SubControlsCX\Generated Files" "RegressionProjects\Features\LinkedMD\SubControlsCX\generated"

CALL :copyProject "RegressionProjects\Features\MarkupExtensions\MarkupExtensionsCX\Generated Files" "RegressionProjects\Features\MarkupExtensions\MarkupExtensionsCX\generated"
CALL :copyProject "RegressionProjects\Features\MarkupExtensions\MarkupExtensionsCppWinRT\Generated Files" "RegressionProjects\Features\MarkupExtensions\MarkupExtensionsCppWinRT\generated"
CALL :copyProject "RegressionProjects\Features\MarkupExtensions\MarkupExtensionsCS\obj\x86\Debug" "RegressionProjects\Features\MarkupExtensions\MarkupExtensionsCS"
CALL :copyProject "RegressionProjects\Features\MarkupExtensions\MarkupExtensionsVB\obj\x86\Debug" "RegressionProjects\Features\MarkupExtensions\MarkupExtensionsVB"

CALL :copyProject "RegressionProjects\Features\Metadata\MetadataTestbedCX\Generated Files" "RegressionProjects\Features\Metadata\MetadataTestbedCX\generated"
CALL :copyProject "RegressionProjects\Features\Metadata\MetadataTestbedCS\obj\x86\Debug" "RegressionProjects\Features\Metadata\MetadataTestbedCS"
CALL :copyProject "RegressionProjects\Features\Metadata\MetadataTestbedVB\obj\x86\Debug" "RegressionProjects\Features\Metadata\MetadataTestbedVB"

CALL :copyProject "RegressionProjects\Features\MultiXamlFiles\MultipleViewsTestbedCpp\Generated Files" "RegressionProjects\Features\MultiXamlFiles\MultipleViewsTestbedCpp\generated"
CALL :copyProject "RegressionProjects\Features\MultiXamlFiles\MultipleViewsTestbedCppWinRT\Generated Files" "RegressionProjects\Features\MultiXamlFiles\MultipleViewsTestbedCppWinRT\generated"
CALL :copyProject "RegressionProjects\Features\MultiXamlFiles\MultipleViewsTestbed\obj\x86\Debug" "RegressionProjects\Features\MultiXamlFiles\MultipleViewsTestbed"
CALL :copyProject "RegressionProjects\Features\MultiXamlFiles\MultipleViewsTestbedVB\obj\x86\Debug" "RegressionProjects\Features\MultiXamlFiles\MultipleViewsTestbedVB"

CALL :copyProject "RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ConsumerCpp\Generated Files" "RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ConsumerCpp\generated"
CALL :copyProject "RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ProviderCpp\Generated Files" "RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ProviderCpp\generated"
CALL :copyProject "RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ConsumerCs\obj\x86\Debug" "RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ConsumerCs"
CALL :copyProject "RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ProviderCs\obj\x86\Debug" "RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ProviderCs"
CALL :copyProject "RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ConsumerVb\obj\x86\Debug" "RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ConsumerVb"
CALL :copyProject "RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ProviderVb\obj\x86\Debug" "RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ProviderVb"

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