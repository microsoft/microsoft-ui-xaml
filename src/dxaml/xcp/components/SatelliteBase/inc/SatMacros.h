// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DebugWriter.h"

#define ROERROR_LOOKUP(hr, msgResourceKey)                                                              \
{                                                                                                       \
    wrl_wrappers::HString tmpErrorMsg;                                                                             \
    IFC(Private::FindStringResource(                                                 \
        msgResourceKey,                                                                                 \
        tmpErrorMsg.GetAddressOf()));                                                         \
    RoOriginateError(hr, tmpErrorMsg.Get());                                                            \
    IFC(hr);                                                                                            \
}

// ARG failure marcos, used in generated code to validate argument parameters.
#define ARG_EXPECT(expr, parameterName) { if (!(expr)) { wf::Diagnostics::OriginateError(E_INVALIDARG, SZ_COUNT(parameterName), L##parameterName); IFC(E_INVALIDARG); } }
#define ARG_NOTNULL(parameter, parameterName) ARG_EXPECT(parameter, parameterName)

#define ARG_EXPECT_RETURN(expr, parameterName) { if (!(expr)) { wf::Diagnostics::OriginateError(E_INVALIDARG, SZ_COUNT(parameterName), L##parameterName); IFC_RETURN(E_INVALIDARG); } }
#define ARG_NOTNULL_RETURN(parameter, parameterName) ARG_EXPECT_RETURN(parameter, parameterName)
// Force AV for NULL return parameter pointers. This is recommended
// behavior from the WinRT API guidelines.
#define ARG_VALIDRETURNPOINTER(parameter) { *parameter = *parameter; }

// Trace debug macros. Minimize the use of these. Consider using ETW tracing instead
// for permanent debug support.
#ifdef DBG
    #define ASYNCTRACE(...) TRACE(TraceAlways, __VA_ARGS__)
#else
    #define ASYNCTRACE(...)
#endif

// Pretty much identical to the WRL's InspectableClass macro
// except AddRef/Release call the base class instead of RuntimeClassT directly and GetIids is not implemented
#define WuxpInspectableClassNoGetIids(runtimeClassName, trustLevel) \
    public: \
        static const wchar_t* STDMETHODCALLTYPE InternalGetRuntimeClassName() noexcept \
        { \
            static_assert((RuntimeClassT::ClassFlags::value & ::Microsoft::WRL::WinRtClassicComMix) == ::Microsoft::WRL::WinRt || \
                (RuntimeClassT::ClassFlags::value & ::Microsoft::WRL::WinRtClassicComMix) == ::Microsoft::WRL::WinRtClassicComMix, \
                    "'InspectableClass' macro must not be used with ClassicCom clasess."); \
            static_assert(__is_base_of(::Microsoft::WRL::Details::RuntimeClassBase, RuntimeClassT), "'InspectableClass' macro can only be used with ::Windows::WRL::RuntimeClass types"); \
            static_assert(!__is_base_of(IActivationFactory, RuntimeClassT), "Incorrect usage of IActivationFactory interface. Make sure that your RuntimeClass doesn't implement IActivationFactory interface use ::Windows::WRL::ActivationFactory instead or 'InspectableClass' macro is not used on ::Windows::WRL::ActivationFactory"); \
            return runtimeClassName; \
        } \
        static TrustLevel STDMETHODCALLTYPE InternalGetTrustLevel() noexcept \
        { \
            return trustLevel; \
        } \
        STDMETHOD(GetRuntimeClassName)(_Out_ HSTRING* runtimeName) \
        { \
            *runtimeName = nullptr; \
            HRESULT hr = S_OK; \
            const wchar_t *name = InternalGetRuntimeClassName(); \
            if (name != nullptr) \
            { \
                hr = ::WindowsCreateString(name, static_cast<UINT32>(::wcslen(name)), runtimeName); \
            } \
            return hr; \
        } \
        STDMETHOD(GetTrustLevel)(_Out_ TrustLevel* trustLvl) \
        { \
            *trustLvl = trustLevel; \
            return S_OK; \
        } \
        STDMETHOD(QueryInterface)(REFIID riid, _Outptr_result_nullonfailure_ void **ppvObject) \
        { \
            return ReferenceTrackerRuntimeClass::QueryInterface(riid, ppvObject); \
        } \
        STDMETHOD_(ULONG, Release)() \
        { \
            return ReferenceTrackerRuntimeClass::Release(); \
        } \
        STDMETHOD_(ULONG, AddRef)() \
        { \
            return ReferenceTrackerRuntimeClass::AddRef(); \
        } \
    private:

// Pretty much identical to the WRL's InspectableClass macro
// except GetIids/AddRef/Release call the base class instead of RuntimeClassT directly.
#define WuxpInspectableClass(runtimeClassName, trustLevel) \
    WuxpInspectableClassNoGetIids(runtimeClassName, trustLevel) \
    public:\
        STDMETHOD(GetIids)(_Out_ ULONG *iidCount, \
            _When_(*iidCount == 0, _At_(*iids, _Post_null_)) \
            _When_(*iidCount > 0, _At_(*iids, _Post_notnull_)) \
            _Outptr_result_buffer_maybenull_(*iidCount) _Result_nullonfailure_ IID **iids) \
        { \
            return __super::GetIids(iidCount, iids); \
        } \
    private: