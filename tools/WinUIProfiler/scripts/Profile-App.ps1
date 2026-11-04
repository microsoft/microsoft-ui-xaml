#requires -Version 7.4
<#
.SYNOPSIS
Profiles a WinUI 3 app that loads private builds of Microsoft.UI.Xaml.dll and Microsoft.UI.Xaml.Controls.dll.

.DESCRIPTION
Stages the two DLLs beside links to the app's own WinUI runtime files, then opens WinUI Profiler. The profiler
launches the app through the Detours launcher (WinUIProfiler.Launcher), so the app loads the staged DLLs, and
profiles it from process start. When the profiler is closed, the script closes the app if it is still running,
deletes the staged DLLs and returns.

The script asks for administrator rights, which live ETW and the runtime links need. The app still runs without
them. Exit codes: 0 when the app was launched with the private DLLs and the staging was removed, 4 when the UAC
prompt was declined, 1 otherwise.

.PARAMETER MuxPath
The private Microsoft.UI.Xaml.dll. It must match the app's architecture and Windows App SDK runtime.

.PARAMETER MuxcPath
The private Microsoft.UI.Xaml.Controls.dll built with it.

.PARAMETER ExePath
An unpackaged app's executable.

.PARAMETER Arguments
The unpackaged app's command-line arguments.

.PARAMETER Aumid
A full-trust packaged app's AUMID, PackageFamilyName!ApplicationId (see Get-StartApps).

.PARAMETER MuxResourceDirectory
The folder of the WinUI runtime that the app normally loads, when the script cannot find it: for example, for an
unpackaged app that uses the Windows App SDK framework package.

.PARAMETER ProfilerPath
The WinUIProfiler.App.exe to use. By default, this checkout's Release build for this machine's architecture,
built by Build-App.ps1 when it is missing.

.PARAMETER Build
Rebuilds the profiler with Build-App.ps1 first.

.EXAMPLE
.\scripts\Profile-App.ps1 -MuxPath D:\out\Microsoft.UI.Xaml.dll -MuxcPath D:\out\Microsoft.UI.Xaml.Controls.dll -ExePath C:\Apps\MyApp\MyApp.exe

.EXAMPLE
.\scripts\Profile-App.ps1 -MuxPath D:\out\Microsoft.UI.Xaml.dll -MuxcPath D:\out\Microsoft.UI.Xaml.Controls.dll -Aumid 'Microsoft.WinUI3ControlsGallery_8wekyb3d8bbwe!App'
#>
[CmdletBinding(DefaultParameterSetName = 'Exe')]
param(
    [Parameter(Mandatory)][string] $MuxPath,
    [Parameter(Mandatory)][string] $MuxcPath,
    [Parameter(Mandatory, ParameterSetName = 'Exe')][string] $ExePath,
    [Parameter(ParameterSetName = 'Exe')][string] $Arguments,
    [Parameter(Mandatory, ParameterSetName = 'Aumid')][string] $Aumid,
    [string] $MuxResourceDirectory,
    [string] $ProfilerPath,
    [switch] $Build,
    # Set when the script relaunches itself with administrator rights.
    [Parameter(DontShow)][string] $ElevatedLogPath
)
$ErrorActionPreference = 'Stop'
$script:ExitCode = 1
$Mux = 'Microsoft.UI.Xaml.dll'
$Muxc = 'Microsoft.UI.Xaml.Controls.dll'

