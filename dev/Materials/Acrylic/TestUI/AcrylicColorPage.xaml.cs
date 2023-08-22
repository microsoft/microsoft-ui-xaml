// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using Windows.Foundation.Metadata;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Markup;

namespace MUXControlsTestApp
{
    public sealed partial class AcrylicColorPage : TestPage
    {
        public AcrylicColorPage()
        {
            this.InitializeComponent();

            foreach (string brushName in AcrylicBrushNames)
            {
                var item = 
                    XamlReader.Load(
                        "<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Margin='5' Background='{ThemeResource " + brushName + "}'>" +
                            "<TextBlock Margin='3' FontSize='12' MinWidth='200' MinHeight='36' MaxHeight='330' TextWrapping='Wrap' Text='" + brushName + "'/>" +
                        "</Grid>");

                BackgroundList.Items.Add(item);
            }
        }

        public static IEnumerable<string> AcrylicBrushNames
        {
            get
            {
                return new List<string>() {
                    "SystemControlAcrylicWindowBrush",
                    "SystemControlAcrylicElementBrush",
                    "SystemControlAccentAcrylicWindowAccentMediumHighBrush",
                    "SystemControlAccentAcrylicElementAccentMediumHighBrush",
                    "SystemControlAccentDark1AcrylicWindowAccentDark1Brush",
                    "SystemControlAccentDark1AcrylicElementAccentDark1Brush",
                    "SystemControlAccentDark2AcrylicWindowAccentDark2MediumHighBrush",
                    "SystemControlAccentDark2AcrylicElementAccentDark2MediumHighBrush",
                    "SystemControlAcrylicWindowMediumHighBrush",
                    "SystemControlAcrylicElementMediumHighBrush",
                    "SystemControlChromeMediumLowAcrylicWindowMediumBrush",
                    "SystemControlChromeMediumLowAcrylicElementMediumBrush",
                    "SystemControlBaseHighAcrylicWindowBrush",
                    "SystemControlBaseHighAcrylicElementBrush",
                    "SystemControlBaseHighAcrylicWindowMediumHighBrush",
                    "SystemControlBaseHighAcrylicElementMediumHighBrush",
                    "SystemControlBaseHighAcrylicWindowMediumBrush",
                    "SystemControlBaseHighAcrylicElementMediumBrush",
                    "SystemControlChromeLowAcrylicWindowBrush",
                    "SystemControlChromeLowAcrylicElementBrush",
                    "SystemControlChromeMediumAcrylicWindowMediumBrush",
                    "SystemControlChromeMediumAcrylicElementMediumBrush",
                    "SystemControlChromeHighAcrylicWindowMediumBrush",
                    "SystemControlChromeHighAcrylicElementMediumBrush",
                    "SystemControlBaseLowAcrylicWindowBrush",
                    "SystemControlBaseLowAcrylicElementBrush",
                    "SystemControlBaseMediumLowAcrylicWindowMediumBrush",
                    "SystemControlBaseMediumLowAcrylicElementMediumBrush",
                    "SystemControlAltLowAcrylicWindowBrush",
                    "SystemControlAltLowAcrylicElementBrush",
                    "SystemControlAltMediumLowAcrylicWindowMediumBrush",
                    "SystemControlAltMediumLowAcrylicElementMediumBrush",
                    "SystemControlAltHighAcrylicWindowBrush",
                    "SystemControlAltHighAcrylicElementBrush"
                };
            }
        }
        
        public static string GetBrushName(string fullName)
        {
            if (fullName == null || fullName.Length <= 25)
                return string.Empty;

            return fullName.Substring(20, fullName.Length - 25);
        }
    }
}
