# Builds one HierarchyBench.exe per hierarchy variant into $OutRoot\<Control>, the layout
# harness\run-all.ps1 expects (Resolve-BenchExe, override with TVPERF_HIERARCHY_APPS).
# Each variant needs its WinUI package (3.0.0-perf-*) in the repo PackageStore first.
param(
  [string[]]$Only,
  [string]$OutRoot = "C:\perf\hikbench"
)
$ErrorActionPreference = 'Continue'
$repo = Resolve-Path (Join-Path $PSScriptRoot "..\..\..")
Set-Location $repo
$variants = @(
  # Control name           WinUI package           HierarchyApi
  @('TableViewKeyBy',     '3.0.0-perf-keyby2',    'KeyBy'),
  @('TableViewCombined',  '3.0.0-perf-combined1', 'ParentKey2'),
  @('TableViewChildren3', '3.0.0-perf-children3', 'Children'))
foreach ($v in $variants) {
  if ($Only -and $Only -notcontains $v[0]) { continue }
  $log = Join-Path $env:TEMP "bench-$($v[0]).log"
  .\initrun.ps1 msb /restore /t:Rebuild Samples\HierarchyBench\HierarchyBench.csproj /p:Platform=x64 /p:Configuration=Release "/p:WinUIVersion=$($v[1])" "/p:HierarchyApi=$($v[2])" /v:m /nologo *> $log
  if (-not (Select-String -Path $log -Pattern "Build succeeded" -Quiet)) { throw "$($v[0]) build failed; see $log" }
  $exe = Get-ChildItem "BuildOutput\obj\amd64fre\Samples\HierarchyBench" -Recurse -Filter HierarchyBench.exe |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
  if (-not $exe) { throw "no HierarchyBench.exe found for $($v[0])" }
  robocopy $exe.DirectoryName (Join-Path $OutRoot $v[0]) /MIR /NFL /NDL /NJH /NJS /NP | Out-Null
  Write-Host "$($v[0]) -> $(Join-Path $OutRoot $v[0])"
}
