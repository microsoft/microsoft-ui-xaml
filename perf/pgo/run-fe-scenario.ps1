#Requires -RunAsAdministrator

Param(
    [Parameter(Mandatory = $true)]
    [string] $modulesPath,
    [Parameter(Mandatory = $true)]
    [string] $toolsPath,
    [Parameter(Mandatory = $true)]
    [ValidateSet("x86", "x64", "arm64")]
    [string] $platform,
    [Parameter(Mandatory = $true)]
    [string] $outputPath,
    [bool] $debugTrace = $false
)

$ErrorActionPreference = "Stop"

Import-Module $PSScriptRoot\..\scripts\log-pipeline.psm1 -DisableNameChecking
Import-Module $PSScriptRoot\..\scripts\helpers.psm1 -DisableNameChecking

# Destination package path for patching. Discovered at runtime from the running explorer.exe (see
# Get-WinUIModulePath) so we patch wherever File Explorer actually loads WinUI3 from. This hardcoded
# CBS location is only the fallback if discovery fails.
$fallbackDestPath = [System.Environment]::ExpandEnvironmentVariables( "%SYSTEMROOT%\SystemApps\Microsoft.WindowsAppRuntime.CBS_8wekyb3d8bbwe" )
$destPath = $fallbackDestPath

# Modules to sweep for PGC data after training.
$pgoModules = @(
    "microsoft.ui.xaml.dll"
    "microsoft.ui.xaml.controls.dll"
    "microsoft.ui.xaml.phone.dll"
    "winuiedit.dll"
)

# Desktop context menu does not load phone.dll or winuiedit.dll.
$contextMenuModules = @(
    "microsoft.ui.xaml.dll"
    "microsoft.ui.xaml.controls.dll"
)

$otherModules = @(
    "coremessagingxp.dll"
    "dcompi.dll"
    "dwmcorei.dll"
    "dwmscenei.dll"
    "marshal.dll"
    "microsoft.directmanipulation.dll"
    "microsoft.graphics.display.dll"
    "microsoft.inputstatemanager.dll"
    "microsoft.internal.frameworkudk.dll"
    "microsoft.ui.composition.ossupport.dll"
    "microsoft.ui.input.dll"
    "microsoft.ui.windowing.core.dll"
    "microsoft.ui.windowing.dll"
    "microsoft.ui.xaml.internal.dll"
    "microsoft.ui.xaml.phone.dll"
    "microsoft.ui.xaml.resources.19h1.dll"
    "microsoft.ui.xaml.resources.common.dll"
    "microsoft.windows.applicationmodel.resources.dll"
    "mrm.dll"
    "winuiedit.dll"
    "wuceffectsi.dll"
)

$patchToolPath = Resolve-Path -LiteralPath ( Join-Path $toolsPath "pgohelper-install.cmd" )
$pgoSweepPath  = Resolve-Path -LiteralPath ( Join-Path $toolsPath "pgosweep.exe" )

Add-Type -Path "$PSScriptRoot\DesktopContextMenu.cs"

function Capture-Screenshot ( [string]$name )
{
    try {
        Add-Type -AssemblyName System.Windows.Forms -ErrorAction SilentlyContinue
        Add-Type -AssemblyName System.Drawing -ErrorAction SilentlyContinue
        $bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
        $bmp = New-Object System.Drawing.Bitmap($bounds.Width, $bounds.Height)
        $gfx = [System.Drawing.Graphics]::FromImage($bmp)
        $gfx.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
        $gfx.Dispose()
        $file = Join-Path $outputPath "$name.png"
        $bmp.Save($file, [System.Drawing.Imaging.ImageFormat]::Png)
        $bmp.Dispose()
        Log-Info "Screenshot saved: $file"
    }
    catch {
        Log-Warning "Screenshot capture failed (non-fatal): $_"
    }
}

function Get-WinUIModulePath ( )
{
    # Return the directory File Explorer actually loads WinUI3 from (so we patch the right package
    # even if the OS moves it away from the hardcoded CBS path), or $null if it isn't loaded. Caller
    # must have launched File Explorer first so explorer.exe has microsoft.ui.xaml.controls.dll loaded.
    $moduleName = "microsoft.ui.xaml.controls.dll"

    foreach ( $proc in Get-Process explorer -ErrorAction SilentlyContinue )
    {
        try
        {
            $module = $proc.Modules | Where-Object { $_.ModuleName -ieq $moduleName } | Select-Object -First 1
            if ( $module )
            {
                return Split-Path -Parent $module.FileName
            }
        }
        catch
        {
            # A process can exit or deny module access mid-enumeration; keep looking.
        }
    }

    return $null
}

function Patch-Binaries( $modules )
{
    foreach ( $module in $modules )
    {
        $toolArgs = @(
            Join-Path $modulesPath $module
            Join-Path $destPath $module
        )

        Run-Tool $patchToolPath $toolArgs
    }
}

function Patch-PRI ( )
{
    $priSource = Join-Path $modulesPath "Microsoft.UI.Xaml.Controls.pri"
    $priRenamed = Join-Path $modulesPath "resources.pri"

    Copy-Item $priSource $priRenamed -Force

    $priDest = Join-Path $destPath "resources.pri"
    Run-Tool $patchToolPath @( $priRenamed, $priDest )
}

