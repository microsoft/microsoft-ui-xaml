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

using Pager = Microsoft.UI.Xaml.Controls.Pager;
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "Pager")]
    public sealed partial class PagerPage : TestPage
    {
        public ObservableCollection<string> events = new ObservableCollection<string>();
        public PagerPage()
        {
            this.InitializeComponent();
            EventPresenter.ItemsSource = events;
            events.Add("Pager started at page " + ProtoPager.SelectedIndex);
            //ProtoPager.RegisterPropertyChangedCallback(PrototypePager.PagerDisplayModeProperty, new DependencyPropertyChangedCallback(OnPagerDisplayModeChange));
        }

        private void PrototypePager_PageChanged(PrototypePager sender, PageChangedEventArgs args)
        {
            events.Add(sender.Name + " changed to page " + args.CurrentPage);
        }
    }
}
