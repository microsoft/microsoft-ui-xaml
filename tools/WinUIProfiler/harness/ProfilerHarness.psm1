Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$script:HarnessRoot = $PSScriptRoot
$script:SupportedStages = @(
    "validate",
    "system-ready",
    "app-build-deploy",
    "profiler-build-deploy",
    "create-automation-test",
    "capture",
    "profile-and-apply-changes"
)

function Get-OptionalProperty {
    param(
        [AllowNull()]$InputObject,
        [Parameter(Mandatory)][string]$Name,
        $Default = $null
    )

    if ($null -eq $InputObject) {
        return $Default
    }

    $property = $InputObject.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value) {
        return $Default
    }

    return $property.Value
}

function Get-RequiredProperty {
    param(
        [AllowNull()]$InputObject,
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$Path
    )

    $value = Get-OptionalProperty -InputObject $InputObject -Name $Name
    if ($null -eq $value -or ($value -is [string] -and [string]::IsNullOrWhiteSpace($value))) {
        throw "Configuration property '$Path.$Name' is required."
    }

    return $value
}

function Resolve-HarnessPath {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$BaseDirectory
    )

    $expanded = [Environment]::ExpandEnvironmentVariables($Path)
    if ([IO.Path]::IsPathFullyQualified($expanded)) {
        return [IO.Path]::GetFullPath($expanded)
    }

    return [IO.Path]::GetFullPath((Join-Path $BaseDirectory $expanded))
}

function Resolve-HarnessAgencySource {
    [CmdletBinding()]
    param(
        [AllowNull()][string]$ConfiguredPath,
        [Parameter(Mandatory)][string]$BaseDirectory
    )

    if (-not [string]::IsNullOrWhiteSpace($ConfiguredPath)) {
        $resolved = Resolve-HarnessPath $ConfiguredPath $BaseDirectory
        $executable = if (Test-Path -LiteralPath $resolved -PathType Leaf) {
            $resolved
        }
        else {
            Join-Path $resolved "agency.exe"
        }
        if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
            throw "Configured Agency source '$resolved' does not contain agency.exe."
        }
        return Split-Path -Parent $executable
    }

    $command = Get-Command agency -CommandType Application -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($null -eq $command -or -not (Test-Path -LiteralPath $command.Source -PathType Leaf)) {
        return $null
    }
    return Split-Path -Parent $command.Source
}

function Expand-HarnessTokens {
    param(
        [AllowNull()]$Value,
        [Parameter(Mandatory)][hashtable]$Tokens
    )

    if ($null -eq $Value) {
        return $null
    }

    $expanded = [Environment]::ExpandEnvironmentVariables([string]$Value)
    foreach ($entry in $Tokens.GetEnumerator()) {
        $expanded = $expanded.Replace("{$($entry.Key)}", [string]$entry.Value, [StringComparison]::OrdinalIgnoreCase)
    }
    return $expanded
}

function ConvertTo-StringArray {
    param([AllowNull()]$Value)
    if ($null -eq $Value) {
        return @()
    }
    if ($Value -is [string]) {
        return @([string]$Value)
    }
    return @($Value | ForEach-Object { [string]$_ })
}

function Write-HarnessJson {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)]$Value
    )

    $directory = Split-Path -Parent $Path
    if ($directory) {
        [IO.Directory]::CreateDirectory($directory) | Out-Null
    }
    $json = $Value | ConvertTo-Json -Depth 100
    [IO.File]::WriteAllText($Path, $json, [Text.UTF8Encoding]::new($false))
}

function Read-HarnessJson {
    param([Parameter(Mandatory)][string]$Path)
    return Get-Content -LiteralPath $Path -Raw -Encoding UTF8 | ConvertFrom-Json -Depth 100
}

function Test-HarnessCommandDefinition {
    param(
        [AllowNull()]$Definition,
        [Parameter(Mandatory)][string]$Path
    )

    if ($null -eq $Definition) {
        return
    }

    Get-RequiredProperty $Definition "file" $Path | Out-Null
    $arguments = Get-OptionalProperty $Definition "arguments" @()
    if ($arguments -is [string]) {
        throw "Configuration property '$Path.arguments' must be an array."
    }
}

function Test-ProfilerHarnessConfiguration {
    [CmdletBinding()]
    param([Parameter(Mandatory)]$Configuration)

    if ([int](Get-RequiredProperty $Configuration "schemaVersion" "config") -ne 1) {
        throw "Only profiler harness schemaVersion 1 is supported."
    }

    Get-RequiredProperty $Configuration "name" "config" | Out-Null
    $repositories = Get-RequiredProperty $Configuration "repositories" "config"
    $app = Get-RequiredProperty $repositories "app" "config.repositories"
    $profiler = Get-RequiredProperty $repositories "profiler" "config.repositories"
    foreach ($entry in @(@("app", $app), @("profiler", $profiler))) {
        $role = $entry[0]
        $repo = $entry[1]
        Get-RequiredProperty $repo "path" "config.repositories.$role" | Out-Null
        $branch = [string](Get-RequiredProperty $repo "branch" "config.repositories.$role")
        if ($branch -notmatch '^user/[^/]+/profiler_harness$') {
            throw "config.repositories.$role.branch must match user/<alias>/profiler_harness."
        }
    }

    $target = Get-RequiredProperty $Configuration "target" "config"
    $kind = [string](Get-RequiredProperty $target "kind" "config.target")
    if ($kind -notin @("local", "hyperv", "ixp")) {
        throw "config.target.kind must be local, hyperv, or ixp."
    }
    Get-RequiredProperty $target "stageRoot" "config.target" | Out-Null
    if ($kind -eq "hyperv") {
        $hyperv = Get-RequiredProperty $target "hyperv" "config.target"
        Get-RequiredProperty $hyperv "name" "config.target.hyperv" | Out-Null
        Get-RequiredProperty $hyperv "userNameEnvironmentVariable" "config.target.hyperv" | Out-Null
        Get-RequiredProperty $hyperv "passwordEnvironmentVariable" "config.target.hyperv" | Out-Null
    }
    if ($kind -eq "ixp") {
        $ixp = Get-RequiredProperty $target "ixp" "config.target"
        Get-RequiredProperty $ixp "name" "config.target.ixp" | Out-Null
        Get-RequiredProperty $ixp "branch" "config.target.ixp" | Out-Null
        $connectionMode = [string](Get-OptionalProperty $ixp "connectionMode" "auto")
        if ($connectionMode -notin @("auto", "background", "psremote")) {
            throw "config.target.ixp.connectionMode must be auto, background, or psremote."
        }
        $minimumOsBuild = Get-OptionalProperty $ixp "minimumOsBuild"
        if ($null -ne $minimumOsBuild -and [int]$minimumOsBuild -lt 1) {
            throw "config.target.ixp.minimumOsBuild must be positive."
        }
    }

    $appConfig = Get-RequiredProperty $Configuration "application" "config"
    $launch = Get-RequiredProperty $appConfig "launch" "config.application"
    $launchKind = [string](Get-RequiredProperty $launch "kind" "config.application.launch")
    if ($launchKind -notin @("packaged", "unpackaged", "attach")) {
        throw "config.application.launch.kind must be packaged, unpackaged, or attach."
    }
    Get-RequiredProperty $launch "target" "config.application.launch" | Out-Null

    $profiling = Get-RequiredProperty $Configuration "profiling" "config"
    $attempts = [int](Get-OptionalProperty (Get-OptionalProperty $Configuration "optimization") "maxAttempts" 5)
    if ($attempts -lt 1 -or $attempts -gt 5) {
        throw "config.optimization.maxAttempts must be between 1 and 5."
    }
    $budget = [double](Get-OptionalProperty $profiling "frameBudgetMilliseconds" 16.7)
    if ($budget -le 0) {
        throw "config.profiling.frameBudgetMilliseconds must be positive."
    }

    foreach ($command in @(Get-OptionalProperty $appConfig "buildCommands" @())) {
        Test-HarnessCommandDefinition $command "config.application.buildCommands"
    }
    $deploy = Get-OptionalProperty $appConfig "deployCommands"
    if ($null -ne $deploy) {
        foreach ($provider in @("local", "hyperv", "ixp")) {
            foreach ($command in @(Get-OptionalProperty $deploy $provider @())) {
                Test-HarnessCommandDefinition $command "config.application.deployCommands.$provider"
            }
        }
    }
    foreach ($command in @(Get-OptionalProperty $appConfig "cleanupCommands" @())) {
        Test-HarnessCommandDefinition $command "config.application.cleanupCommands"
    }
    $targetInstallScripts = Get-OptionalProperty $appConfig "targetInstallScripts"
    if ($null -ne $targetInstallScripts) {
        foreach ($provider in @("hyperv", "ixp")) {
            $script = Get-OptionalProperty $targetInstallScripts $provider
            if ($null -ne $script -and $script -isnot [string]) {
                throw "config.application.targetInstallScripts.$provider must be a PowerShell script string."
            }
        }
    }

    return $true
}

function Read-ProfilerHarnessConfiguration {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Path)

    $fullPath = [IO.Path]::GetFullPath($Path)
    if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
        throw "Harness configuration '$fullPath' does not exist."
    }

    $config = Read-HarnessJson $fullPath
    Test-ProfilerHarnessConfiguration $config | Out-Null
    $config | Add-Member -NotePropertyName "__configPath" -NotePropertyValue $fullPath -Force
    $config | Add-Member -NotePropertyName "__configDirectory" -NotePropertyValue (Split-Path -Parent $fullPath) -Force
    return $config
}

