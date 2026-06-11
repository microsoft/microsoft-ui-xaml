// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ConnectedAnimationService.g.h"
#include "ConnectedAnimationService.h"
#include "ConnectedAnimation.h"
#include "Corep.h"
#include "EasingFunctionBase.g.h"
#include "UIElement.g.h"

using namespace DirectUI;

_Check_return_ HRESULT
DirectUI::ConnectedAnimationServiceFactory::GetForCurrentViewImpl(_Outptr_ xaml_animation::IConnectedAnimationService** ppReturnValue)
{
    *ppReturnValue = nullptr;


    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCEXPECT_RETURN(pCore);

    pCore->GetHandle()->EnsureConnectedAnimationService();
    CConnectedAnimationService* coreAnimationService = pCore->GetHandle()->GetConnectedAnimationServiceNoRef();
    IFCEXPECT_RETURN(coreAnimationService);

    ctl::ComPtr<DependencyObject> dependencyObject;
    IFC_RETURN(pCore->GetPeer(coreAnimationService, &dependencyObject));
    IFC_RETURN(ctl::do_query_interface<xaml_animation::IConnectedAnimationService>(*ppReturnValue, dependencyObject.Get()));
    return S_OK;
}

_Check_return_ HRESULT
DirectUI::ConnectedAnimationService::PrepareToAnimateImpl(_In_ HSTRING key, _In_ xaml::IUIElement* pSource, _Outptr_ xaml_animation::IConnectedAnimation** ppReturnValue)
{
    *ppReturnValue = nullptr;

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCEXPECT_RETURN(pCore);

    CConnectedAnimationService* coreAnimationService = static_cast<CConnectedAnimationService*>(GetHandle());

    xref_ptr<CConnectedAnimation> coreAnimation;
    IFC_RETURN(coreAnimationService->CreateAnimation(xephemeral_string_ptr(key), coreAnimation.ReleaseAndGetAddressOf()));

    CUIElement* coreElement = static_cast<CUIElement*>(static_cast<UIElement*>(pSource)->GetHandle());
    IFC_RETURN(coreAnimation->Prepare(coreElement));

    ctl::ComPtr<DependencyObject> dependencyObject;
    IFC_RETURN(pCore->GetPeer(coreAnimation.get(), &dependencyObject));
    IFC_RETURN(ctl::do_query_interface<xaml_animation::IConnectedAnimation>(*ppReturnValue, dependencyObject.Get()));
    return S_OK;
}

_Check_return_ HRESULT
DirectUI::ConnectedAnimationService::GetAnimationImpl(_In_ HSTRING key, _Outptr_opt_ xaml_animation::IConnectedAnimation** ppReturnValue)
{
    *ppReturnValue = nullptr;

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCEXPECT_RETURN(pCore);

    CConnectedAnimationService* coreAnimationService = static_cast<CConnectedAnimationService*>(GetHandle());

    xref_ptr<CConnectedAnimation> coreAnimation;
    IFC_RETURN(coreAnimationService->GetAnimation(xephemeral_string_ptr(key), coreAnimation.ReleaseAndGetAddressOf()));

    if (coreAnimation != nullptr)
    {
        ctl::ComPtr<DependencyObject> dependencyObject;
        IFC_RETURN(pCore->GetPeer(coreAnimation.get(), &dependencyObject));
        IFC_RETURN(ctl::do_query_interface<xaml_animation::IConnectedAnimation>(*ppReturnValue, dependencyObject.Get()));
    }
    return S_OK;
}

_Check_return_ HRESULT DirectUI::ConnectedAnimationService::get_DefaultDurationImpl(_Out_ wf::TimeSpan* pValue)
{
    CConnectedAnimationService* coreAnimationService = static_cast<CConnectedAnimationService*>(GetHandle());
    *pValue = coreAnimationService->GetDefaultDuration();
    return S_OK;
}

_Check_return_ HRESULT DirectUI::ConnectedAnimationService::put_DefaultDurationImpl(_In_ wf::TimeSpan value)
{
    CConnectedAnimationService* coreAnimationService = static_cast<CConnectedAnimationService*>(GetHandle());
    coreAnimationService->SetDefaultDuration(value);
    return S_OK;
}

_Check_return_ HRESULT DirectUI::ConnectedAnimationService::get_DefaultEasingFunctionImpl(_Outptr_result_maybenull_ WUComp::ICompositionEasingFunction** ppValue)
{
    CConnectedAnimationService* coreAnimationService = static_cast<CConnectedAnimationService*>(GetHandle());
    IFC_RETURN(coreAnimationService->GetDefaultEasingFunction(ppValue));
    return S_OK;
}

_Check_return_ HRESULT DirectUI::ConnectedAnimationService::put_DefaultEasingFunctionImpl(_In_opt_ WUComp::ICompositionEasingFunction* pValue)
{
    CConnectedAnimationService* coreAnimationService = static_cast<CConnectedAnimationService*>(GetHandle());
    IFC_RETURN(coreAnimationService->SetDefaultEasingFunction(pValue))
    return S_OK;
}

