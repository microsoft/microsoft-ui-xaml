// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControls.TestAppUtils;
using System;
using System.Threading;
using Windows.Foundation;
using Windows.Graphics.Display;
using Windows.System.Threading;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;

// The Templated Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234235

namespace MUXControlsTestApp
{
    public sealed class TestFrame : Frame
    {
        private static string _error = string.Empty;
        private static string _log = string.Empty;

        private Viewbox _rootViewbox = null;
        private Grid _rootGrid = null;
        private Button _backButton = null;
        private Button _goBackInvokerButton = null;
        private Button _goFullScreenInvokerButton = null;
        private Button _toggleThemeButton = null;
        private ToggleButton _innerFrameInLabDimensions = null;
        private Button _closeAppInvokerButton = null;
        private Button _waitForIdleInvokerButton = null;
        private CheckBox _idleStateEnteredCheckBox = null;
        private TextBox _errorReportingTextBox = null;
        private TextBox _logReportingTextBox = null;
        private TextBlock _currentPageTextBlock = null;
        private CheckBox _viewScalingCheckBox = null;
        private Button _waitForDebuggerInvokerButton = null;
        private CheckBox _debuggerAttachedCheckBox = null;
        private TextBox _unhandledExceptionReportingTextBox = null;
        private Type _mainPageType = null;
        private ContentPresenter _pagePresenter = null;

        public TestFrame(Type mainPageType)
        {
            _mainPageType = mainPageType;
            this.DefaultStyleKey = typeof(TestFrame);
            Application.Current.UnhandledException += OnUnhandledException;
        }

        public void ChangeBarVisibility(Visibility visibility)
        {
            UIElement bar = (UIElement)GetTemplateChild("TestFrameBar");
            if (bar != null)
            {
                bar.Visibility = visibility;
            }
        }

        private void TestFrame_Navigated(object sender, Windows.UI.Xaml.Navigation.NavigationEventArgs e)
        {
            if (e.SourcePageType == _mainPageType)
            {
                _backButton.Visibility = Visibility.Collapsed;
                _currentPageTextBlock.Text = "Home";
            }
            else
            {
                _backButton.Visibility = Visibility.Visible;
                _currentPageTextBlock.Text = (e.Parameter is string ? e.Parameter as string : "");
            }

            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();
        }

        protected override void OnApplyTemplate()
        {
            base.OnApplyTemplate();

            this.Navigated += TestFrame_Navigated;

            _rootViewbox = (Viewbox)GetTemplateChild("RootViewbox");
            _rootGrid = (Grid)GetTemplateChild("RootGrid");

            // The root grid is in a Viewbox, which lays out its child with infinite
            // available size, so set the grid's dimensions to match the window's.
            // When running tests on phone, these dimensions will end up getting set to
            // match the screen's unscaled dimensions to deal with scale factor differences
            // between devices and provide some consistency.
            SetRootGridSizeFromWindowSize();

            Window.Current.SizeChanged += OnWindowSizeChanged;

            // NOTE: The "BackButton" element automatically has a handler hooked up to it by Frame
            // just by being named "BackButton"
            _backButton = (Button)GetTemplateChild("BackButton");

            _toggleThemeButton = (Button)GetTemplateChild("ToggleThemeButton");
            _toggleThemeButton.Click += ToggleThemeButton_Click;

            _pagePresenter = (ContentPresenter)GetTemplateChild("PagePresenter");

            _innerFrameInLabDimensions = (ToggleButton)GetTemplateChild("InnerFrameInLabDimensions");
            _innerFrameInLabDimensions.Checked += _innerFrameInLabDimensions_Checked;
            _innerFrameInLabDimensions.Unchecked += _innerFrameInLabDimensions_Unchecked;
            if(_innerFrameInLabDimensions.IsChecked == true)
            {
                _innerFrameInLabDimensions_Checked(null, null);
            }
            else
            {
                _innerFrameInLabDimensions_Unchecked(null, null);
            }

            _goBackInvokerButton = (Button)GetTemplateChild("GoBackInvokerButton");
            _goBackInvokerButton.Click += GoBackInvokerButton_Click;

            _closeAppInvokerButton = (Button)GetTemplateChild("CloseAppInvokerButton");
            _closeAppInvokerButton.Click += CloseAppInvokerButton_Click;

            _waitForIdleInvokerButton = (Button)GetTemplateChild("WaitForIdleInvokerButton");
            _waitForIdleInvokerButton.Click += WaitForIdleInvokerButton_Click;

            _idleStateEnteredCheckBox = (CheckBox)GetTemplateChild("IdleStateEnteredCheckBox");

            _errorReportingTextBox = (TextBox)GetTemplateChild("ErrorReportingTextBox");
            _logReportingTextBox = (TextBox)GetTemplateChild("LogReportingTextBox");

            _currentPageTextBlock = (TextBlock)GetTemplateChild("CurrentPageTextBlock");
            _currentPageTextBlock.Text = "Home";

            _viewScalingCheckBox = (CheckBox)GetTemplateChild("ViewScalingCheckBox");
            _viewScalingCheckBox.Checked += OnViewScalingCheckBoxChanged;
            _viewScalingCheckBox.Unchecked += OnViewScalingCheckBoxChanged;

            _waitForDebuggerInvokerButton = (Button)GetTemplateChild("WaitForDebuggerInvokerButton");
            _waitForDebuggerInvokerButton.Click += WaitForDebuggerInvokerButton_Click;

            _debuggerAttachedCheckBox = (CheckBox)GetTemplateChild("DebuggerAttachedCheckBox");

            _goFullScreenInvokerButton = (Button)GetTemplateChild("FullScreenInvokerButton");
            _goFullScreenInvokerButton.Click += GoFullScreenInvokeButton_Click;

            _unhandledExceptionReportingTextBox = (TextBox)GetTemplateChild("UnhandledExceptionReportingTextBox");
        }

