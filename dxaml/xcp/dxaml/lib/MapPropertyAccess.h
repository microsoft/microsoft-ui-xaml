// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines the class to access to properties stored in an IMap
//      implementation

#pragma once

#include "PropertyAccess.h"

namespace DirectUI
{
    class MapPropertyAccess: public PropertyAccess
    {
    public:

        MapPropertyAccess():
                   m_pOwner(NULL)
        { }

        ~MapPropertyAccess() override;

        _Check_return_ HRESULT GetValue(_COM_Outptr_result_maybenull_ IInspectable **ppValue) override;
        _Check_return_ HRESULT SetValue(_In_ IInspectable *pValue) override;
        _Check_return_ HRESULT GetType(_Outptr_ const CClassInfo **ppType) override;
        bool IsConnected() override;
        _Check_return_ HRESULT SetSource(_In_opt_ IInspectable *pSource, _In_ BOOLEAN fListenToChanges) override;
        _Check_return_ HRESULT GetSource(_Outptr_ IInspectable **ppSource) override;
        _Check_return_ HRESULT GetSourceType(_Outptr_ const CClassInfo **ppType) override;
        _Check_return_ HRESULT TryReconnect(_In_ IInspectable* pSource, _In_ BOOLEAN fListenToChanges, _Inout_ BOOLEAN& bConnected, _Inout_ const CClassInfo*& pResolvedType) override
        {
            bConnected = FALSE;
            RRETURN(S_OK);
        }
        _Check_return_ HRESULT DisconnectEventHandlers() override;

    public:

        static _Check_return_ HRESULT CreateInstance(
            _In_ IPropertyAccessHost *pOwner,
            _In_ wfc::IMap<HSTRING, IInspectable *> *pSource,
            _In_ bool fListenToChanges,
            _Outptr_ PropertyAccess **ppPropertyAccess);

    private:

        void Initialize(
            _In_ IPropertyAccessHost* const pOwner,
            _In_ wfc::IMap<HSTRING, IInspectable *>* const pSource);

    protected:

        using PropertyAccess::Initialize;

    private:

        _Check_return_ HRESULT MapKeyChanged();

        _Check_return_ HRESULT AddKeyChangedEventHandler();

        // This method is safe to be called from the destructor
        _Check_return_ HRESULT SafeRemoveKeyChangedEventHandler();

    private:

        _Check_return_ HRESULT OnMapChanged(_In_ wfc::IMapChangedEventArgs<HSTRING> *pArgs)
        {
            HRESULT hr = S_OK;
            wrl_wrappers::HString strKey;

            IFC(pArgs->get_Key(strKey.GetAddressOf()));

            if (strKey == m_strProperty)
            {
                IFC(MapKeyChanged());
            }

        Cleanup:
            RRETURN(hr);
        }

    private:

        IPropertyAccessHost *m_pOwner;
        TrackerPtr<wfc::IMap<HSTRING, IInspectable *>> m_tpSource;
        wrl_wrappers::HString m_strProperty;
        ctl::EventPtr<MapChangedEventCallback> m_epMapChangedEventHandler;
    };

}
