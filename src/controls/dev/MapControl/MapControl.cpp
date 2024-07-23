// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "MapControl.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "WebView2.h"
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Windows.Devices.Geolocation.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/windows.data.json.h>
#include "MapElementClickEventArgs.h"
#include "MapControlMapServiceErrorOccurredEventArgs.h"
#include "MapElementsLayer.h"
#include "Vector.h"
#include "MuxcTraceLogging.h"

winrt::hstring MapControl::s_mapHtmlContent{};

MapControl::MapControl()
{
    SetDefaultStyleKey(this);

    // Create layers vector that can hold MapElements to display information and be interacted with on the map
    auto layers = winrt::make<Vector<winrt::MapLayer>>().as<winrt::IObservableVector<winrt::MapLayer>>();
    layers.VectorChanged({ this, &MapControl::OnLayersVectorChanged });
    SetValue(s_LayersProperty, layers);
}

void MapControl::OnApplyTemplate()
{
    m_webviewWebMessageReceivedRevoker.revoke();
    m_webviewNavigationCompletedRevoker.revoke();

    winrt::IControlProtected thisAsControlProtected = *this;
    if (auto webview = GetTemplateChildT<winrt::WebView2>(L"PART_WebView2", thisAsControlProtected))
    {
        m_webView.set(webview);

        m_webviewWebMessageReceivedRevoker = webview.WebMessageReceived(winrt::auto_revoke, { this, &MapControl::WebMessageReceived });
        m_webviewNavigationCompletedRevoker = webview.NavigationCompleted(winrt::auto_revoke, {
        [this](auto const&, winrt::CoreWebView2NavigationCompletedEventArgs const& args)
        {
            XamlTelemetry::MapControl_WebViewNavigationCompleted(reinterpret_cast<uint64_t>(this));
            InitializeWebMap();
        }});

        SetUpWebView();
    }
}

winrt::IAsyncOperation<winrt::CoreWebView2> MapControl::GetCoreWebView2()
{
    if (auto webView = m_webView.get())
    {
        co_await webView.EnsureCoreWebView2Async();
        co_return webView.CoreWebView2();
    }
    co_return nullptr;
}

winrt::fire_and_forget MapControl::SetUpWebView()
{
    if (auto webView = m_webView.get())
    {
        auto core = co_await GetCoreWebView2();

        // azure maps might not be displayed correctly with tracking prevention enabled
        core.Profile().PreferredTrackingPreventionLevel(winrt::CoreWebView2TrackingPreventionLevel::None);
        webView.NavigateToString(co_await GetMapHtmlContent());
    }
}

winrt::IAsyncOperation<winrt::hstring> MapControl::InitializeWebMap()
{
    XamlTelemetry::MapControl_InitializeWebMap(true, reinterpret_cast<uint64_t>(this));

    winrt::hstring returnedValue{};
    auto center = Center();
    winrt::hstring pos = L"0,0";
    if (center != nullptr)
    {
        const auto centerPosition = center.Position();
        pos = std::format(L"{},{}", centerPosition.Longitude, centerPosition.Latitude);
    }

    auto token = winrt::Windows::Data::Json::JsonValue::CreateStringValue(MapServiceToken());
    // Initializing Azure Maps on WebView
    // params: longitude, latitude, mapServiceToken
    auto script = std::format(L"initializeMap({},{});", pos, token.Stringify().c_str());

    if (auto webView = m_webView.get())
    {
        auto core = webView.CoreWebView2();
        returnedValue = co_await core.ExecuteScriptAsync(script);

        for (uint32_t i = 0; i < Layers().Size(); i++)
        {
            winrt::MapElementsLayer layer = Layers().GetAt(i).as<winrt::MapElementsLayer>();
            OnLayerAdded(layer);
        }
    }
    UpdateControlsInWebPage();

    XamlTelemetry::MapControl_InitializeWebMap(false, reinterpret_cast<uint64_t>(this));
    co_return returnedValue;
}

void MapControl::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();

    if (property == s_MapServiceTokenProperty)
    {
        UpdateMapServiceTokenInWebPage();
    }
    else if (property == s_CenterProperty)
    {
        UpdateCenterInWebPage();
    }
    else if (property == s_ZoomLevelProperty)
    {
        UpdateZoomLevelInWebPage();
    }
    else if (property == s_InteractiveControlsVisibleProperty)
    {
        UpdateControlsInWebPage();
    }
}

// Updates Azure Maps Authentication Key on WebView
winrt::fire_and_forget MapControl::UpdateMapServiceTokenInWebPage()
{
    if (auto webView = m_webView.get())
    {
        auto core = co_await GetCoreWebView2();
        // Sends new token to WebView
        // params: mapServiceToken
        auto token = winrt::Windows::Data::Json::JsonValue::CreateStringValue(MapServiceToken());
        auto script = std::format(L"updateMapServiceToken({});", token.Stringify().c_str());
        co_await core.ExecuteScriptAsync(script);

        for (uint32_t i = 0; i < Layers().Size(); i++)
        {
            winrt::MapElementsLayer layer = Layers().GetAt(i).try_as<winrt::MapElementsLayer>();
            OnLayerAdded(layer);
        }

        UpdateControlsInWebPage();
        UpdateCenterInWebPage();
        UpdateZoomLevelInWebPage();
    }
}

