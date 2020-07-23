Param(
    [Parameter(Mandatory = $true)]
    [string]$updatedVisualTreeVerificationFolder,
    [string]$outputFolder = "..\test\MUXControlsTestApp\verification"
)

$currentVisualTreeVerificationFolder = "..\test\MUXControlsTestApp\verification"
$maxOSVersionNumber = 8

if( -Not (Test-Path $outputFolder) )
{
    New-Item $outputFolder -ItemType Directory
}

if(Test-Path $updatedVisualTreeVerificationFolder)
{
    $updatedVerificationFiles = Get-ChildItem $UpdatedVisualTreeVerificationFolder
    $currentVerificationFiles = Get-ChildItem $CurrentVisualTreeVerificationFolder
    $prefixList = @()
    foreach($file in $UpdatedVerificationFiles)
    {
        $prefix = $file.Name.Split('-')[0]
        if($prefixList -NotContains $prefix)
        {
            $prefixList += $prefix
        }
    }

    foreach($prefix in $prefixList)
    {
        $updatedVersionedVerificationFiles = $updatedVerificationFiles | Where { $_.Name.StartsWith("$prefix-") } | Sort-Object -Property Name
        $currentBaseVerificationFile = $currentVerificationFiles | Where { $_.Name.StartsWith("$prefix.") }
        $currentVersionedVerificationFiles = $currentVerificationFiles | Where { $_.Name.StartsWith("$prefix-") } | Sort-Object -Property Name

        $currentVerificationFilesIndexes = @()
        $updatedVerificationFilesIndexes = @()
        for($i=1; $i -lt $maxOSVersionNumber + 1; $i++)
        {
            $currentVersionedVerificationFileForI = $currentVersionedVerificationFiles | Where {$_.Name.StartsWith("$prefix-$i.")}
            if($currentVersionedVerificationFileForI.Count -gt 0)
            {
                $currentVerificationFilesIndexes += $i
            }

            $updatedVersionedVerificationFileForI = $updatedVersionedVerificationFiles | Where {$_.Name.StartsWith("$prefix-$i.")}
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
                $updatedVersionedVerificationFileForI = $updatedVersionedVerificationFiles | Where {$_.Name.StartsWith("$prefix-$i.")}
                $finalVersionedVerificationFiles += $updatedVersionedVerificationFileForI
			}
            else
            {
                $currentVerificationFilesIndexesLessThanI = $currentVerificationFilesIndexes -le $i | Sort-Object -Descending
                if($currentVerificationFilesIndexesLessThanI.Count -gt 0)
                {
                    $currentVersionedVerificationFileIndexForI = $currentVerificationFilesIndexesLessThanI[0]
                    $currentVersionedVerificationFileForI = $currentVersionedVerificationFiles | Where {$_.Name.StartsWith("$prefix-$currentVersionedVerificationFileIndexForI.")}
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

        # This call will produce an error if the souce of the $prefix.xml file is the output directory, which happens frequently.
        Try
        {
            Copy-Item $finalVersionedVerificationFiles[0].FullName "$outputFolder\$prefix.xml" -Force -ErrorAction Stop
            Write-Host "Copied $($finalVersionedVerificationFiles[0].FullName) as updated $prefix.xml"
        }
        catch
        {
            Write-Host "$prefix.xml was up to date."
		}

        foreach($i in $indexesToPublish)
        {
            $j = $i+1
            Copy-Item $finalVersionedVerificationFiles[$i].FullName "$outputFolder\$prefix-$j.xml" -Force
            Write-Host "Copied $($finalVersionedVerificationFiles[$i].FullName) as updated $prefix-$j.xml"
		}
    }
}