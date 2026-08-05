// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace ConsumerCs
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        private bool otherProviderLoaded = false;
        public MainPage()
        {
            this.InitializeComponent();
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            global::Microsoft.UI.Xaml.Markup.IXamlMetadataProvider ApplicationProvider = ConsumerCs.App.Current as global::Microsoft.UI.Xaml.Markup.IXamlMetadataProvider;
            global::Microsoft.UI.Xaml.Markup.IXamlType type = ApplicationProvider.GetXamlType("ProviderCs.MainPage");
            if (!otherProviderLoaded && type!=null)
            {
                throw new Exception();
            }
            if (!otherProviderLoaded)
            {
                global::Microsoft.UI.Xaml.Markup.IXamlMetadataProvider provider;
                provider = new global::ProviderCs.ProviderCs_XamlTypeInfo.XamlMetaDataProvider();
                ConsumerCs.App a = (ConsumerCs.App)(ConsumerCs.App.Current);
                a.AddOtherProvider(provider);
                otherProviderLoaded = true;
            }
            // Retry the operation
            type = ApplicationProvider.GetXamlType("ProviderCs.MainPage");
            if (type == null)
            {
                throw new Exception();
            }

            ProviderCs.MainPage.DoSomething();
            textBlock1.Text = ProviderCs.MainPage.GetTextToShow();
        }
    }
}