        private void _innerFrameInLabDimensions_Click(object sender, RoutedEventArgs e)
        {
            if(double.IsInfinity(_pagePresenter.MaxWidth))
            {
                // Not CI mode, so enter it now
                _pagePresenter.MaxWidth = 1024;
                _pagePresenter.MaxHeight = 664;
            }
            else
            {
                // We are already in "CI mode"
                _pagePresenter.ClearValue(MaxWidthProperty);
                _pagePresenter.ClearValue(MaxHeightProperty);
            }
        }

        private void _innerFrameInLabDimensions_Checked(object sender, RoutedEventArgs e)
        {
            // Enter CI mode
            _pagePresenter.MaxWidth = 1024;
            _pagePresenter.MaxHeight = 664;
        }

        private void _innerFrameInLabDimensions_Unchecked(object sender, RoutedEventArgs e)
        {
            // Leave CI mode
            _pagePresenter.ClearValue(MaxWidthProperty);
            _pagePresenter.ClearValue(MaxHeightProperty);
        }

        private void ToggleThemeButton_Click(object sender,RoutedEventArgs e)
        {
            var contentAsFrameworkElement = Window.Current.Content as FrameworkElement;
            if(contentAsFrameworkElement.RequestedTheme == ElementTheme.Default)
            {
                // Convert theme from default to either dark or light based on application requestedtheme
                contentAsFrameworkElement.RequestedTheme = (Application.Current.RequestedTheme == ApplicationTheme.Light) ? ElementTheme.Light : ElementTheme.Dark;
            }
            // Invert theme
            contentAsFrameworkElement.RequestedTheme = (contentAsFrameworkElement.RequestedTheme == ElementTheme.Light) ? ElementTheme.Dark : ElementTheme.Light;
        }

        private void GoFullScreenInvokeButton_Click(object sender, RoutedEventArgs e)
        {
            var icon = _goFullScreenInvokerButton.Content as SymbolIcon;
            if (icon.Symbol == Symbol.FullScreen)
            {
                if (Windows.UI.ViewManagement.ApplicationView.GetForCurrentView().TryEnterFullScreenMode())
                {
                    icon.Symbol = Symbol.BackToWindow;
                }
            }
            else
            {
                Windows.UI.ViewManagement.ApplicationView.GetForCurrentView().ExitFullScreenMode();
                icon.Symbol = Symbol.FullScreen;
            }
        }

        private void OnWindowSizeChanged(object sender, Windows.UI.Core.WindowSizeChangedEventArgs e)
        {
            SetRootGridSizeFromWindowSize();
        }

