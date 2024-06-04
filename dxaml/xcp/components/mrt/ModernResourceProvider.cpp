// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <appmodel.h>
#include <Pathcch.h>
#include <shlwapi.h>
#include <Windows.Applicationmodel.h>
#include <Windows.Globalization.h>
#include <Windows.Storage.h>
#include <wininet.h>

#include <BasePALResource.h>
#include <corep.h>
#include <CommonResourceProvider.h>
#include <FxCallbacks.h>
#include <MsResourceHelpers.h>
#include <theming\inc\FrameworkTheming.h>
#include <theming\inc\Theme.h>
#include <UriXStringGetters.h>

#include "ModernResourceProvider.h"
#include "MRTKnownQualifierNames.h"
#include "MRTResource.h"

constexpr wchar_t c_resourcesPriName[] = L"resources.pri";
constexpr wchar_t c_frameworkPackageNamePrefix[] = L"Microsoft.WindowsAppRuntime";
constexpr int c_frameworkPackageNamePrefixLength = ARRAY_SIZE(c_frameworkPackageNamePrefix) - 1;
constexpr wchar_t c_cbsPackageNamePrefix[] = L"Microsoft.WindowsAppRuntime.CBS";
constexpr int c_cbsPackageNamePrefixLength = ARRAY_SIZE(c_cbsPackageNamePrefix) - 1;
constexpr wchar_t c_winuiComponentName[] = L"Microsoft.UI.Xaml/";

// NTSTATUS code copied from ntstatus.h
// ntstatus.h and windows.h do not play nicely together; including them both
// results in a spew of macro redefinition warnings. In theory we can avoid this
// by defining WIN32_NO_STATUS before including windows.h, but unfortunately we
// pull in windows.h through the PCH (via xcpwindows.h) which means that this escape
// hatch is not available to us unless we add ntstatus.h to the PCH which seems less
// optimal than copying the definition of the one code that we need.
#define STATUS_BUFFER_TOO_SMALL          ((NTSTATUS)0xC0000023L)

// Package constants copied from AppModel.h. These values were only defined for Vb and later;
// when passed as flags to ::GetCurrentPackageInfo() on pre-Vb, they are simply ignored.
#define PACKAGE_PROPERTY_STATIC             0x00080000
#define PACKAGE_FILTER_STATIC               PACKAGE_PROPERTY_STATIC
#define PACKAGE_PROPERTY_DYNAMIC            0x00100000
#define PACKAGE_FILTER_DYNAMIC              PACKAGE_PROPERTY_DYNAMIC

namespace
{

    // Detect if the specified package is the ProjectReunion framework package
    // by checking its name.
    bool IsProjectReunionFrameworkPackage(const wchar_t* packageFamilyName)
    {
        int nameLength{ static_cast<int>(wcslen(packageFamilyName)) };
        return (nameLength >= c_frameworkPackageNamePrefixLength) &&
               (CompareStringOrdinal(packageFamilyName,
                                     c_frameworkPackageNamePrefixLength,
                                     c_frameworkPackageNamePrefix,
                                     c_frameworkPackageNamePrefixLength,
                                     TRUE) == CSTR_EQUAL);
    }

    // Detect if the specified package is the ProjectReunion CBS package
    // by checking its name.  The CBS should be used exclusively by OS
    // components, so this doubles as a check if we're running as part
    // of an OS experience.
    bool IsProjectReunionCBSPackage(const wchar_t* packageFamilyName)
    {
        int nameLength{ static_cast<int>(wcslen(packageFamilyName)) };
        return (nameLength >= c_cbsPackageNamePrefixLength) &&
            (CompareStringOrdinal(packageFamilyName,
                c_cbsPackageNamePrefixLength,
                c_cbsPackageNamePrefix,
                c_cbsPackageNamePrefixLength,
                TRUE) == CSTR_EQUAL);
    }

    bool IsProjectReunionFrameworkPackageResource(const xstring_ptr& resourceName)
    {
        return (wcsncmp(resourceName.GetBuffer(), c_winuiComponentName, ARRAY_SIZE(c_winuiComponentName) - 1) == 0);
    }

    HRESULT GetCurrentPackageGraph(
        const std::uint32_t flags,
        std::uint32_t& packageCount,
        const PACKAGE_INFO*& packageGraph,
        std::unique_ptr<BYTE[]>& buffer)
    {
        std::uint32_t bufferLength{};
        LONG rc{ ::GetCurrentPackageInfo(flags, &bufferLength, nullptr, nullptr) };
        if ((rc == APPMODEL_ERROR_NO_PACKAGE) || (rc == ERROR_SUCCESS))
        {
            // No/empty package graph
            return S_OK;
        }
        else if (rc != ERROR_INSUFFICIENT_BUFFER)
        {
            IFC_RETURN(HRESULT_FROM_WIN32(rc));
        }

        buffer.reset(new BYTE[bufferLength]);
        IFC_RETURN(HRESULT_FROM_WIN32(::GetCurrentPackageInfo(flags, &bufferLength, buffer.get(), &packageCount)));
        packageGraph = reinterpret_cast<PACKAGE_INFO*>(buffer.get());
        return S_OK;
    }
}

