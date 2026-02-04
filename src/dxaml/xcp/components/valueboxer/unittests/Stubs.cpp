// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "CDependencyObject.h"
#include <ThemeResource.h>
#include "TypeTableStructs.h"
#include "DependencyObjectAbstractionHelpers.h"
#include <WeakReferenceSourceNoThreadId.h>
#include "MetadataAPI.h"
#include "CornerRadius.h"
#include "StaticStore.h"
#include "Value.h"

class CThemeResourceExtension;
class CCoreServices;

// PAL

#ifdef XcpMarkStrongPointer
#undef XcpMarkStrongPointer
#endif

extern "C" void XcpMarkStrongPointer(_In_ void* thisPtr, _In_ void* ptrToStrongPtr)
{
}

#ifdef XcpMarkWeakPointer
#undef XcpMarkWeakPointer
#endif

extern "C" void XcpMarkWeakPointer(_In_ void* thisPtr, _In_ void* ptrToWeakPtr)
{
}

IPALDebuggingServices * __stdcall GetPALDebuggingServices()
{
    ASSERT(false);
    return nullptr;
}

// CDependencyObject

const CClassInfo* CDependencyObject::GetClassInformation() const
{
    return nullptr;
}

const CDependencyProperty* CDependencyObject::GetContentProperty()
{
    return nullptr;
}

bool CDependencyObject::OfTypeByIndex(_In_ KnownTypeIndex nIndex) const
{
    return DirectUI::MetadataAPI::IsAssignableFrom(nIndex, GetTypeIndex());
}

void CDependencyObject::AddRef()
{
    m_requiresThreadSafeAddRefRelease ? m_ref_count.ThreadSafeAddRef() : m_ref_count.AddRef();
}

void CDependencyObject::Release()
{
    if (m_requiresReleaseOverride)
    {
        ReleaseOverride();
    }

    m_requiresThreadSafeAddRefRelease ? m_ref_count.ThreadSafeRelease() : m_ref_count.Release();
}

// CClassInfo

_Check_return_ HRESULT CClassInfo::RunClassConstructorIfNecessary()
{
    return S_OK;
}

// CThemeResource

CThemeResource::CThemeResource(_In_ CThemeResourceExtension* pThemeResourceExtension)
{
}

// CCornerRadius

_Check_return_ HRESULT CCornerRadius::CornerRadiusFromString(
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ XCORNERRADIUS *peValue)
{
    return S_OK;
}

// pinvoke

namespace CoreImports
{

_Check_return_ HRESULT CoreServices_CombineResourceUri(
    _In_ CCoreServices *pCore,
    _In_ const xstring_ptr_view& strBaseUri,
    _In_ const xstring_ptr_view& strUri,
    _Out_ xruntime_string_ptr* pstrCombinedUri)
{
    return S_OK;
}

_Check_return_ HRESULT CoreServices_GetBaseUri(
    _In_ CCoreServices *pCore,
    _Out_ xstring_ptr* pstrBaseUri)
{
    return S_OK;
}

_Check_return_ HRESULT DependencyObject_GetTypeIndex(
    _In_ CDependencyObject* pObject,
    _Out_ KnownTypeIndex* piTypeIndex)
{
    return S_OK;
}
}

using namespace DirectUI;

// Reference

#define REFERENCE_ELEMENT_NAME_IMPL(TYPE, NAME) \
    template<> _Check_return_ HRESULT DirectUI::ReferenceBase<TYPE>::GetRuntimeClassNameImpl(_Out_ HSTRING* pClassName)\
    {\
        HRESULT hr = S_OK;\
        IFC(wrl_wrappers::HStringReference(L"Windows.Foundation.IReference`1<" NAME L">", SZ_COUNT(NAME) + 33).CopyTo(pClassName));\
    Cleanup:\
        RRETURN(hr);\
    }\

