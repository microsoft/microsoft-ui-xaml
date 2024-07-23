// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "TypeTableStructs.h"
#include "TypeTable.g.h"
#include "xcptypes.h"
#include "MetadataAPI.h"
#include "NodeStreamCache.h"
#include <FrameworkTheming.h>
#include <Template.h>
#include <XamlParser.h>
#include <ImageTaskDispatcher.h>
#include <FlyweightCoreAdapter.h>
#include <CDOSharedState.h>
#include <VisualTree.h>

class CD3D11Device;

//
// CCoreServices Stub Implementation for XamlParser
//
//   Implementation for populating known types and querying
//   them are the only implementation stubs present.
//
//   Calls to other CoreServices will ASSERT/Fail.
//
// TODO: Consider refactoring core services for type information
//       into a separate class which can be shared.
//
//       Also consider limiting CCoreServices usage in Parser
//       to an IXamlParserHostServices.
//


class DummySmartPointer
{
public:
    void AddRef() { }
    void Release() { }
};

class CoreWindowRootScale {};

AutomationEventsHelper::PeggedAutomationPeer::~PeggedAutomationPeer(void) {}

class CCoreServices::FontDownloadListener {}; // Pacify linker with this stub.

CCoreServices::CCoreServices()
    : m_cRef(1)
    , m_bIsDestroyingCoreServices(false)
    , m_flyweightState(
        CDOSharedState(
            this,
            CDOSharedState::s_defaultRenderChangedHandler,
            CDOSharedState::s_defaultBaseUri,
            xref::weakref_ptr<VisualTree>()))
    , m_maxTextureSizeProvider(this)
    , m_atlasRequestProvider(this)
    , m_contentRootCoordinator(*this)
{}

CCoreServices::~CCoreServices() noexcept
{
    m_bIsDestroyingCoreServices = TRUE;
}

XUINT32 CCoreServices::Release(void)
{
    XUINT32 cRef = --m_cRef;

    if (0 == cRef)
    {
        delete this;
    }

    return cRef;
}

XUINT32 __thiscall CCoreServices::AddRef(void)
{
    return  ++m_cRef;
}

_Check_return_
HRESULT CCoreServices::GetXamlNodeStreamCacheManager(_Out_ std::shared_ptr<XamlNodeStreamCacheManager>& spXamlNodeStreamCacheManager)
{
    HRESULT hr = S_OK;

    if (!m_spXamlNodeStreamCacheManager)
    {
        IFC(XamlNodeStreamCacheManager::Create(this, m_spXamlNodeStreamCacheManager));
    }

    spXamlNodeStreamCacheManager = m_spXamlNodeStreamCacheManager;

Cleanup:
    return S_OK;
}

std::shared_ptr<XamlSchemaContext> CCoreServices::GetSchemaContext()
{
    return m_spXamlSchemaContext;
}

