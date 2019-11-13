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
using static MUXControls.TestAppUtils.VisualTreeDumper;
using System.Diagnostics;

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
        All,
        None
    }

    // This class is used for internal debug purpose only. We don't have a flag to control log level in UnitTesting.Logging.
    // Like debug information, it's too big and we should not show to customer. You can change the setting by _shouldLogDebugInfo
    class VisualTreeLog
    {
        public static void LogInfo(string info)
        {
            Log.Comment(info);
        }

        [Conditional("DebugInfoON")]
        public static void LogDebugInfo(string debugInfo) 
        {
            Log.Comment(debugInfo);  
        }
    }

    // Useful for scenarios where you would want to run multiple Visual Tree Verifications without throwing an exception
    public class VisualTreeVerifier: IDisposable
    {
        public bool hasFailed = false;

        public void VerifyVisualTreeNoException(string xaml, string masterFilePrefix, Theme theme = Theme.None, IPropertyValueTranslator translator = null, IFilter filter = null, IVisualTreeLogger logger = null)
        {
            try
            {
                VisualTreeTestHelper.VerifyVisualTree(xaml, masterFilePrefix, theme, translator, filter, logger);
            }
            catch (Exception e)
            {
                Log.Error(e.Message);
                hasFailed = true;
            }
        }

        public void VerifyVisualTreeNoException(UIElement root, string masterFilePrefix, Theme theme = Theme.None, IPropertyValueTranslator translator = null, IFilter filter = null, IVisualTreeLogger logger = null)
        {
            try
            {
                VisualTreeTestHelper.VerifyVisualTree(root, masterFilePrefix, theme, translator, filter, logger);
            }
            catch (Exception e)
            {
                Log.Error(e.Message);
                hasFailed = true;
            }
        }

        public void Dispose()
        {
            if(hasFailed)
            {
                Verify.Fail("Visual Tree Verification Failed.");
            }
        }

    }


    public class VisualTreeTestHelper
    {
        // Log MasterFile to MusicLibrary or LocalFolder. By default(AlwaysLogMasterFile=false), It logs the master files to  LocalFolder. 
        // You can change the setting by set AlwaysLogMasterFile = true. After you run the tests and test case fails, the master files are put into MusicLibrary.
        // Finally you can copy master files to master/ directory and check in with code. 
        public static bool AlwaysLogMasterFile = true;
        
        public static void ChangeRequestedTheme(UIElement root, ElementTheme theme)
        {
            FrameworkElement element = root as FrameworkElement;

            VisualTreeLog.LogDebugInfo("Request Theme: " + theme.ToString());
            RunOnUIThread.Execute(() =>
            {
                element.RequestedTheme = theme;
            });
        }

        public static UIElement SetupVisualTree(string xaml)
        {
            VisualTreeLog.LogDebugInfo("SetupVisualTree " + xaml);

            UIElement root = null;
            RunOnUIThread.Execute(() =>
            {
                root = (UIElement)XamlReader.Load(xaml);
                MUXControlsTestApp.App.TestContentRoot = root;
            });

            MUXControlsTestApp.Utilities.IdleSynchronizer.Wait();
            return root;
        }

        public static string DumpVisualTree(DependencyObject root, IPropertyValueTranslator translator = null, IFilter filter = null, IVisualTreeLogger logger = null)
        {
            VisualTreeLog.LogDebugInfo("DumpVisualTree");

            string content = "";
            RunOnUIThread.Execute(() =>
                {
                    content = VisualTreeDumper.DumpToXML(root, translator, filter, logger);
                });
            return content;
        }

        public static void VerifyVisualTree(string xaml, string masterFilePrefix, Theme theme = Theme.None, IPropertyValueTranslator translator = null, IFilter filter = null, IVisualTreeLogger logger = null)
        {
            var root = SetupVisualTree(xaml);
            VerifyVisualTree(root, masterFilePrefix, theme, translator, filter, logger);
        }

        public static void VerifyVisualTree(UIElement root, string masterFilePrefix, Theme theme = Theme.None, IPropertyValueTranslator translator = null, IFilter filter = null, IVisualTreeLogger logger = null)
        {
            VisualTreeLog.LogInfo("VerifyVisualTree for theme " + theme.ToString());
            TestExecution helper = new TestExecution(translator, filter, logger, AlwaysLogMasterFile);

            if (theme == Theme.None)
            {
                helper.DumpAndVerifyVisualTree(root, masterFilePrefix);
            }
            else
            {
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
                    string themeName = requestedTheme.ToString();
                    VisualTreeLog.LogInfo("Change RequestedTheme to " + themeName);
                    ChangeRequestedTheme(root, requestedTheme);

                    helper.DumpAndVerifyVisualTree(root, masterFilePrefix + "_" + themeName, "DumpAndVerifyVisualTree for " + themeName);
                }
            }
            if (helper.HasError())
            {
                Verify.Fail(helper.GetTestResult(), "Test Failed");
            }
        }

        private class TestExecution
        {
            private IPropertyValueTranslator _translator;
            private IFilter _filter;
            private IVisualTreeLogger _logger;
            private StringBuilder _testResult;
            private bool _shouldLogMasterFile;

            public TestExecution(IPropertyValueTranslator translator = null, IFilter filter = null, IVisualTreeLogger logger = null, bool shouldLogMasterFile = true)
            {
                _translator = translator;
                _filter = filter;
                _logger = logger;
                _shouldLogMasterFile = shouldLogMasterFile;
                _testResult = new StringBuilder();
            }

            public bool HasError()
            {
                return (_testResult.Length != 0);
            }

            public string GetTestResult()
            {
                return _testResult.ToString();
            }

            public void DumpAndVerifyVisualTree(UIElement root, string masterFilePrefix, string messageOnError = null)
            {
                VisualTreeLog.LogDebugInfo("DumpVisualTreeAndCompareWithMaster with masterFilePrefix " + masterFilePrefix);

                string expectedContent = "";
                string content = DumpVisualTree(root, _translator, _filter, _logger);

                MasterFileStorage storage = new MasterFileStorage(!_shouldLogMasterFile, masterFilePrefix);
                string bestMatchedMasterFileName = storage.BestMatchedMasterFileName;
                string expectedMasterFileName = storage.ExpectedMasterFileName;

                VisualTreeLog.LogDebugInfo("Target master file: " + expectedMasterFileName);
                VisualTreeLog.LogDebugInfo("Best matched master file: " + bestMatchedMasterFileName);

                if (!string.IsNullOrEmpty(bestMatchedMasterFileName))
                {
                    expectedContent = MasterFileStorage.GetMasterFileContent(bestMatchedMasterFileName);
                }

                string result = new VisualTreeOutputCompare(content, expectedContent).ToString();
                if (!string.IsNullOrEmpty(result))
                {
                    storage.LogMasterFile(expectedMasterFileName, content);
                    storage.LogMasterFile(expectedMasterFileName + ".orig", expectedContent);

                    if (!string.IsNullOrEmpty(messageOnError))
                    {
                        _testResult.AppendLine(messageOnError);
                        _testResult.AppendLine(string.Format("{0}.xml and {0}.orig.xaml is logged", expectedMasterFileName));
                    }
                    _testResult.AppendLine(result);
                }
            }
        }
    }

    class MasterFileStorage
    {
        private StorageFolder _storage;
        public string StorageLocation { get; private set; }

        /*
          master file searching rule: If running os is RS5, the api version = 7, then first file to check is if {masterFileNamePrefix}-7.xml exists, 
          if not, try {masterFileNamePrefix}-6.xml... {masterFileNamePrefix}-2.xml, and finally {masterFileNamePrefix}.xml. If all files doesn't exist, return null.
          ExpectedMasterFileName: masterFileNamePrefix+running os API version, eg: {masterFileNamePrefix}-7.xml
          BestMatchedMasterFileName: the master file name matched by the searching rule.
        */
        public string ExpectedMasterFileName { get; private set; }
        public string BestMatchedMasterFileName { get; private set; }

        public MasterFileStorage(bool useLocalStorage, string masterFileNamePrefix)
        {
            _storage = useLocalStorage ? ApplicationData.Current.LocalFolder : KnownFolders.PicturesLibrary;
            ExpectedMasterFileName = GetExpectedMasterFileName(masterFileNamePrefix);
            BestMatchedMasterFileName = SearchBestMatchedMasterFileName(masterFileNamePrefix);
            StorageLocation = useLocalStorage ? ApplicationData.Current.LocalFolder.Path : "PictureLibrary";
        }

        public void LogMasterFile(string fileName, string content)
        {
            LogMasterFile(_storage, fileName, content);
        }

        private string GetExpectedMasterFileName(string fileNamePrefix)
        {
            return string.Format("{0}-{1}.xml", fileNamePrefix, PlatformConfiguration.GetCurrentAPIVersion());
        }

        private string SearchBestMatchedMasterFileName(string fileNamePrefix)
        {
            for (ushort version = PlatformConfiguration.GetCurrentAPIVersion(); version >= 2; version--)
            {
                string fileName = string.Format("{0}-{1}.xml", fileNamePrefix, version);
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

    // A simple string diff class to provide readable text to show the difference of two strings. Here is a example of output.
    // + This is only in A
    // - This is only in B
    class VisualTreeOutputCompare
    {
        public VisualTreeOutputCompare(string a, string b)
        {
            var diffBuilder = new InlineDiffBuilder(new DiffPlex.Differ());
            var diff = diffBuilder.BuildDiffModel(a, b);

            foreach (var line in diff.Lines)
            {
                switch (line.Type)
                {
                    case ChangeType.Inserted:
                        _sb.AppendLine("+ " + line.Text);
                        break;
                    case ChangeType.Deleted:
                        _sb.AppendLine("- " + line.Text);
                        break;
                    default:
                        break;
                }
            }
        }
        private StringBuilder _sb = new StringBuilder();
        public override string ToString()
        {
            return _sb.ToString();
        }
    }
}
