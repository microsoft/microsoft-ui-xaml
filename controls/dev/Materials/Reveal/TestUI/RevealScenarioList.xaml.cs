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
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using Microsoft.UI.Xaml.Navigation;
using MUXControlsTestApp.Utilities;

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
