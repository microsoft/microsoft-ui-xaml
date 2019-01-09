// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Markup;
using Windows.UI;
using System.Windows.Input;
using System.Collections.ObjectModel;

#if !BUILD_WINDOWS
using Forms = Microsoft.UI.Xaml.Controls.Forms;
using ElementFactoryGetArgs = Microsoft.UI.Xaml.Controls.ElementFactoryGetArgs;
using ElementFactoryRecycleArgs = Microsoft.UI.Xaml.Controls.ElementFactoryRecycleArgs;
#endif

namespace MUXControlsTestApp
{
    public class FormsElementFactory : Microsoft.UI.Xaml.Controls.IElementFactoryShim
    {
        public UIElement GetElement(ElementFactoryGetArgs args)
        {
            // Hopefully.... this is the thing.
            return args.Data as UIElement;
        }

        public void RecycleElement(ElementFactoryRecycleArgs args)
        {
            // do something?
        }
    }

    public sealed partial class FormsPage : TestPage
    {
        public FormsPage()
        {
            this.InitializeComponent();

#if BUILD_WINDOWS
            Repeater.ItemTemplate = (Windows.UI.Xaml.IElementFactory)elementFactory;
#else
            Repeater.ItemTemplate = (Microsoft.UI.Xaml.Controls.IElementFactoryShim)elementFactory;
#endif

            var items = new ObservableCollection<FrameworkElement>();
            
            items.Add(new TextBox() { Header = "First Name" });
            items.Add(new TextBox() { Header = "Last Name" });

            items.Add(new TextBox() { Header = "M.I" });
            items.Add(new TextBox() { Header = "Title" });

            items.Add(new TextBox() { Header = "Nickname" });

            Repeater.ItemsSource = items;
        }
    }
}
