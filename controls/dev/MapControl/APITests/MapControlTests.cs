// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;
using Windows.Devices.Geolocation;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class MapControlTests : ApiTestBase
    {
        // Synthetic, secret-free token: these tests never need a real Azure Maps key or network access.
        private const string c_serviceToken = "mapcontrol-apitest-token-4f1c9e";

        [TestMethod]
        [TestProperty("Description", "Without a template, MapControl exposes its defaults, round-trips its properties and keeps its layer and element collections consistent through add, insert, replace, remove and clear.")]
        public void PropertiesLayersAndElementsWorkWithoutTemplate()
        {
            RunOnUIThreadGuarded(() =>
            {
                MapControl map = new MapControl();
                Verify.IsNotNull(map.Layers);
                Verify.AreEqual(0, map.Layers.Count);
                Verify.AreEqual(0.0, map.ZoomLevel);
                Verify.IsFalse(map.InteractiveControlsVisible);
                Verify.IsTrue(string.IsNullOrEmpty(map.MapServiceToken));
                Verify.IsNull(map.Center);

                map.ZoomLevel = 12.5;
                map.InteractiveControlsVisible = true;
                map.MapServiceToken = c_serviceToken;
                map.Center = CreatePoint(47.64, -122.13);
                Verify.AreEqual(12.5, map.ZoomLevel);
                Verify.IsTrue(map.InteractiveControlsVisible);
                Verify.AreEqual(c_serviceToken, map.MapServiceToken);
                Verify.AreEqual(47.64, map.Center.Position.Latitude);
                Verify.AreEqual(-122.13, map.Center.Position.Longitude);

                TypedEventHandlerCounter counter = new TypedEventHandlerCounter();
                map.MapElementClick += counter.OnMapElementClick;
                map.MapServiceErrorOccurred += counter.OnError;
                map.MapElementClick -= counter.OnMapElementClick;
                map.MapServiceErrorOccurred -= counter.OnError;

                MapElementsLayer first = CreateNamedLayer("first");
                MapElementsLayer middle = CreateNamedLayer("middle");
                MapElementsLayer last = CreateNamedLayer("last");
                map.Layers.Add(first);
                map.Layers.Add(middle);
                map.Layers.Add(last);
                Verify.AreEqual(3, map.Layers.Count);
                Verify.IsNotNull(first.MapElements);
                Verify.AreEqual(0, first.MapElements.Count);

                MapIcon a = new MapIcon() { Location = CreatePoint(1, 2) };
                MapIcon b = new MapIcon() { Location = CreatePoint(3, 4) };
                MapIcon c = new MapIcon() { Location = CreatePoint(5, 6) };
                MapIcon d = new MapIcon() { Location = CreatePoint(7, 8) };
                first.MapElements.Add(a);
                first.MapElements.Add(b);
                first.MapElements.Insert(0, c);
                VerifyElements(first, c, a, b);
                first.MapElements[1] = d;
                VerifyElements(first, c, d, b);
                first.MapElements.RemoveAt(0);
                VerifyElements(first, d, b);

                // Moving an icon that is already on a layer keeps its location value.
                b.Location = CreatePoint(9, 10);
                Verify.AreEqual(9.0, b.Location.Position.Latitude);
                Verify.AreEqual(10.0, b.Location.Position.Longitude);

                first.MapElements.Clear();
                Verify.AreEqual(0, first.MapElements.Count);
                first.MapElements.Add(a);
                VerifyElements(first, a);

                // Removing a layer that is not the last one.
                map.Layers.RemoveAt(1);
                Verify.AreEqual(2, map.Layers.Count);
                Verify.AreEqual("first", AutomationProperties.GetName(map.Layers[0]));
                Verify.AreEqual("last", AutomationProperties.GetName(map.Layers[1]));
                Verify.AreEqual(0, counter.Total);
            });
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // BUG-MAP-5: OnLayersVectorChanged reads Layers().GetAt(index) after the item was removed, so removing the last layer throws E_BOUNDS to the caller.
        [TestProperty("Description", "Removing the last (or only) layer from MapControl.Layers succeeds.")]
        public void RemovingLastLayerSucceeds()
        {
            RunOnUIThreadGuarded(() =>
            {
                MapControl map = new MapControl();
                map.Layers.Add(new MapElementsLayer());
                map.Layers.RemoveAt(0);
                Verify.AreEqual(0, map.Layers.Count);
            });
        }

        private static void VerifyElements(MapElementsLayer layer, params MapIcon[] expected)
        {
            Verify.AreEqual(expected.Length, layer.MapElements.Count);
            for (int i = 0; i < expected.Length; i++)
            {
                MapIcon actual = layer.MapElements[i] as MapIcon;
                Verify.IsNotNull(actual, "Element " + i + " is a MapIcon");
                Verify.AreEqual(expected[i].Location.Position.Latitude, actual.Location.Position.Latitude, "Element " + i + " latitude");
                Verify.AreEqual(expected[i].Location.Position.Longitude, actual.Location.Position.Longitude, "Element " + i + " longitude");
            }
        }

        private static MapElementsLayer CreateNamedLayer(string name)
        {
            MapElementsLayer layer = new MapElementsLayer();
            AutomationProperties.SetName(layer, name);
            return layer;
        }

        // An exception escaping a RunOnUIThread action terminates the test host; report it as a test failure instead.
        private static void RunOnUIThreadGuarded(Action action)
        {
            RunOnUIThread.Execute(() =>
            {
                try
                {
                    action();
                }
                catch (Exception e)
                {
                    Verify.Fail("Unexpected exception on the UI thread: " + e);
                }
            });
        }

        private static Geopoint CreatePoint(double latitude, double longitude)
        {
            return new Geopoint(new BasicGeoposition() { Latitude = latitude, Longitude = longitude });
        }

        private class TypedEventHandlerCounter
        {
            public int Total;

            public void OnMapElementClick(MapControl sender, MapElementClickEventArgs args)
            {
                Total++;
            }

            public void OnError(MapControl sender, MapControlMapServiceErrorOccurredEventArgs args)
            {
                Total++;
            }
        }
    }
}
