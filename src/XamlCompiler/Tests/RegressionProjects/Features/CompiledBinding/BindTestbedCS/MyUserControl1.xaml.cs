// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.UI.Xaml.Controls;

namespace BindTestbed
{
    internal sealed partial class MyUserControl1 : UserControl
    {
        public new string Tag
        {
            get { return base.Tag as string; }
            set { base.Tag = value; }
        }

        public MyUserControl1()
        {
            this.InitializeComponent();
            DetectLeaksPage.TrackObject(this);
        }

        private void aLazyTextBlock_Tapped(object sender, Microsoft.UI.Xaml.Input.TappedRoutedEventArgs e)
        {
        }
    }
}