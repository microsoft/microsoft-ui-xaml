# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

# 
# MarkTestsWithMasterFiles.ps1:
#
#   A lot of files under dxaml\test have associated DComp or visual tree baseline files that are used
#   to ensure that changes don't accidentally change visuals without that being intended.
#   However, it can be difficult to figure out what are all the tests with associated baseline files
#   when a lot of them need to be updated. This script can be used to find and mark those tests
#   with metadata that enables them to be picked out of the full list of tests and run independently
#   of all other tests, which generates new masters that can be used to replace the old masters.
#   You can use the switch -TestsWithMasterFilesOnly when using runtests.cmd to make use of this
#   metadata to only run tests with associated baseline files.
#

$projectRoot = Join-Path $PSScriptRoot ".."
$masterFiles = Get-ChildItem -Path "$projectRoot\dxaml\test\resources\masters" -Filter *.master.xml -File
$cppHeaderFiles = Get-ChildItem -Path "$projectRoot\dxaml\test\native" -Filter *.h -File -Recurse
$csFiles = Get-ChildItem -Path "$projectRoot\dxaml\test\managed" -Filter *.cs -File -Recurse

# First, we'll get which test classes and which tests in those classes have associated baseline files.
[System.Collections.Generic.Dictionary[string, System.Collections.Generic.List[string]]]$testClassToTestListDictionary = @{}

# Some files make it unclear whether an underscore is part of a test name or is part of a variation,
# so to account for that, we associate each name with the other names that they could substitute for.
[System.Collections.Generic.Dictionary[string, System.Collections.Generic.List[string]]]$testNameToRelatedTestNamesDictionary = @{}