ModernResourceProvider::ModernResourceProvider(_In_ CCoreServices *pCore)
    : m_pCore(pCore)
{
}

ModernResourceProvider::~ModernResourceProvider()
{
    DetachEvents();
}

//-----------------------------------------------------------------------------
//
// Attempts to create a ModernResourceProvider that uses an MRT ResourceManager.
// If MRT can't be used, the out param is set to nullptr.
//
//-----------------------------------------------------------------------------
_Check_return_
HRESULT ModernResourceProvider::TryCreate(
    _In_ CCoreServices *pCore,
    _Outptr_result_maybenull_ ModernResourceProvider** ppResourceProvider)
{
    *ppResourceProvider = nullptr;

    TRACE_HR_NORETURN(ModernResourceProvider::Create(pCore, ppResourceProvider));

    return S_OK;
}

//-----------------------------------------------------------------------------
//
// Creates a ModernResourceProvider that instantiates and uses a MRT ResourceManager.
//
//-----------------------------------------------------------------------------
_Check_return_
HRESULT ModernResourceProvider::Create(
    _In_ CCoreServices *pCore,
    _Outptr_ ModernResourceProvider **ppResourceProvider)
{
    *ppResourceProvider = nullptr;

    xref_ptr<ModernResourceProvider> resourceProvider;
    resourceProvider.attach(new ModernResourceProvider(pCore));

    wrl::ComPtr<mwar::IResourceManagerFactory> resourceManagerFactory;
    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_Windows_ApplicationModel_Resources_ResourceManager).Get(),
        &resourceManagerFactory));

    // Give the app a chance to provide its own ResourceManager to handle app resources
    HRESULT xr = FxCallbacks::FrameworkApplication_GetResourceManagerOverrideFromApp(resourceProvider->m_appResourceManager);
    if (FAILED(xr) || resourceProvider->m_appResourceManager == nullptr)
    {
        IFC_RETURN(wf::ActivateInstance(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_Windows_ApplicationModel_Resources_ResourceManager).Get(),
            &(resourceProvider->m_appResourceManager)));
    }

    xstring_ptr frameworkPackageResourcesPriPath;
    IFC_RETURN(ProbeForFrameworkPackageResourcesPri(&frameworkPackageResourcesPriPath, resourceProvider->m_shouldUseSystemLanguage));
    if (!frameworkPackageResourcesPriPath.IsNullOrEmpty())
    {
        IFC_RETURN(resourceManagerFactory->CreateInstance(
            wrl_wrappers::HStringReference(frameworkPackageResourcesPriPath.GetBuffer(), frameworkPackageResourcesPriPath.GetCount()).Get(),
            &(resourceProvider->m_frameworkPackageResourceManager)));
    }

    IFC_RETURN(resourceProvider->InitializeContext());

    *ppResourceProvider = resourceProvider.detach();

    return S_OK;
}

_Check_return_ HRESULT ModernResourceProvider::InitializeContext()
{
    // ResourceContext instances are tied to the ResourceManager instance that created it

    if (m_appResourceManager && !m_appResourceContext)
    {
        IFC_RETURN(m_appResourceManager->CreateResourceContext(m_appResourceContext.ReleaseAndGetAddressOf()));
    }

    if (m_frameworkPackageResourceManager && !m_frameworkPackageResourceContext)
    {
        IFC_RETURN(m_frameworkPackageResourceManager->CreateResourceContext(m_frameworkPackageResourceContext.ReleaseAndGetAddressOf()));
    }

    IFC_RETURN(UpdateContrastQualifier());
    IFC_RETURN(UpdateDeviceFamilyQualifier());
    IFC_RETURN(UpdateHomeRegionQualifier());
    IFC_RETURN(UpdateLanguageAndLayoutDirectionQualifiers());
    IFC_RETURN(SetScaleFactor(100));
    IFC_RETURN(UpdateThemeQualifier());

    return S_OK;
}

_Check_return_
HRESULT ModernResourceProvider::UpdateQualifierValue(HSTRING name, HSTRING value)
{
    wrl::ComPtr<wfc::IMap<HSTRING, HSTRING>> qualifierValues;
    boolean replaced = false;
    if (m_appResourceContext)
    {
        IFC_RETURN(m_appResourceContext->get_QualifierValues(qualifierValues.ReleaseAndGetAddressOf()));
        IFC_RETURN(qualifierValues->Insert(
            name,
            value,
            &replaced));
    }

    if (m_frameworkPackageResourceContext)
    {
        IFC_RETURN(m_frameworkPackageResourceContext->get_QualifierValues(qualifierValues.ReleaseAndGetAddressOf()));
        IFC_RETURN(qualifierValues->Insert(
            name,
            value,
            &replaced));
    }

    return S_OK;
}