function Get-LocalPath([string] $Path) {
    $full = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Path)
    if ($full -notmatch '^[A-Za-z]:\\' -or $full.Substring(2).Contains(':')) {
        throw "Use a local filesystem path: $Path"
    }
    for ($item = [IO.DirectoryInfo]::new($full); $null -ne $item; $item = $item.Parent) {
        if ((Test-Path -LiteralPath $item.FullName) -and
            ((Get-Item -LiteralPath $item.FullName -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Reparse-point paths are not supported: $($item.FullName)"
        }
    }
    return [IO.Path]::TrimEndingDirectorySeparator($full)
}

function Get-PeMachine([string] $Path) {
    $reader = [IO.BinaryReader]::new([IO.File]::OpenRead($Path))
    try {
        if ($reader.ReadUInt16() -ne 0x5A4D) { throw "Not a Windows binary: $Path" }
        $reader.BaseStream.Position = 0x3C
        $reader.BaseStream.Position = $reader.ReadInt32()
        if ($reader.ReadUInt32() -ne 0x4550) { throw "Invalid PE header: $Path" }
        return $reader.ReadUInt16()
    }
    finally { $reader.Dispose() }
}

function Get-FileIdentity([string] $Path) {
    if (-not ('ProfileAppFileIdentity' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

public static class ProfileAppFileIdentity {
    [StructLayout(LayoutKind.Sequential)]
    private struct FileId {
        public ulong Volume;
        public ulong Low;
        public ulong High;
    }
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool GetFileInformationByHandleEx(SafeFileHandle file,
        int informationClass, out FileId information, uint size);
    public static string Read(string path) {
        using (var file = File.OpenHandle(path, FileMode.Open, FileAccess.Read,
            FileShare.ReadWrite | FileShare.Delete)) {
            FileId id;
            if (!GetFileInformationByHandleEx(file, 18, out id, (uint)Marshal.SizeOf<FileId>()))
                throw new Win32Exception(Marshal.GetLastWin32Error());
            return $"{id.Volume:x16}:{id.Low:x16}:{id.High:x16}";
        }
    }
}
'@
    }
    return [ProfileAppFileIdentity]::Read($Path)
}

function Test-SameFile([string] $First, [string] $Second) {
    if ([string]::IsNullOrEmpty($First)) { return $false }
    if ([string]::Equals([IO.Path]::GetFullPath($First), [IO.Path]::GetFullPath($Second), [StringComparison]::OrdinalIgnoreCase)) {
        return $true
    }
    try { return (Get-FileIdentity $First) -ceq (Get-FileIdentity $Second) }
    catch { return $false }
}

function Test-Administrator {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    try {
        return [Security.Principal.WindowsPrincipal]::new($identity).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
    }
    finally { $identity.Dispose() }
}

function Get-ProfilerExecutable([string] $Root, [string] $Platform, [bool] $Rebuild) {
    $outputFile = Join-Path $Root "WinUIProfiler.App\obj\ProfilerAppOutput.$Platform.Release.txt"
    $executable = if (Test-Path -LiteralPath $outputFile -PathType Leaf) {
        Join-Path (Get-Content -LiteralPath $outputFile -Raw).Trim() 'WinUIProfiler.App.exe'
    }
    if ($Rebuild -or -not $executable -or -not (Test-Path -LiteralPath $executable -PathType Leaf) -or
        -not (Test-Path -LiteralPath (Join-Path (Split-Path -Parent $executable) 'NativeLauncher\manifest.json') -PathType Leaf)) {
        Write-Host "Building WinUI Profiler (Release, $Platform) with Build-App.ps1."
        & (Join-Path $PSScriptRoot 'Build-App.ps1') -Configuration Release -Platform $Platform | Out-Host
        $executable = Join-Path (Get-Content -LiteralPath $outputFile -Raw).Trim() 'WinUIProfiler.App.exe'
    }
    return Get-LocalPath $executable
}

function Get-PrivateDll([string] $Path, [string[]] $OriginalNames, [int] $Machine) {
    $Path = Get-LocalPath $Path
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Private DLL not found: $Path" }
    $version = [Diagnostics.FileVersionInfo]::GetVersionInfo($Path)
    if ($version.OriginalFilename -notin $OriginalNames) {
        throw "'$Path' is not a $($OriginalNames[0]) build; its original file name is '$($version.OriginalFilename)'."
    }
    $dllMachine = Get-PeMachine $Path
    if ($dllMachine -ne $Machine) {
        throw ("'{0}' has PE machine 0x{1:X4}, but the app has PE machine 0x{2:X4}. Use DLLs built for the app's architecture." -f
            $Path, $dllMachine, $Machine)
    }
    return @{ path = $Path; sha256 = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash; fileVersion = $version.FileVersion }
}

function Get-PackagedApp([string] $Aumid) {
    if ($Aumid -notmatch '^([^!]+)!([^!]+)$') { throw 'The AUMID must be PackageFamilyName!ApplicationId.' }
    $family, $applicationId = $Matches[1], $Matches[2]
    Import-Module Appx -UseWindowsPowerShell -WarningAction SilentlyContinue
    $packages = @(Get-AppxPackage | Where-Object PackageFamilyName -eq $family)
    if ($packages.Count -ne 1) { throw "The AUMID must identify exactly one package installed for this user: $Aumid" }
    $directory = Get-LocalPath $packages[0].InstallLocation
    $dependencies = @($packages[0].Dependencies | ForEach-Object { [string]$_ })
    $dependencyDirectories = @(Get-AppxPackage -PackageTypeFilter Framework |
        Where-Object { $_.PackageFullName -in $dependencies } | ForEach-Object InstallLocation)
    [xml] $manifest = Get-Content -LiteralPath (Join-Path $directory 'AppxManifest.xml') -Raw
    $applications = @($manifest.Package.Applications.Application | Where-Object Id -eq $applicationId)
    if ($applications.Count -ne 1 -or
        -not $applications[0].GetAttribute('Executable').EndsWith('.exe', [StringComparison]::OrdinalIgnoreCase)) {
        throw "The package has no desktop application '$applicationId'."
    }
    $application = $applications[0]
    if ($application.GetAttribute('EntryPoint') -ne 'Windows.FullTrustApplication' -and
        @($application.Attributes | Where-Object { $_.LocalName -eq 'TrustLevel' -and $_.Value -eq 'mediumIL' }).Count -eq 0) {
        throw 'UWP and AppContainer apps cannot load private WinUI DLLs; use a full-trust app.'
    }
    $executable = Get-LocalPath (Join-Path $directory $application.GetAttribute('Executable'))
    $images = @(@([IO.Path]::GetFileNameWithoutExtension($executable)) + @($manifest.SelectNodes('//@Executable') |
        Where-Object { $_.Value.EndsWith('.exe', [StringComparison]::OrdinalIgnoreCase) } |
        ForEach-Object { [IO.Path]::GetFileNameWithoutExtension($_.Value) }) | Sort-Object -Unique)
    return @{
        family = $family; fullName = [string]$packages[0].PackageFullName; directory = $directory
        dependencyDirectories = $dependencyDirectories; executable = $executable; images = $images
    }
}

function Get-OriginalMuxDirectory(
    [string] $Executable, [string] $PackageDirectory,
    [string[]] $DependencyDirectories, [string] $OverrideDirectory
) {
    if (-not [string]::IsNullOrWhiteSpace($OverrideDirectory)) {
        $directory = Get-LocalPath $OverrideDirectory
        if (-not (Test-Path -LiteralPath $directory -PathType Container)) {
            throw "Original WinUI runtime directory not found: $directory"
        }
        return $directory
    }
    $appDirectories = @((Split-Path -Parent $Executable), $PackageDirectory) |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Sort-Object -Unique
    foreach ($group in @(@{ directories = @($appDirectories) }, @{ directories = @($DependencyDirectories) })) {
        $candidates = @($group.directories |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Where-Object {
            Test-Path -LiteralPath (Join-Path $_ 'Microsoft.UI.Xaml.dll') -PathType Leaf
        } | ForEach-Object { Get-LocalPath $_ } | Sort-Object -Unique)
        if ($candidates.Count -eq 1) { return $candidates[0] }
        if ($candidates.Count -gt 1) {
            throw "Multiple original WinUI runtimes were found: $($candidates -join ', '). Use -MuxResourceDirectory to select the runtime the app normally uses."
        }
    }
    throw 'Cannot locate the app''s original WinUI runtime. Use -MuxResourceDirectory with the folder of the Microsoft.UI.Xaml.dll that the app normally loads.'
}

function Get-MuxCompanions([string] $Directory, [bool] $RequireResources) {
    $Directory = Get-LocalPath $Directory
    if ($RequireResources) {
        foreach ($name in @('Microsoft.UI.Xaml.Resources.Common.dll', 'Microsoft.UI.Xaml.Resources.19H1.dll')) {
            if (-not (Test-Path -LiteralPath (Join-Path $Directory $name) -PathType Leaf)) {
                throw "The original WinUI runtime is missing '$name' in '$Directory'. Use -MuxResourceDirectory to identify the existing runtime containing the app's original companions."
            }
        }
    }
    # These companions also use LoadLibraryExWAbs, but do not have the XAML prefix.
    $moduleRelativeCompanions = @('WinUIEdit.dll', 'Microsoft.UI.Input.dll', 'Microsoft.DirectManipulation.dll')
    foreach ($file in (Get-ChildItem -LiteralPath $Directory -File | Where-Object {
        $_.Name -like 'Microsoft.UI.Xaml.*' -or $_.Name -in $moduleRelativeCompanions
    } |
        Where-Object Name -NE 'Microsoft.UI.Xaml.dll')) {
        $source = Get-LocalPath $file.FullName
        @{
            name = $file.Name
            source = $source
            isDirectory = $false
            fileIdentity = Get-FileIdentity $source
            sha256 = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
            fileVersion = if ($file.Extension -eq '.dll') { $file.VersionInfo.FileVersion } else { $null }
        }
    }
    foreach ($folder in (Get-ChildItem -LiteralPath $Directory -Directory)) {
        $source = Get-LocalPath $folder.FullName
        if ($folder.Name -eq 'Microsoft.UI.Xaml' -or
            @(Get-ChildItem -LiteralPath $source -File -Filter 'Microsoft.UI.Xaml*.mui').Count) {
            @{ name = $folder.Name; source = $source; isDirectory = $true }
        }
    }
}

function Get-MuxMuiInfo([string] $Path) {
    if (-not ('ProfileAppMui' -as [type])) {
        Add-Type -Path (Join-Path $PSScriptRoot 'ProfileAppMui.cs')
    }
    return [ProfileAppMui]::Read((Get-LocalPath $Path))
}

function Test-MuxMuiReassociation($Source, $Original) {
    if ($Source.FileType -ne 2) { return $false }
    if ($Original.FileType -ne 2 -or
        [string]::IsNullOrWhiteSpace($Source.ServiceChecksum) -or
        $Source.ServiceChecksum -eq ('0' * 32) -or
        $Source.ServiceChecksum -cne $Original.ServiceChecksum -or
        $Source.MainTypes -cne $Original.MainTypes -or $Source.MuiTypes -cne $Original.MuiTypes -or
        [string]::IsNullOrWhiteSpace($Source.Language) -or $Source.Language -ine $Original.Language) {
        throw 'The private Microsoft.UI.Xaml.dll and the original runtime have different localized resource definitions, so they cannot share the original MUI files. Build the private DLL against the app''s runtime resource definitions.'
    }
    return $Source.Checksum -cne $Original.Checksum
}

function Get-MuiResourceTool {
    $command = Get-Command muirct.exe -CommandType Application -ErrorAction SilentlyContinue
    if ($command) { return Get-LocalPath $command.Source }
    $architecture = [Runtime.InteropServices.RuntimeInformation]::ProcessArchitecture.ToString().ToLowerInvariant()
    $roots = @((Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Windows Kits\10'))
    foreach ($key in @('HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots',
        'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows Kits\Installed Roots')) {
        if (Test-Path -LiteralPath $key) {
            $property = (Get-ItemProperty -LiteralPath $key).PSObject.Properties['KitsRoot10']
            if ($property -and $property.Value) { $roots += [string]$property.Value }
        }
    }
    foreach ($root in ($roots | Select-Object -Unique)) {
        $bin = Join-Path $root 'bin'
        if (-not (Test-Path -LiteralPath $bin -PathType Container)) { continue }
        foreach ($version in (Get-ChildItem -LiteralPath $bin -Directory |
            Where-Object Name -Match '^\d+\.\d+\.\d+\.\d+$' | Sort-Object { [version]$_.Name } -Descending)) {
            $tool = Join-Path $version.FullName "$architecture\muirct.exe"
            if (Test-Path -LiteralPath $tool -PathType Leaf) { return Get-LocalPath $tool }
        }
    }
    throw 'The private Microsoft.UI.Xaml.dll needs its MUI association aligned with the app''s runtime, but the Windows SDK MUIRCT tool was not found. Install the Windows SDK or put muirct.exe on PATH.'
}

# Returns the DLL to stage: the private Microsoft.UI.Xaml.dll, or a copy whose MUI checksum matches the original
# runtime's language files, so the app's own .mui files serve its resources.
function Get-PreparedMuxDll(
    [string] $SourcePath, [string] $SourceSha256, [string] $OriginalDirectory,
    [object[]] $Companions, [string] $Directory
) {
    $sourceInfo = Get-MuxMuiInfo $SourcePath
    $result = @{
        sourcePath = $SourcePath; sourceSha256 = $SourceSha256
        path = $SourcePath; sha256 = $SourceSha256; reassociated = $false; sourceMui = $sourceInfo
    }
    if ($sourceInfo.FileType -ne 2) { return $result }
    $originalCore = Get-LocalPath (Join-Path $OriginalDirectory 'Microsoft.UI.Xaml.dll')
    $originalHash = (Get-FileHash -LiteralPath $originalCore -Algorithm SHA256).Hash
    $originalInfo = Get-MuxMuiInfo $originalCore
    $result.originalPath = $originalCore
    $result.originalSha256 = $originalHash
    $result.originalMui = $originalInfo
    $reassociate = Test-MuxMuiReassociation $sourceInfo $originalInfo
    if (-not $reassociate) { return $result }
    if ((Get-AuthenticodeSignature -LiteralPath $SourcePath).Status -ne 'NotSigned') {
        throw 'Aligning the MUI association would change a signed private Microsoft.UI.Xaml.dll. Use an unsigned build, or align its MUI association before signing it.'
    }
    $languageFiles = @()
    foreach ($companion in ($Companions | Where-Object isDirectory)) {
        $path = Join-Path $companion.source 'Microsoft.UI.Xaml.dll.mui'
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { continue }
        $path = Get-LocalPath $path
        $info = Get-MuxMuiInfo $path
        if ($info.FileType -ne 4 -or $info.Checksum -cne $originalInfo.Checksum -or
            $info.ServiceChecksum -cne $sourceInfo.ServiceChecksum -or $info.Language -ine $companion.name) {
            throw "Original language resource does not match its runtime association/content: $path"
        }
        $languageFiles += @{
            path = $path; language = $info.Language; fileIdentity = Get-FileIdentity $path
            sha256 = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
        }
    }
    if (@($languageFiles | Where-Object language -IEQ $originalInfo.Language).Count -ne 1) {
        throw "The original runtime's '$($originalInfo.Language)' fallback MUI file is missing or ambiguous. Use -MuxResourceDirectory to identify the app's complete existing runtime."
    }
    $result.originalLanguageFiles = $languageFiles
    $tool = Get-MuiResourceTool
    $result.tool = $tool
    $result.toolVersion = [Diagnostics.FileVersionInfo]::GetVersionInfo($tool).FileVersion
    $preparedDirectory = Get-LocalPath (Join-Path $Directory 'prepared-mux')
    $null = New-Item -ItemType Directory -Path $preparedDirectory
    $prepared = Join-Path $preparedDirectory 'Microsoft.UI.Xaml.dll'
    Copy-Item -LiteralPath $SourcePath -Destination $prepared
    if ((Get-FileHash -LiteralPath $prepared -Algorithm SHA256).Hash -cne $SourceSha256 -or
        (Get-FileIdentity $prepared) -ceq (Get-FileIdentity $SourcePath) -or
        (Get-FileIdentity $prepared) -ceq (Get-FileIdentity $originalCore)) {
        throw 'The MUI preparation copy is not an independent copy of the validated private DLL.'
    }
    $result.arguments = @('-c', $originalCore, '-e', $prepared)
    $result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $Directory 'mui-preparation.json')
    Write-Host "Aligning the private DLL's MUI association in a copy; reusing $($languageFiles.Count) original language files."
    Invoke-SystemTool $tool $result.arguments (Join-Path $Directory 'mui-preparation')
    $preparedInfo = Get-MuxMuiInfo $prepared
    if ((Test-MuxMuiReassociation $preparedInfo $originalInfo) -or
        $preparedInfo.Checksum -cne $originalInfo.Checksum) {
        throw 'MUIRCT did not produce the expected original-runtime resource association.'
    }
    [ProfileAppMui]::VerifyChecksumOnlyChange($SourcePath, $prepared, $sourceInfo.Checksum, $originalInfo.Checksum)
    if ((Get-FileHash -LiteralPath $SourcePath -Algorithm SHA256).Hash -cne $SourceSha256 -or
        (Get-FileHash -LiteralPath $originalCore -Algorithm SHA256).Hash -cne $originalHash) {
        throw 'A source/reference DLL changed during MUI preparation; do not stage this copy.'
    }
    foreach ($language in $languageFiles) {
        if ((Get-FileIdentity $language.path) -cne $language.fileIdentity -or
            (Get-FileHash -LiteralPath $language.path -Algorithm SHA256).Hash -cne $language.sha256) {
            throw "An original language resource changed during MUI preparation: $($language.path)"
        }
    }
    $result.path = $prepared
    $result.sha256 = (Get-FileHash -LiteralPath $prepared -Algorithm SHA256).Hash
    $result.reassociated = $true
    $result.preparedMui = $preparedInfo
    $result.onlyMuiAssociationChanged = $true
    $result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $Directory 'mui-preparation.json')
    return $result
}

function New-MuxCompanionLinks([string] $Directory, [object[]] $Companions, [hashtable] $CreatedLinks) {
    $Directory = Get-LocalPath $Directory
    foreach ($companion in $Companions) {
        if ([string]::IsNullOrWhiteSpace($companion.name) -or
            [IO.Path]::GetFileName($companion.name) -cne $companion.name -or
            $companion.name -in @('.', '..', 'Microsoft.UI.Xaml.dll')) {
            throw "Invalid original companion name: $($companion.name)"
        }
        $source = Get-LocalPath $companion.source
        if ([string]::Equals($source, $Directory, [StringComparison]::OrdinalIgnoreCase) -or
            $source.StartsWith("$Directory\", [StringComparison]::OrdinalIgnoreCase)) {
            throw 'Original companions cannot come from inside the staging directory.'
        }
        $destination = Join-Path $Directory $companion.name
        if ($companion.isDirectory) {
            $null = [IO.Directory]::CreateSymbolicLink($destination, $source)
        }
        else {
            $null = [IO.File]::CreateSymbolicLink($destination, $source)
        }
        $CreatedLinks[$companion.name] = @{ target = $source; isDirectory = $companion.isDirectory }
        if (-not $companion.isDirectory -and (
            (Get-FileIdentity $destination) -cne $companion.fileIdentity -or
            (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash -ne $companion.sha256)) {
            throw "The companion link does not identify the original runtime file: $destination"
        }
    }
}

function Invoke-SystemTool([string] $Executable, [string[]] $Arguments, [string] $LogPrefix) {
    $start = [Diagnostics.ProcessStartInfo]::new($Executable)
    $start.UseShellExecute = $false
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    foreach ($argument in $Arguments) { $start.ArgumentList.Add($argument) }
    $process = [Diagnostics.Process]::Start($start)
    try {
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        $process.WaitForExit()
        $stdout.GetAwaiter().GetResult() | Set-Content -LiteralPath "$LogPrefix.stdout.log" -Encoding utf8
        $stderr.GetAwaiter().GetResult() | Set-Content -LiteralPath "$LogPrefix.stderr.log" -Encoding utf8
        if ($process.ExitCode -ne 0) {
            throw "$Executable exited $($process.ExitCode): $($Arguments -join ' '). See $LogPrefix.stdout.log and $LogPrefix.stderr.log."
        }
    }
    finally { $process.Dispose() }
}

function New-RedirectionDirectory([string] $Path, [ref] $Created) {
    $Path = Get-LocalPath $Path
    $null = New-Item -ItemType Directory -Path $Path
    $Created.Value = $true
    $acl = Get-Acl -LiteralPath $Path
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    try {
        $acl.AddAccessRule([Security.AccessControl.FileSystemAccessRule]::new(
            $identity.User, 'ReadAndExecute', 'ContainerInherit,ObjectInherit', 'None', 'Allow'))
        Set-Acl -LiteralPath $Path -AclObject $acl
    }
    finally { $identity.Dispose() }
}

function Remove-PrivateRuntime(
    [string] $Directory, [string] $MuxSha256, [string] $MuxcSha256, [hashtable] $CompanionLinks, [bool] $CanRemove
) {
    $Directory = Get-LocalPath $Directory
    $files = @(Get-ChildItem -LiteralPath $Directory -Force)
    # Check every entry before deleting any; a changed or in-use folder is kept whole.
    foreach ($file in $files) {
        $expectedHash = if ($file.Name -eq $Mux) { $MuxSha256 } elseif ($file.Name -eq $Muxc) { $MuxcSha256 }
        if ($expectedHash) {
            $CanRemove = $CanRemove -and -not $file.PSIsContainer -and
                -not ($file.Attributes -band [IO.FileAttributes]::ReparsePoint) -and
                (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash -eq $expectedHash
        }
        elseif ($CompanionLinks.ContainsKey($file.Name)) {
            $expected = $CompanionLinks[$file.Name]
            $CanRemove = $CanRemove -and $file.LinkType -eq 'SymbolicLink' -and
                $file.PSIsContainer -eq $expected.isDirectory -and
                [string]::Equals($file.LinkTarget, $expected.target, [StringComparison]::OrdinalIgnoreCase)
        }
        else { $CanRemove = $false }
    }
    if (-not $CanRemove) {
        Write-Warning "The private WinUI runtime was kept at $Directory. Close the app, then delete the folder's links without following them; they point to the app's own runtime."
        return $false
    }
    foreach ($file in $files) {
        # Delete the link itself, never recurse into an original runtime directory.
        if ($file.PSIsContainer) { [IO.Directory]::Delete($file.FullName) }
        else { [IO.File]::Delete($file.FullName) }
    }
    [IO.Directory]::Delete($Directory)
    return $true
}

# Deletes a folder of regular files, such as a launch's copy of the launcher.
function Remove-FileDirectory([string] $Directory) {
    if (-not (Test-Path -LiteralPath $Directory -PathType Container)) { return $true }
    try {
        $entries = @(Get-ChildItem -LiteralPath $Directory -Force)
        if (@($entries | Where-Object { $_.PSIsContainer -or ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) }).Count) {
            throw "It holds more than files."
        }
        foreach ($entry in $entries) { [IO.File]::Delete($entry.FullName) }
        [IO.Directory]::Delete($Directory)
        return $true
    }
    catch {
        Write-Warning "Could not delete ${Directory}: $($_.Exception.Message)"
        return $false
    }
}

function Get-LaunchRecords([string] $Directory) {
    if (-not (Test-Path -LiteralPath $Directory -PathType Container)) { return }
    foreach ($folder in (Get-ChildItem -LiteralPath $Directory -Directory | Where-Object Name -Match '^\d+$' |
        Sort-Object { [int]$_.Name })) {
        $path = Join-Path $folder.FullName 'launch.json'
        $result = $null
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            try { $result = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json -AsHashtable }
            catch { Write-Warning "Unreadable launch result ${path}: $($_.Exception.Message)" }
        }
        [pscustomobject]@{ number = [int]$folder.Name; directory = $folder.FullName; result = $result }
    }
}

# A packaged app's writes under LocalAppData can be redirected to its package's LocalCache.
function Find-RedirectLog([string] $LaunchDirectory, [string] $PackageFamily) {
    $path = Join-Path $LaunchDirectory 'redirect.jsonl'
    if (Test-Path -LiteralPath $path -PathType Leaf) { return $path }
    if ($PackageFamily) {
        $local = [Environment]::GetFolderPath('LocalApplicationData')
        $virtual = Join-Path $local "Packages\$PackageFamily\LocalCache\Local\$([IO.Path]::GetRelativePath($local, $path))"
        if (Test-Path -LiteralPath $virtual -PathType Leaf) { return $virtual }
    }
    return $null
}

function Get-RedirectedFiles([string] $Log, [string] $Runtime) {
    $files = [Collections.Generic.List[string]]::new()
    foreach ($line in (Get-Content -LiteralPath $Log)) {
        try { $entry = $line | ConvertFrom-Json } catch { continue }
        if ($entry.event -eq 'redirected' -and $entry.selected -and
            [string]::Equals((Split-Path -Parent $entry.selected), $Runtime, [StringComparison]::OrdinalIgnoreCase)) {
            $name = Split-Path -Leaf $entry.selected
            if (-not $files.Contains($name)) { $files.Add($name) }
        }
    }
    return , $files
}

function Close-LaunchedApp([int] $ProcessId, [string] $Executable, [datetime] $Since) {
    try { $process = [Diagnostics.Process]::GetProcessById($ProcessId) }
    catch [ArgumentException] { return $true }
    try {
        if ($process.HasExited) { return $true }
        # A reused process ID belongs to another process; the launched app has exited.
        if ($process.StartTime -lt $Since -or -not (Test-SameFile $process.MainModule.FileName $Executable)) { return $true }
        Write-Host "Closing the app (PID $ProcessId)."
        if ($process.CloseMainWindow() -and $process.WaitForExit(15000)) { return $true }
        $process.Kill()
        return $process.WaitForExit(5000)
    }
    catch [InvalidOperationException], [ComponentModel.Win32Exception] {
        Write-Warning "Could not close the app (PID $ProcessId): $($_.Exception.Message)"
        return $false
    }
    finally { $process.Dispose() }
}

function ConvertTo-ProfileArgument([string] $Value) {
    # Quote for CommandLineToArgvW: double the backslashes before an escaped or closing quote.
    if ($Value -and $Value -notmatch '[\s"]') { return $Value }
    return '"' + (($Value -replace '(\\*)"', '$1$1\"') -replace '(\\+)$', '$1$1') + '"'
}

function Invoke-ProfileAppElevated([string] $ScriptPath, [Collections.IDictionary] $Parameters) {
    # Relaunch through UAC and stream the elevated run's transcript; it returns that run's exit code.
    $log = Join-Path ([IO.Path]::GetTempPath()) "WinUIProfiler\profile-app-$([guid]::NewGuid().ToString('N')).log"
    $null = New-Item -ItemType Directory -Force -Path (Split-Path -Parent $log)
    $arguments = @('-NoProfile', '-ExecutionPolicy', [string](Get-ExecutionPolicy),
        '-WorkingDirectory', $PWD.ProviderPath, '-File', $ScriptPath)
    foreach ($entry in $Parameters.GetEnumerator()) {
        if ($entry.Value -is [switch]) {
            if ($entry.Value.IsPresent) { $arguments += "-$($entry.Key)" }
        }
        else {
            $arguments += "-$($entry.Key)"
            $arguments += [string]$entry.Value
        }
    }
    $arguments += '-ElevatedLogPath', $log
    Write-Host 'Live ETW and the private runtime links need administrator rights. Approve the Windows UAC prompt to continue.'
    try {
        $child = Start-Process -FilePath ([Environment]::ProcessPath) -Verb RunAs -WindowStyle Hidden -PassThru `
            -ArgumentList (($arguments | ForEach-Object { ConvertTo-ProfileArgument $_ }) -join ' ')
    }
    catch {
        Write-Warning "Administrator rights were not granted, so nothing was profiled: $($_.Exception.Message)"
        return 4
    }
    $reader = $null
    try {
        do {
            $exited = $child.WaitForExit(1000)
            if ($null -eq $reader -and [IO.File]::Exists($log)) {
                $reader = [IO.StreamReader]::new([IO.FileStream]::new($log, 'Open', 'Read', 'ReadWrite, Delete'))
            }
            # Skip the transcript's PS> echo lines; they include errors the script already handled.
            while ($null -ne $reader -and $null -ne ($line = $reader.ReadLine())) {
                if (-not $line.StartsWith('PS>')) { Write-Host $line }
            }
        } until ($exited)
    }
    finally {
        if ($null -ne $reader) { $reader.Dispose() }
    }
    if ($null -eq $reader) { Write-Warning "The administrator run wrote no log to $log." }
    return $child.ExitCode
}

function Invoke-ProfileApp([string] $Profiler, [string] $HostPlatform) {
    $package = if ($Aumid) { Get-PackagedApp $Aumid }
    $executable = if ($package) { $package.executable } else { Get-LocalPath $ExePath }
    if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) { throw "Executable not found: $executable" }
    $machine = Get-PeMachine $executable
    $architecture = switch ($machine) {
        0x8664 { 'x64' }
        0xAA64 { 'ARM64' }
        default { throw ('The app has PE machine 0x{0:X4}. Private WinUI DLLs can be loaded by x64 and ARM64 apps.' -f $machine) }
    }
    if ($architecture -eq 'ARM64' -and $HostPlatform -ne 'ARM64') { throw 'An ARM64 app needs ARM64 Windows.' }
    $privateMux = Get-PrivateDll $MuxPath @($Mux, "$Mux.MUI") $machine
    $privateMuxc = Get-PrivateDll $MuxcPath @($Muxc) $machine
    $images = if ($package) { $package.images } else { @([IO.Path]::GetFileNameWithoutExtension($executable)) }
    if (@(Get-Process -Name $images -ErrorAction SilentlyContinue).Count) {
        throw "Close all instances of these app executables first: $($images -join ', ')."
    }
    $dotLocal = if ($package) {
        Join-Path $package.directory 'Microsoft.System.Package.Metadata\application.local'
    }
    else { "$executable.local" }
    if ((Test-Path -LiteralPath $dotLocal) -or (Test-Path -LiteralPath "$executable.local")) {
        throw "Existing .local redirection conflicts with the private WinUI launch and has been kept: $dotLocal"
    }
    $original = Get-OriginalMuxDirectory $executable $package.directory $package.dependencyDirectories $MuxResourceDirectory
    $companions = @(Get-MuxCompanions $original $true)
    # The private Microsoft.UI.Xaml.Controls.dll replaces the original, so both loaded DLLs come from one build.
    if (@($companions | Where-Object { -not $_.isDirectory -and $_.name -eq $Muxc }).Count -ne 1) {
        throw "The original WinUI runtime '$original' has no $Muxc to replace. Use -MuxResourceDirectory with the complete runtime the app normally uses."
    }
    $companions = @($companions | Where-Object { $_.isDirectory -or $_.name -ne $Muxc })

    $local = [Environment]::GetFolderPath('LocalApplicationData')
    $run = Get-LocalPath (Join-Path $local "WinUIProfiler\PrivateRuntime\$(Get-Date -Format yyyyMMdd-HHmmss)-$([guid]::NewGuid().ToString('N').Substring(0, 8))")
    $runtime = Join-Path $run 'runtime'
    $launches = Join-Path $run 'launch'
    $null = New-Item -ItemType Directory -Path $run
    Write-Host "App: $executable ($architecture)$(if ($package) { ", package $($package.fullName)" })"
    Write-Host "Original WinUI runtime: $original"
    Write-Host "Private $($Mux): $($privateMux.path) ($($privateMux.fileVersion))"
    Write-Host "Private $($Muxc): $($privateMuxc.path) ($($privateMuxc.fileVersion))"
    $created = $false
    $links = @{}
    $prepared = $null
    $started = $null
    $failure = $null
    try {
        $prepared = Get-PreparedMuxDll $privateMux.path $privateMux.sha256 $original $companions $run
        New-RedirectionDirectory $runtime ([ref]$created)
        Copy-Item -LiteralPath $prepared.path -Destination (Join-Path $runtime $Mux)
        Copy-Item -LiteralPath $privateMuxc.path -Destination (Join-Path $runtime $Muxc)
        if ((Get-FileHash -LiteralPath (Join-Path $runtime $Mux) -Algorithm SHA256).Hash -ne $prepared.sha256 -or
            (Get-FileHash -LiteralPath (Join-Path $runtime $Muxc) -Algorithm SHA256).Hash -ne $privateMuxc.sha256) {
            throw 'A staged private DLL does not match its validated source.'
        }
        New-MuxCompanionLinks $runtime $companions $links
        $null = New-Item -ItemType Directory -Path $launches
        Write-Host "Private WinUI runtime: $runtime"

        $start = [Diagnostics.ProcessStartInfo]::new($Profiler)
        $start.UseShellExecute = $false
        $start.WorkingDirectory = Split-Path -Parent $Profiler
        $profilerArguments = if ($package) { @('--launch-aumid', $Aumid) } else { @('--launch-exe', $executable) }
        if ($Arguments) { $profilerArguments += '--arguments', $Arguments }
        $profilerArguments += '--private-runtime', $runtime, '--runtime-source', $original, '--redirect-controls',
            '--launch-directory', $launches, '--trust-local-binaries', '--connect'
        foreach ($argument in $profilerArguments) { $start.ArgumentList.Add($argument) }
        $started = Get-Date
        $profilerProcess = [Diagnostics.Process]::Start($start)
        try {
            Write-Host "WinUI Profiler (PID $($profilerProcess.Id)) launches the app and profiles it. Close the profiler to finish."
            $profilerProcess.WaitForExit()
            Write-Host "WinUI Profiler exited with code $($profilerProcess.ExitCode)."
        }
        finally { $profilerProcess.Dispose() }
    }
    catch {
        $failure = $_
        throw
    }
    finally {
        $records = @(Get-LaunchRecords $launches)
        $closed = $true
        foreach ($record in $records) {
            if ($null -ne $started -and $record.result -and $record.result.pid -gt 0) {
                $closed = (Close-LaunchedApp ([int]$record.result.pid) $executable $started.AddSeconds(-1)) -and $closed
            }
        }
        $removed = $true
        if ($created) {
            $canRemove = $closed -and -not @(Get-Process -Name $images -ErrorAction SilentlyContinue).Count
            $removed = Remove-PrivateRuntime $runtime $prepared.sha256 $privateMuxc.sha256 $links $canRemove
        }
        if ($prepared -and $prepared.reassociated) {
            $removed = (Remove-FileDirectory (Split-Path -Parent $prepared.path)) -and $removed
        }
        foreach ($record in $records) {
            $removed = (Remove-FileDirectory (Join-Path $record.directory 'native')) -and $removed
        }
        if ($null -eq $failure) {
            $script:ExitCode = Write-ProfileSummary $records $runtime $package $started $removed
        }
    }
}

function Write-ProfileSummary($Records, [string] $Runtime, $Package, [datetime] $Started, [bool] $Removed) {
    $succeeded = 0
    foreach ($record in $Records) {
        $result = $record.result
        $log = Find-RedirectLog $record.directory $Package.family
        $logText = if ($log) { " Log: $log" } else { '' }
        if (-not $result) {
            Write-Warning "Launch $($record.number) wrote no result. See $($record.directory)."
        }
        elseif ($result.error -ne 0) {
            Write-Warning "Launch $($record.number) failed at $($result.stage) with error $($result.error).$logText"
        }
        else {
            $succeeded++
            $redirected = if ($log) { @(Get-RedirectedFiles $log $Runtime) } else { @() }
            $loaded = if ($redirected.Count) { "; loaded the private $($redirected -join ' and ')" } else { '' }
            Write-Host "Launch $($record.number): PID $($result.pid)$loaded.$logText"
        }
    }
    $sessions = Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) 'WinUIProfiler\Sessions'
    if (Test-Path -LiteralPath $sessions -PathType Container) {
        foreach ($session in (Get-ChildItem -LiteralPath $sessions -File -Filter *.sqlite | Where-Object LastWriteTime -GE $Started)) {
            Write-Host "Session: $($session.FullName)"
        }
    }
    if (-not $Records.Count) { Write-Warning 'WinUI Profiler closed without launching the app; it shows the reason.' }
    if ($Removed) { Write-Host 'Removed the private WinUI runtime.' }
    return $(if ($succeeded -and $Removed) { 0 } else { 1 })
}

if ($ElevatedLogPath) { $null = Start-Transcript -LiteralPath $ElevatedLogPath -UseMinimalHeader }
try {
    if (-not $IsWindows) { throw 'Profile-App.ps1 requires Windows.' }
    if (-not [Environment]::Is64BitProcess) { throw 'Use 64-bit PowerShell 7 (x64 or ARM64).' }
    $hostPlatform = switch ([Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString()) {
        'X64' { 'x64' }
        'Arm64' { 'ARM64' }
        default { throw 'WinUI Profiler runs on x64 and ARM64 Windows.' }
    }
    $profiler = if ($ProfilerPath) { Get-LocalPath $ProfilerPath }
    else { Get-ProfilerExecutable (Split-Path -Parent $PSScriptRoot) $hostPlatform $Build }
    if (-not (Test-Path -LiteralPath $profiler -PathType Leaf)) { throw "WinUI Profiler not found: $profiler" }
    if (-not (Test-Path -LiteralPath (Join-Path (Split-Path -Parent $profiler) 'NativeLauncher\manifest.json') -PathType Leaf)) {
        throw "This WinUI Profiler build has no private WinUI launcher: $profiler. Build it with Build-App.ps1."
    }
    if (Test-Administrator) {
        Invoke-ProfileApp $profiler $hostPlatform
    }
    elseif ($ElevatedLogPath) { throw 'The relaunched script does not have administrator rights.' }
    else {
        $parameters = [ordered]@{}
        foreach ($entry in $PSBoundParameters.GetEnumerator()) {
            if ($entry.Key -ne 'Build') { $parameters[$entry.Key] = $entry.Value }
        }
        $parameters['ProfilerPath'] = $profiler
        $script:ExitCode = Invoke-ProfileAppElevated $PSCommandPath $parameters
    }
}
catch {
    Write-Host "Profile-App failed: $($_.Exception.Message)" -ForegroundColor Red
    $script:ExitCode = 1
}
finally {
    if ($ElevatedLogPath) { $null = Stop-Transcript }
}
exit $script:ExitCode
