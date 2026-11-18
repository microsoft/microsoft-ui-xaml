using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;
using System.Xml.Linq;

namespace HelixTestHelpers
{
    public class TestResult
    {
        public TestResult()
        {
            Screenshots = new List<string>();
            RerunResults = new List<TestResult>();
            Details = "";
            Skipped = false;
        }

        public string Name { get; set; }
        public string SourceWttFile { get; set; }
        public bool Passed { get; set; }
        public bool Skipped { get; set; }
        public bool CleanupPassed { get; set; }
        public TimeSpan ExecutionTime { get; set; }
        public string Details { get; private set; }

        public void LogToDetails(string str)
        {
            if(Details.Length < 1000)
            {
                Details += str;
            }
        }

        public List<string> Screenshots { get; private set; }
        public List<TestResult> RerunResults { get; private set; }

        public bool IsPassedOrSkipped()
        {
            return Passed || Skipped;
        }

        public bool IsFailed()
        {
            return !IsPassedOrSkipped();
        }

        public bool IsReportAsPassed()
        {
            return !Skipped && Passed;
        }

        public bool IsReportAsUnreliable(int requiredNumberOfPasses)
        {
            if(Skipped || IsReportAsPassed())
            {
                return false;
            }
            else if (RerunResults.Count == 1 && RerunResults[0].Passed)
            {
                return RerunResults[0].Passed;
            }
            else
            {
                return  RerunResults.Where(r => r.Passed).Count() >= requiredNumberOfPasses;
            }
        }

        public bool IsReportAsFailed(int requiredNumberOfPasses)
        {
            return !Skipped && !IsReportAsPassed() && !IsReportAsUnreliable(requiredNumberOfPasses);
        }
    }
    
    //
    // Azure DevOps doesn't currently provide a way to directly report sub-results for tests that failed at least once
    // that were run multiple times.  To get around that limitation, we'll mark the test as "Skip" since
    // that's the only non-pass/fail result we can return, and will then report the information about the
    // runs in a separate json file for the skipped test.  
    // This json file gets uploaded to Helix storage, and is downloaded by the Pipeline which uses it to update test results.
    // "Skipped" tests with a sufficiently high pass rate, get switched to 'Pass'. "Skipped" tests will a lower pass rate get 
    // updated to 'Fail'. See UpdateUnreliableTests.ps1.
    [DataContract]  
    internal class JsonSerializableTestResults
    {  
        [DataMember]
        internal string[] errors;
        
        [DataMember]
        internal JsonSerializableTestResult[] results;
    }
    
    [DataContract]  
    internal class JsonSerializableTestResult  
    {
        [DataMember]
        internal string outcome;

        [DataMember]
        internal int duration;
        
        [DataMember(EmitDefaultValue = false)]
        internal string log;
        
        [DataMember(EmitDefaultValue = false)]
        internal string[] screenshots;
        
        [DataMember(EmitDefaultValue = false)]
        internal int errorIndex;
    }
    
    public class TestPass
    {
        public TimeSpan TestPassExecutionTime { get; set; }
        public List<TestResult> TestResults { get; set; }
        
        public static TestPass ParseTestWttFile(string fileName, bool cleanupFailuresAreRegressions, bool truncateTestNames, bool ignoreErrorsInCleanup = false)
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

                        currentResult.Skipped = element.Attribute("Result").Value == "Skipped";

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
                            currentResult.LogToDetails("\r\n[Error]: " + element.Attribute("UserText").Value);
                            if (element.Attribute("File") != null && element.Attribute("File").Value != "")
                            {
                                currentResult.LogToDetails(" [File " + element.Attribute("File").Value);
                                if (element.Attribute("Line") != null)
                                    currentResult.LogToDetails(" Line: " + element.Attribute("Line").Value);
                                currentResult.LogToDetails("]");
                            }
                        }


