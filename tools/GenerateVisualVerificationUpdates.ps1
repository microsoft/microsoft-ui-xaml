Param(
    [Parameter(Mandatory = $true)]
    [string]$updatedVisualTreeVerificationFolder,
    [string]$outputFolder = "..\test\MUXControlsTestApp\verification"
)

# This script is responsible for updating the visual verification files that are checked into the project by comparing the tests result files to the files currently checked in.
# It does this by first, for each visual verification test, determining which file is used for each os version.  For example, the OS versions are given numbers between 1 and 8, and we
# combine verification files where possible so we might have ColorPicker.xml Colorpicker-4.xml and ColorPicker-7.xml.  This means that ColorPicker.xml is used for OS version 1-3,
# ColorPicker 4 is used for OS versions 4-6 and Colorpicker-7 is used for OS versions 7 and 8.  We then make an array of files where the index in the array is the OS version number-1.
# we then replace each file in this array with the file from the test pass if that file exists, so in this example if we had updates for OS versions 5 we would have an array
# as such [1,1,1,4,5,4,7,7]  We then combine the files where possible by comparing each file in the array to the next file.  In this example that would result in a Colorpicker.xml and
# a -4.xml,-5.xml,-6.xml, and -7.xml files where the -6.xml file was a copy of the original -4.xml.  We then delete all of the files that were in the verifications folder and then
# copy the files that remain in the array to the verification folder.

$staging = "staging"
$currentVisualTreeVerificationFolder = "..\test\MUXControlsTestApp\verification"
$maxOSVersionNumber = 8

if( -Not (Test-Path $outputFolder) )
{
    New-Item $outputFolder -ItemType Directory
}
if( -Not (Test-Path $staging) )
{
    New-Item $staging -ItemType Directory
}

if(Test-Path $updatedVisualTreeVerificationFolder)
{
    $updatedVerificationFiles = Get-ChildItem $UpdatedVisualTreeVerificationFolder
    $currentVerificationFiles = Get-ChildItem $CurrentVisualTreeVerificationFolder
    $baseNameList = @()
    foreach($file in $UpdatedVerificationFiles)
    {
        $baseName = $file.Name.Split('-')[0]
        if($baseNameList -NotContains $baseName)
        {
            $baseNameList += $baseName
        }
    }

    if($baseNameList.Count -lt 1)
    {
        Write-Host "No verification files were found, did you call the script on the correct directory?"
	}

    foreach($baseName in $baseNameList)
    {
        Write-Host "Processing updates for $baseName"
        $updatedVersionedVerificationFiles = $updatedVerificationFiles | Where { $_.Name.StartsWith("$baseName-") } | Sort-Object -Property Name
        $currentBaseVerificationFile = $currentVerificationFiles | Where { $_.Name.StartsWith("$baseName.") }
        $currentVersionedVerificationFiles = $currentVerificationFiles | Where { $_.Name.StartsWith("$baseName-") } | Sort-Object -Property Name

        $currentVerificationFilesIndexes = @()
        $updatedVerificationFilesIndexes = @()
        for($i=1; $i -lt $maxOSVersionNumber + 1; $i++)
        {
            $currentVersionedVerificationFileForI = $currentVersionedVerificationFiles | Where {$_.Name.StartsWith("$baseName-$i.")}
            if($currentVersionedVerificationFileForI.Count -gt 0)
            {
                $currentVerificationFilesIndexes += $i
            }

            $updatedVersionedVerificationFileForI = $updatedVersionedVerificationFiles | Where {$_.Name.StartsWith("$baseName-$i.")}
            if($updatedVersionedVerificationFileForI.Count -gt 0)
            {
                $updatedVerificationFilesIndexes += $i
            }
        }

        $finalVersionedVerificationFiles = @()
        for($i=1; $i -lt $maxOSVersionNumber + 1; $i++)
        {
            if($updatedVerificationFilesIndexes -contains $i)
            {
                $updatedVersionedVerificationFileForI = $updatedVersionedVerificationFiles | Where {$_.Name.StartsWith("$baseName-$i.")}
                $finalVersionedVerificationFiles += $updatedVersionedVerificationFileForI
            }
            else
            {
                $currentVerificationFilesIndexesLessThanI = $currentVerificationFilesIndexes -le $i | Sort-Object -Descending
                if($currentVerificationFilesIndexesLessThanI.Count -gt 0)
                {
                    $currentVersionedVerificationFileIndexForI = $currentVerificationFilesIndexesLessThanI[0]
                    $currentVersionedVerificationFileForI = $currentVersionedVerificationFiles | Where {$_.Name.StartsWith("$baseName-$currentVersionedVerificationFileIndexForI.")}
                    $finalVersionedVerificationFiles += $currentVersionedVerificationFileForI        
                }
                else
                {
                    $finalVersionedVerificationFiles += $currentBaseVerificationFile				
                }
            }
        }

        $indexesToPublish = @()
        for ($i=0; $i -lt $finalVersionedVerificationFiles.Length-1; $i++)
        {
            $v1 = Get-Content $finalVersionedVerificationFiles[$i].FullName
            $v2 = Get-Content $finalVersionedVerificationFiles[$i+1].FullName
            $diff = Compare-Object $v1 $v2
            if($diff.Length -ne 0)
            {
                $indexesToPublish += $i+1
            }
        }
        
        Copy-Item $finalVersionedVerificationFiles[0].FullName "$staging\$baseName.xml" -Force
        foreach($i in $indexesToPublish)
        {
            $j = $i+1
            Copy-Item $finalVersionedVerificationFiles[$i].FullName "staging\$baseName-$j.xml" -Force
            Write-Host "Copied $($finalVersionedVerificationFiles[$i].FullName) as updated $baseName-$j.xml"
        }
        Remove-Item $outputFolder\$baseName.xml
        Remove-Item $outputFolder\$baseName-*.xml
        Copy-item -Force -Recurse "$staging\*" -Destination $outputFolder
        Remove-Item $staging\*.*
    }
    Remove-Item $staging
}
else
{
    Write-Host "Invalid path to visual verification updates"
}