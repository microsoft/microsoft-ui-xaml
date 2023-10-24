// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <PalResourceManager.h>
#include <Microsoft.Windows.ApplicationModel.Resources.h>

class CCoreServices;
class CommonResourceProvider;

class ModernResourceProvider final : public IPALResourceProvider, private CReferenceCount
{
public:
    ~ModernResourceProvider();

    static _Check_return_ HRESULT TryCreate(
        _In_ CCoreServices *pCore,
        _Outptr_result_maybenull_ ModernResourceProvider** ppResourceProvider);

    FORWARD_ADDREF_RELEASE(CReferenceCount);

    _Check_return_ HRESULT TryGetLocalResource(
        _In_ IPALUri* pResourceUri,
        _Outptr_result_maybenull_ IPALResource** ppResource
        ) override;

    _Check_return_ HRESULT GetString(
        _In_ const xstring_ptr_view& key,
        _Out_ xstring_ptr* pstrString
        ) override
    {
        RRETURN(E_NOTIMPL);
    }

    _Check_return_ HRESULT GetPropertyBag(
        _In_ const IPALUri *pUri,
        _Out_ PropertyBag& propertyBag
        ) noexcept override;

    _Check_return_ HRESULT SetScaleFactor(
        XUINT32 ulScaleFactor
        ) override;

    _Check_return_ HRESULT NotifyThemeChanged(
        ) override;

    _Check_return_ HRESULT SetProcessMUILanguages(
        ) override;

    void DetachEvents() override { /* no events */ }

private:
    ModernResourceProvider(_In_ CCoreServices *pCore);

    static _Check_return_ HRESULT Create(
        _In_ CCoreServices *pCore,
        _Outptr_ ModernResourceProvider** ppResourceProvider);

    static _Check_return_ HRESULT PrepareModernResourceName(
        _In_ XUINT32 cRelativeUri,
        _In_reads_(cRelativeUri) const WCHAR *pRelativeUri,
        _Out_ xstring_ptr* pstrResourceName
        );

    static _Check_return_ HRESULT ProbeForAppResourcesPri(_Out_ xstring_ptr* appResourcePriPath);
    static _Check_return_ HRESULT ProbeForFrameworkPackageResourcesPri(_Out_ xstring_ptr* frameworkPackageResourcePriPath, _Out_ bool& isUsingCbsPackage);

    _Check_return_ HRESULT InitializeContext();

    _Check_return_ HRESULT EnsureFallbackResourceProvider();

    _Check_return_ HRESULT TryGetResourceCandidate(
        _In_ IPALUri* pMSResourceUri,
        _Outptr_result_maybenull_ wrl::ComPtr<mwar::IResourceCandidate>& resourceCandidate);

    _Check_return_ HRESULT UpdateContrastQualifier();
    _Check_return_ HRESULT UpdateDeviceFamilyQualifier();
    _Check_return_ HRESULT UpdateHomeRegionQualifier();
    _Check_return_ HRESULT UpdateLanguageAndLayoutDirectionQualifiers();
    _Check_return_ HRESULT UpdateScaleQualifier(XUINT32 ulScaleFactor);
    _Check_return_ HRESULT UpdateThemeQualifier();

    _Check_return_ HRESULT UpdateQualifierValue(HSTRING name, HSTRING value);

private:
    CCoreServices* m_pCore;

    // The MRT Resource Manager used for loading the app's resources.pri
    wrl::ComPtr<mwar::IResourceManager> m_appResourceManager;
    wrl::ComPtr<mwar::IResourceContext> m_appResourceContext;
    // The MRT Resource Manager used for loading the Project Reunion framework package's resources.pri
    wrl::ComPtr<mwar::IResourceManager> m_frameworkPackageResourceManager;
    wrl::ComPtr<mwar::IResourceContext> m_frameworkPackageResourceContext;

    xref_ptr<CommonResourceProvider> m_pFallbackResourceProvider;

    // Note: any usage of m_shouldUseSystemLanguage should be contained behind 46751006
    // If running as an OS component and using the CBS package,
    // we should localize using the Windows display language
    // instead of the user's preferred language list
    bool m_shouldUseSystemLanguage = false;
};
