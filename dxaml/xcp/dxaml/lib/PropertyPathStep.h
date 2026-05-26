// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
class BindingExpression;
class PropertyPathListener;

class PropertyPathStep : public ctl::WeakReferenceSource
{

public:

    _Check_return_ HRESULT Initialize(PropertyPathListener *pListener);
    using ctl::WeakReferenceSource::Initialize;

    ~PropertyPathStep() override;

    void SetNext(_In_ PropertyPathStep* const pNextStep)
    {
        SetPtrValue(m_tpNext, pNextStep);
    }

    PropertyPathStep* GetNextStep() const
    {
        return m_tpNext.Get(); 
    }

    virtual _Check_return_ HRESULT ReConnect(_In_ IInspectable *pSource) = 0;

    virtual _Check_return_ HRESULT GetValue(_Outptr_ IInspectable **ppValue) = 0;
    virtual _Check_return_ HRESULT SetValue(_In_  IInspectable *pValue) = 0;
    virtual _Check_return_ HRESULT GetSourceType(_Outptr_ const CClassInfo **ppType) = 0;
    virtual bool IsConnected() = 0;

    virtual _Check_return_ HRESULT GetType(_Outptr_ const CClassInfo **ppType) = 0;

    virtual WCHAR *DebugGetPropertyName() { return nullptr; }

protected:

    virtual void Disconnect();
    virtual void DisconnectCurrentItem() { }
    virtual _Check_return_ HRESULT CollectionViewCurrentChanged() { return E_NOTIMPL; }

    
    _Check_return_ HRESULT AddCurrentChangedEventHandler();

    // This method is safe to be called from the destructor
    _Check_return_ HRESULT SafeRemoveCurrentChangedEventHandler();

    _Check_return_ HRESULT RaiseSourceChanged();

protected:

    TrackerPtr<PropertyPathStep> m_tpNext;
    ctl::WeakRefPtr m_spListener; 
    
    ctl::EventPtr<CurrentChangedEventCallback> m_epCurrentChangedHandler;
    TrackerPtr<xaml_data::ICollectionView> m_tpSourceAsCV;
};

}
