// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Markup;
using Windows.UI.Xaml.Navigation;
using System;
using System.Diagnostics;
using System.Threading.Tasks;

#if !BUILD_WINDOWS
using WebView2 = Microsoft.UI.Xaml.Controls.WebView2;
using CoreWebView2NavigationStartingEventArgs = Microsoft.Web.WebView2.Core.CoreWebView2NavigationStartingEventArgs;
using CoreWebView2NavigationCompletedEventArgs = Microsoft.Web.WebView2.Core.CoreWebView2NavigationCompletedEventArgs;
using CoreWebView2ProcessFailedEventArgs = Microsoft.Web.WebView2.Core.CoreWebView2ProcessFailedEventArgs;
#endif

namespace MUXControlsTestApp
{
    public sealed partial class WebView2CoreObjectsPage : TestPage
    {
        enum TestList
        {
            BasicCoreObjectCreationAndDestructionTest,
            ConcurrentCreationRequestsTest,
            EdgeProcessFailedTest
        };

        readonly WebView2Common _helpers;
        WebView2 _wv2_Programmatic_Offline;
        WebView2 _wv2_Programmatic_Live;
        WebView2 _wv2_Markup;
        WebView2 _wv2_ConcurrentCreation;

        WebView2 _wv2_CoreProcessFailed;

        int _coreWebView2InitializedCount = 0;
        int _ensureCoreWebView2CompletionCount = 0;
        int _navigationStartingCount = 0;
        int _navigationCompletedCount = 0;
        int _loadedCount = 0;
        int _unloadedCount = 0;
        int _closedExceptionCount = 0;
        int _coreProcessFailedCount = 0;

        bool _areWebviewElementsCleanedUp = false;

        public WebView2CoreObjectsPage()
        {
            InitializeComponent();
            _helpers = new WebView2Common(this);
            Loaded += Page_Loaded;
        }

        private void Page_Loaded(object sender, RoutedEventArgs e)
        {
            _helpers.ClearStatus();
            _helpers.CopyTestPagesLocally();
        }

        void SetCoreWebView2InitializedCount(int value)
        {
            CoreWebView2InitializedCount.Text = value.ToString();
            _coreWebView2InitializedCount = value;
        }
        void SetEnsureCoreWebView2CompletionCount(int value)
        {
            EnsureCoreWebView2CompletionCount.Text = value.ToString();
            _ensureCoreWebView2CompletionCount = value;
        }

        void SetNavigationStartingCount(int value)
        {
            NavigationStartingCount.Text = value.ToString();
            _navigationStartingCount = value;
        }

        void SetNavigationCompletedCount(int value)
        {
            NavigationCompletedCount.Text = value.ToString();
            _navigationCompletedCount = value;
        }

        void SetLoadedCount(int value)
        {
            LoadedCount.Text = value.ToString();
            _loadedCount = value;
        }

        void SetUnloadedCount(int value)
        {
            UnloadedCount.Text = value.ToString();
            _unloadedCount = value;
        }

        void SetClosedExceptionCount(int value)
        {
            ClosedExceptionCount.Text = value.ToString();
            _closedExceptionCount = value;
        }

        void SetCoreProcessFailedCount(int value)
        {
            CoreProcessFailedCount.Text = value.ToString();
            _coreProcessFailedCount = value;
        }

        void ResetCounts()
        {
            SetCoreWebView2InitializedCount(0);
            SetEnsureCoreWebView2CompletionCount(0);
            SetNavigationStartingCount(0);
            SetNavigationCompletedCount(0);
            SetLoadedCount(0);
            SetUnloadedCount(0);
            SetClosedExceptionCount(0);
            SetCoreProcessFailedCount(0);
        }

        void AddWebViewEventHandlers(WebView2 webview)
        {
            ResetCounts();

            webview.NavigationStarting += OnNavigationStarting;
            webview.NavigationCompleted += OnNavigationCompleted;
            webview.CoreWebView2Initialized += OnCoreWebView2Initialized;
            webview.Loaded += OnLoaded;
            webview.Unloaded += OnUnloaded;
            webview.CoreProcessFailed += OnCoreProcessFailed;
        }

        void RemoveWebViewEventHandlers(WebView2 webview)
        {
            ResetCounts();

            webview.NavigationStarting -= OnNavigationStarting;
            webview.NavigationCompleted -= OnNavigationCompleted;
            webview.CoreWebView2Initialized -= OnCoreWebView2Initialized;
            webview.Loaded -= OnLoaded;
            webview.Unloaded -= OnUnloaded;
            webview.CoreProcessFailed -= OnCoreProcessFailed;
        }