_Check_return_
HRESULT ModernResourceProvider::SetScaleFactor(XUINT32 ulScaleFactor)
{
    const XUINT32 minScale = 50;
    if (ulScaleFactor < minScale)
    {
        ulScaleFactor = minScale;
    }

    IFC_RETURN(UpdateScaleQualifier(ulScaleFactor));

    // Keep the fallback provider in sync. This is a no-op today,
    // so this is currently just done to avoid surprises in the future
    // if the fallback provider starts to use scale.
    if (m_pFallbackResourceProvider)
    {
        IFC_RETURN(m_pFallbackResourceProvider->SetScaleFactor(ulScaleFactor));
    }

    return S_OK;
}

_Check_return_
HRESULT ModernResourceProvider::NotifyThemeChanged()
{
    IFC_RETURN(UpdateContrastQualifier());
    IFC_RETURN(UpdateThemeQualifier());

    return S_OK;
}

_Check_return_
HRESULT ModernResourceProvider::UpdateContrastQualifier()
{
    wrl_wrappers::HStringReference newQualifierValueReference(L"");
    auto highContrastTheme = m_pCore->GetFrameworkTheming()->GetHighContrastTheme();

    switch (highContrastTheme)
    {
        case Theming::Theme::HighContrastNone:
            newQualifierValueReference = wrl_wrappers::HStringReference(L"standard");
            break;
        case Theming::Theme::HighContrast:
        case Theming::Theme::HighContrastCustom:
            newQualifierValueReference = wrl_wrappers::HStringReference(L"high");
            break;
        case Theming::Theme::HighContrastBlack:
            newQualifierValueReference = wrl_wrappers::HStringReference(L"black");
            break;
        case Theming::Theme::HighContrastWhite:
            newQualifierValueReference = wrl_wrappers::HStringReference(L"white");
            break;
        default:
            IFC_RETURN(E_UNEXPECTED);
    }

    IFC_RETURN(UpdateQualifierValue(
        wrl_wrappers::HStringReference(MRTKnownQualifierNames::Contrast).Get(),
        newQualifierValueReference.Get()));

    return S_OK;
}

_Check_return_
HRESULT ModernResourceProvider::UpdateDeviceFamilyQualifier()
{
    ULONG deviceFamilyBufferSize = 0;
    XStringBuilder deviceFamilyStringBuilder;
    wchar_t* deviceFamilyBuffer = nullptr;

    ULONG deviceClassBufferSize = 0;
    XStringBuilder deviceClassStringBuilder;
    wchar_t* deviceClassBuffer = nullptr;

    // Get required buffer sizes
    // deviceFamilyBufferSize and deviceClassBufferSize will be filled
    // with the required buffer size (including the terminating null character)
    NTSTATUS status = RtlConvertDeviceFamilyInfoToString(
        &deviceFamilyBufferSize,
        &deviceClassBufferSize,
        nullptr, nullptr);

    if (status != STATUS_BUFFER_TOO_SMALL)
    {
        IFC_RETURN(HRESULT_FROM_NT(status));
    }

    // allocate space
    // RtlConvertDeviceFamilyInfoToString gives us the required buffer size in
    // *bytes* (multiplying the character count by the size of a wchar_t), so we
    // need to convert it back to character count when initializing our string builder
    IFC_RETURN(deviceFamilyStringBuilder.InitializeAndGetFixedBuffer((deviceFamilyBufferSize / sizeof(wchar_t)) - 1, &deviceFamilyBuffer));
    IFC_RETURN(deviceClassStringBuilder.InitializeAndGetFixedBuffer((deviceClassBufferSize / sizeof(wchar_t)) - 1, &deviceClassBuffer));

    // get the actual data
    IFC_RETURN(HRESULT_FROM_NT(RtlConvertDeviceFamilyInfoToString(
        &deviceFamilyBufferSize,
        &deviceClassBufferSize,
        deviceFamilyBuffer,
        deviceClassBuffer)));

    // The kernel gives us names of the form "Windows.*" but we only normally care about the part after the dot.
    // To guard against possible addition of other prefixes, we convert any remaining dots to underscore so they
    // can be expressed in a file name.
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(DefaultDeviceFamilyPrefix, L"Windows.");
    xstring_ptr deviceFamilyWithPrefix;
    xstring_ptr deviceFamily;
    IFC_RETURN(deviceFamilyStringBuilder.DetachString(&deviceFamilyWithPrefix));
    if (deviceFamilyWithPrefix.StartsWith(DefaultDeviceFamilyPrefix))
    {
        IFC_RETURN(deviceFamilyStringBuilder.Initialize(deviceFamilyWithPrefix));
        IFC_RETURN(deviceFamilyStringBuilder.ShiftLeft(DefaultDeviceFamilyPrefix.GetCount()));
        deviceFamilyStringBuilder.Replace(L'.', L'_');
        IFC_RETURN(deviceFamilyStringBuilder.DetachString(&deviceFamily));
    }
    else
    {
        deviceFamily = deviceFamilyWithPrefix;
    }

    wrl_wrappers::HString newQualifierValue;
    IFC_RETURN(newQualifierValue.Set(deviceFamily.GetBuffer(), deviceFamily.GetCount()));
    IFC_RETURN(UpdateQualifierValue(
        wrl_wrappers::HStringReference(MRTKnownQualifierNames::DeviceFamily).Get(),
        newQualifierValue));

    return S_OK;
}