                        // The test cleanup errors will often come after the test claimed to have
                        // 'passed'. We treat them as errors as well. 
                        if (inTestCleanup && !ignoreErrorsInCleanup)
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
                            currentResult.LogToDetails("\r\n[Warn]: " + element.Attribute("UserText").Value);
                        }

                        if (element.Attribute("File") != null && element.Attribute("File").Value != "")
                        {
                            currentResult.LogToDetails(" [File " + element.Attribute("File").Value);
                            if (element.Attribute("Line") != null)
                                currentResult.LogToDetails(" Line: " + element.Attribute("Line").Value);
                            currentResult.LogToDetails("]");
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
        
        public static TestPass ParseTestWttFileWithReruns(string fileName, string singleRerunFileName, string multipleRerunFileName, string moreRerunFileName, bool cleanupFailuresAreRegressions, bool truncateTestNames)
        {
            TestPass testPass = ParseTestWttFile(fileName, cleanupFailuresAreRegressions, truncateTestNames);
            TestPass singleRerunTestPass = File.Exists(singleRerunFileName) ? ParseTestWttFile(singleRerunFileName, cleanupFailuresAreRegressions, truncateTestNames) : null;
            TestPass multipleRerunTestPass = File.Exists(multipleRerunFileName) ? ParseTestWttFile(multipleRerunFileName, cleanupFailuresAreRegressions, truncateTestNames) : null;
            TestPass moreRerunTestPass = File.Exists(moreRerunFileName) ? ParseTestWttFile(moreRerunFileName, cleanupFailuresAreRegressions, truncateTestNames) : null;
            
            List<TestResult> rerunTestResults = new List<TestResult>();

            if (singleRerunTestPass != null)
            {
                rerunTestResults.AddRange(singleRerunTestPass.TestResults);
            }

            if (multipleRerunTestPass != null)
            {
                rerunTestResults.AddRange(multipleRerunTestPass.TestResults);
            }

            if (moreRerunTestPass != null)
            {
                rerunTestResults.AddRange(moreRerunTestPass.TestResults);
            }

            // For each failed test result, we'll check to see whether the test passed at least once upon rerun.
            // If so, we'll set PassedOnRerun to true to flag the fact that this is an unreliable test
            // rather than a genuine test failure.
            foreach (TestResult failedTestResult in testPass.TestResults.Where(r => r.IsFailed()))
            {
                failedTestResult.RerunResults.AddRange(rerunTestResults.Where(r => r.Name == failedTestResult.Name));
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
        public static string GetFailedTestQuery(string wttInputPath)
        {
            var testPass = TestPass.ParseTestWttFile(wttInputPath, cleanupFailuresAreRegressions: false, truncateTestNames: false);
            
            List<string> failedTestNames = new List<string>();
            
            foreach (var result in testPass.TestResults)
            {
                if (result.IsFailed())
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
            
                return failedTestSelectQuery;
            }
            else
            {
                return "";
            }
        }
    }

    public enum WttLogState
    {
        Complete,
        Recovered,
        Unrecoverable
    }

    public class WttLogRecoveryResult
    {
        public WttLogRecoveryResult()
        {
            InFlightTests = new List<string>();
        }

        public WttLogState State { get; set; }
        public bool LogFileExisted { get; set; }
        public string ReadError { get; set; }
        public int TestsFromLog { get; set; }
        public int TestsFromConsole { get; set; }
        public List<string> InFlightTests { get; private set; }
    }

    public class SelectionCheckResult
    {
        public SelectionCheckResult()
        {
            TestsWithoutResults = new List<string>();
            DataDrivenTestsCutShort = new List<string>();
        }

        // Selected tests that have no result in the log.
        public List<string> TestsWithoutResults { get; private set; }

        // Selected data-driven tests that te.exe was running, or had just run, when it exited. Their later variations
        // may not have run.
        public List<string> DataDrivenTestsCutShort { get; private set; }

        public bool AllSelectedTestsRan
        {
            get { return TestsWithoutResults.Count == 0 && DataDrivenTestsCutShort.Count == 0; }
        }
    }

    // te.exe buffers te.wtl, so when te.exe exits abnormally the log ends mid-element and can be missing results that
    // te.exe already printed to the console. RecoverIncompleteLog moves such a log to incompleteLogCopyPath and, if
    // anything can be recovered, writes a readable log in its place. The new log holds the complete entries of the old
    // one plus the console's "EndGroup: <test> [<result>]" results that never reached it. A test that started but
    // never ended is reported as failed.
    public static class WttLogRecovery
    {
        private const string DefaultRootName = "WTT-Logger";

        // Only used if te.exe exited before writing the RTI element that has the real timer frequency.
        private const string DefaultTimerFrequency = "10000000";

        private const string ConsoleResultNote =
            "te.exe exited before writing this result to te.wtl. The result comes from te.exe's console output, " +
            "which also has the failure details.";

        private const string InFlightTestNote =
            "te.exe exited while this test was running, so the test has no result. It's reported as failed.";

        private static readonly Regex ConsoleStartGroup = new Regex(@"^\s*StartGroup: (.+?)\s*$");
        private static readonly Regex ConsoleEndGroup = new Regex(@"^\s*EndGroup: (.+) \[([^\]]+)\]\s*$");

        public static WttLogRecoveryResult RecoverIncompleteLog(string wttPath, string consoleLogPath, string incompleteLogCopyPath)
        {
            if (string.IsNullOrEmpty(incompleteLogCopyPath))
            {
                throw new ArgumentNullException("incompleteLogCopyPath");
            }

            var result = new WttLogRecoveryResult();
            result.LogFileExisted = File.Exists(wttPath);

            if (result.LogFileExisted)
            {
                try
                {
                    using (FileStream stream = File.OpenRead(wttPath))
                    {
                        XDocument.Load(stream);
                    }

                    result.State = WttLogState.Complete;
                    return result;
                }
                catch (XmlException ex)
                {
                    result.ReadError = ex.Message;
                }
            }
            else
            {
                result.ReadError = "te.exe didn't create the file.";
            }

            XName rootName;
            List<XElement> logElements = ReadCompleteElements(wttPath, result.LogFileExisted, out rootName);

            // Count each test's ends in the log, and track the tests that started but never ended (outermost first).
            var endsInLog = new Dictionary<string, int>();
            var openTests = new List<string>();
            string lastTimeStamp = "0";
            bool hasTimerFrequency = false;
            foreach (XElement element in logElements)
            {
                if (element.Name == "RTI" && element.Attribute("Frequency") != null)
                {
                    hasTimerFrequency = true;
                }

                foreach (XElement traceInfo in element.Descendants("WexTraceInfo"))
                {
                    XAttribute timeStamp = traceInfo.Attribute("TimeStamp");
                    if (timeStamp != null)
                    {
                        lastTimeStamp = timeStamp.Value;
                    }
                }

                if (element.Name == "StartTest")
                {
                    openTests.Add(GetTitle(element));
                }
                else if (element.Name == "EndTest" && openTests.Count > 0)
                {
                    Increment(endsInLog, openTests[openTests.Count - 1]);
                    openTests.RemoveAt(openTests.Count - 1);
                    if (openTests.Count == 0)
                    {
                        result.TestsFromLog++;
                    }
                }
            }

            var consoleResults = new List<KeyValuePair<string, string>>();
            var consoleStarts = new Dictionary<string, int>();
            var testsStillRunningOnConsole = new List<string>();
            if (!string.IsNullOrEmpty(consoleLogPath) && File.Exists(consoleLogPath))
            {
                foreach (string line in File.ReadLines(consoleLogPath))
                {
                    Match endGroup = ConsoleEndGroup.Match(line);
                    if (endGroup.Success)
                    {
                        string test = endGroup.Groups[1].Value;
                        consoleResults.Add(new KeyValuePair<string, string>(test, endGroup.Groups[2].Value.Trim()));
                        int startIndex = testsStillRunningOnConsole.LastIndexOf(test);
                        if (startIndex >= 0)
                        {
                            testsStillRunningOnConsole.RemoveAt(startIndex);
                        }

                        continue;
                    }

                    Match startGroup = ConsoleStartGroup.Match(line);
                    if (startGroup.Success)
                    {
                        string test = startGroup.Groups[1].Value;
                        Increment(consoleStarts, test);
                        testsStillRunningOnConsole.Add(test);
                    }
                }
            }

            var recovered = new List<XElement>(logElements);
            if (!hasTimerFrequency)
            {
                recovered.Insert(0, new XElement("RTI", new XAttribute("Frequency", DefaultTimerFrequency)));
            }

            // A test's console results beyond the number of times it ended in the log never reached the log.
            var consoleResultsSeen = new Dictionary<string, int>();
            foreach (KeyValuePair<string, string> consoleResult in consoleResults)
            {
                string test = consoleResult.Key;
                int endCount;
                endsInLog.TryGetValue(test, out endCount);
                if (Increment(consoleResultsSeen, test) <= endCount)
                {
                    continue;
                }

                string wttResult = ToWttResult(consoleResult.Value);
                int openIndex = openTests.LastIndexOf(test);
                if (openIndex >= 0)
                {
                    // The log has this test's start but not its end.
                    if (IsFailure(wttResult))
                    {
                        recovered.Add(NewError(ConsoleResultNote, lastTimeStamp));
                    }

                    while (openTests.Count > openIndex)
                    {
                        recovered.Add(NewTestElement("EndTest", openTests[openTests.Count - 1], wttResult, lastTimeStamp));
                        openTests.RemoveAt(openTests.Count - 1);
                    }

                    if (openTests.Count == 0)
                    {
                        result.TestsFromConsole++;
                    }

                    continue;
                }

                EndInFlightTests(recovered, openTests, result, lastTimeStamp);
                recovered.Add(NewTestElement("StartTest", test, null, lastTimeStamp));
                if (IsFailure(wttResult))
                {
                    recovered.Add(NewError(ConsoleResultNote, lastTimeStamp));
                }

                recovered.Add(NewTestElement("EndTest", test, wttResult, lastTimeStamp));
                result.TestsFromConsole++;
            }

            EndInFlightTests(recovered, openTests, result, lastTimeStamp);

            // A test the console shows starting but not ending, unless the log has ended every run of it. That happens
            // when te.exe exits before its last console output is flushed.
            foreach (string test in testsStillRunningOnConsole.Distinct())
            {
                int endCount;
                endsInLog.TryGetValue(test, out endCount);
                if (endCount < consoleStarts[test] && !result.InFlightTests.Contains(test))
                {
                    recovered.Add(NewTestElement("StartTest", test, null, lastTimeStamp));
                    recovered.Add(NewError(InFlightTestNote, lastTimeStamp));
                    recovered.Add(NewTestElement("EndTest", test, "Fail", lastTimeStamp));
                    result.InFlightTests.Add(test);
                }
            }

            bool anythingRecovered = (result.TestsFromLog + result.TestsFromConsole + result.InFlightTests.Count) > 0;
            result.State = anythingRecovered ? WttLogState.Recovered : WttLogState.Unrecoverable;

            if (result.LogFileExisted)
            {
                File.Copy(wttPath, incompleteLogCopyPath, true);
                File.Delete(wttPath);
            }

            if (anythingRecovered)
            {
                WriteLog(wttPath, rootName, recovered);
            }

            return result;
        }

        // Checks a readable log against the tests "te.exe /list" selects, to tell whether a run that te.exe didn't
        // finish still ran every selected test. inFlightTests are the tests RecoverIncompleteLog reported as failed
        // because te.exe exited while they were running. /list names a data-driven test once, while the log has a
        // result for each of its variations, named "<test>#<variation>". TAEF runs a test's variations back to back,
        // so only a test te.exe was running, or had just run, when it exited can be missing variations.
        public static SelectionCheckResult CheckSelectedTestsRan(string wttPath, string[] selectedTests, string[] inFlightTests)
        {
            var results = new List<string>();
            using (FileStream stream = File.OpenRead(wttPath))
            {
                foreach (XElement endTest in XDocument.Load(stream).Descendants("EndTest"))
                {
                    results.Add(GetTitle(endTest));
                }
            }

            var testsWithResults = new HashSet<string>(StringComparer.Ordinal);
            foreach (string result in results)
            {
                testsWithResults.Add(result);
                string dataDrivenTest = GetDataDrivenTest(result);
                if (dataDrivenTest != null)
                {
                    testsWithResults.Add(dataDrivenTest);
                }
            }

            var check = new SelectionCheckResult();
            var selected = new HashSet<string>(selectedTests, StringComparer.Ordinal);
            foreach (string test in selectedTests.Distinct(StringComparer.Ordinal))
            {
                if (!testsWithResults.Contains(test))
                {
                    check.TestsWithoutResults.Add(test);
                }
            }

            var lastTests = new List<string>(inFlightTests ?? new string[0]);
            if (results.Count > 0)
            {
                lastTests.Add(results[results.Count - 1]);
            }

            foreach (string test in lastTests)
            {
                string dataDrivenTest = GetDataDrivenTest(test);
                if (dataDrivenTest != null && selected.Contains(dataDrivenTest) && !check.DataDrivenTestsCutShort.Contains(dataDrivenTest))
                {
                    check.DataDrivenTestsCutShort.Add(dataDrivenTest);
                }
            }

            return check;
        }

        // The data-driven test that a result named "<test>#<variation>" belongs to, or null for any other result.
        private static string GetDataDrivenTest(string result)
        {
            int variation = result.IndexOf('#');
            return variation > 0 ? result.Substring(0, variation) : null;
        }

        private static List<XElement> ReadCompleteElements(string wttPath, bool logFileExists, out XName rootName)
        {
            rootName = DefaultRootName;
            var elements = new List<XElement>();
            if (!logFileExists)
            {
                return elements;
            }

            try
            {
                using (FileStream stream = File.OpenRead(wttPath))
                using (XmlReader reader = XmlReader.Create(stream))
                {
                    reader.MoveToContent();
                    rootName = XName.Get(reader.LocalName, reader.NamespaceURI);
                    if (reader.IsEmptyElement)
                    {
                        return elements;
                    }

                    reader.Read();
                    while (!reader.EOF && reader.NodeType != XmlNodeType.EndElement)
                    {
                        if (reader.NodeType == XmlNodeType.Element)
                        {
                            elements.Add((XElement)XNode.ReadFrom(reader));
                        }
                        else
                        {
                            reader.Read();
                        }
                    }
                }
            }
            catch (XmlException)
            {
                // The log stops where te.exe stopped writing it; keep every element that was complete before that.
            }

            return elements;
        }

        private static void EndInFlightTests(List<XElement> recovered, List<string> openTests, WttLogRecoveryResult result, string timeStamp)
        {
            if (openTests.Count == 0)
            {
                return;
            }

            recovered.Add(NewError(InFlightTestNote, timeStamp));
            while (openTests.Count > 0)
            {
                string test = openTests[openTests.Count - 1];
                recovered.Add(NewTestElement("EndTest", test, "Fail", timeStamp));
                openTests.RemoveAt(openTests.Count - 1);
                result.InFlightTests.Add(test);
            }
        }

        private static void WriteLog(string wttPath, XName rootName, List<XElement> elements)
        {
            var root = new XElement(rootName);
            foreach (XElement element in elements)
            {
                root.Add(new XText("\n"), element);
            }

            root.Add(new XText("\n"));

            var settings = new XmlWriterSettings();
            settings.Encoding = new UnicodeEncoding(false, true);
            using (XmlWriter writer = XmlWriter.Create(wttPath, settings))
            {
                new XDocument(root).Save(writer);
            }
        }

        private static XElement NewTestElement(string elementName, string test, string wttResult, string timeStamp)
        {
            var element = new XElement(elementName, new XAttribute("Title", test));
            if (wttResult != null)
            {
                element.Add(new XAttribute("Result", wttResult));
            }

            element.Add(NewTraceData(timeStamp));
            return element;
        }

        private static XElement NewError(string message, string timeStamp)
        {
            return new XElement("Error", new XAttribute("UserText", message), NewTraceData(timeStamp));
        }

        private static XElement NewTraceData(string timeStamp)
        {
            return new XElement("Data", new XElement("WexTraceInfo", new XAttribute("TimeStamp", timeStamp)));
        }

        private static string GetTitle(XElement element)
        {
            XAttribute title = element.Attribute("Title");
            return title != null ? title.Value : string.Empty;
        }

        private static string ToWttResult(string consoleResult)
        {
            switch (consoleResult)
            {
                case "Passed":
                    return "Pass";
                case "Skipped":
                    return "Skipped";
                case "Blocked":
                    return "Blocked";
                default:
                    return "Fail";
            }
        }

        private static bool IsFailure(string wttResult)
        {
            return wttResult != "Pass" && wttResult != "Skipped";
        }

        private static int Increment(Dictionary<string, int> counts, string key)
        {
            int count;
            counts.TryGetValue(key, out count);
            count++;
            counts[key] = count;
            return count;
        }
    }

    public class TestResultParser
    {
        private string testNamePrefix;

        public TestResultParser(string testNamePrefix)
        {
            this.testNamePrefix = testNamePrefix;
        }

        public void ConvertWttLogToXUnitLog(string wttInputPath, string wttSingleRerunInputPath, string wttMultipleRerunInputPath, string wttMoreRerunInputPath,string xunitOutputPath, int requiredPassRateThreshold, bool useRetryLogic = true)
        {
            TestPass testPass = null;
            if (useRetryLogic)
            {
                testPass = TestPass.ParseTestWttFileWithReruns(wttInputPath, wttSingleRerunInputPath, wttMultipleRerunInputPath, wttMoreRerunInputPath, cleanupFailuresAreRegressions: false, truncateTestNames: false);
            }
            else
            {
                if (!string.IsNullOrEmpty(wttMultipleRerunInputPath) || !string.IsNullOrEmpty(wttMultipleRerunInputPath))
                {
                    throw new InvalidOperationException("wttMultipleRerunInputPath and wttMultipleRerunInputPath should only be used with useRetryLogic=true.");
                }
                testPass = TestPass.ParseTestWttFile(wttInputPath, cleanupFailuresAreRegressions: false, truncateTestNames: false);
            }

            var outdir = Path.GetDirectoryName(xunitOutputPath);

            var results = testPass.TestResults;

            int resultCount = results.Count;
            int passedCount = results.Where(r => r.IsReportAsPassed()).Count();
            int skippedCount = results.Where(r => r.Skipped).Count();

            // Since we re-run tests on failure, we'll mark every test that failed at least once as "skipped" rather than "failed".
            // If the test failed sufficiently often enough for it to count as a failed test (determined by a property on the
            // Azure DevOps job), we'll later mark it as failed during test results processing.
            int failedCount = 0;
            if (useRetryLogic)
            {
                failedCount = results.Where(r => r.IsReportAsFailed(requiredPassRateThreshold)).Count();
                skippedCount += results.Where(r => r.IsReportAsUnreliable(requiredPassRateThreshold)).Count();
            }
            else
            {
                failedCount = resultCount - passedCount - skippedCount;
            }

            var root = new XElement("assemblies");

            var assembly = new XElement("assembly");
            assembly.SetAttributeValue("name", "MUXControls.Test.dll");
            assembly.SetAttributeValue("test-framework", "TAEF");
            assembly.SetAttributeValue("run-date", DateTime.Now.ToString("yyyy-MM-dd"));

            // This doesn't need to be completely accurate since it's not exposed anywhere.
            // If we need accurate an start time we can probably calculate it from the te.wtl file, but for
            // now this is fine.
            assembly.SetAttributeValue("run-time", (DateTime.Now - testPass.TestPassExecutionTime).ToString("HH:mm:ss"));

            assembly.SetAttributeValue("total", resultCount);
            assembly.SetAttributeValue("passed", passedCount);
            assembly.SetAttributeValue("failed", failedCount);
            assembly.SetAttributeValue("skipped", skippedCount);

            assembly.SetAttributeValue("time", (int)testPass.TestPassExecutionTime.TotalSeconds);
            assembly.SetAttributeValue("errors", 0);
            root.Add(assembly);

            var collection = new XElement("collection");
            collection.SetAttributeValue("total", resultCount);
            collection.SetAttributeValue("passed", passedCount);
            collection.SetAttributeValue("failed", failedCount);
            collection.SetAttributeValue("skipped", skippedCount);
            collection.SetAttributeValue("name", "Test collection");
            collection.SetAttributeValue("time", (int)testPass.TestPassExecutionTime.TotalSeconds);
            assembly.Add(collection);

            foreach (var result in results)
            {
                var test = new XElement("test");
                string testName = result.Name;
                if(!string.IsNullOrEmpty(testNamePrefix))
                {
                    testName = testNamePrefix + "." + result.Name;
                }
                test.SetAttributeValue("name", testName);

                var className = GetTestClassName(result.Name);
                var methodName = GetTestMethodName(result.Name);

                test.SetAttributeValue("type", className);
                test.SetAttributeValue("method", methodName);

                test.SetAttributeValue("time", result.ExecutionTime.TotalSeconds);

                string resultString = string.Empty;

                if (result.IsReportAsPassed())
                {
                    resultString = "Pass";
                }
                else if(result.Skipped)
                {
                    resultString = "Skip";
                }
                else if (useRetryLogic)
                {
                    if(result.IsReportAsUnreliable(requiredPassRateThreshold))
                    {
                        resultString = "Skip";
                    }
                    else
                    {
                        resultString = "Fail";
                    }
                }
                else
                {
                    resultString = "Fail";
                }

                test.SetAttributeValue("result", resultString);

                if(useRetryLogic)
                {
                    if (!result.IsReportAsPassed() && !result.Skipped)
                    {
                        // If a test failed, we'll have rerun it multiple times.
                        // We'll save the subresults to a JSON text file that we'll upload to the helix results container -
                        // this allows it to be as long as we want, whereas the reason field in Azure DevOps has a 4000 character limit.

                        string subResultsFileName = Guid.NewGuid().ToString() + "_subresults.json";
                        string subResultsOutputPath = Path.Combine(outdir, subResultsFileName);

                        WriteSubResultsToJsonFile(result, subResultsOutputPath);

                        if (result.IsReportAsUnreliable(requiredPassRateThreshold))
                        {
                            var reason = new XElement("reason");
                            reason.Add(subResultsFileName);
                            test.Add(reason);
                        }
                        else
                        {
                            var failure = new XElement("failure");
                            var message = new XElement("message");
                            message.Add(subResultsFileName);
                            failure.Add(message);
                            test.Add(failure);
                        }
                    }
                }
                else // useRetryLogic=false
                {
                    if(result.Skipped)
                    {
                        var reason = new XElement("reason");
                        reason.Add(new XCData(result.Details));
                        test.Add(reason);
                    }
                    else if(result.IsFailed())
                    {
                        var failure = new XElement("failure");
                        var message = new XElement("message");
                        message.Add(new XCData(result.Details));
                        failure.Add(message);
                        test.Add(failure);
                    }
                }
                
                collection.Add(test);
            }

            File.WriteAllText(xunitOutputPath, root.ToString());
        }

        private void WriteSubResultsToJsonFile(TestResult result, string subResultsOutputPath)
        {
            // If a test failed, we'll have rerun it multiple times.  We'll record the results of each run
            // formatted as JSON.
            JsonSerializableTestResults serializableResults = new JsonSerializableTestResults();

            List<string> errorList = new List<string>();
            errorList.Add(result.Details);

            foreach (TestResult rerunResult in result.RerunResults)
            {
                errorList.Add(rerunResult.Details);
            }

            serializableResults.errors = errorList.Distinct().Where(s => s != null).ToArray();

            List<JsonSerializableTestResult> serializableResultList = new List<JsonSerializableTestResult>();
            serializableResultList.Add(ConvertToSerializableResult(result, serializableResults.errors));

            foreach (TestResult rerunResult in result.RerunResults)
            {
                serializableResultList.Add(ConvertToSerializableResult(rerunResult, serializableResults.errors));
            }

            serializableResults.results = serializableResultList.ToArray();

            using (MemoryStream stream = new MemoryStream())
            {
                DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(JsonSerializableTestResults));
                serializer.WriteObject(stream, serializableResults);
                stream.Position = 0;

                using (StreamReader streamReader = new StreamReader(stream))
                {
                    var json = streamReader.ReadToEnd();
                    File.WriteAllText(subResultsOutputPath, json);
                }
            }
        }

        private JsonSerializableTestResult ConvertToSerializableResult(TestResult rerunResult, string[] uniqueErrors)
        {
            var serializableResult = new JsonSerializableTestResult();

            serializableResult.outcome = rerunResult.IsPassedOrSkipped() ? "Passed" : "Failed";
            serializableResult.duration = (int)Math.Round(rerunResult.ExecutionTime.TotalMilliseconds);

            if (rerunResult.IsFailed())
            {
                serializableResult.log = Path.GetFileName(rerunResult.SourceWttFile);

                if (rerunResult.Screenshots.Any())
                {
                    List<string> screenshots = new List<string>();

                    foreach (var screenshot in rerunResult.Screenshots)
                    {
                        screenshots.Add(Path.GetFileName(screenshot));
                    }

                    serializableResult.screenshots = screenshots.ToArray();
                }

                // To conserve space, we'll log the index of the error to index in a list of unique errors rather than
                // jotting down every single error in its entirety. We'll add one to the result so we can avoid
                // serializing this property when it has the default value of 0.
                serializableResult.errorIndex = Array.IndexOf(uniqueErrors, rerunResult.Details) + 1;
            }

            return serializableResult;
        }

        private string GetTestNameSeparator(string testname)
        {
            var separatorString = ".";
            if (!testname.Contains(separatorString))
            {
                separatorString = "::";
            }
            return separatorString;
        }

        private string GetTestMethodName(string fullyQualifiedName)
        {
            var separatorString = GetTestNameSeparator(fullyQualifiedName);
            var methodName = fullyQualifiedName.Substring(fullyQualifiedName.LastIndexOf(separatorString) + separatorString.Length);

            return methodName;
        }

        private string GetTestClassName(string fullyQualifiedName)
        {
            var separatorString = GetTestNameSeparator(fullyQualifiedName);
            var className = fullyQualifiedName.Substring(0, fullyQualifiedName.LastIndexOf(separatorString));

            return className;
        }
    }
}