        private void TestNameComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            // TestList test = (TestList)Enum.Parse(typeof(TestList), TestNameComboBox.GetSelectedText());
            // Nothing needed per test?
        }

        private void OnCoreWebView2Initialized(WebView2 sender, object args)
        {
            SetCoreWebView2InitializedCount(_coreWebView2InitializedCount + 1);

            string s = string.Format("[{0}]: Got CoreWebView2Initialized, ", sender.Name);
            _helpers.AppendMessage(s);
            Status2.Text = Status2.Text + " " + s;
        }

        // Set Source from OnCoreWebView2Initialized handler (for ConcurrentCreationRequestsTest)
        private void OnCoreWebView2Initialized_Source(WebView2 sender, object args)
        {
            sender.CoreWebView2Initialized -= OnCoreWebView2Initialized_Source; // Don't spin creation on next CWV2Initialized!
            WebView2Common.NavigateToUri(sender, WebView2Common.GetTestPageUri("SimplePageWithButton.html"));
        }

        private async void OnCoreWebView2Initialized_Ensure(WebView2 sender, object args)
        {
            sender.CoreWebView2Initialized -= OnCoreWebView2Initialized_Ensure; // Don't spin creation on next CWV2Initialized!
            await sender.EnsureCoreWebView2Async();
            {
                SetEnsureCoreWebView2CompletionCount(_ensureCoreWebView2CompletionCount + 1);

                string s = string.Format("[{0}]: Got EnsureCWV2 Completion, ", sender.Name);
                _helpers.AppendMessage(s);
                Status2.Text = Status2.Text + " " + s;
            }
        }

        private void OnNavigationStarting(WebView2 sender, CoreWebView2NavigationStartingEventArgs args)
        {
            SetNavigationStartingCount(_navigationStartingCount + 1);

            string s = string.Format("[{0}]: Got NavigationStarting ({1}), ", sender.Name, args.Uri);
            _helpers.AppendMessage(s);
            Status1.Text = s;
            Status3.Text = args.Uri;
        }

        private void OnNavigationCompleted(WebView2 sender, CoreWebView2NavigationCompletedEventArgs args)
        {
            SetNavigationCompletedCount(_navigationCompletedCount + 1);

            string s = string.Format("[{0}]: Got NavigationcOompleted, ", sender.Name);
            _helpers.AppendMessage(s);
            Status2.Text = Status2.Text + " " + s;
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            SetLoadedCount(_loadedCount + 1);

            string s = string.Format("[{0}]: Got Loaded, ", (sender as WebView2).Name);
            _helpers.AppendMessage(s);
            Status2.Text = Status2.Text + " " + s;
        }

        private void OnUnloaded(object sender, RoutedEventArgs e)
        {
            SetUnloadedCount(_unloadedCount + 1);

            string s = string.Format("[{0}]: Got Unloaded, ", (sender as WebView2).Name);
            _helpers.AppendMessage(s);
            Status2.Text = Status2.Text + " " + s;
        }

        private void OnCoreProcessFailed(object sender, CoreWebView2ProcessFailedEventArgs e)
        {
            var error = e.ProcessFailedKind;
            EventDetail.Text = error.ToString();

            string s = string.Format("[{0}]: Got CoreProcessFailed ({1}), ", (sender as WebView2).Name, error.ToString());
            _helpers.AppendMessage(s);
            Status2.Text = Status2.Text + " " + s;

            SetCoreProcessFailedCount(_coreProcessFailedCount + 1);
        }

        public void OnResetCounts_ButtonClicked(object sender, RoutedEventArgs args)
        {
            ResetCounts();
        }

