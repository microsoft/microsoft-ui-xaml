// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Threading.Tasks;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;

namespace MetadataTestbedCS
{
    public sealed partial class MainPage : Page
    {
        IXamlMetadataProvider provider;
        Type metadataProviderType;

        public object TestProperty { get; }

        public MainPage()
        {
            this.InitializeComponent();
            metadataProviderType = Type.GetType("MetadataTestbedCS.MetadataTestbedCS_XamlTypeInfo.XamlMetaDataProvider");
        }

        void GetTypeMemberManyTimesClicked(Object sender, Microsoft.UI.Xaml.RoutedEventArgs e)
        {
            GetTypeMemberButton.IsEnabled = false;
            for (int i = 0; i < 50000; i++)
            {
                provider = Activator.CreateInstance(metadataProviderType) as IXamlMetadataProvider;
                var t1 = Task.Run(() => GetTypeMemberTest());
                var t2 = Task.Run(() => GetTypeMemberTest());
                t1.Wait();
                t2.Wait();
            }
            GetTypeMemberButton.IsEnabled = true;
        }

        void GetTypeMemberTest()
        {
            var type = provider.GetXamlType("MetadataTestbedCS.MainPage");
            var member = type.GetMember("TestProperty");
            if (member == null)
            {
                throw new EntryPointNotFoundException("Test failed");
            }
        }
    }
}