REFERENCE_ELEMENT_NAME_IMPL(xaml::CornerRadius, L"Microsoft.UI.Xaml.CornerRadius");
REFERENCE_ELEMENT_NAME_IMPL(xaml::Duration, L"Microsoft.UI.Xaml.Duration");
REFERENCE_ELEMENT_NAME_IMPL(wut::FontWeight, L"Windows.UI.Text.FontWeight");
REFERENCE_ELEMENT_NAME_IMPL(xaml::GridLength, L"Microsoft.UI.Xaml.GridLength");
REFERENCE_ELEMENT_NAME_IMPL(xaml::Thickness, L"Microsoft.UI.Xaml.Thickness");
REFERENCE_ELEMENT_NAME_IMPL(xaml_animation::KeyTime, L"Microsoft.UI.Xaml.Media.Animation.KeyTime");
REFERENCE_ELEMENT_NAME_IMPL(xaml_animation::RepeatBehavior, L"Microsoft.UI.Xaml.Media.Animation.RepeatBehavior");
REFERENCE_ELEMENT_NAME_IMPL(wu::Color, L"Windows.UI.Color");
REFERENCE_ELEMENT_NAME_IMPL(xaml_media::Matrix, L"Microsoft.UI.Xaml.Media.Matrix");
REFERENCE_ELEMENT_NAME_IMPL(xaml_media::Media3D::Matrix3D, L"Microsoft.UI.Xaml.Media.Media3D.Matrix3D");
REFERENCE_ELEMENT_NAME_IMPL(wxaml_interop::TypeName, L"Microsoft.UI.Xaml.Interop.TypeName");
REFERENCE_ELEMENT_NAME_IMPL(wf::DateTime, L"Windows.Foundation.DateTime");
REFERENCE_ELEMENT_NAME_IMPL(xaml_docs::TextRange, L"Microsoft.UI.Xaml.Documents.TextRange");

void DirectUI::ReferenceDetails::ReferenceTraits<wxaml_interop::TypeName>::Destroy(wxaml_interop::TypeName& member)
{
    ::WindowsDeleteString(member.Name);
}

STDMETHODIMP DirectUI::ReferenceDetails::ReferenceTraits<wxaml_interop::TypeName>::Set(
    wxaml_interop::TypeName& member,
    const wxaml_interop::TypeName& param
)
{
    ::WindowsDeleteString(member.Name);

    member.Kind = param.Kind;
    return WindowsDuplicateString(param.Name, &member.Name);
}

STDMETHODIMP DirectUI::ReferenceDetails::ReferenceTraits<wxaml_interop::TypeName>::Get(
    const wxaml_interop::TypeName& member,
    wxaml_interop::TypeName* param)
{
    param->Kind = member.Kind;
    return WindowsDuplicateString(member.Name, &param->Name);
}

// DependencyObject

namespace DirectUI
{
    class DependencyObject
        : public xaml::IDependencyObject
        , public xaml::ISourceInfoPrivate
        , public ctl::WeakReferenceSourceNoThreadId
    {
    public:
        CDependencyObject* GetHandle() const;
    };
}

CDependencyObject* DependencyObject::GetHandle() const
{
    return nullptr;
}

// ActivationAPI

namespace DirectUI
{
    class ActivationAPI
    {
    public:
        static _Check_return_ HRESULT ActivateInstanceFromString(_In_ const CClassInfo* pType, _In_ const xstring_ptr_view& strValue, _Outptr_ IInspectable** ppInstance);
    };
}

_Check_return_ HRESULT ActivationAPI::ActivateInstanceFromString(_In_ const CClassInfo* pType, _In_ const xstring_ptr_view& strValue, _Outptr_ IInspectable** ppInstance)
{
    return S_OK;
}

// DXamlServices

namespace DirectUI
{
    interface IDXamlCore;
    interface IPeerTableHost;

    namespace DXamlServices
    {
        __maybenull IDXamlCore* GetDXamlCore()
        {
            return nullptr;
        }

        __maybenull IPeerTableHost* GetPeerTableHost()
        {
            return nullptr;
        }

        bool IsDXamlCoreInitializing()
        {
            return false;
        }

        bool IsDXamlCoreInitialized()
        {
            return true;
        }

        bool IsDXamlCoreShutdown()
        {
            return false;
        }

        CCoreServices* GetHandle()
        {
            return nullptr;
        }

        HRESULT ActivatePeer(_In_ KnownTypeIndex nTypeIndex, _Out_ DirectUI::DependencyObject** ppObject)
        {
            return S_OK;
        }

        HRESULT GetPeer(_In_ CDependencyObject* pDO, _Out_ DirectUI::DependencyObject** ppObject)
        {
            return S_OK;
        }
    }

