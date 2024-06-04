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
HRESULT CCoreServices::CreateErrorService(struct IErrorService * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::CheckUri(_In_ const xstring_ptr&,unsigned int)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::UnsecureDownloadFromSite(_In_ const xstring_ptr&,struct IPALUri *,struct IPALDownloadResponseCallback *,unsigned int,struct IPALAbortableOperation * *,struct IPALUri *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::RegisterScriptCallback(void *,EVENTPFN, EVENTPFN)
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
HRESULT CCoreServices::GetDefaultTextBrush(class CBrush * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::GetTextSelectionGripperBrush(class CBrush * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::GetSystemTextSelectionBackgroundBrush(class CSolidColorBrush * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::GetSystemTextSelectionForegroundBrush(class CSolidColorBrush * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::GetTransparentBrush(class CBrush * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
xref_ptr<CDependencyObject> CCoreServices::TryGetElementByName(_In_ const xstring_ptr_view&, CDependencyObject*)
{
    ASSERT(FALSE);
    return nullptr;
}

_Check_return_
HRESULT CCoreServices::CreateObject(const class CClassInfo *,_In_ const xstring_ptr&, CDependencyObject * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::CreateObject(const class CClassInfo *, CDependencyObject *, CDependencyObject * *)
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
HRESULT CCoreServices::LoadXaml(unsigned int,const unsigned char *,CDependencyObject * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::GetResourceManager(struct IPALResourceManager * *)
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
HRESULT CCoreServices::putVisualRoot(CDependencyObject *)
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
HRESULT CCoreServices::NWDrawMainTree(class CWindowRenderTarget *,bool,bool *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::Tick(bool, bool*, bool*)
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
HRESULT CCoreServices::VirtualSurfaceImageSourcePerFrameWork(struct XRECTF_RB const *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::HitTest(struct XPOINTF,CDependencyObject *,CDependencyObject * *,bool)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::GetTextElementBoundingRect(CDependencyObject *,struct XRECTF_WH *, bool)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::HitTestLinkFromTextControl(struct XPOINTF,CDependencyObject *,CDependencyObject * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::ProcessInput(struct InputMessage *,CContentRoot*,int *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::ProcessTouchInteractionCallback(const xref_ptr<CUIElement> &, TouchInteractionMsg *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

void CCoreServices::RegisterHostSite(struct IXcpHostSite *)
{
    ASSERT(FALSE);
}

void CCoreServices::UnregisterHostSite(void)
{
    ASSERT(FALSE);
}

_Check_return_
HRESULT CCoreServices::RegisterBrowserHost(struct IXcpBrowserHost *)
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
HRESULT CCoreServices::RegisterDownloadSite(struct ICoreServicesSite *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

void CCoreServices::UnregisterDownloadSite(void)
{
    ASSERT(FALSE);
}

_Check_return_
HRESULT CCoreServices::GetSystemGlyphTypefaces(CDependencyObject * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::CLR_FireEvent(CDependencyObject *,EventHandle,CDependencyObject *,CEventArgs *,unsigned int)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::ShouldFireEvent(CDependencyObject *,EventHandle,CDependencyObject *,CEventArgs *,int,int *)
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
HRESULT CCoreServices::SetCurrentApplication(class CApplication *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

CWindowRenderTarget *CCoreServices::NWGetWindowRenderTarget(void)
{
    ASSERT(FALSE);
    return NULL;
}

_Check_return_
HRESULT CCoreServices::CreateMediaQueue(struct IMediaQueueClient *,struct IMediaQueue * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::CreateCompositorScheduler(class WindowsGraphicsDeviceManager *,class CompositorScheduler * *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

void CCoreServices::CreateUIThreadScheduler(struct IXcpDispatcher *,class CompositorScheduler *,struct ITickableFrameScheduler * *)
{
    ASSERT(FALSE);
}

_Check_return_
HRESULT CCoreServices::CreateWindowRenderTarget(class WindowsGraphicsDeviceManager *,class CompositorScheduler *,class WindowsPresentTarget *)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT CCoreServices::ExecuteOnUIThread(struct IPALExecuteOnUIThread *, const ReentrancyBehavior)
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

CD3D11Device* CCoreServices::GetGraphicsDevice()
{
    return nullptr;
}

ABI::Windows::Foundation::Size CCoreServices::GetContentRootMaxSize()
{
    return ABI::Windows::Foundation::Size{ 0, 0 };
}