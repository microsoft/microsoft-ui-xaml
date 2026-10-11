<#
.SYNOPSIS
  Writes env.json (hardware/OS/runtime) and footprint.json (per-control DLL/package size)
  next to a results CSV so report.py can render the Test-environment and Binary-impact sections.
#>
param(
  [Parameter(Mandatory = $true)][string]$OutDir,
  [string]$ControlVersion = "TableView v1"
)

$ErrorActionPreference = "Stop"
if (-not (Test-Path $OutDir)) { New-Item -ItemType Directory -Path $OutDir | Out-Null }

# --- Portable path resolution (works on any machine; missing paths degrade to null, never crash) ---
function FirstExisting { param([string[]]$Paths) foreach ($p in $Paths) { if ($p -and (Test-Path $p)) { return $p } } return $null }
function FileVersionOrNull { param([string]$Path) if ($Path -and (Test-Path $Path)) { return (Get-Item $Path).VersionInfo.FileVersion } return $null }
$odApps = if ($env:OneDrive) { Join-Path $env:OneDrive 'Documents\PerfBenchApps' } else { $null }
$winuiBin  = FirstExisting @($(if ($odApps) { Join-Path $odApps 'WinUIBench' }), 'C:\Users\hik\tv-perf-winui\TableViewBench\bin\x64\Release\net8.0-windows10.0.19041.0\win-x64')
$wahmadBin = FirstExisting @($(if ($odApps) { Join-Path $odApps 'OssTableViewBench' }), 'C:\Users\hik\tv-perf-wahmad\WinUITableViewBench\bin\x64\Release\net8.0-windows10.0.19041.0\win-x64')
$wpfBin    = FirstExisting @($(if ($odApps) { Join-Path $odApps 'WpfBench' }), 'C:\Users\hik\tv-perf\app\bin\Release\net8.0-windows')
$wahmadCsprojDir = FirstExisting @('C:\Users\hik\tv-perf-wahmad\WinUITableViewBench')
# MUX TableView control source: walk up from the kit to find a repo's controls\dev\TableView, else fall back.
$tvSrcRoot = $null
$probe = $PSScriptRoot
for ($i = 0; $i -lt 8 -and $probe; $i++) {
  $cand = Join-Path $probe 'controls\dev\TableView'
  if (Test-Path $cand) { $tvSrcRoot = $cand; break }
  $probe = Split-Path $probe -Parent
}
if (-not $tvSrcRoot) { $tvSrcRoot = FirstExisting @('C:\mux-wt-colvirt-adv\controls\dev\TableView', 'C:\mux-adv\controls\dev\TableView') }

$cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
$cs = Get-CimInstance Win32_ComputerSystem
$os = Get-CimInstance Win32_OperatingSystem
$gpu = (Get-CimInstance Win32_VideoController | Where-Object { $_.Name -notmatch 'Basic|Remote|Meta' } | Select-Object -First 1).Name
$dotnet = (& dotnet --version) 2>$null
$wasdk = $null
if ($wahmadCsprojDir) {
  $wasdk = (Select-String -Path (Join-Path $wahmadCsprojDir '*.csproj') -Pattern 'WindowsAppSDK"\s+Version="([\d\.\-a-z]+)"' -ErrorAction SilentlyContinue |
    ForEach-Object { $_.Matches.Groups[1].Value } | Select-Object -First 1)
}
$tabularDll = if ($winuiBin) { FirstExisting @((Join-Path $winuiBin 'Microsoft.UI.Xaml.Controls.Tabular.dll')) } else { $null }

$env = [ordered]@{
  control_version      = $ControlVersion
  control_file_version = (FileVersionOrNull $tabularDll)
  control_binary       = $tabularDll
  cpu       = $cpu.Name.Trim()
  cores     = "$($cpu.NumberOfCores)C / $($cpu.NumberOfLogicalProcessors)T"
  ram       = "{0:N0} GB" -f ($cs.TotalPhysicalMemory / 1GB)
  gpu       = $gpu
  os        = "$($os.Caption) (build $($os.BuildNumber))"
  build     = "Release x64"
  dotnet    = $dotnet
  winappsdk = "TableView/ListView/ItemsView: in-repo advanced-poc dev build (amd64fre); WinUITableView OSS: WindowsAppSDK $wasdk"
  captured  = (Get-Date).ToString('s')
}
$env | ConvertTo-Json | Set-Content (Join-Path $OutDir 'env.json') -Encoding utf8

function DirKb($p) { if (Test-Path $p) { [math]::Round((Get-ChildItem $p -Recurse -File -ErrorAction SilentlyContinue | Measure-Object Length -Sum).Sum / 1KB, 0) } else { $null } }
function FileKb($p) { if (Test-Path $p) { [math]::Round((Get-Item $p).Length / 1KB, 0) } else { $null } }

