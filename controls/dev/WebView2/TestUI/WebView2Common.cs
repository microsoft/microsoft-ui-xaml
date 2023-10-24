// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
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
