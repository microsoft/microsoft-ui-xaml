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
        // Note: This branch is meant to address UWP scenarios, and can be removed once test
        // dependency on the CoreWindow is resolved.
        
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
        auto rotationValue = ixp::ContentDisplayOrientations_None;
        if (SUCCEEDED(GetIslandDisplayOrientation(dependencyObject, rotationValue)))
        {
            // Map to XamlDisplayOrientation type
            switch(rotationValue)
            {
                case ixp::ContentDisplayOrientations_Landscape:
                    currentOrientation = Orientation::Landscape;
                    break;
                case ixp::ContentDisplayOrientations_LandscapeFlipped:
                    currentOrientation = Orientation::LandscapeFlipped;
                    break;
                case ixp::ContentDisplayOrientations_Portrait:
                    currentOrientation = Orientation::Portrait;
                    break;
                case ixp::ContentDisplayOrientations_PortraitFlipped:
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
    GetIslandDisplayOrientation(_In_ CDependencyObject* dependencyObject, _Out_ ixp::ContentDisplayOrientations& rotationValue)
    {
        IFCPTR_RETURN(dependencyObject);
        CXamlIslandRoot* xamlIslandRoot = VisualTree::GetXamlIslandRootForElement(dependencyObject);
        if (xamlIslandRoot)
        {
            wrl::ComPtr<ixp::IContentIslandEnvironment> islandEnvironment = xamlIslandRoot->GetContentIslandEnvironment();
            
            if (islandEnvironment)
            {
                wrl::ComPtr<ixp::IContentIslandEnvironment2> islandEnvironment2;
                IFCFAILFAST(islandEnvironment.As(&islandEnvironment2));

                IFC_RETURN(islandEnvironment2->get_CurrentOrientation(&rotationValue));
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