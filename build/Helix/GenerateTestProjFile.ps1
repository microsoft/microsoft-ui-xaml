Param(
    [Parameter(Mandatory = $true)] 
    [string]$TestFile,

    [Parameter(Mandatory = $true)] 
    [string]$OutputProjFile,

    [Parameter(Mandatory = $true)] 
    [string]$TestSuiteName,

    [Parameter(Mandatory = $true)] 
    [string]$TaefPath
)

Add-Type -Language CSharp -ReferencedAssemblies System.Xml,System.Xaml @"
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;
using System.Xml;
using System.Xaml;

namespace TestProjFileGeneration
{
    public class TestCollection
    {
        public string Name { get; set; }
        public string SetupMethodName { get; set; }
        public string TeardownMethodName { get; set; }

        public IDictionary<string, string> Properties { get; private set; }

        public TestCollection(string name)
        {
            Name = name;
            Properties = new Dictionary<string, string>();
        }
    }

    public class TestModule : TestCollection
    {
        public IList<TestProjFileGeneration.TestClass> TestClasses { get; private set; }

        public TestModule(string name) : base(name)
        {
            TestClasses = new List<TestProjFileGeneration.TestClass>();
        }
    }

    public class TestClass : TestCollection
    {
        public IList<TestProjFileGeneration.Test> Tests { get; private set; }

        public TestClass(string name) : base(name)
        {
            Tests = new List<TestProjFileGeneration.Test>();
        }
    }

    public class Test : TestCollection
    {
        public Test(string name) : base(name) { }
    }

    public static class TestInfoParser
    {
        private enum LineType
        {
            None,
            TestModule,
            TestClass,
            Test,
            Setup,
            Teardown,
            Property,
        }

        public static List<TestProjFileGeneration.TestModule> Parse(string taefOutput)
        {
            string[] lines = taefOutput.Split(new string[] { Environment.NewLine, "\n" }, StringSplitOptions.RemoveEmptyEntries);
            List<TestProjFileGeneration.TestModule> testModules = new List<TestProjFileGeneration.TestModule>();

            TestProjFileGeneration.TestModule currentTestModule = null;
            TestProjFileGeneration.TestClass currentTestClass = null;
            TestProjFileGeneration.Test currentTest = null;

            TestProjFileGeneration.TestCollection lastTestCollection = null;
            
            foreach (string rawLine in lines)
            {
                LineType lineType = GetLineType(rawLine);

                // We don't need the whitespace around the line anymore, so we'll discard it to make things easier.
                string line = rawLine.Trim();

                switch (lineType)
                {
                case LineType.TestModule:
                    if (currentTest != null && currentTestClass != null)
                    {
                        currentTestClass.Tests.Add(currentTest);
                    }

                    if (currentTestClass != null && currentTestModule != null)
                    {
                        currentTestModule.TestClasses.Add(currentTestClass);
                    }

                    if (currentTestModule != null)
                    {
                        testModules.Add(currentTestModule);
                    }

                    currentTestModule = new TestProjFileGeneration.TestModule(line);
                    currentTestClass = null;
                    currentTest = null;
                    lastTestCollection = currentTestModule;
                    break;

                case LineType.TestClass:
                    if (currentTest != null && currentTestClass != null)
                    {
                        currentTestClass.Tests.Add(currentTest);
                    }

                    if (currentTestClass != null && currentTestModule != null)
                    {
                        currentTestModule.TestClasses.Add(currentTestClass);
                    }

                    currentTestClass = new TestProjFileGeneration.TestClass(line);
                    currentTest = null;
                    lastTestCollection = currentTestClass;
                    break;

                case LineType.Test:
                    if (currentTest != null && currentTestClass != null)
                    {
                        currentTestClass.Tests.Add(currentTest);
                    }

                    currentTest = new TestProjFileGeneration.Test(line);
                    lastTestCollection = currentTest;
                    break;

                case LineType.Setup:
                    if (lastTestCollection != null)
                    {
                        lastTestCollection.SetupMethodName = line.Replace(setupBeginning, "");
                    }
                    break;

                case LineType.Teardown:
                    if (lastTestCollection != null)
                    {
                        lastTestCollection.TeardownMethodName = line.Replace(teardownBeginning, "");
                    }
                    break;

                case LineType.Property:
                    if (lastTestCollection != null)
                    {
                        foreach (Match match in Regex.Matches(line, "Property\\[(.*)\\]\\s+=\\s+(.*)"))
                        {
                            string propertyKey = match.Groups[1].Value;
                            string propertyValue = match.Groups[2].Value;
                            lastTestCollection.Properties.Add(propertyKey, propertyValue);
                        }
                    }
                    break;
                }
            }

            if (currentTest != null && currentTestClass != null)
            {
                currentTestClass.Tests.Add(currentTest);
            }

            if (currentTestClass != null && currentTestModule != null)
            {
                currentTestModule.TestClasses.Add(currentTestClass);
            }

            if (currentTestModule != null)
            {
                testModules.Add(currentTestModule);
            }

            return testModules;
        }
        
