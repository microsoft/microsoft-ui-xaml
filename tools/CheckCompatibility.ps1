[CmdLetBinding()]
Param()

Import-Module $PSScriptRoot\Utils.psm1

Add-Type -Language CSharp @"
using System;

namespace Compatibility
{
    public class Error
    {
        public Error(string filePath)
        {
            FilePath = filePath;
            LineNumber = 0;
            ColumnNumber = 0;
        }

        public Error(string filePath, int lineNumber, int columnNumber)
        {
            FilePath = filePath;
            LineNumber = lineNumber;
            ColumnNumber = columnNumber;
        }

        public Error(string filePath, int lineNumber, int columnNumber, string context)
        {
            FilePath = filePath;
            LineNumber = lineNumber;
            ColumnNumber = columnNumber;
            Context = context;
        }

        public string FilePath { get; private set; }
        public int LineNumber { get; private set; }
        public int ColumnNumber { get; private set; }
        public string Context { get; private set; }
    }
}
"@

$files = Get-ChildItem $PSScriptRoot\..\dev -Include "*.cpp","*.h" -Recurse

[System.Collections.Generic.List[Compatibility.Error]]$apiInformationErrors = @()

foreach ($file in $files)
{
    Write-Verbose "Checking $file"
    $lineNumber = 1
    
    if (-not $file.Name.Contains("SharedHelpers.cpp"))
    {
        Get-Content $file.FullName | ForEach-Object {
            $fileLine = $_
            $indexOfApiInformation = 0
            $characterOffset = 0

            while ($indexOfApiInformation -ge 0)
            {
                $indexOfApiInformation = $fileLine.IndexOf("winrt::ApiInformation")

                if ($indexOfApiInformation -ge 0)
                {
                    $apiInformationErrors.Add((New-Object Compatibility.Error -ArgumentList $file.FullName, $lineNumber, $($indexOfApiInformation + $characterOffset)))
                    $fileLine = $fileLine.Substring($indexOfApiInformation + 10)
                    $characterOffset += $indexOfApiInformation + 10
                }
            }

            $lineNumber++
        }
    }
}

foreach ($error in $apiInformationErrors)
{
    Write-ErrorInFile "Usage of winrt::ApiInformation should never occur outside of SharedHelpers.cpp. Otherwise, we can incur an unnecessary load of WinMD files." $error.FilePath $error.LineNumber $error.ColumnNumber
}

[System.Collections.Generic.List[Compatibility.Error]]$versionUsages = @()
[System.Collections.Generic.List[Compatibility.Error]]$missingWebhosthiddenAttributes = @()

$files = Get-ChildItem $PSScriptRoot\..\dev -Include "*.idl" -Exclude "Microsoft.UI.Xaml.idl","Microsoft.UI.Composition.Effects.idl" -Recurse

Write-Verbose ""
Write-Verbose "Checking IDL files..."
Write-Verbose ""

foreach ($file in $files)
{
    $lineNumber = 1
    Write-Verbose "$file"

    Get-Content $file.FullName | ForEach-Object {
        $fileLine = $_
        $indexOfVersion = 0
        $characterOffset = 0;

        while ($indexOfVersion -ge 0)
        {
            $indexOfVersion = $fileLine.IndexOf("version(")

            if ($indexOfVersion -ge 0)
            {
                $versionUsages.Add((New-Object Compatibility.Error -ArgumentList $file.FullName, $lineNumber, $($indexOfAbi + $characterOffset)))
                $fileLine = $fileLine.Substring($indexOfVersion + 8)
                $characterOffset += $indexOfVersion + 8
            }
        }

        $lineNumber++
    }

    ([regex]"(?m)(?:\[.*\]\s*)+(?:unsealed\s+)?(runtimeclass\s+(\w+))").Matches((Get-Content $file.FullName -Raw)) | ForEach-Object {
        $pattern = $_.Groups[1].Value
        $class = $_.Groups[2].Value
        $lineNumber = 1

        if ($_.Groups[0].Value -inotmatch "webhosthidden")
        {
            Get-Content $file.FullName | ForEach-Object {
                $column = $_.IndexOf($pattern)

                if ($column -ge 0)
                {
                    $missingWebhosthiddenAttributes.Add((New-Object Compatibility.Error -ArgumentList $file.FullName, $lineNumber, $_.IndexOf($class), $class))
                }

                $lineNumber++
            }
        }
    }
}

foreach ($error in $versionUsages)
{
    Write-ErrorInFile "Usage of version() detected, which will not compile on the Windows side.  Use MUX_PREVIEW or MUX_INTERNAL or WUXC_VERSION_RSx." $error.FilePath $error.LineNumber $error.ColumnNumber
}

foreach ($error in $missingWebhosthiddenAttributes)
{
    Write-ErrorInFile "Runtime class `"$($error.Context)`" is missing a webhosthidden attribute." $error.FilePath $error.LineNumber $error.ColumnNumber
}