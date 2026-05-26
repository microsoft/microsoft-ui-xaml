// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "DefaultStyles.h"
#include "XamlReader.g.h"
#include "Style.g.h"
#include "Control.g.h"
#include "DXamlCore.h"
#include <ParserAPI.h>
#include <CColor.h>
#include <FrameworkTheming.h>
#include "CValueUtil.h"
#include <xstrutil.h>
#include <TypeNameHelper.h>
#include "xcperrorresource.h"
#include <wininet.h>

using namespace DirectUI;
using namespace xaml_hosting;

#if DBG
static const WCHAR XAML_DEBUG_KEY_NAME[] = XAML_ROOT_KEY L"\\Debug";
static const WCHAR XAML_DEBUG_VALUE_NAME[] = L"GenericXamlPath";

namespace
{
    //---------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Determines whether we should use generic XBF or XAML files for
    //      default styles.
    //
    //  Notes:
    //      Queries the String value "GenericXamlPath" in the registry key
    //      HKLM\[XAML_ROOT_KEY]\Debug. If it doesn't exist, returns TRUE.
    //      If it does exist, its value should be the fully qualified path to generic.xaml.
    //
    //      This was added to help the controls team speed their development process,
    //      since they update generic.xaml frequently.
    //
    //---------------------------------------------------------------------------
    bool ShouldUseGenericXbf()
    {
        WCHAR szXamlGenericXamlPath[MAX_PATH];
        DWORD cbData = sizeof(szXamlGenericXamlPath);

        return ::RegGetValue(HKEY_LOCAL_MACHINE, XAML_DEBUG_KEY_NAME, XAML_DEBUG_VALUE_NAME, RRF_RT_REG_SZ, NULL, szXamlGenericXamlPath, &cbData) != ERROR_SUCCESS;
    }
}
#endif

StyleResourceHelper::StyleResourceHelper()
{
}

StyleResourceHelper::~StyleResourceHelper()
{
}

_Check_return_ HRESULT StyleResourceHelper::GetResourceUriPath(
    _In_opt_z_ const WCHAR* wszLibraryName,
    _In_z_ const WCHAR* wszResourceFileName,
    _Out_ xstring_ptr* pstrResourceUri
    )
{
    HRESULT hr = S_OK;

    XStringBuilder resourceUriBuilder;

    pstrResourceUri->Reset();

    IFC(resourceUriBuilder.Initialize(INTERNET_MAX_URL_LENGTH));

    IFC(resourceUriBuilder.Append(STR_LEN_PAIR(L"ms-resource:///Files/")));

    if (wszLibraryName)
    {
        IFC(resourceUriBuilder.Append(wszLibraryName, xstrlen(wszLibraryName)));
        IFC(resourceUriBuilder.AppendChar(L'/'));
    }

    IFC(resourceUriBuilder.Append(wszResourceFileName, xstrlen(wszResourceFileName)));

    IFC(resourceUriBuilder.DetachString(pstrResourceUri));

Cleanup:
    RRETURN(hr);
}

StyleCache::StyleCache() :
    m_pFrameworkStyles(NULL),
    m_fLoadedAppStyles(FALSE),
    m_fLoadedThemeXaml(FALSE),
    m_fLoadGenericXaml(FALSE),
    m_pAppStyles(NULL)
{
}

StyleCache::~StyleCache()
{
    Clear();
}