_Check_return_
HRESULT
CCoreServices::CreateErrorServiceForXbfGenerator()
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::CreateErrorService(_Outptr_ struct IErrorService * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::CheckUri(_In_ const xstring_ptr&, _In_ unsigned int)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::UnsecureDownloadFromSite(
    _In_ const xstring_ptr&,
    _In_opt_ struct IPALUri *,
    _In_ struct IPALDownloadResponseCallback *,
    _In_ unsigned int,
    _Outptr_opt_ struct IPALAbortableOperation**,
    _In_opt_ struct IPALUri *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::RegisterScriptCallback(_In_ void *,_In_ EVENTPFN, _In_ EVENTPFN)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

void CCoreServices::ForceGCCollect(void)
{
    ASSERT(FALSE);
}

_Check_return_ HRESULT
CCoreServices::GetDefaultInheritedPropertyValue(_In_ KnownPropertyIndex nUserIndex, _Out_ CValue* pValue)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_ HRESULT
CCoreServices::GetDefaultFocusVisualSolidColorBrush(
    _In_ bool forFocusVisualSecondaryBrush,
    _In_ XUINT32 color,
    _Outptr_ CSolidColorBrush** ppSolidColorBrush)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::GetDefaultTextBrush(_Outptr_ class CBrush * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::GetTextSelectionGripperBrush(_Outptr_ class CBrush * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::GetSystemTextSelectionBackgroundBrush(_Outptr_ class CSolidColorBrush * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::GetSystemTextSelectionForegroundBrush(_Outptr_ class CSolidColorBrush * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::GetTransparentBrush(_Outptr_ class CBrush * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
xref_ptr<CDependencyObject> CCoreServices::TryGetElementByName(
    _In_ const xstring_ptr_view&,
    _In_ CDependencyObject*)
{
    ASSERT(FALSE);
    return nullptr;
}

_Check_return_
HRESULT CCoreServices::CreateObject(
    _In_ const class CClassInfo *,
    _In_ const xstring_ptr&,
    _Outptr_result_maybenull_ CDependencyObject * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::CreateObject(
    _In_ const class CClassInfo *,
    _In_ CDependencyObject *,
    _Outptr_result_maybenull_ CDependencyObject * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

bool CCoreServices::HasActiveAnimations(void)
{
    ASSERT(FALSE);
    return false;
}

_Check_return_
HRESULT CCoreServices::LoadXaml(
    _In_ unsigned int cBuffer,
    _In_reads_opt_(cBuffer) const unsigned char *,
    _Outptr_result_maybenull_ CDependencyObject * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::GetResourceManager(_Outptr_ struct IPALResourceManager * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::ParseXaml(
    _In_ const Parser::XamlBuffer&,
    _In_ bool,
    _In_ bool,
    _In_ bool,
    _Outptr_ CDependencyObject**,
    _In_ const xstring_ptr_view&,
    _In_ bool,
    _In_ const xstring_ptr_view&)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::ParseXamlWithEventRoot(
    _In_ const Parser::XamlBuffer&,
    _In_ bool,
    _In_ bool,
    _In_ bool,
    _Outptr_ CDependencyObject**,
    _In_opt_ IPALUri*,
    _In_ const xstring_ptr_view&,
    _In_ bool)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::ParseXamlWithExistingFrameworkRoot(
    _In_ const Parser::XamlBuffer&,
    _In_ CDependencyObject*,
    _In_ const xstring_ptr_view&,
    _In_ const xstring_ptr_view&,
    _In_ const bool,
    _Outptr_ CDependencyObject**)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::ResetCoreWindowVisualTree(void)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::putVisualRoot(_In_opt_ CDependencyObject *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
CDependencyObject* CCoreServices::getVisualRoot(void)
{
    ASSERT(FALSE);
    return NULL;
}

_Check_return_
CDependencyObject* CCoreServices::getRootScrollViewer(void)
{
    ASSERT(FALSE);
    return NULL;
}

_Check_return_
HRESULT CCoreServices::getErrorService(_Out_ IErrorService **ppErrorService)
{
    HRESULT hr = S_OK;

    if (ppErrorService == NULL)
    {
        IFC(E_INVALIDARG);
    }

    *ppErrorService = NULL;

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT CCoreServices::GetParserErrorService(_Out_ IErrorService **ppErrorService)
{
    RRETURN(getErrorService(ppErrorService));
}

_Check_return_
HRESULT CCoreServices::ReportUnhandledError(long)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::NWDrawMainTree(
    _In_ CWindowRenderTarget*,
    bool,
    _Out_ bool*)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::Tick(
    bool,
    _Out_opt_ bool*,
    _Out_opt_ bool*)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::CallPerFrameCallback(float)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::RaiseQueuedSurfaceContentsLostEvent(void)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::AddSurfaceImageSource(class CSurfaceImageSource *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::RemoveSurfaceImageSource(class CSurfaceImageSource *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::AddVirtualSurfaceImageSource(class CVirtualSurfaceImageSource *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::RemoveVirtualSurfaceImageSource(class CVirtualSurfaceImageSource *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::VirtualSurfaceImageSourcePerFrameWork(_In_ struct XRECTF_RB const *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::HitTest(
    struct XPOINTF,
     _In_opt_ CDependencyObject *,
    _Outptr_result_maybenull_ CDependencyObject**,
    bool)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::GetTextElementBoundingRect(
    _In_ CDependencyObject *,
    _Out_ struct XRECTF_WH *,
    _In_ bool)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::HitTestLinkFromTextControl(
    _In_ struct XPOINTF,
    _In_ CDependencyObject *,
    _Outptr_result_maybenull_ CDependencyObject * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::ProcessInput(
    _In_ struct InputMessage *,
    _In_opt_ CContentRoot*,
    _Out_ int *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::ProcessTouchInteractionCallback(_In_ const xref_ptr<CUIElement> &, _In_ TouchInteractionMsg *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

void CCoreServices::RegisterHostSite(_In_ struct IXcpHostSite *)
{
    ASSERT(FALSE);
}

void CCoreServices::UnregisterHostSite(void)
{
    ASSERT(FALSE);
}

_Check_return_
HRESULT CCoreServices::RegisterBrowserHost(_In_ struct IXcpBrowserHost *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::UnregisterBrowserHost(void)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::RegisterDownloadSite(_In_ struct ICoreServicesSite *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

void CCoreServices::UnregisterDownloadSite(void)
{
    ASSERT(FALSE);
}

_Check_return_
HRESULT CCoreServices::GetSystemGlyphTypefaces(_Outptr_ CDependencyObject * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::CLR_FireEvent(
    _In_ CDependencyObject *,
    _In_ EventHandle,
    _In_ CDependencyObject *,
    _In_ CEventArgs *,
    _In_ unsigned int)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::ShouldFireEvent(
    _In_ CDependencyObject *,
    _In_ EventHandle,
    _In_ CDependencyObject *,
    _In_ CEventArgs *,
    _In_ int,
    _Out_ int *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::ApplicationStartupEventComplete(void)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

xstringmap<bool>&
CCoreServices::GetResourceDictionaryUriCache()
{
    return m_resourceDictionaryUriCache;
}

_Check_return_
HRESULT CCoreServices::SetCurrentApplication(_In_ class CApplication *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Ret_maybenull_
CWindowRenderTarget *CCoreServices::NWGetWindowRenderTarget(void)
{
    ASSERT(FALSE);
    return NULL;
}

_Check_return_
HRESULT CCoreServices::CreateMediaQueue(_In_ struct IMediaQueueClient *,struct IMediaQueue * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::CreateCompositorScheduler(_In_ class WindowsGraphicsDeviceManager *, _Outptr_ class CompositorScheduler * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

void CCoreServices::CreateUIThreadScheduler(
    _In_ struct IXcpDispatcher *,
    _In_ class CompositorScheduler *,
    _Outptr_ struct ITickableFrameScheduler * *)
{
    ASSERT(FALSE);
}

_Check_return_
HRESULT CCoreServices::CreateWindowRenderTarget(
    _In_ class WindowsGraphicsDeviceManager *,
    _In_ class CompositorScheduler *,
    _In_ class WindowsPresentTarget *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::ExecuteOnUIThread(_In_ struct IPALExecuteOnUIThread *, const ReentrancyBehavior)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

IAsyncImageFactory* CCoreServices::GetImageFactory()
{
    ASSERT(FALSE);
    return nullptr;
}

_Check_return_
HRESULT CCoreServices::SetWindowVisibility(bool, bool, bool)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::DetermineDeviceLost(_Out_opt_ bool *pIsDeviceLost)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

IPALUri* CCoreServices::GetBaseUriNoRef()
{
    ASSERT(FALSE);
    return NULL;
}

_Check_return_
HRESULT CCoreServices::ConfigureNumberSubstitution()
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

__override _Check_return_ HRESULT CCoreServices::SetLayoutCompletedNeeded(const LayoutCompletedNeededReason reason)
{
    return S_OK;
}

VisualTree* CCoreServices::GetMainVisualTree()
{
    return nullptr;
}

_Check_return_ HRESULT
CCoreServices::OnDebugSettingsChanged()
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

DOSourceFileInformation::DOSourceFileInformation()
{
    ASSERT(FALSE);
}

DOSourceFileInformation::~DOSourceFileInformation()
{
    ASSERT(FALSE);
}

DOSourceFileInformation::DOSourceFileInformation(
    _In_ unsigned int uiLineNumber,
    _In_ unsigned int uiLinePosition,
    _In_ const xstring_ptr& spStrFilePath)
{
    ASSERT(FALSE);
}

void DOSourceFileInformation::AddPropertySource(
    _In_ KnownPropertyIndex dPropertyIndex,
    _In_ unsigned int uiLineNumber,
    _In_ unsigned int uiLinePosition,
    _In_ const xstring_ptr& spStrFilePath)
{
    ASSERT(FALSE);
}

HRESULT DOSourceFileInformation::GetPropertySource(
    _In_ KnownPropertyIndex dPropertyIndex,
    _Out_ SourceFileInformation* pPropertySource)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

// Will only be enabled if the key is found in registry
SourceFileInformationLookup::SourceFileInformationLookup()
{
    ASSERT(FALSE);
}

SourceFileInformationLookup::~SourceFileInformationLookup()
{
    ASSERT(FALSE);
}

void SourceFileInformationLookup::InsertObjectSourceInformation(
    _In_ CDependencyObject* pDependencyObject,
    _In_ const xstring_ptr& spStrFilePath,
    _In_ unsigned int uiLineNumber,
    _In_ unsigned int uiLinePosition)
{
    ASSERT(FALSE);
}

void SourceFileInformationLookup::InsertPropertySourceInformation(
    _In_ CDependencyObject* pDependencyObject,
    _In_ KnownPropertyIndex uiIndex,
    _In_ const xstring_ptr& spStrFilePath,
    _In_ unsigned int uiLineNumber,
    _In_ unsigned int uiLinePosition)
{
    ASSERT(FALSE);
}

HRESULT SourceFileInformationLookup::GetObjectSourceInformation(
    _In_ CDependencyObject* pDependencyObject,
    _Out_ SourceFileInformation* pSource)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

HRESULT SourceFileInformationLookup::GetPropertySourceInformation(
    _In_ CDependencyObject* pDependencyObject,
    _In_ KnownPropertyIndex uiIndex,
    _Out_ SourceFileInformation* pSource)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

XUINT64 SourceFileInformationLookup::GetCPUTicks()
{
    ASSERT(FALSE);
    return 0;
}

HRESULT SourceFileInformationLookup::LogObjectCreationStart()
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

HRESULT SourceFileInformationLookup::LogObjectCreationEnd(
    _In_ CDependencyObject* pDependencyObject)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

uint32_t CCoreServices::GetMaxTextureSize()
{
    ASSERT(FALSE);
    return 0;
}

MaxTextureSizeProvider& CCoreServices::GetMaxTextureSizeProvider()
{
    return m_maxTextureSizeProvider;
}

MaxTextureSizeProvider::MaxTextureSizeProvider(_In_ CCoreServices* core)
{
}

AtlasRequestProvider::AtlasRequestProvider(_In_ CCoreServices* core)
{
}

_Check_return_
HRESULT CParser::LoadXaml(
    _In_ CCoreServices*,
    _In_ const CParserSettings&,
    _In_ const Parser::XamlBuffer&,
    _Outptr_ CDependencyObject**,
    _In_ const xstring_ptr_view&,
    _In_ const std::array<byte, Parser::c_xbfHashSize>&
    )
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Ret_maybenull_
CD3D11Device* CCoreServices::GetGraphicsDevice()
{
    return nullptr;
}

ABI::Windows::Foundation::Size CCoreServices::GetContentRootMaxSize()
{
    return ABI::Windows::Foundation::Size{ 0, 0 };
}