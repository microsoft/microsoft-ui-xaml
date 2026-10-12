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
using Microsoft.Web.WebView2.Core;

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
            myMap.MapServiceToken = BogusMapServiceToken;
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
            var argsClasses = new SortedSet<string>(StringComparer.Ordinal);
            var testLayer = new MapElementsLayer();
            TypedEventHandler<MapControl, MapElementClickEventArgs> onClick = (s, a) => { clicks.Add(a); argsClasses.Add(NativeRuntimeClassName(a)); };
            TypedEventHandler<MapElementsLayer, MapElementClickEventArgs> onLayerClick = (s, a) => { layerClicks++; };
            TypedEventHandler<MapControl, MapControlMapServiceErrorOccurredEventArgs> onError = (s, a) => { errors.Add(a.DiagnosticMessage); argsClasses.Add(NativeRuntimeClassName(a)); };
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
                // MapLayer has no public constructor, so null is the only value in Layers that is not a MapElementsLayer;
                // a click for it must be ignored. (A null layer must never reach a token change or a page reload, see
                // BUG-MAP-10, so it is only added in this injection run.)
                int nullLayer = myMap.Layers.Count;
                myMap.Layers.Add(null);

                string l = layer.ToString(CultureInfo.InvariantCulture);
                string outOfRange = (nullLayer + 1).ToString(CultureInfo.InvariantCulture);
                // map.html sends a pin at [longitude, latitude] with position[0] as "latitude" and position[1] as "longitude";
                // this is the message for a pin at latitude 47.6, longitude -122.3.
                await Post(webView, "{type:'pushpinClickEvent', coordinate:{latitude:-122.3, longitude:47.6}, layer:" + l + ", point:987654, text:'Pushpin clicked!'}");
                string[] malformed = new string[] {
                    "{type:'pushpinClickEvent', coordinate:{latitude:1, longitude:2}, layer:" + outOfRange + ", point:1}",
                    "{type:'pushpinClickEvent', coordinate:{latitude:1, longitude:2}, layer:" + nullLayer.ToString(CultureInfo.InvariantCulture) + ", point:1}",
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
                ";tokenInErrors=" + tokenInErrors +
                ";argsClasses=" + string.Join(",", argsClasses);
        }

        // IInspectable::GetRuntimeClassName of the native object behind a projected object (CsWinRT never asks a
        // sealed class's instance for it).
        private static string NativeRuntimeClassName(object projected)
        {
            return new global::WinRT.IInspectable(((global::WinRT.IWinRTObject)projected).NativeObject).GetRuntimeClassName();
        }

        // ---------------------------------------------------------------------------------------------------------
        // Map page protocol checks. MapControl talks to map.html only by calling its global functions through
        // CoreWebView2.ExecuteScriptAsync. These checks replace those functions with recorders (or give map.html a fake
        // 'atlas'), so everything MapControl sends can be read back without network access or a map key.
        // Rules for every scenario: Center is never set to null (BUG-MAP-3), icons of a layer that already had icons when
        // it was added are never removed or replaced (BUG-MAP-6), and no null layer is added (BUG-MAP-10).
        // ---------------------------------------------------------------------------------------------------------
        private const string BogusMapServiceToken = "bogus-but-syntactically-valid-token_0123456789";
        private bool _runningMapScenario;

        private const string RecordersScript = @"
(function () {
    window.__mapCalls = [];
    window.__layerIds = [];
    var fmt = function (args) { return Array.prototype.map.call(args, function (a) { return JSON.stringify(a); }).join(','); };
    var rec = function (call) { window.__mapCalls.push(call); };
    updateMapServiceToken = function (token) { rec('updateMapServiceToken(' + JSON.stringify(token) + ')'); };
    // Recorded order-insensitively: which value is the latitude is checked end to end by the Center scenario.
    updateCenter = function (a, b) { rec('updateCenter{' + [a, b].sort(function (x, y) { return x - y; }).join(',') + '}'); };
    updateZoom = function () { rec('updateZoom(' + fmt(arguments) + ')'); };
    interactiveControlsVisible = function () { rec('interactiveControlsVisible(' + fmt(arguments) + ')'); };
    addSymbolLayer = function () { var id = 'L' + (window.__layerIds.length + 1); window.__layerIds.push(id); rec('addSymbolLayer()'); return id; };
    addPoint = function (longitude, latitude, layerId, name) {
        rec('addPoint(' + fmt([longitude, latitude, name]) + ')' + (window.__layerIds.indexOf(layerId) >= 0 ? '' : '!unknownLayer:' + JSON.stringify(layerId)));
        return 100 + window.__mapCalls.length;
    };
    clearLayer = function (layerId) { rec('clearLayer(' + JSON.stringify(layerId) + ')'); };
    removePoint = function () { rec('removePoint(' + fmt(arguments) + ')'); };
    updatePoint = function (longitude, latitude) { rec('updatePoint(' + fmt([longitude, latitude]) + ')'); };
})();";

        // A minimal stand-in for the Azure Maps SDK that logs what map.html does with it. Without atlas.layer.BubbleLayer the
        // bundled HtmlMarkerLayer module in map.html stops early (as it does offline), so these classes are the ones used.
        private const string FakeAtlasScript = @"