_Check_return_
HRESULT ModernResourceProvider::UpdateHomeRegionQualifier()
{
    wrl_wrappers::HString newQualifierValue;

    wrl::ComPtr<wg::IGeographicRegion> region;
    IFC_RETURN(wf::ActivateInstance(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_GeographicRegion).Get(),
        &region));

    // Per MSDN, HomeRegion is any ISO 3166-1 alpha-2 two-letter region code, plus the set of
    // ISO 3166-1 numeric three-digit geographic codes for composed regions.
    //
    // https://docs.microsoft.com/en-us/windows/uwp/app-resources/tailor-resources-lang-scale-contrast?branch=live#homeregion
    wrl_wrappers::HString twoLetterCode;
    wrl_wrappers::HString threeDigitCode;
    IFC_RETURN(region->get_CodeTwoLetter(twoLetterCode.GetAddressOf()));
    if (twoLetterCode == wrl_wrappers::HStringReference(L"ZZ"))
    {
        // two letter code doesn't exist; try three digit code
        IFC_RETURN(region->get_CodeThreeDigit(threeDigitCode.GetAddressOf()));
        if (threeDigitCode == wrl_wrappers::HStringReference(L"999"))
        {
            // three letter code doesn't exist; default to the "World" tag
            newQualifierValue.Attach(wrl_wrappers::HStringReference(L"001").Get());
        }
        else
        {
            newQualifierValue = std::move(threeDigitCode);
        }
    }
    else
    {
        newQualifierValue = std::move(twoLetterCode);
    }

    IFC_RETURN(UpdateQualifierValue(
        wrl_wrappers::HStringReference(MRTKnownQualifierNames::HomeRegion).Get(),
        newQualifierValue));

    return S_OK;
}

