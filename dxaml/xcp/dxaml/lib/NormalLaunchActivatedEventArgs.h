// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    class NormalLaunchActivatedEventArgs : 
        public ctl::SupportErrorInfo,
        public waa::ILaunchActivatedEventArgs,
        public waa::IActivatedEventArgs
    {
    public:

        static _Check_return_ HRESULT Create(_Outptr_ NormalLaunchActivatedEventArgs** ppOut)
        {
            return ctl::ComObject<NormalLaunchActivatedEventArgs>::CreateInstance(ppOut);
        }

        // IActivatedEventArgs

        /* [propget] */ HRESULT STDMETHODCALLTYPE get_Kind( 
            /* [out][retval] */ __RPC__out waa::ActivationKind *value) override
        {
            *value = waa::ActivationKind_Launch;
            return S_OK;
        }
                    
        /* [propget] */ HRESULT STDMETHODCALLTYPE get_PreviousExecutionState( 
            /* [out][retval] */ __RPC__out waa::ApplicationExecutionState *value) override
        {
            *value = waa::ApplicationExecutionState_NotRunning;
            return S_OK;
        }
                    
        /* [propget] */ HRESULT STDMETHODCALLTYPE get_SplashScreen( 
            /* [out][retval] */ __RPC__deref_out_opt waa::ISplashScreen **value) override

        {
            *value = NULL;
            return S_OK;
        }        

        // ILaunchActivatedEventArgs

        /* [propget] */ HRESULT STDMETHODCALLTYPE get_Arguments( 
            /* [out][retval] */ __RPC__deref_out_opt HSTRING *value) override
        {
            *value = NULL;
            return S_OK;
        }        
                    
        /* [propget] */ HRESULT STDMETHODCALLTYPE get_TileId( 
            /* [out][retval] */ __RPC__deref_out_opt HSTRING *value) override
        {
            *value = NULL;
            return S_OK;
        }

        INSPECTABLE_CLASS(L"");

        BEGIN_INTERFACE_MAP(NormalLaunchActivatedEventArgs, SupportErrorInfo)
            INTERFACE_ENTRY(NormalLaunchActivatedEventArgs, waa::ILaunchActivatedEventArgs)
            INTERFACE_ENTRY(NormalLaunchActivatedEventArgs, waa::IActivatedEventArgs)
        END_INTERFACE_MAP(NormalLaunchActivatedEventArgs, SupportErrorInfo)
        
    protected:

        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (__uuidof(waa::ILaunchActivatedEventArgs) == iid)
            {
                *ppObject = static_cast<waa::ILaunchActivatedEventArgs*>(this);
            }
            else if (__uuidof(waa::IActivatedEventArgs) == iid)
            {
                *ppObject = static_cast<waa::IActivatedEventArgs*>(this);
            }
            else
            {
                return SupportErrorInfo::QueryInterfaceImpl(iid, ppObject);
            }
        
            AddRefOuter();
            RRETURN(S_OK);
        }
     };
}