_Check_return_ HRESULT StyleCache::LoadStylesFromResourceUri(
    _In_ const xstring_ptr& strResourceUri,
    _Outptr_ ResourceDictionary** ppStyles,
    _Out_ bool *pfResourceLocated)
{
    xref_ptr<IPALMemory> spXamlMemory;
    bool bIsBinaryXaml = false;

    *pfResourceLocated = FALSE;

    if (FAILED(CoreImports::CoreServices_TryGetApplicationResource(DXamlCore::GetCurrent()->GetHandle(), strResourceUri, spXamlMemory.ReleaseAndGetAddressOf(), &bIsBinaryXaml)))
    {
        return S_OK;
    }

    *pfResourceLocated = TRUE;

    Parser::XamlBuffer buffer;
    if (!bIsBinaryXaml)
    {
        buffer.m_bufferType = Parser::XamlBufferType::Text;
        buffer.m_count = spXamlMemory->GetSize();
        buffer.m_buffer = static_cast<const XUINT8*>(spXamlMemory->GetAddress());
    }
    else
    {
        // create dummy data
        buffer.m_bufferType = Parser::XamlBufferType::Text;
        buffer.m_buffer = reinterpret_cast<const XUINT8*>(L" ");
        buffer.m_count = 2;
    }

    IFC_RETURN(LoadStylesFromBuffer(buffer, XSTRING_PTR_EPHEMERAL(L"Microsoft.UI.Xaml.dll"), strResourceUri, ppStyles));

    return S_OK;
}

