// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media.Imaging;
using System;
using System.Threading.Tasks;
using Windows.Foundation;

namespace WinUICsDesktopSampleApp
{
    public sealed partial class MainWindow : Window
    {
        private Button buttonCloseRuntimeWindow = null;
        private Window markupWindow = null;
        private Window runtimeWindow = null;
        private bool isMarkupWindowActivated = false;
        private bool isRuntimeWindowActivated = false;
        private ContentDialog dialog = null;

        public MainWindow()
        {
            this.InitializeComponent();

            this.Activated += Window_Activated;
            this.Closed += Window_Closed;
            this.SizeChanged += Window_SizeChanged;
            this.VisibilityChanged += Window_VisibilityChanged;

            // Toggle custom titlebar functionalty on/off to validation check
            // toolbar offset robustness. It should remain off
            // by default unless the "CustomTitleBar_Click" button is clicked
            this.ExtendsContentIntoTitleBar = true;
            this.SetTitleBar(this.customTitleBarTest);
        }

        private void NavigationView_SelectionChanged(NavigationView sender, NavigationViewSelectionChangedEventArgs args)
        {
            // If we were on the Key Press page but no longer are, unsubscribe from events
            if (stackPanelKeyPress.Visibility == Visibility.Visible)
            {
                this.Content.XamlRoot.Content.PreviewKeyDown -= PreviewKeyDownHandler;
                this.Content.XamlRoot.Content.PreviewKeyUp -= PreviewKeyUpHandler;
            }

            switch ((args.SelectedItem as NavigationViewItem).Content)
            {
                case "External Control":
                    stackPanelExternalControl.Visibility = Visibility.Visible;
                    stackPanelWindow.Visibility = Visibility.Collapsed;
                    stackPanelBinding.Visibility = Visibility.Collapsed;
                    stackPanelKeyPress.Visibility = Visibility.Collapsed;
                    break;
                case "Window":
                    stackPanelExternalControl.Visibility = Visibility.Collapsed;
                    stackPanelWindow.Visibility = Visibility.Visible;
                    stackPanelKeyPress.Visibility = Visibility.Collapsed;
                    break;
                case "Binding":
                    stackPanelExternalControl.Visibility = Visibility.Collapsed;
                    stackPanelWindow.Visibility = Visibility.Collapsed;
                    stackPanelBinding.Visibility = Visibility.Visible;
                    stackPanelKeyPress.Visibility = Visibility.Collapsed;
                    break;
                case "Key Press":
                    stackPanelExternalControl.Visibility = Visibility.Collapsed;
                    stackPanelWindow.Visibility = Visibility.Collapsed;
                    stackPanelBinding.Visibility = Visibility.Collapsed;
                    stackPanelKeyPress.Visibility = Visibility.Visible;
                    this.Content.XamlRoot.Content.PreviewKeyDown += PreviewKeyDownHandler;
                    this.Content.XamlRoot.Content.PreviewKeyUp += PreviewKeyUpHandler;
                    break;
            }
        }

        private void ButtonResetBounds_Click(object sender, RoutedEventArgs e)
        {
            textBlockBoundsX.Text = textBlockBoundsY.Text = textBlockBoundsWidth.Text = textBlockBoundsHeight.Text = string.Empty;
        }

        private void ButtonGetBounds_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                Rect bounds = this.SelectedWindow.Bounds;

