// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlIslandRoot.g.h"
#include "FrameworkApplication_Partial.h"
#include "PopupRoot_Partial.h"
#include "JupiterControl.h"
#include "TransitionRoot.g.h"
#include "XamlIslandRoot.h"
#include "CScrollContentPresenter.g.h"
#include "ScrollContentPresenter_Partial.h"
#include "ScrollViewer.h"
#include "ScrollViewer_Partial.h"

using namespace DirectUI;
using namespace ctl;
using namespace xaml;
using namespace xaml_hosting;

DirectUI::XamlIslandRoot::XamlIslandRoot()
    : m_contentManager(this, false /* isUwpWindowContent */)
{}

DirectUI::XamlIslandRoot::~XamlIslandRoot()
{}

_Check_return_ HRESULT DirectUI::XamlIslandRoot::Initialize(_In_ std::nullptr_t)
{
    CXamlIslandRoot* xamlIslandRoot = static_cast<CXamlIslandRoot*>(GetHandle());
    IFC_RETURN(xamlIslandRoot->Initialize());
    return S_OK;
}

_Check_return_ HRESULT DirectUI::XamlIslandRoot::Initialize(_In_ WUComp::Desktop::IDesktopWindowContentBridgeInterop* contentBridge)
{
    CXamlIslandRoot* xamlIslandRoot = static_cast<CXamlIslandRoot*>(GetHandle());
    IFC_RETURN(xamlIslandRoot->Initialize(contentBridge));
    return S_OK;
}

_Check_return_ HRESULT DirectUI::XamlIslandRoot::get_ContentImpl(_Outptr_result_maybenull_ xaml::IUIElement** resultContent)
{
    IFCPTR_RETURN(resultContent);
    *resultContent = nullptr;

    IFC_RETURN(CheckThread());

    *resultContent = m_contentManager.GetContent();
    AddRefInterface(*resultContent);

    return S_OK;
}

void DirectUI::XamlIslandRoot::OnSizeChangedStatic(_In_ CXamlIslandRoot* xamlIslandRoot)
{
    ctl::ComPtr<DirectUI::DependencyObject> directuiDo;
    IFCFAILFAST(DXamlCore::GetCurrent()->GetPeer(xamlIslandRoot, &directuiDo));
    DirectUI::XamlIslandRoot* pThis = static_cast<DirectUI::XamlIslandRoot*>(directuiDo.Get());

    IFCFAILFAST(pThis->m_contentManager.OnWindowSizeChanged());
}

template <typename T_core, typename T_directui, typename T_interface>
T_core* ToCorePtr(_In_opt_ T_interface* interfacePtr)
{
    ctl::ComPtr<T_interface> spPtr(interfacePtr);
    if (spPtr.Get() == nullptr)
    {
        return nullptr;
    }
    ctl::ComPtr<T_directui> directUIPtr;
    IFCFAILFAST(spPtr.As(&directUIPtr));
    return static_cast<T_core*>(directUIPtr->GetHandle());
}

_Check_return_ HRESULT DirectUI::XamlIslandRoot::put_ContentImpl(_In_opt_ xaml::IUIElement* value)
{
    IFC_RETURN(CheckThread());
    IFC_RETURN(m_contentManager.SetContent(value));

    CScrollViewer* coreRootScrollViewer = ToCorePtr<CScrollViewer, DirectUI::ScrollViewer>(m_contentManager.GetRootScrollViewer());
    if (coreRootScrollViewer)
    {
        CValue alignmentValue;
        alignmentValue.Set(DirectUI::VerticalAlignment::Top);
        IFCFAILFAST(coreRootScrollViewer->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_VerticalAlignment, alignmentValue));
        // Set the root ScrollViewer explicitly on the ScrollContentControl.
        coreRootScrollViewer->SetRootScrollViewer(TRUE);
    }

    CContentPresenter* coreContentPresenter = ToCorePtr<CScrollContentPresenter, DirectUI::ScrollContentPresenter>(m_contentManager.GetRootSVContentPresenter());
    CUIElement* coreContent = ToCorePtr<CUIElement, DirectUI::UIElement>(value);
    CXamlIslandRoot* coreXamlIsland = static_cast<CXamlIslandRoot*>(GetHandle());

    IFC_RETURN(coreXamlIsland->SetPublicRootVisual(coreContent, coreRootScrollViewer, coreContentPresenter));
    return S_OK;
}

xaml_controls::IScrollViewer* DirectUI::XamlIslandRoot::GetRootScrollViewer()
{
    return m_contentManager.GetRootScrollViewer();
}

_Check_return_ HRESULT DirectUI::XamlIslandRootFactory::GetIslandFromElementImpl(
    _In_ xaml::IDependencyObject* element,
    _Outptr_result_maybenull_ xaml_hosting::IXamlIslandRoot** result)
{
    *result = nullptr;

    DXamlCore* dxamlCore = DXamlCore::GetCurrent();
    CCoreServices* core = static_cast<CCoreServices*>(dxamlCore->GetHandle());
    CDependencyObject* coreElement = static_cast<DirectUI::DependencyObject*>(element)->GetHandle();
    CXamlIslandRoot* coreXamlIsland = do_pointer_cast<CXamlIslandRoot>(core->GetRootForElement(coreElement));
    if (coreXamlIsland)
    {
        ctl::ComPtr<XamlIslandRoot> xamlIslandRoot;
        dxamlCore->GetPeer<XamlIslandRoot>(coreXamlIsland, &xamlIslandRoot);

        *result = xamlIslandRoot.Detach();
    }
    return S_OK;
}

_Check_return_ HRESULT DirectUI::XamlIslandRoot::get_FocusControllerImpl(_Outptr_ IInspectable** value)
{
    *value = nullptr;

    auto xamlIslandRoot = static_cast<CXamlIslandRoot*>(GetHandle());
    wrl::ComPtr<IUnknown> spUnk;
    IFC_RETURN(xamlIslandRoot->get_FocusController(&spUnk));
    IFC_RETURN(spUnk.CopyTo(value));

    return S_OK;
}

void DirectUI::XamlIslandRoot::SetOwner(_In_opt_ IInspectable* owner)
{
    if (owner != nullptr)
    {
        IGNOREHR(wrl::AsWeak(owner, &m_owner));
    }
    else
    {
        m_owner = nullptr;
    }
}

bool DirectUI::XamlIslandRoot::TryGetOwner(_COM_Outptr_opt_result_maybenull_ IInspectable** owner)
{
    *owner = nullptr;
    wrl::ComPtr<IInspectable> resolvedOwner;
    if (m_owner && SUCCEEDED(m_owner.As(&resolvedOwner)))
    {
        *owner = resolvedOwner.Detach();
        return true;
    }
    return false;
}


