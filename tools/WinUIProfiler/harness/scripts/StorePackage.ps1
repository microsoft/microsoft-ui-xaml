[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateSet("Deploy", "Stop", "Restore", "Status")]
    [string]$Action,

    [Parameter(Mandatory)]
    [string]$RepoRoot,

    [string]$Configuration = "Release",

    [ValidateSet("x64", "ARM64")]
    [string]$Platform = "x64",

    [string]$TargetFramework = "net10.0-windows10.0.28000.0",

    [Parameter(Mandatory)]
    [string]$StatePath,

    [string]$SigningCertificatePath,

    [switch]$AllowMissingOriginalPackage
)

$ErrorActionPreference = "Stop"
$packageName = "Microsoft.WindowsStore"
$packagePublisher = "CN=Microsoft Corporation, O=Microsoft Corporation, L=Redmond, S=Washington, C=US"
$securityManagerPath = "HKLM:\SOFTWARE\Microsoft\SecurityManager"
$internalDevUnlockName = "InternalDevUnlock"
$developerSettings = @(
    [pscustomobject]@{
        path = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock"
        name = "AllowDevelopmentWithoutDevLicense"
        value = 1
    },
    [pscustomobject]@{
        path = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock"
        name = "AllowAllTrustedApps"
        value = 1
    },
    [pscustomobject]@{
        path = "HKLM:\SOFTWARE\Policies\Microsoft\Windows\Appx"
        name = "AllowAllTrustedApps"
        value = 1
    }
)

function Assert-Administrator {
    $principal = [Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
    if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        throw "Store package operations must run from an elevated PowerShell session."
    }
}

function Stop-StoreProcesses {
    $processNames = @("WinStore.App", "StoreDesktopExtension", "store")
    Get-Process -ErrorAction SilentlyContinue |
        Where-Object { $_.ProcessName -in $processNames } |
        ForEach-Object {
            Stop-Process -Id $_.Id -Force -ErrorAction Stop
        }
}

function Get-LatestStoreMsix {
    $appPackages = Join-Path $RepoRoot "bin\$Configuration\WinStore2\$TargetFramework\$Platform\AppPackages"
    if (-not (Test-Path -LiteralPath $appPackages -PathType Container)) {
        throw "Store AppPackages directory was not found: '$appPackages'. Run the app build first."
    }

    $msix = Get-ChildItem -LiteralPath $appPackages -Filter "*.msix" -File -Recurse |
        Sort-Object LastWriteTimeUtc -Descending |
        Select-Object -First 1
    if ($null -eq $msix) {
        throw "No Store MSIX was found below '$appPackages'."
    }
    return $msix
}

function Get-StoreSigningCertificatePath {
    $path = $SigningCertificatePath
    if ([string]::IsNullOrWhiteSpace($path)) {
        $path = Join-Path $RepoRoot "StorePackageSigner.cer"
    }
    elseif (-not [IO.Path]::IsPathRooted($path)) {
        $path = Join-Path $RepoRoot $path
    }

    $fullPath = [IO.Path]::GetFullPath($path)
    if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
        throw "Store package signing certificate was not found: '$fullPath'. Prepare a signed Store payload first."
    }
    return $fullPath
}

function Remove-TrustedSigningCertificate {
    param([Parameter(Mandatory)][string]$Thumbprint)

    $store = [Security.Cryptography.X509Certificates.X509Store]::new(
        [Security.Cryptography.X509Certificates.StoreName]::TrustedPeople,
        [Security.Cryptography.X509Certificates.StoreLocation]::LocalMachine)
    try {
        $store.Open([Security.Cryptography.X509Certificates.OpenFlags]::ReadWrite)
        $matches = $store.Certificates.Find(
            [Security.Cryptography.X509Certificates.X509FindType]::FindByThumbprint,
            $Thumbprint,
            $false)
        foreach ($certificate in $matches) {
            $store.Remove($certificate)
        }
    }
    finally {
        $store.Close()
    }
}

