// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <FontHelperPrivate.h>

namespace Private { namespace Infrastructure {

    class FontHelper : public Microsoft::WRL::RuntimeClass<test_infra::IFontHelper>
                        , IFontHelperNative
    {
        public:
        static_assert((RuntimeClass::ClassFlags::value & ::Microsoft::WRL::WinRtClassicComMix) == ::Microsoft::WRL::WinRt || \
            (RuntimeClass::ClassFlags::value & ::Microsoft::WRL::WinRtClassicComMix) == ::Microsoft::WRL::WinRtClassicComMix, \
                "'InspectableClass' macro must not be used with ClassicCom clasess."); \
        static __forceinline const wchar_t* InternalGetRuntimeClassName() \
        { \
            static_assert(__is_base_of(::Microsoft::WRL::Details::RuntimeClassBase, RuntimeClass), "'InspectableClass'macro can only be used with ::Windows::WRL::RuntimeClass types"); \
            return RuntimeClass_Private_Infrastructure_FontHelper; \
        } \
        static __forceinline TrustLevel InternalGetTrustLevel() \
        { \
            return TrustLevel::BaseTrust; \
        } \
        STDMETHOD(GetRuntimeClassName)(_Out_ HSTRING* runtimeName) \
        { \
            *runtimeName = nullptr; \
            HRESULT hr = S_OK; \
            const wchar_t *name = InternalGetRuntimeClassName(); \
            if (name != nullptr) \
            { \
                hr = ::WindowsCreateString(name, (UINT32)wcslen(name), runtimeName); \
            } \
            return hr; \
        } \
        STDMETHOD(GetTrustLevel)(_Out_ TrustLevel* trustLvl) \
        { \
            *trustLvl = TrustLevel::BaseTrust; \
            return S_OK; \
        } \
        STDMETHOD(GetIids)(_Out_ ULONG *iidCount, _Outptr_ _Deref_post_cap_(*iidCount) IID **iids) \
        { \
            return RuntimeClass::GetIids(iidCount, iids); \
        } \
        STDMETHOD(QueryInterface)(REFIID riid, _Outptr_ void **ppvObject) \
        { \
            *ppvObject = nullptr;
            if (InlineIsEqualGUID(riid, __uuidof(IFontHelperNative)))
            {
                *ppvObject = static_cast<IFontHelperNative *>(this);
            }
            else if (InlineIsEqualGUID(riid, __uuidof(test_infra::IFontHelper)))
            {
                *ppvObject = static_cast<test_infra::IFontHelper *>(this);
            }
            else
            {
                 LogThrow_LastErrorIf(true);
            }
            
            reinterpret_cast<IUnknown*>(this)->AddRef();
            return S_OK;
        } \
        STDMETHOD_(ULONG, Release)() \
        { \
            return RuntimeClass::Release(); \
        } \
        STDMETHOD_(ULONG, AddRef)() \
        { \
            return RuntimeClass::AddRef(); \
        } \

        public:
            IFACEMETHOD(RuntimeClassInitialize)();
            HRESULT GetCustomSystemFontCollection(_Out_ IUnknown** fontCollection) override;
            HRESULT SetSystemFontCollectionOverride(_In_ IUnknown* fontCollection) override;
            IFACEMETHOD(ClearSystemFontCollectionOverride)();
            IFACEMETHOD(get_IsCustomSystemFontCollectionInUse)(_Out_ BOOLEAN* isCustomSystemFontCollectionInUse) override;
            IFACEMETHOD(UpdateFontScale)(_In_ float scale) override;
            static void IsCustomSystemFontCollectionInUseStatic(_Out_ BOOLEAN* isCustomSystemFontCollectionInUse);
            static float GetFontScaleStatic() {return s_fontScale;}
            HRESULT CreateCustomFontCollection(
                wchar_t const* fontFileNames[], unsigned int cFontFileNames,
                wchar_t const* fontNames[], unsigned int cFontNames,
                _Out_ IUnknown** ppFontCollection) override;

        private:
            wrl::ComPtr<IUnknown> m_customSystemFontCollection;
            static BOOLEAN s_isCustomSystemFontCollectionInUse;
            static float s_fontScale;
    };

} }

