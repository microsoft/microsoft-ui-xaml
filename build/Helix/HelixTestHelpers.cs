using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Text;
using System.Xml.Linq;

namespace HelixTestHelpers
{
    public class TestResult
    {
        public TestResult()
        {
            Screenshots = new List<string>();
            RerunResults = new List<TestResult>();
        }

        public string Name { get; set; }
        public string SourceWttFile { get; set; }
        public bool Passed { get; set; }
        public bool PassedOnRerun { get; set; }
        public bool CleanupPassed { get; set; }
        public TimeSpan ExecutionTime { get; set; }
        public string Details { get; set; }

        public List<string> Screenshots { get; private set; }
        public List<TestResult> RerunResults { get; private set; }
    }
    
    [DataContract]  
    internal class JsonSerializableTestResult  
    {  
        [DataMember]
        internal string outcome;

        [DataMember]
        internal int durationInMs;
        
        [DataMember]
        internal string log;
        
        [DataMember]
        internal string[] screenshots;
        
        [DataMember]
        internal string errorMessage;
    }
    
    public class TestPass
    {
        public TimeSpan TestPassExecutionTime { get; set; }
        public List<TestResult> TestResults { get; set; }
        
        public static TestPass ParseTestWttFile(string fileName, bool cleanupFailuresAreRegressions, bool truncateTestNames)
        {
            using (var stream = File.OpenRead(fileName))
            {
                var doc = XDocument.Load(stream);
                var testResults = new List<TestResult>();
                var testExecutionTimeMap = new Dictionary<string, List<double>>();

                TestResult currentResult = null;
                long frequency = 0;
                long startTime = 0;
                long stopTime = 0;
                bool inTestCleanup = false;

                bool shouldLogToTestDetails = false;

                long testPassStartTime = 0;
                long testPassStopTime = 0;

                Func<XElement, bool> isScopeData = (elt) =>
                {
                    return
                        elt.Element("Data") != null &&
                        elt.Element("Data").Element("WexContext") != null &&
                        (
                            elt.Element("Data").Element("WexContext").Value == "Cleanup" ||
                            elt.Element("Data").Element("WexContext").Value == "TestScope" ||
                            elt.Element("Data").Element("WexContext").Value == "TestScope" ||
                            elt.Element("Data").Element("WexContext").Value == "ClassScope" ||
                            elt.Element("Data").Element("WexContext").Value == "ModuleScope"
                        );
                };

                Func<XElement, bool> isModuleOrClassScopeStart = (elt) =>
                {
                    return
                        elt.Name == "Msg" &&
                        elt.Element("Data") != null &&
                        elt.Element("Data").Element("StartGroup") != null &&
                        elt.Element("Data").Element("WexContext") != null &&
                            (elt.Element("Data").Element("WexContext").Value == "ClassScope" ||
                            elt.Element("Data").Element("WexContext").Value == "ModuleScope");
                };

                Func<XElement, bool> isModuleScopeEnd = (elt) =>
                {
                    return
                        elt.Name == "Msg" &&
                        elt.Element("Data") != null &&
                        elt.Element("Data").Element("EndGroup") != null &&
                        elt.Element("Data").Element("WexContext") != null &&
                        elt.Element("Data").Element("WexContext").Value == "ModuleScope";
                };

                Func<XElement, bool> isClassScopeEnd = (elt) =>
                {
                    return
                        elt.Name == "Msg" &&
                        elt.Element("Data") != null &&
                        elt.Element("Data").Element("EndGroup") != null &&
                        elt.Element("Data").Element("WexContext") != null &&
                        elt.Element("Data").Element("WexContext").Value == "ClassScope";
                };

                int testsExecuting = 0;
                foreach (XElement element in doc.Root.Elements())
                {
                    // Capturing the frequency data to record accurate
                    // timing data.
                    if (element.Name == "RTI")
                    {
                        frequency = Int64.Parse(element.Attribute("Frequency").Value);
                    }

                    // It's possible for a test to launch another test. If that happens, we won't modify the
                    // current result. Instead, we'll continue operating like normal and expect that we get two
                    // EndTests nodes before our next StartTests. We'll check that we've actually got a stop time
                    // before creating a new result. This will result in the two results being squashed
                    // into one result of the outer test that ran the inner one.
                    if (element.Name == "StartTest")
                    {
                        testsExecuting++;
                        if (testsExecuting == 1)
                        {
                            string testName = element.Attribute("Title").Value;
                            
                            if (truncateTestNames)
                            {
                                const string xamlNativePrefix = "Windows::UI::Xaml::Tests::";
                                const string xamlManagedPrefix = "Windows.UI.Xaml.Tests.";
                                if (testName.StartsWith(xamlNativePrefix))
                                {
                                    testName = testName.Substring(xamlNativePrefix.Length);
                                }
                                else if (testName.StartsWith(xamlManagedPrefix))
                                {
                                    testName = testName.Substring(xamlManagedPrefix.Length);
                                }
                            }

                            currentResult = new TestResult() { Name = testName, SourceWttFile = fileName, Passed = true, CleanupPassed = true };
                            testResults.Add(currentResult);
                            startTime = Int64.Parse(element.Descendants("WexTraceInfo").First().Attribute("TimeStamp").Value);
                            inTestCleanup = false;
                            shouldLogToTestDetails = true;
                            stopTime = 0;
                        }
                    }
                    else if (currentResult != null && element.Name == "EndTest")
                    {
                        testsExecuting--;

                        // If any inner test fails, we'll still fail the outer
                        currentResult.Passed &= element.Attribute("Result").Value == "Pass";

                        // Only gather execution data if this is the outer test we ran initially
                        if (testsExecuting == 0)
                        {
                            stopTime = Int64.Parse(element.Descendants("WexTraceInfo").First().Attribute("TimeStamp").Value);
                            if (!testExecutionTimeMap.Keys.Contains(currentResult.Name))
                                testExecutionTimeMap[currentResult.Name] = new List<double>();
                            testExecutionTimeMap[currentResult.Name].Add((double)(stopTime - startTime) / frequency);
                            currentResult.ExecutionTime = TimeSpan.FromSeconds(testExecutionTimeMap[currentResult.Name].Average());

                            startTime = 0;
                            inTestCleanup = true;
                        }
                    }
                    else if (currentResult != null &&
                            (isModuleOrClassScopeStart(element) || isModuleScopeEnd(element) || isClassScopeEnd(element)))
                    {
                        shouldLogToTestDetails = false;
                        inTestCleanup = false;
                    }

                    // Log-appending methods.
                    if (currentResult != null && element.Name == "Error")
                    {
                        if (shouldLogToTestDetails)
                        {
                            currentResult.Details += "\r\n[Error]: " + element.Attribute("UserText").Value;
                            if (element.Attribute("File") != null && element.Attribute("File").Value != "")
                            {
                                currentResult.Details += (" [File " + element.Attribute("File").Value);
                                if (element.Attribute("Line") != null)
                                    currentResult.Details += " Line: " + element.Attribute("Line").Value;
                                currentResult.Details += "]";
                            }
                        }


                        // The test cleanup errors will often come after the test claimed to have
                        // 'passed'. We treat them as errors as well. 
                        if (inTestCleanup)
                        {
                            currentResult.CleanupPassed = false;
                            currentResult.Passed = false;
                            // In stress mode runs, this test will run n times before cleanup is run. If the cleanup
                            // fails, we want to fail every test.
                            if (cleanupFailuresAreRegressions)
                            {
                                foreach (var result in testResults.Where(res => res.Name == currentResult.Name))
                                {
                                    result.Passed = false;
                                    result.CleanupPassed = false;
                                }
                            }
                        }
                    }

                    if (currentResult != null && element.Name == "Warn")
                    {
                        if (shouldLogToTestDetails)
                        {
                            currentResult.Details += "\r\n[Warn]: " + element.Attribute("UserText").Value;
                        }

                        if (element.Attribute("File") != null && element.Attribute("File").Value != "")
                        {
                            currentResult.Details += (" [File " + element.Attribute("File").Value);
                            if (element.Attribute("Line") != null)
                                currentResult.Details += " Line: " + element.Attribute("Line").Value;
                            currentResult.Details += "]";
                        }
                    }

                    if (currentResult != null && element.Name == "Msg")
                    {
                        var dataElement = element.Element("Data");
                        if (dataElement != null)
                        {
                            var supportingInfo = dataElement.Element("SupportingInfo");
                            if (supportingInfo != null)
                            {
                                var screenshots = supportingInfo.Elements("Item")
                                    .Where(item => GetAttributeValue(item, "Name") == "Screenshot")
                                    .Select(item => GetAttributeValue(item, "Value"));

                                foreach(var screenshot in screenshots)
                                {
                                    string fileNameSuffix = string.Empty;
                                    
                                    if (fileName.Contains("_rerun_multiple"))
                                    {
                                        fileNameSuffix = "_rerun_multiple";
                                    }
                                    else if (fileName.Contains("_rerun"))
                                    {
                                        fileNameSuffix = "_rerun";
                                    }
                                    
                                    currentResult.Screenshots.Add(screenshot.Replace(".jpg", fileNameSuffix + ".jpg"));
                                }
                            }
                        }
                    }
                }

                testPassStartTime = Int64.Parse(doc.Root.Descendants("WexTraceInfo").First().Attribute("TimeStamp").Value);
                testPassStopTime = Int64.Parse(doc.Root.Descendants("WexTraceInfo").Last().Attribute("TimeStamp").Value);

                var testPassTime = TimeSpan.FromSeconds((double)(testPassStopTime - testPassStartTime) / frequency);

                var testpass = new TestPass
                {
                    TestPassExecutionTime = testPassTime,
                    TestResults = testResults
                };

                return testpass;
            }
        }
        
