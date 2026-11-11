// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;

using BindTestbedModel;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace BindTestbed
{
    /// <summary>
    /// Provides application-specific behavior to supplement the default Application class.
    /// </summary>
    public partial class App : Application
    {
        /// <summary>
        /// Initializes the singleton application object.  This is the first line of authored code
        /// executed, and as such is the logical equivalent of main() or WinMain().
        /// </summary>
        public App()
        {
            this.InitializeComponent();
        }

        /// <summary>
        /// Invoked when the application is launched.
        /// </summary>
        /// <param name="args">Details about the launch request and process.</param>
        protected override async void OnLaunched(Microsoft.UI.Xaml.LaunchActivatedEventArgs args)
        {
            if (Array.IndexOf(Environment.GetCommandLineArgs(), "--test-static-template-bindings") >= 0)
            {
                UnhandledException += (sender, error) =>
                {
                    Console.Error.WriteLine("Unhandled static template binding test failure: " + error.Exception);
                    Environment.Exit(1);
                };
                int exitCode = 0;
                try
                {
                    await VerifyStaticTemplateBindingsAsync();
                    Console.WriteLine("Static template binding tests passed.");
                }
                catch (Exception error) when (error is InvalidOperationException ||
                    error is TimeoutException || error is COMException)
                {
                    Console.Error.WriteLine("Static template binding test failed: " + error);
                    exitCode = 1;
                }
                // Keep the dispatcher alive until the async result is reported; process exit
                // releases the test window without losing a failure during last-window shutdown.
                Environment.Exit(exitCode);
                return;
            }

            Frame rootFrame = new Frame();
            rootFrame.NavigationFailed += OnNavigationFailed;
            m_window = new Window();
            m_window.Content = rootFrame;
            rootFrame.Navigate(typeof(MainPage), args.Arguments);
            m_window.Activate();
        }

        private async Task VerifyStaticTemplateBindingsAsync()
        {
            var owner = new ListAndTemplateTests();
            var host = new StackPanel();
            foreach (string key in new[] { "StaticUntypedBindingTemplate", "StaticBindingTemplate" })
            {
                host.Children.Add(new ContentControl { ContentTemplate = (DataTemplate)owner.Resources[key] });
            }

            // Enter a real live tree with no item; synthesizing a data-context notification would
            // miss the original ContentControl activation failure, especially without x:DataType.
            var loaded = new TaskCompletionSource<bool>(TaskCreationOptions.RunContinuationsAsynchronously);
            RoutedEventHandler onLoaded = (sender, args) => loaded.TrySetResult(true);
            host.Loaded += onLoaded;
            m_window = new Window { Content = host };
            try
            {
                m_window.Activate();
                await loaded.Task.WaitAsync(TimeSpan.FromSeconds(30));
                host.UpdateLayout();
                foreach (ContentControl control in host.Children)
                {
                    if (control.Content != null || control.DataContext != null ||
                        FindTextBlock(control)?.Text != DataModel.StaticFunctionOnModelNoArgs())
                    {
                        throw new InvalidOperationException(
                            "An initially null-content static template did not display its function result.");
                    }
                }
                Console.WriteLine("Initial null-content templates passed.");

                await VerifyUntypedTemplateContextsAsync(host, owner);
                Console.WriteLine("Untyped template contexts and transitions passed.");
                VerifyUntypedTemplateComponents(owner);
                Console.WriteLine("Untyped template component entry points passed.");
                VerifyStaticTemplateBindings(owner);
                VerifyPhasedTemplateBindings(owner);
                Console.WriteLine("ProcessBindings and recycling passed.");
            }
            finally
            {
                host.Loaded -= onLoaded;
            }
        }

        private async Task WaitForTemplateUpdateAsync(StackPanel host)
        {
            // Observe after live-tree template connections and their dispatcher work complete.
            host.UpdateLayout();
            var ready = new TaskCompletionSource<bool>(TaskCreationOptions.RunContinuationsAsynchronously);
            if (!m_window.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low,
                () => ready.TrySetResult(true)))
            {
                throw new InvalidOperationException("Could not queue the template update observation.");
            }
            await ready.Task.WaitAsync(TimeSpan.FromSeconds(10));
        }

        private async Task VerifyUntypedTemplateContextsAsync(StackPanel host, ListAndTemplateTests owner)
        {
            var template = (DataTemplate)owner.Resources["StaticUntypedBindingTemplate"];
            string expected = DataModel.StaticFunctionOnModelNoArgs();
            var contentControl = new ContentControl { ContentTemplate = template, Content = "ordinary item" };
            var inheritedControl = new ContentControl { ContentTemplate = template };
            var inheritedHost = new StackPanel { DataContext = FakeSettings.Instance };
            inheritedHost.Children.Add(inheritedControl);
            host.Children.Add(contentControl);
            host.Children.Add(inheritedHost);
            // Exercise the real ContentPresenter's item and inheritance paths. Neither may
            // require conversion to the synthetic DataTemplate root when no binding uses it.
            await WaitForTemplateUpdateAsync(host);
            if (FindTextBlock(contentControl)?.Text != expected)
            {
                throw new InvalidOperationException("An untyped static template failed with non-null Content.");
            }
            if (inheritedControl.Content != null || inheritedControl.DataContext == null ||
                FindTextBlock(inheritedControl)?.Text != expected)
            {
                throw new InvalidOperationException("An untyped static template failed with inherited non-null context.");
            }

            var transitionControl = (ContentControl)host.Children[0];
            foreach (object item in new object[] { "replacement item", null })
            {
                transitionControl.Content = item;
                await WaitForTemplateUpdateAsync(host);
                if (FindTextBlock(transitionControl)?.Text != expected)
                {
                    throw new InvalidOperationException("An untyped static template failed across null/item/null Content.");
                }
            }
            foreach (object item in new object[] { null, new FakeSettings(), null })
            {
                // Context changes reuse the visual, so overwrite its previous result to
                // distinguish a fresh update from stale text left by earlier initialization.
                FindTextBlock(inheritedControl).Text = "not updated";
                inheritedHost.DataContext = item;
                await WaitForTemplateUpdateAsync(host);
                if (FindTextBlock(inheritedControl)?.Text != expected)
                {
                    throw new InvalidOperationException("An untyped static template skipped a context transition.");
                }
            }

            var ordinary = (TextBlock)((DataTemplate)owner.Resources["InstanceOnlyBindingTemplate"]).LoadContent();
            var settings = new FakeSettings();
            ordinary.DataContext = settings;
            settings.BoolSetting = true;
            if (!Equals(ordinary.Tag, true))
            {
                throw new InvalidOperationException("An ordinary item binding did not receive its source update.");
            }
            ordinary.Tag = false;
            if (settings.BoolSetting)
            {
                throw new InvalidOperationException("An ordinary item binding lost its reverse assignment.");
            }
        }

        private static void VerifyUntypedTemplateComponents(ListAndTemplateTests owner)
        {
            var template = (DataTemplate)owner.Resources["StaticUntypedPhasedBindingTemplate"];
            foreach (object item in new object[] { null, "ordinary item", new FakeSettings() })
            {
                var root = (TextBlock)template.LoadContent();
                var component = XamlBindingHelper.GetDataTemplateComponent(root);
                // The component owns item delivery and phases; no fake DataContext change may
                // conceal its own placeholder cast or initialize a deferred phase early.
                component.ProcessBindings(item, 0, 0, out int nextPhase);
                if (nextPhase != 1 || root.Text != "")
                {
                    throw new InvalidOperationException("The untyped component did not defer phase one.");
                }
                component.ProcessBindings(item, 0, 1, out nextPhase);
                if (nextPhase != -1 || root.Text != DataModel.StaticFunctionOnModelNoArgs())
                {
                    throw new InvalidOperationException("The untyped component failed to update its independent binding.");
                }
                component.Recycle();
            }
        }

        private static TextBlock FindTextBlock(DependencyObject root)
        {
            if (root is TextBlock text)
            {
                return text;
            }
            for (int index = 0; index < VisualTreeHelper.GetChildrenCount(root); index++)
            {
                var child = FindTextBlock(VisualTreeHelper.GetChild(root, index));
                if (child != null)
                {
                    return child;
                }
            }
            return null;
        }

        private static void VerifyPhasedTemplateBindings(ListAndTemplateTests owner)
        {
            var template = (DataTemplate)owner.Resources["StaticPhasedBindingTemplate"];
            var root = (StackPanel)template.LoadContent();
            var itemTarget = (TextBlock)root.Children[0];
            var staticTarget = (TextBlock)root.Children[1];
            var functionTarget = (TextBlock)root.Children[2];
            var component = XamlBindingHelper.GetDataTemplateComponent(root);
            var settings = FakeSettings.Instance;
            settings.BoolSetting = false;

            // The component, rather than DataContext inheritance, owns the item and phase schedule.
            component.ProcessBindings(settings, 0, 0, out int nextPhase);
            if (nextPhase != 1 || itemTarget.Tag != null || (bool)staticTarget.Tag)
            {
                throw new InvalidOperationException("Phase zero did not defer the item binding to phase one.");
            }
            root.DataContext = new FakeSettings { BoolSetting = true };
            component.ProcessBindings(settings, 0, 1, out nextPhase);
            if (nextPhase != -1 || (bool)itemTarget.Tag ||
                functionTarget.Text != DataModel.StaticFunctionOnModelNoArgs())
            {
                throw new InvalidOperationException("Phase one did not complete the component's item/static function bindings.");
            }

            // Recycling must detach both aliased listener paths, then rebuild independent and new
            // item paths when the same template instance is assigned a different item.
            component.Recycle();
            settings.BoolSetting = true;
            if ((bool)itemTarget.Tag || (bool)staticTarget.Tag)
            {
                throw new InvalidOperationException("A recycled component still received source notifications.");
            }
            var replacement = new FakeSettings();
            component.ProcessBindings(replacement, 1, 0, out nextPhase);
            component.ProcessBindings(replacement, 1, 1, out nextPhase);
            // Check restoration before the singleton returns to the target's old value.
            if ((bool)itemTarget.Tag || !(bool)staticTarget.Tag)
            {
                throw new InvalidOperationException("Reusing the component did not restore its static source's current value.");
            }
            replacement.BoolSetting = true;
            settings.BoolSetting = false;
            if (!(bool)itemTarget.Tag || (bool)staticTarget.Tag)
            {
                throw new InvalidOperationException("A recycled component did not reconnect independent and replacement-item listeners.");
            }

            component.Recycle();
        }

        private static void VerifyStaticTemplateBindings(ListAndTemplateTests owner)
        {
            Console.WriteLine("Testing observable source notifications.");
            // Use the same observable object as both the item and a static source so notifications
            // after item removal can expose stale item paths without disabling the static paths.
            var dataTemplate = (DataTemplate)owner.Resources["StaticObservableBindingTemplate"];
            var root = (StackPanel)dataTemplate.LoadContent();
            var itemTarget = (TextBlock)root.Children[0];
            var staticTarget = (TextBlock)root.Children[1];
            var settings = FakeSettings.Instance;
            settings.BoolSetting = false;
            root.DataContext = settings;
            settings.BoolSetting = true;
            Console.WriteLine("Observable source notifications initialized.");
            if (!(bool)itemTarget.Tag || !(bool)staticTarget.Tag)
            {
                throw new InvalidOperationException("The item and static bindings did not receive the initial notification.");
            }

            root.DataContext = null;
            settings.BoolSetting = false;
            Console.WriteLine("Observable source null transition completed.");
            if (!(bool)itemTarget.Tag || (bool)staticTarget.Tag)
            {
                throw new InvalidOperationException("A static-source notification updated the removed item binding.");
            }

            // Force a target callback while the item is absent, then verify the static reverse path.
            itemTarget.Tag = false;
            itemTarget.Tag = true;
            if (settings.BoolSetting)
            {
                throw new InvalidOperationException("A TwoWay target wrote to the removed item.");
            }

            staticTarget.Tag = true;
            if (!settings.BoolSetting)
            {
                throw new InvalidOperationException("The independent static TwoWay binding stopped updating its source.");
            }

            // Supplying the item again must rebuild the listeners that removal detached.
            root.DataContext = settings;
            settings.BoolSetting = false;
            if ((bool)itemTarget.Tag || (bool)staticTarget.Tag)
            {
                throw new InvalidOperationException("The bindings did not reconnect when the item was restored.");
            }

            // Named sources belong to the template namescope, not its item; their listeners must
            // survive both replacement and removal after deferred creation through FindName.
            var namedTemplate = (DataTemplate)owner.Resources["StaticNamedSourceTemplate"];
            Console.WriteLine("Testing deferred named source.");
            var namedRoot = (StackPanel)namedTemplate.LoadContent();
            var deferredSource = (TextBlock)namedRoot.FindName("deferredSource");
            var namedTarget = (TextBlock)namedRoot.FindName("namedTarget");
            namedRoot.DataContext = settings;
            deferredSource.Text = "connected";
            if (namedTarget.Text != "connected")
            {
                throw new InvalidOperationException("The deferred named source did not connect.");
            }

            namedRoot.DataContext = new FakeSettings();
            deferredSource.Text = "after replacement";
            if (namedTarget.Text != "after replacement")
            {
                throw new InvalidOperationException("The independent named-source listener was not restored after item replacement.");
            }

            namedRoot.DataContext = null;
            deferredSource.Text = "after null";
            if (namedTarget.Text != "after null")
            {
                throw new InvalidOperationException("The independent named-source listener was not restored after item removal.");
            }
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

        private Window m_window;
        internal static DataModel Model = new DataModel();
        internal static DOModel DOModel = new DOModel();
        internal static LanguageSpecific LanguageModel = new LanguageSpecific();
    }
}
