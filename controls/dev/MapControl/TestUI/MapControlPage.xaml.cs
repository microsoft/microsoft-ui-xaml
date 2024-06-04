// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Markup;
using Windows.UI;
using System.Windows.Input;
using Windows.Devices.Geolocation;
using System.Collections.Generic;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "MapControl")]
    public sealed partial class MapControlPage : TestPage
    {
        public MapControlPage()
        {
            this.InitializeComponent();

            myMap.MapServiceToken = MapControlApiKey.AzureMapsToken;
            
            BasicGeoposition firstCenterPosition = new BasicGeoposition() { Latitude = 0, Longitude = 0 };
            Geopoint firstCenterPoint = new Geopoint(firstCenterPosition);
            myMap.Center = firstCenterPoint;

            myMap.MapElementClick += Map_MapElementClick;
            myMap.MapServiceErrorOccurred += Map_MapServiceErrorOccurred;
        }

        private void GoButton_Click(object sender, RoutedEventArgs e)
        {
            myMap.MapServiceToken = MapServiceToken.Password;
        }

        private void Map_MapServiceErrorOccurred(MapControl sender, MapControlMapServiceErrorOccurredEventArgs args)
        {
            output.Text += args.DiagnosticMessage;
        }

        private void ToggleSwitch_Toggled(object sender, RoutedEventArgs e)
        {
            ToggleSwitch toggleSwitch = sender as ToggleSwitch;
            if (toggleSwitch != null)
            {
                myMap.InteractiveControlsVisible = toggleSwitch.IsOn;
            }
        }
        private void CenterButton_Click(object sender, RoutedEventArgs e)
        {
            BasicGeoposition centerPosition = new BasicGeoposition()
            { 
                Latitude = double.Parse(latitudeText.Text), 
                Longitude = double.Parse(longitudeText.Text)
            };
            Geopoint centerPoint = new Geopoint(centerPosition);
            myMap.Center = centerPoint;
        }
        private void SetZoom_Click(object sender, RoutedEventArgs e)
        {
            myMap.ZoomLevel = double.Parse(zoomText.Text);
        }
        private void AddPin_Click(object sender, RoutedEventArgs e)
        {
            BasicGeoposition position = new BasicGeoposition()
            {
                Latitude = double.Parse(latitudePinText.Text),
                Longitude = double.Parse(longitudePinText.Text)
            };
            Geopoint point = new Geopoint(position);

            var icon = new MapIcon
            {
                Location = point,
            };

            var selectedLayer = layersSelection.SelectedItem as MapElementsLayer;
            selectedLayer.MapElements.Add(icon);
        }
        private void AddLayer_Click(object sender, RoutedEventArgs e)
        {
            var newLayer = new MapElementsLayer{};

            newLayer.MapElementClick += Layer_MapElementClick;
            myMap.Layers.Add(newLayer);
            layersSelection.Items.Add(newLayer);
            if(layersSelection.SelectedIndex == -1)
            {
                layersSelection.SelectedIndex = 0;
            }
        }

        private void Layer_MapElementClick(MapElementsLayer sender, MapElementClickEventArgs args)
        {
            var icon = args.Element as MapIcon;
            sender.MapElements.Remove(icon);
            output.Text += "Layer Clicked\n";            
        }
        private void Map_MapElementClick(MapControl sender, MapElementClickEventArgs args)
        {
            output.Text += "Map Clicked\n";
        }
    }
}
