// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
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
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Navigation;

namespace MUXControlsTestApp
{
    public sealed partial class RevealScenarioList : TestPage
    {
        private ObservableCollection<RevealGridItem> listSource;
        private ObservableCollection<RevealBackgroundItem> backgroundTypeSource;

        public RevealScenarioList()
        {
            this.InitializeComponent();

            listSource = RevealGridItem.GetItems();
            backgroundTypeSource = RevealBackgroundItem.GetItems();
        }

        private void BackgroundComboBoxChanged(object sender, SelectionChangedEventArgs e)
        {
            ComboBox box = (ComboBox)sender;
            RevealBackgroundItem item = (RevealBackgroundItem)box.SelectedItem;
            VisualStateManager.GoToState(this, item.Name, false);
        }
    }

}