                textBlockBoundsX.Text = bounds.X.ToString();
                textBlockBoundsY.Text = bounds.Y.ToString();
                textBlockBoundsWidth.Text = bounds.Width.ToString();
                textBlockBoundsHeight.Text = bounds.Height.ToString();
            }
            catch (Exception ex)
            {
                textBoxError.Text = ex.Message;
            }
        }

        private void ButtonResetVisible_Click(object sender, RoutedEventArgs e)
        {
            textBlockVisible.Text = string.Empty;
        }

        private void ButtonGetVisible_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                textBlockVisible.Text = this.SelectedWindow.Visible ? "True" : "False";
            }
            catch (Exception ex)
            {
                textBoxError.Text = ex.Message;
            }
        }

        private void ButtonResetContent_Click(object sender, RoutedEventArgs e)
        {
            textBlockContent.Text = string.Empty;
        }

        private void ButtonGetContent_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                FrameworkElement contentAsFE = this.SelectedWindow.Content as FrameworkElement;
                if (contentAsFE != null)
                {
                    textBlockContent.Text = contentAsFE.Name;
                }
            }
            catch (Exception ex)
            {
                textBoxError.Text = ex.Message;
            }
        }

        private void ButtonResetTitle_Click(object sender, RoutedEventArgs e)
        {
            textBoxTitle.Text = string.Empty;
        }

        private void ButtonGetTitle_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                textBoxTitle.Text = this.SelectedWindow.Title;
            }
            catch (Exception ex)
            {
                textBoxError.Text = ex.Message;
            }
        }

        private void ButtonSetTitle_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                this.SelectedWindow.Title = textBoxTitle.Text;
            }
            catch (Exception ex)
            {
                textBoxError.Text = ex.Message;
            }
        }

        private void ButtonResetCurrent_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                textBlockCurrent.Text = string.Empty;
            }
            catch (Exception ex)
            {
                textBoxError.Text = ex.Message;
            }
        }

        private void ButtonGetCurrent_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                textBlockCurrent.Text = Window.Current == null ? "null" : Window.Current.ToString();
            }
            catch (Exception ex)
            {
                textBoxError.Text = ex.Message;
            }
        }

        private void ButtonResetCoreWindow_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                textBlockCoreWindow.Text = string.Empty;
            }
            catch (Exception ex)
            {
                textBoxError.Text = ex.Message;
            }
        }

        private void ButtonGetCoreWindow_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                Window targetWindow = this.SelectedWindow;
                textBlockCoreWindow.Text = targetWindow.CoreWindow == null ? "null" : targetWindow.CoreWindow.ToString();
            }
            catch (Exception ex)
            {
                textBoxError.Text = ex.Message;
            }
        }

        private void ButtonResetDispatcher_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                textBlockDispatcher.Text = string.Empty;
            }
            catch (Exception ex)
            {
                textBoxError.Text = ex.Message;
            }
        }

        private void ButtonGetDispatcher_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                Window targetWindow = this.SelectedWindow;
                textBlockDispatcher.Text = targetWindow.Dispatcher == null ? "null" : targetWindow.Dispatcher.ToString();
            }
            catch (Exception ex)
            {
                textBoxError.Text = ex.Message;
            }
        }

        private void ButtonResetCompositor_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                textBlockCompositor.Text = string.Empty;
            }
            catch (Exception ex)
            {
                textBoxError.Text = ex.Message;
            }
        }

        private void ButtonGetCompositor_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                Window targetWindow = this.SelectedWindow;
                textBlockCompositor.Text = targetWindow.Compositor == null ? "null" : targetWindow.Compositor.ToString();
            }
            catch (Exception ex)
            {
                textBoxError.Text = ex.Message;
            }
        }

        private void ButtonCreateWindow_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                LogWindowEvent("ButtonCreateWindow_Click - entry");

                if (comboBoxWindowName.SelectedIndex == 1)
                {
                    if (checkBoxAsyncWindowLifetimeOperation.IsChecked == true)
                    {
                        this.DispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
                        {
                            CreateWindow(ref markupWindow, true);
                        }));
                    }
                    else
                    {
                        CreateWindow(ref markupWindow, true);
                    }
                }
                else
                {
                    if (checkBoxAsyncWindowLifetimeOperation.IsChecked == true)
                    {
                        this.DispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
                        {
                            CreateWindow(ref runtimeWindow, false);
                            runtimeWindow.Title = "WinUI Desktop - Runtime Window";
                            LogWindowEvent("ButtonCreateWindow_Click - setting Window.Content");
                            SetRuntimeWindowContent();
                        }));
                    }
                    else
                    {
                        CreateWindow(ref runtimeWindow, false);
                        runtimeWindow.Title = "WinUI Desktop - Runtime Window";
                        LogWindowEvent("ButtonCreateWindow_Click - setting Window.Content");
                        SetRuntimeWindowContent();
                    }
                }

                LogWindowEvent("ButtonCreateWindow_Click - exit");
            }
            catch (Exception ex)
            {
                textBoxError.Text = ex.Message;
            }
        }

        private void CreateWindow(ref Window window, bool createMarkupWindow)
        {
            window = createMarkupWindow ? new MarkupWindow() : new Window();
            window.Activated += Window_Activated;
            window.Closed += Window_Closed;
            window.SizeChanged += Window_SizeChanged;
            window.VisibilityChanged += Window_VisibilityChanged;
            RefreshEnabledButtons();
        }

        private void SetRuntimeWindowContent()
        {
            var textBlockRuntimeWindow = new TextBlock()
            {
                Name = "textBlockRuntimeWindow",
                Text = "Runtime Window",
                FontSize = 20,
                Margin = new Thickness(2)
            };

            buttonCloseRuntimeWindow = new Button()
            {
                Name = "buttonCloseRuntimeWindow",
                Content = "Close Window",
                Margin = new Thickness(2)
            };
            AutomationProperties.SetName(buttonCloseRuntimeWindow, "buttonCloseRuntimeWindow");
            buttonCloseRuntimeWindow.Click += ButtonCloseRuntimeWindow_Click;

            var stackPanel = new StackPanel();
            stackPanel.Name = "stackPanelRoot";
            stackPanel.Children.Add(textBlockRuntimeWindow);
            stackPanel.Children.Add(buttonCloseRuntimeWindow);

            runtimeWindow.Content = stackPanel;
        }

        private void PreviewKeyDownHandler(object sender, Microsoft.UI.Xaml.Input.KeyRoutedEventArgs e)
        {
            downTextBlock.Text = "Last key down detected was: " + e.Key;
        }

        private void PreviewKeyUpHandler(object sender, Microsoft.UI.Xaml.Input.KeyRoutedEventArgs e)
        {
            upTextBlock.Text = "Last key up detected was: " + e.Key;
        }

        private void openPopupButton_Click(object sender, RoutedEventArgs e)
        {
            if (keyPressPopup.IsOpen)
            {
                keyPressPopup.IsOpen = false;
                openPopupButton.Content = "Open Popup";
            }
            else
            {
                keyPressPopup.IsOpen = true;
                openPopupButton.Content = "Close Popup";
            }
        }

        private void ButtonCloseRuntimeWindow_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                LogWindowEvent("ButtonCloseRuntimeWindow_Click - entry");

                runtimeWindow.Close();

                LogWindowEvent("ButtonCloseRuntimeWindow_Click - exit");
            }
            catch (Exception ex)
            {
                textBoxError.Text = ex.Message;
            }
        }

        private void ButtonActivateWindow_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                LogWindowEvent("ButtonActivateWindow_Click - entry");

                if (comboBoxWindowName.SelectedIndex == 1)
                {
                    if (checkBoxAsyncWindowLifetimeOperation.IsChecked == true)
                    {
                        markupWindow.DispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
                        {
                            markupWindow.Activate();
                        }));
                    }
                    else
                    {
                        markupWindow.Activate();
                    }
                    isMarkupWindowActivated = true;
                }
                else
                {
                    if (checkBoxAsyncWindowLifetimeOperation.IsChecked == true)
                    {
                        runtimeWindow.DispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
                        {
                            runtimeWindow.Activate();
                        }));
                    }
                    else
                    {
                        runtimeWindow.Activate();
                    }
                    isRuntimeWindowActivated = true;
                }
                RefreshEnabledButtons();

                LogWindowEvent("ButtonActivateWindow_Click - exit");
            }
            catch (Exception ex)
            {
                textBoxError.Text = ex.Message;
            }
        }

        private void ButtonCloseWindow_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                LogWindowEvent("ButtonCloseWindow_Click - entry");

                switch (comboBoxWindowName.SelectedIndex)
                {
                    case 0:
                        if (checkBoxAsyncWindowLifetimeOperation.IsChecked == true)
                        {
                            this.DispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
                            {
                                this.Close();
                            }));
                        }
                        else
                        {
                            this.Close();
                        }
                        break;
                    case 1:
                        if (checkBoxAsyncWindowLifetimeOperation.IsChecked == true)
                        {
                            markupWindow.DispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
                            {
                                markupWindow.Close();
                            }));
                        }
                        else
                        {
                            markupWindow.Close();
                        }
                        isMarkupWindowActivated = false;
                        break;
                    default:
                        if (checkBoxAsyncWindowLifetimeOperation.IsChecked == true)
                        {
                            runtimeWindow.DispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
                            {
                                runtimeWindow.Close();
                            }));
                        }
                        else
                        {
                            runtimeWindow.Close();
                        }
                        isRuntimeWindowActivated = false;
                        break;
                }
                RefreshEnabledButtons();

                LogWindowEvent("ButtonCloseWindow_Click - exit");
            }
            catch (Exception ex)
            {
                textBoxError.Text = ex.Message;
            }
        }

        private void ButtonDiscardWindow_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                LogWindowEvent("ButtonDiscardWindow_Click - entry");

                if (comboBoxWindowName.SelectedIndex == 1)
                {
                    if (checkBoxAsyncWindowLifetimeOperation.IsChecked == true)
                    {
                        markupWindow.DispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
                        {
                            DiscardWindow(ref markupWindow);
                        }));
                    }
                    else
                    {
                        DiscardWindow(ref markupWindow);
                    }
                }
                else
                {
                    if (checkBoxAsyncWindowLifetimeOperation.IsChecked == true)
                    {
                        runtimeWindow.DispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
                        {
                            DiscardWindow(ref runtimeWindow);
                        }));
                    }
                    else
                    {
                        DiscardWindow(ref runtimeWindow);
                    }
                }

                LogWindowEvent("ButtonDiscardWindow_Click - exit");
            }
            catch (Exception ex)
            {
                textBoxError.Text = ex.Message;
            }
        }

        private void DiscardWindow(ref Window window)
        {
            window.Activated -= Window_Activated;
            window.Closed -= Window_Closed;
            window.SizeChanged -= Window_SizeChanged;
            window.VisibilityChanged -= Window_VisibilityChanged;
            window = null;
            RefreshEnabledButtons();
        }

        private void ButtonUseDispatcherQueue_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                LogWindowEvent("ButtonUseDispatcherQueue_Click - entry");

                Window targetWindow = this.SelectedWindow;
                DispatcherQueue dispatcherQueue = targetWindow.DispatcherQueue;
                
                dispatcherQueue.ShutdownStarting += DispatcherQueue_ShutdownStarting;
                dispatcherQueue.ShutdownCompleted += DispatcherQueue_ShutdownCompleted;
                
                bool result = dispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
                {
                    LogWindowEvent("ButtonUseDispatcherQueue_Click - enqueued work");
                }));

                Task task = Task.Run(() =>
                {
                    DispatcherQueue asyncDispatcherQueue = targetWindow.DispatcherQueue;

                    bool asyncResult = asyncDispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
                    {
                        LogWindowEvent("ButtonUseDispatcherQueue_Click - async enqueued work");
                    }));
                });

                LogWindowEvent($"ButtonUseDispatcherQueue_Click - exit, result={result}");
            }
            catch (Exception ex)
            {
                textBoxError.Text = ex.Message;
            }
        }

        private void DispatcherQueue_ShutdownStarting(DispatcherQueue sender, DispatcherQueueShutdownStartingEventArgs args)
        {
            LogWindowEvent("DispatcherQueue_ShutdownStarting");
        }

        private void DispatcherQueue_ShutdownCompleted(DispatcherQueue sender, object args)
        {
            LogWindowEvent("DispatcherQueue_ShutdownCompleted");
        }

        private void Window_Activated(object sender, WindowActivatedEventArgs args)
        {
            LogWindowEvent($"Window_Activated for {WindowName(sender)} - Handled={args.Handled}, WindowActivationState={args.WindowActivationState}");
        }

        private void Window_Closed(object sender, WindowEventArgs args)
        {
            string windowName = WindowName(sender);

            LogWindowEvent($"Window_Closed for {windowName} - Handled={args.Handled}");

            if (windowName == "RuntimeWindow")
            {
                buttonCloseRuntimeWindow.Click -= ButtonCloseRuntimeWindow_Click;
                buttonCloseRuntimeWindow = null;

                isRuntimeWindowActivated = false;
            }
            else if (windowName == "MarkupWindow")
            {
                isMarkupWindowActivated = false;
            }

            RefreshEnabledButtons();
        }

        private void Window_SizeChanged(object sender, WindowSizeChangedEventArgs args)
        {
            LogWindowEvent($"Window_SizeChanged for {WindowName(sender)} - Handled={args.Handled}");
        }

        private void Window_VisibilityChanged(object sender, WindowVisibilityChangedEventArgs args)
        {
            LogWindowEvent($"Window_VisibilityChanged for {WindowName(sender)} - Handled={args.Handled}, Visible={args.Visible}");
        }

        private void ButtonClearEvents_Click(object sender, RoutedEventArgs e)
        {
            textBoxLastEvent.Text = string.Empty;
            comboBoxEvents.Items.Clear();
        }

        private void ButtonClearError_Click(object sender, RoutedEventArgs e)
        {
            textBoxError.Text = string.Empty;
        }

        private void LogWindowEvent(string strEvent)
        {
            textBoxLastEvent.Text = strEvent;
            comboBoxEvents.Items.Add(strEvent);
            comboBoxEvents.SelectedIndex = comboBoxEvents.Items.Count - 1;
        }

        private void ComboBoxWindowName_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            RefreshEnabledButtons();
        }

        private void ButtonResetDODispatcher_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                textBlockDODispatcher.Text = string.Empty;
            }
            catch (Exception ex)
            {
                textBoxError.Text = ex.Message;
            }
        }

        private void ButtonGetDODispatcher_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                var dispatcher = ((DependencyObject)sender).Dispatcher;
                textBlockDODispatcher.Text = dispatcher == null ? "null" : dispatcher.ToString();
            }
            catch (Exception ex)
            {
                textBoxError.Text = ex.Message;
            }
        }

        private void RefreshEnabledButtons()
        {
            if (buttonGetBounds == null) return;

            bool isWindowCreated;
            bool isWindowActivated;

            switch (comboBoxWindowName.SelectedIndex)
            {
                case 0:
                    isWindowCreated = true;
                    isWindowActivated = true;
                    break;
                case 1:
                    isWindowCreated = markupWindow != null;
                    isWindowActivated = isMarkupWindowActivated;
                    break;
                default:
                    isWindowCreated = runtimeWindow != null;
                    isWindowActivated = isRuntimeWindowActivated;
                    break;
            }

            buttonGetBounds.IsEnabled = isWindowCreated;
            buttonGetVisible.IsEnabled = isWindowCreated;
            buttonGetContent.IsEnabled = isWindowCreated;
            buttonGetTitle.IsEnabled = isWindowCreated;
            buttonSetTitle.IsEnabled = isWindowCreated;
            buttonGetCoreWindow.IsEnabled = isWindowCreated;
            buttonGetDispatcher.IsEnabled = isWindowCreated;
            buttonGetCompositor.IsEnabled = isWindowCreated;
            buttonUseDispatcherQueue.IsEnabled = isWindowCreated;

            buttonCreateWindow.IsEnabled = !isWindowCreated;
            buttonActivateWindow.IsEnabled = isWindowCreated;
            buttonCloseWindow.IsEnabled = isWindowCreated && isWindowActivated; // Remove " && isWindowActivated" once the window activation issue is fixed.
            buttonDiscardWindow.IsEnabled = isWindowCreated && !isWindowActivated;
        }

        private Window SelectedWindow
        {
            get
            {
                switch (comboBoxWindowName.SelectedIndex)
                {
                    case 0:
                        return this;
                    case 1:
                        return markupWindow;
                    default:
                        return runtimeWindow;
                }
            }
        }

        private string WindowName(object sender)
        {
            string windowName = "MainWindow";
            Window window = sender as Window;

            if (window == markupWindow)
            {
                windowName = "MarkupWindow";
            }
            else if (window == runtimeWindow)
            {
                windowName = "RuntimeWindow";
            }

            return windowName;
        }

        private void OnClickCreateWindow(object sender, RoutedEventArgs e)
        {
            ((App)App.Current).AddWindow();
        }

        private void OnClickToggleText(object sender, RoutedEventArgs e)
        {
            if (AcceleratorTextBlock.Text == "lorem ipsum")
            {
                AcceleratorTextBlock.Text = "go bears!";
            }
            else
            {
                AcceleratorTextBlock.Text = "lorem ipsum";
            }
        }

        private void OnClickCloseLatestWindow(object sender, RoutedEventArgs e)
        {
            ((App)App.Current).RemoveLastestWindow();
        }
        private async void OnOpenContentDialogButtonClick(object sender, RoutedEventArgs e)
        {
            dialog = new ContentDialog()
            {
                Title = "Example Content Dialog",
                Content = "Sample Content",
                CloseButtonText = "Ok"
            };

            dialog.Closed += ((_, _) =>
            {
                textBoxLastEvent.Text = "ContentDialog_closed";
            });
            dialog.Opened += ((_, _) =>
            {
                textBoxLastEvent.Text = "ContentDialog_opened";
            });

            dialog.XamlRoot = this.Content.XamlRoot;
            await dialog.ShowAsync();
        }

        private void OnCloseContentDialogButtonClick(object sender, RoutedEventArgs e)
        {
            if (dialog != null)
            {
                dialog.Hide();
            }
        }

        private void addImageButton_Click(object sender, RoutedEventArgs e)
        {
            Image image = new Image
            {
                Width = 100,
                Height = 100,
                Source = new BitmapImage
                {
                    UriSource = new Uri("ms-appx:///Images/TestImage2.png")
                }
            };
            imageStackPanel.Children.Add(image);
        }

        private void CustomTitleBar_Click(object sender, RoutedEventArgs e)
        {
            this.customTitleBarTest.Height += 15;
            this.ExtendsContentIntoTitleBar = !this.ExtendsContentIntoTitleBar;
        }

        private void navigationView_PaneClosed(NavigationView sender, object args)
        {
            buttonControl.NavigationPaneStatus().Text = "Navigation View: Pane Closed";
        }
    }

    public static class AttachedPropSampleExtensions
    {
        public static readonly DependencyProperty MyAttachedPropProperty = DependencyProperty.RegisterAttached(
            "MyAttachedProp",
            typeof(DependencyObject),
            typeof(AttachedPropSampleExtensions),
            new PropertyMetadata(null));

        public static DependencyObject GetMyAttachedProp(DependencyObject element)
        {
            return (DependencyObject)element.GetValue(MyAttachedPropProperty);
        }

        public static void SetMyAttachedProp(DependencyObject element, DependencyObject value)
        {
            element.SetValue(MyAttachedPropProperty, value);
        }
    }
}
