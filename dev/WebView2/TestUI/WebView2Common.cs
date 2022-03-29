// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml.Controls;
using System;
using System.IO;
using System.Reflection;
using Windows.Storage;

#if !BUILD_WINDOWS
using WebView2 = Microsoft.UI.Xaml.Controls.WebView2;
#endif

namespace MUXControlsTestApp
{
    public class WebView2Common
    {
        readonly TestPage _testPage;
        readonly TextBox Status1;
        readonly TextBox Status2;
        readonly TextBox Status3;
        readonly TextBox MessageLog;

        public WebView2Common(TestPage testPage)
        {
            _testPage = testPage;

            Status1 = _testPage.FindName("Status1") as TextBox;
            Status2 = _testPage.FindName("Status2") as TextBox;
            Status3 = _testPage.FindName("Status3") as TextBox;
            MessageLog = _testPage.FindName("MessageLog") as TextBox;
        }

        public void ClearStatus()
        {
            Status1.Text = string.Empty;
            Status2.Text = string.Empty;
            Status3.Text = string.Empty;
        }

#if DESKTOP_APP
        // In Desktop (Packaged) context, CWV2 can access the files directly from the app's Assets folder (and copying like the UWP case fails).
        public void CopyTestPagesLocally()
        {
            Status1.Text = "HTML files ready";
        }

        // [Desktop (Packaged)] Get a file:// URI for the test page from app's Assets folder
        // Ex: file:///C:/Program%20Files/WindowsApps/MUXControlsTestApp.Desktop_1.0.0.0_x86__6f07fta6qpts2/Assets/SimplePage.html
        public static Uri GetTestPageUri(string testPageName)
        {
            string appDirectory  = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            string fullUri = @"file:///" + appDirectory + @"/Assets/" + testPageName;

            return new Uri(fullUri);
        }

#else

        readonly string[] TestPageNames =
        {
            "SimplePage.html",
            "SimplePageWithButton.html",
            "SimplePageWithScrollableColoredBlocks.html",
            "SimplePageForFocus.html",
            "SimplePageWithText.html",
            "SimpleInputPage.html",
            "SimplePageWithManyButtons.html",
        };

        // In UWP context, the CoreWebView2 can't load files from app-dir via file://, so copy them to %LOCALAPPDATA%
        async public void CopyTestPagesLocally()
        {
            foreach (string fileName in TestPageNames)
            {
                // Read file from Appx
                StorageFile sourceFile = await StorageFile.GetFileFromApplicationUriAsync(
                                                new Uri("ms-appx:///Assets/" + fileName));
                String sourceText = await FileIO.ReadTextAsync(sourceFile);

                // Write file to Appdata - LocalFolder. Anaheim can read it via fully qualified file:// URI
                StorageFolder storageFolder = Windows.Storage.ApplicationData.Current.LocalFolder;
                StorageFile destFile = await storageFolder.CreateFileAsync(fileName, CreationCollisionOption.ReplaceExisting);
                await FileIO.WriteTextAsync(destFile, sourceText);
            }
            Status1.Text = "HTML files ready"; // indicating all test files have been copied
        }

        // UWP Version: Get a file:// URI for the test page in app data (can be loaded by Anahaeim)
        // Ex: file:///C:/Users/<user>/AppData/Local/Packages/MUXControlsTestApp_6f07fta6qpts2/TempState/SimplePage.htm
        public static Uri GetTestPageUri(string testPageName)
        {
            string localAppDataPath = AppDataPaths.GetDefault().LocalAppData;
            string fullUri = @"file:///" + localAppDataPath + @"/" + testPageName;

            return new Uri(fullUri);
        }
#endif

        public static void NavigateToUri(WebView2 webview, string UriString)
        {
            Uri newUri;
            if (!string.IsNullOrEmpty(UriString) &&
                Uri.TryCreate(UriString, UriKind.RelativeOrAbsolute, out newUri) == true)
            {
                webview.Source = newUri;
            }
        }

        public static void NavigateToUri(WebView2 webview, Uri uri)
        {
            string uriString = uri.ToString();
            NavigateToUri(webview, uriString);
        }

        public static void NavigateToStringMessage(WebView2 webview)
        {
            webview.NavigateToString("You've navigated to a string message.");
        }

        public static void LoadWebPage(WebView2 webview, string testPageName)
        {
             Uri testPageUri = GetTestPageUri(testPageName);
             NavigateToUri(webview, testPageUri);
        }

        public void AppendMessage(string message)
        {
            if (MessageLog != null)
            {
                MessageLog.Text = MessageLog.Text + message + Environment.NewLine;
            }
        }
    }
}
