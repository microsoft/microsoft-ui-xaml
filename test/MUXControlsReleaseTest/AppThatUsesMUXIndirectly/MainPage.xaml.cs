using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

namespace AppThatUsesMUXIndirectly
{
    public sealed partial class MainPage : Page
    {
        // This app does NOT directly consume MUX, but it references a library and a nupkg that use MUX.
        public MainPage()
        {
            this.InitializeComponent();
            
            var userControl = new LibraryThatUsesMUX.TestUserControl1();
            layoutRoot.Children.Add(userControl);
            
            var muxcInterfaceImplementation = new RuntimeComponentThatUsesMUX.MUXCInterfaceImplementation();
            object dummy;
            muxcInterfaceImplementation.TryCreateAnimatedVisual(null, out dummy);
        }
    }
}