function Invoke-HarnessProcess {
    param(
        [Parameter(Mandatory)][string]$File,
        [string[]]$Arguments = @(),
        [string]$WorkingDirectory,
        [hashtable]$Environment = @{},
        [int]$TimeoutSeconds = 3600,
        [string]$LogBasePath,
        [switch]$DryRun,
        [switch]$AllowFailure
    )

    if ($DryRun) {
        return [pscustomobject]@{
            file = $File
            arguments = $Arguments
            workingDirectory = $WorkingDirectory
            exitCode = 0
            stdout = ""
            stderr = ""
            dryRun = $true
        }
    }

    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName = $File
    $start.UseShellExecute = $false
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    $start.CreateNoWindow = $true
    if ($WorkingDirectory) {
        $start.WorkingDirectory = $WorkingDirectory
    }
    foreach ($argument in $Arguments) {
        $start.ArgumentList.Add($argument)
    }
    foreach ($entry in $Environment.GetEnumerator()) {
        if ($null -eq $entry.Value) {
            $start.Environment.Remove($entry.Key) | Out-Null
        }
        else {
            $start.Environment[$entry.Key] = [string]$entry.Value
        }
    }

    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $start
    $stdout = ""
    $stderr = ""
    $exitCode = $null
    $timedOut = $false
    try {
        if (-not $process.Start()) {
            throw "Failed to start '$File'."
        }

        $stdoutTask = $process.StandardOutput.ReadToEndAsync()
        $stderrTask = $process.StandardError.ReadToEndAsync()
        if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
            $timedOut = $true
            try { $process.Kill($true) } catch { }
            if (-not $process.WaitForExit(30000)) {
                throw "Failed to stop timed-out command '$File'."
            }
        }
        $process.WaitForExit()
        $stdout = $stdoutTask.GetAwaiter().GetResult()
        $stderr = $stderrTask.GetAwaiter().GetResult()
        $exitCode = $process.ExitCode
    }
    finally {
        $process.Dispose()
    }

    if ($LogBasePath) {
        [IO.Directory]::CreateDirectory((Split-Path -Parent $LogBasePath)) | Out-Null
        [IO.File]::WriteAllText("$LogBasePath.stdout.log", $stdout, [Text.UTF8Encoding]::new($false))
        [IO.File]::WriteAllText("$LogBasePath.stderr.log", $stderr, [Text.UTF8Encoding]::new($false))
    }

    $result = [pscustomobject]@{
        file = $File
        arguments = $Arguments
        workingDirectory = $WorkingDirectory
        exitCode = $exitCode
        stdout = $stdout
        stderr = $stderr
        dryRun = $false
        timedOut = $timedOut
    }
    if ($timedOut) {
        $logMessage = if ($LogBasePath) {
            " Output was saved to '$LogBasePath.stdout.log' and '$LogBasePath.stderr.log'."
        }
        else { "" }
        throw "Command '$File' exceeded its $TimeoutSeconds second timeout.$logMessage"
    }
    if ($exitCode -ne 0 -and -not $AllowFailure) {
        $message = if ($stderr.Trim()) { $stderr.Trim() } else { $stdout.Trim() }
        throw "Command '$File' failed with exit code $exitCode. $message"
    }
    return $result
}

