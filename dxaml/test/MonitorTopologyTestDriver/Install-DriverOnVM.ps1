# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

[CmdletBinding(SupportsShouldProcess, ConfirmImpact = 'High')]
param(
    [Parameter(Mandatory = $true)]
    [string]$VMName,

    [PSCredential]$Credential,

    [string]$DriverPackageDir = (Join-Path $PSScriptRoot 'MonitorTopologyTestDriver\out\x64\Debug\MonitorTopologyTestDriver'),

    [string]$WdkToolsDir = (Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10'),

    [string]$SdkVersion = '10.0.26100.0'
)

$ErrorActionPreference = 'Stop'
$hardwareId = 'Root\MonitorTopologyTest'
$deviceName = 'Monitor Topology Test Virtual Adapter'
$signtool = Join-Path $WdkToolsDir "bin\$SdkVersion\x64\signtool.exe"
$devgen = Join-Path $WdkToolsDir "Tools\$SdkVersion\x64\devgen.exe"

foreach ($file in @($signtool, $devgen)) {
    if (-not (Test-Path -LiteralPath $file -PathType Leaf)) {
        throw "Required WDK tool not found: $file"
    }
}

$package = Resolve-Path -LiteralPath $DriverPackageDir -ErrorAction Stop
foreach ($name in @('MonitorTopologyTestDriver.inf', 'MonitorTopologyTestDriver.dll', 'monitortopologytestdriver.cat')) {
    if (-not (Test-Path -LiteralPath (Join-Path $package $name) -PathType Leaf)) {
        throw "Missing $name in $package. Build the driver package first or specify -DriverPackageDir."
    }
}

$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
    [Security.Principal.WindowsBuiltInRole]::Administrator)
$inHyperVGroup = (whoami /groups 2>$null) -match 'Hyper-V Administrators'
if (-not $isAdmin -and -not $inHyperVGroup) {
    throw 'PowerShell Direct requires local administrator or Hyper-V Administrators membership.'
}

$vm = Get-VM -Name $VMName -ErrorAction Stop
if (@($vm).Count -ne 1 -or $vm.Name -ne $VMName) {
    throw "Specify the exact name of one Hyper-V VM, not a wildcard: '$VMName'."
}
if ($vm.State -ne 'Running') {
    throw "VM '$VMName' must be running."
}

if (-not $PSCmdlet.ShouldProcess($VMName, 'Sign and install the MonitorTopologyTest driver inside this VM')) {
    return
}

if (-not $Credential) {
    $key = $VMName -replace '[^a-zA-Z0-9]', '_'
    $cachedCredential = Join-Path $env:USERPROFILE ".winui-test\vmcred-$key.xml"
    $Credential = if (Test-Path -LiteralPath $cachedCredential) {
        Import-Clixml -LiteralPath $cachedCredential
    } else {
        Get-Credential -Message "Credentials for Hyper-V VM '$VMName'"
    }
}

