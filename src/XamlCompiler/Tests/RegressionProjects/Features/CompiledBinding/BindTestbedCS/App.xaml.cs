// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Navigation;

using BindTestbedModel;
//using BindTestbedCXModel;

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
        protected override void OnLaunched(Microsoft.UI.Xaml.LaunchActivatedEventArgs args)
        {
            Frame rootFrame = new Frame();
            rootFrame.NavigationFailed += OnNavigationFailed;
            m_window = new Window();
            m_window.Content = rootFrame;
            rootFrame.Navigate(typeof(MainPage), args.Arguments);
            m_window.Activate();
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
        //TODO: Convert BindTestbedModelCX to C++/WinRT
        //internal static ModelCX ModelCX = new ModelCX();
    }
}