function Resolve-HarnessDotNet {
    param(
        [Parameter(Mandatory)][string]$RequiredSdkVersion,
        [switch]$DryRun
    )

    $candidates = [Collections.Generic.List[string]]::new()
    foreach ($root in @($env:ProgramW6432, $env:ProgramFiles, $env:DOTNET_ROOT)) {
        if (-not [string]::IsNullOrWhiteSpace($root)) {
            $candidates.Add((Join-Path $root "dotnet\dotnet.exe"))
            $candidates.Add((Join-Path $root "dotnet.exe"))
        }
    }
    $command = Get-Command "dotnet" -ErrorAction SilentlyContinue
    if ($null -ne $command -and -not [string]::IsNullOrWhiteSpace($command.Source)) {
        $candidates.Add($command.Source)
    }

    $seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $existing = @(
        $candidates |
            Where-Object {
                -not [string]::IsNullOrWhiteSpace($_) -and
                $seen.Add($_) -and
                (Test-Path -LiteralPath $_ -PathType Leaf)
            }
    )
    if ($DryRun) {
        if ($existing.Count -gt 0) {
            return $existing[0]
        }
        return "dotnet"
    }

    $sdkPattern = "(?m)^$([Regex]::Escape($RequiredSdkVersion))\s+\["
    foreach ($candidate in $existing) {
        $result = Invoke-HarnessProcess -File $candidate -Arguments @("--list-sdks") `
            -TimeoutSeconds 30 -AllowFailure
        if ($result.exitCode -eq 0 -and $result.stdout -match $sdkPattern) {
            return $candidate
        }
    }

    throw "The profiler requires .NET SDK $RequiredSdkVersion, but no dotnet installation containing it was found."
}

function Invoke-HarnessCommand {
    param(
        [Parameter(Mandatory)]$Definition,
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$Name,
        [switch]$AllowFailure
    )

    $file = Expand-HarnessTokens (Get-RequiredProperty $Definition "file" $Name) $Context.Tokens
    $arguments = @(
        ConvertTo-StringArray (Get-OptionalProperty $Definition "arguments" @()) |
            ForEach-Object { Expand-HarnessTokens $_ $Context.Tokens }
    )
    $workingDirectory = Expand-HarnessTokens (Get-OptionalProperty $Definition "workingDirectory" $Context.Tokens.appRepo) $Context.Tokens
    $timeout = [int](Get-OptionalProperty $Definition "timeoutSeconds" 3600)
    $environment = @{}
    $environmentObject = Get-OptionalProperty $Definition "environment"
    if ($null -ne $environmentObject) {
        foreach ($property in $environmentObject.PSObject.Properties) {
            $environment[$property.Name] = Expand-HarnessTokens $property.Value $Context.Tokens
        }
    }
    $safeName = $Name -replace '[^A-Za-z0-9_.-]', '-'
    return Invoke-HarnessProcess `
        -File $file `
        -Arguments $arguments `
        -WorkingDirectory $workingDirectory `
        -Environment $environment `
        -TimeoutSeconds $timeout `
        -LogBasePath (Join-Path $Context.RunDirectory "logs\$safeName") `
        -DryRun:$Context.DryRun `
        -AllowFailure:$AllowFailure
}

function Invoke-Git {
    param(
        [Parameter(Mandatory)][string]$Repository,
        [Parameter(Mandatory)][string[]]$Arguments,
        [Parameter(Mandatory)]$Context,
        [string]$Name = "git",
        [switch]$AllowFailure
    )

    return Invoke-HarnessProcess `
        -File "git" `
        -Arguments (@("-C", $Repository) + $Arguments) `
        -WorkingDirectory $Repository `
        -TimeoutSeconds 600 `
        -LogBasePath (Join-Path $Context.RunDirectory "logs\$Name") `
        -DryRun:$Context.DryRun `
        -AllowFailure:$AllowFailure
}

function Initialize-HarnessRepository {
    param(
        [Parameter(Mandatory)]$Definition,
        [Parameter(Mandatory)][string]$Role,
        [Parameter(Mandatory)]$Context
    )

    $path = $Context.Tokens["${Role}Repo"]
    if (-not (Test-Path -LiteralPath $path -PathType Container)) {
        $remote = [string](Get-RequiredProperty $Definition "remoteUrl" "config.repositories.$Role")
        if ($Context.DryRun) {
            return
        }
        [IO.Directory]::CreateDirectory((Split-Path -Parent $path)) | Out-Null
        Invoke-HarnessProcess -File "git" -Arguments @("clone", $remote, $path) -TimeoutSeconds 3600 `
            -LogBasePath (Join-Path $Context.RunDirectory "logs\clone-$Role") | Out-Null
    }

    if (-not (Test-Path -LiteralPath (Join-Path $path ".git"))) {
        $inside = Invoke-HarnessProcess -File "git" -Arguments @("-C", $path, "rev-parse", "--is-inside-work-tree") `
            -AllowFailure -TimeoutSeconds 60 -DryRun:$Context.DryRun
        if (-not $Context.DryRun -and ($inside.exitCode -ne 0 -or $inside.stdout.Trim() -ne "true")) {
            throw "'$path' is not a Git worktree."
        }
    }

    $allowDirty = [bool](Get-OptionalProperty $Definition "allowDirty" $false)
    if (-not $Context.DryRun -and -not $allowDirty) {
        $status = Invoke-Git $path @("status", "--porcelain", "--untracked-files=no") $Context "status-$Role"
        if ($status.stdout.Trim()) {
            throw "Repository '$path' has tracked changes. Commit, stash, or set allowDirty explicitly before branch preparation."
        }
    }

    $desired = [string](Get-RequiredProperty $Definition "branch" "config.repositories.$Role")
    if ($Context.DryRun) {
        return
    }
    $current = Invoke-Git $path @("symbolic-ref", "--short", "-q", "HEAD") $Context "branch-$Role" -AllowFailure
    if ($current.exitCode -eq 0 -and $current.stdout.Trim() -eq $desired) {
        return
    }

    $local = Invoke-Git $path @("show-ref", "--verify", "--quiet", "refs/heads/$desired") $Context "local-branch-$Role" -AllowFailure
    if ($local.exitCode -eq 0) {
        Invoke-Git $path @("switch", $desired) $Context "switch-$Role" | Out-Null
        return
    }

    $remote = Invoke-Git $path @("show-ref", "--verify", "--quiet", "refs/remotes/origin/$desired") $Context "remote-branch-$Role" -AllowFailure
    if ($remote.exitCode -eq 0) {
        Invoke-Git $path @("switch", "--track", "-c", $desired, "origin/$desired") $Context "track-$Role" | Out-Null
        return
    }

    $baseRef = [string](Get-OptionalProperty $Definition "baseRef" "HEAD")
    Invoke-Git $path @("switch", "-c", $desired, $baseRef) $Context "create-$Role" | Out-Null
}

function Get-HyperVCredential {
    param([Parameter(Mandatory)]$Context)

    $hyperv = Get-RequiredProperty $Context.Config.target "hyperv" "config.target"
    $userVariable = [string](Get-RequiredProperty $hyperv "userNameEnvironmentVariable" "config.target.hyperv")
    $passwordVariable = [string](Get-RequiredProperty $hyperv "passwordEnvironmentVariable" "config.target.hyperv")
    $userName = [Environment]::GetEnvironmentVariable($userVariable)
    $password = [Environment]::GetEnvironmentVariable($passwordVariable)
    if ([string]::IsNullOrWhiteSpace($userName) -or [string]::IsNullOrWhiteSpace($password)) {
        throw "Hyper-V credentials must be supplied through environment variables '$userVariable' and '$passwordVariable'."
    }
    $secure = ConvertTo-SecureString $password -AsPlainText -Force
    return [PSCredential]::new($userName, $secure)
}

function New-HarnessIxpDirectSession {
    param([Parameter(Mandatory)]$Context)

    $name = [string](Get-RequiredProperty $Context.Config.target.ixp "name" "config.target.ixp")
    $userName = [string](Get-IxpDeviceLocalAdminName -ForDevice $name)
    $password = Get-IxpDeviceLocalAdminPassword -ForDevice $name -AsSecureString
    if ([string]::IsNullOrWhiteSpace($userName) -or $null -eq $password) {
        throw "Ixp credentials for '$name' could not be resolved for PowerShell Direct transfer."
    }
    return New-PSSession -VMName $name -Credential ([PSCredential]::new($userName, $password))
}

function Test-HarnessIxpUsesPowerShellDirect {
    param([Parameter(Mandatory)]$Context)

    $connectionMode = [string](Get-OptionalProperty $Context.Config.target.ixp "connectionMode" "auto")
    return $connectionMode -eq "psremote" -or
        ($connectionMode -eq "auto" -and (Test-DeviceConnected -PSRemote))
}

function Connect-HarnessIxpDevice {
    param([Parameter(Mandatory)]$Context)

    Import-Module IXPTools -Global -ErrorAction Stop
    $settings = $Context.Config.target.ixp
    $name = [string](Get-RequiredProperty $settings "name" "config.target.ixp")
    $connectionMode = [string](Get-OptionalProperty $settings "connectionMode" "auto")
    $connectArguments = @{
        Name = $name
        Quiet = $true
        SkipTShellUpdate = $true
    }
    switch ($connectionMode) {
        "psremote" {
            connectd @connectArguments -PSRemote | Out-Null
        }
        "background" {
            connectd @connectArguments -Background | Out-Null
        }
        "auto" {
            try {
                connectd @connectArguments -PSRemote | Out-Null
            }
            catch {
                connectd @connectArguments -Background | Out-Null
            }
        }
    }
    $probe = cmdx -PassThru cmd /c "echo IXP_READY"
    if ($probe.ExitCode -ne 0 -or $probe.StdOut -notmatch "IXP_READY") {
        throw "Ixp device '$name' did not pass its liveness probe."
    }
}

function Get-HarnessIxpCreationParameters {
    param(
        [Parameter(Mandatory)]$Settings,
        [Parameter(Mandatory)][string]$ConfigDirectory
    )

    $parameters = @{
        Name = [string]$Settings.name
        Branch = [string]$Settings.branch
        Latest = $true
        Wait = $true
        Force = $true
        NoUI = $true
        Flavor = [string](Get-OptionalProperty $Settings "flavor" "Desktop")
        Arch = [string](Get-OptionalProperty $Settings "arch" "amd64")
    }
    $savePath = Get-OptionalProperty $Settings "savePath"
    if ($savePath) { $parameters.SavePath = Resolve-HarnessPath ([string]$savePath) $ConfigDirectory }
    $switchName = Get-OptionalProperty $Settings "switchName"
    if ($switchName) { $parameters.VirtualSwitchName = [string]$switchName }
    $memory = Get-OptionalProperty $Settings "memoryInGb"
    if ($null -ne $memory) { $parameters.VmMemInGb = [double]$memory }
    $processors = Get-OptionalProperty $Settings "processorCount"
    if ($null -ne $processors) { $parameters.VmNumProcessors = [int]$processors }
    $tools = @(ConvertTo-StringArray (Get-OptionalProperty $Settings "tools" @()))
    if ($tools.Count -gt 0) { $parameters.Tool = $tools }
    $apps = @(ConvertTo-StringArray (Get-OptionalProperty $Settings "winGetApps" @()))
    if ($apps.Count -gt 0) { $parameters.WinGetApps = $apps }
    if ([bool](Get-OptionalProperty $Settings "allowOffline" $false)) {
        $parameters.AllowOffline = $true
    }
    if ([bool](Get-OptionalProperty $Settings "enableCdKeys" $false)) {
        $parameters.EnableCDKeys = $true
    }
    $velocityKeysToEnable = @(ConvertTo-StringArray (Get-OptionalProperty $Settings "velocityKeysToEnable" @()))
    if ($velocityKeysToEnable.Count -gt 0) {
        $parameters.VelocityKeysToEnable = $velocityKeysToEnable
    }
    $velocityKeysToDisable = @(ConvertTo-StringArray (Get-OptionalProperty $Settings "velocityKeysToDisable" @()))
    if ($velocityKeysToDisable.Count -gt 0) {
        $parameters.VelocityKeysToDisable = $velocityKeysToDisable
    }
    return $parameters
}

function Initialize-HarnessTarget {
    param([Parameter(Mandatory)]$Context)

    $kind = [string]$Context.Config.target.kind
    if ($kind -eq "local" -or $Context.DryRun) {
        return
    }

    if ($kind -eq "hyperv") {
        Import-Module Hyper-V -ErrorAction Stop
        $settings = $Context.Config.target.hyperv
        $name = [string]$settings.name
        $vm = Get-VM -Name $name -ErrorAction SilentlyContinue
        if ($null -eq $vm) {
            $sourceVhd = Resolve-HarnessPath ([string](Get-RequiredProperty $settings "sourceVhdPath" "config.target.hyperv")) $Context.Config.__configDirectory
            $vmVhd = Resolve-HarnessPath ([string](Get-RequiredProperty $settings "vmVhdPath" "config.target.hyperv")) $Context.Config.__configDirectory
            if (-not (Test-Path -LiteralPath $sourceVhd -PathType Leaf)) {
                throw "Hyper-V source VHD '$sourceVhd' does not exist."
            }
            [IO.Directory]::CreateDirectory((Split-Path -Parent $vmVhd)) | Out-Null
            Copy-Item -LiteralPath $sourceVhd -Destination $vmVhd
            $memory = [int64](Get-OptionalProperty $settings "memoryStartupBytes" 8589934592)
            $generation = [int](Get-OptionalProperty $settings "generation" 2)
            $switchName = [string](Get-OptionalProperty $settings "switchName" "")
            $parameters = @{
                Name = $name
                Generation = $generation
                VHDPath = $vmVhd
                MemoryStartupBytes = $memory
            }
            if ($switchName) {
                $parameters.SwitchName = $switchName
            }
            $vm = New-VM @parameters
            Set-VMProcessor -VM $vm -Count ([int](Get-OptionalProperty $settings "processorCount" 4))
        }
        if ($vm.State -ne "Running") {
            Start-VM -VM $vm | Out-Null
        }
        $credential = Get-HyperVCredential $Context
        $deadline = [DateTime]::UtcNow.AddMinutes(5)
        do {
            try {
                $session = New-PSSession -VMName $name -Credential $credential -ErrorAction Stop
                Remove-PSSession $session
                return
            }
            catch {
                Start-Sleep -Seconds 5
            }
        } while ([DateTime]::UtcNow -lt $deadline)
        throw "Hyper-V VM '$name' did not become available through PowerShell Direct."
    }

    if ($kind -eq "ixp") {
        Import-Module IXPTools -Global -ErrorAction Stop
        $settings = $Context.Config.target.ixp
        $name = [string]$settings.name
        $device = Get-IxpDevice -Name $name -ErrorAction SilentlyContinue
        if ($null -eq $device) {
            if (-not [bool](Get-OptionalProperty $settings "ensureCreated" $true)) {
                throw "Ixp device '$name' does not exist and config.target.ixp.ensureCreated is false."
            }
            $parameters = Get-HarnessIxpCreationParameters $settings $Context.Config.__configDirectory
            New-TestMachine @parameters | Out-Null
        }
        Connect-HarnessIxpDevice $Context
        $minimumOsBuild = Get-OptionalProperty $settings "minimumOsBuild"
        if ($null -ne $minimumOsBuild) {
            $buildProbe = Invoke-HarnessTargetPowerShell $Context `
                'Write-Output ("HARNESS_OS_BUILD=" + (Get-CimInstance Win32_OperatingSystem).BuildNumber)' 60
            if ($buildProbe.exitCode -ne 0) {
                throw "Could not determine the OS build for Ixp device '$name'. $($buildProbe.stderr)"
            }
            $match = [regex]::Match([string]$buildProbe.stdout, 'HARNESS_OS_BUILD=(\d+)')
            if (-not $match.Success) {
                throw "Could not parse the OS build for Ixp device '$name' from '$($buildProbe.stdout)'."
            }
            $actualOsBuild = [int]$match.Groups[1].Value
            if ($actualOsBuild -lt [int]$minimumOsBuild) {
                throw "Ixp device '$name' is Windows build $actualOsBuild; this configuration requires build $minimumOsBuild or newer."
            }
        }
        return
    }
}

function Invoke-HarnessTargetPowerShell {
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$Script,
        [int]$TimeoutSeconds = 1800,
        [switch]$AsUser
    )

    if ($Context.DryRun) {
        return [pscustomobject]@{ exitCode = 0; stdout = ""; stderr = ""; dryRun = $true }
    }

    $kind = [string]$Context.Config.target.kind
    $encoded = [Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($Script))
    if ($kind -eq "local") {
        return Invoke-HarnessProcess -File "pwsh" -Arguments @("-NoLogo", "-NoProfile", "-EncodedCommand", $encoded) `
            -TimeoutSeconds $TimeoutSeconds
    }
    if ($kind -eq "hyperv") {
        $credential = Get-HyperVCredential $Context
        $session = New-PSSession -VMName ([string]$Context.Config.target.hyperv.name) -Credential $credential
        try {
            if ($AsUser) {
                throw "Interactive Hyper-V execution uses Invoke-HarnessCapture, not the generic PowerShell runner."
            }
            $result = Invoke-Command -Session $session -ScriptBlock {
                param($Payload)
                $bytes = [Convert]::FromBase64String($Payload)
                $text = [Text.Encoding]::Unicode.GetString($bytes)
                & ([ScriptBlock]::Create($text))
            } -ArgumentList $encoded
            return [pscustomobject]@{ exitCode = 0; stdout = ($result | Out-String); stderr = ""; dryRun = $false }
        }
        finally {
            Remove-PSSession $session
        }
    }
    if ($kind -eq "ixp") {
        Connect-HarnessIxpDevice $Context
        if ($AsUser) {
            $result = execd -AsUser -FileName "powershell.exe" `
                -Arguments @("-NoProfile", "-EncodedCommand", $encoded) `
                -Timeout $TimeoutSeconds -HideOutput
            return [pscustomobject]@{
                exitCode = [int]$result.ExitCode
                stdout = [string]$result.StdOut
                stderr = [string]$result.StdErr
                dryRun = $false
            }
        }
        $result = cmdx -PassThru -Timeout $TimeoutSeconds powershell.exe -NoProfile -EncodedCommand $encoded
        return [pscustomobject]@{
            exitCode = [int]$result.ExitCode
            stdout = [string]$result.StdOut
            stderr = [string]$result.StdErr
            dryRun = $false
        }
    }
}

function Copy-HarnessDirectoryToTarget {
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$Source,
        [Parameter(Mandatory)][string]$Destination,
        [Parameter(Mandatory)][string]$Name
    )

    if ($Context.DryRun -or [string]$Context.Config.target.kind -eq "local") {
        return
    }
    if (-not (Test-Path -LiteralPath $Source -PathType Container)) {
        throw "Deployment source '$Source' does not exist."
    }

    $archive = Join-Path $Context.RunDirectory "$Name.zip"
    if (Test-Path -LiteralPath $archive) {
        Remove-Item -LiteralPath $archive -Force
    }
    Compress-Archive -Path (Join-Path $Source "*") -DestinationPath $archive -CompressionLevel Fastest
    $targetArchive = "$Destination.zip"
    $kind = [string]$Context.Config.target.kind
    if ($kind -eq "hyperv") {
        $credential = Get-HyperVCredential $Context
        $session = New-PSSession -VMName ([string]$Context.Config.target.hyperv.name) -Credential $credential
        try {
            Invoke-Command -Session $session -ScriptBlock {
                param($Path)
                New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Path) | Out-Null
            } -ArgumentList $targetArchive
            Copy-Item -LiteralPath $archive -Destination $targetArchive -ToSession $session -Force
        }
        finally {
            Remove-PSSession $session
        }
    }
    elseif ($kind -eq "ixp") {
        Connect-HarnessIxpDevice $Context
        if (Test-HarnessIxpUsesPowerShellDirect $Context) {
            $session = New-HarnessIxpDirectSession $Context
            try {
                Invoke-Command -Session $session -ScriptBlock {
                    param($Path)
                    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Path) | Out-Null
                } -ArgumentList $targetArchive
                Copy-Item -LiteralPath $archive -Destination $targetArchive -ToSession $session -Force
            }
            finally {
                Remove-PSSession $session
            }
        }
        else {
            $parent = Split-Path -Parent $targetArchive
            cmdx cmd /c "mkdir `"$parent`" 2>nul" | Out-Null
            putd $archive $parent
            $uploadedArchive = Join-Path $parent (Split-Path -Leaf $archive)
            if (-not $uploadedArchive.Equals($targetArchive, [StringComparison]::OrdinalIgnoreCase)) {
                cmdx cmd /c "move /y `"$uploadedArchive`" `"$targetArchive`"" | Out-Null
            }
        }
    }
    $script = @"
