// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
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

                // Add a pin
                var addPinButton = FindElement.ByName<Button>("Add Pin");
                var latitudeTextBox = FindElement.ById<Edit>("latitudePinText");
                var longitudeTextBox = FindElement.ById<Edit>("longitudePinText");

                KeyboardHelper.EnterText(latitudeTextBox, "0");
                KeyboardHelper.EnterText(longitudeTextBox, "0");
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
                Verify.IsTrue(logBox.GetText().Contains("Layer Clicked"));
                Verify.IsTrue(logBox.GetText().Contains("Map Clicked"));

            }
        }

        [TestMethod]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion._19H1)]
        [TestProperty("Description", "Web messages posted into the map's WebView2 the way map.html posts them raise MapElementClick (map and layer) with the pin location and a map.html script error raises MapServiceErrorOccurred with the service token redacted; malformed messages raise nothing. Needs no network access or map key.")]
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
                    "clicks=2;layerClicks=2;first=47.6,-122.3;firstElementNull=True;second=10,20;error=javascriptError,MapControlUiTest,marker-5821 [REDACTED];tokenInErrors=False",
                    results.Value);
            }
        }
    }
}