(function () {
    var log = window.__atlasLog = [];
    var layerCount = 0;
    function Events() {}
    Events.prototype.add = function (name, a, b) { if (name === 'ready') { (typeof a === 'function' ? a : b)(); } };
    function Map(container, options) {
        log.push('Map[' + options.center.join(',') + ']');
        this.events = new Events();
        this.sources = { add: function () {} };
        this.layers = { add: function () {}, getLayers: function () { return []; } };
        this.controls = { add: function () {}, remove: function () {} };
        this.camera = { center: options.center };
    }
    Map.prototype.setCamera = function (o) { if (o.center) { log.push('setCamera[' + o.center.join(',') + ']'); this.camera.center = o.center; } };
    Map.prototype.getCamera = function () { return this.camera; };
    function DataSource() {}
    DataSource.prototype.add = function (features) {
        features.forEach(function (f) { log.push('point[' + f.geometry.coordinates.join(',') + ',' + JSON.stringify(f.properties.name) + ']'); });
    };
    DataSource.prototype.remove = function () {};
    DataSource.prototype.clear = function () {};
    function HtmlMarkerLayer() { layerCount++; this.id = 'fake-layer-' + layerCount; log.push('HtmlMarkerLayer'); }
    HtmlMarkerLayer.prototype.getId = function () { return this.id; };
    function Control() {}
    window.atlas = {
        Map: Map,
        HtmlMarker: function () {},
        source: { DataSource: DataSource },
        layer: { HtmlMarkerLayer: HtmlMarkerLayer },
        data: { Point: function (coordinates) { this.coordinates = coordinates; }, Feature: function (geometry, properties, id) { this.geometry = geometry; this.properties = properties; this.id = id; } },
        control: { ZoomControl: Control, CompassControl: Control, PitchControl: Control, StyleControl: Control }
    };
})();";

        private async void RunProtocolChecksButton_Click(object sender, RoutedEventArgs e)
        {
            await RunMapScenarioAsync(RunProtocolChecksAsync);
        }

        private async void RunCenterCheckButton_Click(object sender, RoutedEventArgs e)
        {
            await RunMapScenarioAsync(RunCenterCheckAsync);
        }

        private async void RunRemoveMiddleLayerCheckButton_Click(object sender, RoutedEventArgs e)
        {
            await RunMapScenarioAsync(RunRemoveMiddleLayerCheckAsync);
        }

        private async void RunMovePinCheckButton_Click(object sender, RoutedEventArgs e)
        {
            await RunMapScenarioAsync(RunMovePinCheckAsync);
        }

        private async Task RunMapScenarioAsync(Func<CoreWebView2, Task<string>> scenario)
        {
            if (_runningMapScenario)
            {
                return;
            }
            _runningMapScenario = true;
            protocolResults.Text = "Running";
            try
            {
                WebView2 webView = VisualTreeHelper.GetChildrenCount(myMap) > 0 ? VisualTreeHelper.GetChild(myMap, 0) as WebView2 : null;
                if (webView == null)
                {
                    protocolResults.Text = "No WebView2";
                    return;
                }
                await webView.EnsureCoreWebView2Async();
                CoreWebView2 core = webView.CoreWebView2;
                bool pageReady = false;
                for (int i = 0; i < 60 && !pageReady; i++)
                {
                    pageReady = (await core.ExecuteScriptAsync("typeof sendErrorInfo === 'function' && currentMapServiceToken.length > 0")) == "true";
                    if (!pageReady)
                    {
                        await Task.Delay(500);
                    }
                }
                if (!pageReady)
                {
                    protocolResults.Text = "map.html not loaded";
                    return;
                }
                // Let the calls MapControl makes right after initializing the page finish before anything is recorded.
                await Task.Delay(1000);
                string result = await scenario(core);
                string realToken = MapControlApiKey.AzureMapsToken;
                if (!string.IsNullOrEmpty(realToken))
                {
                    // CI injects a real key into this page; never display it.
                    result = result.Replace(realToken, "<TOKEN>");
                }
                protocolResults.Text = result;
            }
            catch (Exception ex)
            {
                protocolResults.Text = "Exception: " + ex.GetType().Name + " " + ex.Message;
            }
            finally
            {
                _runningMapScenario = false;
            }
        }

        private async Task<string> RunProtocolChecksAsync(CoreWebView2 core)
        {
            await core.ExecuteScriptAsync(RecordersScript);

            var layerA = new MapElementsLayer();
            myMap.Layers.Add(layerA);
            await WaitForCallsAsync(core, 1);
            layerA.MapElements.Add(CreateNamedIcon(1, 2, "a1"));
            await WaitForCallsAsync(core, 2);

            // Layer B already holds two icons when it is added (its icons are never removed or replaced: BUG-MAP-6).
            var layerB = new MapElementsLayer();
            layerB.MapElements.Add(CreateNamedIcon(3, 4, "b1"));
            layerB.MapElements.Add(CreateNamedIcon(5, 6, "b2"));
            myMap.Layers.Add(layerB);
            await WaitForCallsAsync(core, 5);

            myMap.ZoomLevel = 7;
            await WaitForCallsAsync(core, 6);
            myMap.Center = new Geopoint(new BasicGeoposition() { Latitude = 10, Longitude = 20 });
            await WaitForCallsAsync(core, 7);

            // A new token re-sends the layers, their icons, the controls, the center and the zoom level.
            myMap.MapServiceToken = BogusMapServiceToken;
            await WaitForCallsAsync(core, 16);
            await Task.Delay(1000);

            List<string> calls = await ReadStringListAsync(core, "__mapCalls");
            int tokenIndex = calls.FindIndex(c => c.StartsWith("updateMapServiceToken(", StringComparison.Ordinal));
            int thirdLayerIndex = calls.Select((c, i) => new { c, i }).Where(x => x.c == "addSymbolLayer()").Skip(2).Select(x => x.i).DefaultIfEmpty(-1).First();
            string protocol = "calls=" + calls.Count + ";" + string.Join("|", calls.OrderBy(c => c, StringComparer.Ordinal)) +
                ";tokenBeforeResend=" + (tokenIndex >= 0 && thirdLayerIndex > tokenIndex);

            // Reloading the page re-initializes it: InitializeWebMap re-adds every layer and its icons. map.html gets a fake
            // 'atlas' (the Azure Maps CDN is answered with 404), so this works the same with or without network access.
            core.AddWebResourceRequestedFilter("https://atlas.microsoft.com/*", CoreWebView2WebResourceContext.All);
            core.WebResourceRequested += (CoreWebView2 s, CoreWebView2WebResourceRequestedEventArgs args) =>
            {
                args.Response = s.Environment.CreateWebResourceResponse(null, 404, "Not Found", "");
            };
            await core.AddScriptToExecuteOnDocumentCreatedAsync(FakeAtlasScript);
            core.Reload();

            List<string> atlasLog = new List<string>();
            for (int i = 0; i < 75; i++)
            {
                await Task.Delay(200);
                try
                {
                    atlasLog = await ReadStringListAsync(core, "window.__atlasLog || []");
                }
                catch (Exception)
                {
                    // The document may be navigating; try again.
                    continue;
                }
                if (atlasLog.Count(x => x.StartsWith("point[", StringComparison.Ordinal)) >= 3)
                {
                    break;
                }
            }
            await Task.Delay(500);
            atlasLog = await ReadStringListAsync(core, "window.__atlasLog || []");
            return protocol +
                ";reinit=" + string.Join("|", atlasLog.Where(x => x.StartsWith("Map[", StringComparison.Ordinal))) +
                ";markerLayers=" + atlasLog.Count(x => x == "HtmlMarkerLayer") +
                ";points=" + string.Join("|", atlasLog.Where(x => x.StartsWith("point[", StringComparison.Ordinal)).OrderBy(x => x, StringComparer.Ordinal));
        }

        // Correct behaviour: the map camera is centered on [longitude, latitude] (Azure Maps order).
        private async Task<string> RunCenterCheckAsync(CoreWebView2 core)
        {
            await core.ExecuteScriptAsync(@"window.__camera = []; map = { setCamera: function (o) { if (o.center) { window.__camera.push('[' + o.center.join(',') + ']'); } } };");
            myMap.Center = new Geopoint(new BasicGeoposition() { Latitude = 10, Longitude = 20 });
            List<string> camera = new List<string>();
            for (int i = 0; i < 25 && camera.Count == 0; i++)
            {
                await Task.Delay(200);
                camera = await ReadStringListAsync(core, "window.__camera");
            }
            return "camera=" + string.Join("|", camera);
        }

        // Correct behaviour: removing the middle of three layers clears that layer in the page.
        private async Task<string> RunRemoveMiddleLayerCheckAsync(CoreWebView2 core)
        {
            await core.ExecuteScriptAsync(RecordersScript);
            for (int i = 1; i <= 3; i++)
            {
                myMap.Layers.Add(new MapElementsLayer());
                await WaitForCallsAsync(core, i);
            }
            await Task.Delay(300);
            myMap.Layers.RemoveAt(1);
            await WaitForCallsAsync(core, 4, timeoutMs: 5000);
            await Task.Delay(500);
            List<string> calls = await ReadStringListAsync(core, "__mapCalls");
            return "cleared=" + string.Join("|", calls.Where(c => c.StartsWith("clearLayer(", StringComparison.Ordinal)));
        }

        // Correct behaviour: moving a pin that is on the map moves it in the page.
        private async Task<string> RunMovePinCheckAsync(CoreWebView2 core)
        {
            await core.ExecuteScriptAsync(RecordersScript);
            var layer = new MapElementsLayer();
            myMap.Layers.Add(layer);
            await WaitForCallsAsync(core, 1);
            MapIcon icon = CreateNamedIcon(1, 2, "pin");
            layer.MapElements.Add(icon);
            await WaitForCallsAsync(core, 2);
            await Task.Delay(300);
            icon.Location = new Geopoint(new BasicGeoposition() { Latitude = 7, Longitude = 8 });
            await WaitForCallsAsync(core, 3, timeoutMs: 5000);
            List<string> calls = await ReadStringListAsync(core, "__mapCalls");
            return "moved=" + string.Join("|", calls.Where(c => c.StartsWith("updatePoint(", StringComparison.Ordinal)));
        }

        private static MapIcon CreateNamedIcon(double latitude, double longitude, string name)
        {
            var icon = new MapIcon() { Location = new Geopoint(new BasicGeoposition() { Latitude = latitude, Longitude = longitude }) };
            Microsoft.UI.Xaml.Automation.AutomationProperties.SetName(icon, name);
            return icon;
        }

        // Waits until the recorders have seen at least 'count' calls, then a little longer so MapControl can store the ids
        // returned by those calls before the next step.
        private static async Task WaitForCallsAsync(CoreWebView2 core, int count, int timeoutMs = 10000)
        {
            for (int waited = 0; waited < timeoutMs; waited += 100)
            {
                string length = await core.ExecuteScriptAsync("window.__mapCalls ? window.__mapCalls.length : -1");
                if (int.TryParse(length, NumberStyles.Integer, CultureInfo.InvariantCulture, out int value) && value >= count)
                {
                    break;
                }
                await Task.Delay(100);
            }
            await Task.Delay(200);
        }

        private static async Task<List<string>> ReadStringListAsync(CoreWebView2 core, string expression)
        {
            string json = await core.ExecuteScriptAsync("JSON.stringify(" + expression + ")");
            var list = new List<string>();
            if (string.IsNullOrEmpty(json) || json == "null")
            {
                return list;
            }
            foreach (IJsonValue value in JsonArray.Parse(JsonValue.Parse(json).GetString()))
            {
                list.Add(value.GetString());
            }
            return list;
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
