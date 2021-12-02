<#
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

this file work is a derivative of https://github.com/microsoft/MixedReality-GraphicsTools-Unreal/blob/main/Tools/scripts/Common.psm1
#>

<#
.SYNOPSIS
    Run clang-format on all source files.
.PARAMETER Path
    Path to the directory (or file) to be processed recursively. By default scans the entire repo.
.PARAMETER ClangFormat
    Path to clang-format executable, e.g. "C:\Tools\clang-format.exe"
.PARAMETER ModifiedOnly
    Scan only files modified in current git checkout.
.PARAMETER Staged
    Check only files staged for commit
.PARAMETER ChangesFile
    Scan only files listed in provided txt file (one path per line). Paths need to be relative to repo root.
.PARAMETER Verify
    Whether to fail if files are not formatted (instead of applying changes).
.PARAMETER UseVS2019
    Use ClangFormat provided with Visual Studio 2019. This version will take precedence over -ClangFormat parameter
    and PATH variable.
.PARAMETER NoFail`
    Do not set RC=1 when errors found, i.e. only report errors in output.
#>
[CmdletBinding()]
param (
    [string]$Path = $null,
    [string]$ClangFormat = $null,
    [boolean]$ModifiedOnly = $True,
    [boolean]$Staged = $false,
    [string]$ChangesFile = $null,
    [boolean]$Verify = $false,
    [boolean]$UseVS2019 = $true,
    [boolean]$NoFail = $false
)

# Only check source files
$FilePatterns = "\.(h|cpp|hpp|c)$"

#PerfApp is C++/Cx project and not supported
$IgnoreFolders = "(Generated Files|GeneratedExperimental|test|Generated|nodejs|vscode|community|dotnet|node_modules|BuildOutput|out|.git|.vs|.vscode|bin|CMakeFiles|generated|debug|x64)$"

$RepoRoot = (Resolve-Path "$PSScriptRoot\..\dev")

<#
.SYNOPSIS
    Given the path to the list of raw git changes, returns an array of
    those changes rooted in the git root directory.
.DESCRIPTION
    For example, the raw git changes will contain lines like:

    Assets/File.cs

    This function will return a list of paths that look like (assuming
    that RepoRoot is C:\repo):

    C:\repo\Assets\File.cs
#>
function GetChangedFiles { 
    [CmdletBinding()]
    param(
        [string]$Filename,
        [string]$RepoRoot
    )
    process {
        $rawContent = Get-Content -Path $Filename
        $processedContent = @()
        foreach ($line in $rawContent) {
            $joinedPath = Join-Path -Path $RepoRoot -ChildPath $line
            $processedContent += $joinedPath
        }
        $processedContent
    }
}

if ([string]::IsNullOrEmpty($Path))
{
    $Path = $RepoRoot
}

$ModifiedFiles = $null

if ($ChangesFile)
{
    if (-not (Test-Path -Path $ChangesFile -PathType Leaf))
    {
        Write-Host -ForegroundColor Red "ChangesFile not found: $ChangesFile"
        Write-Host "Checking all source files"
    }
    else
    {
        $ModifiedFiles = GetChangedFiles -Filename $ChangesFile -RepoRoot $RepoRoot
        if (($null -eq $ModifiedFiles) -or ($ModifiedFiles.Count -eq 0))
        {
            Write-Host -ForegroundColor Green "No modified files to format."
            exit 0
        }
    }
}
elseif ($ModifiedOnly -or $Staged)
{
    $ModifiedFiles = @()
    Push-Location -Path $RepoRoot
    $Success = $False
    try
    {
        if ($Staged)
        {
            $Status = (& git diff-index --cached --name-only HEAD)
            $Success = ($LASTEXITCODE -eq 0)
            $Status | ForEach-Object {
                $FullPath = Resolve-Path $_ -ErrorAction SilentlyContinue
                $FileName = Split-Path -Leaf -Path $FullPath
                if ($FileName -match $FilePatterns)
                {
                    $ModifiedFiles += $FullPath
                }
            }
        }
        else
        {
            $Status = (& git status --porcelain)
            $Success = ($LASTEXITCODE -eq 0)
            $Status | ForEach-Object {
                $FullPath = (Resolve-Path ($_.Trim() -split " ",2)[-1] -ErrorAction SilentlyContinue)
                $FileName = Split-Path -Leaf -Path $FullPath
                if ($FileName -match $FilePatterns)
                {
                    $ModifiedFiles += $FullPath
                }
            }
        }
    }
    catch
    {
        # empty
    }
    if (-not $Success)
    {
        Write-Host -ForegroundColor Red "Could not get the list of modified files. Check if git is configured correctly."
        exit 1
    }
    Pop-Location
    if (($null -eq $ModifiedFiles) -or ($ModifiedFiles.Count -eq 0))
    {
        Write-Host -ForegroundColor Green "No modified files to format."
        exit 0
    }
}

if ($UseVS2019)
{
    # Attempt to use clang-format from Visual Studio 2019 if available
    $LLVMDirs = @("${Env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Enterprise\VC\Tools\Llvm",
                  "${Env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Professional\VC\Tools\Llvm",
                  "${Env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Community\VC\Tools\Llvm")
    foreach ($LLVMDir in $LLVMDirs)
    {
        $VS2019ClangFormatX64 = "$LLVMDir\x64\bin\clang-format.exe"
        $VS2019ClangFormatX86 = "$LLVMDir\bin\clang-format.exe"
        if (Test-Path -Type Leaf -Path $VS2019ClangFormatX64)
        {
            $ClangFormat = $VS2019ClangFormatX64
            break
        }
        elseif (Test-Path -Type Leaf -Path $VS2019ClangFormatX86)
        {
            $ClangFormat = $VS2019ClangFormatX86
            break
        }
    }
}
elseif ([string]::IsNullOrEmpty($ClangFormat) -or (-not (Test-Path -Type Leaf -Path $ClangFormat)))
{
    # Check for clang-format in PATH
    $ClangFormat = (Get-Command -Name "clang-format.exe" -ErrorAction SilentlyContinue)
}

if ([string]::IsNullOrEmpty($ClangFormat))
{
    Write-Host -ForegroundColor Red "clang-format.exe not found. Please install VS2019 or make sure clang-format.exe is in PATH."
    exit 1
}

Write-Host "[clang-format] Path is $ClangFormat"  
Write-Host "[clang-format] Version is: "
    & $ClangFormat --version

function Format-Directory
{
    [CmdletBinding()]
    param (
        [Parameter(Mandatory=$True)]
        [string]$Path,
        [Parameter(Mandatory=$True)]
        [string]$ClangFormat,
        [string]$RepoRoot = $null,
        [AllowEmptyString()][string]$FilePatterns = $null,
        [string[]]$ModifiedFiles = $null,
        [boolean]$Verify = $false
    )
    process
    {
        if (-not (Test-Path -Path $Path))
        {
            Write-Host -ForegroundColor Red "Item not found: $Path"
            return $False
        }
        if ($null -eq $FilePatterns)
        {
            $FilePatterns = ""
        }
        $Path = Resolve-Path $Path
        $Success = $True
        $FilesToFormat = @()
        if ((Get-Item -Path $Path) -is [System.IO.DirectoryInfo])
        {
            Get-ChildItem -Path $Path -File `
            | Where-Object { $_ -match $FilePatterns } `
            | ForEach-Object {
                $FilePath = "$Path\$_"
                if (($null -eq $ModifiedFiles) -or ($ModifiedFiles -contains $FilePath))
                {
                    if (!($FilePath -match "Intermediate"))
                    {
                        $FilesToFormat += $FilePath
                    }
                }
            }
            Get-ChildItem -Path $Path -Directory `
            | Where-Object { $_ -notmatch $IgnoreFolders } `
            | ForEach-Object {
                $SubResult = (Format-Directory -Path "$Path\$_" `
                                               -ClangFormat $ClangFormat `
                                               -RepoRoot $RepoRoot `
                                               -FilePatterns $FilePatterns `
                                               -ModifiedFiles $ModifiedFiles `
                                               -Verify $Verify)
                $Success = $SubResult -and $Success
            }
        }
        else
        {
            $FilesToFormat += $Path
        }
        $FilesToFormat | ForEach-Object {
            if ($Verify)
            {
                Write-Host "[clang-format] Checking formatting: $_"
                & $ClangFormat --style=file -Werror --dry-run $_
            }
            else
            {
                Write-Host "[clang-format] Formatting $_"
                & $ClangFormat --style=file -Werror -i $_
            }
            $Success = (0 -eq $LASTEXITCODE) -and $Success
        }
        return $Success
    }
}

$Success = (Format-Directory -Path $Path `
                             -ClangFormat $ClangFormat `
                             -FilePatterns $FilePatterns `
                             -ModifiedFiles $ModifiedFiles `
                             -RepoRoot $RepoRoot `
                             -Verify $Verify)

if ($Success)
{
    Write-Host "Done."
    exit 0
}
else
{
    Write-Host "You may need to update Visual studio and have the correct clang-format version in your local machine"
    Write-Host -ForegroundColor Red "Errors found (see output). Please make sure to resolve all issues before opening a Pull Request."
    Write-Host -ForegroundColor Red "Formatting can be applied by running:"
    Write-Host -ForegroundColor Red "   PowerShell.exe -ExecutionPolicy Bypass $PSCommandPath -ModifiedOnly `$False` [-Path <path to file or directory>]"
    if ($NoFail)
    {
        exit 0  # do not prevent commit when used in pre-commit hook
    }
    exit 1
}