function Patch-MUI ( )
{
    $muiModules = @(
        "microsoft.ui.xaml.dll.mui"
        "microsoft.ui.xaml.phone.dll.mui"
    )
    $srcDir = Join-Path $modulesPath "en-us"
    $dstDir = Join-Path $destPath "en-us"
    foreach ( $mui in $muiModules )
    {
        $src = Join-Path $srcDir $mui
        $dst = Join-Path $dstDir $mui
        if ( Test-Path $src )
        {
            Run-Tool $patchToolPath @( $src, $dst )
        }
        else
        {
            Log-Warning "MUI file not found (skipping): $src"
        }
    }
}

function Sweep-Binaries ( [string[]]$modules, [string]$pgcFileName )
{
    $sweepFailures = @()
    foreach ( $module in $modules )
    {
        try {
            $pgcPath = Join-Path $outputPath $module

            if ( ! ( Test-Path $pgcPath -PathType Container ) )
            {
                New-Item $pgcPath -ItemType Directory | Out-Null
            }

            $pgcFile = Join-Path $pgcPath $pgcFileName
            $toolArgs = @(
                $module
                $pgcFile
            )

            Run-Tool $pgoSweepPath $toolArgs

            # Validate PGC file was actually produced and is non-empty
            if ( -not ( Test-Path $pgcFile ) )
            {
                $sweepFailures += $module
                Log-Warning "PGC file not produced for module: $module"
            }
            elseif ( ( Get-Item $pgcFile ).Length -eq 0 )
            {
                $sweepFailures += $module
                Log-Warning "PGC file is empty (0 bytes) for module: $module"
            }
            else
            {
                Log-Info "PGC file created for ${module}: $( ( Get-Item $pgcFile ).Length ) bytes"
            }
        }
        catch {
            $sweepFailures += $module
            Log-Warning "Failed to sweep module ${module}: $_"
        }
    }

    if ( $sweepFailures.Count -gt 0 )
    {
        throw "PGO sweep failed for $($sweepFailures.Count)/$($modules.Count) modules: $($sweepFailures -join ', '). Training data is incomplete."
    }
}

function Install-Dependencies ( )
{
    Copy-Item ( Join-Path $toolsPath "pgort140.dll" ) -Destination $destPath
    $vcRedistPath = Join-Path $toolsPath ( Join-Path "crt" "vc_redist.$platform.exe" )
    & $vcRedistPath /install /quiet /norestart | Out-Null
}

$script:scriptSuccess = $false

if ( $debugTrace )
{
    & wpr.exe -start "GeneralProfile" -start "ReferenceSet" -filemode | Out-Null
}

try
{
    Log-Info "Launching File Explorer to load WinUI3 modules for patch-path discovery."
    Start-Process "explorer.exe"
    Start-Sleep 15

    $destPath = Get-WinUIModulePath
    if ( $destPath )
    {
        Log-Info "Located WinUI3 patch destination: $destPath"
    }
    else
    {
        Log-Warning "Could not locate WinUI3 module in explorer.exe; falling back to: $fallbackDestPath"
        $destPath = $fallbackDestPath
    }

    Log-Info "Kill Explorer."
    & taskkill.exe /im explorer.exe /f

    Log-Info "Installing PGO runtime dependencies."
    Install-Dependencies

    Start-Sleep 10

    Log-Info "Patch binaries."
    Patch-Binaries $pgoModules
    Patch-Binaries $otherModules

    Log-Info "Patch PRI."
    Patch-PRI

    Log-Info "Patch MUI."
    Patch-MUI

    Start-Sleep 3

    Log-Info "Starting Explorer process."
    Start-Process "explorer.exe"

    Start-Sleep 30

    Log-Info "Starting File Explorer."
    Start-Process "explorer.exe"

    Start-Sleep 30

    Log-Info "Sweep File Explorer scenario."
    Sweep-Binaries $pgoModules "explorer.pgc"
    Capture-Screenshot "after-explorer-sweep"

    # --- Desktop context menu scenario ---
    # Restart explorer shell and right-click the desktop to open the context menu.
    Log-Info "Kill Explorer for desktop context menu scenario."
    & taskkill.exe /im explorer.exe /f
    Start-Sleep 5

    Log-Info "Starting Explorer shell for desktop context menu scenario."
    Start-Process "explorer.exe"
    Start-Sleep 30

    # StartMenuExperienceHost may take focus after shell restart on x64 agents,
    # blocking desktop input.
    Stop-Process -Name "StartMenuExperienceHost" -Force -ErrorAction SilentlyContinue
    Start-Sleep 3

    Log-Info "Minimizing all windows to expose desktop."
    (New-Object -ComObject Shell.Application).MinimizeAll()
    Start-Sleep 5

    Log-Info "Right-clicking desktop to open context menu."
    [DesktopContextMenu]::RightClickScreenCenter()
    Start-Sleep 30

    Log-Info "Sweep desktop context menu scenario."
    Sweep-Binaries $contextMenuModules "contextmenu.pgc"
    Capture-Screenshot "after-contextmenu-sweep"

    Log-Info "All scenarios complete."
    $script:scriptSuccess = $true
}
catch
{
    Log-Warning "Training failed: $_"
    Log-Warning "Stack: $($_.ScriptStackTrace)"
    Capture-Screenshot "on-failure"
}
finally
{
    if ( $debugTrace )
    {
        $etl = Join-Path $outputPath "trace.etl"
        & wpr.exe -stop $etl -skipPdbGen -compress | Out-Null
    }

    if ( $script:scriptSuccess )
    {
        exit 0
    }
    else
    {
        exit 1
    }
}