        private static readonly string testModuleIndentation = "        ";
        private static readonly string testClassIndentation = "            ";
        private static readonly string setupBeginning = "Setup: ";
        private static readonly string teardownBeginning = "Teardown: ";
        private static readonly string propertyBeginning = "Property[";

        private static LineType GetLineType(string line)
        {
            if (line.Contains(setupBeginning))
            {
                return LineType.Setup;
            }
            else if (line.Contains(teardownBeginning))
            {
                return LineType.Teardown;
            }
            else if (line.Contains(propertyBeginning))
            {
                return LineType.Property;
            }
            else if (line.StartsWith(testModuleIndentation) && !line.StartsWith(testModuleIndentation + " "))
            {
                return LineType.TestModule;
            }
            else if (line.StartsWith(testClassIndentation) && !line.StartsWith(testClassIndentation + " "))
            {
                return LineType.TestClass;
            }
            else
            {
                return LineType.Test;
            }
        }
    }
}
"@

$taefExe = "$TaefPath\te.exe"
[string]$taefOutput = & "$taefExe" /listproperties $TestFile | Out-String

[System.Collections.Generic.IList`1[TestProjFileGeneration.TestModule]]$testModules = [TestProjFileGeneration.TestInfoParser]::Parse($taefOutput)

$projFileContent = @"
<Project>
  <ItemGroup>
"@

$testGroupSize = 10

foreach ($testModule in $testModules)
{
    Write-Host $testModule.Name

    foreach ($testClass in $testModules.TestClasses)
    {
        if ($testClass.Tests.Count -gt $testGroupSize)
        {
            [System.Collections.Generic.List[string]]$tests = @()
            $helixWorkItemCount = 0
        
            foreach ($test in $testClass.Tests)
            {
                if ($tests.Count -eq $testGroupSize)
                {
                    $helixWorkItemCount++
                    $projFileContent += @"

    <HelixWorkItem Include="$($testClass.Name)$helixWorkItemCount" Condition="'`$(TestSuite)'=='$($TestSuiteName)'">
        <Timeout>00:20:00</Timeout>
        <Command>call %HELIX_CORRELATION_PAYLOAD%\runtests.cmd /select:"(@Name='$($tests -join "' or @Name='")')"</Command>
    </HelixWorkItem>
"@
                    $tests.Clear()
                }

                $tests.Add($test.Name)
            }

            if ($tests.Count -gt 0)
            {
                $helixWorkItemCount++

                $projFileContent += @"

    <HelixWorkItem Include="$($testClass.Name)$helixWorkItemCount" Condition="'`$(TestSuite)'=='$($TestSuiteName)'">
        <Timeout>00:20:00</Timeout>
        <Command>call %HELIX_CORRELATION_PAYLOAD%\runtests.cmd /select:"(@Name='$($tests -join "' or @Name='")')"</Command>
    </HelixWorkItem>
"@
            }
        }
        else
        {
            $projFileContent += @"

    <HelixWorkItem Include="$($testClass.Name)" Condition="'`$(TestSuite)'=='$($TestSuiteName)'">
      <Timeout>00:20:00</Timeout>
      <Command>call %HELIX_CORRELATION_PAYLOAD%\runtests.cmd /name:$($testClass.Name).*</Command>
    </HelixWorkItem>
"@
        }
    }
}

$projFileContent += @"

  </ItemGroup>
</Project>
"@

Set-Content $OutputProjFile $projFileContent -NoNewline -Encoding UTF8