[CmdLetBinding()]
Param(
    [Parameter(Mandatory = $true)] 
    [string]$TestFilePattern,

    [Parameter(Mandatory = $true)] 
    [string]$TestBinaryDirectoryPath,

    [Parameter(Mandatory = $true)] 
    [string]$OutputProjFile,

    [string]$TaefBaseQuery,

    [string]$TaefExtraParameters,

    [string]$TestTimeout = "00:30:00",

    [string]$TestNamePrefix,

    [string]$WorkItemPrefix,

    [int]$TestExecutionMultiplier = 1,

    [bool]$RunIgnoredTests = $false,

    [string]$TaefExePath,

    [switch]$FailOnTaefDiscoveryError,

    [hashtable]$ExcludedTestsByWorkItem,

    [switch]$AllowMissingExcludedTests
)

Class TestCollection
{
    [string]$Name
    [string]$SetupMethodName
    [string]$TeardownMethodName
    [System.Collections.Generic.Dictionary[string, string]]$Properties

    TestCollection()
    {
        if ($this.GetType() -eq [TestCollection])
        {
            throw "This class should never be instantiated directly; it should only be derived from."
        }
    }

    TestCollection([string]$name)
    {
        $this.Init($name)
    }

    hidden Init([string]$name)
    {
        $this.Name = $name
        $this.Properties = @{}
    }
}

Class Test : TestCollection 
{
    Test([string]$name)
    {
        $this.Init($name)
    }
}

Class TestClass : TestCollection
{
    [System.Collections.Generic.List[Test]]$Tests

    TestClass([string]$name)
    {
        $this.Init($name)
        $this.Tests = @{}
    }
}

Class TestModule : TestCollection
{
    [System.Collections.Generic.List[TestClass]]$TestClasses

    TestModule([string]$name)
    {
        $this.Init($name)
        $this.TestClasses = @{}
    }
}