function Install-StoreSigningCertificate {
    param(
        [Parameter(Mandatory)]$State,
        [Parameter(Mandatory)][IO.FileInfo]$Msix,
        [Parameter(Mandatory)][string]$CertificatePath
    )

    $certificate = [Security.Cryptography.X509Certificates.X509Certificate2]::new($CertificatePath)
    if ($certificate.Subject -ne $packagePublisher) {
        throw "Store signing certificate publisher '$($certificate.Subject)' does not match '$packagePublisher'."
    }

    try {
        $signedCertificate =
            [Security.Cryptography.X509Certificates.X509Certificate]::CreateFromSignedFile($Msix.FullName)
        $signerCertificate =
            [Security.Cryptography.X509Certificates.X509Certificate2]::new($signedCertificate)
    }
    catch {
        throw "Store MSIX '$($Msix.FullName)' does not contain a readable signing certificate. $($_.Exception.Message)"
    }
    if ($signerCertificate.Thumbprint -ne $certificate.Thumbprint) {
        throw "Store MSIX '$($Msix.FullName)' is not signed by '$CertificatePath'."
    }

    $previous = $State.packageSigningCertificate
    if ($null -ne $previous -and
        -not [string]::IsNullOrWhiteSpace([string]$previous.thumbprint) -and
        [string]$previous.thumbprint -ne $certificate.Thumbprint -and
        $previous.importedByHarness -eq $true -and
        $previous.wasTrusted -ne $true) {
        Remove-TrustedSigningCertificate -Thumbprint ([string]$previous.thumbprint)
    }

    $store = [Security.Cryptography.X509Certificates.X509Store]::new(
        [Security.Cryptography.X509Certificates.StoreName]::TrustedPeople,
        [Security.Cryptography.X509Certificates.StoreLocation]::LocalMachine)
    try {
        $store.Open([Security.Cryptography.X509Certificates.OpenFlags]::ReadWrite)
        $trustedCertificates = $store.Certificates.Find(
            [Security.Cryptography.X509Certificates.X509FindType]::FindByThumbprint,
            $certificate.Thumbprint,
            $false)
        $alreadyTrusted = $trustedCertificates.Count -gt 0
        if (-not $alreadyTrusted) {
            $store.Add($certificate)
        }
    }
    finally {
        $store.Close()
    }
    $samePreviousCertificate =
        $null -ne $previous -and
        [string]$previous.thumbprint -eq $certificate.Thumbprint
    $wasTrusted = if ($samePreviousCertificate) {
        $previous.wasTrusted -eq $true
    }
    else {
        $alreadyTrusted
    }
    $importedByHarness = if ($samePreviousCertificate) {
        $previous.importedByHarness -eq $true
    }
    else {
        -not $alreadyTrusted
    }

    $State | Add-Member -NotePropertyName packageSigningCertificate -NotePropertyValue ([pscustomobject]@{
        thumbprint = $certificate.Thumbprint
        subject = $certificate.Subject
        wasTrusted = $wasTrusted
        importedByHarness = $importedByHarness
    }) -Force
    $State.schemaVersion = 6
    Write-StoreState $State
    return $certificate
}

function Restore-StoreSigningCertificate {
    param([Parameter(Mandatory)]$State)

    $certificate = $State.packageSigningCertificate
    if ($null -eq $certificate) {
        return
    }
    if ($certificate.importedByHarness -eq $true -and $certificate.wasTrusted -ne $true) {
        Remove-TrustedSigningCertificate -Thumbprint ([string]$certificate.thumbprint)
    }
}

function Get-InternalDevUnlockState {
    $registryKey = Get-Item -LiteralPath $securityManagerPath -ErrorAction SilentlyContinue
    $valueNames = if ($null -eq $registryKey) { @() } else { @($registryKey.GetValueNames()) }
    $valueExists = $internalDevUnlockName -in $valueNames
    [pscustomobject]@{
        existed = $valueExists
        value = if ($valueExists) { [int]$registryKey.GetValue($internalDevUnlockName) } else { $null }
    }
}

function Get-RegistryValueState {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$Name
    )

    $registryKey = Get-Item -LiteralPath $Path -ErrorAction SilentlyContinue
    $valueNames = if ($null -eq $registryKey) { @() } else { @($registryKey.GetValueNames()) }
    $valueExists = $Name -in $valueNames
    [pscustomobject]@{
        path = $Path
        name = $Name
        existed = $valueExists
        value = if ($valueExists) { [int]$registryKey.GetValue($Name) } else { $null }
    }
}

