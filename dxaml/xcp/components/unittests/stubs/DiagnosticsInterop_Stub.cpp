// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "diagnosticsInterop\inc\ResourceGraph.h"
#include "diagnosticsInterop\inc\PropertyChainIterator.h"
#include "diagnosticsInterop\inc\PropertyChainEvaluator.h"
#include "DiagnosticsInterop.h"

namespace Diagnostics
{
    void ResourceGraph::RegisterResourceDependency(
        _In_ CDependencyObject* ,
        KnownPropertyIndex ,
        _In_ CResourceDictionary*,
        const xstring_ptr&,
        ResourceType)
    {
    }

    void ResourceGraph::RegisterControlTemplateDependency(
        _In_ CControlTemplate* ,
        _In_ CResourceDictionary* ,
        const xstring_ptr& )
    {
    }

    CStyle* ResourceGraph::GetOwningStyle(_In_ CSetterBaseCollection *)
    {
        return nullptr;
    }

    _Ret_maybenull_ CustomWriterRuntimeContext* ResourceGraph::GetCachedRuntimeContext(_In_ CStyle*)
    {
        return nullptr;
    }

    CVisualState* DiagnosticsInterop::TryFindVisualState(_In_ const CDependencyObject*)
    {
        return nullptr;
    }

    xref_ptr<CDependencyObject> DiagnosticsInterop::ConvertToCore(_In_ IInspectable*, _Out_opt_ bool* wasPeerPegged)
    {
        if (wasPeerPegged != nullptr)
        {
            *wasPeerPegged = false;
        }
        return nullptr;
    }

    HRESULT DiagnosticsInterop::GetApplicationStatic(_Outptr_ xaml::IApplication**)
    {
        return E_NOTIMPL;
    }

    PropertyChainIterator::PropertyChainIterator()
    {
    }

    PropertyChainIterator& PropertyChainIterator::operator++()
    {
        return *this;
    }

    bool PropertyChainIterator::operator!=(_In_ const PropertyChainIterator& rhs) const
    {   
        return true;
    }

    PropertyChainData& PropertyChainIterator::operator*()
    {
        return m_data;
    }

    PropertyChainEvaluator::PropertyChainEvaluator(_In_ CDependencyObject *)
    {
    }

    PropertyChainIterator PropertyChainEvaluator::begin()
    {
        return PropertyChainIterator();
    }

    PropertyChainIterator PropertyChainEvaluator::end()
    {
        return PropertyChainIterator();
    }

    _Check_return_ HRESULT PropertyChainEvaluator::Evaluate(_In_ const PropertyChainData&, _Out_ EvaluatedValue& evaluatedValue)
    {
        ASSERT(FALSE);
        evaluatedValue = EvaluatedValue(nullptr, wil::unique_propertychainvalue());
        return S_OK;
    }

    _Check_return_ HRESULT PropertyChainEvaluator::GetEffectiveValue(_In_ const xref_ptr<CDependencyObject> &, _In_ const CPropertyBase *, _Out_ ctl::ComPtr<IInspectable>& value)
    {
        ASSERT(FALSE);
        value = nullptr;
        return S_OK;
    }

    KnownPropertyIndex ResourceGraph::GetTargetProperty(_In_ CMarkupExtensionBase*)
    {
        return KnownPropertyIndex::UnknownType_UnknownProperty;
    }

    xref_ptr<CDependencyObject> ResourceGraph::GetTargetObject(_In_ CMarkupExtensionBase*)
    {
        return nullptr;
    }

    void ResourceGraph::AddStyleContext(_In_ CStyle* pStyle, _In_ CDependencyObject* owner, bool isImplicit)
    {

    }

    _Check_return_ HRESULT ResourceGraph::TryRefreshImplicitStyle(_In_ CStyle* pStyle, _Out_ bool* pWasImplicit)
    {
        return E_NOTIMPL;
    }

    CDependencyObject* GetParentForElementStateChanged(_In_ CDependencyObject*)
    {
        return nullptr;
    }
}