    CCoreServices* GetCoreForCurrentThread()
    {
        return nullptr;
    }
}

// DependencyObjectAbstractionHelpers

DependencyObject* DependencyObjectAbstractionHelpers::IDOtoDO(
    _In_ xaml::IDependencyObject* ido)
{
    return static_cast<DependencyObject*>(ido);
}

DependencyObject* DependencyObjectAbstractionHelpers::IRTItoDO(
    _In_ xaml_hosting::IReferenceTrackerInternal* irti)
{
    auto ptr = ctl::impl_cast<ctl::WeakReferenceSourceNoThreadId>(irti);
    return static_cast<DependencyObject*>(ptr);
}

xaml::IDependencyObject* DependencyObjectAbstractionHelpers::IRTItoIDO(
    _In_ xaml_hosting::IReferenceTrackerInternal* irti)
{
    return IRTItoDO(irti);
}

xaml_hosting::IReferenceTrackerInternal* DependencyObjectAbstractionHelpers::DOtoIRTI(
    _In_ DependencyObject* obj)
{
    return ctl::interface_cast<xaml_hosting::IReferenceTrackerInternal>(obj);
}

::ctl::WeakReferenceSourceNoThreadId* DependencyObjectAbstractionHelpers::DOtoWRSNTI(
    _In_ DependencyObject* obj)
{
    return static_cast<::ctl::WeakReferenceSourceNoThreadId*>(obj);
}

::ctl::WeakReferenceSourceNoThreadId* DependencyObjectAbstractionHelpers::IDOtoWRSNTI(
    _In_ xaml::IDependencyObject* ido)
{
    return static_cast<DependencyObject*>(ido);
}

CDependencyObject* DependencyObjectAbstractionHelpers::GetHandle(_In_ DependencyObject* obj)
{
    return nullptr;
}

// StaticStore

xref_ptr<StaticStore> StaticStore::GetInstance()
{
    return nullptr;
}

unsigned int StaticStore::Release()
{
    return 0;
}

// Enums.g.h

_Check_return_ HRESULT GetEnumValueFromKnownWinRTBox(_In_ IInspectable* pBox, _In_ const CClassInfo* pType, _Out_ UINT* pnValue)
{
    return S_OK;
}

_Check_return_ HRESULT GetKnownWinRTBoxFromEnumValue(_In_ UINT nValue, _In_ const CClassInfo* pType, _Outptr_ IInspectable** ppBox)
{
    return S_OK;
}

// ExternalObjectReference

namespace DirectUI
{
    class ExternalObjectReference
    {
    public:
        static _Check_return_ HRESULT ConditionalWrap(_In_ IInspectable *pInValue, _Outptr_ DependencyObject **ppWrapped, _Out_opt_ BOOLEAN *pWasWrapped);
    };
}

_Check_return_ HRESULT ExternalObjectReference::ConditionalWrap(_In_ IInspectable *pInValue, _Outptr_ DependencyObject **ppWrapped, _Out_opt_ BOOLEAN *pWasWrapped)
{
    return S_OK;
}

// ThemeResourceExpression

namespace DirectUI
{
    class ThemeResourceExpression
    {
    public:
        static _Check_return_ HRESULT Create(_In_ CThemeResource* pCoreThemeResource, _Out_ ThemeResourceExpression** ppExpression);
    };
}

_Check_return_ HRESULT ThemeResourceExpression::Create(_In_ CThemeResource* pCoreThemeResource, _Out_ ThemeResourceExpression** ppExpression)
{
    *ppExpression = nullptr;
    return S_OK;
}

// PropertyPath

namespace DirectUI
{
    class PropertyPath
    {
    public:
        static _Check_return_ HRESULT CreateInstance(_In_ HSTRING, DirectUI::PropertyPath **out);
    };

    class FontFamily
    {
    public:
        _Check_return_ HRESULT put_Source(_In_opt_ HSTRING value);
    };
}

_Check_return_ HRESULT PropertyPath::CreateInstance(_In_ HSTRING, DirectUI::PropertyPath **out)
{
    return S_OK;
}

_Check_return_ HRESULT FontFamily::put_Source(_In_opt_ HSTRING value)
{
    ASSERT(false);
    return E_FAIL;
}