`$ErrorActionPreference = 'Stop'
if (Test-Path -LiteralPath '$Destination') { Remove-Item -LiteralPath '$Destination' -Recurse -Force }
New-Item -ItemType Directory -Force -Path '$Destination' | Out-Null
Expand-Archive -LiteralPath '$targetArchive' -DestinationPath '$Destination' -Force
Remove-Item -LiteralPath '$targetArchive' -Force
"@
    $result = Invoke-HarnessTargetPowerShell $Context $script 1800
    if ($result.exitCode -ne 0) {
        throw "Deploying '$Name' to the target failed. $($result.stderr)"
    }
}

function Copy-HarnessDirectoryFromTarget {
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$Source,
        [Parameter(Mandatory)][string]$Destination,
        [Parameter(Mandatory)][string]$Name
    )

    if ([string]$Context.Config.target.kind -eq "local") {
        return
    }
    [IO.Directory]::CreateDirectory($Destination) | Out-Null
    $targetArchive = "$Source.zip"
    $script = @"
`$ErrorActionPreference = 'Stop'
if (Test-Path -LiteralPath '$targetArchive') { Remove-Item -LiteralPath '$targetArchive' -Force }
Compress-Archive -Path '$Source\*' -DestinationPath '$targetArchive' -CompressionLevel Fastest
"@
    $result = Invoke-HarnessTargetPowerShell $Context $script 1800
    if ($result.exitCode -ne 0) {
        throw "Packaging target results failed. $($result.stderr)"
    }
    $localArchive = Join-Path $Context.RunDirectory "$Name-results.zip"
    $kind = [string]$Context.Config.target.kind
    if ($kind -eq "hyperv") {
        $credential = Get-HyperVCredential $Context
        $session = New-PSSession -VMName ([string]$Context.Config.target.hyperv.name) -Credential $credential
        try {
            Copy-Item -LiteralPath $targetArchive -Destination $localArchive -FromSession $session -Force
        }
        finally {
            Remove-PSSession $session
        }
    }
    elseif ($kind -eq "ixp") {
        Connect-HarnessIxpDevice $Context
        if (Test-HarnessIxpUsesPowerShellDirect $Context) {
            $session = New-HarnessIxpDirectSession $Context
            try {
                Copy-Item -LiteralPath $targetArchive -Destination $localArchive -FromSession $session -Force
            }
            finally {
                Remove-PSSession $session
            }
        }
        else {
            $localParent = Split-Path -Parent $localArchive
            getd $targetArchive $localParent
            $downloadedArchive = Join-Path $localParent (Split-Path -Leaf $targetArchive)
            if (-not $downloadedArchive.Equals($localArchive, [StringComparison]::OrdinalIgnoreCase)) {
                Move-Item -LiteralPath $downloadedArchive -Destination $localArchive -Force
            }
        }
    }
    Expand-Archive -LiteralPath $localArchive -DestinationPath $Destination -Force
}

