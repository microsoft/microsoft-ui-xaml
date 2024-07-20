// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"
#include "MapControl.g.h"
#include "MapControl.properties.h"
#include "WebView2.h"
#include "MapElementsLayer.h"
#include "winrt/Windows.Foundation.h"
#include <winrt/Windows.Devices.Geolocation.h>


class MapControl :
    public ReferenceTracker<MapControl, winrt::implementation::MapControlT>,
    public MapControlProperties
{

public:
    MapControl();

    void OnApplyTemplate();
    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    winrt::fire_and_forget LayerElementsChanged(const winrt::MapElementsLayer& layer, const winrt::IObservableVector<winrt::MapElement>& elements, const winrt::Collections::IVectorChangedEventArgs& args, winrt::hstring elementId);
    winrt::fire_and_forget MapControl::UpdateMapIcon(winrt::Geopoint mapIconPoint, winrt::hstring pointId, winrt::hstring layerId);

private:
 
    tracker_ref<winrt::WebView2> m_webView{ this };

    bool IsValidMapServiceToken();
    winrt::IAsyncOperation<winrt::hstring> InitializeWebMap();
    winrt::fire_and_forget SetUpWebView();
    winrt::fire_and_forget OnLayerAdded(const winrt::MapElementsLayer layer);
    winrt::IAsyncOperation<winrt::hstring> AddMapIcon(winrt::Geopoint mapIconPoint, winrt::hstring layerId);
    winrt::IAsyncOperation<winrt::CoreWebView2> MapControl::GetCoreWebView2();
    winrt::fire_and_forget RemoveMapIcon(winrt::hstring pointId, winrt::hstring layerId);
    winrt::fire_and_forget ClearLayer(winrt::hstring layerId);
    winrt::fire_and_forget ResetLayerCollection();
    void OnLayersVectorChanged(const winrt::IObservableVector<winrt::MapLayer>&, const winrt::Collections::IVectorChangedEventArgs& args);
    void WebMessageReceived(winrt::WebView2 sender, winrt::CoreWebView2WebMessageReceivedEventArgs args);

    winrt::WebView2::WebMessageReceived_revoker m_webviewWebMessageReceivedRevoker{};
    winrt::WebView2::NavigationCompleted_revoker m_webviewNavigationCompletedRevoker{};

    winrt::fire_and_forget UpdateMapServiceTokenInWebPage();
    winrt::fire_and_forget UpdateCenterInWebPage();
    winrt::fire_and_forget UpdateZoomLevelInWebPage();
    winrt::fire_and_forget UpdateControlsInWebPage();

    winrt::event_token m_layersVectorChangedToken{};

    static winrt::IAsyncOperation<winrt::hstring> GetMapHtmlContent();
    static winrt::hstring s_mapHtmlContent;
};
