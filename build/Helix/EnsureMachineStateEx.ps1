Write-Host "EnsureMachineStateEx"

# Kill any running test processes
$testProcessNames = @(
    "MUXControlsTestApp",
    "IXMPTestApp",
    "NugetPackageTestApp",
    "NugetPackageTestAppCX",
    "te",
    "te.processhost",
    "AppThatUsesMUXIndirectly",
    "TextInputHost")
foreach($testProcessName in $testProcessNames)
{
    $procs = Get-Process -Name $testProcessName -ErrorAction SilentlyContinue
    foreach($proc in $procs) 
    { 
        Write-Host "Found running proc: $($proc.Name) - $($proc.Id)" 
        Write-Host "Killing Process" 
        Stop-Process $proc -Force
    }
}