winrt::fire_and_forget MapControl::UpdateCenterInWebPage()
{
    if (auto webView = m_webView.get())
    {
        auto core = co_await GetCoreWebView2();

        auto newCenter = Center().Position();
        auto pos = std::format(L"{},{}", newCenter.Longitude, newCenter.Latitude);
        // Updates center of map in WebView
        // params: longitude, latitude
        co_await core.ExecuteScriptAsync(L"updateCenter(" + pos + L")");
    }
}

winrt::fire_and_forget MapControl::UpdateZoomLevelInWebPage()
{
    if (auto webView = m_webView.get())
    {
        auto core = co_await GetCoreWebView2();
        core.ExecuteScriptAsync(L"updateZoom(" + winrt::to_hstring(ZoomLevel()) + L")");
    }
}

winrt::fire_and_forget MapControl::UpdateControlsInWebPage()
{
    if (auto webView = m_webView.get())
    {
        auto core = co_await GetCoreWebView2();
        core.ExecuteScriptAsync(L"interactiveControlsVisible(" + winrt::to_hstring(InteractiveControlsVisible()) + L")");
    }
}

void MapControl::WebMessageReceived(winrt::WebView2 sender, winrt::CoreWebView2WebMessageReceivedEventArgs args)
{
    auto jsonAsString = args.WebMessageAsJson();
    if (!jsonAsString.empty())
    {
        winrt::Windows::Data::Json::JsonObject obj = winrt::Windows::Data::Json::JsonObject();
        if (winrt::Windows::Data::Json::JsonObject::TryParse(jsonAsString, obj))
        {
            auto type = obj.TryLookup(L"type");
            // Handles click events on MapElements
            if (type.GetString() == L"pushpinClickEvent")
            {
                auto clickedLayer = Layers().GetAt(static_cast<uint32_t>(obj.TryLookup(L"layer").GetNumber())).try_as<MapElementsLayer>();
                auto location = winrt::Geopoint{ winrt::BasicGeoposition{
                    obj.TryLookup(L"coordinate").GetObject().TryLookup(L"longitude").GetNumber(),
                    obj.TryLookup(L"coordinate").GetObject().TryLookup(L"latitude").GetNumber() }};

                auto pointId = obj.TryLookup(L"point").GetNumber();
                winrt::MapElement clickedElement{nullptr};
                auto elements = clickedLayer->MapElements();
                for (uint32_t i = 0; i < elements.Size(); i++)
                {
                    auto elem = elements.GetAt(i);
                    if (winrt::get_self<MapElement>(elem)->Id == winrt::to_hstring(pointId))
                    {
                        clickedElement = elem.try_as<winrt::MapElement>();
                    }
                }
                auto clickArgs = winrt::make_self<MapElementClickEventArgs>(location, clickedElement);
                m_mapElementClickEventSource(*this, *clickArgs);

                clickedLayer->RaiseMapElementClick(*clickArgs);
            }
            // Raises error events from WebView
            else if (type.GetString() == L"javascriptError")
            {
                auto errorArgs = winrt::make_self<MapControlMapServiceErrorOccurredEventArgs>(jsonAsString);
                XamlTelemetry::MapControl_WebMessageReceived_Error(reinterpret_cast<uint64_t>(this), jsonAsString.data());
                m_mapServiceErrorOccurredEventSource(*this, *errorArgs);
            }
        }
    }
}

winrt::fire_and_forget MapControl::ClearLayer(winrt::hstring layerId)
{
    if (auto webView = m_webView.get())
    {
        auto core = co_await GetCoreWebView2();
        co_await core.ExecuteScriptAsync(L"clearLayer(" + layerId + L");");
    }
}

winrt::fire_and_forget MapControl::ResetLayerCollection()
{
    if (auto webView = m_webView.get())
    {
        auto core = co_await GetCoreWebView2();

        for (uint32_t i = 0; i < Layers().Size(); i++)
        {
            winrt::MapElementsLayer layer = Layers().GetAt(i).as<winrt::MapElementsLayer>();
            auto layerId = winrt::get_self<MapElementsLayer>(layer)->Id;
            co_await core.ExecuteScriptAsync(L"clearLayer(" + winrt::to_hstring(layerId) + L");");

            auto elements = layer.MapElements();
            for (uint32_t j = 0; j < elements.Size(); j++)
            {
                auto element = elements.GetAt(j);
                auto location = element.as<winrt::MapIcon>().Location();
                winrt::get_self<MapElement>(element)->Id = co_await AddMapIcon(location,  layerId);
            }
        }
    }
}

