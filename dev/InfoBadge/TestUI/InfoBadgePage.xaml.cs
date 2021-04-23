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
using Microsoft.UI.Xaml.Controls;
using IconSource = Microsoft.UI.Xaml.Controls.IconSource;
using FontIconSource = Microsoft.UI.Xaml.Controls.FontIconSource;
using BitmapIconSource = Microsoft.UI.Xaml.Controls.BitmapIconSource;
using SymbolIconSource = Microsoft.UI.Xaml.Controls.SymbolIconSource;
using PathIconSource = Microsoft.UI.Xaml.Controls.PathIconSource;
using Windows.UI.Xaml.Media.Imaging;
using Microsoft.UI.Xaml.Controls.AnimatedVisuals;
using ColorChangedEventArgs = Microsoft.UI.Xaml.Controls.ColorChangedEventArgs;

//using InfoBadge = Microsoft.UI.Xaml.Controls.InfoBadge;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "InfoBadge")]
    public sealed partial class InfoBadgePage : TestPage
    {
        IconSource pageIconSource = null;
        SolidColorBrush iconForegroundBrush = new SolidColorBrush();

        public InfoBadgePage()
        {
            this.InitializeComponent();
            iconForegroundBrush.Color = IconForegroundColorPicker.Color;
        }

        public void SwapIsOpenButtonClicked(object sender, RoutedEventArgs args)
        {
            SwapIsOpen(InitiallyClosedInfoBadge);
            SwapIsOpen(InitiallyOpenedInfoBadge);
        }

        public void IconComboBoxSelectionChanged(object sender, SelectionChangedEventArgs args)
        {
            if (IconComboBox.SelectedItem == this.NullIcon)
            {
                UpdateIcon(null);
            }
            else if (IconComboBox.SelectedItem == this.FontIcon)
            {
                UpdateIcon(MakeFontIcon());
            }
            else if (IconComboBox.SelectedItem == this.BitmapIcon)
            {
                UpdateIcon(MakeBitmapIcon());
            }
            else if (IconComboBox.SelectedItem == this.ImageIcon)
            {
                UpdateIcon(MakeImageIcon());
            }
            else if (IconComboBox.SelectedItem == this.SymbolIcon)
            {
                UpdateIcon(MakeSymbolIcon());
            }
            else if (IconComboBox.SelectedItem == this.PathIcon)
            {
                UpdateIcon(MakePathIcon());
            }
            else if (IconComboBox.SelectedItem == this.AnimatedIcon)
            {
                UpdateIcon(MakeAnimatedIcon());
            }
        }

        public void OnIconForegroundColorPickerColorChanged(object sender, ColorChangedEventArgs args)
        {
            iconForegroundBrush.Color = args.NewColor;
            pageIconSource.Foreground = iconForegroundBrush;
        }

        public void OnForegroundToNullButtonClicked(object sender, RoutedEventArgs args)
        {
            pageIconSource.Foreground = null;
        }

        private void SwapIsOpen(InfoBadge badge)
        {
            badge.IsOpen = !badge.IsOpen;
        }

        private void UpdateIcon(IconSource iconSource)
        {
            pageIconSource = iconSource;
            InitiallyClosedInfoBadge.IconSource = iconSource;
            InitiallyOpenedInfoBadge.IconSource = iconSource;
        }

        private FontIconSource MakeFontIcon()
        {
            FontIconSource iconSource = new FontIconSource();
            iconSource.Glyph = "99+";
            iconSource.FontFamily = new FontFamily("XamlAutoFontFamily");
            iconSource.Foreground = iconForegroundBrush;
            return iconSource;
        }

        private BitmapIconSource MakeBitmapIcon()
        {
            BitmapIconSource iconSource = new BitmapIconSource();
            iconSource.ShowAsMonochrome = true;
            Uri uri = new Uri("ms-appx:///Assets/ingredient2.png");
            iconSource.UriSource = uri;
            iconSource.Foreground = iconForegroundBrush;
            return iconSource;
        }

        private ImageIconSource MakeImageIcon()
        {
            ImageIconSource iconSource = new ImageIconSource();
            var uri = new Uri("ms-appx:///Assets/Nuclear_symbol.svg");
            iconSource.ImageSource = new SvgImageSource(uri);
            iconSource.Foreground = iconForegroundBrush;
            return iconSource;
        }

        private SymbolIconSource MakeSymbolIcon()
        {
            SymbolIconSource iconSource = new SymbolIconSource();
            iconSource.Symbol = Symbol.Setting;
            iconSource.Foreground = iconForegroundBrush;
            return iconSource;
        }

        private PathIconSource MakePathIcon()
        {
            PathIconSource iconSource = new PathIconSource();
            var geometry = new RectangleGeometry();
            geometry.Rect = new Windows.Foundation.Rect { Width=5, Height=5, X=0, Y=0 };
            iconSource.Data = geometry;
            iconSource.Foreground = iconForegroundBrush;
            return iconSource;
        }

        private AnimatedIconSource MakeAnimatedIcon()
        {
            AnimatedIconSource iconSource = new AnimatedIconSource();
            iconSource.Source = new AnimatedSettingsVisualSource();
            iconSource.FallbackIconSource = new SymbolIconSource { Symbol = Symbol.Setting, Foreground = iconForegroundBrush };
            iconSource.Foreground = iconForegroundBrush;
            return iconSource;
        }
    }
}
