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
using System.Globalization;
using System.Linq;
using System.Threading.Tasks;
using Windows.Data.Json;
using Windows.Foundation;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "MapControl")]
    public sealed partial class MapControlPage : TestPage
    {
        // Set while InjectWebMessagesAsync runs; overlapping clicks are ignored so handler subscriptions stay balanced.
        private bool _injectingWebMessages;

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

        private void SetBogusTokenButton_Click(object sender, RoutedEventArgs e)
        {
            // A syntactically valid token that Azure Maps rejects at runtime. This
            // exercises the asynchronous error path: Azure Maps surfaces the rejected
            // subscription key through the map's 'error' event in map.html, which is
            // forwarded (latched per-token and token-scrubbed) to the native
            // WebMessageReceived handler and raised as a public MapServiceErrorOccurred
            // event. Use this for the manual repro of the "blank map for a rejected
            // token" case described in AB#49260132.
            myMap.MapServiceToken = "bogus-but-syntactically-valid-token_0123456789";
        }

        private void Map_MapServiceErrorOccurred(MapControl sender, MapControlMapServiceErrorOccurredEventArgs args)
        {
            output.Text += args.DiagnosticMessage;
        }

        private void ClearOutputButton_Click(object sender, RoutedEventArgs e)
        {
            output.Text = "";
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
            // The location lets interaction tests verify the latitude/longitude round trip through map.html.
            output.Text += "Map Clicked at " + FormatLocation(args) + "\n";
        }

        private async void InjectWebMessagesButton_Click(object sender, RoutedEventArgs e)
        {
            if (_injectingWebMessages)
            {
                return;
            }
            _injectingWebMessages = true;
            webMessageResults.Text = "Running";
            try
            {
                webMessageResults.Text = await InjectWebMessagesAsync();
            }
            catch (Exception ex)
            {
                webMessageResults.Text = "Exception: " + ex.GetType().Name + " " + ex.Message;
            }
            finally
            {
                _injectingWebMessages = false;
            }
        }

        // Posts web messages into the map's WebView2 exactly as map.html does (pushpin clicks and script errors),
        // plus malformed messages, and reports what MapControl raised. Needs neither network access nor a map key.
        private async Task<string> InjectWebMessagesAsync()
        {
            WebView2 webView = VisualTreeHelper.GetChildrenCount(myMap) > 0 ? VisualTreeHelper.GetChild(myMap, 0) as WebView2 : null;
            if (webView == null)
            {
                return "No WebView2";
            }
            await webView.EnsureCoreWebView2Async();

            // Wait until map.html's script is loaded (it defines sendErrorInfo) and the map was initialized with the service token.
            bool pageReady = false;
            for (int i = 0; i < 60 && !pageReady; i++)
            {
                pageReady = (await webView.CoreWebView2.ExecuteScriptAsync("typeof sendErrorInfo === 'function' && currentMapServiceToken.length > 0")) == "true";
                if (!pageReady)
                {
                    await Task.Delay(500);
                }
            }
            if (!pageReady)
            {
                return "map.html not loaded";
            }

            var clicks = new List<MapElementClickEventArgs>();
            int layerClicks = 0;
            var errors = new List<string>();
            var testLayer = new MapElementsLayer();
            TypedEventHandler<MapControl, MapElementClickEventArgs> onClick = (s, a) => { clicks.Add(a); };
            TypedEventHandler<MapElementsLayer, MapElementClickEventArgs> onLayerClick = (s, a) => { layerClicks++; };
            TypedEventHandler<MapControl, MapControlMapServiceErrorOccurredEventArgs> onError = (s, a) => { errors.Add(a.DiagnosticMessage); };
            bool generalErrorHandlerDetached = false;
            try
            {
                myMap.MapElementClick += onClick;
                myMap.MapServiceErrorOccurred += onError;
                // The injected script error deliberately contains the live token (to exercise map.html's redaction); keep it
                // out of the page's general output box.
                myMap.MapServiceErrorOccurred -= Map_MapServiceErrorOccurred;
                generalErrorHandlerDetached = true;
                testLayer.MapElementClick += onLayerClick;
                int layer = myMap.Layers.Count;
                myMap.Layers.Add(testLayer);

                string l = layer.ToString(CultureInfo.InvariantCulture);
                string outOfRange = (layer + 1).ToString(CultureInfo.InvariantCulture);
                // map.html sends a pin at [longitude, latitude] with position[0] as "latitude" and position[1] as "longitude";
                // this is the message for a pin at latitude 47.6, longitude -122.3.
                await Post(webView, "{type:'pushpinClickEvent', coordinate:{latitude:-122.3, longitude:47.6}, layer:" + l + ", point:987654, text:'Pushpin clicked!'}");
                string[] malformed = new string[] {
                    "{type:'pushpinClickEvent', coordinate:{latitude:1, longitude:2}, layer:" + outOfRange + ", point:1}",
                    "{type:'pushpinClickEvent', coordinate:{latitude:1, longitude:2}, point:1}",
                    "{type:'pushpinClickEvent', layer:" + l + ", point:1}",
                    "{type:'pushpinClickEvent', coordinate:{latitude:'1', longitude:2}, layer:" + l + ", point:1}",
                    "{type:'pushpinClickEvent', coordinate:{longitude:2}, layer:" + l + ", point:1}",
                    "{type:'pushpinClickEvent', coordinate:{latitude:1, longitude:2}, layer:'" + l + "', point:1}",
                    "{type:'pushpinClickEvent', coordinate:{latitude:1, longitude:2}, layer:" + l + ", point:'Unknown'}",
                    "{type:'pushpinClickEvent', coordinate:7, layer:" + l + ", point:1}",
                    "{type:5, coordinate:{latitude:1, longitude:2}, layer:" + l + ", point:1}",
                    "{coordinate:{latitude:1, longitude:2}, layer:" + l + ", point:1}",
                    "'pushpinClickEvent'",
                    "[1, 2, 3]" };
                foreach (string message in malformed)
                {
                    await Post(webView, message);
                }
                // Sentinel click (pin at latitude 10, longitude 20), then a script error raised through map.html's own error path,
                // whose message contains the service token (which map.html must redact before it reaches native code).
                await Post(webView, "{type:'pushpinClickEvent', coordinate:{latitude:20, longitude:10}, layer:" + l + ", point:4242}");
                await webView.CoreWebView2.ExecuteScriptAsync("sendErrorInfo({ code: 'MapControlUiTest', message: 'marker-5821 ' + currentMapServiceToken });");

                for (int i = 0; i < 100 && !(clicks.Count >= 2 && errors.Any(x => x.Contains("marker-5821"))); i++)
                {
                    await Task.Delay(100);
                }
            }
            finally
            {
                myMap.MapElementClick -= onClick;
                myMap.MapServiceErrorOccurred -= onError;
                testLayer.MapElementClick -= onLayerClick;
                if (generalErrorHandlerDetached)
                {
                    // Restores exactly the one subscription made in the constructor.
                    myMap.MapServiceErrorOccurred += Map_MapServiceErrorOccurred;
                }
            }

            string marker = errors.FirstOrDefault(x => x.Contains("marker-5821"));
            string errorSummary = "none";
            if (marker != null)
            {
                JsonObject parsed = JsonObject.Parse(marker);
                errorSummary = parsed.GetNamedString("type") + "," + parsed.GetNamedString("code") + "," + parsed.GetNamedString("message");
            }
            string token = myMap.MapServiceToken;
            bool tokenInErrors = !string.IsNullOrEmpty(token) && errors.Any(x => x.Contains(token));
            if (!string.IsNullOrEmpty(token))
            {
                // This text is UIA-readable and logged by the interaction test, and CI injects a real map key into this
                // page: never display the token, even if map.html's redaction regresses (the test then still fails,
                // because it requires "[REDACTED]").
                errorSummary = errorSummary.Replace(token, "<UNREDACTED-TOKEN>");
            }
            return "clicks=" + clicks.Count +
                ";layerClicks=" + layerClicks +
                ";first=" + FormatLocation(clicks.Count > 0 ? clicks[0] : null) +
                ";firstElementNull=" + (clicks.Count > 0 && clicks[0].Element == null) +
                ";second=" + FormatLocation(clicks.Count > 1 ? clicks[1] : null) +
                ";error=" + errorSummary +
                ";tokenInErrors=" + tokenInErrors;
        }

        private static async Task Post(WebView2 webView, string messageScript)
        {
            await webView.CoreWebView2.ExecuteScriptAsync("window.chrome.webview.postMessage(" + messageScript + ");");
        }

        private static string FormatLocation(MapElementClickEventArgs args)
        {
            if (args == null || args.Location == null)
            {
                return "none";
            }
            BasicGeoposition position = args.Location.Position;
            return position.Latitude.ToString(CultureInfo.InvariantCulture) + "," + position.Longitude.ToString(CultureInfo.InvariantCulture);
        }
    }
}
