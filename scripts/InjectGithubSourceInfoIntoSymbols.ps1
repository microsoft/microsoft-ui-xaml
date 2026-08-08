# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

<#
.SYNOPSIS
Script that takes a directory of PDBs that have already been source indexed and
injects the necessary information to allow SrcSrv to extract source files from
Github; the original source server information is preserved for use as a fallback.
Warning: PDBs are updated in-place!

.DESCRIPTION

.PARAMETER SymbolsDirectory
Specifies the path to the directory containing the symbol files that will be modified. This path 
can be relative.

.PARAMETER GithubRepositoryName
Specifies the name (including that of the organization, e.g. 'microsoft/microsoft-ui-xaml') of the
Github repository containing the source files.

.PARAMETER GithubRepositoryDirectory
Specifies the path to the directory containing the Github repository. This path can be relative.

.PARAMETER CommitHash
Specifies the commit hash in the Github repository to map the source files to.

.PARAMETER TargetRepositorySubdirectory
(Optional) Specifies a subdirectory of the 'target' repository that the source code should be mirrored into
instead of the repository root.

.PARAMETER WindowsBuildNumber
(Optional) Build number of Windows SDK to source the Windows Debugging Tools from.
As long as the tools are available, the specific build number is unimportant as the tools
are very stable. Defaults to '22621'.

#>