$winuiCtl = FileKb "$winuiBin\Microsoft.UI.Xaml.Controls.dll"
$wahmadCtl = FileKb "$wahmadBin\WinUI.TableView.dll"

# MUX TableView control source weight — the achievable "how much does TableView add" proxy
# (precise linked-DLL delta needs an A/B framework build; no symbol-size tool on this box).
$tvSrc = if ($tvSrcRoot) { Get-ChildItem $tvSrcRoot -Recurse -Include *.cpp, *.h, *.idl, *.xaml -ErrorAction SilentlyContinue } else { $null }
function ExtCount($ext) { ($tvSrc | Where-Object { $_.Extension -eq $ext }).Count }

$fp = [ordered]@{}
foreach ($c in 'TableView', 'ListView', 'ItemsView') {
  $fp[$c] = [ordered]@{ framework = 'WinUI 3 (native)'; app_carried_kb = 0; framework_dll = 'Microsoft.UI.Xaml.Controls.dll'; framework_dll_kb = $winuiCtl; ship_vehicle = 'Windows App SDK runtime (shared dependency)' }
}
$fp['WinUITableView'] = [ordered]@{ framework = 'WinUI 3 (OSS)'; app_carried_kb = $wahmadCtl; framework_dll = 'WinUI.TableView.dll'; framework_dll_kb = $wahmadCtl; ship_vehicle = 'App-carried NuGet DLL' }
foreach ($c in 'DataGrid', 'ListBox') {
  $fp[$c] = [ordered]@{ framework = 'WPF (.NET)'; app_carried_kb = 0; framework_dll = 'PresentationFramework.dll'; framework_dll_kb = $null; ship_vehicle = '.NET desktop runtime (built-in)' }
}
if ($tvSrc) {
  $fp['_mux_tableview'] = [ordered]@{
    source_kb  = [math]::Round(($tvSrc | Measure-Object Length -Sum).Sum / 1KB, 0)
    source_loc = (($tvSrc | Get-Content -ErrorAction SilentlyContinue) | Measure-Object -Line).Lines
    files      = $tvSrc.Count
    cpp        = (ExtCount '.cpp'); h = (ExtCount '.h'); idl = (ExtCount '.idl'); xaml = (ExtCount '.xaml')
  }
}
$fp | ConvertTo-Json | Set-Content (Join-Path $OutDir 'footprint.json') -Encoding utf8

# Hierarchy variant benches: package version + MUXC/Tabular dll per variant.
$hierRoot = if ($env:TVPERF_HIERARCHY_APPS) { $env:TVPERF_HIERARCHY_APPS } else { 'C:\perf\hikbench' }
if (Test-Path $hierRoot) {
  $envObj = Get-Content (Join-Path $OutDir 'env.json') -Raw | ConvertFrom-Json
  $fpObj = Get-Content (Join-Path $OutDir 'footprint.json') -Raw | ConvertFrom-Json
  $variants = [ordered]@{}
  foreach ($d in Get-ChildItem $hierRoot -Directory) {
    $muxc = Join-Path $d.FullName 'Microsoft.UI.Xaml.Controls.dll'
    $tab = Join-Path $d.FullName 'Microsoft.UI.Xaml.Controls.Tabular.dll'
    $deps = Get-ChildItem $d.FullName -Filter *.deps.json | Select-Object -First 1
    $pkg = if ($deps) { ([regex]::Match((Get-Content $deps.FullName -Raw), 'Microsoft\.WindowsAppSDK\.WinUI/([^"]+)')).Groups[1].Value } else { $null }
    $variants[$d.Name] = "pkg $pkg; Tabular.dll $(FileVersionOrNull $tab)"
    $fpObj | Add-Member -NotePropertyName $d.Name -NotePropertyValue ([ordered]@{ framework = 'WinUI 3 (native)'; app_carried_kb = 0; framework_dll = 'Microsoft.UI.Xaml.Controls(.Tabular).dll'; framework_dll_kb = ((FileKb $muxc) + (FileKb $tab)); ship_vehicle = "WinUI package $pkg" }) -Force
  }
  $envObj.control_version = "TableView hierarchy variants"
  $envObj.winappsdk = ($variants.GetEnumerator() | ForEach-Object { "$($_.Key): $($_.Value)" }) -join ' | '
  $envObj | ConvertTo-Json | Set-Content (Join-Path $OutDir 'env.json') -Encoding utf8
  $fpObj | ConvertTo-Json | Set-Content (Join-Path $OutDir 'footprint.json') -Encoding utf8
}

Write-Host "wrote env.json + footprint.json -> $OutDir"
