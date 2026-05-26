// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Windows.Media.Casting.h>
#include <mfmediaengine.h>
#include <castinginterop.h>
#include <intsafe.h>
#include <NamespaceAliases.h>

class CPropVariant : public PROPVARIANT
{
public:
    CPropVariant(PCWSTR value)
    {
        vt = VT_LPWSTR;
        pwszVal = const_cast<PWSTR>(value);
    }

    CPropVariant(UINT32 value)
    {
        vt = VT_UI4;
        ulVal = value;
    }

    CPropVariant(bool value)
    {
        vt = VT_BOOL;
        boolVal = value ? VARIANT_TRUE : VARIANT_FALSE;
    }
};

template<typename Element>
class CXamlCastingSource :
    public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::WinRtClassicComMix>,
        wm::Casting::ICastingSource,
        Microsoft::WRL::CloakedIid<ICastingSourceInfo>,
        Microsoft::WRL::CloakedIid<ICastingController>,
        Microsoft::WRL::CloakedIid<IMFMediaEngineNotify>>
{
    InspectableClass(L"", BaseTrust);
public:
    CXamlCastingSource() : m_protectedContent(false), m_supportedCastingTypes(wm::Casting::CastingPlaybackTypes_None)
    {
    }

    template<typename Element>
    HRESULT SetElement(_In_ Element *pElement)
    {
        // We do not take a ref on pElement here to avoid circular dependencies.  The owner
        // of this class must free it before pElement is destroyed.
        m_pElementNoRef = pElement;

        // Since we're setting a new element, we should reset all our internal properties that depend
        // on the element.
        m_protectedContent = false;
        m_supportedCastingTypes = wm::Casting::CastingPlaybackTypes_None;

        return S_OK;
    }

    // ICastingSource
    IFACEMETHODIMP get_PreferredSourceUri(_Outptr_result_maybenull_ wf::IUriRuntimeClass **ppValue) override
    {
        IFCPTR_RETURN(ppValue);
        *ppValue = nullptr;

        if (m_spPreferredSourceUri)
        {
            IFC_RETURN(m_spPreferredSourceUri.CopyTo(IID_PPV_ARGS(ppValue)));
        }

        return S_OK;
    }

    IFACEMETHODIMP put_PreferredSourceUri(_In_opt_ wf::IUriRuntimeClass *pValue) override
    {
        m_spPreferredSourceUri = pValue;
        return S_OK;
    }

    // ICastingSourceInfo
    IFACEMETHODIMP GetController(_COM_Outptr_ ICastingController **ppValue) override
    {
        IFCPTR_RETURN(ppValue);
        IFC_RETURN(QueryInterface(IID_PPV_ARGS(ppValue)));

        return S_OK;
    }

    IFACEMETHODIMP GetProperties(_COM_Outptr_ INamedPropertyStore **ppValue) override
    {
        IFCPTR_RETURN(ppValue);
        *ppValue = nullptr;

        Microsoft::WRL::ComPtr<INamedPropertyStore> _pStore;
        IFC_RETURN(PSCreateMemoryPropertyStore(IID_PPV_ARGS(_pStore.GetAddressOf())));

        // Set the Preferred Source Uri property if once exists
        if (m_spPreferredSourceUri)
        {
            wrl_wrappers::HString schemeName;
            IFC_RETURN(m_spPreferredSourceUri->get_SchemeName(schemeName.GetAddressOf()));
            IFC_RETURN(_pStore->SetNamedValue(CastingSourceInfo_Property_PreferredSourceUriScheme, CPropVariant(schemeName.GetRawBuffer(nullptr))));
        }

        // Set the CastingTypes property
        IFC_RETURN(_pStore->SetNamedValue(CastingSourceInfo_Property_CastingTypes, CPropVariant(static_cast<UINT32>(m_supportedCastingTypes))));

        // Set the ProtectedMedia property
        IFC_RETURN(_pStore->SetNamedValue(CastingSourceInfo_Property_ProtectedMedia, CPropVariant(m_protectedContent)));

        *ppValue = _pStore.Detach();
        return S_OK;
    }

    // ICastingController
    IFACEMETHODIMP Advise(_In_ ICastingEventHandler *pEventHandler, _Out_ DWORD *cookie) override
    {
        size_t index = 0;
        bool foundEmptyNode = false;
        Microsoft::WRL::ComPtr<ICastingEventHandler> spEventHandler = pEventHandler;
        auto lock = m_eventHandlersLock.Lock();

        IFCPTR_RETURN(pEventHandler);
        IFCPTR_RETURN(cookie);
        *cookie = 0;

        for (size_t i = 0; !foundEmptyNode && (i < m_eventHandlers.size()); i++)
        {
            if (m_eventHandlers[i] == nullptr)
            {
                index = i;
                foundEmptyNode = true;
            }
        }

        if (foundEmptyNode)
        {
            m_eventHandlers[index] = std::move(spEventHandler);
        }
        else
        {
            m_eventHandlers.push_back(std::move(spEventHandler));
            index = m_eventHandlers.size() - 1;
        }

        // Clients might initialize their cookies to 0 to indicate that it has not been used to advice (or has been unadvised),
        // so make sure we don't return a 0 cookie value here by adding 1 to the index.
        IFC_RETURN(SizeTAdd(index, 1, &index));
        IFC_RETURN(SizeTToDWord(index, cookie));

        return S_OK;
    }

    IFACEMETHODIMP UnAdvise(_In_ DWORD cookie) override
    {
        size_t index;
        auto lock = m_eventHandlersLock.Lock();

        // Subtract 1 from the cookie to get the index into the array (see Advise method above)
        IFC_RETURN(SizeTSub(cookie, 1, &index));

        IFCEXPECTRC_RETURN(index < m_eventHandlers.size(), E_INVALIDARG);

        m_eventHandlers[index] = nullptr;

        return S_OK;
    }

    IFACEMETHODIMP Initialize(_In_ IUnknown *pCastingEngine, _In_ IUnknown *pCastingSource) override
    {
        Microsoft::WRL::ComPtr<IUnknown> spUnk;
        Microsoft::WRL::ComPtr<IInspectable> spInspCastingEngine;
        Microsoft::WRL::ComPtr<wm::Casting::ICastingSource> spCastingSource;

        IFCPTRRC_RETURN(m_pElementNoRef, HRESULT_FROM_WIN32(ERROR_INVALID_STATE));

        IFC_RETURN(QueryInterface(IID_PPV_ARGS(&spUnk)));
        IFC_RETURN(pCastingEngine->QueryInterface(IID_PPV_ARGS(&spInspCastingEngine)));
        IFC_RETURN(pCastingSource->QueryInterface(IID_PPV_ARGS(&spCastingSource)));

        IFC_RETURN(m_pElementNoRef->InitializeCasting(spInspCastingEngine.Get(), spUnk.Get(), spCastingSource.Get()));

        return S_OK;
    }

    IFACEMETHODIMP Connect() override
    {
        IFCPTRRC_RETURN(m_pElementNoRef, HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
        IFC_RETURN(m_pElementNoRef->Connect(nullptr, false /*fCreateSharingEngine*/));

        return S_OK;
    }

    IFACEMETHODIMP Disconnect() override
    {
        IFCPTRRC_RETURN(m_pElementNoRef, HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
        IFC_RETURN(m_pElementNoRef->Disconnect());

        return S_OK;
    }

    // Property setters
    HRESULT put_IsProtectedContent(_In_ boolean value)
    {
        m_protectedContent = !!value;
        return S_OK;
    }

    HRESULT put_SupportedCastingTypes(_In_ wm::Casting::CastingPlaybackTypes value)
    {
        m_supportedCastingTypes = value;
        return S_OK;
    }

    // IMFMediaEngineNotify
    IFACEMETHODIMP EventNotify(_In_ DWORD event, _In_ DWORD_PTR param1, _In_ DWORD param2) override
    {
        switch (event)
        {
        case MF_MEDIA_ENGINE_EVENT_PLAYING:
            IFC_RETURN(FireStateChange(CASTING_CONNECTION_STATE_RENDERING));
            break;

        case MF_SHARING_ENGINE_EVENT_DISCONNECT:
            IFC_RETURN(FireStateChange(CASTING_CONNECTION_STATE_DISCONNECTED));
            break;

        case MF_SHARING_ENGINE_EVENT_STOPPED:
        case MF_MEDIA_ENGINE_EVENT_PAUSE:
        case MF_MEDIA_ENGINE_EVENT_EMPTIED:
        case MF_MEDIA_ENGINE_EVENT_ENDED:
            IFC_RETURN(FireStateChange(CASTING_CONNECTION_STATE_CONNECTED));
            break;
        }

        return S_OK;
    }

private:
    void GetEventHandlers(_Inout_ std::vector<Microsoft::WRL::ComPtr<ICastingEventHandler>> &eventHandlers)
    {
        auto lock = m_eventHandlersLock.Lock();
        for (const Microsoft::WRL::ComPtr<ICastingEventHandler>& handler : m_eventHandlers)
        {
            if (handler != nullptr)
            {
                // Ignore errors here - best effort
                eventHandlers.push_back(handler);
            }
        }
    }

    HRESULT FireStateChange(_In_ CASTING_CONNECTION_STATE state)
    {
        // Make sure we don't hold the m_eventHandlersLock while firing the event in case there is reentrancy
        std::vector<Microsoft::WRL::ComPtr<ICastingEventHandler>> eventHandlersTemp;
        GetEventHandlers(eventHandlersTemp);

        for (Microsoft::WRL::ComPtr<ICastingEventHandler>& handler : eventHandlersTemp)
        {
            // Ignore errors here - best effort. Keep going until all clients are notified.
            handler->OnStateChanged(state);
        }

        return S_OK;
    }

    HRESULT FireError(_In_ CASTING_CONNECTION_ERROR_STATUS error)
    {
        // Make sure we don't hold the m_eventHandlersLock while firing the event in case there is reentrancy
        std::vector<Microsoft::WRL::ComPtr<ICastingEventHandler>> eventHandlersTemp;
        GetEventHandlers(eventHandlersTemp);

        for (Microsoft::WRL::ComPtr<ICastingEventHandler>& handler : eventHandlersTemp)
        {
            // Ignore errors here - best effort. Keep going until all clients are notified.
            handler->OnError(error, nullptr);
        }

        return S_OK;
    }
    Element*                                                        m_pElementNoRef{};
    Microsoft::WRL::ComPtr<wf::IUriRuntimeClass>                    m_spPreferredSourceUri;
    std::vector<Microsoft::WRL::ComPtr<ICastingEventHandler>>       m_eventHandlers;
    wrl_wrappers::CriticalSection                                   m_eventHandlersLock;
    bool                                                            m_protectedContent;
    wm::Casting::CastingPlaybackTypes                               m_supportedCastingTypes;
};
