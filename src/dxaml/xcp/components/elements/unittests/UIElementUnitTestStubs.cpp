// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <corep.h>
#include <set>

class DummySmartPointer
{
public:
    void AddRef() { }
    void Release() { }
};

class CoreWindowRootScale {};
class ThemeWalkResourceCache {};
class FrameworkTheming {};
class CDeferredMapping {};
class CCoreServices::FontDownloadListener {};
struct IDWriteFontDownloadQueue : public DummySmartPointer {};
class ImageTaskDispatcher : public DummySmartPointer {};
class CDisplayMemberTemplate : public DummySmartPointer {};
struct AccessKeys::AccessKeyExportImpl {};
class Jupiter::NameScoping::NameScopeTable {};

AutomationEventsHelper::PeggedAutomationPeer::~PeggedAutomationPeer(void) {}
AccessKeys::AccessKeyExport::~AccessKeyExport(void) {}
Jupiter::NameScoping::NameScopeRoot::~NameScopeRoot(void) {}
Jupiter::NameScoping::NameScopeRoot::NameScopeRoot(void) {}
MaxTextureSizeProvider::MaxTextureSizeProvider(_In_ class CCoreServices *) {}
AtlasRequestProvider::AtlasRequestProvider(_In_ class CCoreServices *) {}

CCoreServices::CCoreServices()
: m_flyweightState(
        CDOSharedState(
            this,
            CDOSharedState::s_defaultRenderChangedHandler,
            CDOSharedState::s_defaultBaseUri,
            xref::weakref_ptr<VisualTree>()))
, m_maxTextureSizeProvider(this)
, m_atlasRequestProvider(this)
, m_facadeStorage()
, m_sparseTables()
, m_changeHandlersTables()
, m_contentRootCoordinator(*this)
{
    m_nThreadID = GetCurrentThreadId();
}

CCoreServices::~CCoreServices() {}

unsigned int CCoreServices::AddRef(void) { return 1; }
unsigned int CCoreServices::Release(void) { return 1; }

_Check_return_ long CCoreServices::GetResourceManager(_Outptr_ struct IPALResourceManager * *) { return 0; }
_Check_return_ long CCoreServices::CreateMediaQueue(_In_ struct IMediaQueueClient *,struct IMediaQueue * *) { return 0; }
_Check_return_ long CCoreServices::GetXamlNodeStreamCacheManager(_Out_ class std::shared_ptr<class XamlNodeStreamCacheManager> &) { return 0; }
_Check_return_ long CCoreServices::GetParserErrorService(_Out_ struct IErrorService * *) { return 0; }

XUINT32 CInputServices::Release(void) { return 1; }


