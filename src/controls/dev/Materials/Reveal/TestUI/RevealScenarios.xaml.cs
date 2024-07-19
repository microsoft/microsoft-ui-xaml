﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
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
using MUXControlsTestApp.Utilities;

namespace MUXControlsTestApp
{
    public sealed partial class RevealScenarios : TestPage
    {
        private ObservableCollection<RevealScenarioItem> listSource;
        ObservableCollection<RevealThemeItem> themeSource;
        public RevealScenarios()
        {
            this.InitializeComponent();

            listSource = new ObservableCollection<RevealScenarioItem>();
            listSource.Add(new RevealScenarioItem { Header = "Adjust Lights", SubHeader = "Click to adjust light props", Icon = "\uE105", Page = typeof(RevealScenarioLights) });
            listSource.Add(new RevealScenarioItem { Header = "Adjust Colors", SubHeader = "Click to adjust color props", Icon = "\uE105", Page = typeof(RevealScenarioColors) });
            listSource.Add(new RevealScenarioItem { Header = "Grid Test", SubHeader = "Click to see GridView tests", Icon = "\uE105", Page = typeof(RevealScenarioGrid) });
            listSource.Add(new RevealScenarioItem { Header = "List Test", SubHeader = "Click to see ListView", Icon = "\uE105", Page = typeof(RevealScenarioList) });
            listSource.Add(new RevealScenarioItem { Header = "Make another view", SubHeader = "Make secondary views", Icon = "\uE105", Page = typeof(RevealScenarioSecondaryView) });
            listSource.Add(new RevealScenarioItem { Header = "Test Item 4", SubHeader = "Click to do nothing", Icon = "\uE105" });

            themeSource = new ObservableCollection<RevealThemeItem>();
            foreach (ElementTheme theme in Enum.GetValues(typeof(ElementTheme)))
            {
                themeSource.Add(new RevealThemeItem { Name = Enum.GetName(typeof(ElementTheme), theme), Theme = theme });
            }
        }

        private void RevealList_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ListView list = (ListView)sender;
            RevealScenarioItem item = (RevealScenarioItem)list.SelectedItem;
            if (item.Page != null)
            {
                ContentFrame.NavigateWithoutAnimation(item.Page);
            }
        }

        private void ComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ComboBox box = (ComboBox)sender;
            this.RequestedTheme = ((RevealThemeItem)box.SelectedItem).Theme;
        }

        private void ToggleSwitch_Toggled(object sender, RoutedEventArgs e)
        {
            ToggleSwitch toggle = (ToggleSwitch)sender;
            if (toggle.IsOn)
            {
                VisualStateManager.GoToState(this, "UseAcrylic", false);
            }
            else
            {
                VisualStateManager.GoToState(this, "NoAcrylic", false);
            }
        }
    }
    public class RevealScenarioItem
    {
        public string Header;
        public string SubHeader;
        public string Icon;
        public Type Page;
    }

    public class RevealThemeItem
    {
        public string Name;
        public ElementTheme Theme;
    }
}