        private void OnViewScalingCheckBoxChanged(object sender, RoutedEventArgs e)
        {
            // On phone, set the root grid's dimensions to match the device resolution to
            // allow our test pages to effectively lay out at 1.0 scale factor.
            if (_viewScalingCheckBox.IsChecked.Value
                && PlatformConfiguration.IsDevice(DeviceType.Phone))
            {
                var displayInfo = DisplayInformation.GetForCurrentView();

                _rootGrid.Width = displayInfo.ScreenWidthInRawPixels;
                _rootGrid.Height = displayInfo.ScreenHeightInRawPixels;
                _rootViewbox.Stretch = Windows.UI.Xaml.Media.Stretch.UniformToFill;

                Window.Current.SizeChanged -= OnWindowSizeChanged;
            }
            else
            {
                SetRootGridSizeFromWindowSize();
                Window.Current.SizeChanged += OnWindowSizeChanged;
            }
        }

        private void SetRootGridSizeFromWindowSize()
        {
            var size = new Size(Window.Current.Bounds.Width, Window.Current.Bounds.Height);

            if (Windows.Foundation.Metadata.ApiInformation.IsTypePresent("Windows.UI.Xaml.XamlRoot"))
            {
                try
                {
                    var xamlRoot = _rootGrid.XamlRoot;
                    size = xamlRoot.Size;
                }
                catch (InvalidCastException)
                {
                    // If running on mismatched OS build, just fall back to window bounds.
                }
            }
            _rootGrid.Width = size.Width;
            _rootGrid.Height = size.Height;
        }

        private void GoBackInvokerButton_Click(object sender, RoutedEventArgs e)
        {
            if (CanGoBack)
            {
                GoBack();
            }
        }

        private void CloseAppInvokerButton_Click(object sender, RoutedEventArgs e)
        {
#if HOSTED_IN_WPF_ISLAND
            System.Windows.Application.Current.Shutdown();
#else
            Application.Current.Exit();
#endif
        }

        private async void WaitForIdleInvokerButton_Click(object sender, RoutedEventArgs e)
        {
            _idleStateEnteredCheckBox.IsChecked = false;
            await Windows.System.Threading.ThreadPool.RunAsync(WaitForIdleWorker);

            _logReportingTextBox.Text = _log;

            if (_error.Length == 0)
            {
                _idleStateEnteredCheckBox.IsChecked = true;
            }
            else
            {
                // Setting Text will raise a property-changed event, so even if we
                // immediately set it back to the empty string, we'll still get the
                // error-reported event that we can detect and handle.
                _errorReportingTextBox.Text = _error;
                _errorReportingTextBox.Text = string.Empty;

                _error = string.Empty;
            }
        }

        private static void WaitForIdleWorker(IAsyncAction action)
        {
#if HOSTED_IN_WPF_ISLAND
            // Wait for Idle WPF dispatcher.
            System.Windows.Application.Current.Dispatcher.Invoke(System.Windows.Threading.DispatcherPriority.Background,
                                          new Action(delegate { }));
            _error = string.Empty;
#else

            _error = IdleSynchronizer.TryWait(out _log);
#endif
        }

        private async void WaitForDebuggerInvokerButton_Click(object sender, RoutedEventArgs e)
        {
            _debuggerAttachedCheckBox.IsChecked = false;
            await Windows.System.Threading.ThreadPool.RunAsync(WaitForDebuggerWorker);
            _debuggerAttachedCheckBox.IsChecked = true;
        }

        private static void WaitForDebuggerWorker(IAsyncAction action)
        {
            var waitEvent = new AutoResetEvent(false);

            while (!global::System.Diagnostics.Debugger.IsAttached)
            {
                ThreadPoolTimer.CreateTimer((timer) => { waitEvent.Set(); }, TimeSpan.FromSeconds(1));
                waitEvent.WaitOne();
            }

            global::System.Diagnostics.Debugger.Break();
        }

        private void OnUnhandledException(object sender, Windows.UI.Xaml.UnhandledExceptionEventArgs e)
        {
            e.Handled = true;
            _unhandledExceptionReportingTextBox.Text = e.Exception.ToString();
            Application.Current.Exit();
        }
    }
}