        public static TestPass ParseTestWttFileWithReruns(string fileName, string singleRerunFileName, string multipleRerunFileName, bool cleanupFailuresAreRegressions, bool truncateTestNames)
        {
            TestPass testPass = ParseTestWttFile(fileName, cleanupFailuresAreRegressions, truncateTestNames);
            TestPass singleRerunTestPass = File.Exists(singleRerunFileName) ? ParseTestWttFile(singleRerunFileName, cleanupFailuresAreRegressions, truncateTestNames) : null;
            TestPass multipleRerunTestPass = File.Exists(multipleRerunFileName) ? ParseTestWttFile(multipleRerunFileName, cleanupFailuresAreRegressions, truncateTestNames) : null;
            
            List<TestResult> rerunTestResults = new List<TestResult>();

            if (singleRerunTestPass != null)
            {
                rerunTestResults.AddRange(singleRerunTestPass.TestResults);
            }

            if (multipleRerunTestPass != null)
            {
                rerunTestResults.AddRange(multipleRerunTestPass.TestResults);
            }

            // For each failed test result, we'll check to see whether the test passed at least once upon rerun.
            // If so, we'll set PassedOnRerun to true to flag the fact that this is an unreliable test
            // rather than a genuine test failure.
            foreach (TestResult failedTestResult in testPass.TestResults.Where(r => !r.Passed))
            {
                failedTestResult.RerunResults.AddRange(rerunTestResults.Where(r => r.Name == failedTestResult.Name));
                
                if (rerunTestResults.Where(r => r.Name == failedTestResult.Name && r.Passed).Count() > 0)
                {
                    failedTestResult.PassedOnRerun = true;
                }
            }
            
            return testPass;
        }

