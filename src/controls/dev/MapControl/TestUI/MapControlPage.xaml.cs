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
            
            BasicGeoposition firstCenterPosition = new BasicGeoposition() { Latitude = 0, Longitude = 0 };
            Geopoint firstCenterPoint = new Geopoint(firstCenterPosition);
            myMap.Center = firstCenterPoint;

            var iconLayer = new MapElementsLayer
            {
                MapElements = new List<MapElement>()
                {
                    new MapIcon
                    {
                        Location = new Geopoint(new BasicGeoposition
                        {
                            Latitude = 14.865162,
                            Longitude = -14.858078
                        }),
                    }
                }
            };
            myMap.Layers.Add(iconLayer);
        }

        private void GoButton_Click(object sender, RoutedEventArgs e)
        {
            myMap.MapServiceToken = MapServiceToken.Password;
        }
    }
}