_Check_return_ HRESULT StyleCache::LoadStylesFromFile(
    _In_z_ const WCHAR* wszXamlFilePath,
    _Outptr_ ResourceDictionary** ppStyles)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spObject;
    ctl::ComPtr<xaml::IResourceDictionary> spResourceDictionary;

    IFC(StyleCache::LoadFromFile(wszXamlFilePath, &spObject));
    IFC(spObject.As(&spResourceDictionary));

    *ppStyles = static_cast<ResourceDictionary*>(spResourceDictionary.Detach());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT StyleCache::LoadFromFile(
    _In_z_ const WCHAR* wszXamlFilePath,
    _Outptr_ IInspectable** ppObject)
{
    DWORD dwRead = 0;
    wrl_wrappers::HString strXaml;
    ctl::ComPtr<XamlReaderFactory> spXamlReader;

    HANDLE hFile = CreateFile(
        wszXamlFilePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    auto guard = wil::scope_exit([&hFile]()
    {
        if (hFile)
        {
            CloseHandle(hFile);
        }
    });

    if (INVALID_HANDLE_VALUE == hFile)
    {
        IFC_RETURN(HRESULT_FROM_WIN32(GetLastError()));
    }

    DWORD dwSize = GetFileSize(hFile, nullptr);
    std::unique_ptr<char[]> pBuffer(new char[dwSize]);

    if (!ReadFile(
        hFile,
        pBuffer.get(),
        dwSize,
        &dwRead,
        nullptr))
    {
        IFC_RETURN(HRESULT_FROM_WIN32(GetLastError()));
    }

    IFCEXPECT_RETURN(dwSize == dwRead);

    XUINT32 cchXaml = MultiByteToWideChar(
        CP_UTF8,
        0,
        pBuffer.get(),
        dwSize,
        nullptr,
        0);

    std::unique_ptr<WCHAR[]> wXaml(new WCHAR[cchXaml + 1]);

    if (0 == MultiByteToWideChar(
        CP_UTF8,
        0,
        pBuffer.get(),
        dwSize,
        wXaml.get(),
        cchXaml))
    {
        IFC_RETURN(HRESULT_FROM_WIN32(GetLastError()));
    }

    wXaml[cchXaml] = static_cast<WCHAR>(0);

    IFC_RETURN(strXaml.Set(wXaml.get(), cchXaml));

    IFC_RETURN(ctl::make<XamlReaderFactory>(&spXamlReader));
    IFC_RETURN(spXamlReader->Load(strXaml.Get(), ppObject));


    return S_OK;
}

_Check_return_ HRESULT StyleCache::LoadStylesFromBuffer(
    _In_ const Parser::XamlBuffer& buffer,
    _In_ const xstring_ptr_view& strSourceAssemblyName,
    _In_ const xstring_ptr_view& strResourceUri,
    _Outptr_ ResourceDictionary** ppStyles)
{
    CValue value;

    *ppStyles = nullptr;

    IFC_RETURN(CoreImports::CreateFromXamlBytes(
            DXamlCore::GetCurrent()->GetHandle(),
            buffer,
            strSourceAssemblyName,
            true,
            false,
            strResourceUri,
            &value));

    ctl::ComPtr<IInspectable> spResourceDictionaryAsInsp;
    ctl::ComPtr<xaml::IResourceDictionary> spResourceDictionary;

    IFC_RETURN(CValueBoxer::UnboxObjectValue(&value, nullptr, &spResourceDictionaryAsInsp));

    ctl::ComPtr<xaml::IDependencyObject> pDO;
    pDO.Attach(ctl::query_interface<xaml::IDependencyObject>(spResourceDictionaryAsInsp.Get()));

    // Release reference held by the parser now that UnboxedObject holds a new reference.
    if (pDO)
    {
        static_cast<DependencyObject *>(pDO.Get())->UnpegNoRef();
    }

    IFC_RETURN(spResourceDictionaryAsInsp.As(&spResourceDictionary));

    *ppStyles = static_cast<ResourceDictionary*>(spResourceDictionary.Detach());

    return S_OK;
}

_Check_return_
HRESULT StyleCache::LoadStylesFromResource(
    _In_ const xstring_ptr_view& strResourceName,
    _In_ const xstring_ptr_view& strResourceUri,
    _Outptr_ ResourceDictionary **ppStyles)
{
    Parser::XamlBuffer buffer;

    IFC_RETURN(DXamlCore::GetCurrent()->GetResourceBytes(strResourceName.GetBuffer(), &buffer));
    IFCEXPECT_RETURN(buffer.m_buffer);

    IFC_RETURN(LoadStylesFromBuffer(
        buffer,
        XSTRING_PTR_EPHEMERAL(L"Microsoft.UI.Xaml.dll"), //TODO: do we care about the URIs hardcoding Microsoft.UI.Xaml.dll?
        strResourceUri,
        ppStyles));

    return S_OK;
}

_Check_return_ HRESULT StyleCache::GetFrameworkStyles(_Outptr_ ResourceDictionary** ppStyles)
{
    HRESULT hr = S_OK;

    if (!m_pFrameworkStyles)
    {
        DXamlCore* pCore = DXamlCore::GetCurrent();

        // Check the generic Xaml file path that is set by MUX and load the generic Xaml file from the specified path if it is available.

        bool isGenericXamlFilePathAvailable = IsGenericXamlFilePathAvailableFromMUX();

        if (pCore && isGenericXamlFilePathAvailable)
        {
            m_fLoadGenericXaml = TRUE;

            if (SUCCEEDED(LoadStylesFromFile(pCore->GetGenericXamlFilePath(), &m_pFrameworkStyles)))
            {
                *ppStyles = m_pFrameworkStyles.Get();
                ctl::addref_interface(*ppStyles);

                // Set the ThemeResources with the FrameworkStyles since Generic.Xaml is loaded for Styles and ThemeResources.
                pCore->GetHandle()->SetThemeResources(static_cast<CResourceDictionary*>(m_pFrameworkStyles->GetHandle()));
                m_fLoadedThemeXaml = TRUE;

                goto Cleanup;
            }
            else
            {
                pCore->ClearGenericXamlFilePathFromMUX();

                m_fLoadGenericXaml = FALSE;
            }
        }

#if DBG
        WCHAR szXamlGenericXamlPath[MAX_PATH];
        DWORD cbData = sizeof(szXamlGenericXamlPath);

        if (::RegGetValue(HKEY_LOCAL_MACHINE, XAML_DEBUG_KEY_NAME, XAML_DEBUG_VALUE_NAME, RRF_RT_REG_SZ, NULL, szXamlGenericXamlPath, &cbData) == ERROR_SUCCESS)
        {
            m_fLoadGenericXaml = TRUE;
            if (SUCCEEDED(LoadStylesFromFile(szXamlGenericXamlPath, &m_pFrameworkStyles)))
            {
                *ppStyles = m_pFrameworkStyles.Get();
                ctl::addref_interface(*ppStyles);

                // Set the ThemeResources with the FrameworkStyles since Generic.Xaml is loaded for Styles and ThemeResources.
                pCore->GetHandle()->SetThemeResources(static_cast<CResourceDictionary*>(m_pFrameworkStyles->GetHandle()));
                m_fLoadedThemeXaml = TRUE;

                goto Cleanup;
            }
            else
            {
                m_fLoadGenericXaml = FALSE;
            }
        }
#endif
        // In case we should use XBF
        if (!IsGenericXamlFilePathAvailableFromMUX())
        {
            IFC(LoadStylesFromResource(
                XSTRING_PTR_EPHEMERAL(L"Styles.xbf"),
                XSTRING_PTR_EPHEMERAL(L"Microsoft.UI.Xaml;component/themes/generic.xaml"),
                &m_pFrameworkStyles));
        }

        if (!m_pFrameworkStyles)
        {
            ctl::ComPtr<ResourceDictionary> spResourceDictionary;
            IFC(ctl::make(&spResourceDictionary));

            m_pFrameworkStyles = spResourceDictionary;
        }
    }

    *ppStyles = m_pFrameworkStyles.Get();
    ctl::addref_interface(*ppStyles);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT StyleCache::GetAppStyles(_Outptr_result_maybenull_ ResourceDictionary** ppStyles)
{
    HRESULT hr = S_OK;
    WCHAR* wszGenericXamlFilePath = NULL;
    xstring_ptr strResourceUri;
    bool fResourceLocated = false;

    if (!m_fLoadedAppStyles)
    {
        // look for a generic.xaml that's in the main resource map (i.e. the app's resources)
        IFC(m_resourceHelper.GetResourceUriPath(NULL, L"themes/generic.xaml", &strResourceUri));

        IFC(LoadStylesFromResourceUri(strResourceUri, &m_pAppStyles, &fResourceLocated));
        if (!fResourceLocated)
        {
            m_fLoadedAppStyles = TRUE;
        }
    }

    *ppStyles = m_pAppStyles;
    ctl::addref_interface(*ppStyles);

Cleanup:
    delete[] wszGenericXamlFilePath;

    RRETURN(hr);
}

_Check_return_ HRESULT StyleCache::LoadThemeResources()
{
    if (m_fLoadedThemeXaml)
    {
        return S_OK;
    }

    if (m_fLoadGenericXaml)
    {
        return S_OK;
    }

    // Prevent recursive calls by setting to TRUE before resources are loaded,
    // because StaticResources in themes could cause another request for
    // ThemeResources to be loaded.
    m_fLoadedThemeXaml = TRUE;
    auto guardLoaded = wil::scope_exit([this] {
        m_fLoadedThemeXaml = FALSE;
    });

    TRACE(TraceAlways, L"Loading theme resources");

    bool useXbf = !IsGenericXamlFilePathAvailableFromMUX();

#if DBG
    useXbf = useXbf ? !!ShouldUseGenericXbf() : false;
#endif
    DXamlCore::GetCurrent()->GetHandle()->SetIsLoadingGlobalThemeResources(true);
    auto guardIsLoading = wil::scope_exit([] {
        DXamlCore::GetCurrent()->GetHandle()->SetIsLoadingGlobalThemeResources(false);
    });

    if (useXbf)
    {
        ctl::ComPtr<ResourceDictionary> themeResources;
        IFC_RETURN(LoadStylesFromResource(
            XSTRING_PTR_EPHEMERAL(L"themeresources.xbf"),
            XSTRING_PTR_EPHEMERAL(L"Microsoft.UI.Xaml;component/themes/themeresources.xbf"),
            themeResources.ReleaseAndGetAddressOf()));
        DXamlCore::GetCurrent()->GetHandle()->SetThemeResources(static_cast<CResourceDictionary*>(themeResources->GetHandle()));
        TRACE(TraceAlways, L"Loaded theme resources from themeresources.xbf");
    }
    else
    {
        ctl::ComPtr<ResourceDictionary> styles;
        IFC_RETURN(GetFrameworkStyles(&styles));
        DXamlCore::GetCurrent()->GetHandle()->SetThemeResources(static_cast<CResourceDictionary*>(styles->GetHandle()));
        TRACE(TraceAlways, L"Loaded theme resources from loose file generic.xaml because GenericXamlPath reg key was set.");
    }

    guardLoaded.release();

    return S_OK;
}

void StyleCache::Clear()
{
    if (m_pFrameworkStyles)
    {
        m_pFrameworkStyles->UnpegNoRef();
        m_pFrameworkStyles.Reset();
    }
    ctl::release_interface(m_pAppStyles);
    m_fLoadedAppStyles = FALSE;
    m_stylesMap.clear();

    DXamlCore *pCore = DXamlCore::GetCurrent();
    if (pCore != NULL)
    {
        CCoreServices *pCoreServices = pCore->GetHandle();
        if (pCoreServices != NULL)
        {
            pCoreServices->SetThemeResources(nullptr);
        }
    }

    m_fLoadedThemeXaml = FALSE;
    m_fLoadGenericXaml = FALSE;
}

_Check_return_ HRESULT StyleCache::GetStyles(
    _In_z_ const WCHAR* wszNamespace,
    _In_opt_z_ const WCHAR* wszAssemblyName,
    _In_opt_ wf::IUriRuntimeClass* pUri,
    _Outptr_result_maybenull_ ResourceDictionary** ppStyles)
{
    HRESULT hr = S_OK;
    bool fFound = false;
    bool fAddToCache = true;
    ResourceDictionary* pStyles = NULL;
    bool fFrameworkNamespace;
    WCHAR* wszGenericXamlFilePath = NULL;
    WCHAR* wszParentNamespace = NULL;
    bool fResourceLocated = false;

    // the library name is the assembly name if available, else the namespace
    const WCHAR* wszLibraryName = wszAssemblyName ? wszAssemblyName : wszNamespace;
    xstring_ptr strStyleCacheKey;

    // create the cache key for this control's style
    if (!pUri)
    {
        // if DefaultStyleResourceUri is not defined, use the assembly name or namespace as the cache key
        IFC(xstring_ptr::CloneBuffer(wszLibraryName, xstrlen(wszLibraryName), &strStyleCacheKey));
    }
    else
    {
        // if DefaultStyleResourceUri is defined, use the full URI string as the cache key
        wrl_wrappers::HString strResourceUri;
        IFC(pUri->get_AbsoluteUri(strResourceUri.GetAddressOf()));
        IFC(xstring_ptr::CloneRuntimeStringHandle(strResourceUri.Get(), &strStyleCacheKey));
    }

    *ppStyles = NULL;

    // Styles can reference theme resources, so make sure it's loaded
    if (!m_fLoadedThemeXaml)
    {
        IFC(LoadThemeResources());
    }

    // fast path: check cache
    {
        NamespaceEntry namespaceEntry;
        auto itStylesMap = m_stylesMap.find(strStyleCacheKey);

        if (itStylesMap != m_stylesMap.end())
        {
            namespaceEntry = itStylesMap->second;
            fFound = TRUE;
            fAddToCache = FALSE; // already in cache
            namespaceEntry.GetStyles(&pStyles);
        }
    }

    // check for DefaultStyleKeyUri: if available, use the full URI to find the style
    if (pUri && !fFound)
    {
        IFC(LoadStylesFromResourceUri(strStyleCacheKey, &pStyles, &fFound));

        // report error and exit the application if DefaultStyleKeyUri is set and invalid
        if (!fFound)
        {
            IFC(AgError(AG_E_RUNTIME_STYLE_DEFAULTSTYLERESOURCEURI_NOT_FOUND));
        }
    }

    // check for a framework type
    if (!fFound)
    {
        IFC(TypeNameHelper::IsFrameworkNamespace(wszNamespace, &fFrameworkNamespace));
        if (fFrameworkNamespace)
        {
            fFound = TRUE;
            IFC(GetFrameworkStyles(&pStyles));
        }
    }

    // look for generic.xaml in the resource map associated with the assembly name or namespace
    if (!fFound)
    {
        xstring_ptr strResourceUri;

        IFC(m_resourceHelper.GetResourceUriPath(
            wszLibraryName,
            L"themes/generic.xaml",
            &strResourceUri));

        IFC(LoadStylesFromResourceUri(strResourceUri, &pStyles, &fResourceLocated));
        if (fResourceLocated)
        {
            fFound = TRUE;
        }
    }

    // check parent namespace - unless an assembly name was available
    if (!fFound && !wszAssemblyName)
    {
        IFC(TypeNameHelper::GetNamespace(wszNamespace, &wszParentNamespace));
        if (wszParentNamespace)
        {
            fFound = TRUE;
            IFC(GetStyles(wszParentNamespace, nullptr, nullptr, &pStyles));
        }
    }

    // last resort: fall back to app styles
    if (!fFound)
    {
        fFound = TRUE;
        IFC(GetAppStyles(&pStyles));
    }

    if (fAddToCache)
    {
        VERIFY_COND(m_stylesMap.insert({ strStyleCacheKey, NamespaceEntry(pStyles) }), .second);
    }

    // return to caller
    *ppStyles = pStyles;
    pStyles = NULL;

Cleanup:
    ctl::release_interface(pStyles);
    delete[] wszGenericXamlFilePath;
    delete[] wszParentNamespace;

    RRETURN(hr);
}

bool StyleCache::IsGenericXamlFilePathAvailableFromMUX() const
{
    return !DXamlCore::GetCurrent()->GetGenericXamlFilePathFromMUX().IsNull();
}

__inline DWORD FormatToARGB(DWORD color)
{
    return (((((GetRValue(color) << 8) + GetGValue(color)) << 8) + GetBValue(color)) | (0xFF000000 & color));
}

_Check_return_ HRESULT
DefaultStyles::RefreshImmersiveColors()
{
    // Force a theme change to rebuild the color resources.
    IFC_RETURN(DXamlCore::GetCurrent()->GetHandle()->GetFrameworkTheming()->OnThemeChanged(true /*forceUpdate*/));
    return S_OK;
}

_Check_return_ HRESULT DefaultStyles::GetDefaultStyleByTypeInfo(
    _In_ const CClassInfo* pType,
    _Outptr_result_maybenull_ Style** ppStyle)
{
    HRESULT hr = S_OK;
    ResourceDictionary* pStyles = NULL;

    // For now, only allow this function to be called on built-in types. We'll need to refactor
    // things further to enable this fast-path for custom types as well.
    ASSERT(pType->IsBuiltinType());

    IFC(m_cache.GetFrameworkStyles(&pStyles));

    IFC(ResolveStyle(pType, pStyles, ppStyle));

Cleanup:
    ctl::release_interface(pStyles);
    RRETURN(hr);
}

_Check_return_ HRESULT DefaultStyles::GetDefaultStyleByTypeName(
    _In_ const xstring_ptr& strTypeName,
    _In_ const xstring_ptr& strAssemblyName,
    _In_opt_ wf::IUriRuntimeClass* pUri,
    _Outptr_result_maybenull_ Style** ppStyle)
{
    HRESULT hr = S_OK;
    WCHAR* wszNamespace = NULL;
    ResourceDictionary* pStyles = NULL;
    wchar_t* szTraceString = nullptr;

    IFC(TypeNameHelper::GetNamespace(strTypeName.GetBuffer(), &wszNamespace));
    hr = m_cache.GetStyles(wszNamespace, strAssemblyName.GetBuffer(), pUri, &pStyles);

    // If DefaultStyleResourceUri is defined and invalid, trace and fail-fast
    if(AgCodeFromHResult(hr) == AG_E_RUNTIME_STYLE_DEFAULTSTYLERESOURCEURI_NOT_FOUND)
    {
        // Prepare the error parameters
        wrl_wrappers::HString strResourceUri;
        IFC(pUri->get_AbsoluteUri(strResourceUri.GetAddressOf()));
        xstring_ptr strUri;
        IFC(xstring_ptr::CloneRuntimeStringHandle(strResourceUri.Get(), &strUri));

        xephemeral_string_ptr parameters[3];
        strTypeName.Demote(&parameters[0]);
        strAssemblyName.Demote(&parameters[1]);
        strUri.Demote(&parameters[2]);

        // Get the ErrorService
        DXamlCore* pCore = DXamlCore::GetCurrentNoCreate();
        IFCPTR(pCore);
        CCoreServices* pCCoreServices = pCore->GetHandle();
        IErrorService *pErrorService = nullptr;
        IFC(pCCoreServices->getErrorService(&pErrorService));
        IFCPTR(pErrorService);

        // Report error : 'DefaultStyleResourceUri not found'
        IFC(pErrorService->ReportGenericError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_RUNTIME_STYLE_DEFAULTSTYLERESOURCEURI_NOT_FOUND, 1, 0, 0, parameters, 3));

        // Trace error
        IError* pError = nullptr;
        xstring_ptr message;
        IFC(pErrorService->GetLastReportedError(&pError));
        IFC(pError->GetErrorMessage(&message));
        OutputDebugString(message.GetBuffer());

        // CXcpBrowserHost::ApplicationStartupEvent will swallow the failed hresult : fail-fast here
        FAIL_FAST_USING_EXISTING_ERROR_CONTEXT(E_NER_INVALID_OPERATION);
    }

    IFC(hr);

    //
    // If we have a platform metadata provider, and are being asked to resolve
    // a framework type by name, this means we're trying to obtain the style
    // for a platform-specific type. If that's the case, we need to make sure
    // to load the XBF, just as we do for built-in types...
    //
    bool fFrameworkNamespace = false;

    IFC(TypeNameHelper::IsFrameworkNamespace(
        wszNamespace,
        &fFrameworkNamespace));

    IFC(ResolveStyle(strTypeName, pStyles, ppStyle));

Cleanup:
    delete[] wszNamespace;
    delete[] szTraceString;
    ctl::release_interface(pStyles);

    RRETURN(hr);
}

_Check_return_ HRESULT DefaultStyles::ResolveStyle(
    _In_ const CClassInfo* pType,
    _In_opt_ ResourceDictionary* pStyles,
    _Outptr_result_maybenull_ Style** ppStyle)
{
    return ResolveStyle(pType->GetFullName(), pStyles, ppStyle);
}

_Check_return_ HRESULT DefaultStyles::ResolveStyle(
    _In_ const xstring_ptr& strTypeName,
    _In_opt_ ResourceDictionary* pStyles,
    _Outptr_result_maybenull_ Style** ppStyle)
{
    CValue valueFromCore;

    *ppStyle = nullptr;

    if (!pStyles)
    {
        return S_OK;
    }

    IFC_RETURN(pStyles->GetItem(
        strTypeName,
        false,
        true,
        &valueFromCore));

    if (valueFromCore.IsNull())
    {
        return S_OK;
    }

    KnownTypeIndex valueTypeIndex = CValueUtil::GetTypeIndex(valueFromCore);
    IFCEXPECT_RETURN(KnownTypeIndex::Style == valueTypeIndex);

    ctl::ComPtr<DependencyObject> styleAsDO;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(
        valueFromCore.AsObject(),
        valueTypeIndex,
        &styleAsDO));

    *ppStyle = static_cast<Style*>(styleAsDO.Detach());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   DefaultStyles::ResolveTypeName
//
//  Synopsis: Takes an IInspectable* (a value of the DefaultStyleKey property),
//            and resolves it to a type name.
//
//            The IInspectable can be in several supported forms.
//            For CLR, we support IReference<TypeName>. This corresponds to
//                this.DefaultStyleKey = typeof(MyControl);
//
//            For C++ and ABI, we support IPropertyValue of type string. This
//            corresponds to
//                this.DefaultStyleKey = "MyNamespace.MyControl";
//
//------------------------------------------------------------------------
_Check_return_ HRESULT DefaultStyles::ResolveTypeName(
    _In_ IInspectable* pValue,
    _Out_ xstring_ptr* pstrTypeName,
    _Out_ xstring_ptr* pstrAssemblyName,
    _Outptr_result_maybenull_ const CClassInfo** ppType)
{
    HRESULT hr = S_OK;
    wf::IReference<wxaml_interop::TypeName>* pTypeNameRef = NULL;
    wxaml_interop::TypeName typeName = {};
    wf::IPropertyValue* pPropValue = NULL;
    wrl_wrappers::HString strValue;

    pstrTypeName->Reset();
    pstrAssemblyName->Reset();
    *ppType = NULL;

    // IReference<TypeName>
    if (SUCCEEDED(ctl::do_query_interface(pTypeNameRef, pValue)))
    {
        xstring_ptr strAssemblyQualifiedName;

        IFC(pTypeNameRef->get_Value(&typeName));
        IFC(MetadataAPI::GetClassInfoByTypeName(typeName, ppType));
        IFC(xstring_ptr::CloneRuntimeStringHandle(typeName.Name, &strAssemblyQualifiedName));

        IFC(TypeNameHelper::ParseAssemblyQualifiedTypeName(strAssemblyQualifiedName, pstrTypeName, pstrAssemblyName));

        goto Cleanup;
    }

    // IPropertyValue
    if (SUCCEEDED(ctl::do_query_interface(pPropValue, pValue)))
    {
        wf::PropertyType propType;

        IFC(pPropValue->get_Type(&propType));

        if (wf::PropertyType_String == propType)
        {
            IFC(pPropValue->GetString(strValue.GetAddressOf()));

            IFC(xstring_ptr::CloneRuntimeStringHandle(strValue.Get(), pstrTypeName));
        }
    }

Cleanup:
    ReleaseInterface(pTypeNameRef);
    DELETE_STRING(typeName.Name);
    ReleaseInterface(pPropValue);

    RRETURN(hr);
}

_Check_return_ HRESULT DefaultStyles::GetDefaultStyleByKey(
    _In_ DependencyObject* pDO,
    _Outptr_result_maybenull_ Style** ppStyle)
{

    HRESULT hr = S_OK;
    xaml_controls::IControlProtected* pControl = NULL;
    IInspectable* pValue = NULL;
    xstring_ptr strTypeName;
    xstring_ptr strAssemblyName;
    const CClassInfo* pType = NULL;

    *ppStyle = NULL;

    // The GetDefaultStyleBy* methods below can go through GetFrameworkStyles() to load
    // theme resources. Just return if we're currently loading them so we don't reenter.
    if (DXamlCore::GetCurrent()->GetHandle()->IsLoadingGlobalThemeResources())
    {
        goto Cleanup;
    }

    pControl = ctl::query_interface<xaml_controls::IControlProtected>(pDO);
    if (!pControl)
    {
        goto Cleanup;
    }

    IFC(static_cast<Control*>(pControl)->GetCalculatedDefaultStyleKey(&pType, &pValue));
    if (!pType && !pValue)
    {
        goto Cleanup;
    }

    if (pType)
    {
        ASSERT(pType->IsBuiltinType());

        IFC(GetDefaultStyleByTypeInfo(pType, ppStyle));
    }
    else
    {
        IFC(ResolveTypeName(pValue, &strTypeName, &strAssemblyName, &pType));

        if (pType && pType->IsBuiltinType())
        {
            // Handle the case where a custom type is using a DefaultStyleKey of a known type that it subclasses.
            IFC(GetDefaultStyleByTypeInfo(pType, ppStyle));
        }
        else if (!strTypeName.IsNullOrEmpty())
        {
            // Handle default styles for custom types.
            Microsoft::WRL::ComPtr<wf::IUriRuntimeClass> pUri;
            IFC(static_cast<Control*>(pControl)->get_DefaultStyleResourceUri(pUri.GetAddressOf()));
            IFC(GetDefaultStyleByTypeName(strTypeName, strAssemblyName, pUri.Get(), ppStyle));
        }
    }

Cleanup:
    ReleaseInterface(pControl);
    ReleaseInterface(pValue);

    RRETURN(hr);
}