        public void OnCreate_OfflineElement_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_Programmatic_Offline != null)
            {
                RemoveWebViewEventHandlers(_wv2_Programmatic_Offline);
                _wv2_Programmatic_Offline.Close();
            }

            var myWebView = new WebView2 {
                Name = "WV2_Programmatic_Offline"
            };
            AddWebViewEventHandlers(myWebView);
            _wv2_Programmatic_Offline = myWebView;
        }

        public void OnSetSource_OfflineElement_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_Programmatic_Offline != null)
            {
                try
                {
                    Uri newUri = WebView2Common.GetTestPageUri("SimplePage.html");
                    _wv2_Programmatic_Offline.Source = newUri;
                }
                catch (Exception e)
                {
                    SetClosedExceptionCount(_closedExceptionCount + 1);

                    string s = string.Format("{0} Caught exception: {1}", _wv2_Programmatic_Offline.Name, e.ToString());
                    _helpers.AppendMessage(s);
                    Status2.Text = Status2.Text + " " + s;
                }
            }
        }

        public async void OnEnsureCoreWebView2_OfflineElement_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_Programmatic_Offline != null)
            {
                try
                {
                    await _wv2_Programmatic_Offline.EnsureCoreWebView2Async();
                    {
                        SetEnsureCoreWebView2CompletionCount(_ensureCoreWebView2CompletionCount + 1);

                        string s = string.Format("[{0}]: Got EnsureCWV2 Completion, ", _wv2_Programmatic_Offline.Name);
                        _helpers.AppendMessage(s);
                        Status2.Text = Status2.Text + " " + s;
                    }
                }
                catch (Exception e)
                {
                    SetClosedExceptionCount(_closedExceptionCount + 1);

                    string s = string.Format("{0} Caught exception: {1}", _wv2_Programmatic_Offline.Name, e.ToString());
                    _helpers.AppendMessage(s);
                    Status2.Text = Status2.Text + " " + s;
                }

            }
        }

        public void OnAdd_OfflineElement_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_Programmatic_Offline != null)
            {
                WebView2_Container1.Child = _wv2_Programmatic_Offline;
            }
        }

        public void OnRemove_OfflineElement_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_Programmatic_Offline != null)
            {
                WebView2_Container1.Child = null;
            }
        }

        public void OnReleaseReference_OfflineElement_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_Programmatic_Offline != null)
            {
                _wv2_Programmatic_Offline = null;
            }
        }

        public void OnClose_OfflineElement_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_Programmatic_Offline != null)
            {
                _wv2_Programmatic_Offline.Close();
            }
        }

        public void OnSetCustomSource_OfflineElement_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_Programmatic_Offline != null)
            {
                try
                {
                    Uri newUri;
                    string newUriString = CustomSource_UriTextBox.Text;
                    Uri.TryCreate(newUriString, UriKind.RelativeOrAbsolute, out newUri);
                    _wv2_Programmatic_Offline.Source = newUri;
                }
                catch (Exception e)
                {
                    SetClosedExceptionCount(_closedExceptionCount + 1);

                    string s = string.Format("{0} Caught exception: {1}", _wv2_Programmatic_Offline.Name, e.ToString());
                    _helpers.AppendMessage(s);
                    Status2.Text = Status2.Text + " " + s;
                }
            }
        }

        public void OnCreate_LiveElement_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_Programmatic_Live != null)
            {
                RemoveWebViewEventHandlers(_wv2_Programmatic_Live);
                _wv2_Programmatic_Live.Close();
            }

            var myWebView = new WebView2 {
                Name = "WV2_Programmatic_Live"
            };
            AddWebViewEventHandlers(myWebView);
            _wv2_Programmatic_Live = myWebView;
            WebView2_Container2.Child = _wv2_Programmatic_Live;
        }

        public void OnSetSource_LiveElement_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_Programmatic_Live != null)
            {
                try
                {
                    WebView2Common.NavigateToUri(_wv2_Programmatic_Live, WebView2Common.GetTestPageUri("SimplePage.html"));
                }
                catch (Exception e)
                {
                    SetClosedExceptionCount(_closedExceptionCount + 1);

                    string s = string.Format("{0} Caught exception: {1}", _wv2_Programmatic_Live.Name, e.ToString());
                    _helpers.AppendMessage(s);
                    Status2.Text = Status2.Text + " " + s;
                }
            }
        }

        public async void OnEnsureCoreWebView2_LiveElement_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_Programmatic_Live != null)
            {
                try
                {
                    await _wv2_Programmatic_Live.EnsureCoreWebView2Async();
                    {
                        SetEnsureCoreWebView2CompletionCount(_ensureCoreWebView2CompletionCount + 1);

                        string s = string.Format("[{0}]: Got EnsureCWV2 Completion, ", _wv2_Programmatic_Live.Name);
                        _helpers.AppendMessage(s);
                        Status2.Text = Status2.Text + " " + s;
                    }
                }
                catch (Exception e)
                {
                    SetClosedExceptionCount(_closedExceptionCount + 1);

                    string s = string.Format("{0} Caught exception: {1}", _wv2_Programmatic_Live.Name, e.ToString());
                    _helpers.AppendMessage(s);
                    Status2.Text = Status2.Text + " " + s;
                }
            }
        }

        public void OnRemove_LiveElement_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_Programmatic_Live != null)
            {
                WebView2_Container2.Child = null;
            }
        }

        public void OnReleaseReference_LiveElement_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_Programmatic_Live != null)
            {
                _wv2_Programmatic_Live = null;
            }
        }

        public void OnClose_LiveElement_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_Programmatic_Live != null)
            {
                _wv2_Programmatic_Live.Close();
            }
        }

        public void OnSetCustomSource_LiveElement_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_Programmatic_Live != null)
            {
                try
                {
                    Uri newUri;
                    string newUriString = CustomSource_UriTextBox.Text;
                    Uri.TryCreate(newUriString, UriKind.RelativeOrAbsolute, out newUri);
                    _wv2_Programmatic_Live.Source = newUri;
                }
                catch (Exception e)
                {
                    SetClosedExceptionCount(_closedExceptionCount + 1);

                    string s = string.Format("{0} Caught exception: {1}", _wv2_Programmatic_Live.Name, e.ToString());
                    _helpers.AppendMessage(s);
                    Status2.Text = Status2.Text + " " + s;
                }
            }
        }

        public void OnCreate_MarkupElement_Blank_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_Markup != null)
            {
                RemoveWebViewEventHandlers(_wv2_Markup);
                _wv2_Markup.Close();
            }
        
            var myWebView = (WebView2) XamlReader.Load(@"
                <controls:WebView2 xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                          xmlns:controls='using:Microsoft.UI.Xaml.Controls'
                          xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                          x:Name='WV2_Markup'/>");

            AddWebViewEventHandlers(myWebView);
            _wv2_Markup = myWebView;
            WebView2_Container3.Child = _wv2_Markup;
        }

        public void OnCreate_MarkupElement_Source_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_Markup != null)
            {
                _wv2_Markup.Close();
                RemoveWebViewEventHandlers(_wv2_Markup);
            }

            string uriString = WebView2Common.GetTestPageUri("SimplePage.html").ToString();

            var myWebView = (WebView2) XamlReader.Load(@"
                <controls:WebView2 xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                          xmlns:controls='using:Microsoft.UI.Xaml.Controls'
                          xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                          x:Name='_wv2_Markup'
                          Source='" +  uriString + @"'/>");

            AddWebViewEventHandlers(myWebView);
            _wv2_Markup = myWebView;
            WebView2_Container3.Child = _wv2_Markup;
        }

        public void OnSetSource_MarkupElement_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_Markup != null)
            {
                try
                {
                    WebView2Common.NavigateToUri(_wv2_Markup, WebView2Common.GetTestPageUri("SimplePage.html"));
                }
                catch (Exception e)
                {
                    SetClosedExceptionCount(_closedExceptionCount + 1);

                    string s = string.Format("{0} Caught exception: {1}", _wv2_Markup.Name, e.ToString());
                    _helpers.AppendMessage(s);
                    Status2.Text = Status2.Text + " " + s;
                }
            }
        }

        public async void OnEnsureCoreWebView2_MarkupElement_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_Markup != null)
            {
                try
                {
                    await _wv2_Markup.EnsureCoreWebView2Async();
                    {
                        SetEnsureCoreWebView2CompletionCount(_ensureCoreWebView2CompletionCount + 1);

                        string s = string.Format("[{0}]: Got EnsureCWV2 Completion, ", _wv2_Markup.Name);
                        _helpers.AppendMessage(s);
                        Status2.Text = Status2.Text + " " + s;
                    }
                }
                catch (Exception e)
                {
                    SetClosedExceptionCount(_closedExceptionCount + 1);

                    string s = string.Format("{0} Caught exception: {1}", _wv2_Markup.Name, e.ToString());
                    _helpers.AppendMessage(s);
                    Status2.Text = Status2.Text + " " + s;
                }
            }
        }

        public void OnRemove_MarkupElement_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_Markup != null)
            {
                WebView2_Container3.Child = null;
            }
        }

        public void OnClose_MarkupElement_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_Markup != null)
            {
                _wv2_Markup.Close();
            }
        }

        public void OnSetCustomSource_MarkupElement_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_Markup != null)
            {
                try
                {
                    Uri newUri;
                    string newUriString = CustomSource_UriTextBox.Text;
                    Uri.TryCreate(newUriString, UriKind.RelativeOrAbsolute, out newUri);
                    _wv2_Markup.Source = newUri;
                }
                catch (Exception e)
                {
                    SetClosedExceptionCount(_closedExceptionCount + 1);

                    string s = string.Format("{0} Caught exception: {1}", _wv2_Markup.Name, e.ToString());
                    _helpers.AppendMessage(s);
                    Status2.Text = Status2.Text + " " + s;
                }
            }
        }


        public void OnCreate_ConcurrentCreationElement_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_ConcurrentCreation != null)
            {
                RemoveWebViewEventHandlers(_wv2_ConcurrentCreation);
                _wv2_ConcurrentCreation.Close();
            }

            var myWebView = new WebView2 {
                Name = "WV2_ConcurrentCreation"
            };
            AddWebViewEventHandlers(myWebView);
            _wv2_ConcurrentCreation = myWebView;
            WebView2_Container4.Child = _wv2_ConcurrentCreation;
        }

        // Kick off Source1 (trigger creation), then Source2 while 1 is in progress
        public void OnSource_Source_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_ConcurrentCreation != null)
            {
                // We'll set Source #2 in this special CWV2Initialized Handler
                _wv2_ConcurrentCreation.CoreWebView2Initialized += OnCoreWebView2Initialized_Source;

                // Set Source #1 and attach  special CoreWebView2Initialized handler to apply Source #2 at that time
                WebView2Common.NavigateToUri(_wv2_ConcurrentCreation, WebView2Common.GetTestPageUri("SimplePage.html"));
            }
        }

        public void OnSource_Ensure_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_ConcurrentCreation != null)
            {
                // We'll call EnsureCWV2() in this special CWV2Initialized Handler
                _wv2_ConcurrentCreation.CoreWebView2Initialized += OnCoreWebView2Initialized_Ensure;

                // Set Source #1
                WebView2Common.NavigateToUri(_wv2_ConcurrentCreation, WebView2Common.GetTestPageUri("SimplePage.html"));
            }
        }

        public async void OnEnsure_Source_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_ConcurrentCreation != null)
            {
                // We'll set Source in this special CWV2Initialized Handler
                _wv2_ConcurrentCreation.CoreWebView2Initialized += OnCoreWebView2Initialized_Source;

                // EnsureCWV2() 
                await _wv2_ConcurrentCreation.EnsureCoreWebView2Async();
                {
                    SetEnsureCoreWebView2CompletionCount(_ensureCoreWebView2CompletionCount + 1);

                    string s = string.Format("[{0}]: Got EnsureCWV2 Completion, ", _wv2_ConcurrentCreation.Name);
                    _helpers.AppendMessage(s);
                    Status2.Text = Status2.Text + " " + s;
                }
            }
        }

        public async void OnEnsure_Ensure_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_ConcurrentCreation != null)
            {
                // We'll call EnsureCWV2() (#2) in this special CWV2Initialized Handler
                _wv2_ConcurrentCreation.CoreWebView2Initialized += OnCoreWebView2Initialized_Ensure;

                // EnsureCWV2() 
                await _wv2_ConcurrentCreation.EnsureCoreWebView2Async();
                {
                    SetEnsureCoreWebView2CompletionCount(_ensureCoreWebView2CompletionCount + 1);

                    string s = string.Format("[{0}]: Got EnsureCWV2 Completion, ", _wv2_ConcurrentCreation.Name);
                    _helpers.AppendMessage(s);
                    Status2.Text = Status2.Text + " " + s;
                }
            }
        }

        public void OnRemove_ConcurrentCreationElement_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_ConcurrentCreation != null)
            {
                WebView2_Container4.Child = null;
            }
        }

        public void OnClose_ConcurrentCreationElement_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_ConcurrentCreation != null)
            {
                _wv2_ConcurrentCreation.Close();
            }
        }
        public void OnCreate_CoreProcessFailed_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_CoreProcessFailed != null)
            {
                RemoveWebViewEventHandlers(_wv2_CoreProcessFailed);
                _wv2_CoreProcessFailed.Close();
            }

            var myWebView = new WebView2 {
                Name = "WV2_CoreProcessFailed"
            };
            AddWebViewEventHandlers(myWebView);
            _wv2_CoreProcessFailed = myWebView;
            WebView2_Container5.Child = _wv2_CoreProcessFailed;
        }

        public void OnBad_Source_Browser_ButtonClicked(object sender, RoutedEventArgs args)
        {
            // Crashes the browser - recover by re-creating CoreWebView2
            OnBadSourceButtonClickedHelper("edge://inducebrowsercrashforrealz/");
        }

        public void OnBad_Source_Render_ButtonClicked(object sender, RoutedEventArgs args)
        {
            // Crashes the current rendering process - recover with Reload()
            OnBadSourceButtonClickedHelper("edge://crash");
        }

        private void OnBadSourceButtonClickedHelper(string badUri)
        {
            if (_wv2_CoreProcessFailed != null)
            {
                try
                {
                    WebView2Common.NavigateToUri(_wv2_CoreProcessFailed, new Uri(badUri));
                }
                catch (Exception e)
                {
                    SetClosedExceptionCount(_closedExceptionCount + 1);

                    string s = string.Format("{0} Caught exception: {1}", _wv2_CoreProcessFailed.Name, e.ToString());
                    _helpers.AppendMessage(s);
                    Status2.Text = Status2.Text + " " + s;
                }
            }
        }

        public void OnGood_Source_ButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_CoreProcessFailed != null)
            {
                try
                {
                    WebView2Common.NavigateToUri(_wv2_CoreProcessFailed, WebView2Common.GetTestPageUri("SimplePage.html"));
                }
                catch (Exception e)
                {
                    SetClosedExceptionCount(_closedExceptionCount + 1);

                    string s = string.Format("{0} Caught exception: {1}", _wv2_CoreProcessFailed.Name, e.ToString());
                    _helpers.AppendMessage(s);
                    Status2.Text = Status2.Text + " " + s;
                }
            }
        }

        private void OnReloadButtonClicked(object sender, RoutedEventArgs args)
        {
            if (_wv2_CoreProcessFailed != null)
            {
                try
                {
                    _wv2_CoreProcessFailed.Reload();
                }
                catch (Exception e)
                {
                    SetClosedExceptionCount(_closedExceptionCount + 1);

                    string s = string.Format("{0} Caught exception: {1}", _wv2_CoreProcessFailed.Name, e.ToString());
                    _helpers.AppendMessage(s);
                    Status2.Text = Status2.Text + " " + s;
                }
            }
        }


        // Ensure destruction and cleanup (including UIA tree of web content) of all webviews
        //
        // NOTE: We need a small wait a bit before completing this test, otherwise WebView2 tests were failing sporadically in CatGats.
        // This is because the UIA tree from the browser HWND does not get disconnected synchronously on WebView2 Element destruction,
        // and its presence after leaving the test page prevents the test runner from activating (also via UIA) the next test.
        // 
        // TODO_WebView2: Work with Anaheim to provide a "Disconnect" method on CoreWebView2 that syncrhonously removes web UIA tree. 
        private async Task CleanupWebViewElements()
        {
            RemoveAllWebViewControls();
            GC.Collect();
            await Task.Delay(3000);
        }

        void RemoveAllWebViewControls()
        {
            if (_wv2_Programmatic_Offline != null)
            {
                RemoveWebViewEventHandlers(_wv2_Programmatic_Offline);
                _wv2_Programmatic_Offline.Close();
                WebView2_Container1.Child = null;
            }

            if (_wv2_Programmatic_Live != null)
            {
                RemoveWebViewEventHandlers(_wv2_Programmatic_Live);
                _wv2_Programmatic_Live.Close();
                WebView2_Container2.Child = null;
            }

            if (_wv2_Markup != null)
            {
                RemoveWebViewEventHandlers(_wv2_Markup);
                _wv2_Markup.Close();
                WebView2_Container3.Child = null;
            }

            if (_wv2_ConcurrentCreation != null)
            {
                RemoveWebViewEventHandlers(_wv2_ConcurrentCreation);
                _wv2_ConcurrentCreation.Close();
                WebView2_Container4.Child = null;
            }

            _areWebviewElementsCleanedUp = true;
        }

        async public void CleanupCurrentTest(object sender, RoutedEventArgs args)
        {
            // Ensure any WebViewElements are destroyed + removed from UIA tree prior to signaling test completion 
            // to avoid impacting next test.
            await CleanupWebViewElements();
            CleanupResultTextBox.Text = "Cleanup completed.";
        }

        protected override void OnNavigatingFrom(NavigatingCancelEventArgs e)
        {
            // For test stability, webviews should be fully cleaned up before we leave the test page.
            Debug.Assert(_areWebviewElementsCleanedUp);
        }
    }
}
