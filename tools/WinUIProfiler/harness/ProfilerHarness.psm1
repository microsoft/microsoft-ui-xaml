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

function Expand-HarnessTokens {
    param(
        [AllowNull()]$Value,
        [Parameter(Mandatory)][hashtable]$Tokens
    )

    if ($null -eq $Value) {
        return $null
    }

    $expanded = [string]$Value
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

    Assert-HarnessHostArtifactPath $Path
    $directory = Split-Path -Parent $Path
    if ($directory) {
        [IO.Directory]::CreateDirectory($directory) | Out-Null
    }
    $json = $Value | ConvertTo-Json -Depth 100
    [IO.File]::WriteAllText($Path, $json, [Text.UTF8Encoding]::new($false))
}

function Assert-HarnessHostArtifactPath {
    param([Parameter(Mandatory)][string]$Path)

    $root = [IO.Path]::GetFullPath((Join-Path $script:HarnessRoot ".artifacts")).TrimEnd('\')
    $fullPath = [IO.Path]::GetFullPath($Path)
    if ($fullPath -ne $root -and -not $fullPath.StartsWith($root + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw "Host output must remain inside the harness .artifacts directory. Refusing '$Path'."
    }
    for ($item = $fullPath; $item; $item = Split-Path -Parent $item) {
        if ((Test-Path -LiteralPath $item) -and
            ((Get-Item -LiteralPath $item -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Host output cannot traverse a symlink/junction: '$item'."
        }
    }
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
    if ([string](Get-RequiredProperty $Definition "scope" $Path) -ne "guest") {
        throw "Configuration property '$Path.scope must be guest'; host commands are disabled."
    }
    $arguments = Get-OptionalProperty $Definition "arguments" @()
    if ($arguments -is [string]) {
        throw "Configuration property '$Path.arguments' must be an array."
    }
}

function Test-ProfilerHarnessConfiguration {
    [CmdletBinding()]
    param([Parameter(Mandatory)]$Configuration)

    if ([int](Get-RequiredProperty $Configuration "schemaVersion" "config") -ne 2) {
        throw "Only profiler harness schemaVersion 2 is supported. Migrate legacy host commands before running."
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
        if ($branch -notmatch '^user/[A-Za-z0-9_.-]+/profiler_harness$') {
            throw "config.repositories.$role.branch must match user/<alias>/profiler_harness."
        }
    }

    $target = Get-RequiredProperty $Configuration "target" "config"
    $kind = [string](Get-RequiredProperty $target "kind" "config.target")
    if ($kind -notin @("hyperv", "ixp")) {
        throw "Devbox execution is disabled. config.target.kind must be hyperv or ixp."
    }
    Get-RequiredProperty $target "stageRoot" "config.target" | Out-Null
    if ($kind -ne "local") {
        foreach ($property in @("vmId", "guestMachineGuid")) {
            $identifier = [Guid]::Empty
            if (-not [Guid]::TryParse([string](Get-RequiredProperty $target $property "config.target"), [ref]$identifier) -or
                $identifier -eq [Guid]::Empty) {
                throw "config.target.$property must be a nonempty GUID obtained from the chosen VM."
            }
        }
        Get-RequiredProperty $target "guestComputerName" "config.target" | Out-Null
        Get-RequiredProperty $target "checkpointName" "config.target" | Out-Null
        $stageRoot = [string]$target.stageRoot
        if ($stageRoot -notmatch '^[A-Za-z]:\\WinUIProfilerHarness(?:-[A-Za-z0-9_-]+)?$') {
            throw "config.target.stageRoot must be a dedicated local guest directory such as C:\WinUIProfilerHarness."
        }
    }
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
        $connectionMode = [string](Get-OptionalProperty $ixp "connectionMode" "psremote")
        if ($connectionMode -ne "psremote") {
            throw "config.target.ixp.connectionMode must be psremote; ambient TShell/background connections are disabled."
        }
        if ([bool](Get-OptionalProperty $ixp "ensureCreated" $false)) {
            throw "Automatic IXP creation is disabled. Use the manual provisioning script separately."
        }
        $minimumOsBuild = Get-OptionalProperty $ixp "minimumOsBuild"
        if ($null -ne $minimumOsBuild -and [int]$minimumOsBuild -lt 1) {
            throw "config.target.ixp.minimumOsBuild must be positive."
        }
    }

    $appConfig = Get-RequiredProperty $Configuration "application" "config"
    Get-RequiredProperty $appConfig "resetScript" "config.application" | Out-Null
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
    $samples = [int](Get-OptionalProperty $Configuration.optimization "samplesPerVersion" 3)
    if ($samples -lt 3 -or $samples -gt 10) {
        throw "config.optimization.samplesPerVersion must be between 3 and 10."
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
        foreach ($provider in @("hyperv", "ixp")) {
            foreach ($command in @(Get-OptionalProperty $deploy $provider @())) {
                Test-HarnessCommandDefinition $command "config.application.deployCommands.$provider"
            }
        }
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

    $schemaConfig = @{}
    foreach ($property in $Configuration.PSObject.Properties) {
        if ($property.Name -notin @("__configPath", "__configDirectory")) {
            $schemaConfig[$property.Name] = $property.Value
        }
    }
    if (-not (Test-Json -Json ($schemaConfig | ConvertTo-Json -Depth 100) `
        -SchemaFile (Join-Path $script:HarnessRoot "config.schema.json") -ErrorAction Stop)) {
        throw "Configuration does not satisfy the schema 2 contract."
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

    if (-not $DryRun -and $File -ne "git") {
        throw "Host process execution is disabled for '$File'. Run builds, tools, and agents in the selected VM."
    }
    if ($File -eq "git") {
        $operation = if ($Arguments.Count -ge 3 -and $Arguments[0] -eq "-C") { $Arguments[2] } else { $Arguments[0] }
        if ($operation -notin @("status", "rev-parse", "ls-files")) {
            throw "Host Git operation '$operation' is disabled. Clones, branches and worktrees belong in the VM."
        }
        $Arguments = @("--no-pager", "-c", "core.hooksPath=NUL", "-c", "core.fsmonitor=false",
            "-c", "protocol.ext.allow=never") + $Arguments
    }
    if ($LogBasePath) { Assert-HarnessHostArtifactPath $LogBasePath }
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
            try { $process.Kill($true) } catch { Write-Warning "Stopping the timed-out Git process failed: $($_.Exception.Message)" }
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

    throw "SDK resolution is guest-only. Use Ensure-DotNetSdk.ps1 inside the approved VM."
}

function Invoke-HarnessCommand {
    param(
        [Parameter(Mandatory)]$Definition,
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$Name,
        [switch]$AllowFailure
    )

    Assert-HarnessVmTarget $Context
    Test-HarnessCommandDefinition $Definition $Name
    $tokens = Get-HarnessGuestTokens $Context
    $file = Expand-HarnessTokens (Get-RequiredProperty $Definition "file" $Name) $tokens
    $arguments = @(
        ConvertTo-StringArray (Get-OptionalProperty $Definition "arguments" @()) |
            ForEach-Object { Expand-HarnessTokens $_ $tokens }
    )
    $workingDirectory = Expand-HarnessTokens (Get-OptionalProperty $Definition "workingDirectory" $tokens.appRepo) $tokens
    Assert-HarnessGuestPath $Context $workingDirectory
    $timeout = [int](Get-OptionalProperty $Definition "timeoutSeconds" 3600)
    $environment = @{}
    $environmentObject = Get-OptionalProperty $Definition "environment"
    if ($null -ne $environmentObject) {
        foreach ($property in $environmentObject.PSObject.Properties) {
            if ($property.Name -like "PROFILER_HARNESS_*") {
                throw "Guest execution identity environment variables cannot be overridden by commands."
            }
            $environment[$property.Name] = Expand-HarnessTokens $property.Value $tokens
        }
    }
    $safeName = $Name -replace '[^A-Za-z0-9_.-]', '-'
    $definitionJson = @{ file = $file; arguments = $arguments; workingDirectory = $workingDirectory; environment = $environment } |
        ConvertTo-Json -Depth 20 -Compress
    $encoded = [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes($definitionJson))
    $script = @"
`$ErrorActionPreference = 'Stop'
`$command = [Text.Encoding]::UTF8.GetString([Convert]::FromBase64String('$encoded')) | ConvertFrom-Json
foreach (`$property in `$command.environment.PSObject.Properties) {
    [Environment]::SetEnvironmentVariable(`$property.Name, [string]`$property.Value, 'Process')
}
Set-Location -LiteralPath `$command.workingDirectory
`$arguments = @(`$command.arguments)
& `$command.file @arguments
if (`$LASTEXITCODE -ne 0) { throw "Guest command failed with exit code `$LASTEXITCODE." }
"@
    $result = Invoke-HarnessTargetPowerShell $Context $script $timeout
    [IO.File]::WriteAllText((Join-Path $Context.RunDirectory "$safeName.log"), $result.stdout + $result.stderr)
    if ($result.exitCode -ne 0 -and -not $AllowFailure) {
        throw "Guest command '$Name' failed. $($result.stderr)"
    }
    return $result
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
        Get-RequiredProperty $Definition "remoteUrl" "config.repositories.$Role" | Out-Null
        return
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
            throw "Repository '$path' has tracked changes. Review them or set allowDirty explicitly before guest source staging."
        }
    }

    # Topic branches are created in guest source copies, never checked out over host files.
}

function Get-HyperVCredential {
    param([Parameter(Mandatory)]$Context)

    if ([string]$Context.Config.target.kind -eq "ixp") {
        Connect-HarnessIxpDevice $Context
        $name = [string]$Context.Config.target.ixp.name
        $userName = [string](Get-IxpDeviceLocalAdminName -ForDevice $name)
        $password = Get-IxpDeviceLocalAdminPassword -ForDevice $name -AsSecureString
        if ([string]::IsNullOrWhiteSpace($userName) -or $null -eq $password) {
            throw "IXP guest credentials are unavailable. Configure them manually before running."
        }
        return [PSCredential]::new($userName, $password)
    }
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

function Connect-HarnessIxpDevice {
    param([Parameter(Mandatory)]$Context)

    Assert-HarnessVmTarget $Context
    if (-not (Get-Module IXPTools)) {
        throw "IXPTools must be initialized manually in this session. Import can build host dependencies; the harness will not import it."
    }
}

function Get-HarnessIxpCreationParameters {
    param(
        [Parameter(Mandatory)]$Settings,
        [Parameter(Mandatory)][string]$ConfigDirectory
    )

    throw "Automatic IXP creation is disabled. Use the manual provisioning script separately."
}

function Assert-HarnessVmTarget {
    param([Parameter(Mandatory)]$Context)

    $kind = [string]$Context.Config.target.kind
    if ($kind -notin @("hyperv", "ixp")) {
        throw "Devbox execution is disabled. Select an isolated Hyper-V or IXP VM; there is no local fallback."
    }
}

function Assert-HarnessGuestPath {
    param([Parameter(Mandatory)]$Context, [Parameter(Mandatory)][string]$Path)

    $root = [IO.Path]::GetFullPath([string]$Context.Config.target.stageRoot).TrimEnd('\') + '\'
    if ($Path -notmatch '^[A-Za-z]:\\' -or $Path -match "['`r`n]" -or $Path.Substring(2).Contains(':') -or
        -not [IO.Path]::GetFullPath($Path).StartsWith($root, [StringComparison]::OrdinalIgnoreCase)) {
        throw "'$Path' must be a child of the dedicated guest staging directory '$root'."
    }
}

function Get-HarnessGuestTokens {
    param([Parameter(Mandatory)]$Context)

    $tokens = $Context.Tokens.Clone()
    $tokens.appRepo = $Context.GuestAppRepo
    $tokens.profilerRepo = $Context.GuestProfilerRepo
    $tokens.harnessRoot = $Context.GuestHarnessRoot
    $tokens.configDirectory = Join-Path $Context.GuestHarnessRoot "examples"
    $tokens.artifactsRoot = Join-Path $tokens.stageRoot "artifacts"
    $tokens.runDirectory = $tokens.targetRunRoot
    return $tokens
}

function Get-HarnessVm {
    param([Parameter(Mandatory)]$Context)

    Assert-HarnessVmTarget $Context
    Import-Module Hyper-V -ErrorAction Stop
    $settings = Get-RequiredProperty $Context.Config.target ([string]$Context.Config.target.kind) "config.target"
    $matches = @(Get-VM -Name ([string]$settings.name) -ErrorAction Stop)
    if ($matches.Count -ne 1) { throw "VM name must resolve to exactly one existing VM." }
    $vm = $matches[0]
    if ($vm.Id -ne [Guid]$Context.Config.target.vmId -or $vm.State -ne "Running") {
        throw "The named VM must match config.target.vmId and already be running. No VM is created, started, or replaced automatically."
    }
    $checkpoints = @(Get-VMSnapshot -VM $vm -Name ([string]$Context.Config.target.checkpointName) -ErrorAction Stop)
    if ($checkpoints.Count -ne 1 -or $checkpoints[0].Name -ne [string]$Context.Config.target.checkpointName) {
        throw "The approved clean checkpoint must exist exactly once on the selected VM."
    }
    return $vm
}

function New-HarnessTargetSession {
    param([Parameter(Mandatory)]$Context)

    $vm = Get-HarnessVm $Context
    $session = New-PSSession -VMId $vm.Id -Credential (Get-HyperVCredential $Context) -ErrorAction Stop
    try {
        $identity = Invoke-Command -Session $session -ErrorAction Stop -ScriptBlock {
            [pscustomobject]@{
                machineGuid = (Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Cryptography' -Name MachineGuid -ErrorAction Stop).MachineGuid
                computerName = $env:COMPUTERNAME
                isHyperVGuest = Test-Path 'HKLM:\SOFTWARE\Microsoft\Virtual Machine\Guest\Parameters'
            }
        }
        if (-not $identity.isHyperVGuest -or
            [Guid]$identity.machineGuid -ne [Guid]$Context.Config.target.guestMachineGuid -or
            [Guid]$identity.machineGuid -eq [Guid]$Context.HostMachineGuid -or
            $identity.computerName -ne [string]$Context.Config.target.guestComputerName) {
            throw "Guest identity does not match the approved VM, or resolves to the host. Execution refused."
        }
        return $session
    }
    catch {
        Remove-PSSession $session
        throw
    }
}

function Initialize-HarnessTarget {
    param([Parameter(Mandatory)]$Context)

    Assert-HarnessVmTarget $Context
    if ($Context.DryRun) { return }
    $session = New-HarnessTargetSession $Context
    try {
        $readiness = Invoke-Command -Session $session -ErrorAction Stop -ScriptBlock {
            [pscustomobject]@{
                powerShell7 = $null -ne (Get-Command pwsh -ErrorAction SilentlyContinue)
                interactiveUser = (Get-CimInstance Win32_ComputerSystem).UserName
                credentialUser = [Security.Principal.WindowsIdentity]::GetCurrent().Name
                build = [int](Get-CimInstance Win32_OperatingSystem).BuildNumber
            }
        }
        if (-not $readiness.powerShell7 -or [string]::IsNullOrWhiteSpace($readiness.interactiveUser) -or
            $readiness.interactiveUser -ne $readiness.credentialUser) {
            throw "Complete guest PowerShell 7 installation and interactive sign-in as the configured guest credential before running."
        }
        if ([string]$Context.Config.target.kind -eq "ixp") {
            $minimum = [int](Get-OptionalProperty $Context.Config.target.ixp "minimumOsBuild" 1)
            if ($readiness.build -lt $minimum) { throw "Guest OS build $($readiness.build) is below the configured minimum $minimum." }
        }
    }
    finally {
        Remove-PSSession $session
    }
}

function Invoke-HarnessTargetPowerShell {
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$Script,
        [int]$TimeoutSeconds = 1800,
        [switch]$AsUser
    )

    Assert-HarnessVmTarget $Context
    if ($AsUser) {
        throw "Interactive execution must use the guest capture task; generic commands run as the approved guest credential."
    }
    if ($Context.DryRun) {
        return [pscustomobject]@{ exitCode = 0; stdout = ""; stderr = ""; dryRun = $true }
    }

    $guard = @"
`$env:PROFILER_HARNESS_HOST_MACHINE_GUID = '$($Context.HostMachineGuid)'
`$env:PROFILER_HARNESS_GUEST_MACHINE_GUID = '$($Context.Config.target.guestMachineGuid)'
`$ErrorActionPreference = 'Stop'
`$global:LASTEXITCODE = 0
for (`$item = '$($Context.Config.target.stageRoot)'; `$item; `$item = Split-Path -Parent `$item) {
    if ((Test-Path -LiteralPath `$item) -and
        ((Get-Item -LiteralPath `$item -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Guest staging root traverses a reparse point: '`$item'."
    }
}
if (Get-CimInstance Win32_LogicalDisk -Filter 'DriveType=4') {
    throw 'Disconnect mapped network drives from this isolated guest before execution.'
}
"@
    $encoded = [Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($guard + "`r`n" + $Script))
    $session = New-HarnessTargetSession $Context
    $job = $null
    $guestLogs = Join-Path $Context.Tokens.targetRunRoot "command-logs"
    Assert-HarnessGuestPath $Context $guestLogs
    try {
        $job = Invoke-Command -Session $session -AsJob -ErrorAction Stop -ScriptBlock {
            param($Payload, $Logs, $Timeout)
            $ErrorActionPreference = "Stop"
            for ($item = $Logs; $item; $item = Split-Path -Parent $item) {
                if ((Test-Path -LiteralPath $item) -and
                    ((Get-Item -LiteralPath $item -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
                    throw "Guest log path traverses a reparse point: '$item'."
                }
            }
            New-Item -ItemType Directory -Force -Path $Logs | Out-Null
            $log = Join-Path $Logs ([Guid]::NewGuid().ToString("N"))
            $pwsh = Get-Command pwsh -CommandType Application -ErrorAction Stop
            $process = Start-Process -FilePath $pwsh.Source `
                -ArgumentList @("-NoLogo", "-NoProfile", "-EncodedCommand", $Payload) `
                -RedirectStandardOutput "$log.stdout.log" -RedirectStandardError "$log.stderr.log" `
                -PassThru -NoNewWindow
            $null = $process.Handle
            $timedOut = -not $process.WaitForExit($Timeout * 1000)
            try {
                if ($timedOut) {
                    Stop-Process -Id $process.Id -Force -ErrorAction Stop
                    if (-not $process.WaitForExit(30000)) { throw "Timed-out guest PowerShell did not stop." }
                }
                $process.WaitForExit()
                $process.Refresh()
                [pscustomobject]@{
                    exitCode = $process.ExitCode
                    stdout = [IO.File]::ReadAllText("$log.stdout.log")
                    stderr = [IO.File]::ReadAllText("$log.stderr.log")
                    timedOut = $timedOut
                    guestLogPath = $log
                }
            }
            finally { $process.Dispose() }
        } -ArgumentList $encoded, $guestLogs, $TimeoutSeconds
        $jobTimedOut = $null -eq (Wait-Job -Job $job -Timeout ($TimeoutSeconds + 60))
        $remoteErrors = @()
        $result = Receive-Job -Job $job -ErrorAction SilentlyContinue -ErrorVariable remoteErrors
        $logPath = Join-Path $Context.RunDirectory "guest-command-$([Guid]::NewGuid().ToString('N')).log"
        Assert-HarnessHostArtifactPath $logPath
        [IO.File]::WriteAllText($logPath, ($result | ConvertTo-Json -Depth 10) + ($remoteErrors | Out-String))
        if ($jobTimedOut) {
            throw "Guest execution timed out. Guest logs are in '$guestLogs'; inspect remaining guest child processes manually."
        }
        if ($remoteErrors.Count -gt 0 -or $job.State -ne "Completed" -or $null -eq $result) {
            throw "Guest command failed. Output/errors are preserved at '$logPath'. $($remoteErrors | Out-String)"
        }
        if ($result.timedOut -or $result.exitCode -ne 0) {
            throw "Guest command failed/timed out with exit code $($result.exitCode). Evidence: '$logPath'. $($result.stderr)"
        }
        return [pscustomobject]@{ exitCode = $result.exitCode; stdout = [string]$result.stdout; stderr = [string]$result.stderr; dryRun = $false }
    }
    finally {
        if ($null -ne $job) { Remove-Job -Job $job -Force }
        Remove-PSSession $session
    }
}

function Copy-HarnessDirectoryToTarget {
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$Source,
        [Parameter(Mandatory)][string]$Destination,
        [Parameter(Mandatory)][string]$Name
    )

    Assert-HarnessVmTarget $Context
    Assert-HarnessGuestPath $Context $Destination
    if ($Context.DryRun) {
        return
    }
    if (-not (Test-Path -LiteralPath $Source -PathType Container)) {
        throw "Deployment source '$Source' does not exist."
    }

    $archive = Join-Path $Context.RunDirectory "$Name.zip"
    if (Test-Path -LiteralPath $archive) {
        Remove-Item -LiteralPath $archive -Force
    }
    [IO.Compression.ZipFile]::CreateFromDirectory($Source, $archive, [IO.Compression.CompressionLevel]::Fastest, $false)
    $targetArchive = "$Destination.zip"
    $session = New-HarnessTargetSession $Context
    try {
        Invoke-Command -Session $session -ErrorAction Stop -ScriptBlock {
            param($Path)
            $parent = Split-Path -Parent $Path
            for ($item = $parent; $item; $item = Split-Path -Parent $item) {
                if ((Test-Path -LiteralPath $item) -and
                    ((Get-Item -LiteralPath $item -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
                    throw "Refusing a guest deployment path containing a reparse point: '$item'."
                }
            }
            New-Item -ItemType Directory -Force -Path $parent | Out-Null
        } -ArgumentList $targetArchive
        Copy-Item -LiteralPath $archive -Destination $targetArchive -ToSession $session -Force
    }
    finally {
        Remove-PSSession $session
    }
    $script = @"
`$ErrorActionPreference = 'Stop'
for (`$item = '$Destination'; `$item; `$item = Split-Path -Parent `$item) {
    if ((Test-Path -LiteralPath `$item) -and
        ((Get-Item -LiteralPath `$item -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Refusing a guest destination containing a reparse point: '`$item'."
    }
}
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

    Assert-HarnessVmTarget $Context
    Assert-HarnessGuestPath $Context $Source
    Assert-HarnessHostArtifactPath $Destination
    if ($Context.DryRun) { return }
    [IO.Directory]::CreateDirectory($Destination) | Out-Null
    $targetArchive = "$Source.zip"
    $script = @"
`$ErrorActionPreference = 'Stop'
foreach (`$path in @('$Source', '$targetArchive')) {
    for (`$item = `$path; `$item; `$item = Split-Path -Parent `$item) {
        if ((Test-Path -LiteralPath `$item) -and
            ((Get-Item -LiteralPath `$item -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Evidence source traverses a reparse point: '`$item'."
        }
    }
}
if (Get-ChildItem -LiteralPath '$Source' -Recurse -Force | Where-Object { `$_.Attributes -band [IO.FileAttributes]::ReparsePoint }) {
    throw 'Guest evidence contains a reparse point.'
}
if (Test-Path -LiteralPath '$targetArchive') { Remove-Item -LiteralPath '$targetArchive' -Force }
Compress-Archive -Path '$Source\*' -DestinationPath '$targetArchive' -CompressionLevel Fastest
"@
    $result = Invoke-HarnessTargetPowerShell $Context $script 1800
    if ($result.exitCode -ne 0) {
        throw "Packaging target results failed. $($result.stderr)"
    }
    $localArchive = Join-Path $Context.RunDirectory "$Name-results.zip"
    $session = New-HarnessTargetSession $Context
    try {
        Copy-Item -LiteralPath $targetArchive -Destination $localArchive -FromSession $session -Force
    }
    finally {
        Remove-PSSession $session
    }
    $zip = [IO.Compression.ZipFile]::OpenRead($localArchive)
    try {
        foreach ($entry in $zip.Entries) {
            Assert-HarnessEvidenceEntry $Destination $entry.FullName
        }
    }
    finally {
        $zip.Dispose()
    }
    Expand-Archive -LiteralPath $localArchive -DestinationPath $Destination -Force
}

function Assert-HarnessEvidenceEntry {
    param([Parameter(Mandatory)][string]$Destination, [Parameter(Mandatory)][string]$EntryName)
    Assert-HarnessHostArtifactPath $Destination
    $path = [IO.Path]::GetFullPath((Join-Path $Destination $EntryName))
    Assert-HarnessHostArtifactPath $path
    if (-not $path.StartsWith([IO.Path]::GetFullPath($Destination).TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase) -or
        $path.Substring(2).Contains(':') -or
        ([IO.Path]::GetFileName($EntryName) -and [IO.Path]::GetExtension($path) -notin
            @(".json", ".md", ".txt", ".log", ".db", ".sqlite", ".sqlite3", ".db-wal", ".db-shm", ".sqlite-wal", ".sqlite-shm", ".etl"))) {
        throw "Refusing executable or out-of-directory guest evidence '$EntryName'."
    }
}

function Ensure-HarnessAgents {
    param([Parameter(Mandatory)]$Context)

    Assert-HarnessVmTarget $Context
    $script = @'
$ErrorActionPreference = "Stop"
function Refresh-Path {
    $env:Path = [Environment]::GetEnvironmentVariable("Path", "Machine") + ";" +
        [Environment]::GetEnvironmentVariable("Path", "User")
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
    $result = Invoke-HarnessTargetPowerShell $Context $script 1800
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
    if ($Context.DryRun) {
        return [pscustomobject]@{
            payloadDirectory = Join-Path $Context.Tokens.targetProfilerRoot "ConnectionPayloads"
            driverExecutable = Join-Path $Context.Tokens.targetDriverRoot "ProfilerHarness.Driver.exe"
        }
    }
    $path = Get-HarnessDeploymentStatePath $Context
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "No deployment state exists for this configuration and target. Run profiler-build-deploy first."
    }
    $state = Read-HarnessJson $path
    if ([Guid](Get-OptionalProperty $state "vmId" ([Guid]::Empty)) -ne [Guid]$Context.Config.target.vmId -or
        [Guid](Get-OptionalProperty $state "guestMachineGuid" ([Guid]::Empty)) -ne [Guid]$Context.Config.target.guestMachineGuid) {
        throw "Deployment state belongs to a different VM/guest. Run profiler-build-deploy for this approved target."
    }
    return $state
}

function Initialize-HarnessGuestRepository {
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][ValidateSet("app", "profiler")][string]$Role
    )

    Assert-HarnessVmTarget $Context
    if ($Context.DryRun) { return }
    $source = $Context.Tokens["${Role}Repo"]
    $destination = if ($Role -eq "app") { $Context.GuestAppRepo } else { $Context.GuestProfilerRepo }
    $branch = [string]$Context.Config.repositories.$Role.branch
    if (-not (Test-Path -LiteralPath $source -PathType Container)) {
        $remote = ([string](Get-RequiredProperty $Context.Config.repositories.$Role "remoteUrl" "config.repositories.$Role")).Replace("'", "''")
        $baseRef = ([string](Get-OptionalProperty $Context.Config.repositories.$Role "baseRef" "HEAD")).Replace("'", "''")
        Invoke-HarnessTargetPowerShell $Context @"
if (Test-Path -LiteralPath '$destination') { throw 'Guest clone destination already exists. Inspect it manually; it will not be replaced.' }
New-Item -ItemType Directory -Force -Path (Split-Path -Parent '$destination') | Out-Null
git -c core.hooksPath=NUL -c protocol.ext.allow=never clone --recurse-submodules -- '$remote' '$destination'
if (`$LASTEXITCODE -ne 0) { throw 'Guest clone failed. Authenticate in the guest and verify repository access.' }
git -C '$destination' -c core.hooksPath=NUL checkout -b '$branch' '$baseRef'
if (`$LASTEXITCODE -ne 0) { throw 'Guest topic branch creation failed.' }
"@ 3600 | Out-Null
    }
    else {
        Copy-HarnessSourceToTarget $Context $source $destination "source-$Role"
    }
    if ($Role -eq "profiler") {
        Copy-HarnessSourceToTarget $Context $script:HarnessRoot $Context.GuestHarnessRoot "harness-source"
    }
    Invoke-HarnessTargetPowerShell $Context @"
Set-Location -LiteralPath '$destination'
if (-not (Test-Path -LiteralPath '.git')) {
    git init --initial-branch '$branch'
    if (`$LASTEXITCODE -ne 0) { throw 'Guest repository initialization failed.' }
}
`$currentBranch = git -c core.hooksPath=NUL symbolic-ref --short HEAD
if (`$LASTEXITCODE -ne 0 -or `$currentBranch -cne '$branch') { throw 'Guest source branch does not match the configured topic branch.' }
git -c core.hooksPath=NUL add --all
if (`$LASTEXITCODE -ne 0) { throw 'Guest source staging failed.' }
git -c core.hooksPath=NUL -c user.name=ProfilerHarness -c user.email=profiler-harness@localhost commit --allow-empty -m 'Harness source snapshot' -m 'Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>'
if (`$LASTEXITCODE -ne 0) { throw 'Guest source snapshot commit failed.' }
"@ 600 | Out-Null
}

function Copy-HarnessSourceToTarget {
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$Source,
        [Parameter(Mandatory)][string]$Destination,
        [Parameter(Mandatory)][string]$Name
    )
    $source = [IO.Path]::GetFullPath($Source).TrimEnd('\')
    $staging = Join-Path $Context.RunDirectory $Name
    Assert-HarnessHostArtifactPath $staging
    [IO.Directory]::CreateDirectory($staging) | Out-Null
    try {
        $index = Invoke-Git $source @("ls-files", "--stage", "--recurse-submodules", "-z") $Context "$Name-index"
        foreach ($entry in @($index.stdout -split '\x00' | Where-Object { $_ -match '^160000 ' })) {
            $submodule = Join-Path $source ($entry -split "`t", 2)[1]
            if (-not (Test-Path -LiteralPath (Join-Path $submodule ".git"))) {
                throw "Submodule '$submodule' is uninitialized. Initialize it manually before staging source."
            }
        }
        $tracked = Invoke-Git $source @("ls-files", "--cached", "--recurse-submodules", "-z") $Context "$Name-tracked"
        $untracked = Invoke-Git $source @("ls-files", "--others", "--exclude-standard", "-z") $Context "$Name-untracked"
        $files = @(($tracked.stdout + $untracked.stdout) -split '\x00' | Where-Object { $_ } | Sort-Object -Unique)
        foreach ($relative in $files) {
            $path = [IO.Path]::GetFullPath((Join-Path $source $relative))
            if (-not $path.StartsWith([IO.Path]::GetFullPath($source).TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)) {
                throw "Source path escapes the repository: '$relative'."
            }
            if (-not (Test-Path -LiteralPath $path)) { continue }
            for ($item = $path; $item -and $item -ne $source; $item = Split-Path -Parent $item) {
                if ((Get-Item -LiteralPath $item -Force -ErrorAction Stop).Attributes -band [IO.FileAttributes]::ReparsePoint) {
                    throw "Source staging refuses symlinks/junctions: '$item'."
                }
            }
            if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
                throw "Source '$relative' is unavailable. Initialize submodules manually before staging."
            }
            $copy = Join-Path $staging $relative
            [IO.Directory]::CreateDirectory((Split-Path -Parent $copy)) | Out-Null
            Copy-Item -LiteralPath $path -Destination $copy
        }
        Copy-HarnessDirectoryToTarget $Context $staging $Destination $Name
    }
    finally {
        Remove-Item -LiteralPath $staging -Recurse -Force
    }
}

function Copy-HarnessGuestDirectory {
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$Source,
        [Parameter(Mandatory)][string]$Destination
    )
    Assert-HarnessGuestPath $Context $Source
    Assert-HarnessGuestPath $Context $Destination
    $sourceRoot = [IO.Path]::GetFullPath($Source).TrimEnd('\')
    $destinationRoot = [IO.Path]::GetFullPath($Destination).TrimEnd('\')
    if ($sourceRoot -eq $destinationRoot -or $sourceRoot.StartsWith($destinationRoot + '\', [StringComparison]::OrdinalIgnoreCase) -or
        $destinationRoot.StartsWith($sourceRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw "Guest deployment source and destination must not overlap."
    }
    $script = @"
if (-not (Test-Path -LiteralPath '$Source' -PathType Container)) { throw 'Guest build output is missing.' }
foreach (`$path in @('$Source', '$Destination')) {
    for (`$item = `$path; `$item; `$item = Split-Path -Parent `$item) {
        if ((Test-Path -LiteralPath `$item) -and
            ((Get-Item -LiteralPath `$item -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Refusing a guest build/deployment path containing a reparse point: '`$item'."
        }
    }
}
if (Get-ChildItem -LiteralPath '$Source' -Recurse -Force | Where-Object { `$_.Attributes -band [IO.FileAttributes]::ReparsePoint }) {
    throw 'Guest build output contains a reparse point.'
}
if (Test-Path -LiteralPath '$Destination') { Remove-Item -LiteralPath '$Destination' -Recurse -Force }
New-Item -ItemType Directory -Path '$Destination' | Out-Null
Copy-Item -Path '$Source\*' -Destination '$Destination' -Recurse -Force
"@
    Invoke-HarnessTargetPowerShell $Context $script 1800 | Out-Null
}

function Invoke-AppBuildDeploy {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [switch]$SkipBuild,
        [switch]$SkipSourceStaging
    )

    $application = $Context.Config.application
    Assert-HarnessVmTarget $Context
    if (-not $SkipBuild -and -not $SkipSourceStaging) {
        Initialize-HarnessGuestRepository $Context "app"
    }
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
    if ($payloadPath) {
        $guestTokens = Get-HarnessGuestTokens $Context
        $resolved = Expand-HarnessTokens $payloadPath $guestTokens
        Copy-HarnessGuestDirectory $Context $resolved $Context.Tokens.targetAppRoot
        $installScripts = Get-OptionalProperty $application "targetInstallScripts"
        $installScript = Get-OptionalProperty $installScripts ([string]$Context.Config.target.kind)
        if ($installScript) {
            $expandedScript = Expand-HarnessTokens ([string]$installScript) $guestTokens
            $result = Invoke-HarnessTargetPowerShell $Context $expandedScript 1800
            if ($result.exitCode -ne 0) {
                throw "Target-side app installation failed. $($result.stderr) $($result.stdout)"
            }
        }
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
    Assert-HarnessVmTarget $Context
    if ($configuration -notin @("Debug", "Release") -or $platform -notin @("x64", "ARM64")) {
        throw "Unsupported profiler configuration or platform."
    }
    if (-not $SkipBuild) { Initialize-HarnessGuestRepository $Context "profiler" }
    $profilerRepo = $Context.GuestProfilerRepo
    $profilerRoot = Join-Path $profilerRepo "tools\WinUIProfiler"
    $buildScript = Join-Path $profilerRoot "scripts\Build-App.ps1"
    $driverProject = Join-Path $Context.GuestHarnessRoot "Driver\ProfilerHarness.Driver.csproj"
    $driverOutput = Join-Path $Context.Tokens.stageRoot "artifacts\driver\$configuration\win-$($platform.ToLowerInvariant())"
    if (-not $SkipBuild) {
        $sdkScript = Join-Path $Context.GuestHarnessRoot "scripts\Ensure-DotNetSdk.ps1"
        $sdkSetup = @"
`$sdk = & '$sdkScript' -RepoRoot '$profilerRoot'
`$env:DOTNET_ROOT = `$sdk.dotnetRoot
`$env:DOTNET_ROOT_X64 = `$sdk.dotnetRoot
`$env:PATH = "`$(`$sdk.dotnetRoot);`$env:PATH"
"@
        Invoke-HarnessCommand ([pscustomobject]@{
            scope = "guest"
            file = "pwsh"
            arguments = @("-NoLogo", "-NoProfile", "-Command",
                "$sdkSetup`r`n& '$buildScript' -Configuration '$configuration' -Platform '$platform'; if (`$LASTEXITCODE -ne 0) { throw 'Profiler build failed.' }")
            workingDirectory = $profilerRoot
            timeoutSeconds = [int](Get-OptionalProperty $profiler "buildTimeoutSeconds" 7200)
        }) $Context "profiler-build" | Out-Null
        Invoke-HarnessCommand ([pscustomobject]@{
            scope = "guest"
            file = "pwsh"
            arguments = @(
                "-NoLogo", "-NoProfile", "-Command",
                "$sdkSetup`r`n& (Join-Path `$sdk.dotnetRoot 'dotnet.exe') publish '$driverProject' --configuration '$configuration' --runtime 'win-$($platform.ToLowerInvariant())' --self-contained true --output '$driverOutput' -p:PublishSingleFile=false; if (`$LASTEXITCODE -ne 0) { throw 'Driver publish failed.' }"
            )
            workingDirectory = Join-Path $Context.GuestHarnessRoot "Driver"
            timeoutSeconds = 3600
        }) $Context "driver-publish" | Out-Null
    }

    $outputMarker = Join-Path $profilerRoot "WinUIProfiler.App\obj\ProfilerAppOutput.$platform.$configuration.txt"
    if ($Context.DryRun) {
        $profilerOutput = Join-Path $Context.Tokens.stageRoot "artifacts\dry-run\profiler"
    }
    else {
        $probe = Invoke-HarnessTargetPowerShell $Context @"
if (-not (Test-Path -LiteralPath '$outputMarker' -PathType Leaf)) { throw 'Guest profiler output marker is missing. Build in the VM first.' }
`$output = (Get-Content -LiteralPath '$outputMarker' -Raw -Encoding UTF8).Trim()
if (-not (Test-Path -LiteralPath `$output -PathType Container) -or
    -not (Test-Path -LiteralPath '$driverOutput' -PathType Container)) { throw 'Guest profiler/driver output is missing.' }
Write-Output `$output
"@ 60
        $profilerOutput = $probe.stdout.Trim()
    }

    $targetKind = [string]$Context.Config.target.kind
    Copy-HarnessGuestDirectory $Context $profilerOutput $Context.Tokens.targetProfilerRoot
    Copy-HarnessGuestDirectory $Context $driverOutput $Context.Tokens.targetDriverRoot
    $targetProfiler = $Context.Tokens.targetProfilerRoot
    $targetDriver = $Context.Tokens.targetDriverRoot

    $state = [pscustomobject]@{
        schemaVersion = 2
        generatedUtc = [DateTime]::UtcNow.ToString("O")
        targetKind = $targetKind
        vmId = [string]$Context.Config.target.vmId
        guestMachineGuid = [string]$Context.Config.target.guestMachineGuid
        profilerGuestOutput = $profilerOutput
        driverGuestOutput = $driverOutput
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

    try {
        $valid = Test-Json -Json ($Scenario | ConvertTo-Json -Depth 100) `
            -SchemaFile (Join-Path $script:HarnessRoot "automation.schema.json") -ErrorAction Stop
    }
    catch {
        throw [IO.InvalidDataException]::new("Invalid automation scenario: $($_.Exception.Message)", $_.Exception)
    }
    if (-not $valid) { throw "Invalid automation scenario." }
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

    if (-not $OnTarget) {
        throw "Agents may run only inside the selected VM. A host worktree is not an isolation boundary."
    }
    Assert-HarnessVmTarget $Context
    Assert-HarnessGuestPath $Context $WorkingDirectory
    $common = @(
        "-p", $Prompt,
        "-s",
        "--output-format", "text",
        "--deny-tool", "task_complete",
        "--disable-builtin-mcps",
        "--no-custom-instructions",
        "--no-ask-user",
        "--no-color",
        "-C", $WorkingDirectory
    )
    if ($Editing) {
        $common += "--allow-all-tools"
    }
    else {
        foreach ($tool in @("view", "glob", "rg")) { $common += @("--allow-tool", $tool) }
    }
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
    if ($OnTarget) {
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
"@
        }
        else { "" }
        $script = @"
`$ErrorActionPreference = 'Stop'
`$prompt = [Text.Encoding]::UTF8.GetString([Convert]::FromBase64String('$promptBase64'))
`$workingDirectory = [Text.Encoding]::UTF8.GetString([Convert]::FromBase64String('$workingBase64'))
for (`$item = `$workingDirectory; `$item; `$item = Split-Path -Parent `$item) {
    if ((Test-Path -LiteralPath `$item) -and
        ((Get-Item -LiteralPath `$item -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Guest agent working directory traverses a reparse point: '`$item'."
    }
}
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
Write-Output `$output
if (`$exitCode -ne 0) { throw "Agent '$Engine' failed with exit code `$exitCode." }
"@
        $result = Invoke-HarnessTargetPowerShell $Context $script 1800
        if ($result.exitCode -ne 0) {
            throw "Target agent '$Engine' failed. $($result.stderr) $($result.stdout)"
        }
        return $result
    }

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
    if (-not [IO.Path]::GetFullPath($OutputPath).StartsWith($Context.ArtifactsRoot.TrimEnd('\') + '\',
        [StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetExtension($OutputPath) -ne ".json") {
        throw "Generated scenarios must be JSON files inside the harness artifacts directory; host source/binary paths cannot be overwritten."
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
    $agentWorkingDirectory = $Context.Tokens.targetRunRoot
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
        schemaVersion = 2
        hostMachineGuid = [string]$Context.HostMachineGuid
        guestMachineGuid = [string]$Context.Config.target.guestMachineGuid
        launch = [pscustomobject]@{
            kind = [string]$launch.kind
            target = Expand-HarnessTokens ([string]$launch.target) (Get-HarnessGuestTokens $Context)
            arguments = Expand-HarnessTokens ([string](Get-OptionalProperty $launch "arguments" "")) (Get-HarnessGuestTokens $Context)
            workingDirectory = Expand-HarnessTokens (Get-OptionalProperty $launch "workingDirectory") (Get-HarnessGuestTokens $Context)
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

    Assert-HarnessVmTarget $Context
    if ($Context.DryRun) { return }
    Assert-HarnessGuestPath $Context $Driver
    Assert-HarnessGuestPath $Context $RequestPath
    Assert-HarnessGuestPath $Context $ResultPath
    $credential = Get-HyperVCredential $Context
    $session = New-HarnessTargetSession $Context
    $taskName = "ProfilerHarness-$([Guid]::NewGuid().ToString('N'))"
    try {
        $userName = $credential.UserName
        $argument = "capture --request `"$RequestPath`""
        Invoke-Command -Session $session -ErrorAction Stop -ScriptBlock {
            param($Name, $Executable, $Arguments, $UserName, $Request, $Result, $Timeout)
            foreach ($path in @($Executable, $Request, $Result)) {
                for ($item = $path; $item; $item = Split-Path -Parent $item) {
                    if ((Test-Path -LiteralPath $item) -and
                        ((Get-Item -LiteralPath $item -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
                        throw "Guest capture path traverses a reparse point: '$item'."
                    }
                }
            }
            $action = New-ScheduledTaskAction -Execute $Executable -Argument $Arguments
            $principal = New-ScheduledTaskPrincipal -UserId $UserName -LogonType Interactive -RunLevel Highest
            $settings = New-ScheduledTaskSettingsSet -ExecutionTimeLimit ([TimeSpan]::FromSeconds($Timeout)) -DisallowHardTerminate:$false
            Register-ScheduledTask -TaskName $Name -Action $action -Principal $principal -Settings $settings -Force | Out-Null
            Start-ScheduledTask -TaskName $Name
        } -ArgumentList $taskName, $Driver, $argument, $userName, $RequestPath, $ResultPath, $TimeoutSeconds
        $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
        do {
            $exists = Invoke-Command -Session $session -ErrorAction Stop -ScriptBlock { param($Path) Test-Path -LiteralPath $Path } -ArgumentList $ResultPath
            if ($exists) { return }
            Start-Sleep -Seconds 2
        } while ([DateTime]::UtcNow -lt $deadline)
        $message = "Interactive capture task '$taskName' did not produce '$ResultPath' within $TimeoutSeconds seconds."
        try {
            Invoke-Command -Session $session -ErrorAction Stop -ScriptBlock {
                param($Name)
                Stop-ScheduledTask -TaskName $Name -ErrorAction Stop
                for ($index = 0; $index -lt 5; $index++) {
                    if ((Get-ScheduledTask -TaskName $Name -ErrorAction Stop).State -ne "Running") { return }
                    Start-Sleep -Seconds 1
                }
                throw "The capture task is still running after the stop request."
            } -ArgumentList $taskName
        }
        catch {
            throw [TimeoutException]::new("$message Stopping the task failed; inspect the VM manually. $($_.Exception.Message)", $_.Exception)
        }
        throw [TimeoutException]::new("$message The task was stopped; its definition and evidence were retained for manual inspection.")
    }
    finally {
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
        if ($null -ne $ExecutionFailure) { throw $ExecutionFailure }
        $validity = Get-RequiredProperty $result "captureValidity" "capture"
        $etwAvailable = Get-RequiredProperty $validity "etwAvailable" "capture.captureValidity"
        if ($etwAvailable -isnot [bool] -or -not $etwAvailable -or
            [int](Get-RequiredProperty $validity "lostEvents" "capture.captureValidity") -ne 0 -or
            [int64](Get-RequiredProperty $validity "droppedEvents" "capture.captureValidity") -ne 0 -or
            (Get-OptionalProperty $validity "etwError") -or (Get-OptionalProperty $validity "recordingError")) {
            throw "Capture diagnostics are incomplete or lost events. Evidence was preserved at '$ResultPath'."
        }
        foreach ($name in @("beforeSnapshot", "afterSnapshot")) {
            $snapshot = Get-RequiredProperty $result $name "capture"
            $snapshotPath = [string](Get-RequiredProperty $snapshot "path" "capture.$name")
            if ([int](Get-RequiredProperty $snapshot "roots" "capture.$name") -lt 1 -or
                ([int](Get-OptionalProperty $snapshot "xamlNodes" 0) + [int](Get-OptionalProperty $snapshot "compositionNodes" 0)) -lt 1) {
                throw "Snapshot '$name' has no captured roots or nodes. Private diagnostics readiness is not established."
            }
            $localSnapshot = Join-Path (Split-Path -Parent $ResultPath) ("snapshots\" + [IO.Path]::GetFileName($snapshotPath))
            if (-not (Test-Path -LiteralPath $localSnapshot -PathType Leaf)) {
                throw "Snapshot evidence '$localSnapshot' is missing. Capture is not complete."
            }
            Read-HarnessJson $localSnapshot | Out-Null
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

    Assert-HarnessVmTarget $Context
    $scenarioFullPath = Resolve-HarnessPath $ScenarioPath $Context.Config.__configDirectory
    $scenario = Read-HarnessJson $scenarioFullPath
    Test-AutomationScenario $scenario | Out-Null
    $deployment = Get-HarnessDeploymentState $Context
    Assert-HarnessGuestPath $Context ([string]$deployment.payloadDirectory)
    $captureRoot = Join-Path $Context.RunDirectory $Name
    [IO.Directory]::CreateDirectory($captureRoot) | Out-Null

    $kind = [string]$Context.Config.target.kind
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
    $localResult = Join-Path $captureRoot "capture-result.json"
    if ($Context.DryRun) { return $localResult }
    Copy-HarnessDirectoryToTarget $Context $requestStaging $targetCaptureRoot "$Name-request"
    $resetScript = Expand-HarnessTokens ([string]$Context.Config.application.resetScript) (Get-HarnessGuestTokens $Context)
    Invoke-HarnessTargetPowerShell $Context $resetScript 300 | Out-Null

    $timeout = [int](Get-OptionalProperty $Context.Config.profiling "firstFrameTimeoutSeconds" 60) +
        [int](Get-OptionalProperty $Context.Config.profiling "scenarioTimeoutSeconds" 180) + 180
    $executionFailure = $null
    try {
        Invoke-HyperVCapture $Context ([string]$deployment.driverExecutable) $targetRequest $targetResult $timeout
    }
    catch {
        $executionFailure = $_
    }

    Copy-HarnessDirectoryFromTarget $Context $targetCaptureRoot $captureRoot $Name
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
    $value = [double]$property.Value
    if ([double]::IsNaN($value) -or [double]::IsInfinity($value)) {
        throw "Capture metric '$Metric' must be finite."
    }
    return $value
}

function Test-HarnessCaptureValidity {
    param([Parameter(Mandatory)]$Capture)

    $validity = Get-OptionalProperty $Capture "captureValidity"
    $metrics = Get-OptionalProperty $Capture "metrics"
    $etwAvailable = Get-OptionalProperty $validity "etwAvailable" $false
    return [string](Get-OptionalProperty $Capture "status" "") -eq "succeeded" -and
        $etwAvailable -is [bool] -and $etwAvailable -and
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
    if ($direction -notin @("lower", "higher")) {
        throw "Optimization direction must be lower or higher."
    }
    if ($minimum -lt 0 -or [double]::IsNaN($minimum) -or [double]::IsInfinity($minimum)) {
        throw "The minimum improvement percentage must be finite and nonnegative."
    }
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
        improved = $valid -and $percent -gt 0 -and $percent -ge $minimum
    }
}

function Test-HarnessRepeatedImprovement {
    param(
        [Parameter(Mandatory)][object[]]$Baseline,
        [Parameter(Mandatory)][object[]]$Candidate,
        [Parameter(Mandatory)]$Optimization
    )
    if ($Baseline.Count -lt 3 -or $Baseline.Count -ne $Candidate.Count) {
        throw "Improvement requires at least three equally sized baseline/candidate capture sets."
    }
    $comparisons = @(
        for ($index = 0; $index -lt $Baseline.Count; $index++) {
            Test-HarnessImprovement $Baseline[$index] $Candidate[$index] $Optimization
        }
    )
    return [pscustomobject]@{
        samplesPerVersion = $Baseline.Count
        comparisons = $comparisons
        improved = @($comparisons | Where-Object { -not $_.improved }).Count -eq 0
    }
}

function Invoke-HarnessCaptureSeries {
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$ScenarioPath,
        [Parameter(Mandatory)][string]$Name
    )
    $count = [int](Get-OptionalProperty $Context.Config.optimization "samplesPerVersion" 3)
    for ($index = 1; $index -le $count; $index++) {
        Invoke-HarnessCapture $Context $ScenarioPath "$Name-$index"
    }
}

function Assert-HarnessGuestBaseline {
    param(
        [Parameter(Mandatory)]$Context,
        [ValidatePattern('^$|^[0-9a-f]{40}([0-9a-f]{24})?$')][string]$ExpectedCommit = ""
    )

    Assert-HarnessVmTarget $Context
    $repo = $Context.GuestAppRepo
    Assert-HarnessGuestPath $Context $repo
    $branch = [string]$Context.Config.repositories.app.branch
    $result = Invoke-HarnessTargetPowerShell $Context @"
`$branch = git -C '$repo' -c core.hooksPath=NUL symbolic-ref --short HEAD
if (`$LASTEXITCODE -ne 0 -or `$branch -cne '$branch') { throw 'The guest baseline branch changed; stop for manual inspection.' }
`$head = git -C '$repo' -c core.hooksPath=NUL rev-parse --verify HEAD
if (`$LASTEXITCODE -ne 0) { throw 'Cannot read the guest baseline HEAD.' }
if ('$ExpectedCommit' -and `$head -cne '$ExpectedCommit') { throw 'The guest baseline HEAD changed; stop for manual inspection.' }
`$status = git -C '$repo' -c core.hooksPath=NUL status --porcelain --untracked-files=normal
if (`$LASTEXITCODE -ne 0) { throw 'Cannot read the guest baseline status.' }
if (`$status) { throw 'The guest baseline is not clean; stop for manual inspection.' }
Write-Output `$head
"@ 60
    $head = $result.stdout.Trim()
    if ($head -notmatch '^[0-9a-f]{40}([0-9a-f]{24})?$') { throw "The guest baseline HEAD is invalid." }
    return $head
}

function New-HarnessAttemptWorktree {
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][int]$Attempt,
        [Parameter(Mandatory)][ValidatePattern('^[0-9a-f]{40}([0-9a-f]{24})?$')][string]$BaseCommit
    )

    Assert-HarnessVmTarget $Context
    $repo = $Context.GuestAppRepo
    $root = Join-Path $Context.Tokens.stageRoot "worktrees"
    $path = Join-Path $root "$($Context.RunId)-$Attempt"
    Invoke-HarnessTargetPowerShell $Context @"
New-Item -ItemType Directory -Force -Path '$root' | Out-Null
git -C '$repo' -c core.hooksPath=NUL worktree add --detach '$path' '$BaseCommit'
if (`$LASTEXITCODE -ne 0) { throw 'Guest attempt worktree creation failed.' }
"@ 300 | Out-Null
    return [pscustomobject]@{ path = $path }
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
    Assert-HarnessVmTarget $Context
    $workingDirectory = $Context.Tokens.targetRunRoot
    $agentCapturePath = Join-Path (Join-Path $workingDirectory $CaptureName) "capture-result.json"
    $prompt = @"
Analyze this WinUI Profiler capture and return an actionable Markdown report only:
$agentCapturePath

Read the before/after snapshot JSON paths recorded in capture-result.json and the adjacent
session-evidence.json export of the SQLite session's frames and object event spans. Compare
all baseline-* capture directories in this run. Event spans are not proof of allocation/destruction
or a complete object lifetime. Use frame timings, event counts, XAML/composition node deltas,
and correlation data. Rank distinct improvement
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
    Initialize-HarnessRepository $Context.Config.repositories.app "app" $Context
    Initialize-HarnessGuestRepository $Context "app"
    $baselineHead = if (-not $Context.DryRun) { Assert-HarnessGuestBaseline $Context }
    Invoke-AppBuildDeploy $Context -SkipSourceStaging
    if (-not $Context.DryRun) { Assert-HarnessGuestBaseline $Context $baselineHead | Out-Null }
    $baselinePaths = @(Invoke-HarnessCaptureSeries $Context $ScenarioPath "baseline")
    if ($Context.DryRun) {
        return
    }
    Assert-HarnessGuestBaseline $Context $baselineHead | Out-Null
    Write-HarnessJson (Join-Path $Context.RunDirectory "baseline-source.json") ([pscustomobject]@{
        branch = $Context.Config.repositories.app.branch
        commit = $baselineHead
    })
    $baselinePath = $baselinePaths[0]
    $baseline = @($baselinePaths | ForEach-Object { Read-HarnessJson $_ })
    if (@($baseline | Where-Object { -not (Test-HarnessCaptureValidity $_) }).Count -gt 0) {
        throw "The baseline capture is not valid for optimization. Inspect '$baselinePath' for ETW, recording, or frame evidence errors."
    }
    $analysisPath = Invoke-HarnessAnalysis $Context $baselinePath "baseline-1" $Engine
    $attemptRecords = [Collections.Generic.List[object]]::new()
    $signatures = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $proposalRound = 0
    $attempt = 0

    while ($attempt -lt $maxAttempts -and $proposalRound -lt $maxProposalRounds) {
        $proposalRound++
        Assert-HarnessGuestBaseline $Context $baselineHead | Out-Null
        $worktree = New-HarnessAttemptWorktree $Context $proposalRound $baselineHead
        $attemptContext = $null
        try {
            $history = if ($attemptRecords.Count -eq 0) { "None." } else {
                ($attemptRecords | ConvertTo-Json -Depth 20)
            }
            $evidenceRoot = Join-Path $worktree.path ".profiler-harness-evidence"
            $evidenceStaging = Join-Path $Context.RunDirectory "attempt-$proposalRound-evidence"
            [IO.Directory]::CreateDirectory($evidenceStaging) | Out-Null
            Copy-Item -LiteralPath $baselinePath -Destination (Join-Path $evidenceStaging "capture-result.json")
            Copy-Item -LiteralPath $analysisPath -Destination (Join-Path $evidenceStaging "analysis.md")
            Copy-HarnessDirectoryToTarget $Context $evidenceStaging $evidenceRoot "attempt-$proposalRound-evidence"
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
            $agentResult = Invoke-HarnessAgent $Context $Engine $prompt $worktree.path "attempt-$proposalRound" -Editing -OnTarget
            [IO.File]::WriteAllText((Join-Path $Context.RunDirectory "attempt-$proposalRound-agent.md"), $agentResult.stdout,
                [Text.UTF8Encoding]::new($false))
            $diffPath = Join-Path $Context.RunDirectory "attempt-$proposalRound.patch"
            $diff = Invoke-HarnessTargetPowerShell $Context @"
Remove-Item -LiteralPath '$evidenceRoot' -Recurse -Force
git -C '$($worktree.path)' -c core.hooksPath=NUL add -N -- .
if (`$LASTEXITCODE -ne 0) { throw 'Guest diff staging failed.' }
git -C '$($worktree.path)' -c core.hooksPath=NUL diff --binary --no-ext-diff --no-textconv HEAD
if (`$LASTEXITCODE -ne 0) { throw 'Guest diff extraction failed.' }
"@ 300
            [IO.File]::WriteAllText($diffPath, $diff.stdout, [Text.UTF8Encoding]::new($false))
            if ($diff.stdout -match '(?m)^GIT binary patch$|^Binary files .* differ$') {
                throw "An agent proposed a binary patch. Stop for manual review; only source changes are accepted."
            }
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
            Assert-HarnessGuestBaseline $Context $baselineHead | Out-Null
            $attemptContext = New-HarnessContext $Context.Config $Context.DryRun $Engine
            $attemptContext.GuestAppRepo = $worktree.path
            try {
                Invoke-AppBuildDeploy $attemptContext -SkipSourceStaging
                $candidatePaths = @(Invoke-HarnessCaptureSeries $attemptContext $ScenarioPath "candidate-$attempt")
                Assert-HarnessGuestBaseline $Context $baselineHead | Out-Null
                $candidate = @($candidatePaths | ForEach-Object { Read-HarnessJson $_ })
                $evaluation = Test-HarnessRepeatedImprovement $baseline $candidate $optimization
                if (@($evaluation.comparisons | Where-Object { -not $_.captureValid }).Count -gt 0) {
                    throw "Candidate capture evidence is invalid. Stop and inspect the VM before another deployment."
                }
                $record = [pscustomobject]@{
                    attempt = $attempt
                    proposalRound = $proposalRound
                    status = if ($evaluation.improved) { "verified-improvement" } else { "no-improvement" }
                    signature = $signature
                    patchPath = $diffPath
                    captureResults = $candidatePaths
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
                $attemptRecords.Add($record)
                Write-HarnessJson (Join-Path $Context.RunDirectory "manual-intervention-required.json") $record
                throw
            }
            $attemptRecords.Add($record)
            Write-HarnessJson (Join-Path $Context.RunDirectory "attempts.json") $attemptRecords
            if ($record.status -eq "verified-improvement") {
                Assert-HarnessGuestBaseline $Context $baselineHead | Out-Null
                Write-HarnessJson (Join-Path $Context.RunDirectory "successful-attempt.json") $record
                Write-HarnessJson (Join-Path $Context.RunDirectory "manual-intervention-required.json") ([pscustomobject]@{
                    reason = "Candidate improved consistently across repeated captures. Review the patch manually; it has NOT been applied on the devbox."
                    patchPath = $diffPath
                    checkpointName = $Context.Config.target.checkpointName
                })
                return
            }
        }
        finally {
            Write-HarnessJson (Join-Path $Context.RunDirectory "attempts.json") $attemptRecords
        }
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
        checkpointName = $Context.Config.target.checkpointName
        recovery = "Inspect the guest, then restore the approved VM checkpoint manually. No package reinstall or rollback was attempted."
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
    Assert-HarnessHostArtifactPath $artifactsRoot
    [IO.Directory]::CreateDirectory($artifactsRoot) | Out-Null
    $runId = "{0}-{1}-{2}" -f ([DateTime]::UtcNow.ToString("yyyyMMdd-HHmmss")),
        (($Configuration.name -replace '[^A-Za-z0-9_.-]', '-')),
        ([Guid]::NewGuid().ToString("N").Substring(0, 8))
    $runDirectory = Join-Path $artifactsRoot "runs\$runId"
    [IO.Directory]::CreateDirectory($runDirectory) | Out-Null
    $appRepo = Resolve-HarnessPath ([string]$Configuration.repositories.app.path) $configDirectory
    $profilerRepo = Resolve-HarnessPath ([string]$Configuration.repositories.profiler.path) $configDirectory
    $stageRoot = [string]$Configuration.target.stageRoot
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
        HostMachineGuid = [string](Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Cryptography" -Name MachineGuid -ErrorAction Stop).MachineGuid
        GuestAppRepo = Join-Path $stageRoot "source\app"
        GuestProfilerRepo = Join-Path $stageRoot "source\profiler"
        GuestHarnessRoot = Join-Path $stageRoot "source\profiler\tools\WinUIProfiler\harness"
    }
    Write-HarnessJson (Join-Path $runDirectory "run.json") ([pscustomobject]@{
        schemaVersion = 2
        runId = $runId
        createdUtc = [DateTime]::UtcNow.ToString("O")
        configPath = $Configuration.__configPath
        targetKind = $Configuration.target.kind
        vmId = $Configuration.target.vmId
        guestMachineGuid = $Configuration.target.guestMachineGuid
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
    Ensure-HarnessAgents $Context
    Invoke-ProfilerBuildDeploy $Context -SkipBuild:$SkipBuild | Out-Null
    Invoke-AppBuildDeploy $Context -SkipBuild:$SkipBuild
    if (-not $Context.DryRun) {
        $scenarioPath = Join-Path $Context.RunDirectory "readiness-scenario.json"
        Write-HarnessJson $scenarioPath ([pscustomobject]@{
            schemaVersion = 1
            name = "readiness-first-frame-and-diagnostics"
            steps = @([pscustomobject]@{ action = "wait"; milliseconds = 0 })
        })
        $capturePath = Invoke-HarnessCapture $Context $scenarioPath "readiness"
        Write-HarnessJson (Join-Path $Context.RunDirectory "system-readiness.json") ([pscustomobject]@{
            harnessReady = $true
            vmId = $Context.Config.target.vmId
            guestMachineGuid = $Context.Config.target.guestMachineGuid
            launchAndDiagnosticsEvidence = $capturePath
        })
    }
}

function Get-HarnessPrerequisites {
    return @(
        "Choose IXP VM, local Hyper-V VM (default), or devbox. Devbox execution is disabled.",
        "Provision/boot an isolated VM manually; do not share host drives, credentials, or writable host repositories.",
        "For IXP, use scripts\Initialize-IxpVm.ps1 separately. Initialize IXPTools manually; the harness never imports it.",
        "Complete guest setup, PowerShell 7, interactive sign-in, and repository-specific build prerequisites inside the guest.",
        "Record the Hyper-V VM ID, guest MachineGuid and computer name; verify they are not the devbox identity.",
        "Create a clean post-setup checkpoint and configure its name. Failed deployments require manual checkpoint recovery.",
        "Configure repository paths, user/<alias>/profiler_harness branches, and guest-only command scopes using schemaVersion 2.",
        "Review each build/deploy/reset command. The reset script must restore the same scenario state before every capture.",
        "Authenticate Agency and standalone GitHub Copilot independently in the guest; never copy host tokens.",
        "Verify app launch, loaded runtime compatibility, first-frame evidence, and private snapshot support before scenarios.",
        "Approve VM execution explicitly. Proposed patches are returned for manual review, never applied on the devbox."
    )
}

function Show-HarnessPrerequisites {
    $steps = @(Get-HarnessPrerequisites)
    Write-Host "Harness prerequisites (complete in order before execution):"
    for ($index = 0; $index -lt $steps.Count; $index++) { Write-Host ("{0}. {1}" -f ($index + 1), $steps[$index]) }
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
        [switch]$SkipBuild,
        [ValidateSet("local", "hyperv", "ixp")][string]$TargetKind,
        [switch]$PrerequisitesConfirmed,
        [switch]$ApproveVmExecution
    )

    if ($Stage -ne "validate") { Show-HarnessPrerequisites }
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
    if (-not $TargetKind) {
        $answer = Read-Host "Select target: hyperv (local VM), ixp (IXP VM), or local (devbox; blocked) [hyperv]"
        $TargetKind = if ([string]::IsNullOrWhiteSpace($answer)) { "hyperv" } else { $answer.Trim().ToLowerInvariant() }
    }
    if ($TargetKind -notin @("hyperv", "ixp")) { throw "Devbox execution is disabled. Select hyperv or ixp." }
    if ($TargetKind -ne [string]$config.target.kind) {
        throw "Selected target '$TargetKind' does not match the configuration. Update the VM configuration; no fallback or target substitution is allowed."
    }
    if (-not $DryRun) {
        if (-not $PrerequisitesConfirmed) {
            $checks = @(Get-HarnessPrerequisites)[1..8]
            foreach ($check in $checks) {
                if ($TargetKind -ne "ixp" -and $check.StartsWith("For IXP,")) { continue }
                if ((Read-Host "Confirm $check is complete [yes/y]") -notin @("yes", "y")) {
                    throw "Prerequisite '$check' was not confirmed. No execution started."
                }
            }
        }
        $name = [string]$config.target.$TargetKind.name
        if (-not $ApproveVmExecution -and (Read-Host "Type EXECUTE $name to authorize guest builds, installs and profiling") -cne "EXECUTE $name") {
            throw "VM execution was not approved."
        }
    }
    $context = New-HarnessContext $config $DryRun.IsPresent $Engine
    try {
        Initialize-HarnessTarget $context
        switch ($Stage) {
        "system-ready" {
            Invoke-SystemReady $context -SkipRepositoryPreparation:$SkipRepositoryPreparation -SkipBuild:$SkipBuild
        }
        "app-build-deploy" {
            if (-not $SkipRepositoryPreparation) {
                Initialize-HarnessRepository $config.repositories.app "app" $context
                Initialize-HarnessRepository $config.repositories.profiler "profiler" $context
            }
            if (-not $SkipBuild) { Initialize-HarnessGuestRepository $context "profiler" }
            Invoke-AppBuildDeploy $context -SkipBuild:$SkipBuild
        }
        "profiler-build-deploy" {
            if (-not $SkipRepositoryPreparation) {
                Initialize-HarnessRepository $config.repositories.profiler "profiler" $context
            }
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
    }
    catch {
        Write-HarnessJson (Join-Path $context.RunDirectory "stage-failure.json") ([pscustomobject]@{
            stage = $Stage
            error = $_.Exception.ToString()
            vmId = $config.target.vmId
            checkpointName = $config.target.checkpointName
            recovery = "Stop. Inspect the VM and restore its checkpoint manually if required. No host fallback, DLL replacement, package reinstall, or automatic rollback was attempted."
        })
        throw
    }
    [pscustomobject]@{
        completed = -not $context.DryRun -and $Stage -ne "profile-and-apply-changes"
        planned = $context.DryRun
        stage = $Stage
        runId = $context.RunId
        runDirectory = $context.RunDirectory
        dryRun = $context.DryRun
        manualReviewRequired = $Stage -eq "profile-and-apply-changes"
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
