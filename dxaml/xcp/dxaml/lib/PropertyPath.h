// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "WeakReferenceSource.h"

class CClassInfo;

namespace DirectUI
{
class BindingExpression;
class PropertyPathListener;
class PropertyPathParser;
struct IPropertyPathListenerHost;

MIDL_INTERFACE("7d9f13e8-f258-4cc4-8735-601ac9366c90")
IPropertyPathListener: IInspectable
{
};

class PropertyPathListener : 
    public IPropertyPathListener,
    public ctl::WeakReferenceSource
{
public:

    PropertyPathListener(): 
        m_pOwner(NULL)
    {}

    _Check_return_ HRESULT Initialize(
        _In_ IPropertyPathListenerHost *pOwner, 
        _In_ PropertyPathParser *pPropertyPathParser, 
        _In_ bool fListenToChanges, 
        _In_ bool fUseWeakReferenceForSource);
    using ctl::WeakReferenceSource::Initialize;
    
    _Check_return_ HRESULT SetSource(_In_ IInspectable *pSource);

    _Check_return_ HRESULT GetValue(_Out_ IInspectable **pValue);
    _Check_return_ HRESULT SetValue(_In_ IInspectable *pValue);

    bool FullPathExists() const;

    _Check_return_ HRESULT GetLeafType(_Outptr_ const CClassInfo **ppLeafType);

    // Tracing support
    _Check_return_ HRESULT GetTraceString(_Outptr_ const WCHAR **pszTraceString);

    PropertyPathStep* DebugGetFirstStep();

    void ClearOwner();

protected:
    HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Out_ void** ppObject) override
    {
        if (InlineIsEqualGUID(riid, __uuidof(IPropertyPathListener)))
        {
            *ppObject = static_cast<IPropertyPathListener*>(this);
        }
        else 
        {
            RRETURN(ctl::WeakReferenceSource::QueryInterfaceImpl(riid, ppObject));
        }
        
        AddRefOuter();
        RRETURN(S_OK);
    }

private:

    _Check_return_ HRESULT ConnectPathStep(_In_ PropertyPathStep *pStep, _In_ IInspectable *pSource);

    _Check_return_ HRESULT PropertyPathStepChanged(_In_ PropertyPathStep *pStep);

    void AppendStep(_In_ PropertyPathStep* const pStep);

private:

    IPropertyPathListenerHost *m_pOwner;
    TrackerPtr<PropertyPathStep> m_tpFirst;
    TrackerPtr<PropertyPathStep> m_tpLast;

    friend class PropertyPathStep;
};

}
