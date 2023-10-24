// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading;
using System.Linq;
using Windows.ApplicationModel;
using Windows.Globalization;
using Windows.System.Profile;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using Microsoft.UI.Private.Media;
using Common;
using System.Runtime.InteropServices;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace MUXControlsTestApp
{
    public static class Program
    {
        public static readonly string SetupWinRTDetoursParameter = "-SetupWinRTDetours";

        [DllImport("AppTestAutomationHelpers.dll")]
        private static extern void SetAsUnpackaged();

        [DllImport("Microsoft.WindowsAppRuntime.dll", CharSet = CharSet.Unicode, ExactSpelling = true)]
        internal static extern int WindowsAppRuntime_EnsureIsLoaded();

        [DllImport("Microsoft.UI.Xaml.dll")]
        private static extern void XamlCheckProcessRequirements();

        [STAThread]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Style", "IDE0060:Remove unused parameter", Justification = "Required parameter.")]
        public static void Main(string[] args)
        {
            // In 19H1 and above, WinRT activation is set up for us.  Below 19H1, we need to manually set up detours.
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.NineteenH1))
            {
                SetAsUnpackaged();
                _ = WindowsAppRuntime_EnsureIsLoaded();
            }

            XamlCheckProcessRequirements();

            WinRT.ComWrappersSupport.InitializeComWrappers();
            Microsoft.UI.Xaml.Application.Start((p) => {
                var context = new Microsoft.UI.Dispatching.DispatcherQueueSynchronizationContext(Microsoft.UI.Dispatching.DispatcherQueue.GetForCurrentThread());
                SynchronizationContext.SetSynchronizationContext(context);
                _ = new App();
            });
        }
    }

    /// <summary>
    /// Provides application-specific behavior to supplement the default Application class.
    /// This is defined globally to be able to remove it later.
    /// </summary>
    public sealed partial class App : Application
    {
        private const int SW_SHOWMAXIMIZED = 3;

        /// <summary>
        /// AdditionalStyles.xaml file for ScrollViewer tests
        /// </summary>
        /// 
        private static ResourceDictionary additionStylesXaml = null;
        public static ResourceDictionary AdditionStylesXaml
        {
            get
            {
                if (additionStylesXaml == null)
                {
                    additionStylesXaml = new ResourceDictionary();
                }

                return additionStylesXaml;
            }
        }
        
        /// <summary>
        /// Initializes the singleton application object.  This is the first line of authored code
        /// executed, and as such is the logical equivalent of main() or WinMain().
        /// </summary>
        public App()
        {
            this.InitializeComponent();
            this.Suspending += OnSuspending;

            if (AnalyticsInfo.VersionInfo.DeviceFamily == "Windows.Xbox")
            {
                RequiresPointerMode = ApplicationRequiresPointerMode.WhenRequested;
                this.FocusVisualKind = FocusVisualKind.Reveal;
            }

            // Instantiate this very early to exercise MaterialHelper's ability to be instantiated before XAML has a dispatcher.
            MaterialHelperTestApi.IgnoreAreEffectsFast = true;

            // OneCore and desktop have no splash screen, so we'll ignore the splash-screen requirement there.
            _isSplashScreenDismissed = true;
        }

        [DllImport("user32.dll")]
        static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

        [ThreadStatic] private static Window currentWindow = null;
        public static Window CurrentWindow
        {
            get
            {
                if (currentWindow == null)
                {
                    currentWindow = new Window() { Title = "MUXControlsTestApp.Desktop" };
                }

                return currentWindow;
            }
        }

        private static readonly AutoResetEvent appLaunchedEvent = new(false);
        public static AutoResetEvent AppLaunchedEvent
        {
            get
            {
                return appLaunchedEvent;
            }
        }

        public Frame RootFrame
        {
            get;
            set;
        }

        private string _currentLanguage = "en-US";

        public static string LanguageOverride
        {
            get
            {
                return ((App)Application.Current)._currentLanguage;
            }
            set
            {
                ((App)Application.Current)._currentLanguage = value;
            }
        }

        // Home for arbitrary test content so people don't muck with App.CurrentWindow.Content
        public static UIElement TestContentRoot
        {
            get
            {
                var rootFrame = App.CurrentWindow.Content as TestFrame;
                return rootFrame.Content as UIElement;
            }
            set
            {
                var rootFrame = App.CurrentWindow.Content as TestFrame;
                if (value != null)
                {
                    rootFrame.Content = value;
                }
                else
                {
                    rootFrame.NavigateWithoutAnimation(typeof(MainPage));
                }
            }
        }

        private static ResourceDictionary StyleOverridesPlaceholder
        {
            get
            {
                foreach (ResourceDictionary rd in ((App)Application.Current).Resources.MergedDictionaries)
                {
                    // NOTE: This is just a loose contract with the structure of app.xaml.
                    if (rd.Source == null)
                    {
                        return rd;
                    }
                }

                throw new Exception("App.xaml needs a placeholder ResourceDictionary for test to dynamically add entries to");
            }
        }

        public static bool DisableLongAnimations
        {
            get
            {
                return StyleOverridesPlaceholder.MergedDictionaries.Count != 0;
            }
            set
            {
#if !INNERLOOP_BUILD // The xaml files below need to be factored better into appropriate feature area projects - Tracked by Issue: 1044 
                if (value != DisableLongAnimations)
                {
                    if (value)
                    {
                        AppendResourceToMergedDictionaries("DisableAnimationsStyles.xaml", StyleOverridesPlaceholder);
                    }
                    else
                    {
                        StyleOverridesPlaceholder.MergedDictionaries.Clear();
                    }
                }
#endif
            }
        }

        private bool _isSplashScreenDismissed;
        private bool _isRootCreated = false;
        private List<Action> _actionsToRunAfterSplashScreenDismissedAndRootIsCreated = new List<Action>();


        public static void RunAfterSplashScreenDismissed(Action action)
        {
            var app = Application.Current as App;
            lock (app._actionsToRunAfterSplashScreenDismissedAndRootIsCreated)
            {
                if (app._isSplashScreenDismissed && app._isRootCreated)
                {
                    action();
                }
                else
                {
                    app._actionsToRunAfterSplashScreenDismissedAndRootIsCreated.Add(action);
                }
            }
        }

        private void SplashScreen_Dismissed(Windows.ApplicationModel.Activation.SplashScreen sender, object args)
        {
            _isSplashScreenDismissed = true;
            if (_isRootCreated)
            {
                SplashScreenDismissedAndRootCreated();
            }
        }

        private void SplashScreenDismissedAndRootCreated()
        {
            lock (_actionsToRunAfterSplashScreenDismissedAndRootIsCreated)
            {
                foreach (var action in _actionsToRunAfterSplashScreenDismissedAndRootIsCreated)
                {
                    action();
                }
                _actionsToRunAfterSplashScreenDismissedAndRootIsCreated.Clear();
            }
        }

        /// <summary>
        /// Invoked when the application is launched normally by the end user.  Other entry points
        /// will be used such as when the application is launched to open a specific file.
        /// </summary>
        /// <param name="e">Details about the launch request and process.</param>
        protected override void OnLaunched(Microsoft.UI.Xaml.LaunchActivatedEventArgs e)
        {
            MUXControlsTestApp.Utilities.IdleSynchronizer.Init();

            _isRootCreated = false;
            // Load the resource dictionary now
            // Since the resource is only available with ScrollView feature enabled, try this but expect it to fail sometimes
            AppendResourceToMergedDictionaries("AdditionalStyles.xaml");

            // For test purposes, add styles that disable long animations.
            DisableLongAnimations = true;

#if DEBUG
            if (System.Diagnostics.Debugger.IsAttached)
            {
                this.DebugSettings.EnableFrameRateCounter = false;
                this.DebugSettings.BindingFailed += (object sender, BindingFailedEventArgs args) =>
                {
                    Debug.WriteLine(args.Message);
                };
            }
#endif

            MaterialHelperTestApi.IgnoreAreEffectsFast = false;
            GC.Collect();

            Action createRoot = () =>
            {
                var rootFrame = App.CurrentWindow.Content as TestFrame;

                // Do not repeat app initialization when the Window already has content,
                // just ensure that the window is active
                if (rootFrame == null)
                {
                    // Create a Frame to act as the navigation context and navigate to the first page
                    rootFrame = new TestFrame(typeof(MainPage));

                    rootFrame.NavigationFailed += OnNavigationFailed;

                    App.CurrentWindow.Content = rootFrame;
                    rootFrame.NavigateWithoutAnimation(typeof(MainPage));
                }
                _isRootCreated = true;
                if (_isSplashScreenDismissed)
                {
                    SplashScreenDismissedAndRootCreated();
                }
            };

            // To exercise a couple different ways of setting up the tree, when run in APPX test mode then delay-attach the root.
            // TODO: Use e.Arguments instead of Environment.GetCommandLineArgs() when 28995344 is fixed.
            if (Environment.GetCommandLineArgs().Length <= 1)
            {
                createRoot();
            }
            else
            {   
                var dispatcherQueue = DispatcherQueue.GetForCurrentThread();
                if (dispatcherQueue == null)
                {
                    throw new Exception("DispatcherQueue not available");
                }
                   
                System.Threading.Tasks.Task.Delay(2000).ContinueWith(
                    (t) => {
                            var ignored = dispatcherQueue.TryEnqueue(DispatcherQueuePriority.Normal,
                            () =>
                            {
                                createRoot();
                            });
                    });
            }

            App.CurrentWindow.Activated += OnWindowActivated;

            // Ensure the current window is active
            App.CurrentWindow.Activate();

            ShowWindow(Process.GetCurrentProcess().MainWindowHandle, SW_SHOWMAXIMIZED);

            // By default Verify throws exception on errors and exceptions cause TAEF AppX tests to fail in non-graceful ways
            // (we get the test failure and then TE keeps trying to talk to the crashing process so we get "TE session timed out" errors too).
            // Just disable exceptions in this scenario.
            Verify.DisableVerifyFailureExceptions = true;
            appLaunchedEvent.Set();
        }

        /// <summary>
        /// Invoked when Navigation to a certain page fails
        /// </summary>
        /// <param name="sender">The Frame which failed navigation</param>
        /// <param name="e">Details about the navigation failure</param>
        void OnNavigationFailed(object sender, NavigationFailedEventArgs e)
        {
            throw new Exception("Failed to load Page " + e.SourcePageType.FullName);
        }

        /// <summary>
        /// Invoked when application execution is being suspended.  Application state is saved
        /// without knowing whether the application will be terminated or resumed with the contents
        /// of memory still intact.
        /// </summary>
        /// <param name="sender">The source of the suspend request.</param>
        /// <param name="e">Details about the suspend request.</param>
        private void OnSuspending(object sender, SuspendingEventArgs e)
        {
            var deferral = e.SuspendingOperation.GetDeferral();
            //TODO: Save application state and stop any background activity
            deferral.Complete();
        }

        private void OnWindowActivated(object sender, Microsoft.UI.Xaml.WindowActivatedEventArgs e)
        {

            if (e.WindowActivationState == WindowActivationState.Deactivated)
            {
                Log.Warning("Test window lost focus.");
            }
            else
            {
                Log.Comment("Test window got focus.");
            }
        }
        
        private static void AppendResourceToMergedDictionaries(string resource, ResourceDictionary targetDictionary = null)
        {
            ResourceDictionary resourceDictionary = new ResourceDictionary();
            Application.LoadComponent(resourceDictionary, new Uri(
                "ms-appx:///Themes/"+ resource), ComponentResourceLocation.Nested);
            (targetDictionary ?? Application.Current.Resources).MergedDictionaries.Add(resourceDictionary);
        }

        public static void AppendResourceDictionaryToMergedDictionaries(ResourceDictionary dictionary)
        {
            // Check for null and dictionary not present
            if (!(dictionary is null) && 
                !Application.Current.Resources.MergedDictionaries.Contains(dictionary))
            {
                Application.Current.Resources.MergedDictionaries.Add(dictionary);
            }
        }

        public static void RemoveResourceDictionaryFromMergedDictionaries(ResourceDictionary dictionary)
        {
            // Check for null and dictionary is in list
            if(!(dictionary is null) &&
                Application.Current.Resources.MergedDictionaries.Contains(dictionary))
            { 
                Application.Current.Resources.MergedDictionaries.Remove(dictionary);
            }
        }
    }
}
