// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#include "precomp.h"
#include "Window.g.h"
#include "XamlRoot.g.h"
#include <CStaticLock.h>
#include "IApplicationBarService.h"

using namespace std::placeholders;
using namespace DirectUI;
using namespace DirectUISynonyms;

LayoutBoundsChangedHelper::~LayoutBoundsChangedHelper()
{
    LayoutBoundsChangedHelper::DisconnectWindowEventHandlers();
}

void LayoutBoundsChangedHelper::DisconnectWindowEventHandlers()
{
    auto xamlRoot = m_weakXamlRoot.AsOrNull<xaml::IXamlRoot>();

    if (m_xamlRootChangedEventHandler && xamlRoot)
    {
        VERIFYHR(m_xamlRootChangedEventHandler.DetachEventHandler(xamlRoot.Get()));
    }

    if (m_tokAppViewVisibleBoundsChanged.value)
    {
        ctl::ComPtr<wuv::IApplicationView2> spAppView2;
        IGNOREHR(GetApplicationView2(&spAppView2));

        if (spAppView2)
        {
            spAppView2->remove_VisibleBoundsChanged(m_tokAppViewVisibleBoundsChanged);
        }

        m_tokAppViewVisibleBoundsChanged.value = 0;
    }
}

_Check_return_ HRESULT LayoutBoundsChangedHelper::GetApplicationView2(_Out_ wuv::IApplicationView2** appView2)
{
    *appView2 = nullptr;

    AppPolicyWindowingModel policy = AppPolicyWindowingModel_None;

    if (AppPolicyGetWindowingModel(GetCurrentThreadEffectiveToken(), &policy) != ERROR_SUCCESS)
    {
        IFC_RETURN(E_FAIL);
    }

    // If we're in the context of XAML islands, then we don't want to use GetForCurrentView -
    // that requires CoreWindow or application view, which is not supported in islands.

    if (policy == AppPolicyWindowingModel_Universal)
    {
        ctl::ComPtr<wuv::IApplicationViewStatics2> spAppViewStatics;
        ctl::ComPtr<wuv::IApplicationView> spAppView;
        ctl::ComPtr<wuv::IApplicationView2> spAppView2;

        IFC_RETURN(ctl::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_ApplicationView).Get(), &spAppViewStatics));
        IFC_RETURN(spAppViewStatics->GetForCurrentView(&spAppView));

        IFC_RETURN(spAppView.As<wuv::IApplicationView2>(&spAppView2));
        *appView2 = spAppView2.Detach();
    }

    return S_OK;
}

_Check_return_ HRESULT LayoutBoundsChangedHelper::GetDesiredBoundsMode(_Out_ wuv::ApplicationViewBoundsMode* boundsMode)
{
    ctl::ComPtr<wuv::IApplicationView2> spAppView2;
    IGNOREHR(GetApplicationView2(&spAppView2));
    if(spAppView2)
    {
        IFC_RETURN(spAppView2->get_DesiredBoundsMode(boundsMode));
    }
    else
    {
        *boundsMode = wuv::ApplicationViewBoundsMode::ApplicationViewBoundsMode_UseCoreWindow;
    }

    return S_OK;
}

void LayoutBoundsChangedHelper::AddLayoutBoundsChangedCallback(
    _In_ std::function<HRESULT()> callback,
    _Out_ EventRegistrationToken* tokParentBoundsChanged)
{
    m_callbacks[m_eventTokenNumber] = callback;
    tokParentBoundsChanged->value = m_eventTokenNumber;
    m_eventTokenNumber++;
}

void LayoutBoundsChangedHelper::RemoveLayoutBoundsChangedCallback(_In_ EventRegistrationToken* tokParentBoundsChanged)
{
    if (tokParentBoundsChanged && tokParentBoundsChanged->value)
    {
        m_callbacks.erase(static_cast<int>(tokParentBoundsChanged->value));
        tokParentBoundsChanged->value = 0;
    }
}

_Check_return_ HRESULT LayoutBoundsChangedHelper::InitializeSizeChangedHandlers(_In_ xaml::IXamlRoot* xamlRoot)
{
    if (!m_xamlRootChangedEventHandler)
    {
        IFC_RETURN(ctl::AsWeak(xamlRoot, &m_weakXamlRoot));
        IFC_RETURN(m_xamlRootChangedEventHandler.AttachEventHandler(
            xamlRoot,
            std::bind(&LayoutBoundsChangedHelper::OnXamlRootChanged, this, _1, _2)));
    }

    if(m_tokAppViewVisibleBoundsChanged.value == 0)
    {
        ctl::ComPtr<wuv::IApplicationView2> spAppView2;
        IGNOREHR(GetApplicationView2(&spAppView2));

        if(spAppView2)
        {
            IFC_RETURN(spAppView2->add_VisibleBoundsChanged(
                Microsoft::WRL::Callback<wf::ITypedEventHandler<wuv::ApplicationView*, IInspectable*>>(
                this,
                &LayoutBoundsChangedHelper::OnApplicationViewVisibleBoundsChanged).Get(),
                &m_tokAppViewVisibleBoundsChanged));
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
LayoutBoundsChangedHelper::OnXamlRootChanged(
    _In_ xaml::IXamlRoot* pSender,
    _In_ xaml::IXamlRootChangedEventArgs* pArgs)
{
    IFC_RETURN(OnPotentialParentBoundsChanged(wuv::ApplicationViewBoundsMode::ApplicationViewBoundsMode_UseCoreWindow));
    return S_OK;
}

_Check_return_ HRESULT
LayoutBoundsChangedHelper::OnApplicationViewVisibleBoundsChanged(
    _In_ wuv::IApplicationView* pSender,
    _In_ IInspectable* pArgs)
{
    IFC_RETURN(OnPotentialParentBoundsChanged(wuv::ApplicationViewBoundsMode::ApplicationViewBoundsMode_UseVisible));
    return S_OK;
}


_Check_return_ HRESULT LayoutBoundsChangedHelper::OnPotentialParentBoundsChanged(wuv::ApplicationViewBoundsMode desiredBoundsMode)
{
    HRESULT result = S_OK;
    wuv::ApplicationViewBoundsMode boundsMode;
    IFC_RETURN(GetDesiredBoundsMode(&boundsMode));

    if(boundsMode == desiredBoundsMode)
    {
        for(auto&& callback : m_callbacks)
        {
            auto const returnValue = callback.second();
            if(FAILED(returnValue))
            {
                result = returnValue;
            }
        }
    }

    UpdateApplicationBarServiceBounds();

    return result;
}

_Check_return_ HRESULT LayoutBoundsChangedHelper::UpdateApplicationBarServiceBounds()
{
    auto xamlRoot = m_weakXamlRoot.AsOrNull<xaml::IXamlRoot>();

    if (xamlRoot)
    {
        ctl::ComPtr<IApplicationBarService> applicationBarService;
        IFC_RETURN(xamlRoot.Cast<XamlRoot>()->TryGetApplicationBarService(applicationBarService));
        if (applicationBarService)
        {
            IFC_RETURN(applicationBarService->OnBoundsChanged());
        }
    }

    return S_OK;
}
