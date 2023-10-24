// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DisplayOrientationHelper.h"
#include "Window.g.h"

using namespace DirectUI;

namespace XamlDisplay
{
// Xaml needs to use different APIs in order to determine the display orientation depending on what context it is running in.
// UWP apps make use of the DisplayInformation type while Win32 apps need to use the HMONITOR and HWIND APIs.
_Check_return_ HRESULT 
    GetDisplayOrientation(_In_ CDependencyObject* dependencyObject, _Out_ Orientation& orientation)
{
    IFCPTR_RETURN(dependencyObject);
    auto currentOrientation = Orientation::None;

    CCoreServices* coreServices = DXamlServices::GetHandle();
    if (coreServices->GetInitializationType() != InitializationType::IslandsOnly)
    {
        ctl::ComPtr<wgrd::IDisplayInformation> displayInformation;
        wgrd::DisplayOrientations currentDisplayOrientation = wgrd::DisplayOrientations_None;
        GetUWPDisplayInformation(displayInformation);

        IFC_RETURN(displayInformation->get_CurrentOrientation(&currentDisplayOrientation));

        // Map to XamlDisplayOrientation type
        switch(currentDisplayOrientation)
        {
            case wgrd::DisplayOrientations_Landscape:
                currentOrientation = Orientation::Landscape;
                break;
            case wgrd::DisplayOrientations_LandscapeFlipped:
                currentOrientation = Orientation::LandscapeFlipped;
                break;
            case wgrd::DisplayOrientations_Portrait:
                currentOrientation = Orientation::Portrait;
                break;
            case wgrd::DisplayOrientations_PortraitFlipped:
                currentOrientation = Orientation::PortraitFlipped;
                break;
            default:
                break;
        }
    }
    else
    {
        UINT rotationValue;
        if(SUCCEEDED(GetWin32DisplayOrientation(dependencyObject, rotationValue)))
        {
            // Map to XamlDisplayOrientation type
            switch(rotationValue)
            {
                case DMDO_DEFAULT:
                    // Currently assuming that DMDO_DEFAULT (natural orientation of the device) is always landscape.
                    currentOrientation = Orientation::Landscape;
                    break;
                case DMDO_180:
                    currentOrientation = Orientation::LandscapeFlipped;
                    break;
                case DMDO_90:
                    currentOrientation = Orientation::Portrait;
                    break;
                case DMDO_270:
                    currentOrientation = Orientation::PortraitFlipped;
                    break;
                default:
                    break;
            }
        }
    }

    orientation = currentOrientation;
    return S_OK;
}

_Check_return_ HRESULT 
    GetWin32DisplayOrientation(_In_ CDependencyObject* dependencyObject, _Out_ UINT& rotationValue)
    {
        IFCPTR_RETURN(dependencyObject);
        CXamlIslandRoot* xamlIslandRoot = VisualTree::GetXamlIslandRootForElement(dependencyObject);
        if(xamlIslandRoot)
        {
            HWND backingHwnd = xamlIslandRoot->GetPositioningHWND();
            HMONITOR myMonitor = MonitorFromWindow(backingHwnd, MONITOR_DEFAULTTONEAREST);
            if(myMonitor != NULL)
            {
                MONITORINFOEX monitorInfo = {};
                monitorInfo.cbSize = sizeof(monitorInfo);
                GetMonitorInfo(myMonitor, &monitorInfo);

                DEVMODEW devMode = {};
                devMode.dmSize = sizeof(devMode);
                devMode.dmSpecVersion = DM_SPECVERSION;

                EnumDisplaySettings(monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &devMode);

                if (devMode.dmFields & DM_DISPLAYORIENTATION)
                {
                    rotationValue = devMode.dmDisplayOrientation;
                    return S_OK;
                }
            }
        }

        return E_FAIL;
    }

_Check_return_ HRESULT 
    GetUWPDisplayInformation(_Out_ ctl::ComPtr<wgrd::IDisplayInformation>& displayInformation)
    {
        ctl::ComPtr<wgrd::IDisplayInformationStatics> displayInformationStatics;

        IFC_RETURN(ctl::GetActivationFactory(wrl_wrappers::HStringReference(
                RuntimeClass_Windows_Graphics_Display_DisplayInformation).Get(),
                &displayInformationStatics));
        IFC_RETURN(displayInformationStatics->GetForCurrentView(&displayInformation));

        return S_OK;
    }
}