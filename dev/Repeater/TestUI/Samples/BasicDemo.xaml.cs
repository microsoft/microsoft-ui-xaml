// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Linq;
using Windows.UI.Xaml.Controls;

#if !BUILD_WINDOWS
using RecyclingElementFactory = Microsoft.UI.Xaml.Controls.RecyclingElementFactory;
using SelectTemplateEventArgs = Microsoft.UI.Xaml.Controls.SelectTemplateEventArgs;
#endif

namespace MUXControlsTestApp.Samples
{
    public sealed partial class BasicDemo : Page
    {
        public BasicDemo()
        {
            this.InitializeComponent();
            goBackButton.Click += delegate { Frame.GoBack(); };
#if BUILD_WINDOWS
            repeater.ItemTemplate = (Windows.UI.Xaml.IElementFactory)elementFactory;
#else
            repeater.ItemTemplate = elementFactory;
#endif
            repeater.ItemsSource = Enumerable.Range(0, 10000).Select(x => x.ToString());
        }

        private void OnSelectTemplateKey(RecyclingElementFactory sender, SelectTemplateEventArgs args)
        {
            args.TemplateKey = (int.Parse(args.DataContext.ToString()) % 2 == 0) ? "even" : "odd";
        }
    }
}