param(
    [parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $SymbolsDirectory,

    [parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $GithubRepositoryName,

    [parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $GithubRepositoryDirectory,

    [parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $CommitHash,

    [parameter(Mandatory=$false)]
    [ValidateNotNullOrEmpty()]
    [string]$TargetRepositorySubdirectory,

    [string] $WindowsBuildNumber = [string] "22621"
)

Import-Module -Name $PSScriptRoot\WindowsSdkInstallerFunctions.psm1 -DisableNameChecking -Force

function Run-LoggedCommand {
    param(
        [parameter(Mandatory=$true)]
        [string] $command,

        [bool] $throwOnErrorCode = $true
    )

    Write-Host "##[command]$command $arguments"
    Invoke-Expression $command

    if ($throwOnErrorCode) {
        if ($command.StartsWith("robocopy ")) {
            if ($LASTEXITCODE -gt 7) {
                # Robocopy doesn't follow exit code conventions; any code from 0-7 indicates success
                # while those greater than 7 indicate failure.
                Write-Error "##[error]Command '$command' terminated with error code '$LASTEXITCODE'"
                throw
            }
        }
        elseif ($LASTEXITCODE) {
            Write-Error "##[error]Command '$command' terminated with error code '$LASTEXITCODE'"
            throw
        }
    }
}

function Get-Is64Bit
{
    return $env:PROCESSOR_ARCHITECTURE -eq "AMD64"
}

function Get-DebuggingToolsRoot
{
    # Constants
    $windowsSDKRegPath = if (Get-Is64Bit) { "HKLM:\Software\WOW6432Node\Microsoft\Windows Kits\Installed Roots" } else { "HKLM:\Software\Microsoft\Windows Kits\Installed Roots" }
    $windowsDebuggingToolsRegRootKey = "WindowsDebuggersRoot10"

    try
    {
        return Get-ItemProperty -Path $windowsSDKRegPath | Select-Object -ExpandProperty $windowsDebuggingToolsRegRootKey
    }
    catch
    {
        return $null
    }
}

function Install-DebuggingTools(
    [parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()] 
    [string] $buildNumber)
{
    $installerPath = Ensure-WindowsSDKInstaller $buildNumber

    Write-Host "##[debug]Installing Debugging Tools for Windows..."

    $windowsSDKOptions = @("OptionId.WindowsDesktopDebuggers")
    $options = "/features $($windowsSDKOptions -join " ") /log $LogDirectory\DebuggingToolsInstall.log"
    Run-LoggedCommand "$installerPath $options"

    return $installerPath
}

function Uninstall-DebuggingTools(
    [parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()] 
    [string] $installerPath)
{

    Write-Host "##[debug]Uninstalling Debugging Tools for Windows..."

    $windowsSDKOptions = @("OptionId.WindowsDesktopDebuggers")
    $options = "/uninstall /features $($windowsSDKOptions -join " ") /log $LogDirectory\DebuggingToolsUninstall.log"
    Run-LoggedCommand "$installerPath $options"

    Cleanup-WindowsSDKInstaller $installerPath
}

function Rewrite-SrcSrvStream(
    [parameter(Mandatory=$true)]
    [ValidateScript({if (Test-Path $_) { $true } else { throw "Path $_ is invalid."}})]
    [string] $toolsRootx86,

    [parameter(Mandatory=$true)]
    [ValidateScript({if (Test-Path $_) { $true } else { throw "Path $_ is invalid."}})]
    [string] $toolsRootx64,

    [parameter(Mandatory=$true)]
    [ValidateScript({if (Test-Path $_) { $true } else { throw "Path $_ is invalid."}})]
    [string] $pdbPath,

    [parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $githubRepositoryName,

    [parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string]$githubRepositoryDirectory,

    [parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $commitHash)
{
    # SRCSRV stream specification: 
    # https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/language-specification-1

    enum SrcSrvStreamSection {
        Header
        Variables
        Sources
        EOF
        None
    }

    Write-Host "##[group]Processing '$pdbPath'"

    $pdbStrPathx86 = [IO.Path]::Combine($toolsRootx86, "pdbstr.exe")
    $pdbStrPathx64 = [IO.Path]::Combine($toolsRootx64, "pdbstr.exe")
    $srcToolPathx86 = [IO.Path]::Combine($toolsRootx86, "srctool.exe")
    $srcToolPathx64 = [IO.Path]::Combine($toolsRootx64, "srctool.exe")
    $srcSrvStreamOutputPath = "$pdbPath.stream"
    $rewrittenSrcSrvStreamOutputPath = "$pdbPath.stream.rewritten"
    
    # Extract SRCSRV stream from the PDB for rewriting. If no source files were indexed (exit code of `-1` returned
    # by `srctool -c`) or the stream does not exist then skip the PDB entirely.
    Run-LoggedCommand "& '$srcToolPathx64' -c `"$pdbPath`"" $false
    if ($LASTEXITCODE -lt -1) {
        Write-Host "##[debug]'srctool.exe' failed unexpectedly with exit code '$LASTEXITCODE'. Retrying with x86 version."

        Run-LoggedCommand "& '$srcToolPathx86' -c `"$pdbPath`"" $false
    }
    if ($LASTEXITCODE -eq -1) {
        Write-Host "##[debug]Skipping PDB (reason: no indexed files)"
        Write-Host "##[endgroup]"

        return
    }

    # We empirically know that the x64 version of pdbstr.exe fails in some CI container environments,
    # although we do not know the
    # reason for this. So even though x64 is the "native" version of the utility, we'll try the x86 version first
    # because we know x64 is highly likely to fail.
    Run-LoggedCommand "& '$pdbStrPathx86' -r -s:srcsrv -p:`"$pdbPath`" -i:`"$srcSrvStreamOutputPath`"" $false
    if ($LASTEXITCODE -lt 0) {
        Write-Host "##[debug]'pdbstr.exe' failed unexpectedly with exit code '$LASTEXITCODE'. Retrying with x64 version."

        Run-LoggedCommand "& '$pdbStrPathx64' -r -s:srcsrv -p:`"$pdbPath`" -i:`"$srcSrvStreamOutputPath`"" $false
    }

    if ($LASTEXITCODE -ne 0) {
        Write-Host "##[debug]Skipping PDB (reason: error extracting srcsrv stream; exitcode: $LASTEXITCODE)"
        Write-Host "##[endgroup]"

        return
    }
    if (!(Test-Path $srcSrvStreamOutputPath)) {
        Write-Host "##[debug]Skipping PDB (reason: no srcsrv stream present)"
        Write-Host "##[endgroup]"

        return
    }

    # Process and rewrite stream
    # The format is simple enough that we can just process it line-by-line,
    # keeping track of which section (ini, variables, or source files) we're
    # currently examining.
    #
    # Note: references to TF(S) (Team Foundation [Server]) in the SRCSRV stream are legacy branding that is 
    # equivalent to Azure DevOps.
    $rewrittenLines = New-Object Collections.Generic.List[String]
    $currentSection = [SrcSrvStreamSection]::None
    foreach ($line in Get-Content -Path $srcSrvStreamOutputPath) {
        if ($line.StartsWith("SRCSRV: ini")) {
            Write-Host "##[debug]Processing header section"

            $currentSection = [SrcSrvStreamSection]::Header
            $rewrittenLines.Add($line)

            continue
        }
        if ($line.StartsWith("SRCSRV: variables")) {
            Write-Host "##[debug]Processing variables section"

            $currentSection = [SrcSrvStreamSection]::Variables
            $rewrittenLines.Add($line)

            # Inject additional variables to support retrieving source file from Github
            $rewrittenLines.Add("GITHUB_COMMIT=$commitHash")
            $rewrittenLines.Add("GITHUB_REPOSITORY_NAME=$githubRepositoryName")
            $rewrittenLines.Add("GITHUB_BASE_URL=https://raw.githubusercontent.com/%GITHUB_REPOSITORY_NAME%/%GITHUB_COMMIT%")
            $rewrittenLines.Add("GITHUB_EXTRACT_CMD=powershell -command `"Invoke-WebRequest '%GITHUB_BASE_URL%%var7%' -OutFile '%SRCSRVTRG%'`"")
            $rewrittenLines.Add("SRCSRVERRDESC_2=Exception")

            continue
        }
        if ($line.StartsWith("SRCSRV: source files")) {
            Write-Host "##[debug]Processing source files section"

            $currentSection = [SrcSrvStreamSection]::Sources
            $rewrittenLines.Add($line)

            continue
        }
        if ($line.StartsWith("SRCSRV: end")) {
            Write-Host "##[debug]Reached end of stream"

            $currentSection = [SrcSrvStreamSection]::End
            $rewrittenLines.Add($line)

            break
        }

        switch ($currentSection) {
            ([SrcSrvStreamSection]::None) {
                Write-Error "##[error]Potentially malformed SRCSRV stream in '$pdbPath'. Manual investigation required."
                Write-Host "##[endgroup]"
                
                return
            }
            ([SrcSrvStreamSection]::Header) {
                # Original header does not need to be modified
                $rewrittenLines.Add($line)

                break
            }
            ([SrcSrvStreamSection]::Variables) {
                if ($line.StartsWith("SRCSRVCMD=")) {
                    # Instruct SrcSrv to use %var9% from the source entry to determine which command 
                    # (GITHUB_EXTRACT_CMD or TFS_EXTRACT_CMD) should be used to extract the source file. 
                    # This is necessary because not all source files are available on Github but if an
                    # _internal_ developer were to specify MSDL before https://symweb in their SYMPATH we
                    # want them to still be able to extract source files that we have withheld from our
                    # Github source code drops.
                    $rewrittenLines.Add("SRCSRVCMD=%fnvar%(%var9%)")
                }
                else {
                    $rewrittenLines.Add($line)
                }

                break
            }
            ([SrcSrvStreamSection]::Sources) {
                $rewrittenLine = Rewrite-SourceLine "$line" "$githubRepositoryDirectory"
                $rewrittenLines.Add($rewrittenLine)

                break
            }
        }
    }

    # Write rewritten lines to file
    Set-Content -Force -Path $rewrittenSrcSrvStreamOutputPath -Value $rewrittenLines

    # Inject rewritten stream back into the PDB
    Run-LoggedCommand "& '$pdbStrPathx86' -w -s:srcsrv -p:`"$pdbPath`" -i:`"$rewrittenSrcSrvStreamOutputPath`"" $false
    if ($LASTEXITCODE -lt 0) {
        Write-Host "##[debug]'pdbstr.exe' failed unexpectedly with exit code '$LASTEXITCODE'. Retrying with x64 version."

        Run-LoggedCommand "& '$pdbStrPathx64' -w -s:srcsrv -p:`"$pdbPath`" -i:`"$rewrittenSrcSrvStreamOutputPath`"" $false
    }
    if ($LASTEXITCODE -ne 0) {
        Write-Host "##[error]Error writing stream to '$pdbPath' (exitcode: $LASTEXITCODE)"
        Write-Host "##[endgroup]"

        throw
    }


    Write-Host "##[endgroup]"
}

function Rewrite-SourceLine(
    [parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $originalLine,

    [parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $githubRepositoryDirectory)
{
    # The contents of each line are interpreted as a series of variables, separated by asterisks,
    # assigned the names var1, var2, var3, and so on until var10. These are used in conjunction with
    # the variables defined in the VARIABLES section of the SRCSRV stream to inform SrcSrv how to
    # extract a given source file.
    # 
    # For example, the following line:
    # C:\__w\1\s\dxaml\xcp\dxaml\dllsrv\winrt\Microsoft.UI.Xaml.def*TFS_COLLECTION*TFS_TEAM_PROJECT*TFS_REPO*TFS_COMMIT*TFS_SHORT_COMMIT*/dxaml/xcp/dxaml/dllsrv/winrt/Microsoft.UI.Xaml.def*TFS_APPLY_FILTERS
    #
    # Is interpreted as the following variables:
    # var1 = C:\__w\1\s\dxaml\xcp\dxaml\dllsrv\winrt\Microsoft.UI.Xaml.def
    # var2 = TFS_COLLECTION
    # var3 = TFS_TEAM_PROJECT
    # var4 = TFS_REPO
    # var5 = TFS_COMMIT
    # var6 = TFS_SHORT_COMMIT
    # var7 = /dxaml/xcp/dxaml/dllsrv/winrt/Microsoft.UI.Xaml.def
    # var8 = TFS_APPLY_FILTERS
    #
    # This method performs the following actions:
    # 1) Determines if the corresponding source file exists in the Github repository clone specified by 
    #    $githubRepositoryDirectory.
    # 2) Append a new variable, var9, with either GITHUB_EXTRACT_CMD (the default) or TFS_EXTRACT_CMD (if step 1 
    #    determined that the file does not exist on Github), to indicate which extraction command should be used for
    #    the source entry.
    # 3) If the file exists in Github, then var2's value is changed to "GITHUB" otherwise it is left unchanged. var2
    #    is used, through the SRCSRVERRVAR variable, to match a source entry with previously failed extraction
    #    commands so that SrcSrv won't attempt to retrieve a file from a faulty source control server. This allows 
    #    external developers to be able to retrieve files from Github even if they had earlier attempted to (and failed 
    #    because of lack of access rights) retrieve a different file from Azure DevOps.

    $splitLine = $originalLine.Split('*')
    $modifiedOriginalFileRelativePath = $splitLine[6]
    if ($TargetRepositorySubdirectory) {
        if ($modifiedOriginalFileRelativePath[0] -eq '/') {
            $modifiedOriginalFileRelativePath = "/$TargetRepositorySubdirectory$modifiedOriginalFileRelativePath"
        } elseif ($splitLine[6][0] -eq '\') {
            $modifiedOriginalFileRelativePath = "\$TargetRepositorySubdirectory$modifiedOriginalFileRelativePath"
        } else {
            $modifiedOriginalFileRelativePath = "$TargetRepositorySubdirectory/$modifiedOriginalFileRelativePath"
        }
    }
    
    $fileRelativePath = $modifiedOriginalFileRelativePathWithBackslashes = $modifiedOriginalFileRelativePath.Replace('/', '\')
    if ($fileRelativePath[0] -eq '\') {
        $fileRelativePath = $fileRelativePath.Substring(1, ($fileRelativePath.Length - 1))
    }

    # Determine if file is in Github
    $fullPath = [IO.Path]::Combine($githubRepositoryDirectory, $fileRelativePath)
    $fileIsInGithub = Test-Path $fullPath

    if ($fileIsInGithub) {
        $splitLine[1] = "GITHUB"
        $splitLine[6] = $modifiedOriginalFileRelativePath
        $splitLine += "GITHUB_EXTRACT_CMD"

        # The local filesystem is often case-insensitive (and case-preserving), while Github URLs are case-sensitive.
        # It is possible to end up with a relative path in the source entry that, when used to construct
        # a Github URL, will result in an HTTP 404 error. As such, we need to look up the canonical full
        # path in the repository clone to determine the actual casing, and correct the source entry if
        # necessary. Generally this is because the itemspec in an MSBuild project file used casing that
        # differed from what was on disk. We'll log the fixups so that we can go back later and amend the 
        # project files.
        # 
        # We are aided in this endeavor by the fact that Powershell's filesystem provider *is* internally
        # aware of casing and can indirectly surface that information through wildcard matches.
        $canonicalFileRelativePath = $FilePathCache[$fullPath]
        if (-not $canonicalFileRelativePath) {
            $canonicalFullPath = Get-ChildItem -Path $fullPath.Replace('\', '\*') | Where FullName -ieq $fullPath | Select -ExpandProperty FullName
            $canonicalFileRelativePath = $canonicalFullPath.Substring($canonicalFullPath.Length - $modifiedOriginalFileRelativePath.Length, $modifiedOriginalFileRelativePath.Length)
            $FilePathCache[$fullPath] = $canonicalFileRelativePath
        }

        if ($canonicalFileRelativePath -cne $modifiedOriginalFileRelativePathWithBackslashes) {
            $splitLine[6] = $canonicalFileRelativePath.Replace('\', '/')

            Write-Host "##[debug]Incorrectly cased source entry: '$modifiedOriginalFileRelativePath' => '$($splitLine[6])'"
        }
    }
    else {
        $splitLine += "TFS_EXTRACT_CMD"
    }

    return $splitLine -join '*'
}

$ErrorActionPreference = "Stop"

$TempDirectory = if ($env:AGENT_TEMPDIRECTORY) { $env:AGENT_TEMPDIRECTORY } else { $env:TEMP }
$LogDirectory = if ($env:OB_OUTPUTDIRECTORY) { $env:OB_OUTPUTDIRECTORY } else { $TempDirectory }

# Querying the filesystem to get the canonical paths for files is very expensive, and we do it
# repeatedly for every indexed file. Let's make that a bit cheaper.
$FilePathCache = @{}

# Build and validate full paths from the parameters. 
# Note: Path.Combine returns the second argument if it is an absolute path
$GithubRepositoryDirectoryFullPath = [IO.Path]::Combine($pwd, $GithubRepositoryDirectory)
if (-not (Test-Path $GithubRepositoryDirectoryFullPath)) { 
    throw "Parameter 'GithubRepositoryDirectory' does not point to a valid path"
}
$SymbolsDirectoryFullPath = [IO.Path]::Combine($pwd, $SymbolsDirectory)
if (-not (Test-Path $SymbolsDirectoryFullPath)) { 
    throw "Parameter 'SymbolsDirectory' does not point to a valid path"
}

$installerPath = $null
try
{
    Write-Host "##[group]Ensure Debugging Tools for Windows is installed"
    $needsDebuggingToolsInstall = $false
    $debuggingToolsRoot = Get-DebuggingToolsRoot
    if (!$debuggingToolsRoot)
    {
        Write-Host "##[debug]Debugging Tools for Windows is not installed"
        $needsDebuggingToolsInstall = $true
    }
    else
    {

        $srcsrvRootx86 = [IO.Path]::Combine($debuggingToolsRoot, "x86", "srcsrv")
        $srcsrvRootx64 = [IO.Path]::Combine($debuggingToolsRoot, "x64", "srcsrv")

        $neededBinaries = @(
            [IO.Path]::Combine($srcsrvRootx86, "dbghelp.dll"),
            [IO.Path]::Combine($srcsrvRootx86, "pdbstr.exe"),
            [IO.Path]::Combine($srcsrvRootx86, "srctool.exe"),
            [IO.Path]::Combine($srcsrvRootx64, "dbghelp.dll"),
            [IO.Path]::Combine($srcsrvRootx64, "pdbstr.exe"),
            [IO.Path]::Combine($srcsrvRootx64, "srctool.exe"))

        foreach ($neededBinary in $neededBinaries) {
            if (!(Test-Path $neededBinary)) {
                Write-Host "##[debug]'$neededBinary' is missing from Debugging Tools for Windows installation"
                $needsDebuggingToolsInstall = $true
                break
            }
        }
    }

    if ($needsDebuggingToolsInstall)
    {
        $installerPath = Install-DebuggingTools $WindowsBuildNumber

        $debuggingToolsRoot = Get-DebuggingToolsRoot
        $archDir = if (Get-Is64Bit) { "x64" } else { "x86" }
        $srcsrvRoot = [IO.Path]::Combine($debuggingToolsRoot, $archDir, "srcsrv")
    }
    Write-Host "##[endgroup]"

    Write-Host "##[debug]Debugging Tools location: '$debuggingToolsRoot'"

    # Process the PDBs
    foreach ($pdb in (Get-ChildItem -Recurse -Filter "*.pdb" $SymbolsDirectoryFullPath))
    {
        $inputPdbPath = $pdb.FullName
        Rewrite-SrcSrvStream $srcsrvRootx86 $srcsrvRootx64 $inputPdbPath $GithubRepositoryName $GithubRepositoryDirectoryFullPath $CommitHash
    }

}
finally
{
    # If we installed the debugging tools, uninstall them as part of cleanup
    if ($installerPath)
    {
        Uninstall-DebuggingTools $installerPath
    }
}