void MapControl::OnLayersVectorChanged(const winrt::IObservableVector<winrt::MapLayer>&, const winrt::Collections::IVectorChangedEventArgs& args)
{
    if (args.CollectionChange() == winrt::Collections::CollectionChange::ItemInserted)
    {
        if (winrt::MapElementsLayer elementLayer = Layers().GetAt(args.Index()).try_as<winrt::MapElementsLayer>())
        {
            OnLayerAdded(elementLayer);
        }
    }
    else if (args.CollectionChange() == winrt::Collections::CollectionChange::ItemRemoved)
    {

        if (winrt::MapElementsLayer elementLayer = Layers().GetAt(args.Index()).try_as<winrt::MapElementsLayer>())
        {
            auto layerId = winrt::get_self<MapElementsLayer>(elementLayer)->Id;
            ClearLayer(layerId);
        }
    }
    else if (args.CollectionChange() == winrt::Collections::CollectionChange::Reset)
    {
        ResetLayerCollection();
    }
}

winrt::fire_and_forget MapControl::OnLayerAdded(const winrt::MapElementsLayer layer)
{
    auto mapLayer = winrt::get_self<MapElementsLayer>(layer);
    mapLayer->SetParentMapControl(*this);
    if (auto webView = m_webView.get())
    {
        auto core = co_await GetCoreWebView2();
        auto layerId = winrt::get_self<MapElementsLayer>(layer)->Id;
        winrt::get_self<MapElementsLayer>(layer)->Id = co_await core.ExecuteScriptAsync(L"addSymbolLayer();");

        auto elements = layer.MapElements();
        for (uint32_t i = 0; i < elements.Size(); i++)
        {
            auto element = elements.GetAt(i);
            auto location = element.as<winrt::MapIcon>().Location();
            winrt::get_self<MapElement>(element)->Id = co_await AddMapIcon(location,  winrt::get_self<MapElementsLayer>(layer)->Id);
        }
    }
}

winrt::IAsyncOperation<winrt::hstring> MapControl::AddMapIcon(winrt::Geopoint mapIconPoint, winrt::hstring layerId)
{
    winrt::hstring returnedValue{};
    if (auto webView = m_webView.get())
    {
        auto core = co_await GetCoreWebView2();

        const auto mapIconPosition = mapIconPoint.Position();
        const auto latitude = winrt::to_hstring(mapIconPosition.Latitude);
        const auto longitude = winrt::to_hstring(mapIconPosition.Longitude);

        returnedValue = co_await core.ExecuteScriptAsync(L"addPoint(" + longitude + L", " + latitude + L"," + layerId + L");");
    }
    co_return returnedValue;
}

winrt::fire_and_forget MapControl::RemoveMapIcon(winrt::hstring pointId, winrt::hstring layerId)
{
    if (auto webView = m_webView.get())
    {
        auto core = co_await GetCoreWebView2();

        auto result = co_await core.ExecuteScriptAsync(L"removePoint(" + pointId + L"," + layerId + L");");
    }
}

winrt::fire_and_forget MapControl::UpdateMapIcon(winrt::Geopoint mapIconPoint, winrt::hstring pointId, winrt::hstring layerId)
{
    if (auto webView = m_webView.get())
    {
        auto core = co_await GetCoreWebView2();

        const auto mapIconPosition = mapIconPoint.Position();
        const auto latitude = winrt::to_hstring(mapIconPosition.Latitude);
        const auto longitude = winrt::to_hstring(mapIconPosition.Longitude);

        co_await core.ExecuteScriptAsync(L"updatePoint(" + longitude + L", " + latitude + L"," + pointId + L"," + layerId + L");");
    }
}

winrt::IAsyncAction MapControl::LayerElementsChanged(const winrt::MapElementsLayer& layer, winrt::IVector<winrt::MapElement> elements, const winrt::Collections::IVectorChangedEventArgs& args, winrt::hstring elementId)
{
    const auto index = args.Index();
    auto layerId = winrt::get_self<MapElementsLayer>(layer)->Id;

    switch (args.CollectionChange())
    {
    case winrt::Collections::CollectionChange::ItemInserted:
    {
        auto element = elements.GetAt(index);
        winrt::get_self<MapElement>(element)->Id = co_await AddMapIcon(element.as<winrt::MapIcon>().Location(), layerId);
        break;
    }
    case winrt::Collections::CollectionChange::ItemRemoved:
        RemoveMapIcon(elementId,  layerId);
        break;
    case winrt::Collections::CollectionChange::ItemChanged:
    {
        auto element = elements.GetAt(index);
        RemoveMapIcon(elementId, layerId);
        winrt::get_self<MapElement>(element)->Id = co_await AddMapIcon(element.as<winrt::MapIcon>().Location(), layerId);
        break;
    }
    case winrt::Collections::CollectionChange::Reset:
    {
        OnLayerAdded(layer);
        break;
    }
    }
    co_return;
}

winrt::IAsyncOperation<winrt::hstring> MapControl::GetMapHtmlContent()
{
    if (s_mapHtmlContent.size() == 0)
    {
        s_mapHtmlContent = co_await ResourceAccessor::GetFileContents(FR_Map_Html);
    }

    co_return s_mapHtmlContent;
}