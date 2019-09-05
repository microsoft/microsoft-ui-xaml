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

// The User Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234236

namespace LibraryThatUsesMUX
{
    public sealed partial class TestUserControl1 : UserControl
    {
        public TestUserControl1()
        {
            EnsureResources();
            this.InitializeComponent();
        }

        private static bool ensureResourcesComplete;

        private static void EnsureResources()
        {
            if (ensureResourcesComplete)
                return;
            var dic = Application.Current?.Resources?.MergedDictionaries;
            if (dic == null)
                return;
            if (!dic.Any(t => t is Microsoft.UI.Xaml.Controls.XamlControlsResources))
            {
                Application.Current.Resources.MergedDictionaries.Add(new Microsoft.UI.Xaml.Controls.XamlControlsResources());
            }
            ensureResourcesComplete = true;
        }
    }
}