function Ensure-HarnessAgents {
    param([Parameter(Mandatory)]$Context)

    $portableAgencyRoot = ""
    if ([string]$Context.Config.target.kind -ne "local") {
        $agents = Get-RequiredProperty $Context.Config "agents" "config"
        $agencySource = Resolve-HarnessAgencySource `
            -ConfiguredPath ([string](Get-OptionalProperty $agents "agencySourcePath" "")) `
            -BaseDirectory ([string]$Context.Config.__configDirectory)
        if ($null -ne $agencySource) {
            $portableAgencyRoot = Join-Path $Context.Tokens.targetAgentRoot "agency"
            Copy-HarnessDirectoryToTarget $Context $agencySource $portableAgencyRoot "agency"
        }
    }

    $portableAgencyLiteral = "'$($portableAgencyRoot.Replace("'", "''"))'"
    $script = "`$portableAgencyRoot = $portableAgencyLiteral`r`n" + @'
$ErrorActionPreference = "Stop"
function Refresh-Path {
    $env:Path = [Environment]::GetEnvironmentVariable("Path", "Machine") + ";" +
        [Environment]::GetEnvironmentVariable("Path", "User")
}
if ($portableAgencyRoot) {
    $agencyExecutable = Join-Path $portableAgencyRoot "agency.exe"
    if (-not (Test-Path -LiteralPath $agencyExecutable -PathType Leaf)) {
        throw "Portable Agency deployment is missing '$agencyExecutable'."
    }
    $userPath = [Environment]::GetEnvironmentVariable("Path", "User")
    if (@($userPath -split ";" | Where-Object { $_ }) -notcontains $portableAgencyRoot) {
        $updatedUserPath = if ([string]::IsNullOrWhiteSpace($userPath)) {
            $portableAgencyRoot
        }
        else {
            "$portableAgencyRoot;$userPath"
        }
        [Environment]::SetEnvironmentVariable("Path", $updatedUserPath, "User")
    }
}
Refresh-Path
if (-not (Get-Command agency -ErrorAction SilentlyContinue)) {
    iex "& { $(irm https://aka.ms/InstallTool.ps1) } agency"
    Refresh-Path
}
if (-not (Get-Command agency -ErrorAction SilentlyContinue)) {
    throw "Agency installation completed but agency is not on PATH. Restart the target session and rerun system-ready."
}
if (-not (Get-Command copilot -ErrorAction SilentlyContinue)) {
    if (-not (Get-Command npm -ErrorAction SilentlyContinue)) {
        $winget = Get-Command winget -ErrorAction SilentlyContinue
        if ($null -eq $winget) {
            throw "Neither npm nor WinGet is available. Install Node.js LTS, then rerun system-ready."
        }
        & $winget.Source install --id OpenJS.NodeJS.LTS --exact --silent `
            --accept-package-agreements --accept-source-agreements --disable-interactivity
        Refresh-Path
        if (-not (Get-Command npm -ErrorAction SilentlyContinue)) {
            throw "WinGet did not make npm available after installing OpenJS.NodeJS.LTS."
        }
    }
    npm install --global @github/copilot
    if ($LASTEXITCODE -ne 0) { throw "npm failed to install @github/copilot." }
    Refresh-Path
    $npmPrefix = (& npm prefix --global).Trim()
    if ($LASTEXITCODE -eq 0 -and $npmPrefix -and $env:Path -notlike "*$npmPrefix*") {
        $env:Path = "$npmPrefix;$env:Path"
    }
}
if (-not (Get-Command copilot -ErrorAction SilentlyContinue)) {
    throw "Standalone GitHub Copilot CLI installation completed but copilot is not on PATH."
}
agency --version
if ($LASTEXITCODE -ne 0) { throw "agency --version failed." }
copilot --version
if ($LASTEXITCODE -ne 0) { throw "copilot --version failed." }
'@
    $asUser = [string]$Context.Config.target.kind -eq "ixp"
    $result = Invoke-HarnessTargetPowerShell $Context $script 1800 -AsUser:$asUser
    if ($result.exitCode -ne 0) {
        throw "Agent readiness failed. $($result.stderr) $($result.stdout)"
    }
    [IO.File]::WriteAllText((Join-Path $Context.RunDirectory "agents.txt"), $result.stdout, [Text.UTF8Encoding]::new($false))
}

function Get-HarnessDeploymentStatePath {
    param([Parameter(Mandatory)]$Context)
    $stateRoot = Join-Path $Context.ArtifactsRoot "state"
    [IO.Directory]::CreateDirectory($stateRoot) | Out-Null
    $targetName = switch ([string]$Context.Config.target.kind) {
        "local" { "local" }
        "hyperv" { [string]$Context.Config.target.hyperv.name }
        "ixp" { [string]$Context.Config.target.ixp.name }
    }
    $safe = "$($Context.Config.name)-$targetName" -replace '[^A-Za-z0-9_.-]', '-'
    return Join-Path $stateRoot "$safe.json"
}

function Save-HarnessDeploymentState {
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)]$State
    )
    Write-HarnessJson (Get-HarnessDeploymentStatePath $Context) $State
}

function Get-HarnessDeploymentState {
    param([Parameter(Mandatory)]$Context)
    $path = Get-HarnessDeploymentStatePath $Context
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "No deployment state exists for this configuration and target. Run profiler-build-deploy first."
    }
    return Read-HarnessJson $path
}

function Invoke-AppBuildDeploy {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [switch]$SkipBuild
    )

    $application = $Context.Config.application
    if (-not $SkipBuild) {
        $index = 0
        foreach ($command in @(Get-OptionalProperty $application "buildCommands" @())) {
            Invoke-HarnessCommand $command $Context "app-build-$index" | Out-Null
            $index++
        }
    }
    $deployCommands = Get-OptionalProperty $application "deployCommands"
    if ($null -ne $deployCommands) {
        $providerCommands = @(Get-OptionalProperty $deployCommands ([string]$Context.Config.target.kind) @())
        $index = 0
        foreach ($command in $providerCommands) {
            Invoke-HarnessCommand $command $Context "app-deploy-$index" | Out-Null
            $index++
        }
    }

    $payloadPath = Get-OptionalProperty $application "payloadPath"
    if ($payloadPath -and [string]$Context.Config.target.kind -ne "local") {
        $resolved = Expand-HarnessTokens $payloadPath $Context.Tokens
        Copy-HarnessDirectoryToTarget $Context $resolved $Context.Tokens.targetAppRoot "app"
        $installScripts = Get-OptionalProperty $application "targetInstallScripts"
        $installScript = Get-OptionalProperty $installScripts ([string]$Context.Config.target.kind)
        if ($installScript) {
            $expandedScript = Expand-HarnessTokens ([string]$installScript) $Context.Tokens
            $asUser = [string]$Context.Config.target.kind -eq "ixp"
            $result = Invoke-HarnessTargetPowerShell $Context $expandedScript 1800 -AsUser:$asUser
            if ($result.exitCode -ne 0) {
                throw "Target-side app installation failed. $($result.stderr) $($result.stdout)"
            }
        }
    }
}

function Invoke-AppCleanup {
    param([Parameter(Mandatory)]$Context)

    $commands = @(Get-OptionalProperty $Context.Config.application "cleanupCommands" @())
    $index = 0
    foreach ($command in $commands) {
        Invoke-HarnessCommand $command $Context "app-cleanup-$index" -AllowFailure | Out-Null
        $index++
    }
}

function Invoke-ProfilerBuildDeploy {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [switch]$SkipBuild
    )

    $profiler = Get-RequiredProperty $Context.Config "profiler" "config"
    $configuration = [string](Get-OptionalProperty $profiler "configuration" "Release")
    $platform = [string](Get-OptionalProperty $profiler "platform" "x64")
    $profilerRepo = $Context.Tokens.profilerRepo
    $profilerRoot = Join-Path $profilerRepo "tools\WinUIProfiler"
    $buildScript = Join-Path $profilerRoot "scripts\Build-App.ps1"
    $driverProject = Join-Path $script:HarnessRoot "Driver\ProfilerHarness.Driver.csproj"
    $driverOutput = Join-Path $Context.ArtifactsRoot "driver\$configuration\win-$($platform.ToLowerInvariant())"
    if (-not $SkipBuild) {
        $profilerGlobalJson = Read-HarnessJson (Join-Path $profilerRoot "global.json")
        $requiredSdkVersion = [string](Get-RequiredProperty $profilerGlobalJson.sdk "version" "profiler global.json sdk")
        $dotnet = Resolve-HarnessDotNet -RequiredSdkVersion $requiredSdkVersion -DryRun:$Context.DryRun
        Invoke-HarnessProcess -File "pwsh" `
            -Arguments @("-NoLogo", "-NoProfile", "-File", $buildScript, "-Configuration", $configuration, "-Platform", $platform) `
            -WorkingDirectory $profilerRepo `
            -TimeoutSeconds ([int](Get-OptionalProperty $profiler "buildTimeoutSeconds" 7200)) `
            -LogBasePath (Join-Path $Context.RunDirectory "logs\profiler-build") `
            -DryRun:$Context.DryRun | Out-Null
        Invoke-HarnessProcess -File $dotnet `
            -Arguments @(
                "publish", $driverProject,
                "--configuration", $configuration,
                "--runtime", "win-$($platform.ToLowerInvariant())",
                "--self-contained", "true",
                "--output", $driverOutput,
                "-p:PublishSingleFile=false"
            ) `
            -WorkingDirectory $profilerRepo `
            -TimeoutSeconds 3600 `
            -LogBasePath (Join-Path $Context.RunDirectory "logs\driver-publish") `
            -DryRun:$Context.DryRun | Out-Null
    }

    $outputMarker = Join-Path $profilerRoot "WinUIProfiler.App\obj\ProfilerAppOutput.$platform.$configuration.txt"
    if ($Context.DryRun) {
        $profilerOutput = Join-Path $Context.ArtifactsRoot "dry-run\profiler"
    }
    else {
        if (-not (Test-Path -LiteralPath $outputMarker -PathType Leaf)) {
            throw "Profiler output marker '$outputMarker' does not exist. Run without -SkipBuild."
        }
        $profilerOutput = (Get-Content -LiteralPath $outputMarker -Raw -Encoding UTF8).Trim()
        if (-not (Test-Path -LiteralPath $profilerOutput -PathType Container)) {
            throw "Profiler output '$profilerOutput' from '$outputMarker' does not exist."
        }
        if (-not (Test-Path -LiteralPath $driverOutput -PathType Container)) {
            throw "Harness driver output '$driverOutput' does not exist."
        }
    }

    $targetKind = [string]$Context.Config.target.kind
    if ($targetKind -eq "local") {
        $targetProfiler = $profilerOutput
        $targetDriver = $driverOutput
    }
    else {
        Copy-HarnessDirectoryToTarget $Context $profilerOutput $Context.Tokens.targetProfilerRoot "profiler"
        Copy-HarnessDirectoryToTarget $Context $driverOutput $Context.Tokens.targetDriverRoot "driver"
        $targetProfiler = $Context.Tokens.targetProfilerRoot
        $targetDriver = $Context.Tokens.targetDriverRoot
    }

    $state = [pscustomobject]@{
        schemaVersion = 1
        generatedUtc = [DateTime]::UtcNow.ToString("O")
        targetKind = $targetKind
        profilerHostOutput = $profilerOutput
        driverHostOutput = $driverOutput
        targetProfilerRoot = $targetProfiler
        targetDriverRoot = $targetDriver
        payloadDirectory = Join-Path $targetProfiler "ConnectionPayloads"
        driverExecutable = Join-Path $targetDriver "ProfilerHarness.Driver.exe"
    }
    if (-not $Context.DryRun) {
        Save-HarnessDeploymentState $Context $state
    }
    return $state
}

function Test-AutomationScenario {
    [CmdletBinding()]
    param([Parameter(Mandatory)]$Scenario)

    if ([int](Get-RequiredProperty $Scenario "schemaVersion" "scenario") -ne 1) {
        throw "Only automation scenario schemaVersion 1 is supported."
    }
    Get-RequiredProperty $Scenario "name" "scenario" | Out-Null
    $steps = @(Get-RequiredProperty $Scenario "steps" "scenario")
    if ($steps.Count -lt 1 -or $steps.Count -gt 200) {
        throw "scenario.steps must contain between 1 and 200 steps."
    }
    $actions = @(
        "waitForWindow", "waitForElement", "invoke", "click", "setText", "sendKeys",
        "select", "toggle", "wait", "assertExists", "assertNotExists"
    )
    for ($i = 0; $i -lt $steps.Count; $i++) {
        $action = [string](Get-RequiredProperty $steps[$i] "action" "scenario.steps[$i]")
        if ($action -notin $actions) {
            throw "scenario.steps[$i].action '$action' is not supported."
        }
        if ($action -eq "wait") {
            $milliseconds = [int](Get-RequiredProperty $steps[$i] "milliseconds" "scenario.steps[$i]")
            if ($milliseconds -lt 0 -or $milliseconds -gt 300000) {
                throw "scenario.steps[$i].milliseconds must be between 0 and 300000."
            }
        }
        elseif ($action -ne "sendKeys" -or $null -ne (Get-OptionalProperty $steps[$i] "selector")) {
            Get-RequiredProperty $steps[$i] "selector" "scenario.steps[$i]" | Out-Null
        }
    }
    return $true
}

function Get-JsonObjectFromText {
    param([Parameter(Mandatory)][string]$Text)

    try {
        return $Text | ConvertFrom-Json -Depth 100
    }
    catch {
        $start = $Text.IndexOf('{')
        $end = $Text.LastIndexOf('}')
        if ($start -lt 0 -or $end -le $start) {
            throw "Agent response did not contain a JSON object."
        }
        return $Text.Substring($start, $end - $start + 1) | ConvertFrom-Json -Depth 100
    }
}

function Invoke-HarnessAgent {
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][ValidateSet("agency", "github")][string]$Engine,
        [Parameter(Mandatory)][string]$Prompt,
        [Parameter(Mandatory)][string]$WorkingDirectory,
        [Parameter(Mandatory)][string]$Name,
        [switch]$Editing,
        [switch]$OnTarget
    )

    $common = @(
        "-p", $Prompt,
        "-s",
        "--output-format", "text",
        "--allow-all-tools",
        "--deny-tool", "task_complete",
        "--disable-builtin-mcps",
        "--no-custom-instructions",
        "--no-ask-user",
        "--no-color",
        "-C", $WorkingDirectory
    )
    if ($Engine -eq "github") {
        $common = @("--trust-folder") + $common
    }
    $file = if ($Engine -eq "agency") { "agency" } else { "copilot" }
    $sessionId = if ($Engine -eq "agency") { [Guid]::NewGuid().ToString() } else { $null }
    $arguments = if ($Engine -eq "agency") {
        @(
            "copilot",
            "--session-id", $sessionId,
            "--no-aec",
            "--no-default-mcps",
            "--no-config-plugins"
        ) + $common
    }
    else { $common }
    if ($OnTarget -and [string]$Context.Config.target.kind -ne "local") {
        $promptBase64 = [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes($Prompt))
        $workingBase64 = [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes($WorkingDirectory))
        $remoteArguments = @($arguments)
        for ($index = 0; $index -lt $remoteArguments.Count; $index++) {
            if ($remoteArguments[$index] -eq $Prompt) { $remoteArguments[$index] = "__HARNESS_PROMPT__" }
            if ($remoteArguments[$index] -eq $WorkingDirectory) { $remoteArguments[$index] = "__HARNESS_WORKING_DIRECTORY__" }
        }
        $argumentJson = $remoteArguments | ConvertTo-Json -Compress
        $argumentsBase64 = [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes($argumentJson))
        $agencyEnvironmentScript = if ($Engine -eq "agency") {
@"
foreach (`$name in @(
    'AGENCY_ENGINE',
    'AGENCY_LOG_SESSION_DIR',
    'AGENCY_OPERATION_ID',
    'AGENCY_REPO_DIR',
    'AGENCY_SESSION_ID',
    'AGENCY_SESSION_SUBPROCESS',
    'COPILOT_AGENT_SESSION_ID',
    'COPILOT_CLI_BINARY_VERSION',
    'COPILOT_CLI_RESOLVED_DIST_DIR',
    'COPILOT_CUSTOM_INSTRUCTIONS_DIRS',
    'COPILOT_LOADER_PID'
)) {
    Remove-Item -LiteralPath "Env:`$name" -ErrorAction SilentlyContinue
}
`$env:COPILOT_HOME = Join-Path ([IO.Path]::GetTempPath()) 'WinUIProfilerHarness\copilot-home-$sessionId'
New-Item -ItemType Directory -Force -Path `$env:COPILOT_HOME | Out-Null
"@
        }
        else { "" }
        $script = @"
`$ErrorActionPreference = 'Stop'
`$prompt = [Text.Encoding]::UTF8.GetString([Convert]::FromBase64String('$promptBase64'))
`$workingDirectory = [Text.Encoding]::UTF8.GetString([Convert]::FromBase64String('$workingBase64'))
New-Item -ItemType Directory -Force -Path `$workingDirectory | Out-Null
`$argumentJson = [Text.Encoding]::UTF8.GetString([Convert]::FromBase64String('$argumentsBase64'))
`$arguments = @(`$argumentJson | ConvertFrom-Json)
for (`$index = 0; `$index -lt `$arguments.Count; `$index++) {
    if (`$arguments[`$index] -eq '__HARNESS_PROMPT__') { `$arguments[`$index] = `$prompt }
    if (`$arguments[`$index] -eq '__HARNESS_WORKING_DIRECTORY__') { `$arguments[`$index] = `$workingDirectory }
}
$agencyEnvironmentScript
`$output = & '$file' @arguments 2>&1 | Out-String
`$exitCode = `$LASTEXITCODE
[Console]::Out.Write(`$output)
if (`$exitCode -ne 0) { throw "Agent '$Engine' failed with exit code `$exitCode." }
"@
        $asUser = [string]$Context.Config.target.kind -eq "ixp"
        $result = Invoke-HarnessTargetPowerShell $Context $script 1800 -AsUser:$asUser
        if ($result.exitCode -ne 0) {
            throw "Target agent '$Engine' failed. $($result.stderr) $($result.stdout)"
        }
        return $result
    }

    $environment = @{
        GIT_CONFIG_COUNT = "1"
        GIT_CONFIG_KEY_0 = "remote.origin.pushurl"
        GIT_CONFIG_VALUE_0 = "DISABLED_BY_PROFILER_HARNESS"
    }
    if ($Engine -eq "agency") {
        foreach ($environmentName in @(
            "AGENCY_ENGINE",
            "AGENCY_LOG_SESSION_DIR",
            "AGENCY_OPERATION_ID",
            "AGENCY_REPO_DIR",
            "AGENCY_SESSION_ID",
            "AGENCY_SESSION_SUBPROCESS",
            "COPILOT_AGENT_SESSION_ID",
            "COPILOT_CLI_BINARY_VERSION",
            "COPILOT_CLI_RESOLVED_DIST_DIR",
            "COPILOT_CUSTOM_INSTRUCTIONS_DIRS",
            "COPILOT_LOADER_PID"
        )) {
            $environment[$environmentName] = $null
        }
        $environment["COPILOT_HOME"] = Join-Path $Context.RunDirectory "copilot-home-$sessionId"
        New-Item -ItemType Directory -Force -Path $environment["COPILOT_HOME"] | Out-Null
    }
    $processFile = $file
    $processArguments = $arguments
    if ($Engine -eq "github" -and -not $Context.DryRun) {
        $copilotCommand = Get-Command "copilot" -ErrorAction Stop
        if ($copilotCommand.CommandType -eq [Management.Automation.CommandTypes]::ExternalScript) {
            $processFile = (Get-Command "pwsh" -CommandType Application -ErrorAction Stop).Source
            $processArguments = @("-NoLogo", "-NoProfile", "-File", $copilotCommand.Source) + $arguments
        }
        elseif ($copilotCommand.CommandType -eq [Management.Automation.CommandTypes]::Application) {
            $processFile = $copilotCommand.Source
        }
        else {
            throw "Unsupported GitHub Copilot command type '$($copilotCommand.CommandType)'."
        }
    }
    return Invoke-HarnessProcess -File $processFile -Arguments $processArguments -WorkingDirectory $WorkingDirectory `
        -Environment $environment -TimeoutSeconds 1800 `
        -LogBasePath (Join-Path $Context.RunDirectory "logs\agent-$Name") -DryRun:$Context.DryRun
}