function Get-DeveloperSettingsState {
    @(
        $developerSettings | ForEach-Object {
            Get-RegistryValueState -Path $_.path -Name $_.name
        }
    )
}

function Read-StoreState {
    if (-not (Test-Path -LiteralPath $StatePath -PathType Leaf)) {
        throw "Original Store package state was not found at '$StatePath'."
    }
    return Get-Content -LiteralPath $StatePath -Raw -Encoding UTF8 | ConvertFrom-Json
}

function Write-StoreState {
    param([Parameter(Mandatory)]$State)

    [IO.Directory]::CreateDirectory((Split-Path -Parent $StatePath)) | Out-Null
    [IO.File]::WriteAllText(
        $StatePath,
        ($State | ConvertTo-Json -Depth 10),
        [Text.UTF8Encoding]::new($false)
    )
}

function Save-OriginalPackageState {
    if (Test-Path -LiteralPath $StatePath -PathType Leaf) {
        $state = Read-StoreState
        $changed = $false
        if ($null -eq $state.internalDevUnlock) {
            $state | Add-Member -NotePropertyName internalDevUnlock -NotePropertyValue (Get-InternalDevUnlockState)
            $changed = $true
        }
        if ($null -eq $state.capabilityPolicyRebootRequired) {
            $currentUnlock = Get-InternalDevUnlockState
            $legacyUnlockNeedsReboot =
                -not $state.internalDevUnlock.existed -and
                $currentUnlock.existed -and
                $currentUnlock.value -eq 1
            $state | Add-Member -NotePropertyName capabilityPolicyRebootRequired `
                -NotePropertyValue $legacyUnlockNeedsReboot
            $state | Add-Member -NotePropertyName internalDevUnlockEnabledUtc `
                -NotePropertyValue $(if ($legacyUnlockNeedsReboot) { [DateTime]::UtcNow.ToString("O") } else { $null })
            $changed = $true
        }
        if ($null -eq $state.wasInstalled) {
            $state | Add-Member -NotePropertyName wasInstalled `
                -NotePropertyValue (-not [string]::IsNullOrWhiteSpace([string]$state.packageFullName))
            $changed = $true
        }
        if ($null -eq $state.developerSettings) {
            $state | Add-Member -NotePropertyName developerSettings -NotePropertyValue @(Get-DeveloperSettingsState)
            $state | Add-Member -NotePropertyName developerPolicyRebootRequired -NotePropertyValue $false
            $state | Add-Member -NotePropertyName developerSettingsEnabledUtc -NotePropertyValue $null
            $changed = $true
        }
        if ("packageSigningCertificate" -notin $state.PSObject.Properties.Name) {
            $state | Add-Member -NotePropertyName packageSigningCertificate -NotePropertyValue $null
            $changed = $true
        }
        if ($changed -or $state.schemaVersion -ne 6) {
            $state.schemaVersion = 6
            Write-StoreState $state
        }
        return $state
    }

    $package = Get-AppxPackage -Name $packageName -ErrorAction SilentlyContinue
    if ($null -eq $package) {
        if (-not $AllowMissingOriginalPackage) {
            throw "Microsoft Store is not registered for the current user, and no prior harness state exists at '$StatePath'. Recover Store before deploying a local build."
        }
        $state = [pscustomobject]@{
            schemaVersion = 6
            capturedUtc = [DateTime]::UtcNow.ToString("O")
            name = $packageName
            packageFullName = $null
            version = $null
            architecture = $null
            installLocation = $null
            wasInstalled = $false
            internalDevUnlock = Get-InternalDevUnlockState
            capabilityPolicyRebootRequired = $false
            internalDevUnlockEnabledUtc = $null
            developerSettings = @(Get-DeveloperSettingsState)
            developerPolicyRebootRequired = $false
            developerSettingsEnabledUtc = $null
            packageSigningCertificate = $null
        }
        Write-StoreState $state
        return $state
    }

    $state = [pscustomobject]@{
        schemaVersion = 6
        capturedUtc = [DateTime]::UtcNow.ToString("O")
        name = $package.Name
        packageFullName = $package.PackageFullName
        version = $package.Version.ToString()
        architecture = $package.Architecture.ToString()
        installLocation = $package.InstallLocation
        wasInstalled = $true
        internalDevUnlock = Get-InternalDevUnlockState
        capabilityPolicyRebootRequired = $false
        internalDevUnlockEnabledUtc = $null
        developerSettings = @(Get-DeveloperSettingsState)
        developerPolicyRebootRequired = $false
        developerSettingsEnabledUtc = $null
        packageSigningCertificate = $null
    }
    Write-StoreState $state
    return $state
}

function Enable-InternalDevUnlock {
    param([Parameter(Mandatory)]$State)

    $current = Get-InternalDevUnlockState
    if ($current.existed -and $current.value -eq 1) {
        return $false
    }

    if (-not (Test-Path -LiteralPath $securityManagerPath)) {
        New-Item -Path $securityManagerPath -Force | Out-Null
    }
    New-ItemProperty `
        -LiteralPath $securityManagerPath `
        -Name $internalDevUnlockName `
        -PropertyType DWord `
        -Value 1 `
        -Force |
        Out-Null

    $State | Add-Member -NotePropertyName internalDevUnlockEnabledUtc `
        -NotePropertyValue ([DateTime]::UtcNow.ToString("O")) -Force
    $State | Add-Member -NotePropertyName capabilityPolicyRebootRequired `
        -NotePropertyValue $true -Force
    $State.schemaVersion = 6
    Write-StoreState $State
    return $true
}

function Enable-DeveloperSettings {
    param([Parameter(Mandatory)]$State)

    $changed = $false
    foreach ($setting in $developerSettings) {
        $current = Get-RegistryValueState -Path $setting.path -Name $setting.name
        if ($current.existed -and $current.value -eq $setting.value) {
            continue
        }
        if (-not (Test-Path -LiteralPath $setting.path)) {
            New-Item -Path $setting.path -Force | Out-Null
        }
        New-ItemProperty `
            -LiteralPath $setting.path `
            -Name $setting.name `
            -PropertyType DWord `
            -Value $setting.value `
            -Force |
            Out-Null
        $changed = $true
    }

    if ($changed) {
        $State | Add-Member -NotePropertyName developerSettingsEnabledUtc `
            -NotePropertyValue ([DateTime]::UtcNow.ToString("O")) -Force
        $State | Add-Member -NotePropertyName developerPolicyRebootRequired `
            -NotePropertyValue $true -Force
        $State.schemaVersion = 6
        Write-StoreState $State
    }
    return $changed
}

function Confirm-DeveloperSettingsReady {
    param([Parameter(Mandatory)]$State)

    if ($State.developerPolicyRebootRequired -ne $true) {
        return
    }

    $enabledUtc = [DateTime]::Parse(
        [string]$State.developerSettingsEnabledUtc,
        [Globalization.CultureInfo]::InvariantCulture,
        [Globalization.DateTimeStyles]::RoundtripKind)
    $lastBootUtc = (Get-CimInstance Win32_OperatingSystem).LastBootUpTime.ToUniversalTime()
    if ($lastBootUtc -le $enabledUtc.ToUniversalTime()) {
        throw "Developer Mode and sideloading policies were enabled. Reboot Windows, then rerun the same harness command. No package is removed by this readiness check."
    }

    $State.developerPolicyRebootRequired = $false
    $State | Add-Member -NotePropertyName developerPolicyReadyBootUtc `
        -NotePropertyValue $lastBootUtc.ToString("O") -Force
    Write-StoreState $State
}

function Confirm-CapabilityPolicyReady {
    param([Parameter(Mandatory)]$State)

    if ($State.capabilityPolicyRebootRequired -ne $true) {
        return
    }

    $enabledUtc = [DateTime]::Parse(
        [string]$State.internalDevUnlockEnabledUtc,
        [Globalization.CultureInfo]::InvariantCulture,
        [Globalization.DateTimeStyles]::RoundtripKind)
    $lastBootUtc = (Get-CimInstance Win32_OperatingSystem).LastBootUpTime.ToUniversalTime()
    if ($lastBootUtc -le $enabledUtc.ToUniversalTime()) {
        throw "InternalDevUnlock was enabled for Store's internal capabilities. Reboot Windows, then rerun the same harness command. No package is removed by this readiness check."
    }

    $State.capabilityPolicyRebootRequired = $false
    $State | Add-Member -NotePropertyName capabilityPolicyReadyBootUtc `
        -NotePropertyValue $lastBootUtc.ToString("O") -Force
    Write-StoreState $State
}

function Restore-DeveloperSettings {
    param([Parameter(Mandatory)]$State)

    foreach ($setting in @($State.developerSettings)) {
        if ($setting.existed) {
            if (-not (Test-Path -LiteralPath $setting.path)) {
                New-Item -Path $setting.path -Force | Out-Null
            }
            New-ItemProperty `
                -LiteralPath $setting.path `
                -Name $setting.name `
                -PropertyType DWord `
                -Value ([int]$setting.value) `
                -Force |
                Out-Null
        }
        else {
            Remove-ItemProperty `
                -LiteralPath $setting.path `
                -Name $setting.name `
                -ErrorAction SilentlyContinue
        }
    }
}

function Restore-InternalDevUnlock {
    param([Parameter(Mandatory)]$State)

    if ($null -eq $State.internalDevUnlock) {
        return
    }

    if ($State.internalDevUnlock.existed) {
        if (-not (Test-Path -LiteralPath $securityManagerPath)) {
            New-Item -Path $securityManagerPath -Force | Out-Null
        }
        New-ItemProperty `
            -LiteralPath $securityManagerPath `
            -Name $internalDevUnlockName `
            -PropertyType DWord `
            -Value ([int]$State.internalDevUnlock.value) `
            -Force |
            Out-Null
    }
    else {
        Remove-ItemProperty `
            -LiteralPath $securityManagerPath `
            -Name $internalDevUnlockName `
            -ErrorAction SilentlyContinue
    }
}

function Remove-RegisteredStorePackages {
    $packages = @(Get-AppxPackage -AllUsers -Name $packageName -ErrorAction SilentlyContinue)
    foreach ($package in $packages) {
        Remove-AppxPackage -Package $package.PackageFullName -AllUsers -ErrorAction Stop
    }

    $deadline = [DateTime]::UtcNow.AddSeconds(60)
    do {
        $remaining = @(Get-AppxPackage -AllUsers -Name $packageName -ErrorAction SilentlyContinue)
        if ($remaining.Count -eq 0) {
            return
        }
        Start-Sleep -Seconds 2
    } while ([DateTime]::UtcNow -lt $deadline)

    $names = ($remaining.PackageFullName -join ", ")
    throw "Store packages remain registered after removal: $names"
}

function Wait-ForStoreRegistration {
    param([int]$TimeoutSeconds = 300)

    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        $package = Get-AppxPackage -Name $packageName -ErrorAction SilentlyContinue
        if ($null -ne $package) {
            return $package
        }
        Start-Sleep -Seconds 5
    } while ([DateTime]::UtcNow -lt $deadline)

    throw "Microsoft Store recovery did not register a package within $TimeoutSeconds seconds."
}

function Restore-SystemStore {
    param([Parameter(Mandatory)]$State)

    if ($State.wasInstalled -eq $false) {
        Stop-StoreProcesses
        Remove-RegisteredStorePackages
        Restore-StoreSigningCertificate $State
        Restore-InternalDevUnlock $State
        Restore-DeveloperSettings $State
        $State | Add-Member -NotePropertyName restoredUtc `
            -NotePropertyValue ([DateTime]::UtcNow.ToString("O")) -Force
        Write-StoreState $State
        return
    }

    Stop-StoreProcesses
    $service = Get-Service -Name InstallService -ErrorAction SilentlyContinue
    if ($null -ne $service -and $service.Status -ne "Stopped") {
        Stop-Service -Name InstallService -Force -ErrorAction Stop
    }

    Get-AppxPackage -Name $packageName -ErrorAction SilentlyContinue |
        Remove-AppxPackage -ErrorAction Stop
    Start-Service -Name InstallService -ErrorAction SilentlyContinue

    $wsreset = Join-Path $env:SystemRoot "System32\wsreset.exe"
    $process = Start-Process -FilePath $wsreset -ArgumentList "-i" -PassThru
    $process.WaitForExit(60000) | Out-Null
    $package = Wait-ForStoreRegistration

    Restore-StoreSigningCertificate $State
    Restore-InternalDevUnlock $State
    Restore-DeveloperSettings $State
    $State | Add-Member -NotePropertyName restoredUtc `
        -NotePropertyValue ([DateTime]::UtcNow.ToString("O")) -Force
    Write-StoreState $State
    return $package
}

switch ($Action) {
    "Deploy" {
        Assert-Administrator
        $state = Save-OriginalPackageState
        $capabilityChanged = Enable-InternalDevUnlock $state
        $developerPolicyChanged = Enable-DeveloperSettings $state
        if ($capabilityChanged -or $developerPolicyChanged) {
            throw "Store capability, Developer Mode, or sideloading policies were enabled. Reboot Windows, then rerun the same harness command. The installed Store package was not changed."
        }
        Confirm-CapabilityPolicyReady $state
        Confirm-DeveloperSettingsReady $state

        $msix = Get-LatestStoreMsix
        $certificatePath = Get-StoreSigningCertificatePath
        $certificate = Install-StoreSigningCertificate $state $msix $certificatePath
        Stop-StoreProcesses
        $service = Get-Service -Name InstallService -ErrorAction SilentlyContinue
        $restartService = $null -ne $service -and $service.Status -eq "Running"
        try {
            if ($null -ne $service -and $service.Status -ne "Stopped") {
                Stop-Service -Name InstallService -Force -ErrorAction Stop
            }
            Remove-RegisteredStorePackages
            Add-AppxPackage -Path $msix.FullName -ForceApplicationShutdown -ErrorAction Stop
        }
        finally {
            if ($restartService) {
                Start-Service -Name InstallService -ErrorAction SilentlyContinue
            }
        }

        $installed = Get-AppxPackage -Name $packageName -ErrorAction Stop
        if ($installed.IsDevelopmentMode -or $installed.SignatureKind.ToString() -eq "None") {
            throw "Store was not installed as a signed MSIX package. SignatureKind='$($installed.SignatureKind)', IsDevelopmentMode='$($installed.IsDevelopmentMode)'."
        }
        [pscustomobject]@{
            action = $Action
            deployment = "signed-msix"
            sourceMsix = $msix.FullName
            signerThumbprint = $certificate.Thumbprint
            packageFullName = $installed.PackageFullName
            installLocation = $installed.InstallLocation
            signatureKind = $installed.SignatureKind.ToString()
            isDevelopmentMode = $installed.IsDevelopmentMode
            statePath = $StatePath
        }
    }
    "Stop" {
        Stop-StoreProcesses
    }
    "Restore" {
        Assert-Administrator
        $state = Read-StoreState
        Confirm-CapabilityPolicyReady $state
        Restore-SystemStore $state |
            Select-Object Name, PackageFullName, Version, Architecture, InstallLocation, SignatureKind, Status
    }
    "Status" {
        $state = if (Test-Path -LiteralPath $StatePath -PathType Leaf) { Read-StoreState } else { $null }
        $package = Get-AppxPackage -Name $packageName -ErrorAction SilentlyContinue
        [pscustomobject]@{
            installed = $null -ne $package
            packageFullName = if ($null -eq $package) { $null } else { $package.PackageFullName }
            version = if ($null -eq $package) { $null } else { $package.Version.ToString() }
            installLocation = if ($null -eq $package) { $null } else { $package.InstallLocation }
            signatureKind = if ($null -eq $package) { $null } else { $package.SignatureKind.ToString() }
            status = if ($null -eq $package) { $null } else { $package.Status.ToString() }
            originalPackageWasInstalled = if ($null -eq $state) { $null } else { $state.wasInstalled }
            capabilityPolicyRebootRequired = $null -ne $state -and $state.capabilityPolicyRebootRequired -eq $true
            developerPolicyRebootRequired = $null -ne $state -and $state.developerPolicyRebootRequired -eq $true
            packageSigningCertificateThumbprint = if ($null -eq $state) {
                $null
            }
            else {
                $state.packageSigningCertificate.thumbprint
            }
            statePath = $StatePath
        }
    }
}