function Parse-TestInfo([string]$taefOutput)
{
    enum LineType
    {
        None
        TestModule
        TestClass
        Test
        Setup
        Teardown
        Property
    }
        
    [string]$testModuleIndentation = "        "
    [string]$testClassIndentation = "            "
    [string]$testIndentation = "                "
    [string]$setupBeginning = "Setup: "
    [string]$teardownBeginning = "Teardown: "
    [string]$propertyBeginning = "Property["

    function Get-LineType([string]$line)
    {
        if ($line.Contains($setupBeginning))
        {
            return [LineType]::Setup;
        }
        elseif ($line.Contains($teardownBeginning))
        {
            return [LineType]::Teardown;
        }
        elseif ($line.Contains($propertyBeginning))
        {
            return [LineType]::Property;
        }
        elseif ($line.StartsWith($testModuleIndentation) -and -not $line.StartsWith("$testModuleIndentation "))
        {
            return [LineType]::TestModule;
        }
        elseif ($line.StartsWith($testClassIndentation) -and -not $line.StartsWith("$testClassIndentation "))
        {
            return [LineType]::TestClass;
        }
        elseif ($line.StartsWith($testIndentation) -and -not $line.StartsWith("$testIndentation "))
        {
            return [LineType]::Test;
        }
        else
        {
            return [LineType]::None;
        }
    }

    [string[]]$lineSeparators = @([Environment]::NewLine, "`n")
    [string[]]$lines = $taefOutput.Split($lineSeparators, [StringSplitOptions]::RemoveEmptyEntries)
    [System.Collections.Generic.List[TestModule]]$testModules = @()

    [TestModule]$currentTestModule = $null
    [TestClass]$currentTestClass = $null
    [Test]$currentTest = $null

    [TestCollection]$lastTestCollection = $null

    foreach ($rawLine in $lines)
    {
        [LineType]$lineType = (Get-LineType $rawLine)

        # We don't need the whitespace around the line anymore, so we'll discard it to make things easier.
        [string]$line = $rawLine.Trim()

        if ($lineType -eq [LineType]::TestModule)
        {
            if ($currentTest -ne $null -and $currentTestClass -ne $null)
            {
                $currentTestClass.Tests.Add($currentTest)
            }

            if ($currentTestClass -ne $null -and $currentTestModule -ne $null)
            {
                $currentTestModule.TestClasses.Add($currentTestClass)
            }

            if ($currentTestModule -ne $null)
            {
                $testModules.Add($currentTestModule)
            }

            $currentTestModule = [TestModule]::new($line)
            $currentTestClass = $null
            $currentTest = $null
            $lastTestCollection = $currentTestModule
        }
        elseif ($lineType -eq [LineType]::TestClass)
        {
            if ($currentTest -ne $null -and $currentTestClass -ne $null)
            {
                $currentTestClass.Tests.Add($currentTest)
            }

            if ($currentTestClass -ne $null -and $currentTestModule -ne $null)
            {
                $currentTestModule.TestClasses.Add($currentTestClass)
            }

            $currentTestClass = [TestClass]::new($line)

            $currentTest = $null
            $lastTestCollection = $currentTestClass
        }
        elseif ($lineType -eq [LineType]::Test)
        {
            if ($currentTest -ne $null -and $currentTestClass -ne $null)
            {
                $currentTestClass.Tests.Add($currentTest)
            }

            $currentTest = [Test]::new($line)
            $lastTestCollection = $currentTest
        }
        elseif ($lineType -eq [LineType]::Setup)
        {
            if ($lastTestCollection -ne $null)
            {
                $lastTestCollection.SetupMethodName = $line.Replace($setupBeginning, "")
            }
        }
        elseif ($lineType -eq [LineType]::Teardown)
        {
            if ($lastTestCollection -ne $null)
            {
                $lastTestCollection.TeardownMethodName = $line.Replace($teardownBeginning, "")
            }
        }
        elseif ($lineType -eq [LineType]::Property)
        {
            if ($lastTestCollection -ne $null)
            {
                foreach ($match in [Regex]::Matches($line, "Property\[(.*)\]\s+=\s+(.*)"))
                {
                    [string]$propertyKey = $match.Groups[1].Value;
                    [string]$propertyValue = $match.Groups[2].Value;
                    $lastTestCollection.Properties.Add($propertyKey, $propertyValue);
                }
            }
        }
    }

    if ($currentTest -ne $null -and $currentTestClass -ne $null)
    {
        $currentTestClass.Tests.Add($currentTest)
    }

    if ($currentTestClass -ne $null -and $currentTestModule -ne $null)
    {
        $currentTestModule.TestClasses.Add($currentTestClass)
    }

    if ($currentTestModule -ne $null)
    {
        $testModules.Add($currentTestModule)
    }

    return $testModules
}

Add-Type -AssemblyName System.Web

function Combine-TaefQuery([string]$baseQuery, [string]$newClause)
{
    if ($baseQuery)
    {
        return "$baseQuery AND $newClause"
    }
    else
    {
        return $newClause
    }
}