function Invoke-CreateAutomationTest {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$Prompt,
        [string]$OutputPath,
        [ValidateSet("agency", "github")][string]$Engine
    )

    if (-not $Engine) {
        $Engine = [string](Get-OptionalProperty $Context.Config.agents "default" "agency")
    }
    if (-not $OutputPath) {
        $OutputPath = Join-Path $Context.RunDirectory "automation-scenario.json"
    }
    elseif (-not [IO.Path]::IsPathFullyQualified($OutputPath)) {
        $OutputPath = Resolve-HarnessPath $OutputPath $Context.Config.__configDirectory
    }

    $schemaPath = Join-Path $script:HarnessRoot "automation.schema.json"
    $schema = Get-Content -LiteralPath $schemaPath -Raw -Encoding UTF8
    $agentPrompt = @"
Convert the following Windows desktop UI scenario into one automation JSON document.
Return JSON only. Do not use tools, edit files, add commentary, or invent unsupported actions.
Prefer stable AutomationId selectors. If unavailable, use Name plus ControlType and a match mode.
The scenario runs against the profiled process, so do not add process launch steps.

Scenario:
$Prompt

Schema:
$schema
"@
    $agentWorkingDirectory = if ([string]$Context.Config.target.kind -eq "local") {
        $script:HarnessRoot
    }
    else {
        $Context.Tokens.targetRunRoot
    }
    $result = Invoke-HarnessAgent $Context $Engine $agentPrompt $agentWorkingDirectory "create-automation" -OnTarget
    if ($Context.DryRun) {
        return $OutputPath
    }
    $scenario = Get-JsonObjectFromText $result.stdout
    Test-AutomationScenario $scenario | Out-Null
    Write-HarnessJson $OutputPath $scenario
    return $OutputPath
}

function New-CaptureRequest {
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)]$Deployment,
        [Parameter(Mandatory)][string]$ScenarioPath,
        [Parameter(Mandatory)][string]$RequestPath,
        [Parameter(Mandatory)][string]$ResultPath,
        [Parameter(Mandatory)][string]$SessionDirectory,
        [Parameter(Mandatory)][string]$SnapshotDirectory
    )

    $launch = $Context.Config.application.launch
    $profiling = $Context.Config.profiling
    $request = [pscustomobject]@{
        schemaVersion = 1
        launch = [pscustomobject]@{
            kind = [string]$launch.kind
            target = Expand-HarnessTokens ([string]$launch.target) $Context.Tokens
            arguments = Expand-HarnessTokens ([string](Get-OptionalProperty $launch "arguments" "")) $Context.Tokens
            workingDirectory = Expand-HarnessTokens (Get-OptionalProperty $launch "workingDirectory") $Context.Tokens
        }
        payloadDirectory = [string]$Deployment.payloadDirectory
        sessionDirectory = $SessionDirectory
        snapshotDirectory = $SnapshotDirectory
        scenarioPath = $ScenarioPath
        resultPath = $ResultPath
        firstFrameTimeoutSeconds = [int](Get-OptionalProperty $profiling "firstFrameTimeoutSeconds" 60)
        scenarioTimeoutSeconds = [int](Get-OptionalProperty $profiling "scenarioTimeoutSeconds" 180)
        settleMilliseconds = [int](Get-OptionalProperty $profiling "settleMilliseconds" 2000)
        frameBudgetMilliseconds = [double](Get-OptionalProperty $profiling "frameBudgetMilliseconds" 16.7)
    }
    Write-HarnessJson $RequestPath $request
    return $request
}

function Invoke-HyperVCapture {
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$Driver,
        [Parameter(Mandatory)][string]$RequestPath,
        [Parameter(Mandatory)][string]$ResultPath,
        [Parameter(Mandatory)][int]$TimeoutSeconds
    )

    $credential = Get-HyperVCredential $Context
    $session = New-PSSession -VMName ([string]$Context.Config.target.hyperv.name) -Credential $credential
    $taskName = "ProfilerHarness-$([Guid]::NewGuid().ToString('N'))"
    try {
        $userName = $credential.UserName
        $argument = "capture --request `"$RequestPath`""
        Invoke-Command -Session $session -ScriptBlock {
            param($Name, $Executable, $Arguments, $UserName)
            $action = New-ScheduledTaskAction -Execute $Executable -Argument $Arguments
            $principal = New-ScheduledTaskPrincipal -UserId $UserName -LogonType Interactive -RunLevel Highest
            Register-ScheduledTask -TaskName $Name -Action $action -Principal $principal -Force | Out-Null
            Start-ScheduledTask -TaskName $Name
        } -ArgumentList $taskName, $Driver, $argument, $userName
        $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
        do {
            $exists = Invoke-Command -Session $session -ScriptBlock { param($Path) Test-Path -LiteralPath $Path } -ArgumentList $ResultPath
            if ($exists) { return }
            Start-Sleep -Seconds 2
        } while ([DateTime]::UtcNow -lt $deadline)
        throw "Interactive capture task '$taskName' did not produce '$ResultPath' within $TimeoutSeconds seconds. Ensure the VM user is logged on."
    }
    finally {
        try {
            Invoke-Command -Session $session -ScriptBlock {
                param($Name)
                Unregister-ScheduledTask -TaskName $Name -Confirm:$false -ErrorAction SilentlyContinue
            } -ArgumentList $taskName
        }
        catch { }
        Remove-PSSession $session
    }
}

function Resolve-HarnessCaptureResult {
    param(
        [Parameter(Mandatory)][string]$ResultPath,
        [bool]$DryRun,
        $ExecutionFailure
    )

    if (-not $DryRun -and -not (Test-Path -LiteralPath $ResultPath -PathType Leaf)) {
        if ($null -ne $ExecutionFailure) {
            throw $ExecutionFailure
        }
        throw "The profiler driver did not produce '$ResultPath'."
    }

    if (-not $DryRun) {
        $result = Read-HarnessJson $ResultPath
        $status = [string](Get-OptionalProperty $result "status" "")
        if ($status -ne "succeeded") {
            $structuredError = [string](Get-OptionalProperty $result "error" "No structured error was reported.")
            $executionError = if ($null -eq $ExecutionFailure) {
                ""
            }
            else {
                $executionMessage = if ($ExecutionFailure -is [Management.Automation.ErrorRecord]) {
                    $ExecutionFailure.Exception.Message
                }
                elseif ($ExecutionFailure -is [Exception]) {
                    $ExecutionFailure.Message
                }
                else {
                    [string]$ExecutionFailure
                }
                " Driver error: $executionMessage"
            }
            throw "Profiler capture failed with status '$status'. Evidence was preserved at '$ResultPath'. Error: $structuredError$executionError"
        }
    }

    return $ResultPath
}

function Invoke-HarnessCapture {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$ScenarioPath,
        [string]$Name = "capture"
    )

    $scenarioFullPath = Resolve-HarnessPath $ScenarioPath $Context.Config.__configDirectory
    $scenario = Read-HarnessJson $scenarioFullPath
    Test-AutomationScenario $scenario | Out-Null
    $deployment = Get-HarnessDeploymentState $Context
    $captureRoot = Join-Path $Context.RunDirectory $Name
    [IO.Directory]::CreateDirectory($captureRoot) | Out-Null

    $kind = [string]$Context.Config.target.kind
    if ($kind -eq "local") {
        $requestPath = Join-Path $captureRoot "capture-request.json"
        $resultPath = Join-Path $captureRoot "capture-result.json"
        $sessionDirectory = Join-Path $captureRoot "session"
        $snapshotDirectory = Join-Path $captureRoot "snapshots"
        New-CaptureRequest $Context $deployment $scenarioFullPath $requestPath $resultPath $sessionDirectory $snapshotDirectory | Out-Null
        $timeout = [int](Get-OptionalProperty $Context.Config.profiling "firstFrameTimeoutSeconds" 60) +
            [int](Get-OptionalProperty $Context.Config.profiling "scenarioTimeoutSeconds" 180) + 180
        $executionFailure = $null
        try {
            Invoke-HarnessProcess -File ([string]$deployment.driverExecutable) `
                -Arguments @("capture", "--request", $requestPath) `
                -WorkingDirectory ([string]$deployment.targetDriverRoot) `
                -TimeoutSeconds $timeout `
                -LogBasePath (Join-Path $captureRoot "driver") `
                -DryRun:$Context.DryRun | Out-Null
        }
        catch {
            $executionFailure = $_
        }
        return Resolve-HarnessCaptureResult $resultPath $Context.DryRun $executionFailure
    }

    $targetCaptureRoot = Join-Path $Context.Tokens.targetRunRoot $Name
    $targetScenario = Join-Path $targetCaptureRoot "scenario.json"
    $targetRequest = Join-Path $targetCaptureRoot "capture-request.json"
    $targetResult = Join-Path $targetCaptureRoot "capture-result.json"
    $targetSession = Join-Path $targetCaptureRoot "session"
    $targetSnapshots = Join-Path $targetCaptureRoot "snapshots"
    $requestStaging = Join-Path $captureRoot "request"
    [IO.Directory]::CreateDirectory($requestStaging) | Out-Null
    Copy-Item -LiteralPath $scenarioFullPath -Destination (Join-Path $requestStaging "scenario.json")
    New-CaptureRequest $Context $deployment $targetScenario (Join-Path $requestStaging "capture-request.json") `
        $targetResult $targetSession $targetSnapshots | Out-Null
    Copy-HarnessDirectoryToTarget $Context $requestStaging $targetCaptureRoot "$Name-request"

    $timeout = [int](Get-OptionalProperty $Context.Config.profiling "firstFrameTimeoutSeconds" 60) +
        [int](Get-OptionalProperty $Context.Config.profiling "scenarioTimeoutSeconds" 180) + 180
    $executionFailure = $null
    try {
        if ($kind -eq "hyperv") {
            Invoke-HyperVCapture $Context ([string]$deployment.driverExecutable) $targetRequest $targetResult $timeout
        }
        elseif ($kind -eq "ixp") {
            Connect-HarnessIxpDevice $Context
            $result = execd -AsUser -FileName ([string]$deployment.driverExecutable) `
                -Arguments @("capture", "--request", $targetRequest) -Timeout $timeout -HideOutput
            if ([int]$result.ExitCode -ne 0) {
                throw "Ixp capture failed with exit code $($result.ExitCode). $($result.StdErr)"
            }
        }
    }
    catch {
        $executionFailure = $_
    }

    Copy-HarnessDirectoryFromTarget $Context $targetCaptureRoot $captureRoot $Name
    $localResult = Join-Path $captureRoot "capture-result.json"
    return Resolve-HarnessCaptureResult $localResult $Context.DryRun $executionFailure
}