        private static string GetAttributeValue(XElement element, string attributeName)
        {
            if(element.Attribute(attributeName) != null)
            {
                return element.Attribute(attributeName).Value;
            }

            return null;
        }
    }

    public static class FailedTestDetector
    {
        public static void OutputFailedTestQuery(string wttInputPath)
        {
            var testPass = TestPass.ParseTestWttFile(wttInputPath, cleanupFailuresAreRegressions: true, truncateTestNames: false);
            
            List<string> failedTestNames = new List<string>();
            
            foreach (var result in testPass.TestResults)
            {
                if (!result.Passed)
                {
                    failedTestNames.Add(result.Name);
                }
            }
            
            if (failedTestNames.Count > 0)
            {
                string failedTestSelectQuery = "(@Name='";
                
                for (int i = 0; i < failedTestNames.Count; i++)
                {
                    failedTestSelectQuery += failedTestNames[i];
                    
                    if (i < failedTestNames.Count - 1)
                    {
                        failedTestSelectQuery += "' or @Name='";
                    }
                }
                
                failedTestSelectQuery += "')";
            
                Console.WriteLine(failedTestSelectQuery);
            }
            else
            {
                Console.WriteLine("");
            }
        }
    }

    public class TestResultParser
    {
        private string testNamePrefix;
        private string helixResultsContainerUri;
        private string helixResultsContainerRsas;
    