function Get-WorkItemSelection
{
    param(
        [Parameter(Mandatory)]
        [string]$WorkItemName,

        [AllowEmptyString()]
        [AllowNull()]
        [string]$BaseQuery,

        [Parameter(Mandatory)]
        [AllowEmptyCollection()]
        [object[]]$Tests
    )

    $testNames = @(
        $Tests |
            ForEach-Object { [string]$_.Name } |
            Sort-Object -Unique
    )

    if ($null -eq $ExcludedTestsByWorkItem -or
        -not $ExcludedTestsByWorkItem.ContainsKey($WorkItemName))
    {
        return [pscustomobject]@{
            Include = $true
            Query = $BaseQuery
        }
    }

    $excludedTestNames = @(
        $ExcludedTestsByWorkItem[$WorkItemName] |
            ForEach-Object { [string]$_ } |
            Sort-Object -Unique
    )
    foreach ($excludedTestName in $excludedTestNames)
    {
        if ($excludedTestName.Contains("'"))
        {
            throw (
                "Excluded test '$excludedTestName' in work item '$WorkItemName' " +
                "cannot be represented in a TAEF query.")
        }
    }

    $applicableExcludedTestNames = @(
        $excludedTestNames |
            Where-Object { $testNames -ccontains $_ }
    )
    $missingTestNames = @(
        $excludedTestNames |
            Where-Object { $testNames -cnotcontains $_ }
    )
    if ($missingTestNames.Count -ne 0 -and
        -not $AllowMissingExcludedTests.IsPresent)
    {
        throw (
            "Work item '$WorkItemName' does not contain excluded test(s): " +
            "$($missingTestNames -join ', ').")
    }
    if ($missingTestNames.Count -ne 0)
    {
        Write-Host (
            "Work item '$WorkItemName' does not select $($missingTestNames.Count) " +
            "excluded test(s) under the current base query; they will not be applied: " +
            "$($missingTestNames -join ', ').")
    }

    foreach ($excludedTestName in $applicableExcludedTestNames)
    {
        [void]$script:matchedExcludedTests.Add(
            "$WorkItemName`t$excludedTestName")
    }

    $remainingTestNames = @(
        $testNames |
            Where-Object { $applicableExcludedTestNames -cnotcontains $_ }
    )
    if ($remainingTestNames.Count -eq 0)
    {
        return [pscustomobject]@{
            Include = $false
            Query = $BaseQuery
        }
    }
    if ($applicableExcludedTestNames.Count -eq 0)
    {
        return [pscustomobject]@{
            Include = $true
            Query = $BaseQuery
        }
    }

    $exclusionClause = (
        'NOT(' +
        (@(
            $applicableExcludedTestNames |
                ForEach-Object { "@Name='$_'" }
        ) -join ' OR ') +
        ')')
    $negativeQuery = Combine-TaefQuery $BaseQuery $exclusionClause
    $selectedQuery = $negativeQuery

    $unrepresentableRemainingTestNames = @(
        $remainingTestNames |
            Where-Object { $_.Contains("'") }
    )
    if ($unrepresentableRemainingTestNames.Count -eq 0)
    {
        $inclusionClause = (
            '(' +
            (@(
                $remainingTestNames |
                    ForEach-Object { "@Name='$_'" }
            ) -join ' OR ') +
            ')')
        $positiveQuery = Combine-TaefQuery $BaseQuery $inclusionClause
        if ($positiveQuery.Length -lt $negativeQuery.Length)
        {
            $selectedQuery = $positiveQuery
        }
    }

    return [pscustomobject]@{
        Include = $true
        Query = $selectedQuery
    }
}


$testDllSubDir = [System.IO.Path]::GetDirectoryName($testFilePattern)

$taefDiscoverySelectStatement = "/select:`"$TaefBaseQuery`""
$TestFilePathPattern = "$($TestBinaryDirectoryPath)\$TestFilePattern"

$runIgnoredTestsStr = if ($RunIgnoredTests) { "/runIgnoredTests" } else { "" }
$taefParameters = "/taefParameters $TaefExtraParameters $runIgnoredTestsStr"

Write-Host "Finding te.exe"
$taefExePathToUse = "$TestBinaryDirectoryPath\te.exe"
if($TaefExePath)
{
    Write-Host "Checking for te.exe at path '$TaefExePath'"
    if(Test-Path $TaefExePath)
    {
        Write-Host "Te.exe found at '$TaefExePath'"
        $taefExePathToUse = $TaefExePath
    }
    else
    {
        Write-Host "Te.exe not found at '$TaefExePath'"
    }
}
Write-Host "Using te.exe from $taefExePathToUse"


$taefListPropertiesCommand = "$taefExePathToUse /listproperties $runIgnoredTestsStr $taefDiscoverySelectStatement $TestFilePathPattern".Replace("`"", "`"`"`"")

Write-Host "Running TAEF command: $taefListPropertiesCommand"
[string]$taefOutput = Invoke-Expression "$taefListPropertiesCommand" | Out-String
$taefExitCode = $LASTEXITCODE
if ($FailOnTaefDiscoveryError.IsPresent -and $taefExitCode -ne 0)
{
    throw "TAEF /listproperties failed with exit code $taefExitCode for '$TestFilePathPattern'."
}

[System.Collections.Generic.List[TestModule]]$testModules = (Parse-TestInfo $taefOutput)

$script:matchedExcludedTests = [System.Collections.Generic.HashSet[string]]::new(
    [StringComparer]::Ordinal)
$script:generatedWorkItemCount = 0
$script:skippedWorkItemCount = 0


$global:projFileContent = @"
<Project>
  <ItemGroup>
"@

