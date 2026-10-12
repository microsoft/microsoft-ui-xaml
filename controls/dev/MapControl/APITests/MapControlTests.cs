// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
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

        [TestMethod]
        [TestProperty("Description", "Without a template, clearing MapControl.Layers empties it and it can be refilled; Layers and MapElementsLayer.MapElements can be replaced with app collections, which the getters then return; clearing a layer's replaced original MapElements collection leaves the current elements untouched.")]
        public void LayersAndElementsCanBeReplacedAndCleared()
        {
            RunOnUIThreadGuarded(() =>
            {
                MapControl map = new MapControl();
                IList<MapLayer> original = map.Layers;
                map.Layers.Add(CreateNamedLayer("a"));
                map.Layers.Add(CreateNamedLayer("b"));
                Verify.AreEqual(2, map.Layers.Count);

                map.Layers.Clear();
                Verify.AreEqual(0, map.Layers.Count, "Clear empties the layers");
                map.Layers.Add(CreateNamedLayer("c"));
                Verify.AreEqual(1, map.Layers.Count, "Only the layer added after Clear remains");
                Verify.AreEqual("c", AutomationProperties.GetName(map.Layers[0]));

                List<MapLayer> replacement = new List<MapLayer>() { CreateNamedLayer("x"), CreateNamedLayer("y") };
                map.Layers = replacement;
                Verify.AreEqual(2, map.Layers.Count, "The getter returns the assigned collection");
                Verify.AreEqual("x", AutomationProperties.GetName(map.Layers[0]));
                Verify.AreEqual("y", AutomationProperties.GetName(map.Layers[1]));
                Verify.AreEqual(1, original.Count, "The original collection is not modified by the replacement");
                Verify.AreEqual("c", AutomationProperties.GetName(original[0]));

                // A layer owned by a (still untemplated) map listens to its original MapElements vector.
                MapControl owner = new MapControl();
                MapElementsLayer layer = CreateNamedLayer("owned");
                Verify.AreEqual(0, layer.MapElements.Count);
                owner.Layers.Add(layer);
                IList<MapElement> originalElements = layer.MapElements;
                MapIcon first = new MapIcon() { Location = CreatePoint(9, 10) };
                originalElements.Add(first);
                VerifyElements(layer, first);

                MapIcon replacementA = new MapIcon() { Location = CreatePoint(11, 12) };
                MapIcon replacementB = new MapIcon() { Location = CreatePoint(13, 14) };
                List<MapElement> elements = new List<MapElement>() { replacementA, replacementB };
                layer.MapElements = elements;
                VerifyElements(layer, replacementA, replacementB);
                Verify.AreEqual(1, originalElements.Count, "Replacing MapElements does not modify the original collection");

                // Clearing the replaced original still reaches the layer, whose Reset handling re-reads the layer's CURRENT
                // MapElements (MapElementsLayer::HandleMapElementsChanged reads MapElements(), not the changed vector).
                // It must leave the current elements untouched.
                originalElements.Clear();
                Verify.AreEqual(0, originalElements.Count, "The original collection is empty");
                VerifyElements(layer, replacementA, replacementB);
                Verify.AreEqual(2, elements.Count, "The assigned collection is not modified");
                Verify.AreEqual(1, owner.Layers.Count, "The layer is still the map's only layer");
                // (owner.Layers is an IList<MapLayer>, so its getter returns a different projected wrapper; compare by name.)
                Verify.IsTrue(owner.Layers[0] is MapElementsLayer, "The map's layer is a MapElementsLayer");
                Verify.AreEqual("owned", AutomationProperties.GetName(owner.Layers[0]), "The map still holds the same layer");
                Verify.AreEqual(2, ((MapElementsLayer)owner.Layers[0]).MapElements.Count, "Read through the map, the layer has the two current elements");
            });
        }

        [TestMethod]
        [TestProperty("Description", "The MapControl activation factories report their runtime class names and reject parameterless IActivationFactory::ActivateInstance with E_NOTIMPL (the classes are composable).")]
        public void MapActivationFactoriesReportRuntimeClassNames()
        {
            VerifyActivationFactory("Microsoft.UI.Xaml.Controls.MapControl");
            VerifyActivationFactory("Microsoft.UI.Xaml.Controls.MapElementsLayer");
            VerifyActivationFactory("Microsoft.UI.Xaml.Controls.MapIcon");
        }

        private static void VerifyActivationFactory(string runtimeClassName)
        {
            Guid IID_IActivationFactory = new Guid("00000035-0000-0000-C000-000000000046");
            using (global::WinRT.IObjectReference factory = global::WinRT.ActivationFactory.Get(runtimeClassName, IID_IActivationFactory))
            {
                Verify.AreEqual(runtimeClassName, new global::WinRT.IInspectable(factory).GetRuntimeClassName(), "Factory runtime class name");
                IntPtr instance;
                int hr = ActivateInstance(factory.ThisPtr, out instance);
                Verify.AreEqual(unchecked((int)0x80004001) /* E_NOTIMPL */, hr, runtimeClassName + ": parameterless activation is not supported");
                Verify.AreEqual(IntPtr.Zero, instance, runtimeClassName + ": no instance is returned");
            }
        }

        // Calls IActivationFactory::ActivateInstance directly. CsWinRT activates these composable classes through their
        // IXxxFactory::CreateInstance, never through ActivateInstance, so this is the only way to observe the answer.
        // Vtable layout: IUnknown (slots 0-2), IInspectable (3-5), IActivationFactory::ActivateInstance (6).
        private static unsafe int ActivateInstance(IntPtr activationFactory, out IntPtr instance)
        {
            IntPtr result = IntPtr.Zero;
            void** vtable = *(void***)activationFactory;
            int hr = ((delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, int>)vtable[6])(activationFactory, &result);
            instance = result;
            if (result != IntPtr.Zero)
            {
                global::System.Runtime.InteropServices.Marshal.Release(result);
            }
            return hr;
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