        public TestResultParser(string testNamePrefix, string helixResultsContainerUri, string helixResultsContainerRsas)
        {
            this.testNamePrefix = testNamePrefix;
            this.helixResultsContainerUri = helixResultsContainerUri;
            this.helixResultsContainerRsas = helixResultsContainerRsas;
        }
        
        public void ConvertWttLogToXUnitLog(string wttInputPath, string wttSingleRerunInputPath, string wttMultipleRerunInputPath, string xunitOutputPath)
        {
            TestPass testPass = TestPass.ParseTestWttFileWithReruns(wttInputPath, wttSingleRerunInputPath, wttMultipleRerunInputPath, cleanupFailuresAreRegressions: true, truncateTestNames: false);
            var results = testPass.TestResults;

            int resultCount = results.Count;
            int passedCount = results.Where(r => r.Passed).Count();
            int passedOnRerunCount = results.Where(r => r.PassedOnRerun).Count();
            int failedCount = resultCount - passedCount;

            var root = new XElement("assemblies");

            var assembly = new XElement("assembly");
            assembly.SetAttributeValue("name", "MUXControls.Test.dll");
            assembly.SetAttributeValue("test-framework", "TAEF");
            assembly.SetAttributeValue("run-date", DateTime.Now.ToString("yyyy-mm-dd"));

            // This doesn't need to be completely accurate since it's not exposed anywhere.
            // If we need accurate an start time we can probably calculate it from the te.wtl file, but for
            // now this is fine.
            assembly.SetAttributeValue("run-time", (DateTime.Now - testPass.TestPassExecutionTime).ToString("hh:mm:ss"));
            
            assembly.SetAttributeValue("total", resultCount);
            assembly.SetAttributeValue("passed", passedCount);
            assembly.SetAttributeValue("failed", failedCount);
            
            // There's no way using the xUnit format to report that a test passed on re-run, so since we
            // aren't using the notion of a "skipped" test, we'll use that as a proxy.
            assembly.SetAttributeValue("skipped", passedOnRerunCount);
            
            assembly.SetAttributeValue("time", (int)testPass.TestPassExecutionTime.TotalSeconds);
            assembly.SetAttributeValue("errors", 0);
            root.Add(assembly);

            var collection = new XElement("collection");
            collection.SetAttributeValue("total", resultCount);
            collection.SetAttributeValue("passed", passedCount);
            collection.SetAttributeValue("failed", failedCount);
            collection.SetAttributeValue("skipped", passedOnRerunCount);
            collection.SetAttributeValue("name", "Test collection");
            collection.SetAttributeValue("time", (int)testPass.TestPassExecutionTime.TotalSeconds);
            assembly.Add(collection);

            foreach (var result in results)
            {
                var test = new XElement("test");
                test.SetAttributeValue("name", testNamePrefix + "." + result.Name);

                var className = result.Name.Substring(0, result.Name.LastIndexOf('.'));
                var methodName = result.Name.Substring(result.Name.LastIndexOf('.') + 1);
                test.SetAttributeValue("type", className);
                test.SetAttributeValue("method", methodName);

                test.SetAttributeValue("time", result.ExecutionTime.TotalSeconds);
                
                // TODO (https://github.com/dotnet/arcade/issues/2773): Once we're able to
                // report things in a more granular fashion than just a binary pass/fail result,
                // we should do that.  For now, we'll use "Skip" to mean "this test was unreliable".
                string resultString = string.Empty;
                
                if (result.Passed)
                {
                    resultString = "Pass";
                }
                else if (result.PassedOnRerun)
                {
                    resultString = "Skip";
                }
                else
                {
                    resultString = "Fail";
                }
                
                test.SetAttributeValue("result", resultString);

                if (!result.Passed)
                {
                    // If the test passed on rerun, then we'll add metadata noting as much.
                    // Otherwise, we'll mark down the failure information.
                    if (result.PassedOnRerun)
                    {
                        var reason = new XElement("reason");
                        List<JsonSerializableTestResult> serializableResults = new List<JsonSerializableTestResult>();
                        serializableResults.Add(ConvertToSerializableResult(result));
                        
                        foreach (TestResult rerunResult in result.RerunResults)
                        {
                            serializableResults.Add(ConvertToSerializableResult(rerunResult));
                        }
                        
                        MemoryStream stream = new MemoryStream();  
                        DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(JsonSerializableTestResult[]));
                        serializer.WriteObject(stream, serializableResults.ToArray());
                        stream.Position = 0;  
                        StreamReader streamReader = new StreamReader(stream);  
                        
                        reason.Add(new XCData(streamReader.ReadToEnd()));
                        test.Add(reason);
                    }
                    else
                    {
                        var failure = new XElement("failure");
                        failure.SetAttributeValue("exception-type", "Exception");

                        var message = new XElement("message");

                        StringBuilder errorMessage = new StringBuilder();

                        errorMessage.AppendLine("Log: " + GetUploadedFileUrl(result.SourceWttFile, helixResultsContainerUri, helixResultsContainerRsas));
                        errorMessage.AppendLine();
                        
                        if(result.Screenshots.Any())
                        {
                            errorMessage.AppendLine("Screenshots:");
                            foreach(var screenshot in result.Screenshots)
                            {
                                errorMessage.AppendLine(GetUploadedFileUrl(screenshot, helixResultsContainerUri, helixResultsContainerRsas));
                                errorMessage.AppendLine();
                            }
                        }

                        errorMessage.AppendLine("Error Log: ");
                        errorMessage.AppendLine(result.Details);

                        message.Add(new XCData(errorMessage.ToString()));
                        failure.Add(message);

                        test.Add(failure);
                    }
                }
                collection.Add(test);
            }

