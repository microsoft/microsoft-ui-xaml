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

namespace MUXControlsTestApp
{
    public sealed class TestFrame : Frame
    {

        private Viewbox _rootViewbox = null;
        private Grid _rootGrid = null;
        private Button _backButton = null;
        private Button _goBackInvokerButton = null;
        private Button _goFullScreenInvokerButton = null;
        private Button _toggleThemeButton = null;
        private TextBlock _currentPageTextBlock = null;
        private Type _mainPageType = null;

        public TestFrame(Type mainPageType)
        {
            _mainPageType = mainPageType;
            this.DefaultStyleKey = typeof(TestFrame);
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

            _goBackInvokerButton = (Button)GetTemplateChild("GoBackInvokerButton");
            _goBackInvokerButton.Click += GoBackInvokerButton_Click;

            _currentPageTextBlock = (TextBlock)GetTemplateChild("CurrentPageTextBlock");
            _currentPageTextBlock.Text = "Home";

            _goFullScreenInvokerButton = (Button)GetTemplateChild("FullScreenInvokerButton");
            _goFullScreenInvokerButton.Click += GoFullScreenInvokeButton_Click;
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
    }
}
