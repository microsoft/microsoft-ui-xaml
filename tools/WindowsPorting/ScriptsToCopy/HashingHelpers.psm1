[System.Collections.Generic.Dictionary[string, string]]$global:fileHashDictionary = @{}
[System.Collections.Generic.List[string]]$global:savedHashFileNames = @()

function Get-FilesToTrackChangesIn
{
    Get-ChildItem "$PSScriptRoot" -Exclude ".expectedEnlistmentFileHashes",".lastSyncedToCommit","*.ps1","*.psm1","*.cmd","sources.dep","autogen.*","Auto-OnecoreUapWindows.*","buildchk.*","buildfre.*" -File -Recurse | where { -not $_.FullName.StartsWith("$PSScriptRoot\winrt") }
}

function Get-FileSubPathForDictionary
{
    Param(
        [Parameter(Mandatory = $true)] 
        [string]$filePath
    )

    return $filePath.Replace($PSScriptRoot, "")
}

function Load-FileHashes
{
    Param(
        [Parameter(Mandatory = $true)] 
        [string]$fileName
    )

    if (Test-Path $fileName)
    {
        (Get-Content $fileName -Force) | ForEach-Object {
            $fileHashComponents = $_.Split("=")
            $global:fileHashDictionary[$fileHashComponents[0]] = $fileHashComponents[1]
        }
    }
}

function Get-FileHasChanged
{
    Param(
        [Parameter(Mandatory = $true)] 
        [string]$filePath
    )
    
    $fileSubPath = Get-FileSubPathForDictionary $filePath
    $fileName = Split-Path $filePath -Leaf

    if (-not (Test-Path -LiteralPath $filePath) -or (-not $global:fileHashDictionary.ContainsKey($fileSubPath) -and -not $global:fileHashDictionary.ContainsKey($fileName)))
    {
        return $true
    }
    else
    {
        if ($global:fileHashDictionary.ContainsKey($fileSubPath))
        {
            return $global:fileHashDictionary[$fileSubPath] -ne (Get-FileHash -LiteralPath $filePath).Hash
        }
        else
        {
            return $global:fileHashDictionary[$fileName] -ne (Get-FileHash -LiteralPath $filePath).Hash
        }
    }
}

function Preserve-FileHash
{
    Param(
        [Parameter(Mandatory = $true)] 
        [string]$filePath,
        [switch]$fileNameOnly
    )
    
    if ($fileNameOnly)
    {
        $fileSubPath = Split-Path $filePath -Leaf
    }
    else
    {
        $fileSubPath = Get-FileSubPathForDictionary $filePath
    }

    if (-not $global:savedHashFileNames.Contains($fileSubPath))
    {
        $global:savedHashFileNames.Add($fileSubPath)
    }
}

function Save-FileHash
{
    Param(
        [Parameter(Mandatory = $true)] 
        [string]$filePath,
        [switch]$fileNameOnly
    )
    
    if ($fileNameOnly)
    {
        $fileSubPath = Split-Path $filePath -Leaf
    }
    else
    {
        $fileSubPath = Get-FileSubPathForDictionary $filePath
    }

    $fileHash = (Get-FileHash -LiteralPath $filePath).Hash

    if ($global:fileHashDictionary.ContainsKey($fileSubPath))
    {
        $global:fileHashDictionary[$fileSubPath] = $fileHash
    }
    else
    {
        $global:fileHashDictionary.Add($fileSubPath, $fileHash)
    }

    if (-not $global:savedHashFileNames.Contains($fileSubPath))
    {
        $global:savedHashFileNames.Add($fileSubPath)
    }
}

function Commit-FileHashes
{
    Param(
        [Parameter(Mandatory = $true)] 
        [string]$fileName
    )

    $fileHashesFile = New-Item $fileName -ItemType File -Force
    [System.IO.StreamWriter]$streamWriter = [System.IO.File]::AppendText($fileHashesFile.FullName)

    foreach ($fileSubPath in $global:savedHashFileNames)
    {
        $streamWriter.WriteLine("$fileSubPath=$($global:fileHashDictionary[$fileSubPath])")
    }

    $streamWriter.Close()
}