// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ConnectedAnimation.g.h"
#include "ConnectedAnimation.h"
#include "Corep.h"
#include <XamlTraceLogging.h>
#include "UIElement.g.h"

using namespace DirectUI;
using namespace xaml_animation;

_Check_return_ HRESULT
DirectUI::ConnectedAnimation::TryStartWithCoordinatedElementsImpl(_In_ xaml::IUIElement* pDestination, _In_opt_ wfc::IIterable<xaml::UIElement*>* pCoordinatedElements, _Out_ BOOLEAN* pReturnValue)
{
    *pReturnValue = false;

    CConnectedAnimation* coreAnimation = static_cast<CConnectedAnimation*>(this->GetHandle());
    CUIElement* coreElement = static_cast<CUIElement*>(static_cast<UIElement*>(pDestination)->GetHandle());

    IFC_RETURN(coreAnimation->TryStart(coreElement, pReturnValue));

    wrl::ComPtr<wfc::IIterator<xaml::UIElement*>> iterator;
    if (pCoordinatedElements != nullptr && SUCCEEDED(pCoordinatedElements->First(&iterator)))
    {
        boolean hasCurrent = false;
        IFC_RETURN(iterator->get_HasCurrent(&hasCurrent));
        while (hasCurrent)
        {
            wrl::ComPtr<xaml::IUIElement> element;
            IFC_RETURN(iterator->get_Current(&element));
            if (element != nullptr)
            {
                IFC_RETURN(coreAnimation->AnimateCoordinatedEntrance(static_cast<UIElement*>(element.Get())->GetHandle()));
            }
            IFC_RETURN(iterator->MoveNext(&hasCurrent));
        }
    }

    // Log "ConnectedAnimation_TryStart_CoordinatedElements" Event
    TraceLoggingWrite(g_hTraceProvider,
         "ConnectedAnimation_TryStart_CoordinatedElements",
         TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
         TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));

    return S_OK;
}

_Check_return_ HRESULT
DirectUI::ConnectedAnimation::TryStartImpl(_In_ xaml::IUIElement* pDestination, _Out_ BOOLEAN* pReturnValue)
{
    return TryStartWithCoordinatedElementsImpl(pDestination, nullptr, pReturnValue);
}

_Check_return_ HRESULT
DirectUI::ConnectedAnimation::CancelImpl()
{
    CConnectedAnimation* coreAnimation = static_cast<CConnectedAnimation*>(this->GetHandle());
    IFC_RETURN(coreAnimation->Cancel());
    return S_OK;
}

_Check_return_ HRESULT
DirectUI::ConnectedAnimation::SetAnimationComponentImpl(_In_ xaml_animation::ConnectedAnimationComponent component, _In_opt_ WUComp::ICompositionAnimationBase *animation)
{
    CConnectedAnimation* coreAnimation = static_cast<CConnectedAnimation*>(this->GetHandle());
    IFC_RETURN(coreAnimation->SetAnimationComponent(component, animation));
    return S_OK;

}

_Check_return_ HRESULT DirectUI::ConnectedAnimation::get_IsScaleAnimationEnabledImpl(_Out_ BOOLEAN* enabled)
{
    CConnectedAnimation* coreAnimation = static_cast<CConnectedAnimation*>(this->GetHandle());
    coreAnimation->GetScaleAnimationEnabled(enabled);
    return S_OK;
}

_Check_return_ HRESULT DirectUI::ConnectedAnimation::put_IsScaleAnimationEnabledImpl(_In_ BOOLEAN enabled)
{
    CConnectedAnimation* coreAnimation = static_cast<CConnectedAnimation*>(this->GetHandle());
    coreAnimation->PutScaleAnimationEnabled(enabled);
    return S_OK;
}

_Check_return_ HRESULT DirectUI::ConnectedAnimation::get_ConfigurationImpl(_Out_ xaml_animation::IConnectedAnimationConfiguration** config)
{
    CConnectedAnimation* coreAnimation = static_cast<CConnectedAnimation*>(this->GetHandle());
    wrl::ComPtr<IConnectedAnimationCoreConfiguration> coreConfig;
    coreAnimation->GetConfiguration(&coreConfig);
    return coreConfig->QueryInterface(IID_PPV_ARGS(config));
}

_Check_return_ HRESULT DirectUI::ConnectedAnimation::put_ConfigurationImpl(_In_ xaml_animation::IConnectedAnimationConfiguration* config)
{
    CConnectedAnimation* coreAnimation = static_cast<CConnectedAnimation*>(this->GetHandle());
    wrl::ComPtr<IConnectedAnimationCoreConfiguration> coreConfig;
    IFC_RETURN(config->QueryInterface(IID_PPV_ARGS(&coreConfig)));
    coreAnimation->PutConfiguration(coreConfig.Get());
    return S_OK;
}