_Check_return_
HRESULT ModernResourceProvider::UpdateLanguageAndLayoutDirectionQualifiers()
{
    wrl::ComPtr<wg::IApplicationLanguagesStatics> applicationLanguagesStatics;
    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_ApplicationLanguages).Get(),
        &applicationLanguagesStatics));

    wrl_wrappers::HString primaryLanguageName;

    if (m_shouldUseSystemLanguage)
    {
        // If using the CBS package, we want to use the Windows display language
        // (the language used by OS components), not the first of the user's preferred
        // languages from ApplicationLanguages.Languages.
        // They can be different, and because
        // Windows components use the Windows display language, can lead to an
        // inconsistent language experience if WinUI 3 always uses
        // ApplicationLanguages.Languages.
        // We need to use GetUserDefaultUILanguage() to get the Windows display language,
        // GetUserDefaultLocaleName() will return the "Regional format"/sort order instead
        // if it has been set which could also be inconsistent with other OS UI.
        // Example of setting the sort order:
        // Win+R ->  intl.cpl -> "Change sorting method" -> "Format": "German (Germany)" ->
        // "Change sorting method" -> "Select the sorting method:" "Phone book (DIN)"
        boolean isLocaleValid = false;
        boolean isWellFormedLanguage = false;

        wrl::ComPtr<wg::ILanguageStatics> languageStatics;
        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_Language).Get(),
            &languageStatics));

        wchar_t lpLocaleName[LOCALE_NAME_MAX_LENGTH] = { 0 };
        isLocaleValid = GetLocaleInfo(MAKELCID(GetUserDefaultUILanguage(), SORT_DEFAULT), LOCALE_SNAME, lpLocaleName, LOCALE_NAME_MAX_LENGTH) != FALSE;

        if (isLocaleValid)
        {
            IFC_RETURN(languageStatics->IsWellFormed(wrl_wrappers::HStringReference(lpLocaleName).Get(), &isWellFormedLanguage));
        }

        if (isLocaleValid && isWellFormedLanguage)
        {
            // CopyTo() calls WindowsDuplicateString() which will make a deep copy of the
            // underlying fast-pass lpLocaleName buffer,
            // making primaryLanguageName safe to use outside of lpLocaleName's scope
            wrl_wrappers::HStringReference(lpLocaleName).CopyTo(primaryLanguageName.GetAddressOf());
        }
        else
        {
            // If the Windows display language isn't bcp47 compliant for some reason, fall back to the user's preferred language list.
            // This could still be an inconsistent localization experience, but is the next best option.
            wrl::ComPtr<wfc::IVectorView<HSTRING>> languages;
            IFC_RETURN(applicationLanguagesStatics->get_Languages(languages.ReleaseAndGetAddressOf()));
            IFC_RETURN(languages->GetAt(0, primaryLanguageName.GetAddressOf()));
        }
    }
    else
    {
        // Current language is the first element of Windows.Globalization.ApplicationLanguages.Languages
        wrl::ComPtr<wfc::IVectorView<HSTRING>> languages;
        IFC_RETURN(applicationLanguagesStatics->get_Languages(languages.ReleaseAndGetAddressOf()));
        IFC_RETURN(languages->GetAt(0, primaryLanguageName.GetAddressOf()));
    }

    wrl::ComPtr<wg::ILanguageFactory> languageFactory;
    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_Language).Get(),
        &languageFactory));
    wrl::ComPtr<wg::ILanguage> primaryLanguage;
    IFC_RETURN(languageFactory->CreateLanguage(primaryLanguageName.Get(), &primaryLanguage));

    // Note: ILanguage2 is only available on RS4+
    wrl::ComPtr<wg::ILanguage2> primaryLanguage2;
    IFC_RETURN(primaryLanguage.As(&primaryLanguage2));

    // Update the Language qualifier
    {
        wrl_wrappers::HString newQualifierValue;
        IFC_RETURN(primaryLanguage->get_LanguageTag(newQualifierValue.GetAddressOf()));
        IFC_RETURN(UpdateQualifierValue(
            wrl_wrappers::HStringReference(MRTKnownQualifierNames::Language).Get(),
            newQualifierValue));
    }

    // Update the LayoutDirection qualifier
    {
        wg::LanguageLayoutDirection layoutDirection;
        wrl_wrappers::HStringReference newQualifierValueReference(L"");
        IFC_RETURN(primaryLanguage2->get_LayoutDirection(&layoutDirection));
        switch (layoutDirection)
        {
            case wg::LanguageLayoutDirection_Ltr:
                newQualifierValueReference = wrl_wrappers::HStringReference(L"LTR");
                break;
            case wg::LanguageLayoutDirection_Rtl:
                newQualifierValueReference = wrl_wrappers::HStringReference(L"RTL");
                break;
            case wg::LanguageLayoutDirection_TtbLtr:
                newQualifierValueReference = wrl_wrappers::HStringReference(L"TTBLTR");
                break;
            case wg::LanguageLayoutDirection_TtbRtl:
                newQualifierValueReference = wrl_wrappers::HStringReference(L"TTBRTL");
                break;
            default:
                IFC_RETURN(E_UNEXPECTED);
        }
        IFC_RETURN(UpdateQualifierValue(
            wrl_wrappers::HStringReference(MRTKnownQualifierNames::LayoutDirection).Get(),
            newQualifierValueReference.Get()));
    }

    return S_OK;
}

_Check_return_
HRESULT ModernResourceProvider::UpdateScaleQualifier(XUINT32 ulScaleFactor)
{
    wchar_t buffer[_MAX_ULTOSTR_BASE10_COUNT];
    if (_ultow_s(ulScaleFactor, buffer, 10))
    {
        IFC_RETURN(HRESULT_FROM_WIN32(_doserrno))
    }

    wrl_wrappers::HString newQualifierValue;
    IFC_RETURN(newQualifierValue.Set(buffer));
    IFC_RETURN(UpdateQualifierValue(
        wrl_wrappers::HStringReference(MRTKnownQualifierNames::Scale).Get(),
        newQualifierValue));

    return S_OK;
}

_Check_return_
HRESULT ModernResourceProvider::UpdateThemeQualifier()
{
    wrl_wrappers::HStringReference newQualifierValueReference(L"");
    auto theme = m_pCore->GetFrameworkTheming()->GetBaseTheme();

    switch (theme)
    {
        case Theming::Theme::Light:
            newQualifierValueReference = wrl_wrappers::HStringReference(L"light");
            break;
        case Theming::Theme::Dark:
            newQualifierValueReference = wrl_wrappers::HStringReference(L"dark");
            break;
        default:
            IFC_RETURN(E_UNEXPECTED);
    }

    IFC_RETURN(UpdateQualifierValue(
        wrl_wrappers::HStringReference(MRTKnownQualifierNames::Theme).Get(),
        newQualifierValueReference.Get()));

    return S_OK;
}

