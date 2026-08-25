$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$failures = [Collections.Generic.List[string]]::new()

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Assert-Equal {
    param($Expected, $Actual, [string]$Message)
    if ($Expected -ne $Actual) {
        throw "$Message Expected '$Expected', got '$Actual'."
    }
}

function Test-Case {
    param([string]$Name, [scriptblock]$Body)
    try {
        & $Body
        Write-Host "[PASS] $Name"
    } catch {
        $failures.Add("${Name}: $($_.Exception.Message)")
        Write-Host "[FAIL] $Name - $($_.Exception.Message)" -ForegroundColor Red
    }
}

function Invoke-FakeBuild {
    param(
        [string[]]$BuildArguments,
        [string]$PathPrefix,
        [string]$TestRepoRoot = $repoRoot
    )

    $argumentText = $BuildArguments -join " "
    $command = @(
        'set "EnvironmentInitialized=1"'
        "set `"RepoRoot=$TestRepoRoot`""
        "set `"reporoot=$TestRepoRoot`""
        'set "_BuildArch=amd64"'
        'set "_BuildType=chk"'
        'set "Configuration=Debug"'
        'set "Platform=x64"'
    )
    if ($PathPrefix) {
        $command += "set `"PATH=$PathPrefix;%PATH%`""
    }
    $command += "call `"$TestRepoRoot\Build.cmd`" $argumentText"

    $output = & $env:ComSpec /d /c ($command -join " && ") 2>&1
    [pscustomobject]@{
        ExitCode = $LASTEXITCODE
        Text = ($output -join "`n")
    }
}

$tempRoot = Join-Path ([IO.Path]::GetTempPath()) ("WinUI agent build tests " + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force -Path $tempRoot | Out-Null

try {
    Test-Case "Invoke-CmdScript preserves success environment and exit code" {
        $cmdPath = Join-Path $tempRoot "set environment.cmd"
        [IO.File]::WriteAllText($cmdPath, "@echo off`r`nset AGENT_BUILD_TEST_VALUE=propagated`r`nexit /b 0`r`n")

        . (Join-Path $repoRoot "scripts\init\Invoke-CmdScript.ps1") $cmdPath ""

        Assert-Equal "propagated" $env:AGENT_BUILD_TEST_VALUE "Environment changes were not propagated."
        Assert-Equal 0 $LASTEXITCODE "Successful batch exit code was not preserved."
    }

    Test-Case "Invoke-CmdScript preserves failure exit code" {
        $cmdPath = Join-Path $tempRoot "fail initialization.cmd"
        [IO.File]::WriteAllText($cmdPath, "@echo off`r`nexit /b 7`r`n")

        . (Join-Path $repoRoot "scripts\init\Invoke-CmdScript.ps1") $cmdPath ""

        Assert-Equal 7 $LASTEXITCODE "Failed batch exit code was lost."
    }

    Test-Case "Windows PowerShell prefers its own modules over PowerShell 7" {
        # Regression guard for Initialize-NuGet.ps1: Windows PowerShell 5 can inherit a
        # PowerShell 7 module path (MSI or Store/MSIX) and then fail to resolve Get-FileHash.
        $initializeNuGet = Join-Path $repoRoot "scripts\init\Initialize-NuGet.ps1"
        $poisonedPath = "C:\Program Files\WindowsApps\Microsoft.PowerShell_7.6.4.0_x64__8wekyb3d8bbwe\Modules"
        $normalization = [IO.File]::ReadAllText($initializeNuGet) -replace '(?s)^.*?\$originalPSModulePath = \$env:PSModulePath', '$originalPSModulePath = $env:PSModulePath'
        $normalization = ($normalization -split '(?m)^try\s*$')[0]
        $command = @"
`$env:PSModulePath = '$poisonedPath;' + `$env:PSModulePath
$normalization
Remove-Module Microsoft.PowerShell.Utility -Force -ErrorAction SilentlyContinue
Import-Module Microsoft.PowerShell.Utility -ErrorAction SilentlyContinue
if (-not (Get-Command Get-FileHash -ErrorAction SilentlyContinue)) { exit 7 }
if ((`$env:PSModulePath -split ';')[0] -ne (Join-Path `$PSHOME 'Modules')) { exit 8 }
exit 0
"@

        & powershell -ExecutionPolicy Bypass -NoProfile -Command $command

        Assert-Equal 0 $LASTEXITCODE "Windows PowerShell module-path normalization failed."
    }

    Test-Case "initrun preserves arguments and command exit code from a spaced path" {
        $fixture = Join-Path $tempRoot "fixture repo"
        New-Item -ItemType Directory -Force -Path $fixture | Out-Null
        Copy-Item (Join-Path $repoRoot "initrun.ps1") $fixture

        [IO.File]::WriteAllText(
            (Join-Path $fixture "init.ps1"),
            "param([Parameter(ValueFromRemainingArguments=`$true)][string[]]`$Forwarded)`r`nexit 0`r`n")
        [IO.File]::WriteAllText(
            (Join-Path $fixture "capture.ps1"),
            "param([Parameter(ValueFromRemainingArguments=`$true)][string[]]`$Forwarded)`r`n" +
            "[IO.File]::WriteAllLines(`$env:AGENT_BUILD_ARGS_LOG, `$Forwarded)`r`n" +
            "& `$env:ComSpec /d /c 'exit 9'`r`n")

        $argsLog = Join-Path $tempRoot "forwarded args.txt"
        $env:AGENT_BUILD_ARGS_LOG = $argsLog
        & pwsh -NoProfile -File (Join-Path $fixture "initrun.ps1") ".\capture.ps1" "/q" "/fake" "product" "C:\project path\sample.vcxproj" *> $null

        Assert-Equal 9 $LASTEXITCODE "Command exit code was not returned by initrun."
        $forwarded = @(Get-Content $argsLog)
        Assert-Equal 4 $forwarded.Count "initrun changed the argument count."
        Assert-Equal "/q" $forwarded[0] "The /q argument changed."
        Assert-Equal "/fake" $forwarded[1] "The /fake argument changed."
        Assert-Equal "product" $forwarded[2] "The target argument changed."
        Assert-Equal "C:\project path\sample.vcxproj" $forwarded[3] "The spaced project path changed."
    }

    Test-Case "initrun stops before the command when initialization fails" {
        $fixture = Join-Path $tempRoot "failed init fixture"
        New-Item -ItemType Directory -Force -Path $fixture | Out-Null
        Copy-Item (Join-Path $repoRoot "initrun.ps1") $fixture
        [IO.File]::WriteAllText((Join-Path $fixture "init.ps1"), "throw 'expected init failure'`r`n")
        [IO.File]::WriteAllText((Join-Path $fixture "should-not-run.ps1"), "[IO.File]::WriteAllText(`$env:AGENT_BUILD_MARKER, 'ran')`r`n")

        $marker = Join-Path $tempRoot "command-ran.txt"
        $env:AGENT_BUILD_MARKER = $marker
        $output = & pwsh -NoProfile -File (Join-Path $fixture "initrun.ps1") ".\should-not-run.ps1" 2>&1

        Assert-Equal 1 $LASTEXITCODE "initrun did not fail after initialization failed."
        Assert-True (-not (Test-Path $marker)) "initrun executed the command after initialization failed."
        Assert-True (($output -join "`n") -match "Run a full init first") "initrun did not provide the recovery command."
    }

    Test-Case "initrun automatically performs first-time initialization" {
        $fixture = Join-Path $tempRoot "automatic init fixture"
        New-Item -ItemType Directory -Force -Path $fixture | Out-Null
        Copy-Item (Join-Path $repoRoot "initrun.ps1") $fixture
        [IO.File]::WriteAllText(
            (Join-Path $fixture "init.ps1"),
            "[IO.File]::AppendAllText(`$env:AGENT_BUILD_INIT_LOG, ((`$args -join '|') + [Environment]::NewLine))`r`n" +
            "if (`$args -contains '/envcheck') { exit 5 }`r`n" +
            "exit 0`r`n")
        [IO.File]::WriteAllText(
            (Join-Path $fixture "build.ps1"),
            "[IO.File]::WriteAllText(`$env:AGENT_BUILD_MARKER, 'built')`r`n" +
            "exit 0`r`n")

        $initLog = Join-Path $tempRoot "automatic init calls.txt"
        $marker = Join-Path $tempRoot "automatic build marker.txt"
        $env:AGENT_BUILD_INIT_LOG = $initLog
        $env:AGENT_BUILD_MARKER = $marker
        $output = & pwsh -NoProfile -File (Join-Path $fixture "initrun.ps1") -EnsureInitialized ".\build.ps1" 2>&1

        Assert-Equal 0 $LASTEXITCODE "initrun failed after automatic initialization succeeded."
        Assert-True (Test-Path $marker) "The build command did not run after automatic initialization."
        $initCalls = @(Get-Content $initLog)
        Assert-Equal 2 $initCalls.Count "initrun did not perform an environment check followed by full initialization."
        Assert-True ($initCalls[0] -match "/envcheck") "The first initialization call was not the lightweight check."
        Assert-True ($initCalls[1] -notmatch "/envcheck") "The second initialization call was not full setup."
        Assert-True (($output -join "`n") -match "Running one-time setup") "Automatic setup was not explained in the output."
    }

    Test-Case "init.cmd requires a successful flavor-specific initialization" {
        $init = Get-Content -Raw (Join-Path $repoRoot "init.cmd")

        Assert-True ($init.Contains('init-complete-%_BuildArch%%_BuildType%')) "init.cmd does not use a flavor-specific completion marker."
        Assert-True ($init.Contains('if "%EnvCheck%"=="true" if not exist "%_InitCompleteMarker%"')) "/envcheck does not require successful initialization."
        Assert-True ($init.Contains('>"%_InitCompleteMarker%" echo initialized')) "Full initialization does not write the completion marker."
        Assert-True ($init.Contains('if exist "%_InitCompleteMarker%" del /q "%_InitCompleteMarker%"')) "A new full initialization does not invalidate stale success state."
        Assert-True ($init.IndexOf('set _InitCompleteMarker=') -gt $init.IndexOf('call :SetEnviromentVariable _BuildType chk')) "The completion marker is computed before the build flavor is known."
        Assert-True ($init.Contains('ver >nul')) "The marker write can inherit an unrelated earlier failure code."
    }

    Test-Case "Build.cmd generates full, product, and MUX commands" {
        $full = Invoke-FakeBuild @("/fake", "/q") ""
        Assert-Equal 0 $full.ExitCode "Full fake build failed."
        Assert-True ($full.Text -match "dxaml\\Microsoft\.UI\.Xaml\.sln") "Full build omitted the product/test solution."
        Assert-True ($full.Text -match "controls\\MUXControls\.sln") "Full build omitted MUXControls."

        $product = Invoke-FakeBuild @("product", "/fake", "/q") ""
        Assert-Equal 0 $product.ExitCode "Product fake build failed."
        Assert-True ($product.Text -match "Microsoft\.UI\.Xaml-Product\.sln") "Product build selected the wrong solution."
        Assert-True ($product.Text -notmatch "controls\\MUXControls\.sln") "Product build unexpectedly selected tests."

        $mux = Invoke-FakeBuild @("mux", "/fake", "/q") ""
        Assert-Equal 0 $mux.ExitCode "MUX fake build failed."
        Assert-True ($mux.Text -match "Microsoft\.ui\.xaml\.vcxproj") "MUX build selected the wrong project."
        Assert-True ($mux.Text -notmatch "Microsoft\.UI\.Xaml-Product\.sln") "MUX build unexpectedly selected the full product."

        $serial = Invoke-FakeBuild @("mux", "/fake", "/q", "/m:1") ""
        Assert-Equal 0 $serial.ExitCode "Serial fake build failed."
        Assert-True ($serial.Text -match "/m:1") "Serial build did not forward the single-process option."
    }

    Test-Case "Build.cmd returns the failing MSBuild exit code and binlog location" {
        $fakeTools = Join-Path $tempRoot "fake tools"
        New-Item -ItemType Directory -Force -Path $fakeTools | Out-Null
        [IO.File]::WriteAllText((Join-Path $fakeTools "msbuild.cmd"), "@echo off`r`nexit /b 7`r`n")

        $result = Invoke-FakeBuild @("mux", "/q") $fakeTools

        Assert-Equal 7 $result.ExitCode "Build.cmd replaced the MSBuild failure code."
        Assert-True ($result.Text -match "Binlog is here:") "Build.cmd did not report the failure binlog."
    }

    Test-Case "Build.cmd returns a failing mock-package exit code" {
        $fixture = Join-Path $tempRoot "mock package failure"
        $fakeTools = Join-Path $fixture "fake tools"
        New-Item -ItemType Directory -Force -Path $fakeTools | Out-Null
        Copy-Item (Join-Path $repoRoot "Build.cmd") $fixture
        [IO.File]::WriteAllText((Join-Path $fakeTools "msbuild.cmd"), "@echo off`r`nexit /b 0`r`n")
        [IO.File]::WriteAllText((Join-Path $fixture "pack.component.cmd"), "@echo off`r`nexit /b 11`r`n")

        $result = Invoke-FakeBuild @("product", "/q") $fakeTools $fixture

        Assert-Equal 11 $result.ExitCode "Build.cmd replaced the mock-package failure code."
    }

    Test-Case "Build.cmd reports a crashing MSBuild as a failure" {
        # Structured-exception exit codes are negative, and "if ERRORLEVEL 1" means
        # "errorlevel >= 1", so a crash must not be misreported as a successful build.
        $fakeTools = Join-Path $tempRoot "crashing fake tools"
        New-Item -ItemType Directory -Force -Path $fakeTools | Out-Null
        [IO.File]::WriteAllText((Join-Path $fakeTools "msbuild.cmd"), "@echo off`r`nexit /b -1073741819`r`n")

        $result = Invoke-FakeBuild @("mux", "/q") $fakeTools

        Assert-True ($result.ExitCode -ne 0) "Build.cmd reported a crashing MSBuild as success."
        Assert-True ($result.Text -match "Binlog is here:") "Build.cmd did not report the failure binlog."
        Assert-True ($result.Text -notmatch "(?m)^Binlog: ") "Build.cmd printed the success binlog line for a crash."
    }

    Test-Case "Build.cmd quiet mode reports successful binlog locations" {
        $fakeTools = Join-Path $tempRoot "successful fake tools"
        New-Item -ItemType Directory -Force -Path $fakeTools | Out-Null
        [IO.File]::WriteAllText((Join-Path $fakeTools "msbuild.cmd"), "@echo off`r`nexit /b 0`r`n")

        $result = Invoke-FakeBuild @("mux", "/q") $fakeTools

        Assert-Equal 0 $result.ExitCode "Quiet build failed with a successful MSBuild."
        Assert-True ($result.Text -match "Binlog: .*\.binlog") "Quiet mode did not report a successful binlog."
    }

    Test-Case "Public Copilot guidance points at the build skill and stays free of internal feed steps" {
        $instructions = Get-Content -Raw (Join-Path $repoRoot ".github\copilot-instructions.md")
        $buildSkill = Get-Content -Raw (Join-Path $repoRoot ".github\skills\build\SKILL.md")

        Assert-True ($instructions.Contains('.github/skills/build/SKILL.md')) "Copilot instructions do not point to the build skill."
        Assert-True ($buildSkill -notmatch 'OS\.Developer|your-PAT|_usersSettings/tokens') "Public build guidance contains internal feed or PAT instructions."
        Assert-True ($buildSkill.Contains('-EnsureInitialized')) "Build guidance does not use self-contained initialization."
    }
} finally {
    Remove-Item -Recurse -Force $tempRoot -ErrorAction SilentlyContinue
    Remove-Item Env:\AGENT_BUILD_TEST_VALUE -ErrorAction SilentlyContinue
    Remove-Item Env:\AGENT_BUILD_ARGS_LOG -ErrorAction SilentlyContinue
    Remove-Item Env:\AGENT_BUILD_INIT_LOG -ErrorAction SilentlyContinue
    Remove-Item Env:\AGENT_BUILD_MARKER -ErrorAction SilentlyContinue
}

if ($failures.Count -gt 0) {
    Write-Host ""
    $failures | ForEach-Object { Write-Host $_ -ForegroundColor Red }
    exit 1
}

Write-Host ""
Write-Host "All agentic build regression tests passed."
