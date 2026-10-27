// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Globalization;
using System.Text.RegularExpressions;
using Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using System.Collections.Generic;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class MapControlTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
        [TestProperty("MUXControlsTestSuite", "SuiteB")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        // Scenario: on the MapControl test page, wait for the map, add a layer and two pins at (0,0) and (10,10) with
        //           the page's text boxes and Add Pin button.
        // Expected: the map's web content exposes exactly 1 UI Automation element before and 5 after the two pins are
        //           added (within 3 seconds).
        // A failure means: pins added through MapControl are not shown on the map (or not exposed to UI Automation).
        [TestMethod]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion._19H1)]
        public void AddMultipleElementsOnMap()
        {
            using (var setup = new TestSetupHelper("MapControl Tests")) 
            {
                //Wait for map to load
                var map = FindElement.ByName("Map Application");
                Wait.RetryUntilEvalFuncSuccessOrTimeout(
                    () => { return (map = FindElement.ByName("Map Application")) != null; },
                    retryTimoutByMilliseconds: 3000
                );

                // Get map contents
                var elements = map.Descendants[0].Descendants;

                //Verify that the interactive map is the only element
                Verify.AreEqual(1, elements.Count);

                // Add a layer
                var addLayerButton = FindElement.ByName<Button>("Add Layer");
                addLayerButton.Invoke();
                Wait.ForIdle();

                // Add a pins
                var addPinButton = FindElement.ByName<Button>("Add Pin");
                var latitudeTextBox = FindElement.ById<Edit>("latitudePinText");
                var longitudeTextBox = FindElement.ById<Edit>("longitudePinText");

                KeyboardHelper.EnterText(latitudeTextBox, "0");
                KeyboardHelper.EnterText(longitudeTextBox, "0");
                Wait.ForIdle();
                addPinButton.Invoke();
                Wait.ForIdle();

                KeyboardHelper.EnterText(latitudeTextBox, "10");
                KeyboardHelper.EnterText(longitudeTextBox, "10");
                Wait.ForIdle();
                addPinButton.Invoke();

                //Verify that the two pins were added
                Wait.RetryUntilEvalFuncSuccessOrTimeout(
                    () => { return (elements.Count) == 5; },
                    retryTimoutByMilliseconds: 3000
                );
                Verify.AreEqual(5, elements.Count); 
            }
        }

        // Scenario: on the MapControl test page, add a layer and a pin at latitude 10, longitude 20, then left-click
        //           the pin.
        // Expected: the map content has 3 UI Automation elements after the pin is added and 1 after the click (the page
        //           removes the clicked pin); the output contains "Layer Clicked" and "Map Clicked at" a location
        //           within 0.1 of (10, 20).
        // A failure means: clicking a pin does not raise MapElementClick on the map and the layer, or reports a wrong
        //                  or swapped location.
        [TestMethod]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion._19H1)]
        public void ValidadePinClickEvent()
        {
            using (var setup = new TestSetupHelper("MapControl Tests")) 
            {
                //Wait for map to load
                var map = FindElement.ByName("Map Application");
                Wait.RetryUntilEvalFuncSuccessOrTimeout(
                    () => { return (map = FindElement.ByName("Map Application")) != null; },
                    retryTimoutByMilliseconds: 3000
                );

                // Get map contents
                var elements = map.Descendants[0].Descendants;

                // Add a layer
                var addLayerButton = FindElement.ByName<Button>("Add Layer");
                addLayerButton.Invoke();
                Wait.ForIdle();

                // Add a pin at an asymmetric location so a latitude/longitude swap anywhere on the way to map.html and back is visible.
                var addPinButton = FindElement.ByName<Button>("Add Pin");
                var latitudeTextBox = FindElement.ById<Edit>("latitudePinText");
                var longitudeTextBox = FindElement.ById<Edit>("longitudePinText");

                KeyboardHelper.EnterText(latitudeTextBox, "10");
                KeyboardHelper.EnterText(longitudeTextBox, "20");
                Wait.ForIdle();
                addPinButton.Invoke();

                //Check pin was created
                Wait.RetryUntilEvalFuncSuccessOrTimeout(
                    () => { return (elements.Count) == 3; },
                    retryTimoutByMilliseconds: 3000
                );
                Verify.AreEqual(3, elements.Count); 

                // Click on the pin
                var pin = elements[1];
                InputHelper.LeftClick(pin);
                Wait.ForIdle();

                //Check pin was deleted
                Wait.RetryUntilEvalFuncSuccessOrTimeout(
                    () => { return (elements.Count) == 1; },
                    retryTimoutByMilliseconds: 3000
                );
                Verify.AreEqual(1, elements.Count); 

                //Check if both click events were raised
                var logBox = new Edit(FindElement.ById("output"));
                string log = logBox.GetText();
                Verify.IsTrue(log.Contains("Layer Clicked"));
                // The box also receives map service diagnostics, so only the click report is parsed and logged.
                var click = Regex.Match(log, @"Map Clicked at (-?[0-9.]+),(-?[0-9.]+)");
                Verify.IsTrue(click.Success, "The map click reports a location");
                Log.Comment(click.Value);
                double latitude = double.Parse(click.Groups[1].Value, CultureInfo.InvariantCulture);
                double longitude = double.Parse(click.Groups[2].Value, CultureInfo.InvariantCulture);
                Verify.IsLessThan(Math.Abs(latitude - 10.0), 0.1, "Latitude of the clicked pin (10)");
                Verify.IsLessThan(Math.Abs(longitude - 20.0), 0.1, "Longitude of the clicked pin (20)");

            }
        }

        // Scenario: on the MapControl test page, run the Inject web messages scenario: post map.html-shaped messages
        //           into the WebView2 (a pin click, a script error, twelve malformed messages, a click for a null layer
        //           entry and a sentinel click). No network access or map key is needed.
        // Expected: the result string is exactly: 2 map clicks and 2 layer clicks, the first at 47.6,-122.3 with a null
        //           element and the second at 10,20; one MapServiceErrorOccurred
        //           "javascriptError,MapControlUiTest,marker-5821 [REDACTED]" with the token never present; event args
        //           of runtime classes MapControlMapServiceErrorOccurredEventArgs and MapElementClickEventArgs.
        // A failure means: messages from the web map are not turned into the right MapControl events, malformed
        //                  messages raise events, or the service token leaks into error reports.
        [TestMethod]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion._19H1)]
        [TestProperty("Description", "Web messages posted into the map's WebView2 the way map.html posts them raise MapElementClick (map and layer) with the pin location and a map.html script error raises MapServiceErrorOccurred with the service token redacted; malformed messages and a click for a null layer entry raise nothing; the raised event args report their own runtime class names. Needs no network access or map key.")]
        public void InjectedWebMessagesRaiseMapEventsAndMalformedMessagesAreIgnored()
        {
            using (var setup = new TestSetupHelper("MapControl Tests"))
            {
                Edit results = new Edit(FindElement.ById("WebMessageResults"));
                Verify.AreEqual("Not run", results.Value);

                FindElement.ById<Button>("InjectWebMessagesButton").Invoke();
                Wait.RetryUntilEvalFuncSuccessOrTimeout(
                    () => { string value = results.Value; return value != "Not run" && value != "Running"; },
                    retryTimoutByMilliseconds: 60000
                );
                Log.Comment("Web message results: " + results.Value);

                // Two clicks: the map.html-shaped click for a pin at (47.6, -122.3) and the sentinel at (10, 20) posted
                // after twelve malformed messages, which must raise nothing.
                Verify.AreEqual(
                    "clicks=2;layerClicks=2;first=47.6,-122.3;firstElementNull=True;second=10,20;error=javascriptError,MapControlUiTest,marker-5821 [REDACTED];tokenInErrors=False" +
                    ";argsClasses=Microsoft.UI.Xaml.Controls.MapControlMapServiceErrorOccurredEventArgs,Microsoft.UI.Xaml.Controls.MapElementClickEventArgs",
                    results.Value);
            }
        }

        // Scenario: after map.html loaded, record its JavaScript calls; add an empty layer and an icon a1, add a layer
        //           that already holds b1 and b2, set ZoomLevel 7, Center (latitude 10, longitude 20) and a test
        //           MapServiceToken; then reload the page with an offline stand-in for the Azure Maps SDK.
        // Expected: exactly 16 recorded calls: addPoint(longitude, latitude, name) for a1, b1 and b2 twice each,
        //           addSymbolLayer() 4 times, interactiveControlsVisible(false), updateCenter{10,20} twice
        //           (order-insensitive), the token update once and updateZoom(7) twice; the token is sent before the
        //           layers are re-sent; the reload re-creates the map at [20,10] with 2 marker layers and the points
        //           a1, b1, b2.
        // A failure means: changes made after the map loaded are not sent to the web map, a new token does not re-send
        //                  the map state, or a page reload loses layers or pins.
        [TestMethod]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion._19H1)]
        [TestProperty("Description", "After the map page loaded, adding layers (empty or already holding icons) and icons, and changing ZoomLevel, Center and MapServiceToken send the matching calls to map.html; a new token re-sends every layer, icon, the controls, center and zoom; reloading the page re-adds every layer and icon. Recorded in the page itself, so no network access or map key is needed.")]
        public void PropertyAndLayerChangesAfterLoadAreSentToTheMapPage()
        {
            using (var setup = new TestSetupHelper("MapControl Tests"))
            {
                string result = RunMapScenario("RunProtocolChecksButton");
                Verify.AreEqual(
                    "calls=16;" +
                    "addPoint(2,1,\"a1\")|addPoint(2,1,\"a1\")|addPoint(4,3,\"b1\")|addPoint(4,3,\"b1\")|addPoint(6,5,\"b2\")|addPoint(6,5,\"b2\")|" +
                    "addSymbolLayer()|addSymbolLayer()|addSymbolLayer()|addSymbolLayer()|" +
                    "interactiveControlsVisible(false)|" +
                    "updateCenter{10,20}|updateCenter{10,20}|" +
                    "updateMapServiceToken(\"bogus-but-syntactically-valid-token_0123456789\")|" +
                    "updateZoom(7)|updateZoom(7);" +
                    "tokenBeforeResend=True;" +
                    "reinit=Map[20,10];markerLayers=2;points=point[2,1,\"a1\"]|point[4,3,\"b1\"]|point[6,5,\"b2\"]",
                    result);
            }
        }

        // Scenario: after map.html loaded, replace its map object with a recorder of setCamera calls, then set Center
        //           to latitude 10, longitude 20.
        // Expected: (correct behaviour) the map camera is centered on [20,10] ([longitude, latitude]).
        // A failure means: the bug is still present: the camera is centered on swapped coordinates ([10,20]).
        // Ignored: reproduces BUG-MAP-2 (MapControl calls updateCenter(longitude, latitude) but map.html takes
        //          (latitude, longitude)); remove the Ignore once fixed.
        [TestMethod]
        [TestProperty("Ignore", "True")] // BUG-MAP-2: MapControl calls updateCenter(longitude, latitude) but map.html declares updateCenter(latitude, longitude), so the camera is centered on swapped coordinates.
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion._19H1)]
        [TestProperty("Description", "Changing Center after the map page loaded centers the map camera on [longitude, latitude].")]
        public void CenterChangeAfterLoadCentersTheMapOnTheNewLocation()
        {
            using (var setup = new TestSetupHelper("MapControl Tests"))
            {
                // Center = latitude 10, longitude 20; Azure Maps takes [longitude, latitude].
                Verify.AreEqual("camera=[20,10]", RunMapScenario("RunCenterCheckButton"));
            }
        }

        // Scenario: after map.html loaded, add three layers (recorded as L1, L2, L3) and remove the middle one.
        // Expected: (correct behaviour) the page receives clearLayer("L2").
        // A failure means: the bug is still present: the next layer (L3) is cleared instead.
        // Ignored: reproduces BUG-MAP-5 (OnLayersVectorChanged reads the layer at the removed index after the removal);
        //          remove the Ignore once fixed.
        [TestMethod]
        [TestProperty("Ignore", "True")] // BUG-MAP-5: OnLayersVectorChanged reads Layers().GetAt(index) after the removal, so removing a middle layer clears the NEXT layer in the page.
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion._19H1)]
        [TestProperty("Description", "Removing the middle of three layers after the map page loaded clears exactly that layer in the page.")]
        public void RemovingMiddleLayerClearsThatLayerInTheMapPage()
        {
            using (var setup = new TestSetupHelper("MapControl Tests"))
            {
                // The page's recorder hands out layer ids L1, L2, L3 in order.
                Verify.AreEqual("cleared=clearLayer(\"L2\")", RunMapScenario("RunRemoveMiddleLayerCheckButton"));
            }
        }

        // Scenario: after map.html loaded, add a pin at latitude 1, longitude 2 and then set its Location to latitude
        //           7, longitude 8.
        // Expected: (correct behaviour) the page receives updatePoint(8,7).
        // A failure means: the bug is still present: moving a pin never reaches the map.
        // Ignored: reproduces BUG-MAP-7 (MapIcon looks for its MapControl through the visual tree, where a MapIcon
        //          never is); remove the Ignore once fixed.
        [TestMethod]
        [TestProperty("Ignore", "True")] // BUG-MAP-7: MapIcon.OnPropertyChanged looks for its MapControl with VisualTreeHelper, but a MapIcon is never in the visual tree, so moving a pin never reaches the page.
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion._19H1)]
        [TestProperty("Description", "Changing MapIcon.Location of a pin that is on the map moves the pin in the map page.")]
        public void MovingAPinAfterItWasAddedMovesItInTheMapPage()
        {
            using (var setup = new TestSetupHelper("MapControl Tests"))
            {
                // The pin moves from latitude 1, longitude 2 to latitude 7, longitude 8; map.html takes (longitude, latitude).
                Verify.AreEqual("moved=updatePoint(8,7)", RunMapScenario("RunMovePinCheckButton"));
            }
        }

        private static string RunMapScenario(string buttonId)
        {
            Edit results = new Edit(FindElement.ById("ProtocolResults"));
            Verify.AreEqual("Not run", results.Value);
            FindElement.ById<Button>(buttonId).Invoke();
            Wait.RetryUntilEvalFuncSuccessOrTimeout(
                () => { string value = results.Value; return value != "Not run" && value != "Running"; },
                retryTimoutByMilliseconds: 90000
            );
            Log.Comment(buttonId + " results: " + results.Value);
            return results.Value;
        }
    }
}
