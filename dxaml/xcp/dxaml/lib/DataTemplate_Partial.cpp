// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DataTemplate.g.h"
#include "UIElementCollection.g.h"
#include "UIElement.g.h"

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT DirectUI::DataTemplate::LoadContentImpl(_Outptr_ xaml::IDependencyObject** returnValue)
{
    xref_ptr<CDependencyObject> returnValueNative;

    // Note that ContentControl/ContentPresenter and controls that use them call CDataTemplate::LoadContent
    // directly, so we can't just look for an app callback here and call it. Instead we have to route
    // everything through the core. Controls that call IElementFactory directly like ItemsRepeater go
    // through the DXaml peer here.
    HRESULT hr = CoreImports::CDataTemplate_LoadContent(static_cast<CDataTemplate*>(GetHandle()), returnValueNative.ReleaseAndGetAddressOf());
    if (FAILED(hr) && !m_elementFactory)
    {
        // Markup case only: translate a parse failure to XamlParseFailed. The CLR knows how to
        // translate this to a XamlParseException. For a code-authored template there is no parse,
        // so the callback's original failure HRESULT is propagated unchanged.
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_XAML, E_XAMLPARSEFAILED);
    }
    IFC_RETURN(hr);

    if (returnValueNative)
    {
        ctl::ComPtr<DependencyObject> returnValuePeer;
        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(returnValueNative.get(), &returnValuePeer));

        // Template loading leaves the root pegged until an owning reference exists
        // outside the parser. The returned interface (or this peer during failure
        // cleanup) now provides that reference.
        auto unpegOnExit = wil::scope_exit([&] { returnValuePeer->UnpegNoRef(); });

        IFC_RETURN(ctl::do_query_interface(*returnValue, returnValuePeer.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT DirectUI::DataTemplate::SetElementFactory(_In_ xaml::IDataTemplateElementFactory* value)
{
    SetPtrValue(m_elementFactory, value);
    // Mark the core object so the core LoadContent path can call this callback. The factory
    // is always non-null (set once at construction), so the core flag is only ever enabled.
    IFC_RETURN(CoreImports::CDataTemplate_SetHasElementFactory(static_cast<CDataTemplate*>(GetHandle()), true));
    return S_OK;
}

_Check_return_ HRESULT DirectUI::DataTemplate::CreateElementFromFactory(
    _In_ CDependencyObject* nativeTemplate,
    _Outptr_result_maybenull_ CDependencyObject** ppResult)
{
    *ppResult = nullptr;

    ctl::ComPtr<DependencyObject> peer;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(nativeTemplate, &peer));

    DataTemplate* dataTemplate = peer.Cast<DataTemplate>();
    ASSERT(dataTemplate->m_elementFactory);

    ctl::ComPtr<xaml::IUIElement> element;
    IFC_RETURN(dataTemplate->m_elementFactory.Get()->Invoke(&element));

    // A null result is tolerated, matching an empty <DataTemplate/>
    if (element)
    {
        CDependencyObject* nativeElement = static_cast<UIElement*>(element.Get())->GetHandle();

        // The app created this element and may have attached handlers (e.g. Loaded, DataContextChanged),
        // so its peer already exists and is stateful, but it is not yet parented. Without entering the tree
        // there's nothing protecting the peer, and if the peer gets released then all its event handlers are
        // lost. Set an expected reference now to bridge the gap until the element is parented. If the element
        // is parented later, the existing logic around CDO::OnParentChange and Set/ClearExpectedReferenceOnPeer
        // keeps it balanced, and if it is never parented then the expected reference is released when the
        // core object is destroyed, preventing a leak.
        IFC_RETURN(nativeElement->SetExpectedReferenceOnPeer());

        SetInterface(*ppResult, nativeElement);
    }

    return S_OK;
}

_Check_return_ HRESULT DirectUI::DataTemplate::GetElementImpl(_In_ xaml::IElementFactoryGetArgs* args, _Outptr_ xaml::IUIElement** retrunValue)
{
    ctl::ComPtr<xaml::IUIElement> candidateElement;
    if (m_elements)
    {
        unsigned count = m_parents.size();
        if (count > 0)
        {
            ctl::ComPtr<xaml::IUIElement> candidateParent;

            // Prefer an element from the same owner or with no owner so that we don't incur
            // the enter/leave cost during recycling.
            ctl::ComPtr<xaml::IUIElement> parentInContext;
            IFC_RETURN(args->get_Parent(&parentInContext));

            int candidateIndex = -1;
            for (UINT i = 0; i < count; i++)
            {
                IFC_RETURN(m_parents[i].As(&candidateParent));
                if (!candidateParent || candidateParent.Get() == parentInContext.Get())
                {
                    // We either have an element whose parent matches the one who is asking,
                    // or we have one with no parent.
                    candidateIndex = i;
                    break;
                }
            }

            if (candidateIndex == -1)
            {
                // No matches, just pick the last one and use it.
                // Reparenting is better than creating a completely new tree.
                candidateIndex = count - 1;
            }

            IFC_RETURN(m_parents[candidateIndex].As(&candidateParent));
            m_parents.erase(m_parents.begin() + candidateIndex);

            auto elementPool = m_elements.Get();
            IFC_RETURN(elementPool->GetAt(candidateIndex, &candidateElement));
            IFC_RETURN(elementPool->RemoveAt(candidateIndex));

            auto parentInContextAsPanel = parentInContext.AsOrNull<xaml_controls::IPanel>();
            auto candidateParentAsPanel = candidateParent.AsOrNull<xaml_controls::IPanel>();
            if (candidateParentAsPanel && candidateParentAsPanel != parentInContextAsPanel)
            {
                // Remove candidate from its old parent
                ctl::ComPtr<wfc::IVector<xaml::UIElement*>> children;
                IFC_RETURN(candidateParentAsPanel->get_Children(&children));
                unsigned int childIndex = 0;
                BOOLEAN result;
                IFC_RETURN(children->IndexOf(candidateElement.Get(), &childIndex, &result));
                if (result)
                {
                    IFC_RETURN(children->RemoveAt(childIndex));
                }
            }
        }
    }

    if (!candidateElement)
    {
        // Nothing in the pool, create one.
        ctl::ComPtr<xaml::IDependencyObject> newElementDO;
        IFC_RETURN(LoadContentImpl(&newElementDO));
        candidateElement = newElementDO.AsOrNull<xaml::IUIElement>();
    }

    // Make sure that the UIEmement has a virtualization information associated
    auto container = static_cast<UIElement*>(candidateElement.Get());
    IFC_RETURN(container->InitVirtualizationInformation());
    auto virtualizationInformation = container->GetVirtualizationInformation();
    virtualizationInformation->SetIsRealized(true);
    virtualizationInformation->SetSelectedTemplate(this);

    *retrunValue = candidateElement.Detach();

    return S_OK;
}

_Check_return_ HRESULT DirectUI::DataTemplate::RecycleElementImpl(_In_ xaml::IElementFactoryRecycleArgs* args)
{
    if (!m_elements)
    {
        ctl::ComPtr<TrackerCollection<xaml::UIElement*>> elements;
        IFC_RETURN(ctl::make<TrackerCollection<xaml::UIElement*>>(&elements));
        SetPtrValue(m_elements, elements);
    }

    ctl::ComPtr<xaml::IUIElement> element;
    ctl::ComPtr<xaml::IUIElement> parent;
    ctl::WeakRefPtr weakParent;

    IFC_RETURN(args->get_Element(&element));
    IFC_RETURN(args->get_Parent(&parent));
    IFC_RETURN(parent.AsWeak(&weakParent));

    auto container = static_cast<UIElement*>(element.Get());
    auto virtualizationInformation = container->GetVirtualizationInformation();
    // If the instance was created using Load instead of GenerateElement, it will
    // not have an associated virtualizationInformation.
    if (virtualizationInformation)
    {
        virtualizationInformation->SetIsRealized(false);
    }

    IFC_RETURN(m_elements.Get()->Append(element.Get()));
    m_parents.emplace_back(weakParent);

    return S_OK;
}

_Check_return_ HRESULT DirectUI::DataTemplateFactory::CreateInstanceFromCallbackImpl(
    _In_ xaml::IDataTemplateElementFactory* pElementFactory,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml::IDataTemplate** ppInstance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDataTemplate> instance;

    ARG_NOTNULL(pElementFactory, "elementFactory");
    ARG_VALIDRETURNPOINTER(ppInstance);

    IFC(CreateInstance(pOuter, ppInner, &instance));
    IFC(instance.Cast<DataTemplate>()->SetElementFactory(pElementFactory));

    *ppInstance = instance.Detach();

Cleanup:
    return hr;
}
