# DebugSettings 

## Table of Contents

- [DebugSettings.EnableFrameRateCounter](#debugsettingsenableframeratecounter)
- [DebugSettings.IsOverdrawHeatMapEnabled](#debugsettingsisoverdrawheatmapenabled)
- [DebugSettings.EnableRedrawRegions](#debugsettingsenableredrawregions)
- [Notes](#notes)

In the 19H1 release, the `Windows.UI.Xaml.DebugSettings` class includes 6 read-write boolean properties and 1 event
(https://docs.microsoft.com/en-us/uwp/api/Windows.UI.Xaml.DebugSettings).

For the following 3 properties and 1 event, the implementations are dependence-free and testing with the latest
lifted alpha release & latest WinUI 3 codebase has shown that they work just fine: 

* `FailFastOnErrors`
* `IsBindingTracingEnabled`
* `BindingFailed`
* `IsTextPerformanceVisualizationEnabled`

So, no action item is needed to ship them in WinUI 3 RTM. 

That leaves us with 3 remaining properties that are more or less dependent on DComp APIs: 

* [`EnableFrameRateCounter`](#debugsettings.enableframeratecounter)
* [`IsOverdrawHeatMapEnabled`](#debugsettings.isoverdrawheatmapenabled)
* [`EnableRedrawRegions`](#debugsettings.enableredrawregions)

## DebugSettings.EnableFrameRateCounter  

`EnableFrameRateCounter` shows two sets of counters: 

***Xaml-specific UI-thread frame count and UI-thread CPU time in ms at the top-left corner of the app (i.e. top-left of XamlRoot element)***

These counters’ implementation is Xaml-specific and testing with the latest lifted alpha release & latest WinUI 3 
codebase has shown that they work just fine. So, no action item is needed to ship them in WinUI 3 RTM. 

***System-wide Composition frame count and CPU time in ms at the top-right corner of the app screen.***

These counters’ implementation is Comp-dependent.
The current implementation in 
[`dxaml/xcp/components/comptree/DCompTreeHost.cpp`](../../dxaml/xcp/components/comptree/DCompTreeHost.cpp)'s
`DCompTreeHost::UpdateDebugSettings(…)` uses old school `IDCompositionDeviceDebug::EnableDebugCounters()` and 
`IDCompositionDeviceDebug::DisableDebugCounters()` APIs that are considered private and cannot be used in WinUI 3.
There is no modern WinComp alternative API now or for WinUI 3. A future implementation
of `IDCompositionDeviceDebug::EnableDebugCounters()/DisableDebugCounters()` replacements is tracked separately.

***Plan***

The plan for WinUI 3 is to keep the `DebugSettings.EnableFrameRateCounter` in WinUI 3 RTM: 

* keep the Xaml-specific counters which work fine, 
* remove the Comp-specific counters for a lack of usable APIs (remove the old IDCompositionDeviceDebug use in
  `DCompTreeHost::UpdateDebugSettings(…)`). 

## DebugSettings.IsOverdrawHeatMapEnabled  

`IsOverdrawHeatMapEnabled` shows UI blocks in semi-transparent red the same way in 19H1 and lifted alpha releases.
The current implementation uses old school
`IDCompositionVisualDebug::EnableHeatMap(D2D1::ColorF(D2D1::ColorF::Red, 0.125f))`
and `IDCompositionVisualDebug::DisableHeatMap()` APIs which
are considered private and cannot be used in WinUI 3.  

There is a new WinComp `ICompositionDebugHeatMaps::ShowOverdraw(IVisual, CompositionDebugOverdrawContentKinds)`
API available and experimentation has shown that calling 
`ShowOverdraw(IVisual, CompositionDebugOverdrawContentKinds::All)` results in no visual effect.
This issue is tracked but not expected to be addressed for WinUI 3. 

The plan is to remove that `DebugSettings.IsOverdrawHeatMapEnabled` property from the public object model in WinUI 3 RTM.
Commenting out the current implementation and using `#ifdef IsOverdrawHeatMapEnabled` seems appropriate since this
would be a temporary removal. 

## DebugSettings.EnableRedrawRegions  

`EnableRedrawRegions` is currently using the private API `IDCompositionVisualDebug::EnableRedrawRegions` 
which cannot be used in WinUI 3 (and apparently does not work in WinUI 3 anyway).
There is a replacement WinComp API `ICompositionDebugHeatMaps::ShowRedraw(IVisual)` but
experimentations have shown that it has no visual effect on the latest alpha and live lifted frameworks.
This issue is tracked but not expected to be addressed for WinUI 3. 

The plan is to remove that `DebugSettings.EnableRedrawRegions` property from the public object model in WinUI 3 RTM.
Commenting out the current implementation and using `#ifdef EnableRedrawRegions` seems appropriate since this would
be a temporary removal. 

## Notes 

For recording purposes and ease of future implementations, here are code snippets used to exercise the new WinComp APIs: 

For the `DebugSettings.IsOverdrawHeatMapEnabled` property prototyping: 

```cpp
_Check_return_ HRESULT 
DCompTreeHost::UpdateDebugSettings( 
    bool isFrameRateCounterEnabled, 
    bool isOverdrawHeatmapEnabled, 
    bool areRedrawRegionsEnabled) 
{ 
  if (m_pHWndRootVisual) 
  { 
    IDCompositionVisual2 *pRootVisualNoRef = m_pHWndRootVisual->GetDCompVisual(); 
    Microsoft::WRL::ComPtr<WUComp::IVisual> visual; 
 
    IGNOREHR(pRootVisualNoRef->QueryInterface(IID_PPV_ARGS(&visual))); 
    if (visual) 
    { 
      wrl_wrappers::HStringReference strIWindowClassId( 
        RuntimeClass_Microsoft_UI_Composition_Diagnostics_CompositionDebugSettings); 
      Microsoft::WRL::ComPtr<XAML_ABI_PARAMETER( 
        Microsoft::UI::Composition::Diagnostics::ICompositionDebugSettingsStatics)> settingsSt; 
 
      IGNOREHR(wf::GetActivationFactory(strIWindowClassId.Get(), &settingsSt)); 
      if (settingsSt) 
      { 
        Microsoft::WRL::ComPtr<XAML_ABI_PARAMETER( 
          Microsoft::UI::Composition::Diagnostics::ICompositionDebugSettings)> settings; 
 
        IGNOREHR(settingsSt->TryGetSettings(GetCompositor(), &settings)); 
        if (settings) 
        { 
          Microsoft::WRL::ComPtr<XAML_ABI_PARAMETER( 
            Microsoft::UI::Composition::Diagnostics::ICompositionDebugHeatMaps)> heatMaps; 
  
          IGNOREHR(settings->get_HeatMaps(&heatMaps)); 
          if (heatMaps) 
          { 
            if (isOverdrawHeatmapEnabled) 
            { 
              IGNOREHR(heatMaps->ShowOverdraw( 
                visual.Get(), 
                static_cast<ABI::M::UI::C::D::CompositionDebugOverdrawContentKinds>(All))); 
            } 
            else 
            { 
              IGNOREHR(heatMaps->Hide(visual.Get())); 
            } 
          } 
        } 
      } 
    } 
  } 
} 
```

For the `DebugSettings.EnableRedrawRegions` property prototyping: 

```cpp
_Check_return_ HRESULT 
DCompTreeHost::UpdateDebugSettings( 
    bool isFrameRateCounterEnabled, 
    bool isOverdrawHeatmapEnabled, 
    bool areRedrawRegionsEnabled) 
{ 
  if (m_pHWndRootVisual) 
  { 
    IDCompositionVisual2 *pRootVisualNoRef = m_pHWndRootVisual->GetDCompVisual(); 
    Microsoft::WRL::ComPtr<WUComp::IVisual> visual; 
 
    IGNOREHR(pRootVisualNoRef->QueryInterface(IID_PPV_ARGS(&visual))); 
    if (visual) 
    { 
      wrl_wrappers::HStringReference strIWindowClassId( 
        RuntimeClass_Microsoft_UI_Composition_Diagnostics_CompositionDebugSettings); 
      Microsoft::WRL::ComPtr<XAML_ABI_PARAMETER( 
        Microsoft::UI::Composition::Diagnostics::ICompositionDebugSettingsStatics)> settingsSt; 
 
      IGNOREHR(wf::GetActivationFactory(strIWindowClassId.Get(), &settingsSt)); 
      if (settingsSt) 
      { 
        Microsoft::WRL::ComPtr<XAML_ABI_PARAMETER( 
          Microsoft::UI::Composition::Diagnostics::ICompositionDebugSettings)> settings; 
 
        IGNOREHR(settingsSt->TryGetSettings(GetCompositor(), &settings)); 
        if (settings) 
        { 
          Microsoft::WRL::ComPtr<XAML_ABI_PARAMETER( 
            Microsoft::UI::Composition::Diagnostics::ICompositionDebugHeatMaps)> heatMaps; 
  
          IGNOREHR(settings->get_HeatMaps(&heatMaps)); 
          if (heatMaps) 
          { 
            if (areRedrawRegionsEnabled) 
            { 
              IGNOREHR(heatMaps->ShowRedraw(visual.Get())); 
            } 
            else 
            { 
              IGNOREHR(heatMaps->Hide(visual.Get())); 
            } 
          } 
        } 
      } 
    } 
  } 
} 
```
