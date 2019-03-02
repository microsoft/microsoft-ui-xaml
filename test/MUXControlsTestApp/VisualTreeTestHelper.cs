// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using System.Threading.Tasks;
using Windows.Storage;
using System.IO;
using MUXControls.TestAppUtils;
using MUXControlsTestApp.Utilities;
using System.Text;
using System.Collections.Generic;
using Windows.UI.Xaml.Markup;
using DiffPlex.DiffBuilder;
using DiffPlex.DiffBuilder.Model;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using PlatformConfiguration = Common.PlatformConfiguration;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    public enum Theme
    {
        Dark,
        Light,
        All
    }

    /// <summary>
    /// TestBaseClass helps to compare visualtree.
    /// Don't add <c>[TestInitialize]</c> in your test case, instead, just override <see cref="OnTestInitialized"/>
    ///  <see cref="VerifyVisualTree"/> helps to compare the visualtree with a 'master' file.
    /// Master filename for testcase would [testclass]_[testname]_[theme].xml or [testclass]_[testname]_[theme]-[apiversion].xml
    /// </summary>
    public class VisualTreeTestBase
    {
        public const string LogMasterFileRuntimeParameterName = "LogMasterFile";

        public VisualTreeDumper.IFilter VisualTreeDumpFilter { get; set; }
        public VisualTreeDumper.IPropertyValueTranslator PropertyValueTranslator { get; set; }

        public TestContext TestContext { get; set; }

        public bool ShouldLogMasterFile { get; set; }

        public string TestCaseName { get; private set; }

        // To avoid filename conflict for master file, we use the format of [testclass]_[testname] as prefix by default.
        // For example, Windows.UI.Xaml.Tests.MUXControls.ApiTests.NavigationViewVisualTreeTests.VerifyVisualTreeForNavView
        // The prefix is NavigationViewVisualTreeTests_VerifyVisualTreeForNavView
        public string MasterFileNamePrefix { get; set; }

        public UIElement SetupVisualTree(string xaml)
        {
            UIElement root = null;
            RunOnUIThread.Execute(() =>
            {
                root = (UIElement)XamlReader.Load(xaml);
                MUXControlsTestApp.App.TestContentRoot = root;
            });

            MUXControlsTestApp.Utilities.IdleSynchronizer.Wait();
            return root;
        }

        public void VerifyVisualTree(string xaml)
        {
            var root = SetupVisualTree(xaml);
            VerifyVisualTree(root, Theme.All);
        }

        public void VerifyVisualTree(UIElement root, Theme theme)
        {
            var element = root as FrameworkElement;
            CheckTrue(element != null, "Expect FrameworkElement");

            bool hasError = false;

            List<ElementTheme> themes = new List<ElementTheme>();
            if (theme == Theme.Dark)
            {
                themes.Add(ElementTheme.Dark);
            }
            else if (theme == Theme.Light)
            {
                themes.Add(ElementTheme.Light);
            }
            else if (theme == Theme.All)
            {
                themes = new List<ElementTheme>() { ElementTheme.Dark, ElementTheme.Light };
            }

            foreach (var requestedTheme in themes)
            {
                Log.Comment("Request Theme: " + requestedTheme.ToString());
                RunOnUIThread.Execute(() =>
                {
                    element.RequestedTheme = requestedTheme;
                });

                try
                {
                    Log.Comment("VerifyVisualTree for " + requestedTheme.ToString());
                    VerifyVisualTree(root, requestedTheme.ToString());
                }
                catch (Exception e)
                {
                    hasError = true;
                    Log.Comment(e.Message);
                }
            }
            Verify.IsFalse(hasError, "VerifyVisualTree should not have error");
        }

        public void VerifyVisualTree(UIElement root, String customName)
        {
            VisualTreeCompare(root, MasterFileNamePrefix, customName);
        }

        private void VerifyVisualTree(UIElement root)
        {
            VisualTreeCompare(root, MasterFileNamePrefix, "");
        }

        [TestInitialize]
        public void TestInitialize()
        {
            CheckTrue(TestContext != null, "Expect framework populate the TestContext");
            CheckTrue(!String.IsNullOrEmpty(TestContext.TestName), "TestName should not be empty");

            // based on the testframework, TestName may be Windows.UI.Xaml.Tests.MUXControls.ApiTests.NavigationViewVisualTreeTests.VerifyVisualTreeForNavView
            // or TestName=VerifyVisualTreeForNavView and FullyQualifiedTestClassName=Windows.UI.Xaml.Tests.MUXControls.ApiTests.NavigationViewVisualTreeTests
            var testCaseFullName = TestContext.TestName;
            if (!TestContext.TestName.Contains("."))
            {
                CheckTrue(!String.IsNullOrEmpty(TestContext.FullyQualifiedTestClassName), "TestContext.FullyQualifiedTestClassName should not be empty");
                testCaseFullName = TestContext.FullyQualifiedTestClassName + "." + TestContext.TestName;
            }

            String[] elements = testCaseFullName.Split('.');
            TestCaseName = elements[elements.Length - 1];
            MasterFileNamePrefix = elements[elements.Length - 2] + "_" + TestCaseName;
            VisualTreeDumpFilter = null;
            PropertyValueTranslator = null;
            LogTestContext();
            ShouldLogMasterFile = TestContextContainsKey(LogMasterFileRuntimeParameterName);

            OnTestInitialized();
        }
        protected virtual void OnTestInitialized() { }

        private void LogTestContext()
        {
#if DEBUG
            Log.Comment("FullyQualifiedTestClassName: " + TestContext.FullyQualifiedTestClassName);
            Log.Comment("TestName: " + TestContext.TestName);
#endif
            Log.Comment("MasterFileNamePrefix: " + MasterFileNamePrefix);

            if (TestContextContainsKey(LogMasterFileRuntimeParameterName))
            {
                Log.Comment(LogMasterFileRuntimeParameterName + ": True");
            }
            else
            {
                Log.Comment(LogMasterFileRuntimeParameterName + ": False");
            }
        }


        private void VisualTreeCompare(UIElement root, string masterFilePrefix, string theme)
        {
            string content = "";
            string expectedContent = "";

            RunOnUIThread.Execute(() =>
            {
                content = VisualTreeDumper.DumpToXML(root, PropertyValueTranslator, VisualTreeDumpFilter, null);
            });

            MasterFileStorage storage = new MasterFileStorage(!ShouldLogMasterFile, masterFilePrefix, theme);
            string bestMatchedMasterFileName = storage.BestMatchedMasterFileName;
            string expectedMasterFileName = storage.ExpectedMasterFileName;

            Log.Comment("Target master file: " + expectedMasterFileName);
            Log.Comment("Best matched master file: " + bestMatchedMasterFileName);

            VisualTreeOutputCompare result = new VisualTreeOutputCompare("", "");
            if (String.IsNullOrEmpty(bestMatchedMasterFileName))
            {
                result.AddError("Can't find master file for " + TestCaseName);
            }
            else
            {
                expectedContent = MasterFileStorage.GetMasterFileContent(bestMatchedMasterFileName);
                result = new VisualTreeOutputCompare(content, expectedContent);
            }

            if (result.HasError())
            {
                storage.LogMasterFile(expectedMasterFileName, content);
                storage.LogMasterFile(expectedMasterFileName + ".orig", expectedContent);

                Log.Comment(result.ToString());
                string error = String.Format("Compare failed, but {0} is put into {1}", expectedMasterFileName, storage.StorageLocation);
                Verify.Fail(error);
            }
        }

        // Avoid too much logs, and only log message when !flag.
        private void CheckTrue(bool flag, string message)
        {
            if (!flag)
            {
                Verify.Fail(message);
            }
        }

        private bool TestContextContainsKey(string key)
        {
#if USING_TAEF
           return TestContext.Properties.Contains(key);
#else
            return TestContext.Properties.ContainsKey(key);
#endif
        }
    }

    class MasterFileStorage
    {
        private StorageFolder _storage;
        public string StorageLocation { get; private set; }
        public string ExpectedMasterFileName { get; private set; }
        public string BestMatchedMasterFileName { get; private set; }
        public MasterFileStorage(bool useLocalStorage, string masterFileNamePrefix, string theme)
        {
            _storage = useLocalStorage ? ApplicationData.Current.LocalFolder : KnownFolders.MusicLibrary;
            string prefix = String.IsNullOrEmpty(theme) ? masterFileNamePrefix : masterFileNamePrefix + "-" + theme;
            ExpectedMasterFileName = GetExpectedMasterFileName(prefix);
            BestMatchedMasterFileName = SearchBestMatchedMasterFileName(prefix);
            StorageLocation = useLocalStorage ? ApplicationData.Current.LocalFolder.Path : "MusicLibrary";
        }

        public void LogMasterFile(string fileName, string content)
        {
            LogMasterFile(_storage, fileName, content);
        }

        private string GetExpectedMasterFileName(string fileNamePrefix)
        {
            return String.Format("{0}-{1}.xml", fileNamePrefix, PlatformConfiguration.GetCurrentAPIVersion());
        }

        private string SearchBestMatchedMasterFileName(string fileNamePrefix)
        {
            for (ushort version = PlatformConfiguration.GetCurrentAPIVersion(); version >= 2; version--)
            {
                string fileName = String.Format("{0}-{1}.xml", fileNamePrefix, version);
                if (MasterFileStorage.IsMasterFilePresent(fileName))
                {
                    return fileName;
                }
            }
            {
                string fileName = fileNamePrefix + ".xml";
                if (MasterFileStorage.IsMasterFilePresent(fileName))
                {
                    return fileName;
                }
            }
            return null;
        }

        public static string GetMasterFileContent(string fileName)
        {
            return GetMasterFileContentAsync(fileName).Result;
        }

        public static bool IsMasterFilePresent(string fileName)
        {
            try
            {
                GetMasterFileContentAsync(fileName).Wait();
                return true;
            }
            catch (AggregateException ae)
            {
                ae.Handle((x) =>
                {
                    return (x is FileNotFoundException); // This we know how to handle.
                });
                return false;
            }
        }

        public static void LogMasterFile(StorageFolder storageFolder, string fileName, string content)
        {
            LogMasterFileAsync(storageFolder, fileName, content).Wait();
        }

        private static readonly string MasterFileFullPathPrefix = "ms-appx:///master/";

        private static async Task<string> GetMasterFileContentAsync(string fileName)
        {
            Uri uri = new Uri(MasterFileFullPathPrefix + fileName);
            var file = await StorageFile.GetFileFromApplicationUriAsync(uri);
            return await FileIO.ReadTextAsync(file);
        }

        private static async Task LogMasterFileAsync(StorageFolder storageFolder, string fileName, string content)
        {
            var file = await storageFolder.CreateFileAsync(fileName, CreationCollisionOption.ReplaceExisting);
            await FileIO.WriteTextAsync(file, content);
        }
    }

    class VisualTreeOutputCompare
    {
        public VisualTreeOutputCompare(string xml1, string xml2)
        {
            var diffBuilder = new InlineDiffBuilder(new DiffPlex.Differ());
            var diff = diffBuilder.BuildDiffModel(xml1, xml2);

            foreach (var line in diff.Lines)
            {
                switch (line.Type)
                {
                    case ChangeType.Inserted:
                        AddError("+ " + line.Text);
                        break;
                    case ChangeType.Deleted:
                        AddError("- " + line.Text);
                        break;
                    default:
                        break;
                }
            }
        }
        private StringBuilder _sb = new StringBuilder();

        public bool HasError()
        {
            return _sb.Length != 0;
        }
        public void AddError(String message)
        {
            _sb.AppendLine(message);
        }

        public override string ToString()
        {
            return _sb.ToString();
        }
    }
}