_Check_return_
HRESULT ModernResourceProvider::TryGetResourceCandidate(
    _In_ IPALUri* pMSResourceUri,
    _Out_ wrl::ComPtr<mwar::IResourceCandidate>& resourceCandidate)
{
    resourceCandidate.Reset();

    xstring_ptr strResourceMapName;
    xstring_ptr strFileRelativePath;
    bool hadFileRelativePath = false;
    IFC_RETURN(MsUriHelpers::CrackMsResourceUri(pMSResourceUri, &strResourceMapName, NULL, &strFileRelativePath, &hadFileRelativePath));
    if (!hadFileRelativePath)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    xstring_ptr strResourceName;
    IFC_RETURN(PrepareModernResourceName(
        strFileRelativePath.GetCount(),
        strFileRelativePath.GetBuffer(),
        &strResourceName));

    // A URI with an empty resource name ("ms-appx://") will always fail to resolve
    // Failing to resolve a resource is not an error condition
    if(strResourceName.IsNullOrEmpty())
    {
        return S_OK;
    }

    // We need to construct the final MRT resource name:
    // [<named resource map>/]Files/<resource name>
    // The use of a named resource map is optional; a scenario where it is used
    // would be a resource contained in a referenced library.
    // If we are using a resource from a named resource map, we also need to supply
    // the "ms-resource://" URI schema prefix so MRT knows to search other resource maps
    // besides the main one
    XStringBuilder mrtResourceNameBuilder;
    {
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(MsResourceUriSchema, L"ms-resource://");
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(FilesMapName, L"Files/");
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(MrtSeparator, L"/");

        IFC_RETURN(mrtResourceNameBuilder.Initialize(
            MsResourceUriSchema.GetCount() + strResourceMapName.GetCount() + MrtSeparator.GetCount() + FilesMapName.GetCount() + strResourceName.GetCount()));
        if (!strResourceMapName.IsNullOrEmpty())
        {
            mrtResourceNameBuilder.Append(MsResourceUriSchema);
            mrtResourceNameBuilder.Append(strResourceMapName);
            mrtResourceNameBuilder.Append(MrtSeparator);
        }
        IFC_RETURN(mrtResourceNameBuilder.Append(FilesMapName));
        IFC_RETURN(mrtResourceNameBuilder.Append(strResourceName));
    }

    wrl::ComPtr<mwar::IResourceManager> resourceManager;
    wrl::ComPtr<mwar::IResourceContext> resourceContext;
    if (m_frameworkPackageResourceManager && IsProjectReunionFrameworkPackageResource(strResourceName))
    {
        resourceManager = m_frameworkPackageResourceManager;
        resourceContext = m_frameworkPackageResourceContext;
    }
    else
    {
        resourceManager = m_appResourceManager;
        resourceContext = m_appResourceContext;
    }

    wrl::ComPtr<mwar::IResourceMap> mainResourceMap;
    IFC_RETURN(resourceManager->get_MainResourceMap(mainResourceMap.ReleaseAndGetAddressOf()));

    // Try to load the resource directly
    // It's not inherently fatal to fail to resolve a requested resource URI;
    // there are several well-known resources (e.g. an app's various 'themes/generic.xaml')
    // that the framework will probe for but are not guaranteed to exist.
    // We want to record these failures to provide information during debugging, but
    // otherwise they should be considered non-fatal at this point in the stack.
    resourceCandidate.Reset();
    HRESULT hr = mainResourceMap->TryGetValueWithContext(
        wrl_wrappers::HStringReference(mrtResourceNameBuilder.GetBuffer(), mrtResourceNameBuilder.GetCount()).Get(),
        resourceContext.Get(),
        resourceCandidate.ReleaseAndGetAddressOf());

    if (FAILED(hr))
    {
        TRACE_HR_NORETURN(hr);
    }

    return S_OK;
}

_Check_return_
HRESULT ModernResourceProvider::TryGetLocalResource(
    _In_ IPALUri* pResourceUri,
    _Outptr_result_maybenull_ IPALResource** ppResource
    )
{
    *ppResource = nullptr;

    bool fIsMsResourceUri;
    IFC_RETURN(MsUriHelpers::IsMsResourceUri(pResourceUri, &fIsMsResourceUri));
    if (!fIsMsResourceUri)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    wrl::ComPtr<mwar::IResourceCandidate> resourceCandidate;
    IFC_RETURN(TryGetResourceCandidate(pResourceUri, resourceCandidate));

    if (resourceCandidate)
    {
        IFC_RETURN(CMRTResource::Create(pResourceUri, pResourceUri, resourceCandidate, ppResource));

        if (gps->IsDebugTraceTypeActive(XCP_TRACE_RESOURCELOADING))
        {
            xstring_ptr strResourceUri;
            IFC_RETURN(pResourceUri->GetCanonical(&strResourceUri));

            IGNOREHR(gps->DebugTrace(XCP_TRACE_RESOURCELOADING, L"ModernResourceProvider::TryGetLocalResource: %s -> %s", strResourceUri.GetBuffer(), (*ppResource)->ToString()));
        }
    }
    else
    {
        // If we couldn't find an MRT virtualized resource, fallback to looking at physical file paths
        if (gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_RESOURCELOADING | XCP_TRACE_VERBOSE)))
        {
            xstring_ptr strResourceUri;
            IFC_RETURN(pResourceUri->GetCanonical(&strResourceUri));

            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_RESOURCELOADING | XCP_TRACE_VERBOSE), L"ModernResourceProvider::TryGetLocalResource: '%s' not found. Falling back to common resource manager.", strResourceUri.GetBuffer()));
        }

        IFC_RETURN(EnsureFallbackResourceProvider());
        IFC_RETURN(m_pFallbackResourceProvider->TryGetLocalResource(pResourceUri, ppResource));
    }

    return S_OK;
}