function Get-PrimaryMetricValue {
    param(
        [Parameter(Mandatory)]$Capture,
        [Parameter(Mandatory)][string]$Metric
    )
    $property = $Capture.metrics.PSObject.Properties[$Metric]
    if ($null -eq $property -or $null -eq $property.Value) {
        throw "Capture result does not contain metric '$Metric'."
    }
    return [double]$property.Value
}

function Test-HarnessCaptureValidity {
    param([Parameter(Mandatory)]$Capture)

    $validity = Get-OptionalProperty $Capture "captureValidity"
    $metrics = Get-OptionalProperty $Capture "metrics"
    return [string](Get-OptionalProperty $Capture "status" "") -eq "succeeded" -and
        [bool](Get-OptionalProperty $validity "etwAvailable" $false) -and
        [int](Get-OptionalProperty $validity "lostEvents" 0) -eq 0 -and
        [int64](Get-OptionalProperty $validity "droppedEvents" 0) -eq 0 -and
        [string]::IsNullOrWhiteSpace([string](Get-OptionalProperty $validity "etwError" "")) -and
        [string]::IsNullOrWhiteSpace([string](Get-OptionalProperty $validity "recordingError" "")) -and
        [int](Get-OptionalProperty $metrics "drawnFrames" 0) -gt 0
}

function Test-HarnessImprovement {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Baseline,
        [Parameter(Mandatory)]$Candidate,
        [Parameter(Mandatory)]$Optimization
    )

    $metric = [string](Get-OptionalProperty $Optimization "primaryMetric" "p95DrawMilliseconds")
    $direction = [string](Get-OptionalProperty $Optimization "direction" "lower")
    $minimum = [double](Get-OptionalProperty $Optimization "minimumImprovementPercent" 5)
    $baselineValue = Get-PrimaryMetricValue $Baseline $metric
    $candidateValue = Get-PrimaryMetricValue $Candidate $metric
    if ($baselineValue -eq 0) {
        $percent = if ($candidateValue -eq 0) { 0 } else { -100 }
    }
    elseif ($direction -eq "higher") {
        $percent = (($candidateValue - $baselineValue) / [Math]::Abs($baselineValue)) * 100
    }
    else {
        $percent = (($baselineValue - $candidateValue) / [Math]::Abs($baselineValue)) * 100
    }

    $baselineValid = Test-HarnessCaptureValidity $Baseline
    $candidateValid = Test-HarnessCaptureValidity $Candidate
    $valid = $baselineValid -and $candidateValid
    return [pscustomobject]@{
        metric = $metric
        direction = $direction
        baseline = $baselineValue
        candidate = $candidateValue
        improvementPercent = $percent
        minimumImprovementPercent = $minimum
        baselineCaptureValid = $baselineValid
        candidateCaptureValid = $candidateValid
        captureValid = $valid
        improved = $valid -and $percent -ge $minimum
    }
}

function New-HarnessAttemptWorktree {
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][int]$Attempt
    )

    $repo = $Context.Tokens.appRepo
    $root = Join-Path $script:HarnessRoot ".worktrees"
    [IO.Directory]::CreateDirectory($root) | Out-Null
    $path = Join-Path $root "$($Context.Config.name)-$($Context.RunId)-$Attempt"
    $branch = "profiler-harness/$($Context.RunId)/attempt-$Attempt"
    Invoke-Git $repo @("worktree", "add", "-b", $branch, $path, "HEAD") $Context "worktree-$Attempt" | Out-Null
    return [pscustomobject]@{ path = $path; branch = $branch }
}

function Remove-HarnessAttemptWorktree {
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)]$Worktree
    )
    if ($Context.DryRun) { return }
    Invoke-Git $Context.Tokens.appRepo @("worktree", "remove", "--force", $Worktree.path) $Context "remove-worktree" -AllowFailure | Out-Null
    Invoke-Git $Context.Tokens.appRepo @("branch", "-D", $Worktree.branch) $Context "remove-attempt-branch" -AllowFailure | Out-Null
}

function Get-FileSha256 {
    param([Parameter(Mandatory)][string]$Path)
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Add-HarnessPatchSignature {
    param(
        [Parameter(Mandatory)][AllowEmptyCollection()][Collections.Generic.HashSet[string]]$Signatures,
        [Parameter(Mandatory)][string]$PatchPath
    )
    return $Signatures.Add((Get-FileSha256 $PatchPath))
}

function Invoke-HarnessAnalysis {
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$CapturePath,
        [Parameter(Mandatory)][string]$CaptureName,
        [Parameter(Mandatory)][ValidateSet("agency", "github")][string]$Engine
    )

    $analysisPath = Join-Path $Context.RunDirectory "$CaptureName-analysis.md"
    if ([string]$Context.Config.target.kind -eq "local") {
        $agentCapturePath = $CapturePath
        $workingDirectory = Split-Path -Parent $CapturePath
    }
    else {
        $workingDirectory = Join-Path $Context.Tokens.targetRunRoot $CaptureName
        $agentCapturePath = Join-Path $workingDirectory "capture-result.json"
    }
    $prompt = @"
Analyze this WinUI Profiler capture and return an actionable Markdown report only:
$agentCapturePath

Read the before/after snapshot JSON paths recorded in capture-result.json. Use frame timings,
event counts, XAML/composition node deltas, and correlation data. Rank distinct improvement
hypotheses by evidence and expected impact. For each hypothesis, cite the concrete evidence,
identify likely source areas to inspect, and state what metric should improve. Do not edit files,
run the scenario, or claim causality that the evidence does not establish.
"@
    $result = Invoke-HarnessAgent $Context $Engine $prompt $workingDirectory "$CaptureName-analysis" -OnTarget
    if (-not $Context.DryRun -and [string]::IsNullOrWhiteSpace($result.stdout)) {
        throw "The '$Engine' analysis session returned no report."
    }
    [IO.File]::WriteAllText($analysisPath, [string]$result.stdout, [Text.UTF8Encoding]::new($false))
    return $analysisPath
}