function Add-HelixWorkItemToOutput
{
    Param ([string]$workItemName, [string]$commandArgs)

    if ($TestExecutionMultiplier -eq 1)
    {
        Add-HelixWorkItemToOutputImpl $workItemName $commandArgs
    }
    else
    {
        for($i = 0; $i -lt $TestExecutionMultiplier; $i++)
        {
            Add-HelixWorkItemToOutputImpl "$workItemName-$i" $commandArgs
        }
    }
}

function Add-HelixWorkItemToOutputImpl
{
    Param ([string]$workItemName, [string]$commandArgs)

    $script:generatedWorkItemCount++
    $global:projFileContent = $global:projFileContent + @"

    <HelixWorkItem Include="$($workItemName)" >
        <Timeout>$TestTimeout</Timeout>
        <Command>call %HELIX_CORRELATION_PAYLOAD%\RunHelixWorkItem.cmd $commandArgs</Command>
        <PreCommands>set testnameprefix=$($TestNamePrefix)</PreCommands>
    </HelixWorkItem>
"@
}

foreach ($testModule in $testModules)
{
    $testFileName = [System.IO.Path]::GetFileName($testModule.Name)
    if ($testDllSubDir)
    {
        $testFileName = Join-Path $testDllSubDir $testFileName
    }

    # We can create a WorkItem per module or per test class
    # We default to per-module
    # If a module or any test class in the module defines HelixWorkItemCreation="CreateWorkItemPerTestClass" we 
    # switch to per-testclass

    $CreateWorkItemPerModule = $true
    if ($testModule.Properties["HelixWorkItemCreation"] -eq "CreateWorkItemPerTestClass")
    {
        $CreateWorkItemPerModule = $false
    }
    foreach ($testClass in $testModule.TestClasses)
    {
        if($testClass.Properties["HelixWorkItemCreation"] -eq "CreateWorkItemPerTestClass")
        {
            $CreateWorkItemPerModule = $false
        }
    }

    if ($CreateWorkItemPerModule)
    {
        $workItemName = [System.IO.Path]::GetFileNameWithoutExtension($testModule.Name)
        
        # Remove redundant prefix from work item name:
        $workItemName = $workItemName -replace "Microsoft.UI.Xaml.Tests."

        if($WorkItemPrefix)
        {
            $workItemName = "$WorkItemPrefix-$workItemName"
        }

        $workItemTests = @(
            $testModule.TestClasses |
                ForEach-Object { $_.Tests }
        )
        $selection = Get-WorkItemSelection `
            -WorkItemName $workItemName `
            -BaseQuery $TaefBaseQuery `
            -Tests $workItemTests
        if ($selection.Include)
        {
            $taefSelectStatement = "/select:`"$($selection.Query)`""
            $commandArgs = [System.Web.HttpUtility]::HtmlEncode(
                "$testFileName /taefQuery $taefSelectStatement $taefParameters")
            Add-HelixWorkItemToOutput $workItemName $commandArgs
        }
        else
        {
            $script:skippedWorkItemCount++
            Write-Host (
                "Skipping Helix work item '$workItemName' because every discovered " +
                "test is excluded.")
        }
    }
    else
    {
        foreach ($testClass in $testModule.TestClasses)
        {
            Write-Host "Generating Helix work item for test class $($testClass.Name)..."
            [System.Collections.Generic.List[string]]$testSuiteNames = @()
            
            $testSuiteExists = $false
            $suitelessTestExists = $false

            foreach ($test in $testClass.Tests)
            {
                # A test method inherits its 'TestSuite' property from its TestClass
                if (!$test.Properties.ContainsKey("TestSuite") -and $testClass.Properties.ContainsKey("TestSuite"))
                {
                    $test.Properties["TestSuite"] = $testClass.Properties["TestSuite"]
                }

                if ($test.Properties.ContainsKey("TestSuite"))
                {
                    [string]$testSuite = $test.Properties["TestSuite"]

                    if (-not $testSuiteNames.Contains($testSuite))
                    {
                        Write-Host "    Found test suite $testSuite. Generating Helix work item for it as well."
                        $testSuiteNames.Add($testSuite)
                    }

                    $testSuiteExists = $true
                }
                else
                {
                    $suitelessTestExists = $true
                }
            }
            
            $testClassSelectPattern = "$($testClass.Name).*"
            if($testClass.Name.Contains("::"))
            {
                $testClassSelectPattern = "$($testClass.Name)::*"
            }
            $testNameQuery= "(@Name='$testClassSelectPattern')"

            $workItemName = $testClass.Name
            # Native tests use '::' as a separator, which is not valid for workItem names.
            $workItemName = $workItemName -replace "::", "-"
            # Remove redundant prefix from work item name:
            $workItemName = $workItemName -replace "Microsoft.UI.Xaml.Tests."
            $workItemName = $workItemName -replace "Microsoft-UI-Xaml-Tests-"

            if ($WorkItemPrefix)
            {
                $workItemName = "$WorkItemPrefix-$workItemName"
            }

            if ($suitelessTestExists)
            {
                $taefSuitelessFullQuery  = (Combine-TaefQuery $TaefBaseQuery $testNameQuery)
                if ($testSuiteExists) 
                { 
                    $taefSuitelessFullQuery = (Combine-TaefQuery $taefSuitelessFullQuery "NOT @TestSuite='*'" )
                }

                $suitelessTests = @(
                    $testClass.Tests |
                        Where-Object {
                            -not $_.Properties.ContainsKey("TestSuite")
                        }
                )
                $selection = Get-WorkItemSelection `
                    -WorkItemName $workItemName `
                    -BaseQuery $taefSuitelessFullQuery `
                    -Tests $suitelessTests
                if ($selection.Include)
                {
                    $commandArgs = [System.Web.HttpUtility]::HtmlEncode(
                        "$testFileName /taefQuery /select:`"$($selection.Query)`" $taefParameters")
                    Add-HelixWorkItemToOutput $workItemName $commandArgs
                }
                else
                {
                    $script:skippedWorkItemCount++
                    Write-Host (
                        "Skipping Helix work item '$workItemName' because every " +
                        "discovered test is excluded.")
                }
            }
            
            foreach ($testSuiteName in $testSuiteNames)
            {
                $taefSuiteQuery = (Combine-TaefQuery $TaefBaseQuery "$testNameQuery AND @TestSuite='$testSuiteName'")

                $suiteWorkItemName = "$($workItemName)-$testSuiteName"
                $suiteTests = @(
                    $testClass.Tests |
                        Where-Object {
                            $_.Properties.ContainsKey("TestSuite") -and
                            $_.Properties["TestSuite"] -eq $testSuiteName
                        }
                )
                $selection = Get-WorkItemSelection `
                    -WorkItemName $suiteWorkItemName `
                    -BaseQuery $taefSuiteQuery `
                    -Tests $suiteTests
                if ($selection.Include)
                {
                    $commandArgs = [System.Web.HttpUtility]::HtmlEncode(
                        "$testFileName /taefQuery /select:`"$($selection.Query)`" $taefParameters")
                    Add-HelixWorkItemToOutput $suiteWorkItemName $commandArgs
                }
                else
                {
                    $script:skippedWorkItemCount++
                    Write-Host (
                        "Skipping Helix work item '$suiteWorkItemName' because every " +
                        "discovered test is excluded.")
                }
            }
        }
    }
}

$global:projFileContent += @"

  </ItemGroup>
</Project>
"@

$outputDir = Split-Path -Parent $OutputProjFile

if (-not (Test-Path $outputDir))
{
    Write-Host "Creating dir $outputDir"
    $ignore = New-Item -ItemType Directory $outputDir
}

Set-Content $OutputProjFile $global:projFileContent -NoNewline -Encoding UTF8

if ($script:generatedWorkItemCount -eq 0)
{
    Write-Host "##[warning]No test work items were generated. All tests may have been filtered out by the TAEF query (e.g. MinOSVer > TargetOSVersion=$TargetOSVersion)."
}
else
{
    Write-Host "Generated $($script:generatedWorkItemCount) work item(s) in $OutputProjFile"
}

if ($null -ne $ExcludedTestsByWorkItem)
{
    [pscustomobject]@{
        MatchedExcludedTests = @(
            $script:matchedExcludedTests |
                Sort-Object
        )
        GeneratedWorkItemCount = $script:generatedWorkItemCount
        SkippedWorkItemCount = $script:skippedWorkItemCount
    }
}