_Check_return_
HRESULT ModernResourceProvider::GetPropertyBag(
    _In_ const IPALUri *pUri,
    _Out_ PropertyBag& propertyBag
    ) noexcept
{
    bool isMsResourceUri = false;
    IFC_RETURN(MsUriHelpers::IsMsResourceUri(pUri, &isMsResourceUri));
    if (!isMsResourceUri)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    // Get the host+path from the URI so we can use it to retrieve
    // the property bag from MRT
    xstring_ptr propertyBagResourcePath;
    {
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(MsResourceUriSchema, L"ms-resource://");
        xstring_ptr host;
        xstring_ptr path;
        IFC_RETURN(UriXStringGetters::GetHost(pUri, &host));
        IFC_RETURN(UriXStringGetters::GetPath(pUri, &path));

        XStringBuilder stringBuilder;
        IFC_RETURN(stringBuilder.Initialize(MsResourceUriSchema.GetCount() + host.GetCount() + path.GetCount()));
        IFC_RETURN(stringBuilder.Append(MsResourceUriSchema));
        IFC_RETURN(stringBuilder.Append(host));
        IFC_RETURN(stringBuilder.Append(path));

        IFC_RETURN(stringBuilder.DetachString(&propertyBagResourcePath));
    }

    wrl::ComPtr<mwar::IResourceManager> resourceManager;
    wrl::ComPtr<mwar::IResourceContext> resourceContext;
    if (m_frameworkPackageResourceManager && IsProjectReunionFrameworkPackageResource(propertyBagResourcePath))
    {
        resourceManager = m_frameworkPackageResourceManager;
        resourceContext = m_frameworkPackageResourceContext;
    }
    else
    {
        resourceManager = m_appResourceManager;
        resourceContext = m_appResourceContext;
    }

    wrl::ComPtr<mwar::IResourceMap> resourceMap;
    IFC_RETURN(resourceManager->get_MainResourceMap(resourceMap.ReleaseAndGetAddressOf()));

    propertyBag.clear();
    wrl::ComPtr<mwar::IResourceMap> elementMap;
    HRESULT hr = resourceMap->TryGetSubtree(
            wrl_wrappers::HStringReference(propertyBagResourcePath.GetBuffer()).Get(),
            elementMap.ReleaseAndGetAddressOf());
    if (FAILED(hr) || elementMap == nullptr)
    {
        // We could not find a matching subtree--give up and return an empty property
        // bag. This is needed to enable XAML to represent the default language, so that
        // the application can still run prior to the addition of the resources.resw files.
        TRACE_HR_NORETURN(hr);
        if (gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_RESOURCELOADING | XCP_TRACE_ERROR)))
        {
            xstring_ptr strCanonicalUri;
            IGNOREHR(pUri->GetCanonical(&strCanonicalUri));
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_RESOURCELOADING | XCP_TRACE_ERROR), L"GetPropertyBag: Failed to resolve resource URI %s", !strCanonicalUri.IsNull() ? strCanonicalUri.GetBuffer() : L"NULL"));
        }

        return S_OK;
    }

    UINT32 resourceCount = 0;
    IFC_RETURN(elementMap->get_ResourceCount(&resourceCount));

    if (gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_RESOURCELOADING | XCP_TRACE_VERBOSE)))
    {
        xstring_ptr strCanonicalUri;
        IGNOREHR(pUri->GetCanonical(&strCanonicalUri));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_RESOURCELOADING | XCP_TRACE_VERBOSE), L"GetPropertyBag: %s -> %d", !strCanonicalUri.IsNull() ? strCanonicalUri.GetBuffer() : L"NULL", resourceCount));
    }

    for (UINT32 resourceIndex = 0; resourceIndex < resourceCount; ++resourceIndex)
    {
        wrl::ComPtr<wfc::IKeyValuePair<HSTRING, mwar::ResourceCandidate*>> namedResource;
        IFC_RETURN(elementMap->GetValueByIndexWithContext(
            resourceIndex,
            resourceContext.Get(),
            namedResource.ReleaseAndGetAddressOf()));

        wrl_wrappers::HString resourceName;
        IFC_RETURN(namedResource->get_Key(resourceName.GetAddressOf()));

        wrl::ComPtr<mwar::IResourceCandidate> resourceCandidate;
        IFC_RETURN(namedResource->get_Value(resourceCandidate.ReleaseAndGetAddressOf()));

        xstring_ptr propertyName;
        IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(resourceName.Get(), &propertyName));

        xstring_ptr propertyValue;
        wrl_wrappers::HString propertyValueAsHString;
        IFC_RETURN(resourceCandidate->get_ValueAsString(propertyValueAsHString.GetAddressOf()));
        IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(propertyValueAsHString.Get(), &propertyValue));

        propertyBag.emplace_back(propertyName, propertyValue);
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Prepares a resource name to use with MRT APIs from a resource URI.
//
//---------------------------------------------------------------------------
_Check_return_
HRESULT ModernResourceProvider::PrepareModernResourceName(
    _In_ XUINT32 uriLength,
    _In_reads_(uriLength) const WCHAR *pResourceUri,
    _Out_ xstring_ptr* pstrResourceName
    )
{
    if (uriLength == 0)
    {
        pstrResourceName->Reset();
    }
    else
    {
        // The URI can contain escape sequences, which MRT doesn't understand (e.g. %20 won't be
        // treated as a space), so unescape the URI first.
        WCHAR unescapedUri[INTERNET_MAX_URL_LENGTH];
        DWORD unescapedUriLength = INTERNET_MAX_URL_LENGTH;
        IFC_RETURN(UrlCanonicalizeW(pResourceUri, unescapedUri, &unescapedUriLength, URL_UNESCAPE));

        IFC_RETURN(xstring_ptr::CloneBuffer(unescapedUri, unescapedUriLength, pstrResourceName));
    }
    return S_OK;
}