function Invoke-ProfileAndApplyChanges {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$ScenarioPath,
        [ValidateSet("agency", "github")][string]$Engine
    )

    if (-not $Engine) {
        $Engine = [string](Get-OptionalProperty $Context.Config.agents "default" "agency")
    }
    $optimization = Get-RequiredProperty $Context.Config "optimization" "config"
    $maxAttempts = [int](Get-OptionalProperty $optimization "maxAttempts" 5)
    $maxProposalRounds = $maxAttempts * 3
    $baselinePath = Invoke-HarnessCapture $Context $ScenarioPath "baseline"
    if ($Context.DryRun) {
        return
    }
    $baseline = Read-HarnessJson $baselinePath
    if (-not (Test-HarnessCaptureValidity $baseline)) {
        throw "The baseline capture is not valid for optimization. Inspect '$baselinePath' for ETW, recording, or frame evidence errors."
    }
    $analysisPath = Invoke-HarnessAnalysis $Context $baselinePath "baseline" $Engine
    $attemptRecords = [Collections.Generic.List[object]]::new()
    $signatures = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $proposalRound = 0
    $attempt = 0

    while ($attempt -lt $maxAttempts -and $proposalRound -lt $maxProposalRounds) {
        $proposalRound++
        $worktree = New-HarnessAttemptWorktree $Context ($attempt + 1)
        $attemptContext = $null
        try {
            $history = if ($attemptRecords.Count -eq 0) { "None." } else {
                ($attemptRecords | ConvertTo-Json -Depth 20)
            }
            $evidenceRoot = Join-Path $worktree.path ".profiler-harness-evidence"
            [IO.Directory]::CreateDirectory($evidenceRoot) | Out-Null
            Copy-Item -LiteralPath $baselinePath -Destination (Join-Path $evidenceRoot "capture-result.json")
            Copy-Item -LiteralPath $analysisPath -Destination (Join-Path $evidenceRoot "analysis.md")
            $prompt = @"
You are applying one focused WinUI performance improvement in an isolated Git worktree.
Read these profiler evidence files inside the worktree:
.profiler-harness-evidence\capture-result.json
.profiler-harness-evidence\analysis.md

Previous distinct attempts:
$history

Make exactly one coherent source change that is supported by the evidence and differs from prior attempts.
Do not commit, push, change branches, modify generated binaries, or edit outside this worktree.
Preserve behavior. Update a focused test only when the change introduces testable behavior.
At the end, summarize the hypothesis and files changed.
"@
            $agentResult = Invoke-HarnessAgent $Context $Engine $prompt $worktree.path "attempt-$proposalRound" -Editing
            [IO.File]::WriteAllText((Join-Path $Context.RunDirectory "attempt-$proposalRound-agent.md"), $agentResult.stdout,
                [Text.UTF8Encoding]::new($false))
            Remove-Item -LiteralPath $evidenceRoot -Recurse -Force
            Invoke-HarnessProcess -File "git" -Arguments @("-C", $worktree.path, "add", "-N", "--", ".") `
                -WorkingDirectory $worktree.path -TimeoutSeconds 300 -AllowFailure | Out-Null
            $diffPath = Join-Path $Context.RunDirectory "attempt-$proposalRound.patch"
            $diff = Invoke-HarnessProcess -File "git" -Arguments @("-C", $worktree.path, "diff", "--binary", "--no-ext-diff") `
                -WorkingDirectory $worktree.path -TimeoutSeconds 300
            [IO.File]::WriteAllText($diffPath, $diff.stdout, [Text.UTF8Encoding]::new($false))
            if (-not $diff.stdout.Trim()) {
                $attemptRecords.Add([pscustomobject]@{
                    attempt = $null
                    proposalRound = $proposalRound
                    status = "rejected-empty"
                    summary = $agentResult.stdout
                })
                continue
            }
            $signature = Get-FileSha256 $diffPath
            if (-not (Add-HarnessPatchSignature $signatures $diffPath)) {
                $attemptRecords.Add([pscustomobject]@{
                    attempt = $null
                    proposalRound = $proposalRound
                    status = "rejected-duplicate"
                    signature = $signature
                    summary = $agentResult.stdout
                })
                continue
            }

            $attempt++
            $attemptContext = New-HarnessContext $Context.Config $Context.DryRun $Engine
            $attemptContext.Tokens.appRepo = $worktree.path
            try {
                Invoke-AppBuildDeploy $attemptContext
                $candidatePath = Invoke-HarnessCapture $attemptContext $ScenarioPath "candidate-$attempt"
                $candidate = Read-HarnessJson $candidatePath
                $evaluation = Test-HarnessImprovement $baseline $candidate $optimization
                $record = [pscustomobject]@{
                    attempt = $attempt
                    proposalRound = $proposalRound
                    status = if ($evaluation.improved) { "verified-improvement" } else { "no-improvement" }
                    signature = $signature
                    patchPath = $diffPath
                    captureResult = $candidatePath
                    evaluation = $evaluation
                    summary = $agentResult.stdout
                }
            }
            catch {
                $record = [pscustomobject]@{
                    attempt = $attempt
                    proposalRound = $proposalRound
                    status = "validation-failed"
                    signature = $signature
                    patchPath = $diffPath
                    captureResult = $null
                    evaluation = $null
                    error = $_.Exception.ToString()
                    summary = $agentResult.stdout
                }
            }
            $attemptRecords.Add($record)
            Write-HarnessJson (Join-Path $Context.RunDirectory "attempts.json") $attemptRecords
            if ($record.status -eq "verified-improvement") {
                Invoke-HarnessProcess -File "git" -Arguments @("-C", $Context.Tokens.appRepo, "apply", "--check", $diffPath) `
                    -WorkingDirectory $Context.Tokens.appRepo -TimeoutSeconds 300 | Out-Null
                Invoke-HarnessProcess -File "git" -Arguments @("-C", $Context.Tokens.appRepo, "apply", $diffPath) `
                    -WorkingDirectory $Context.Tokens.appRepo -TimeoutSeconds 300 | Out-Null
                Write-HarnessJson (Join-Path $Context.RunDirectory "successful-attempt.json") $record
                return
            }
        }
        finally {
            if ($null -ne $attemptContext) {
                Invoke-AppCleanup $attemptContext
            }
            Remove-HarnessAttemptWorktree $Context $worktree
        }
    }

    $restorationError = $null
    try {
        Invoke-AppBuildDeploy $Context
    }
    catch {
        $restorationError = $_.Exception.ToString()
    }
    $reason = if ($attempt -ge $maxAttempts) {
        "No verified improvement after $attempt distinct attempt(s)."
    }
    else {
        "Only $attempt distinct attempt(s) were produced in $proposalRound proposal round(s), with no verified improvement."
    }
    Write-HarnessJson (Join-Path $Context.RunDirectory "manual-intervention-required.json") ([pscustomobject]@{
        reason = $reason
        maxAttempts = $maxAttempts
        maxProposalRounds = $maxProposalRounds
        proposalRounds = $proposalRound
        distinctAttempts = $attempt
        attempts = $attemptRecords
        baselineRestorationError = $restorationError
    })
}

function New-HarnessContext {
    param(
        [Parameter(Mandatory)]$Configuration,
        [bool]$DryRun,
        [string]$Engine
    )

    $configDirectory = [string]$Configuration.__configDirectory
    $artifactsRootSetting = [string](Get-OptionalProperty $Configuration.harness "artifactsRoot" ".artifacts")
    $artifactsRoot = Resolve-HarnessPath $artifactsRootSetting $configDirectory
    [IO.Directory]::CreateDirectory($artifactsRoot) | Out-Null
    $runId = "{0}-{1}-{2}" -f ([DateTime]::UtcNow.ToString("yyyyMMdd-HHmmss")),
        (($Configuration.name -replace '[^A-Za-z0-9_.-]', '-')),
        ([Guid]::NewGuid().ToString("N").Substring(0, 8))
    $runDirectory = Join-Path $artifactsRoot "runs\$runId"
    [IO.Directory]::CreateDirectory($runDirectory) | Out-Null
    $appRepo = Resolve-HarnessPath ([string]$Configuration.repositories.app.path) $configDirectory
    $profilerRepo = Resolve-HarnessPath ([string]$Configuration.repositories.profiler.path) $configDirectory
    $stageRoot = [string]$Configuration.target.stageRoot
    if ([string]$Configuration.target.kind -eq "local") {
        $stageRoot = Resolve-HarnessPath $stageRoot $configDirectory
    }
    $tokens = @{
        harnessRoot = $script:HarnessRoot
        configDirectory = $configDirectory
        artifactsRoot = $artifactsRoot
        appRepo = $appRepo
        profilerRepo = $profilerRepo
        runDirectory = $runDirectory
        stageRoot = $stageRoot
        targetAppRoot = Join-Path $stageRoot "app"
        targetProfilerRoot = Join-Path $stageRoot "profiler"
        targetDriverRoot = Join-Path $stageRoot "driver"
        targetAgentRoot = Join-Path $stageRoot "agents"
        targetRunRoot = Join-Path $stageRoot "runs\$runId"
    }
    $context = [pscustomobject]@{
        Config = $Configuration
        ArtifactsRoot = $artifactsRoot
        RunId = $runId
        RunDirectory = $runDirectory
        Tokens = $tokens
        DryRun = $DryRun
        Engine = $Engine
    }
    Write-HarnessJson (Join-Path $runDirectory "run.json") ([pscustomobject]@{
        schemaVersion = 1
        runId = $runId
        createdUtc = [DateTime]::UtcNow.ToString("O")
        configPath = $Configuration.__configPath
        targetKind = $Configuration.target.kind
        engine = $Engine
        dryRun = $DryRun
    })
    return $context
}

function Invoke-SystemReady {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [switch]$SkipRepositoryPreparation,
        [switch]$SkipBuild
    )

    if (-not $SkipRepositoryPreparation) {
        Initialize-HarnessRepository $Context.Config.repositories.app "app" $Context
        Initialize-HarnessRepository $Context.Config.repositories.profiler "profiler" $Context
    }
    Initialize-HarnessTarget $Context
    Ensure-HarnessAgents $Context
    Invoke-AppBuildDeploy $Context -SkipBuild:$SkipBuild
    Invoke-ProfilerBuildDeploy $Context -SkipBuild:$SkipBuild | Out-Null
}

function Invoke-ProfilerHarness {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][ValidateScript({ $_ -in $script:SupportedStages })][string]$Stage,
        [Parameter(Mandatory)][string]$ConfigPath,
        [string]$ScenarioPrompt,
        [string]$ScenarioPath,
        [ValidateSet("agency", "github")][string]$Engine,
        [switch]$DryRun,
        [switch]$SkipRepositoryPreparation,
        [switch]$SkipBuild
    )

    $config = Read-ProfilerHarnessConfiguration $ConfigPath
    if ($Stage -eq "validate") {
        [pscustomobject]@{
            valid = $true
            config = $config.__configPath
            name = $config.name
            target = $config.target.kind
        }
        return
    }
    $context = New-HarnessContext $config $DryRun.IsPresent $Engine
    switch ($Stage) {
        "system-ready" {
            Invoke-SystemReady $context -SkipRepositoryPreparation:$SkipRepositoryPreparation -SkipBuild:$SkipBuild
        }
        "app-build-deploy" {
            if (-not $SkipRepositoryPreparation) {
                Initialize-HarnessRepository $config.repositories.app "app" $context
            }
            Initialize-HarnessTarget $context
            Invoke-AppBuildDeploy $context -SkipBuild:$SkipBuild
        }
        "profiler-build-deploy" {
            if (-not $SkipRepositoryPreparation) {
                Initialize-HarnessRepository $config.repositories.profiler "profiler" $context
            }
            Initialize-HarnessTarget $context
            Invoke-ProfilerBuildDeploy $context -SkipBuild:$SkipBuild | Out-Null
        }
        "create-automation-test" {
            if ([string]::IsNullOrWhiteSpace($ScenarioPrompt)) {
                throw "-ScenarioPrompt is required for create-automation-test."
            }
            $parameters = @{
                Context = $context
                Prompt = $ScenarioPrompt
            }
            if (-not [string]::IsNullOrWhiteSpace($ScenarioPath)) { $parameters.OutputPath = $ScenarioPath }
            if (-not [string]::IsNullOrWhiteSpace($Engine)) { $parameters.Engine = $Engine }
            Invoke-CreateAutomationTest @parameters
        }
        "capture" {
            if ([string]::IsNullOrWhiteSpace($ScenarioPath)) {
                $ScenarioPath = [string](Get-RequiredProperty $config.automation "scenarioPath" "config.automation")
            }
            Invoke-HarnessCapture $context $ScenarioPath
        }
        "profile-and-apply-changes" {
            if ([string]::IsNullOrWhiteSpace($ScenarioPath)) {
                $ScenarioPath = [string](Get-RequiredProperty $config.automation "scenarioPath" "config.automation")
            }
            $parameters = @{
                Context = $context
                ScenarioPath = $ScenarioPath
            }
            if (-not [string]::IsNullOrWhiteSpace($Engine)) { $parameters.Engine = $Engine }
            Invoke-ProfileAndApplyChanges @parameters
        }
    }
    [pscustomobject]@{
        completed = $true
        stage = $Stage
        runId = $context.RunId
        runDirectory = $context.RunDirectory
        dryRun = $context.DryRun
    }
}

Export-ModuleMember -Function @(
    "Invoke-ProfilerHarness",
    "Read-ProfilerHarnessConfiguration",
    "Test-ProfilerHarnessConfiguration",
    "Resolve-HarnessPath",
    "Test-AutomationScenario",
    "Test-HarnessImprovement"
)