            File.WriteAllText(xunitOutputPath, root.ToString());
        }
        
        private JsonSerializableTestResult ConvertToSerializableResult(TestResult rerunResult)
        {
            var serializableResult = new JsonSerializableTestResult();
            
            serializableResult.outcome = rerunResult.Passed ? "Passed" : "Failed";
            serializableResult.durationInMs = (int)Math.Round(rerunResult.ExecutionTime.TotalMilliseconds);
            
            if (!rerunResult.Passed)
            {
                serializableResult.log = GetUploadedFileUrl(rerunResult.SourceWttFile, helixResultsContainerUri, helixResultsContainerRsas);
                
                if (rerunResult.Screenshots.Any())
                {
                    List<string> screenshots = new List<string>();
                    
                    foreach (var screenshot in rerunResult.Screenshots)
                    {
                        screenshots.Add(GetUploadedFileUrl(screenshot, helixResultsContainerUri, helixResultsContainerRsas));
                    }
                    
                    serializableResult.screenshots = screenshots.ToArray();
                }
                
                serializableResult.errorMessage = rerunResult.Details.Trim();
            }
            
            return serializableResult;
        }

        private string GetUploadedFileUrl(string filePath, string helixResultsContainerUri, string helixResultsContainerRsas)
        {
            var filename = Path.GetFileName(filePath);
            return string.Format("{0}/{1}{2}", helixResultsContainerUri, filename, helixResultsContainerRsas);
        }
    }
}