/* static */ _Check_return_ HRESULT
ModernResourceProvider::ProbeForFrameworkPackageResourcesPri(_Out_ xstring_ptr* frameworkPackageResourcePriPath, _Out_ bool& isUsingCbsPackage)
{
    *frameworkPackageResourcePriPath = xstring_ptr::EmptyString();
    isUsingCbsPackage = false;

    const UINT32 c_filter{ PACKAGE_FILTER_HEAD | PACKAGE_FILTER_DIRECT | PACKAGE_FILTER_STATIC | PACKAGE_FILTER_DYNAMIC | PACKAGE_INFORMATION_BASIC };
    std::uint32_t packageCount{};
    const PACKAGE_INFO* packageGraph{};
    std::unique_ptr<BYTE[]> packageBuffer;
    IFC_RETURN(GetCurrentPackageGraph(c_filter, packageCount, packageGraph, packageBuffer));
    if (packageGraph)
    {
        for (std::uint32_t index=0; index < packageCount; index++)
        {
            const PACKAGE_INFO& packageInfo{ packageGraph[index] };

            if (IsProjectReunionCBSPackage(packageInfo.packageFamilyName))
            {
                isUsingCbsPackage = true;
            }

            if (IsProjectReunionFrameworkPackage(packageInfo.packageFamilyName))
            {
                auto frameworkPackagePath{ packageInfo.path };
                std::size_t frameworkPackagePathLength{ wcslen(packageInfo.path) };

                std::size_t candidatePathLength{ frameworkPackagePathLength + 1 + ARRAY_SIZE(c_resourcesPriName) };
                std::unique_ptr<wchar_t[]> candidatePath{ new wchar_t[candidatePathLength] };

                IFC_RETURN(PathCchCombineEx(
                    candidatePath.get(),
                    candidatePathLength,
                    frameworkPackagePath,
                    c_resourcesPriName,
                    PATHCCH_ALLOW_LONG_PATHS));

                if (!PathFileExists(candidatePath.get()))
                {
                    // Can't find resources.pri, so just return the empty string
                    // (out parameter already initialized at beginning of method)
                    return S_OK;
                }
                else
                {
                    IFC_RETURN(xstring_ptr::CloneBuffer(candidatePath.get(), frameworkPackageResourcePriPath));
                    return S_OK;
                }
            }
        }
    }

    // No dependency on Project Reunion framework package, so just return the empty string
    // (out parameter already initialized at beginning of method)
    return S_OK;
}

_Check_return_ HRESULT ModernResourceProvider::EnsureFallbackResourceProvider()
{
    if (!m_pFallbackResourceProvider)
    {
        IFC_RETURN(CommonResourceProvider::Create(m_pFallbackResourceProvider.ReleaseAndGetAddressOf()));
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
//
// See IPALResourceManager::SetProcessMUILanguages().
//
//-----------------------------------------------------------------------------
_Check_return_
HRESULT ModernResourceProvider::SetProcessMUILanguages()
{
    IFC_RETURN(EnsureFallbackResourceProvider());
    IFC_RETURN(m_pFallbackResourceProvider->SetProcessMUILanguages());

    return S_OK;
}
