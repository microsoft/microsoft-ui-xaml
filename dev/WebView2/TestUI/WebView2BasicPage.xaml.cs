// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.Globalization;
using Windows.System;
using Windows.UI;
using Windows.UI.Text;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

#if !BUILD_WINDOWS
using Popup = Windows.UI.Xaml.Controls.Primitives.Popup;
using WebView2 = Microsoft.UI.Xaml.Controls.WebView2;
using CoreWebView2NavigationStartingEventArgs = Microsoft.Web.WebView2.Core.CoreWebView2NavigationStartingEventArgs;
using CoreWebView2NavigationCompletedEventArgs = Microsoft.Web.WebView2.Core.CoreWebView2NavigationCompletedEventArgs;
using CoreWebView2WebMessageReceivedEventArgs = Microsoft.Web.WebView2.Core.CoreWebView2WebMessageReceivedEventArgs;
using CoreWebView2ProcessFailedEventArgs = Microsoft.Web.WebView2.Core.CoreWebView2ProcessFailedEventArgs;
#endif

namespace MUXControlsTestApp
{
    public class EnumToStringConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            string EnumString;
            try
            {
                EnumString = Enum.GetName((value.GetType()), value);
                return EnumString;
            }
            catch
            {
                return string.Empty;
            }
        }

        // No need to implement converting back on a one-way binding 
        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }

    public class UriToStringConverter : IValueConverter
    {
        // TextBlock can display Uris without conversion.
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            return value;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            Uri newUri;
            string uriString = value as string;
            try
            {
                Uri.TryCreate(uriString, UriKind.RelativeOrAbsolute, out newUri);
                return newUri;
            }
            catch
            {
                return null;
            }
        }
    }

    static class WebView2Extensions
    {
        public static Task<CoreWebView2NavigationCompletedEventArgs> GetNavigationCompletedTask(this WebView2 wv2)
        {
            var tcs = new TaskCompletionSource<CoreWebView2NavigationCompletedEventArgs>();
            TypedEventHandler<WebView2, CoreWebView2NavigationCompletedEventArgs> handler = null;

            handler = (WebView2 sender, CoreWebView2NavigationCompletedEventArgs e) =>
            {
                tcs.SetResult(e);
                wv2.NavigationCompleted -= handler;
            };

            wv2.NavigationCompleted += handler;

            return tcs.Task;
        }

        public static Task<CoreWebView2NavigationStartingEventArgs> GetNavigationStartingTask(this WebView2 wv2)
        {
            var tcs = new TaskCompletionSource<CoreWebView2NavigationStartingEventArgs>();
            TypedEventHandler<WebView2, CoreWebView2NavigationStartingEventArgs> handler = null;

            handler = (WebView2 sender, CoreWebView2NavigationStartingEventArgs e) =>
            {
                tcs.SetResult(e);
                wv2.NavigationStarting -= handler;
            };

            wv2.NavigationStarting += handler;

            return tcs.Task;
        }

        public static Task<CoreWebView2InitializedEventArgs> GetCoreWebView2InitializedTask(this WebView2 wv2)
        {
            var tcs = new TaskCompletionSource<CoreWebView2InitializedEventArgs>();
            TypedEventHandler<WebView2, CoreWebView2InitializedEventArgs> handler = null;

            handler = (WebView2 sender, CoreWebView2InitializedEventArgs e) =>
            {
                tcs.SetResult(e);
                wv2.CoreWebView2Initialized -= handler;
            };

            wv2.CoreWebView2Initialized += handler;
            
            return tcs.Task;
        }
    }

    public class WebView2WithCursor : WebView2
    {
        public Windows.UI.Core.CoreCursor WrappedProtectedCursor => Windows.UI.Core.CoreWindow.GetForCurrentThread().PointerCursor;
    }

    public sealed partial class WebView2BasicPage : TestPage
    {
        enum TestList
        {
            BasicRenderingTest,
            MouseLeftClickTest,
            MouseMiddleClickTest,
            MouseRightClickTest,
            MouseXButton1ClickTest,
            MouseXButton2ClickTest,
            MouseWheelScrollTest,
            NavigationErrorTest,
            Focus_BasicTabTest,
            Focus_ReverseTabTest,
            Focus_BackAndForthTabTest,
            Focus_MouseActivateTest,
            ExecuteScriptTest,
            MultipleWebviews_BasicRenderingTest,
            MultipleWebviews_FocusTest,
            MultipleWebviews_LanguageTest,
            CopyPasteTest,
            BasicKeyboardTest,
            WebMessageReceivedTest,
            MouseCaptureTest,
            ReloadTest,
            NavigateToStringTest,
            SourceBindingTest,
            GoBackAndForwardTest,
            NavigationStartingTest,
            NavigationStartingInvalidTest,
            ResizeTest,
            BasicTapTouchTest,
            BasicFlingTouchTest,
            BasicStretchTouchTest,
            BasicPanTouchTest,
            BasicLongPressTouchTest,
            ScaledTouchTest,
            MoveTest,
            ReparentElementTest,
            SourceBeforeLoadTest,
            VisibilityHiddenTest,
            VisibilityTurnedOnTest,
            ParentVisibilityHiddenTest,
            ParentVisibilityTurnedOnTest,
            SpecificTouchTest,
            CursorUpdateTest,
            CursorClickUpdateTest,
            WebView2CleanedUpTest,
            QueryCoreWebView2BasicTest,
            CoreWebView2InitializedTest,
            CoreWebView2Initialized_FailedTest,
            WindowHiddenTest,
            WindowlessPopupTest,
            PointerReleaseWithoutPressTest,
            HostNameToFolderMappingTest,
            NavigateToVideoTest,
            NavigateToLocalImageTest,
            CloseThenDPIChangeTest,
            AddHostObjectToScriptTest,
            UserAgentTest,
            NonAsciiUriTest,
            OffTreeWebViewInputTest,
            HtmlDropdownTest,
            HiddenThenVisibleTest,
            ParentHiddenThenVisibleTest,
            LifetimeTabTest,
        };

        // Map of TestList entry to its webpage (index in TestPageNames[])
        readonly Dictionary<TestList, int> TestInfoDictionary = new Dictionary<TestList, int>()
        {
            { TestList.BasicRenderingTest, 0 },
            { TestList.MouseLeftClickTest, 1 },
            { TestList.MouseMiddleClickTest, 1 },
            { TestList.MouseRightClickTest, 1 },
            { TestList.MouseXButton1ClickTest, 1 },
            { TestList.MouseXButton2ClickTest, 1 },
            { TestList.MouseWheelScrollTest, 2 },
            { TestList.NavigationErrorTest, 0 },
            { TestList.Focus_BasicTabTest, 3 },
            { TestList.Focus_ReverseTabTest, 3 },
            { TestList.Focus_BackAndForthTabTest, 3 },
            { TestList.Focus_MouseActivateTest, 3 },
            { TestList.ExecuteScriptTest, 4 },
            { TestList.MultipleWebviews_BasicRenderingTest, 0 },
            { TestList.MultipleWebviews_FocusTest, 3 },
            { TestList.MultipleWebviews_LanguageTest, 0 },
            { TestList.CopyPasteTest, 3 },
            { TestList.BasicKeyboardTest, 5 },
            { TestList.WebMessageReceivedTest, 5 },
            { TestList.MouseCaptureTest, 5 },
            { TestList.ReloadTest, 5 },
            { TestList.NavigateToStringTest, 4 },
            { TestList.SourceBindingTest, 5 },
            { TestList.GoBackAndForwardTest, 0 },
            { TestList.NavigationStartingTest, 0 },
            { TestList.NavigationStartingInvalidTest, 4 },
            { TestList.ResizeTest, 0 },
            { TestList.BasicTapTouchTest, 2 },
            { TestList.BasicFlingTouchTest, 2 },
            { TestList.BasicStretchTouchTest, 2 },
            { TestList.BasicPanTouchTest, 2 },
            { TestList.BasicLongPressTouchTest, 2 },
            { TestList.ScaledTouchTest, 2 },
            { TestList.MoveTest, 0 },
            { TestList.ReparentElementTest, 0 },
            { TestList.SourceBeforeLoadTest, 0 },
            { TestList.VisibilityHiddenTest, 3 },
            { TestList.VisibilityTurnedOnTest, 3 },
            { TestList.ParentVisibilityHiddenTest, 3 },
            { TestList.ParentVisibilityTurnedOnTest, 3 },
            { TestList.SpecificTouchTest, 6 },
            { TestList.CursorUpdateTest, 1 },
            { TestList.CursorClickUpdateTest, 1 },
            { TestList.WebView2CleanedUpTest, 1 },
            { TestList.QueryCoreWebView2BasicTest, 0 },
            { TestList.CoreWebView2InitializedTest, 0 },
            { TestList.CoreWebView2Initialized_FailedTest, 0 },
            { TestList.WindowHiddenTest, 3 },
            { TestList.WindowlessPopupTest, 1 },
            { TestList.PointerReleaseWithoutPressTest, 1 },
            { TestList.HostNameToFolderMappingTest, 0 },
            { TestList.NavigateToVideoTest, 4 },
            { TestList.NavigateToLocalImageTest, 0 },
            { TestList.CloseThenDPIChangeTest, 0 },
            { TestList.AddHostObjectToScriptTest, 0 },
            { TestList.UserAgentTest, 0 },
            { TestList.NonAsciiUriTest, 7 },
            { TestList.OffTreeWebViewInputTest, 1 },
            { TestList.HtmlDropdownTest, 5 },
            { TestList.HiddenThenVisibleTest, 1 },
            { TestList.ParentHiddenThenVisibleTest, 1 },
            { TestList.LifetimeTabTest, 0 },
        };

        readonly string[] TestPageNames =
        {
            "SimplePage.html",
            "SimplePageWithButton.html",
            "SimplePageWithScrollableColoredBlocks.html",
            "SimplePageForFocus.html",
            "SimplePageWithText.html",
            "SimpleInputPage.html",
            "SimplePageWithManyButtons.html",
            "SimplePageWithNonÅscií.html",
        };

        readonly WebView2Common _helpers;

        bool _isUserInitiated = false;
        bool _isRedirected = false;
        bool _areWebviewElementsCleanedUp = false;
        bool _isApplicationLanguageOverrideSet = false;

        double _originalHeight;
        double _originalWidth;

        public WebView2BasicPage()
        {
            InitializeComponent();
            _helpers = new WebView2Common(this);
            _helpers.CopyTestPagesLocally();

            // setup first webview and tabstops in the right order
            var tabStop1 = new Button()
            {
                Name = "TabStopButton1",
                Content = "[x1] TabStop1"
            };
            AutomationProperties.SetName(tabStop1, "TabStopButton1");
            tabStop1.GotFocus += (sender, args) =>
            {
                focusLog.Text += "Focus on: [x1]" + "\n";
                focusScrollViewer.ChangeView(null, focusScrollViewer.ScrollableHeight, null);
            };
            var tabStop2 = new Button()
            {
                Name = "TabStopButton2",
                Content = "[x2] TabStop2"
            };
            AutomationProperties.SetName(tabStop2, "TabStopButton2");
            tabStop2.GotFocus += (sender, args) =>
            {
                focusLog.Text += "Focus on: [x2]" + "\n";
                focusScrollViewer.ChangeView(null, focusScrollViewer.ScrollableHeight, null);
            };

            AddWebViewControl("MyWebView2");
            // adding tabstop buttons x1 and x2 before and after default webview and its title, respectively
            var defaultStackPanel = WebView2Collection.Children[0] as StackPanel;
            var titleStackPanel = defaultStackPanel.Children[0] as StackPanel;
            titleStackPanel.Children.Add(tabStop1);
            defaultStackPanel.Children.Add(tabStop2);
        }

        public void OnSetSourceButtonClicked(object sender, RoutedEventArgs args)
        {
            ClearStatus();
            string newUriString = UriTextBox.Text;
            foreach (var unit in WebView2Collection.Children)
            {
                var webviewStackPanel = unit as StackPanel;
                if (webviewStackPanel != null)
                {
                    string stackPanelName = webviewStackPanel.Name;
                    string webviewName = stackPanelName.Replace("StackPanel", ""); // as per convention, stackpanel name is foowebviewStackPanel
                    var toggleBox = FindName(webviewName + "Toggle") as ToggleSwitch;
                    Debug.Assert(toggleBox != null);
                    if (toggleBox.IsOn)
                    {
                        var webview = FindName(webviewName) as WebView2;
                        Debug.Assert(webview != null);
                        WebView2Common.NavigateToUri(webview, newUriString);
                    }
                }
            }
        }

        public async void OnEnsureCWV2ButtonClicked(object sender, RoutedEventArgs args)
        {
            ClearStatus();
            foreach (var unit in WebView2Collection.Children)
            {
                var webviewStackPanel = unit as StackPanel;
                if (webviewStackPanel != null)
                {
                    string stackPanelName = webviewStackPanel.Name;
                    string webviewName = stackPanelName.Replace("StackPanel", ""); // as per convention, stackpanel name is foowebviewStackPanel
                    var toggleBox = FindName(webviewName + "Toggle") as ToggleSwitch;
                    Debug.Assert(toggleBox != null);
                    if (toggleBox.IsOn)
                    {
                        var webview = FindName(webviewName) as WebView2;
                        Debug.Assert(webview != null);
                        await webview.EnsureCoreWebView2Async();
                        Status1.Text = "EnsureCoreWebView2Async() completed";
                    }
                }
            }
        }


        public void OnAddWebviewButtonClicked(object sender, RoutedEventArgs args)
        {
            string prefixName = "MyWebView2";
            var numWebViews = WebView2Collection.Children.Count;
            char suffix = (char)('A' + numWebViews + 1); // choosing between A-Z based on existing webviews
            string finalName = prefixName + suffix;
            AddWebViewControl(finalName);
        }

        public void OnNavigateToStringButtonClicked(object sender, RoutedEventArgs args)
        {
            WebView2Common.NavigateToStringMessage(FindName("MyWebView2") as WebView2);
        }

        public void OnVideoButtonClicked(object sender, RoutedEventArgs args)
        {
            NavigateToVideo(FindName("MyWebView2") as WebView2);
        }

        public void OnGoBackButtonClicked(object sender, RoutedEventArgs args)
        {
            var MyWebView2 = FindName("MyWebView2") as WebView2;
            MyWebView2.GoBack();
        }

        public void OnGoForwardButtonClicked(object sender, RoutedEventArgs args)
        {
            var MyWebView2 = FindName("MyWebView2") as WebView2;
            MyWebView2.GoForward();
        }

        public string GetDocumentTitle(WebView2 wv2)
        {
            string documentTitle = string.Empty;

            var core_wv2 = wv2.CoreWebView2;
            if (core_wv2 != null)
            {
                documentTitle = core_wv2.DocumentTitle;
            }

            var DocumentTitleTextBlock = FindName("DocumentTitleTextBlock") as TextBlock;
            DocumentTitleTextBlock.Text = documentTitle;

            return documentTitle;
        }

        public void OnGetDocumentTitleButtonClicked(object sender, RoutedEventArgs args)
        {
            GetDocumentTitle(FindName("MyWebView2") as WebView2);
        }

        void UpdateSourceOnEnter(object sender, KeyRoutedEventArgs args)
        {
            if (args.Key == VirtualKey.Enter)
            {
                var ActualUriTextBlock = FindName("ActualUriTextBlock") as TextBlock;
                ActualUriTextBlock.Text = UriTextBox.Text;
            }
        }

        // By design, NavigateToString() doesn't work if the HTML has <video> specified with a
        // local URI, so SetVirtualHostNameToFolderMapping() is used to work around this limitation.
        public static void NavigateToVideo(WebView2 webview)
        {
            var core_wv2 = webview.CoreWebView2;
            if (core_wv2 != null)
            {
                core_wv2.SetVirtualHostNameToFolderMapping(
                    "appassets.example", "assets",
                    Microsoft.Web.WebView2.Core.CoreWebView2HostResourceAccessKind.Allow);

                string uriString = "https://appassets.example/CastingVideo.mp4";
                WebView2Common.NavigateToUri(webview, uriString);
            }
        }

        public void TransformWebView(object sender, RoutedEventArgs args)
        {
            var MyWebView2 = FindName("MyWebView2") as WebView2;
            MyWebView2.RenderTransformOrigin = new Windows.Foundation.Point(0, 0);
            var rotateTransform = new RotateTransform { Angle = 45 };
            var scaleTransform = new ScaleTransform { ScaleX = 1.2, ScaleY = 1.2 };
            TransformGroup transformGroup = new TransformGroup();
            transformGroup.Children.Add(rotateTransform);
            transformGroup.Children.Add(scaleTransform);
            MyWebView2.RenderTransform = transformGroup;

            RotateScaleButton.Content = "Reset";
        }

        public void ResetTransformWebView(object sender, RoutedEventArgs args)
        {
            var MyWebView2 = FindName("MyWebView2") as WebView2;
            MyWebView2.RenderTransformOrigin = new Windows.Foundation.Point(0, 0);
            var rotateTransform = new RotateTransform { Angle = 0 };
            var scaleTransform = new ScaleTransform { ScaleX = 1, ScaleY = 1 };
            TransformGroup transformGroup = new TransformGroup();
            transformGroup.Children.Add(rotateTransform);
            transformGroup.Children.Add(scaleTransform);
            MyWebView2.RenderTransform = transformGroup;

            RotateScaleButton.Content = "Rotate and Scale";
        }

        public void OnResizeWebViewVisualButtonClicked(object sender, RoutedEventArgs args)
        {
            var MyWebView2 = FindName("MyWebView2") as WebView2;
            _originalWidth = MyWebView2.ActualWidth;
            _originalHeight = MyWebView2.ActualHeight;
            MyWebView2.Width = MyWebView2.ActualWidth / 2;
            MyWebView2.Height = MyWebView2.ActualHeight / 2;

            MyWebView2.UpdateLayout();

            ResizeWebViewVisualButton.Content = "Reset";
        }

        public void ResetWebViewVisual(object sender, RoutedEventArgs args)
        {
            var MyWebView2 = FindName("MyWebView2") as WebView2;
            MyWebView2.Width = _originalWidth;
            MyWebView2.Height = _originalHeight;
            MyWebView2.UpdateLayout();

            ResizeWebViewVisualButton.Content = "Resize visual";
        }

        public void OnToggleWebViewVisibilityButtonClicked(object sender, RoutedEventArgs args)
        {
            var MyWebView2 = FindName("MyWebView2") as WebView2;
            MyWebView2.Visibility = Visibility.Collapsed;

            ToggleWebViewVisibilityButton.Content = "Reset";
        }

        public void ResetWebViewVisibility(object sender, RoutedEventArgs args)
        {
            var MyWebView2 = FindName("MyWebView2") as WebView2;
            MyWebView2.Visibility = Visibility.Visible;

            ToggleWebViewVisibilityButton.Content = "Hide WebView";
        }

        public void ClearStatus()
        {
            _helpers.ClearStatus();
            var messageReceivedBox = FindName("MessageReceived") as CheckBox;
            messageReceivedBox.IsChecked = false;
            AnaheimFocusTextBox.Text = string.Empty;
        }

        private void TestNameComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            // if there are more webviews than just default one, remove them
            if (WebView2Collection.Children.Count > 1)
            {
                RemoveWebViewControls(true /* keep default webview */);
            }

            var MyWebView2 = FindName("MyWebView2") as WebView2;
            TestList test = (TestList)Enum.Parse(typeof(TestList), TestNameComboBox.GetSelectedText());
            switch (test)
            {
                case TestList.MultipleWebviews_BasicRenderingTest:
                    {
                        AddWebViewControl("MyWebView2B");
                        AddWebViewControl("MyWebView2C");
                        WebView2Common.LoadWebPage(MyWebView2, TestPageNames[TestInfoDictionary[test]]);
                    }
                    break;
                case TestList.MultipleWebviews_FocusTest:
                    {
                        AddWebViewControl("MyWebView2B");
                        // both webviews will load different webpages
                        WebView2Common.LoadWebPage(MyWebView2, TestPageNames[TestInfoDictionary[test]]);
                        WebView2Common.LoadWebPage((FindName("MyWebView2B") as WebView2), TestPageNames[0]);
                    }
                    break;
                case TestList.MultipleWebviews_LanguageTest:
                    {
                        // Set Application Language override to zh-Hans - create new WV2 
                        ApplicationLanguages.PrimaryLanguageOverride = "zh-Hans";
                        _isApplicationLanguageOverrideSet = true;
                        WebView2Common.LoadWebPage(MyWebView2, TestPageNames[TestInfoDictionary[test]]);

                        // TODO_WebView2: When we expose API to set custom UserData, update the 2nd WebView to use a different language 
                        //                (same UserData implies same browser instance and therefore requires same Language)
                        AddWebViewControl("MyWebView2B");
                        StackPanel MyWebView2B_Ancestor = FindName("MyWebView2BStackPanel") as StackPanel;
                        WebView2Common.LoadWebPage((FindName("MyWebView2B") as WebView2), TestPageNames[TestInfoDictionary[test]]);
                    }
                    break;
                case TestList.SourceBeforeLoadTest:
                    {
                        // Remove existing WebView, we're going to replace it
                        Border parentBorder = RemoveExistingWebViewControl(MyWebView2);

                        Uri uri = WebView2Common.GetTestPageUri("SimplePageWithButton.html");
                        var newWebView2 = new WebView2() {
                            Name = "MyWebView2",
                            Margin = new Thickness(8, 8, 8, 8),
                            // Set the source before we put the webview in the tree
                            Source = uri
                        };
                        AutomationProperties.SetName(newWebView2, "MyWebView2");
                        parentBorder.Child = newWebView2;
                        AddWebViewEventHandlers(newWebView2);
                    }
                    break;
                case TestList.VisibilityTurnedOnTest:
                    {
                        MyWebView2.Visibility = Visibility.Collapsed;
                        WebView2Common.LoadWebPage(MyWebView2, TestPageNames[TestInfoDictionary[test]]);
                    }
                    break;
                case TestList.ParentVisibilityTurnedOnTest:
                    {
                        var testContentsBorder = FindName("TestContentsBorder") as Border;
                        testContentsBorder.Visibility = Visibility.Collapsed;
                        WebView2Common.LoadWebPage(MyWebView2, TestPageNames[TestInfoDictionary[test]]);
                    }
                    break;
                case TestList.QueryCoreWebView2BasicTest:
                    {
                        // Defer 1st source set to CompleteCurrentTest to avoid race conditions
                    }
                    break;
                case TestList.CoreWebView2InitializedTest:
                    {
                        // Defer 1st source set to CompleteCurrentTest to listen to event during CoreWV initialization
                    }
                    break;
                case TestList.CoreWebView2Initialized_FailedTest:
                    {
                        // Defer 1st source set to CompleteCurrentTest to listen to event during CoreWV initialization
                    }
                    break;
                case TestList.WindowlessPopupTest:
                    {
                        // Remove existing WebView so it doesn't get confused with the one in the popup
                        _ = RemoveExistingWebViewControl(MyWebView2);

                        // Create popup contents: a new webview
                        var webviewInPopup = new WebView2() {
                            Name = "popWebView2",
                            Margin = new Thickness(8, 8, 8, 8),
                            Height = 500,
                            Width = 500,
                        };
                        AutomationProperties.SetName(webviewInPopup, "popWebView2");
                        AddWebViewEventHandlers(webviewInPopup);

                        // Add a ToggleSwitch back in so that OnWebMessageReceived can find it
                        var toggleSwitch = new ToggleSwitch() 
                        {
                            Name = "popWebView2" + "Toggle",
                            IsOn = true,
                            OffContent = "Not selected",
                            OnContent = "Selected",
                            IsTabStop = false
                        };
                        var messageReceivedSP = (FindName("MessageReceived") as CheckBox).Parent as StackPanel;
                        messageReceivedSP.Children.Add(toggleSwitch);

                        // Create Popup
                        var popupOpened = new System.Threading.AutoResetEvent(false);
                        Popup popup = new Popup {
                            HorizontalOffset = 50,
                            VerticalOffset = 50,
                            Child = webviewInPopup,
                        };
                        popup.Opened += (sender, args) =>
                        {
                            popupOpened.Set();
                        };

                        // Place popup and open it
                        Border testContentsBorder = FindName("TestContentsBorder") as Border;
                        testContentsBorder.Child = popup;
                        if (!popup.IsOpen) { popup.IsOpen = true; }

                        // Wait up to 5 seconds for the popup to open
                        popupOpened.WaitOne(TimeSpan.FromSeconds(5));
                    }
                    break;
                case TestList.CursorUpdateTest:
                case TestList.CursorClickUpdateTest:
                    {
                        // Remove existing WebView, we're going to replace it
                        Border parentBorder = RemoveExistingWebViewControl(MyWebView2);

                        // Insert webview with public ProtectedCursor
                        var newWebView2 = new WebView2WithCursor() {
                            Name = "MyWebView2",
                            Margin = new Thickness(8, 8, 8, 8),
                        };
                        AutomationProperties.SetName(newWebView2, "MyWebView2");
                        parentBorder.Child = newWebView2;
                        AddWebViewEventHandlers(newWebView2);
                        WebView2Common.LoadWebPage(newWebView2, TestPageNames[TestInfoDictionary[test]]);
                    }
                    break;
                case TestList.NonAsciiUriTest:
                    {
                        // Put the URI with non-ascii characters in a TextBox, so we can easily copy/paste for manual testing.
                        var box = FindName("CopyPasteTextBox2") as TextBox;
                        string fileLocation = WebView2Common.GetTestPageUri("SimplePageWithNonÅscií.html").ToString();
                        string query = "?query=";
                        box.Text = fileLocation + query;

                        WebView2Common.LoadWebPage(MyWebView2, TestPageNames[TestInfoDictionary[test]]);
                    }
                    break;
                case TestList.OffTreeWebViewInputTest:
                    {
                        Border parentBorder = RemoveExistingWebViewControl(MyWebView2);
                        parentBorder.BorderBrush = new SolidColorBrush(Colors.Pink);
                    }
                    break;
                case TestList.HiddenThenVisibleTest:
                    {
                        // Remove existing WebView, we're going to replace it with a hidden one
                        Border parentBorder = RemoveExistingWebViewControl(MyWebView2);
                        var parentStackPanel = parentBorder.Parent as StackPanel;
                        parentStackPanel.Children.Remove(parentBorder);

                        // Create a new border that gets its size from the webview
                        var newBorder = new Border()
                        {
                            BorderBrush = new SolidColorBrush(Colors.Red),
                            BorderThickness = new Thickness(5)
                        };

                        // Add a new, collapsed WebView2 into the tree
                        var uri = WebView2Common.GetTestPageUri(TestPageNames[TestInfoDictionary[test]]);
                        var newWebView2 = new WebView2()
                        {
                            Name = "MyWebView2",
                            Margin = new Thickness(8, 8, 8, 8),
                            Width = 670,
                            Height = 370,
                            Source = uri,
                            Visibility = Visibility.Collapsed,
                        };
                        AutomationProperties.SetName(newWebView2, "MyWebView2");
                        newBorder.Child = newWebView2;
                        parentStackPanel.Children.Add(newBorder);
                        AddWebViewEventHandlers(newWebView2);
                    }
                    break;
                case TestList.ParentHiddenThenVisibleTest:
                    {
                        // Remove existing WebView, we're going to replace it with a hidden one
                        Border parentBorder = RemoveExistingWebViewControl(MyWebView2);
                        var parentStackPanel = parentBorder.Parent as StackPanel;
                        parentStackPanel.Children.Remove(parentBorder);

                        // Create a new, collapsed border that gets its size from the webview
                        var newBorder = new Border()
                        {
                            BorderBrush = new SolidColorBrush(Colors.Red),
                            BorderThickness = new Thickness(5),
                            Visibility = Visibility.Collapsed
                        };

                        // Add a new WebView2 into the tree
                        var uri = WebView2Common.GetTestPageUri(TestPageNames[TestInfoDictionary[test]]);
                        var newWebView2 = new WebView2()
                        {
                            Name = "MyWebView2",
                            Margin = new Thickness(8, 8, 8, 8),
                            Width = 670,
                            Height = 370,
                            Source = uri,
                            Visibility = Visibility.Visible,
                        };
                        AutomationProperties.SetName(newWebView2, "MyWebView2");
                        newBorder.Child = newWebView2;
                        parentStackPanel.Children.Add(newBorder);
                        AddWebViewEventHandlers(newWebView2);
                    }
                    break;
                default:
                    WebView2Common.LoadWebPage(MyWebView2, TestPageNames[TestInfoDictionary[test]]);
                    break;
            }
        }

        private void OnNavigationStarting(WebView2 sender, CoreWebView2NavigationStartingEventArgs args)
        {
            // Be careful if changing this message. The "NavigationStarting" string is expected
            // to be logged exactly once per NavigationStarting event by the NonAsciiUriTest.
            AppendMessage(string.Format("[{0}]: Got NavigationStarting ({1}).", sender.Name, args.Uri));

            string expectedUri = "http://www.blockedbynavigationstarting.invalid/";
            if (args.Uri.ToString() == expectedUri)
            {
                args.Cancel = true;
            }
            else
            {
                Status1.Text = sender.Name + " navigation starting...";
                Status3.Text = args.Uri;
                _isUserInitiated = args.IsUserInitiated;
                _isRedirected = args.IsRedirected;
            }
        }

        private void OnNavigationCompleted(WebView2 sender, CoreWebView2NavigationCompletedEventArgs args)
        {
            AppendMessage(string.Format("[{0}]: Got NavigationCompleted.", sender.Name));

            if (args.IsSuccess)
            {
                Status2.Text += string.Format("{0},", sender.Name); // indicating webview has loaded
            }
            else
            {
                HandleWebError(args);
            }
        }

        private static bool AreEqualIgnoringSpace(string string1, string string2)
        {
            string1 = string1.Replace(" ", "%20");
            string2 = string2.Replace(" ", "%20");
            return string1 == string2;
        }

        private void OnWebMessageReceived(WebView2 sender, CoreWebView2WebMessageReceivedEventArgs args)
        {
            try
            {
                AppendMessage(string.Format("[{0}]: Got WebMessageReceived ({1}).", sender.Name, args.TryGetWebMessageAsString()));
            }
            catch
            {
                AppendMessage(string.Format("[{0}]: Got WebMessageReceived ({1}).", sender.Name, string.Empty));
            }

            // Do not process message if webview is not enabled or source is not validated. 
            Status1.Text = string.Empty;
            Status2.Text = string.Empty;
            string webMessageAsString = string.Empty;
            string webMessageAsJson = string.Empty;

            // TODO_WebView2: Create wrapper WebView2Info with easy access to state like toggleswitch
            var toggleSwitch = FindName(sender.Name + "Toggle") as ToggleSwitch;
            if (AreEqualIgnoringSpace(args.Source, sender.Source.ToString()) && toggleSwitch.IsOn)
            {
                try
                {
                    webMessageAsString = args.TryGetWebMessageAsString();
                    if (IsFocusMessage(webMessageAsString))
                    {
                        AnaheimFocusTextBox.Text = webMessageAsString;
                        focusLog.Text += webMessageAsString + "\n";
                        focusScrollViewer.ChangeView(null, focusScrollViewer.ScrollableHeight, null);
                    }
                    else
                    {
                        Status1.Text = webMessageAsJson = args.WebMessageAsJson;
                        Status2.Text = webMessageAsString;
                    }
                }
                catch
                {
                    Status1.Text = webMessageAsJson = args.WebMessageAsJson;
                    Status2.Text = "Invalid string (try args.WebMessageAsJson instead)";
                }
            }

            if (!IsTransientWebMessage(webMessageAsString))
            {
                VerifyWebMessageBasedTest(webMessageAsString, webMessageAsJson);
            }

        }

        private static bool IsFocusMessage(string webMessageAsString)
        {
            return webMessageAsString.StartsWith("Focus on:") || webMessageAsString.StartsWith("Blur on:");
        }

        private void OnCoreProcessFailed(WebView2 sender, CoreWebView2ProcessFailedEventArgs args)
        {
            AppendMessage(string.Format("[{0}]: Got CoreProcessFailed ({1}).", sender.Name, args.ProcessFailedKind.ToString()));
        }

        private void OnCoreWebView2Initialized(WebView2 sender, CoreWebView2InitializedEventArgs args)
        {
            if (args.Exception != null)
            {
                AppendMessage(string.Format("[{0}]: Got CoreWebView2Initialized with exception ({1}).", sender.Name, args.Exception.ToString()));
            }
            else
            {
                AppendMessage(string.Format("[{0}]: Got CoreWebView2Initialized.", sender.Name));
            }
        } 

        void AddWebViewControl(string webviewName)
        {
            if (FindName(webviewName) != null)
            {
                Debug.Fail("Webview already exists by name " + webviewName);
                return;
            }

            var textBlock = new TextBlock() 
            {
                Text = webviewName,
                FontWeight = FontWeights.Bold,
                FontSize = 11,
                Margin = new Thickness(0, 2, 0, 2)
            };

            var toggleSwitch = new ToggleSwitch() 
            {
                Name = webviewName + "Toggle",
                IsOn = true,
                OffContent = "Not selected",
                OnContent = "Selected",
                IsTabStop = false
            };

            var stackPanelText = new StackPanel() 
            {
                Orientation = Orientation.Horizontal
            };
            stackPanelText.Children.Add(textBlock);
            stackPanelText.Children.Add(toggleSwitch);

            var webView2 = new WebView2()
            {
                Name = webviewName,
                Margin = new Thickness(8, 8, 8, 8)
            };
            AutomationProperties.SetName(webView2, webviewName);

            var border = new Border() 
            {
                Width = 670,
                Height = 370,
                BorderBrush = new SolidColorBrush(Colors.Blue),
                BorderThickness = new Thickness(1),
                Name = webviewName + "Border"
            };
            border.Child = webView2;

            Binding binding = new Binding()
            {
                Mode = BindingMode.TwoWay,
                Source = WebViewBorderWidthSlider,
                Path = new PropertyPath("Value")
            };
            border.SetBinding(Border.WidthProperty, binding);

            var stackPanel = new StackPanel() 
            {
                Name = webviewName + "StackPanel"  // makes it easy to get webview name by following this naming convention
            };
            stackPanel.Children.Add(stackPanelText);
            stackPanel.Children.Add(border);
            WebView2Collection.Children.Add(stackPanel);

            AddWebViewEventHandlers(webView2);
        }

        void MoveWebViewControl(string webviewName, WebView2 webView2)
        {
            var border = new Border()
            {
                Width = 670,
                Height = 370,
                BorderBrush = new SolidColorBrush(Colors.Pink),
                BorderThickness = new Thickness(1)
            };
            border.Child = webView2;
            
            var stackPanel = new StackPanel()
            {
                Name = webviewName + "StackPanel"
            };
            stackPanel.Children.Add(border);
            WebView2Collection.Children.Add(stackPanel);
        }

        Border RemoveExistingWebViewControl(WebView2 webview)
        {
            RemoveWebViewEventHandlers(webview);
            webview.Close();
            Border parentBorder = webview.Parent as Border;
            parentBorder.Child = null;
            return parentBorder;
        }

        void AddWebViewEventHandlers(WebView2 webview)
        {
            webview.NavigationStarting += OnNavigationStarting;
            webview.NavigationCompleted += OnNavigationCompleted;
            webview.WebMessageReceived += OnWebMessageReceived;
            webview.CoreProcessFailed += OnCoreProcessFailed;
            webview.CoreWebView2Initialized += OnCoreWebView2Initialized;
        }

        void RemoveWebViewEventHandlers(WebView2 webview)
        {
            webview.NavigationStarting -= OnNavigationStarting;
            webview.NavigationCompleted -= OnNavigationCompleted;
            webview.WebMessageReceived -= OnWebMessageReceived;
            webview.CoreProcessFailed -= OnCoreProcessFailed;
            webview.CoreWebView2Initialized -= OnCoreWebView2Initialized;
        }

        void RemoveWebViewControls(bool keepDefault = false)
        {
            int lastToRemove = keepDefault ? 1 : 0;
            for (int i = WebView2Collection.Children.Count - 1; i >= lastToRemove; i--)
            {
                StackPanel webviewStackPanel = WebView2Collection.Children[i] as StackPanel;
                Debug.Assert(webviewStackPanel != null);
                string stackPanelName = webviewStackPanel.Name;
                string webviewName = stackPanelName.Replace("StackPanel", ""); // as per convention, stackpanel name is foowebviewStackPanel
                var webview = FindName(webviewName) as WebView2;
                if (webview != null)
                {
                    RemoveWebViewEventHandlers(webview);
                    webview.Close();
                }

                WebView2Collection.Children.RemoveAt(i);
            }
        }

        // Ensure destruction and cleanup (including UIA tree of web content) of all webviews
        //
        // NOTE: We need a small wait before completing this test, otherwise WebView2 tests were failing sporadically.
        // This is because the UIA tree from the browser HWND does not get disconnected synchronously on WebView2 Element destruction,
        // and its presence after leaving the test page prevents the test runner from activating (also via UIA) the next test.
        // 
        // TODO_WebView2: Work with Anaheim to provide a "Disconnect" method on CoreWebView2 that synchronously removes web UIA tree. 
        private async Task CleanupWebViewElements()
        {
            RemoveWebViewControls();
            GC.Collect();
            await Task.Delay(3000);
            _areWebviewElementsCleanedUp = true;
        }

        // Handle specific error codes here
        void HandleWebError(CoreWebView2NavigationCompletedEventArgs args)
        {
            var errorStatus = args.WebErrorStatus;
            Status2.Text = errorStatus.ToString("G");
        }

        // Some tests receive multiple messages from WebMessageReceived, but we only care about the last one
        bool IsTransientWebMessage(string webMessageAsString)
        {
            TestList selectedTest = (TestList)Enum.Parse(typeof(TestList), TestNameComboBox.GetSelectedText());

            // The transient message each of these generates is the tap acknowledgment - "Tap received, zoom: 1"
            bool isTapWhenWaitingForSomethingElse = webMessageAsString == "Tap received, zoom: 1" && IsTransientTapTest(selectedTest);
            // Tests may receive a resize message when the webview is initially sized
            bool isInitialResize = webMessageAsString == "Webview resized." && selectedTest != TestList.ResizeTest;
            // Tests may receive a resize message when the webview is initially displayed on screen
            bool isInitialWindowVisible = webMessageAsString.StartsWith("Visibility state is now") && !IsVisiblityTest(selectedTest);

            return isTapWhenWaitingForSomethingElse || isInitialResize || isInitialWindowVisible || IsFocusMessage(webMessageAsString);
        }

        private static bool IsTransientTapTest(TestList selectedTest)
        {
            return selectedTest == TestList.BasicFlingTouchTest     ||
                   selectedTest == TestList.BasicPanTouchTest       ||
                   selectedTest == TestList.BasicLongPressTouchTest ||
                   selectedTest == TestList.BasicStretchTouchTest;
        }

        private static bool IsVisiblityTest(TestList selectedTest)
        {
            return selectedTest == TestList.WindowHiddenTest           ||
                   selectedTest == TestList.VisibilityHiddenTest       ||
                   selectedTest == TestList.VisibilityTurnedOnTest     ||
                   selectedTest == TestList.ParentVisibilityHiddenTest ||
                   selectedTest == TestList.ParentVisibilityTurnedOnTest;
        }

        public void VerifyWebMessageBasedTest(string stringResult, string jsonResult)
        {
            TestList selectedTest = (TestList)Enum.Parse(typeof(TestList), TestNameComboBox.GetSelectedText());
            using (var logger = new ResultsLogger(selectedTest.ToString(), TestResult))
            {
                string expectedStringResult = string.Empty;
                string expectedJsonResult = string.Empty;

                switch (selectedTest)
                {
                    case TestList.MouseLeftClickTest:
                        expectedStringResult = "Left mouse button clicked.";
                        break;

                    case TestList.MouseMiddleClickTest:
                        expectedStringResult = "Middle mouse button clicked.";
                        break;

                    case TestList.MouseRightClickTest:
                        expectedStringResult = "Right mouse button clicked.";
                        break;

                    case TestList.MouseXButton1ClickTest:
                        expectedStringResult = "First X mouse button clicked.";
                        break;

                    case TestList.MouseXButton2ClickTest:
                        expectedStringResult = "Second X mouse button clicked.";
                        break;

                    case TestList.MouseWheelScrollTest:
                        expectedStringResult = "End of page reached.";
                        break;

                    case TestList.WebMessageReceivedTest:
                        {
                             expectedStringResult = "Input button clicked.";
                             expectedJsonResult = "\"Input button clicked.\"";
                        }
                        break;

                    case TestList.ResizeTest:
                        expectedStringResult = "Webview resized.";
                        break;

                    case TestList.BasicTapTouchTest:
                        expectedStringResult = "Tap received, zoom: 1";
                        break;

                    case TestList.BasicFlingTouchTest:
                        expectedStringResult = "Far side of page reached.";
                        break;

                    case TestList.BasicStretchTouchTest:
                        {
                            string[] split = stringResult.Split(' ');
                            string zoom = split[split.Length - 1];
                            float zoomResultAsFloat = float.Parse(zoom);
                            logger.Verify((zoomResultAsFloat > 1.0),
                                          string.Format("Test {0}: Expected zoom value greater than 1.0, actual zoom was {1}",
                                                        selectedTest.ToString(), zoom));
                        }
                        break;

                    case TestList.BasicPanTouchTest:
                        expectedStringResult = "Far side of page reached.";
                        break;

                    case TestList.BasicLongPressTouchTest:
                        expectedStringResult = "Long press received.";
                        break;

                    case TestList.HostNameToFolderMappingTest:
                        expectedStringResult = "Left mouse button clicked.";
                        break;

                    case TestList.NavigateToLocalImageTest:
                        expectedStringResult = "Image loaded.";
                        break;

                    case TestList.ScaledTouchTest:
                        expectedStringResult = "Tap received, zoom: 1";
                        break;

                    case TestList.VisibilityHiddenTest:
                    case TestList.ParentVisibilityHiddenTest:
                        expectedStringResult = "Visibility state is now hidden";
                        break;

                    case TestList.VisibilityTurnedOnTest:
                    case TestList.ParentVisibilityTurnedOnTest:
                        {
                            // We start the test by hiding the WebView so ignore that message if it comes through
                            if (stringResult == "Visibility state is now hidden") return;
                            expectedStringResult = "Visibility state is now visible";
                        }
                        break;

                    case TestList.SpecificTouchTest:
                        {
                            expectedStringResult = "Received: 35";
                        }
                        break;

                    case TestList.WindowHiddenTest:
                        {
                            // We hide the window before showing it again, so ignore the first message
                            if (stringResult == "Visibility state is now hidden") return;
                            expectedStringResult = "Visibility state is now visible";
                        }
                        break;

                    case TestList.WindowlessPopupTest:
                        expectedStringResult = "Left mouse button clicked.";
                        break;

                    case TestList.AddHostObjectToScriptTest:
                        expectedStringResult = "Example property from host object";
                        break;

                    case TestList.OffTreeWebViewInputTest:
                        expectedStringResult = "Left mouse button clicked.";
                        break;

                    case TestList.HiddenThenVisibleTest:
                    case TestList.ParentHiddenThenVisibleTest:
                        expectedStringResult = "Left mouse button clicked.";
                        break;

                    default:
                        {
                            logger.LogError("Unexpected test type in VerifyWebMessageBasedTest(), got message: " + stringResult);
                            return;
                        }
                }

                bool allWebMessagesReceived = true;
                // Most tests just validate string equality - do that here. 
                // Tests doing other validation would do it elsewhere (leaving expectedStringResult/expectedJsonResult empty).
                if (expectedStringResult != string.Empty)
                {
                    logger.Verify((expectedStringResult == stringResult),
                                    string.Format("Test {0}: Expected string text {1} did not match with sampled string text {2}",
                                                selectedTest.ToString(), expectedStringResult, stringResult));
                    allWebMessagesReceived = allWebMessagesReceived && (expectedStringResult == stringResult);
                }

                if (expectedJsonResult != string.Empty)
                {
                    logger.Verify((expectedJsonResult == jsonResult),
                                    string.Format("Test {0}: Expected Json result {1} did not match with sampled Json result {2}",
                                                    selectedTest.ToString(), expectedJsonResult, jsonResult));
                    allWebMessagesReceived = allWebMessagesReceived && (expectedJsonResult == jsonResult);
                }

                // Explicitly check a box to show that the web message was received. CompleteCurrentTest() also logs
                // its pass/fail to the same place, so sometimes we can get false positives here if we're relying
                // on that method to do work before the web message is sent.
                if (allWebMessagesReceived)
                {
                    var messageReceivedBox = FindName("MessageReceived") as CheckBox;
                    messageReceivedBox.IsChecked = true;
                }
            }
        }

        // Apply appropriate verification based on test ID 
        async public void CompleteCurrentTest(object sender, RoutedEventArgs args)
        {
            var MyWebView2 = FindName("MyWebView2") as WebView2;
            TestList selectedTest = (TestList)Enum.Parse(typeof(TestList), TestNameComboBox.GetSelectedText());
            using (var logger = new ResultsLogger(selectedTest.ToString(), TestResult))
            {

                switch (selectedTest)
                {

                    case TestList.NavigationErrorTest:
                        {
                            var task = MyWebView2.GetNavigationCompletedTask();
                            MyWebView2.Source = new Uri("http://www.test.invalid");
                            await task;

                            var testStatus = Status2.Text;
                            bool result = testStatus.Equals("HostNameNotResolved");
                            logger.Verify(result,
                                          string.Format("Test {0}: Expected web error status {1} did not match with received error status {2}",
                                                        selectedTest, "HostNameNotResolved", testStatus));
                        }
                        break;

                    case TestList.ExecuteScriptTest:
                        {
                            string expectedResult = "1000";
                            string result = await MyWebView2.ExecuteScriptAsync("timer();"); // calling timer function which takes >1s to complete
                            logger.Verify(result == expectedResult,
                                          string.Format("Test {0}: Expected result {1} did not match with returned result {2}",
                                                        selectedTest, expectedResult.ToString(), result));
                        }
                        break;

                    case TestList.MultipleWebviews_BasicRenderingTest:
                        {
                            // Clear any other message that may be there
                            Status2.Text = string.Empty;
                            // wait for the other webviews to complete navigation before testing
                            var tasks = new List<Task>();
                            foreach (var unit in WebView2Collection.Children)
                            {
                                var webviewStackPanel = unit as StackPanel;
                                Debug.Assert(webviewStackPanel != null);
                                string stackPanelName = webviewStackPanel.Name;
                                string webviewName = stackPanelName.Replace("StackPanel", ""); // as per convention, stackpanel name is foowebviewStackPanel
                                var webview = FindName(webviewName) as WebView2;
                                Debug.Assert(webview != null);
                                tasks.Add(webview.GetNavigationCompletedTask());

                                WebView2Common.LoadWebPage(webview, TestPageNames[0]);
                                var parentOfWebview = webview.Parent as Border;
                                parentOfWebview.Height = 100;
                                parentOfWebview.UpdateLayout();
                            }
                            await Task.WhenAll(tasks);
                        }
                        break;

                    case TestList.MultipleWebviews_LanguageTest:
                        {
                            var tasks = new List<Task>();
                            
                            var wv1_name = "MyWebView2";
                            var wv1 = FindName(wv1_name) as WebView2;
                            tasks.Add(wv1.GetNavigationCompletedTask());
                            var wv2_name = "MyWebView2B";
                            var wv2 = FindName(wv2_name) as WebView2;
                            tasks.Add(wv2.GetNavigationCompletedTask());

                            wv1.Reload();
                            wv2.Reload();
                            await Task.WhenAll(tasks);
                            
                            string wv1_expectedLanguage = "\"zh-CN\"";
                            string wv1_language = await wv1.ExecuteScriptAsync("getLanguage();");
                            logger.Verify((wv1_language == wv1_expectedLanguage),
                                          string.Format("Test {0}: {1} Language {2} did not match expected value {3}",
                                                         selectedTest.ToString(), wv1_name, wv1_language, wv1_expectedLanguage));

                            // TODO_WebView2: When we expose API to set custom UserData, update the 2nd WebView to use a different language 
                            //                (same UserData implies same browser instance and therefore requires same Language)
                            
                            string wv2_expectedLanguage = "\"zh-CN\"";
                            string wv2_language = await wv2.ExecuteScriptAsync("getLanguage();");
                            logger.Verify((wv2_language == wv2_expectedLanguage),
                                          string.Format("Test {0}: {1} Language {2} did not match expected value {3}",
                                                         selectedTest.ToString(), wv2_name, wv2_language, wv2_expectedLanguage));
                        }
                        break;

                    /*
                        TestList.BasicKeyboardTest: Validated on testrunner side
                    */

                    case TestList.MouseCaptureTest:
                        {
                            string textResult = CopyPasteTextBox2.Text;
                            string expectedResult = "MouseCaptureResult";
                            logger.Verify((textResult == expectedResult),
                                           string.Format("Test {0}: Expected text {1} did not match with sampled text {2}",
                                                         selectedTest.ToString(), expectedResult, textResult));
                        }
                        break;

                    case TestList.ReloadTest:
                        {
                            Uri prevUri = MyWebView2.Source;
                            await MyWebView2.ExecuteScriptAsync("document.getElementById(\"w1\").value='injectedText';");
                            var task = MyWebView2.GetNavigationCompletedTask();
                            MyWebView2.Reload();
                            await task;
                            string finalTextContent = await MyWebView2.ExecuteScriptAsync("document.getElementById(\"w1\").value;");
                            logger.Verify((finalTextContent == "\"\""),
                                          string.Format("Test {0}: TextBox value was not cleared (\"\"), value: {1}",
                                                        selectedTest.ToString(), finalTextContent));
                            Uri newUri = MyWebView2.Source;
                            logger.Verify((prevUri.ToString() == newUri.ToString()),
                                          string.Format("Test {0}: Previous Uri {1} did not match the post-reload Uri {2}",
                                                        selectedTest.ToString(), prevUri.ToString(), newUri.ToString()));
                        }
                        break;

                    case TestList.NavigateToStringTest:
                        {
                            var task = MyWebView2.GetNavigationCompletedTask();
                            WebView2Common.NavigateToStringMessage(MyWebView2);
                            await task;

                            string expectedText = "\"You've navigated to a string message.\"";
                            string textResult = await MyWebView2.ExecuteScriptAsync("document.body.textContent;");
                            logger.Verify((expectedText == textResult),
                                          string.Format("Test {0}: Expected content: {1} did not match with sampled content: {2}",
                                                         selectedTest.ToString(), expectedText, textResult));
                            string expectedSourceUri = "about:blank";
                            string sourceUri = MyWebView2.Source.ToString();
                            logger.Verify((expectedText == textResult) && (expectedSourceUri == sourceUri),
                                          string.Format("Test {0}: Expected Uri: {1} did not match with sampled Uri: {2}",
                                                        selectedTest.ToString(), expectedSourceUri, sourceUri));
                        }
                        break;

                    case TestList.SourceBindingTest:
                        {
                            string expectedInitialUri = WebView2Common.GetTestPageUri("SimplePage.html").ToString();
                            var ActualUriTextBlock = FindName("ActualUriTextBlock") as TextBlock;
                            string initialActualSourceUri = ActualUriTextBlock.Text;
                            logger.Verify(AreEqualIgnoringSpace(expectedInitialUri, initialActualSourceUri),
                                          string.Format("Test {0}: Expected source bound text, {1}, did not match actual text binding {2}",
                                                       selectedTest.ToString(), expectedInitialUri, initialActualSourceUri));
                            var task = MyWebView2.GetNavigationCompletedTask();
                            _ = MyWebView2.ExecuteScriptAsync("navigateToTextPage();");
                            // Confirm that navigation to SimplePageWithText.html occurred, 
                            // meaning SimplePage intermediate was correctly loaded from textbox.
                            string expectedFinalSourceUri = WebView2Common.GetTestPageUri("SimplePageWithText.html").ToString();
                            await task;
                            string finalActualSourceUri = ActualUriTextBlock.Text;
                            logger.Verify(AreEqualIgnoringSpace(finalActualSourceUri, expectedFinalSourceUri),
                                          string.Format("Test {0}: Expected navigate to bound uri text {1} failed with current uri {2}",
                                                        selectedTest.ToString(), expectedFinalSourceUri, finalActualSourceUri));
                        }
                        break;

                    case TestList.GoBackAndForwardTest:
                        {
                            var goBackButton = FindName("GoBackButton") as Button;
                            var goForwardButton = FindName("GoForwardButton") as Button;
                            bool canGoBackBindingVerify = !goBackButton.IsEnabled; // Verify both buttons are initially disabled
                            bool canGoForwardBindingVerify = !goForwardButton.IsEnabled;
                            ActualUriTextBlock = FindName("ActualUriTextBlock") as TextBlock;

                            // Part 1: [SimplePage.html] calls  navigateToTextPage() -> Navigates to [SimplePageWithText.html].
                            //         Ensure we are at the right URI (via  ActualUriTextBlock)
                            //         Scope task to make sure we unsubscribe from NavigationStarting before starting Part 2.
                            {
                                var task = MyWebView2.GetNavigationCompletedTask();
                                _ = MyWebView2.ExecuteScriptAsync("navigateToTextPage();");
                                await task;
                                var expectedInitialUri = WebView2Common.GetTestPageUri("SimplePageWithText.html").ToString();
                                var initialActualSourceUri = ActualUriTextBlock.Text;
                                logger.Verify(AreEqualIgnoringSpace(expectedInitialUri, initialActualSourceUri),
                                              string.Format("\nTest {0}: Failed, Expected initial Uri {1} did not match actual Uri {2}",
                                                            selectedTest, expectedInitialUri, initialActualSourceUri));
                            }

                            // Part 2: Use GoBack to navigate back. Ensure we are back at [SimplePage.html].
                            {
                                canGoBackBindingVerify = canGoBackBindingVerify && goBackButton.IsEnabled; // Should both be true now
                                logger.Verify(canGoBackBindingVerify,
                                              string.Format("\nTest {0}: Failed, canGoBackBindingVerify failed {1}",
                                                            selectedTest, canGoBackBindingVerify));
                                var task = MyWebView2.GetNavigationCompletedTask();
                                MyWebView2.GoBack();
                                string expectedSecondSourceUri = WebView2Common.GetTestPageUri("SimplePage.html").ToString();
                                await task;
                                string actualSecondSourceUri = ActualUriTextBlock.Text;
                                logger.Verify(AreEqualIgnoringSpace(expectedSecondSourceUri, actualSecondSourceUri),
                                              string.Format("\nTest {0}: Failed, Expected intermediate Uri {1} did not match actual Uri {2}",
                                                            selectedTest, expectedSecondSourceUri, actualSecondSourceUri));
                            }

                            // Part 3: Use GoForward to navigate forward. Ensure we are back at [SimplePageWithText.html"].
                            {
                                canGoForwardBindingVerify = canGoForwardBindingVerify && goForwardButton.IsEnabled; // Should both be true now
                                logger.Verify(canGoForwardBindingVerify,
                                              string.Format("\nTest {0}: Failed, canGoForwardBindingVerify failed {1}",
                                                            selectedTest, canGoForwardBindingVerify));
                                var task = MyWebView2.GetNavigationCompletedTask();
                                MyWebView2.GoForward();
                                string expectedFinalSourceUri = WebView2Common.GetTestPageUri("SimplePageWithText.html").ToString();
                                await task;
                                string actualFinalSourceUri = ActualUriTextBlock.Text;
                                logger.Verify(AreEqualIgnoringSpace(expectedFinalSourceUri, actualFinalSourceUri),
                                            string.Format("\nTest {0}: Failed, Expected final Uri {1} did not match actual Uri {2}",
                                                            selectedTest, expectedFinalSourceUri, actualFinalSourceUri));
                            }
                        }
                        break;

                    case TestList.NavigationStartingTest:
                        {
                            // Part 1: [SimplePage.html] calls navigateToTextPage() -> Navigates to [SimplePageWithText.html].
                            //         Ensure we get expected NavigationStarting event/args.
                            var startTask = MyWebView2.GetNavigationStartingTask();
                            _ = MyWebView2.ExecuteScriptAsync("navigateToTextPage();");
                            await startTask;
                            string startingStatus = Status1.Text;
                            string expectedStatus = "MyWebView2 navigation starting...";
                            logger.Verify((startingStatus == expectedStatus),
                                            string.Format("Test {0}: Failed, Expected navigation status: {1} did not match actual navigation status: {2}",
                                                        selectedTest, expectedStatus, startingStatus));
                            string navigationStartingUri = Status3.Text;
                            string expectedUri = WebView2Common.GetTestPageUri("SimplePageWithText.html").ToString();
                            logger.Verify(AreEqualIgnoringSpace(navigationStartingUri, expectedUri),
                                            string.Format("Test {0}: Failed, Expected NavigationStarting Uri {1} did not match actual NavigationStarting Uri {2}",
                                                        selectedTest, expectedUri, navigationStartingUri));
                            logger.Verify(_isUserInitiated,
                                            string.Format("Test {0}: Failed, NavigationStartingArgs.IsUserInitiated value: {1}, did not match expected value: true",
                                                        selectedTest, _isUserInitiated.ToString()));
                            logger.Verify(!_isRedirected,
                                            string.Format("Test {0}: Failed, NavigationStartingArgs.IsRedirected value: {1}, did not match expected value: false",
                                                        selectedTest, _isRedirected.ToString()));
                        }
                        break;

                    case TestList.NavigationStartingInvalidTest:
                        {
                            // Part 2: [SimplePage.html] navigates to http://www.blockedbynavigationstarting.invalid -> navigation cancelled via OnNavigationStarting.
                            //         Ensure we are still on [SimplePage.html] at end.
                            var startTask2 = MyWebView2.GetNavigationStartingTask();
                            _ = MyWebView2.ExecuteScriptAsync("navigateToInvalidPage();");
                            await startTask2;

                            string navigationStartingUri = Status3.Text;
                            string expectedUri = WebView2Common.GetTestPageUri("SimplePageWithText.html").ToString();
                            logger.Verify(AreEqualIgnoringSpace(navigationStartingUri, expectedUri),
                                        string.Format("Test {0}: Failed, Expected NavigationStarting Uri {1} did not match actual NavigationStarting Uri {2}",
                                                        selectedTest, expectedUri, navigationStartingUri));
                        }
                        break;

                    case TestList.ResizeTest:
                        {
                            // Clear any previous resize message from when we created the webview
                            Status2.Text = string.Empty;

                            MyWebView2.Width = MyWebView2.ActualWidth / 2;
                            MyWebView2.Height = MyWebView2.ActualHeight / 2;
                            MyWebView2.UpdateLayout();
                        }
                        break;

                    case TestList.MoveTest:
                        {
                            var yPos1 = await MyWebView2.ExecuteScriptAsync("getWindowYPosition();");

                            var tcs = new TaskCompletionSource<bool>();
                            Task renderedTask = tcs.Task;
                            EventHandler<RenderedEventArgs> renderedHandler = null;
                            renderedHandler += (sender, args) =>  
                            {
                                CompositionTarget.Rendered -= renderedHandler;
                                tcs.SetResult(true);
                            };
                            CompositionTarget.Rendered += renderedHandler;

                            var panel = FindName("WebView2Collection") as StackPanel;
                            Thickness margin = panel.Margin;
                            margin.Top = margin.Top += 100;
                            panel.Margin = margin;
                            panel.UpdateLayout();

                            await renderedTask;

                            var yPos2 = await MyWebView2.ExecuteScriptAsync("getWindowYPosition();");
                            logger.Verify((yPos1 != yPos2),
                                          string.Format("Test {0} Part 1: Failed, Expected MyWebView2 to move",
                                                        selectedTest));

                            var xPos = await MyWebView2.ExecuteScriptAsync("getWindowXPosition();");
                            // Get the point at the top-left of the TestPage panel, relative to the webview
                            // Expected screenPoint value: (-22, -400)
                            GeneralTransform gt = TransformToVisual(MyWebView2);
                            Point screenPoint = gt.TransformPoint(new Point(0,0));
                            double expectedX = screenPoint.X * -1;
                            // This offset accounts for the elements above the webview on the page--
                            // specifically TestPage panel, which has its top at 65px, and the title bar.
                            // This offset is also valid only when the MUXC test app window is in the top left corner of the main monitor display.
                            // Otherwise, the test will fail.
                            int offset = 65;
                            double expectedY = screenPoint.Y * -1 + offset;

                            logger.Verify((xPos == expectedX.ToString()),
                                          string.Format("Test {0} Part 2: Failed, Expected MyWebView2 xPosition to be {1}, Actual: {2}",
                                                        selectedTest, expectedX.ToString(), xPos.ToString()));
                            logger.Verify((yPos2 == expectedY.ToString()),
                                          string.Format("Test {0} Part 3: Failed, Expected MyWebView2 yPosition to be {1}, Actual: {2}",
                                                        selectedTest, expectedY.ToString(), yPos2.ToString()));

                            var width = await MyWebView2.ExecuteScriptAsync("getWindowWidth();");
                            var height = await MyWebView2.ExecuteScriptAsync("getWindowHeight();");
                            logger.Verify((width == MyWebView2.ActualWidth.ToString()),
                                          string.Format("Test {0} Part 4: Failed, Expected MyWebView2 width to be {1}, Actual: {2}",
                                                        selectedTest, MyWebView2.ActualWidth.ToString(), width.ToString()));
                            logger.Verify((height == MyWebView2.ActualHeight.ToString()),
                                          string.Format("Test {0} Part 5: Failed, Expected MyWebView2 height to be {1}, Actual: {2}",
                                                        selectedTest, MyWebView2.ActualHeight.ToString(), height.ToString()));
                        }
                        break;

                    case TestList.ReparentElementTest:
                        {
                            // Pull the webview out of its border
                            var parentOfWebview = MyWebView2.Parent as Border;
                            parentOfWebview.Child = null;

                            // Make the old location smaller so we can see the webview in its new
                            // position even on a small screen
                            parentOfWebview.Height = 10;

                            // Put the Webview inside a new border in WebView2Collection
                            MoveWebViewControl("MyWebView2", MyWebView2);
                        }
                        break;

                    case TestList.SourceBeforeLoadTest:
                        {
                            // TODO_WebView2: This test should validate rendering
                            logger.Verify((MyWebView2.Source != null),
                                string.Format("Test {0}: Failed, Expected MyWebView2.Source to be {1}",
                                    selectedTest, WebView2Common.GetTestPageUri("SimplePageWithButton.html").ToString()));
                        }
                        break;

                    case TestList.VisibilityHiddenTest:
                        {
                            MyWebView2.Visibility = Visibility.Collapsed;
                        }
                        break;

                    case TestList.VisibilityTurnedOnTest:
                        {
                            // Show the ancestor that was previously hidden
                            MyWebView2.Visibility = Visibility.Visible;
                        }
                        break;

                    case TestList.ParentVisibilityHiddenTest:
                        {
                            // Hide the parent border
                            var testContentsBorder = FindName("TestContentsBorder") as Border;
                            testContentsBorder.Visibility = Visibility.Collapsed;
                        }
                        break;

                    case TestList.ParentVisibilityTurnedOnTest:
                        {
                            // Show the ancestor that was previously hidden
                            var testContentsBorder = FindName("TestContentsBorder") as Border;
                            testContentsBorder.Visibility = Visibility.Visible;
                        }
                        break;

                    case TestList.QueryCoreWebView2BasicTest:
                        {
                            // First load test's default webpage SimplePage.html
                            var navCompletedTask1 = MyWebView2.GetNavigationCompletedTask();
                            WebView2Common.LoadWebPage(MyWebView2, TestPageNames[TestInfoDictionary[selectedTest]]);
                            await navCompletedTask1;

                            // Validate CoreWebView2.DocumentTitle
                            string expectedTitle1 = "Simple Page";
                            string actualTitle1 = GetDocumentTitle(MyWebView2);
                            logger.Verify((expectedTitle1 == actualTitle1),
                                          string.Format("Part 1: Expected WV2.CWV2.DocumentTitle: {0}, actual: {1}",
                                                         expectedTitle1, actualTitle1));

                            // Load a different page
                            var navCompletedTask2 = MyWebView2.GetNavigationCompletedTask();
                            WebView2Common.LoadWebPage(MyWebView2, TestPageNames[1]);
                            await navCompletedTask2;

                            string expectedTitle2 = "Simple Page With Button";
                            string actualTitle2 = GetDocumentTitle(MyWebView2);
                            logger.Verify((expectedTitle2 == actualTitle2),
                                          string.Format("Part 2: Expected WV2.CWV2.DocumentTitle: {0}, actual: {1}",
                                                         expectedTitle2, actualTitle2));

                            // Close() WebView2 - CWV2 should be shutdown + reference cleared
                            MyWebView2.Close();
                            string expectedTitle3 = string.Empty;
                            string actualTitle3 = GetDocumentTitle(MyWebView2);
                            logger.Verify((expectedTitle3 == actualTitle3),
                                          string.Format("Part 3: Expected WV2.CWV2.DocumentTitle: {0}, actual: {1}",
                                                         expectedTitle3, actualTitle3));
                        }
                        break;

                    case TestList.CursorUpdateTest:
                    case TestList.CursorClickUpdateTest:
                        {
                            WebView2WithCursor webviewWithCursor = MyWebView2 as WebView2WithCursor;
                            Windows.UI.Core.CoreCursor cursor = webviewWithCursor.WrappedProtectedCursor;
                            logger.Verify(cursor != null, "ProtectedCursor was null");
                            if (cursor == null) break;
                            var actualCursorType = cursor.Type;
                            var expectedCursorType = Windows.UI.Core.CoreCursorType.Hand;
                            logger.Verify((expectedCursorType == actualCursorType),
                                          string.Format("Expected cursor type ({0}) did not match actual: {1}",
                                                        expectedCursorType, actualCursorType));
                        }
                        break;

                    case TestList.CoreWebView2InitializedTest:
                        {
                            // First load test's default webpage SimplePage.html
                            var tasks = new List<Task>();
                            var initializedTask = MyWebView2.GetCoreWebView2InitializedTask();
                            tasks.Add(MyWebView2.GetNavigationCompletedTask());
                            tasks.Add(initializedTask);

                            WebView2Common.LoadWebPage(MyWebView2, TestPageNames[TestInfoDictionary[selectedTest]]);
                            await Task.WhenAll(tasks);

                            CoreWebView2InitializedEventArgs eventArgs = initializedTask.Result;
                            bool isExceptionNull = (eventArgs.Exception == null);
                             
                            logger.Verify(isExceptionNull,
                                          string.Format("Expected CoreWV2InitializedEventArgs.Exception to be null, actual: {0}",
                                                         eventArgs.Exception));
                        }
                        break;

                    case TestList.CoreWebView2Initialized_FailedTest:
                        {
                            // Try to load test's default webpage SimplePage.html
                            // This should fail since we uninstalled WebView2Runtime
                            var tasks = new List<Task>();
                            var initializedTask = MyWebView2.GetCoreWebView2InitializedTask();

                            WebView2Common.LoadWebPage(MyWebView2, TestPageNames[TestInfoDictionary[selectedTest]]);
                            await initializedTask;

                            CoreWebView2InitializedEventArgs eventArgs = initializedTask.Result;
                            bool exceptionFired = (eventArgs.Exception != null);

                            logger.Verify(exceptionFired,
                                          string.Format("Expected CoreWV2InitializedEventArgs.Exception to fire"));
                        }
                        break;

                    case TestList.WindowlessPopupTest:
                        {
                            var popWebView2 = FindName("popWebView2") as WebView2;
                            var navCompletedTask = popWebView2.GetNavigationCompletedTask();
                            WebView2Common.LoadWebPage(popWebView2, TestPageNames[TestInfoDictionary[selectedTest]]);
                            await navCompletedTask;
                        }
                        break;

                    case TestList.HostNameToFolderMappingTest:
                        {
                            var task = MyWebView2.GetNavigationCompletedTask();
                            var core_wv2 = MyWebView2.CoreWebView2;
                            if (core_wv2 != null)
                            {
                                core_wv2.SetVirtualHostNameToFolderMapping(
                                    "testapp.example", "assets",
                                    Microsoft.Web.WebView2.Core.CoreWebView2HostResourceAccessKind.Allow);
                            }

                            string uriString = "https://testapp.example/SimplePageWithButton.html";
                            WebView2Common.NavigateToUri(MyWebView2, uriString);
                            await task;
                        }
                        break;

                    case TestList.NavigateToVideoTest:
                        {
                            var task = MyWebView2.GetNavigationCompletedTask();
                            NavigateToVideo(MyWebView2);
                            await task;

                            string expectedSourceUri = "https://appassets.example/CastingVideo.mp4";
                            string sourceUri = MyWebView2.Source.ToString(); // Actually expecting "https://appassets.example/CastingVideo.mp4 []"
                            logger.Verify(sourceUri.StartsWith(expectedSourceUri),
                                          string.Format("Test {0}: Expected Uri: {1} did not match with sampled Uri: {2}",
                                                        selectedTest.ToString(), expectedSourceUri, sourceUri));
                        }
                        break;

                    case TestList.NavigateToLocalImageTest:
                        {
                            var task = MyWebView2.GetNavigationCompletedTask();
                            var core_wv2 = MyWebView2.CoreWebView2;
                            if (core_wv2 != null)
                            {
                                core_wv2.SetVirtualHostNameToFolderMapping(
                                    "testapp.example", "assets",
                                    Microsoft.Web.WebView2.Core.CoreWebView2HostResourceAccessKind.Allow);
                            }

                            string uriString = "https://testapp.example/SimplePageWithImage.html";
                            WebView2Common.NavigateToUri(MyWebView2, uriString);
                            await task;
                        }
                        break;

                    case TestList.ScaledTouchTest:
                        {
                            MyWebView2.RenderTransformOrigin = new Windows.Foundation.Point(0, 0);
                            var scaleTransform = new ScaleTransform { ScaleX = 1.2, ScaleY = 1.2 };
                            TransformGroup transformGroup = new TransformGroup();
                            transformGroup.Children.Add(scaleTransform);
                            MyWebView2.RenderTransform = transformGroup;
                        }
                        break;

                    case TestList.CloseThenDPIChangeTest:
                        {
                            MyWebView2.Close();
                        }
                        break;

                    case TestList.AddHostObjectToScriptTest:
                        {
                            var core_wv2 = MyWebView2.CoreWebView2;
                            logger.Verify(core_wv2 != null, string.Format(
                                "Test {0}: CoreWebView2 was null.", selectedTest.ToString()));

                            bool winrtNotImpl = false;
                            try
                            {
                                core_wv2.AddHostObjectToScript("bridge", new Bridge());
                                core_wv2.RemoveHostObjectFromScript("bridge");
                            }
                            catch (NotImplementedException)
                            {
                                // WinRT version of AddHostObjectToScript is not implemented yet,
                                // expect a NotImplementedException for now
                                winrtNotImpl = true;
                            }
                            logger.Verify(winrtNotImpl == true, string.Format(
                                "Test {0}: Expected NotImplementedException but did not receive it.", selectedTest.ToString()));

                            // There is a interop we can use to access the method
                            var interop = (ICoreWebView2Interop)(object)core_wv2;
                            try
                            {
                                interop.AddHostObjectToScript("bridge", new Bridge());

                                // AddHostObjectToScript represents the host objects in JavaScript using JavaScript async proxies.
                                // Accessing any member off of the proxy gives back a promise for another async proxy. You need to
                                // await a proxy to get to an actual value. The async proxies have no knowledge of the members of
                                // the actual host object, so JSON.toString on an async proxy won’t tell you anything interesting
                                // because the async proxy doesn’t know it has a property named Property. When you do bridge.Property
                                // you immediately get back an async proxy representing the result of doing a property get on ‘Property’
                                // on that object and a message is sent off to the host process to find that value, but that async proxy
                                // may resolve to an exception if there isn’t actually a property named Property on the actual host object.
                                string result = await MyWebView2.ExecuteScriptAsync(
                                    "(async function() { " +
                                        "let result = await window.chrome.webview.hostObjects.bridge.AnotherObject.Prop; " +
                                        "window.chrome.webview.postMessage(result); })();");
                                // Listen for WebMessageReceived

                                core_wv2.RemoveHostObjectFromScript("bridge");
                            }
                            catch (Exception e)
                            {
                                logger.Verify(false, string.Format("Test {0}: Unexpected exception: {1}", 
                                    selectedTest.ToString(), e.ToString()));
                            }
                        }
                        break;

                    case TestList.UserAgentTest:
                        {
                            var userAgent = string.Empty;
                            var core_wv2 = MyWebView2.CoreWebView2;
                            if (core_wv2 == null)
                            {
                                logger.LogError(string.Format("Test {0}: Couldn't get CoreWebView2 object", selectedTest.ToString()));
                                break;
                            }

                            var core_wv2_settings = core_wv2.Settings;
                            if (core_wv2_settings == null)
                            {
                                logger.LogError(string.Format("Test {0}: Couldn't get CoreWebView2Settings object", selectedTest.ToString()));
                                break;
                            }
                            
                            userAgent = core_wv2_settings.UserAgent;
                            
                            // The "Edg" token identifies the Chromium Edge browser
                            // For more information, see https://docs.microsoft.com/en-us/microsoft-edge/web-platform/user-agent-guidance
                            logger.Verify(userAgent.Contains("Edg"),
                                string.Format("Test {0}: Expected a valid UserAgent, got {1}", selectedTest.ToString(), userAgent));
                        }
                        break;

                    case TestList.OffTreeWebViewInputTest:
                        {
                            // Create a new webview
                            ApplicationTheme appTheme = Application.Current.RequestedTheme;
                            Color backgroundColor = (appTheme == ApplicationTheme.Light) ? Colors.White : Colors.Black;
                            var webView2 = new WebView2() {
                                Name = "MyWebView2",
                                Margin = new Thickness(8, 8, 8, 8)
                            };
                            AutomationProperties.SetName(webView2, "MyWebView2");
                            AddWebViewEventHandlers(webView2);

                            // Ensure the webview before we add it to the tree
                            var task = webView2.EnsureCoreWebView2Async();
                            await task;

                            // Navigate
                            var navCompletedTask = webView2.GetNavigationCompletedTask();
                            WebView2Common.LoadWebPage(webView2, TestPageNames[TestInfoDictionary[selectedTest]]);
                            await navCompletedTask;

                            // Add the webview to the tree so we can see it
                            var border = FindName("MyWebView2Border") as Border;
                            border.Child = webView2;
                        }
                        break;

                    case TestList.HtmlDropdownTest:
                        {
                            var selctedOption = await MyWebView2.ExecuteScriptAsync("getSelectedOption();");
                            logger.Verify(selctedOption.Equals("\"2\""),
                                string.Format("Test {0} Failed, Expected option \"2\" to be selcted, actually got {1}",
                                    selectedTest, selctedOption));;
                        }
                        break;

                    case TestList.HiddenThenVisibleTest:
                        {
                            logger.Verify(MyWebView2.Visibility == Visibility.Collapsed,
                                 string.Format("Test {0}: Incorrect setup, Expected MyWebView2.Visibility to be Collapsed, was {1}",
                                    selectedTest, MyWebView2.Visibility));

                            // Make WebView2 visible
                            MyWebView2.Visibility = Visibility.Visible;

                            logger.Verify(MyWebView2.IsHitTestVisible,
                                 string.Format("Test {0}: Failed, Expected MyWebView2.IsHitTestVisible to be true, was {1}",
                                    selectedTest, MyWebView2.IsHitTestVisible));
                        }
                        break;

                    case TestList.ParentHiddenThenVisibleTest:
                        {
                            var parentBorder = MyWebView2.Parent as Border;

                            logger.Verify(parentBorder.Visibility == Visibility.Collapsed,
                                 string.Format("Test {0}: Incorrect setup, Expected parentBorder.Visibility to be Collapsed, was {1}",
                                    selectedTest, parentBorder.Visibility));

                            // Make WebView2's parent Border visible
                            parentBorder.Visibility = Visibility.Visible;

                            logger.Verify(MyWebView2.IsHitTestVisible,
                                 string.Format("Test {0}: Failed, Expected MyWebView2.IsHitTestVisible to be true, was {1}",
                                    selectedTest, MyWebView2.IsHitTestVisible));
                        }
                        break;

                    case TestList.LifetimeTabTest:
                        {
                            MyWebView2.Close();
                        }
                        break;

                    default:
                        break;
                }
            }
        }

        async public void CleanupCurrentTest(object sender, RoutedEventArgs args)
        {
            // Ensure any WebViewElements are destroyed + removed from UIA tree prior to signaling test completion 
            // to avoid impacting next test.
            await CleanupWebViewElements();

            if (_isApplicationLanguageOverrideSet)
            {
                ApplicationLanguages.PrimaryLanguageOverride = string.Empty;
                _isApplicationLanguageOverrideSet = false;
            }

            CleanupResultTextBox.Text = "Cleanup completed.";
        }

        protected override void OnNavigatingFrom(NavigatingCancelEventArgs e)
        {
            if (TestNameComboBox.GetSelectedText() != "")
            {
                // For test stability, webviews should be fully cleaned up before we leave the test page.
                Debug.Assert(_areWebviewElementsCleanedUp);
            }
            else
            {
                // If we're not running a test, we should clean up any WebView2s before navigating away.
                if (!_areWebviewElementsCleanedUp)
                {
                    CleanupCurrentTest(null, null);
                }
            }

        }

        private void AppendMessage(string message)
        {
            MessageLog.Text = MessageLog.Text + message + Environment.NewLine;
        }
    }

    [ComImport]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    [Guid("912b34a7-d10b-49c4-af18-7cb7e604e01a")]
    public interface ICoreWebView2Interop
    {
        // IDL: HRESULT AddHostObjectToScript([in] LPCWSTR name, [in] VARIANT* object);
        void AddHostObjectToScript([In] string name, [In] ref object obj);
    }

    // Example classes from public documentation of CoreWebView2's AddHostObjectToScript
#pragma warning disable CS0618
    // The.NET version of CoreWebView2.AddHostObjectToScript currently relies on the host object
    // implementing IDispatch and so uses the deprecated ClassInterfaceType.AutoDual feature of.NET.
    // This may change in the future, see https://github.com/MicrosoftEdge/WebView2Feedback/issues/517
    // for more information
    [ClassInterface(ClassInterfaceType.AutoDual)]
#pragma warning restore CS0618
    [ComVisible(true)]
    public class BridgeAnotherClass
    {
        // Sample property.
        public string Prop { get; set; } = "Example property from host object";
    }

#pragma warning disable CS0618
    [ClassInterface(ClassInterfaceType.AutoDual)]
#pragma warning restore CS0618
    [ComVisible(true)]
    public class Bridge
    {
        public string Func(string param)
        {
            return "Example: " + param;
        }

        public BridgeAnotherClass AnotherObject { get; set; } = new BridgeAnotherClass();

        // Sample indexed property.
        [System.Runtime.CompilerServices.IndexerName("Items")]
        public string this[int index]
        {
            get { return m_dictionary[index]; }
            set { m_dictionary[index] = value; }
        }
        private Dictionary<int, string> m_dictionary = new Dictionary<int, string>();
    }
}