$session = $null
$stagingDir = $null
$certificate = $null
$remoteDir = "C:\Windows\Temp\WinUI-MonitorTopology-$([guid]::NewGuid().ToString('N'))"
try {
    $session = New-PSSession -VMName $VMName -Credential $Credential

    $existing = Invoke-Command -Session $session -ScriptBlock {
        param($hardwareId)
        $system = Get-CimInstance Win32_ComputerSystem
        $os = Get-CimInstance Win32_OperatingSystem
        if ($system.Manufacturer -ne 'Microsoft Corporation' -or $system.Model -ne 'Virtual Machine') {
            throw 'The destination is not a Hyper-V guest.'
        }
        if ($os.BuildNumber -ne '26660' -or $os.OSArchitecture -notmatch '64') {
            throw "This driver test is qualified only on Windows x64 build 26660; VM has $($os.Version) ($($os.OSArchitecture))."
        }

        $admin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
            [Security.Principal.WindowsBuiltInRole]::Administrator)
        if (-not $admin) {
            throw 'Administrator rights inside the VM are required to install a display driver.'
        }

        $bootConfig = bcdedit /enum '{current}'
        if ($LASTEXITCODE -ne 0 -or ($bootConfig -join "`n") -notmatch 'testsigning\s+Yes') {
            throw 'The VM must already have test signing enabled. This script will not change boot policy.'
        }

        $devices = @(Get-CimInstance Win32_PnPEntity | Where-Object { $_.HardwareID -contains $hardwareId })
        if ($devices.Count -gt 1) {
            throw "Multiple $hardwareId devices exist. Recover the VM before provisioning."
        }
        if ($devices.Count -eq 1) {
            if ($devices[0].ConfigManagerErrorCode -ne 0 -or $devices[0].Name -ne 'Monitor Topology Test Virtual Adapter') {
                throw "Existing $hardwareId device is not the running test adapter (code $($devices[0].ConfigManagerErrorCode))."
            }
            return $devices[0].PNPDeviceID
        }
    } -ArgumentList $hardwareId

    if ($existing) {
        Write-Host "Healthy adapter already exists on '$VMName': $existing. Updating it from the supplied driver package."
    }

    $stagingDir = Join-Path $env:TEMP "WinUI-MonitorTopology-$([guid]::NewGuid().ToString('N'))"
    New-Item -ItemType Directory -Path $stagingDir | Out-Null
    foreach ($name in @('MonitorTopologyTestDriver.inf', 'MonitorTopologyTestDriver.dll', 'monitortopologytestdriver.cat')) {
        Copy-Item -LiteralPath (Join-Path $package $name) -Destination $stagingDir
    }

    # Keep the private signing key on the host, without adding it to a trusted store.
    $certificate = New-SelfSignedCertificate -Type CodeSigningCert `
        -Subject 'CN=WinUI MonitorTopology VM Test' -CertStoreLocation 'Cert:\CurrentUser\My' `
        -KeyExportPolicy NonExportable -HashAlgorithm SHA256 -NotAfter (Get-Date).AddYears(1)
    $catalog = Join-Path $stagingDir 'monitortopologytestdriver.cat'
    & $signtool sign /fd SHA256 /s My /sha1 $certificate.Thumbprint $catalog
    if ($LASTEXITCODE -ne 0) {
        throw "Catalog signing failed (exit $LASTEXITCODE)."
    }
    $certFile = Join-Path $stagingDir 'MonitorTopologyTest.cer'
    [IO.File]::WriteAllBytes($certFile, $certificate.Export(
        [Security.Cryptography.X509Certificates.X509ContentType]::Cert))

    Invoke-Command -Session $session -ScriptBlock {
        param($path)
        New-Item -ItemType Directory -Path $path -ErrorAction Stop | Out-Null
    } -ArgumentList $remoteDir
    foreach ($name in @('MonitorTopologyTestDriver.inf', 'MonitorTopologyTestDriver.dll', 'monitortopologytestdriver.cat', 'MonitorTopologyTest.cer')) {
        Copy-Item -ToSession $session -LiteralPath (Join-Path $stagingDir $name) -Destination $remoteDir
    }
    Copy-Item -ToSession $session -LiteralPath $devgen -Destination (Join-Path $remoteDir 'devgen.exe')

    Invoke-Command -Session $session -ScriptBlock {
        param($path, $hardwareId, $deviceName, $existingDeviceId)
        $admin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
            [Security.Principal.WindowsBuiltInRole]::Administrator)
        if (-not $admin) {
            throw 'Administrator rights inside the VM are required to install a display driver.'
        }

        $certificatePath = Join-Path $path 'MonitorTopologyTest.cer'
        Import-Certificate -FilePath $certificatePath -CertStoreLocation 'Cert:\LocalMachine\Root' | Out-Null
        Import-Certificate -FilePath $certificatePath -CertStoreLocation 'Cert:\LocalMachine\TrustedPublisher' | Out-Null
        $inf = Join-Path $path 'MonitorTopologyTestDriver.inf'
        $driverVersionLine = Get-Content -LiteralPath $inf |
            Where-Object { $_ -match '^\s*DriverVer\s*=' } |
            Select-Object -First 1
        if (-not $driverVersionLine) {
            throw 'The driver INF does not specify DriverVer.'
        }
        $driverVersionFields = (($driverVersionLine -split '=', 2)[1]) -split ',', 2
        if ($driverVersionFields.Count -ne 2) {
            throw "The driver INF has an invalid DriverVer: $driverVersionLine"
        }
        $expectedDriverVersion = [version]::Parse($driverVersionFields[1].Trim())
        & pnputil /add-driver $inf | Out-Host
        if ($LASTEXITCODE -ne 0) {
            throw "Driver staging failed (exit $LASTEXITCODE)."
        }

        $createdId = $null
        $deviceCreated = $false
        try {
            if ($existingDeviceId) {
                $createdId = $existingDeviceId
            } else {
                & (Join-Path $path 'devgen.exe') /add /bus ROOT /instanceid WinUI-MonitorTopology /hardwareid $hardwareId | Out-Host
                if ($LASTEXITCODE -ne 0) {
                    throw "Root device creation failed (exit $LASTEXITCODE)."
                }
                $devices = @(Get-CimInstance Win32_PnPEntity | Where-Object { $_.HardwareID -contains $hardwareId })
                if ($devices.Count -ne 1) {
                    throw "Expected one $hardwareId device after creation; found $($devices.Count)."
                }
                $createdId = $devices[0].PNPDeviceID
                $deviceCreated = $true
            }

            $installOutput = & pnputil /add-driver $inf /install
            $installExitCode = $LASTEXITCODE
            $installOutput | Out-Host
            $packageAlreadyCurrent = $installExitCode -eq 259
            if ($installExitCode -ne 0 -and -not $packageAlreadyCurrent) {
                throw "Driver binding failed (exit $installExitCode)."
            }
            $device = $null
            for ($attempt = 0; $attempt -lt 40; $attempt++) {
                $device = Get-CimInstance Win32_PnPEntity | Where-Object { $_.PNPDeviceID -eq $createdId }
                if ($device -and $device.ConfigManagerErrorCode -eq 0 -and $device.Name -eq $deviceName) {
                    break
                }
                Start-Sleep -Milliseconds 500
            }
            if (-not $device -or $device.ConfigManagerErrorCode -ne 0 -or $device.Name -ne $deviceName) {
                throw "The adapter did not start successfully: $createdId ($($device.ConfigManagerErrorCode))."
            }
            $installedDrivers = @(Get-CimInstance Win32_PnPSignedDriver | Where-Object { $_.DeviceID -eq $createdId })
            if ($installedDrivers.Count -ne 1) {
                throw "Expected one installed driver record for $createdId; found $($installedDrivers.Count)."
            }
            $installedDriverVersion = [version]::Parse($installedDrivers[0].DriverVersion)
            if ($installedDriverVersion -ne $expectedDriverVersion) {
                throw "The adapter is using driver version $installedDriverVersion; package requires $expectedDriverVersion."
            }
            $action = if ($existingDeviceId -and $packageAlreadyCurrent) {
                'Already current'
            } elseif ($existingDeviceId) {
                'Updated'
            } else {
                'Installed'
            }
            Write-Host "$action and started $deviceName on VM: $createdId"
        } catch {
            if ($deviceCreated) {
                & pnputil /remove-device $createdId | Out-Host
                if ($LASTEXITCODE -ne 0) {
                    throw "Driver installation failed; removal of $createdId also failed. Restore the VM checkpoint. $_"
                }
            }
            throw
        }
    } -ArgumentList $remoteDir, $hardwareId, $deviceName, $existing
} finally {
    if ($session) {
        try {
            Invoke-Command -Session $session -ScriptBlock {
                param($path)
                if (Test-Path -LiteralPath $path) {
                    Remove-Item -LiteralPath $path -Recurse -Force -ErrorAction Stop
                }
            } -ArgumentList $remoteDir -ErrorAction Stop
        } catch {
            Write-Warning "Could not remove temporary driver files from '$VMName' at '$remoteDir': $_"
        }
        Remove-PSSession $session
    }
    if ($certificate) {
        Remove-Item -LiteralPath "Cert:\CurrentUser\My\$($certificate.Thumbprint)" -DeleteKey
    }
    if ($stagingDir) {
        Remove-Item -LiteralPath $stagingDir -Recurse -Force
    }
}