foreach ($masterFile in $masterFiles)
{
    # Baseline file names look like this: $(Namespace)_$(TestClassName)_$(TestName).$(Variation).<baseline-ext>.$(Platform).xml
    # We only care about the test class name and the test name, so we'll split those out from the rest of the file name.
    [System.Collections.Generic.List[string]]$fileNameComponents = $masterFile.Name.Split('.')
    [System.Collections.Generic.List[string]]$testNameComponents = $fileNameComponents[0].Split('_')

    # Some tests have an underscore in the test name, which throws an annoying wrench into trying to determine what the actual test name is
    # from the baseline file name. To work around that, we'll iterate through our test files and check to see if what we think is the test class
    # is actually found in a souce file.  This is very slow, but this script won't be run very often, so that's probably fine.
    Write-Host "Finding class name for baseline file '$($masterFile.Name)'..." -NoNewline
    
    # If we've already found a test class for this baseline file, we'll just use that.
    for ($testClassNameIndex = $testNameComponents.Count - 2; $testClassNameIndex -ge 0; $testClassNameIndex--)
    {
        $testClassName = $testNameComponents[$testClassNameIndex]
        $testName = $testNameComponents[$testClassNameIndex + 1].Split('#')[0]

        if ($testClassToTestListDictionary.Keys.Contains($testClassName))
        {
            break
        }
    }
    
    if ($testClassNameIndex -lt 0)
    {
        for ($testClassNameIndex = $testNameComponents.Count - 2; $testClassNameIndex -ge 0; $testClassNameIndex--)
        {
            $testClassName = $testNameComponents[$testClassNameIndex]
            $testName = $testNameComponents[$testClassNameIndex + 1].Split('#')[0]

            $testClassNameFound = $false
 
            foreach ($file in $cppHeaderFiles)
            {
                $fileContents = Get-Content $file.FullName -Raw
                $testClassNameMatch = [System.Text.RegularExpressions.Regex]::Match($fileContents, "BEGIN_TEST_CLASS\s*\(\s*($testClassName)\s*\)", [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)

                if ($testClassNameMatch.Success)
                {
                    $testClassName = $testClassNameMatch.Groups[1].Value
                    $testClassNameFound = $true
                    break
                }
            }

            if (-not $testClassNameFound)
            {
                foreach ($file in $csFiles)
                {
                    $fileContents = Get-Content $file.FullName -Raw
                    $testClassNameMatch = [System.Text.RegularExpressions.Regex]::Match($fileContents, "public\s+class\s+($testClassName)", [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)

                    if ($testClassNameMatch.Success)
                    {
                        $testClassName = $testClassNameMatch.Groups[1].Value
                        $testClassNameFound = $true
                        break
                    }
                }
            }

            if ($testClassNameFound)
            {
                break
            }
        }
    }

    if ($testClassNameIndex -lt 0)
    {
        Write-Host ""
        Write-Warning "Could not find associated test class name for master file '$($masterFile.Name)'!"
        continue
    }

    Write-Host " $testClassName"

    # If this is the first time we've seen this test class/name pair, let's add it to the list.
    if (-not $testClassToTestListDictionary.ContainsKey($testClassName))
    {
        $testClassToTestListDictionary.Add($testClassName, [System.Collections.Generic.List[string]]::new())
    }

    [System.Collections.Generic.List[string]]$testList = $testClassToTestListDictionary[$testClassName]

    if (-not $testList.Contains($testName))
    {
        $testList.Add($testName)
    }

    # To account for the possibility that a test might have an underscore in its name, we'll add every test name component after
    # what we think was the test name as possible full names, in order to ensure that we don't miss any of those.
    if ($testClassNameIndex -lt $testNameComponents.Count - 2)
    {
        [System.Collections.Generic.List[string]]$possibleFullTestNames = @()

        $possibleFullTestName = $testName
        $possibleFullTestNames.Add($possibleFullTestName)

        for ($i = $testClassNameIndex + 2; $i -lt $testNameComponents.Count; $i++)
        {
            $possibleFullTestName = "${possibleFullTestName}_$($testNameComponents[$i])"

            if (-not $testList.Contains($possibleFullTestName))
            {
                $testList.Add($possibleFullTestName)
                $possibleFullTestNames.Add($possibleFullTestName)
            }
        }

        foreach ($possibleFullTestName in $possibleFullTestNames)
        {
            if (-not $testNameToRelatedTestNamesDictionary.ContainsKey($possibleFullTestName))
            {
                $testNameToRelatedTestNamesDictionary.Add($possibleFullTestName, [System.Collections.Generic.List[string]]::new())
            }
                
            [System.Collections.Generic.List[string]]$relatedTestNames = $testNameToRelatedTestNamesDictionary[$possibleFullTestName]

            foreach ($possibleAlternateFullTestName in $possibleFullTestNames)
            {
                if ($possibleAlternateFullTestName -ne $possibleFullTestName)
                {
                    $relatedTestNames.Add($possibleAlternateFullTestName)
                }
            }
        }
    }
}

# Now that we know all of the tests with associated baseline files, we'll find source files associated with those.
# There can sometimes be multiple files with the same test class name (e.g., managed and native tests both have an
# instance of ButtonIntegrationTests), so we'll maintain a list of source files instead of having a 1:1 mapping.
[System.Collections.Generic.Dictionary[string, System.Collections.Generic.List[string]]]$testClassToFilePathListDictionary = @{}

# To detect the situation of orphaned baseline files without associated tests, we'll track which tests we've found
# associated baseline files for.
[System.Collections.Generic.Dictionary[string, System.Collections.Generic.List[string]]]$testClassToFoundTestListDictionary = @{}

$orderedKeyList = $testClassToTestListDictionary.Keys | Sort-Object

foreach ($testClassName in $orderedKeyList)
{
    Write-Host "Finding source files for test class '$testClassName'..."

    if (-not $testClassToFilePathListDictionary.ContainsKey($testClassName))
    {
        $testClassToFilePathListDictionary.Add($testClassName, [System.Collections.Generic.List[string]]::new())
    }

    if (-not $testClassToFoundTestListDictionary.ContainsKey($testClassName))
    {
        $testClassToFoundTestListDictionary.Add($testClassName, [System.Collections.Generic.List[string]]::new())
    }
    
    [System.Collections.Generic.List[string]]$foundTestList = $testClassToFoundTestListDictionary[$testClassName]
    [System.Collections.Generic.List[string]]$filePathList = $testClassToFilePathListDictionary[$testClassName]

    foreach ($file in $cppHeaderFiles + $csFiles)
    {
        $filePath = $file.FullName
        $fileContents = Get-Content $filePath -Raw

        if ([System.Text.RegularExpressions.Regex]::IsMatch($fileContents, "class\s+($testClassName)", [System.Text.RegularExpressions.RegexOptions]::IgnoreCase))
        {
            $testNameList = $testClassToTestListDictionary[$testClassName]

            foreach ($testName in $testNameList)
            {
                if ($foundTestList.Contains($testName))
                {
                    continue
                }

                if ([System.Text.RegularExpressions.Regex]::IsMatch($fileContents, "public\s+void\s+$testName\s*\(\s*\)", [System.Text.RegularExpressions.RegexOptions]::IgnoreCase) `
                        -or [System.Text.RegularExpressions.Regex]::IsMatch($fileContents, "(BEGIN_TEST_METHOD\s*\(\s*$testName\s*\)|TEST_METHOD\s*\(\s*$testName\s*\))", [System.Text.RegularExpressions.RegexOptions]::IgnoreCase))
                {
                    Write-Host "    Found test '$testName' in source file '$($file.Name)'."

                    if (-not $filePathList.Contains($filePath))
                    {
                        $filePathList.Add($filePath)
                    }

                    if (-not $foundTestList.Contains($testName))
                    {
                        $foundTestList.Add($testName)
                    }
                }
            }
        }
    }
}

foreach ($testClassName in $orderedKeyList)
{
    [System.Collections.Generic.List[string]]$testList = $testClassToTestListDictionary[$testClassName]
    [System.Collections.Generic.List[string]]$foundTestList = $testClassToFoundTestListDictionary[$testClassName]

    foreach ($testName in $testList)
    {
        $nameWasFound = $foundTestList.Contains($testName)

        if (-not $nameWasFound -and $testNameToRelatedTestNamesDictionary.ContainsKey($testName))
        {
            [System.Collections.Generic.List[string]]$relatedTestNames = $testNameToRelatedTestNamesDictionary[$testName]

            foreach ($relatedTestName in $relatedTestNames)
            {
                $nameWasFound = $nameWasFound -or $foundTestList.Contains($relatedTestName)
            }
        }

        if (-not $nameWasFound)
        {
            Write-Warning "Could not find a source file containing the test class/name pair '$testClassName' and '$testName'!"
        }
    }
}

foreach ($testClassName in $orderedKeyList)
{
    foreach ($filePath in $testClassToFilePathListDictionary[$testClassName])
    {
        $isCsFile = [System.IO.Path]::GetExtension($filePath) -ieq ".cs"

        Write-Host "Tagging test methods in $filePath..."

        $fileContents = Get-Content $filePath -Raw
        $originalFileContents = $fileContents

        foreach ($testName in $testClassToFoundTestListDictionary[$testClassName])
        {
            if ($isCsFile)
            {
                $propertyToAdd = "[TestProperty(`"HasAssociatedMasterFile`", `"True`")]"
                $match = [System.Text.RegularExpressions.Regex]::Match($fileContents, "(([ \t]*)\[\s*TestMethod\s*]\s*(?:\[TestProperty.*\s*)*)(\s*public\s+void\s+$testName\s*\(\s*\))", [System.Text.RegularExpressions.RegexOptions]::IgnoreCase -bor [System.Text.RegularExpressions.RegexOptions]::Multiline)

                if ($match.Success)
                {
                    $matchText = $match.Value

                    if (-not $matchText.Contains($propertyToAdd))
                    {
                        $attributes = $match.Groups[1].Value
                        $indentation = $match.Groups[2].Value
                        $methodHeader = $match.Groups[3].Value
                        $replacementText = "${attributes}${propertyToAdd}$([Environment]::NewLine)${indentation}${methodHeader}"

                        $fileContents = $fileContents.Replace($matchText, $replacementText)
                    }
                }
            }
            else
            {
                $propertyToAdd = "TEST_METHOD_PROPERTY(L`"HasAssociatedMasterFile`", L`"True`")"
                $match = [System.Text.RegularExpressions.Regex]::Match($fileContents, "(([ \t]*)BEGIN_TEST_METHOD\s*\(\s*$testName\s*\))((?:.*?\s*?)*?)(\s*END_TEST_METHOD\s*\(\s*\))", [System.Text.RegularExpressions.RegexOptions]::IgnoreCase -bor [System.Text.RegularExpressions.RegexOptions]::Multiline)

                if ($match.Success)
                {
                    $matchText = $match.Value

                    if (-not $matchText.Contains($propertyToAdd))
                    {
                        $beginTestMethod = $match.Groups[1].Value
                        $indentation = $match.Groups[2].Value
                        $testProperties = $match.Groups[3].Value
                        $endTestMethod = $match.Groups[4].Value
                        $replacementText = "${beginTestMethod}${testProperties}$([Environment]::NewLine)${indentation}    ${propertyToAdd}${endTestMethod}"

                        $fileContents = $fileContents.Replace($matchText, $replacementText)
                    }
                }
                else
                {
                    $match = [System.Text.RegularExpressions.Regex]::Match($fileContents, "([ \t]*)TEST_METHOD\s*\(\s*$testName\s*\)", [System.Text.RegularExpressions.RegexOptions]::IgnoreCase -bor [System.Text.RegularExpressions.RegexOptions]::Multiline)

                    if ($match.Success)
                    {
                        $matchText = $match.Value
                        $indentation = $match.Groups[1].Value
                        $replacementText = "${indentation}BEGIN_TEST_METHOD($testName)$([Environment]::NewLine)${indentation}    ${propertyToAdd}$([Environment]::NewLine)${indentation}END_TEST_METHOD()"

                        $fileContents = $fileContents.Replace($matchText, $replacementText)
                    }
                }
            }
        }

        if ($fileContents -ne $originalFileContents)
        {
            Set-Content $filePath $fileContents
        }
    }
}