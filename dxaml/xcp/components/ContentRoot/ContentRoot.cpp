// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <corep.h>
#include "InputServices.h"

#include "ContentRoot.h"

#include <uielement.h>

#include <DXamlServices.h>
#include <InputServices.h>
#include <focusmgr.h>
#include <visualtree.h>
#include <QualifierContext.h>

#include <akexport.h>

#include "CKeyboardAcceleratorCollection.h"

#include "XamlIslandRoot.h"

#include "FocusManagerCoreWindowAdapter.h"
#include "FocusManagerXamlIslandAdapter.h"

#include "FocusObserver.h"
#include "CoreWindowFocusObserver.h"
#include <XamlOneCoreTransforms.h>

#include <host.h>
#include <corewindow.h> // ICoreWindowComponentInterop
#include "RootScale.h"
#include <WRLHelper.h>
#include <XamlIslandRootScale.h>
#include <FxCallbacks.h>

CContentRoot::CContentRoot(_In_ CContentRoot::Type type, _In_ XUINT32 backgroundColor, _In_opt_ CUIElement* rootElement, _In_ CCoreServices& coreServices)
    : m_coreServices(coreServices)
    , m_type(type)
    , m_akExport(&coreServices)
    , m_visualTree(&m_coreServices, backgroundColor, rootElement, *this)
    , m_inputManager(m_coreServices, *this)
    , m_contentRootEventListener(*this)
    , m_focusManager(&m_coreServices, *this)
{
    m_akExport.SetVisualTree(&m_visualTree);
    m_akExport.SetFocusManager(&m_focusManager);

    switch (m_type)
    {
        case CContentRoot::Type::CoreWindow:
        {
            FAIL_FAST_ASSERT(m_coreServices.GetContentRootCoordinator()->m_unsafe_IslandsIncompatible_CoreWindowContentRoot == nullptr); // There should only be one of this per thread
            m_coreServices.GetContentRootCoordinator()->m_unsafe_IslandsIncompatible_CoreWindowContentRoot = this;

            m_focusAdapter = std::make_unique<ContentRootAdapters::FocusManagerCoreWindowAdapter>(*this);
            m_focusManager.SetFocusObserver(std::make_unique<CoreWindowFocusObserver>(&coreServices, this));

            break;
        }
        case CContentRoot::Type::XamlIslandRoot:
        {
            m_focusAdapter = std::make_unique<ContentRootAdapters::FocusManagerXamlIslandAdapter>(*this);
            m_focusManager.SetFocusObserver(std::make_unique<FocusObserver>(&coreServices, this));
            break;
        }
    }
}

CContentRoot::~CContentRoot()
{
    VERIFYHR(Close());

    if (m_type == CContentRoot::Type::CoreWindow)
    {
        m_coreServices.GetContentRootCoordinator()->m_unsafe_IslandsIncompatible_CoreWindowContentRoot = nullptr;
    }

    m_ref_count.end_destroying();
}

CXamlIslandRoot* CContentRoot::GetXamlIslandRootNoRef() const
{
    return m_xamlIslandRoot.get();
}

void CContentRoot::SetXamlIslandRoot(_In_ CXamlIslandRoot* xamlIslandRoot)
{
    m_xamlIslandRoot = xamlIslandRoot;
}

void CContentRoot::SetXamlIslandType(_In_ CContentRoot::IslandType islandType)
{
    if (m_type != Type::XamlIslandRoot)
    {
        XAML_FAIL_FAST();
    }

    m_islandType = islandType;
}

_Check_return_ HRESULT CContentRoot::OnStateChanged()
{
    switch(m_type)
    {
    case Type::XamlIslandRoot:
        m_xamlIslandRoot->OnStateChanged();
        break;
    case Type::CoreWindow:
        IFC_RETURN(FxCallbacks::DxamlCore_OnCompositionContentStateChangedForUWP());
        break;
    }

    return S_OK;
}

_Check_return_ HRESULT CContentRoot::OnAutomationProviderRequested(
    ixp::IContentIsland* content,
    ixp::IContentIslandAutomationProviderRequestedEventArgs* args)
{
    if (m_type == Type::XamlIslandRoot)
    {
        IFC_RETURN(m_xamlIslandRoot->OnContentAutomationProviderRequested(content, args));
    }

    return S_OK;
}

_Check_return_ HRESULT CContentRoot::RegisterCompositionContent(_In_ ixp::IContentIsland* compositionContent)
{
    m_compositionContent = compositionContent;

    IFC_RETURN(m_compositionContent->add_StateChanged(WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<
        ixp::ContentIsland*,
        ixp::ContentIslandStateChangedEventArgs*>>([&](
            ixp::IContentIsland* content,
            ixp::IContentIslandStateChangedEventArgs* args) -> HRESULT
            {
                IFC_RETURN(OnStateChanged());

                return S_OK;
            }).Get(),
            &m_compositionContentStateChangedToken));

    // Accessibility
    IFC_RETURN(m_compositionContent->add_AutomationProviderRequested(WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<
        ixp::ContentIsland*,
        ixp::ContentIslandAutomationProviderRequestedEventArgs*>>([&](
            ixp::IContentIsland* content,
            ixp::IContentIslandAutomationProviderRequestedEventArgs* args) -> HRESULT
            {
                IFC_RETURN(OnAutomationProviderRequested(content, args));

                return S_OK;
            }).Get(),
            &m_automationProviderRequestedToken));

    return S_OK;
}

_Check_return_ HRESULT CContentRoot::ResetCompositionContent()
{
    if (m_compositionContent)
    {
        if (m_compositionContentStateChangedToken.value != 0)
        {
            IFC_RETURN(m_compositionContent->remove_StateChanged(m_compositionContentStateChangedToken));
            m_compositionContentStateChangedToken = { };
        }

        if (m_automationProviderRequestedToken.value != 0)
        {
            IFC_RETURN(m_compositionContent->remove_AutomationProviderRequested(m_automationProviderRequestedToken));
            m_automationProviderRequestedToken = { };
        }

        m_compositionContent.Reset();
    }

    return S_OK;
}

_Check_return_ HRESULT CContentRoot::SetContentIsland(_In_ ixp::IContentIsland* compositionContent)
{
    // ContentRoot is re-used through the life time of an application. Hence, CompositionContent can be set multiple time.
    // ResetCompositionContent will make sure to remove handlers and reset previous CompositionContent.
    IFC_RETURN(ResetCompositionContent());
    IFC_RETURN(RegisterCompositionContent(compositionContent));

    if (const auto rootScale = RootScale::GetRootScaleForContentRoot(this)) // Check that we still have an active tree
    {
        rootScale->SetContentIsland(compositionContent);
    }

    return S_OK;
}

bool CContentRoot::ShouldUseVisualRelativePixels()
{
    if (XamlOneCoreTransforms::IsEnabled())
    {
        return true;
    }

    return false;
}

// CONTENT-TODO: Lifted IXP doesn't support OneCoreTransforms UIA yet.
#if false
UINT64 CContentRoot::GetVisualIdentifier()
{
    UINT64 visualIdentifier = 0;

    if (m_type == Type::XamlIslandRoot)
    {
        if (m_xamlIslandRoot)
        {
            visualIdentifier = m_xamlIslandRoot->GetVisualIdentifier();
        }
    }
    else
    {
        visualIdentifier = m_coreServices.GetCoreWindowCompositionIslandId();
    }

    return visualIdentifier;
}
#endif

CInputManager& CContentRoot::GetInputManager()
{
    return m_inputManager;
}

ContentRootAdapters::FocusAdapter& CContentRoot::GetFocusAdapter()
{
    return *m_focusAdapter.get();
}

XUINT32 CContentRoot::AddRef()
{
    return m_ref_count.AddRef();
}

XUINT32 CContentRoot::Release()
{
    auto refCount = m_ref_count.Release();
    if (0 == refCount)
    {
        m_ref_count.start_destroying();
        delete this;
    }
    return refCount;
}

CFocusManager* CContentRoot::GetFocusManagerNoRef()
{
    return &m_focusManager;
}

_Check_return_ HRESULT CContentRoot::Close()
{
    if (m_lifetimeState == LifetimeState::Closing || m_lifetimeState == LifetimeState::Closed)
    {
        return S_OK;
    }

    // It is possible for the CContentRoot to be deleted while being closed. Hold a
    // ref to keep the instance alive until the completion of Close
    xref_ptr<CContentRoot> keepAlive(this);

    m_lifetimeState = LifetimeState::Closing;
    bool releasePopupRoot = false;

    ResetCompositionContent();

    IFC_RETURN(m_visualTree.ResetRoots(&releasePopupRoot));

    if (releasePopupRoot)
    {
        // Clear popups from PopupRoot, otherwise they would only be cleared when the PopupRoot is disposed,
        // which would only occur after the managed peer check.  These include open popups with no direct
        // managed peer.  See bug 99901.
        m_visualTree.GetPopupRoot()->CloseAllPopupsForTreeReset();
    }

    // Release resources held by FocusManager's FocusRectManager. These
    // elements are automatically created on CFrameworkManager::UpdateFocus()
    // and must be released before core releases its main render target on
    // shutdown. Exposed by fixing core leak RS1 bug #7300521.
    m_focusManager.ReleaseFocusRectManagerResources();

    IFC_RETURN(m_visualTree.Shutdown());

    m_akExport.SetVisualTree(nullptr);
    m_akExport.SetFocusManager(nullptr);

    ContentRootCoordinator* coordinator = m_coreServices.GetContentRootCoordinator();
    coordinator->RemoveContentRoot(this);

    m_lifetimeState = LifetimeState::Closed;

    return S_OK;
}

VectorOfKACollectionAndRefCountPair& CContentRoot::GetAllLiveKeyboardAccelerators()
{
    return m_allLiveKeyboardAccelerators;
}

void CContentRoot::PrepareToClose()
{
    if (m_lifetimeState == LifetimeState::Normal)
    {
        m_lifetimeState = LifetimeState::PreparingToClose;
    }
}

bool CContentRoot::IsShuttingDown() const
{
    return m_lifetimeState == LifetimeState::Closing
        || m_lifetimeState == LifetimeState::Closed
        || m_lifetimeState == LifetimeState::PreparingToClose;
}

// This overrides default compare function for vector.
struct KeyboardAcceleratorCollectionComparer
{
    KeyboardAcceleratorCollectionComparer(const xref::weakref_ptr<CKeyboardAcceleratorCollection> weakKACollection) : _weakKACollection(weakKACollection) { }

    bool operator () (const KACollectionAndRefCountPair& pair) const
    {
        return (pair.first == _weakKACollection);
    }

    const xref::weakref_ptr<CKeyboardAcceleratorCollection>  _weakKACollection;
};

void CContentRoot::AddToLiveKeyboardAccelerators(_In_ CKeyboardAcceleratorCollection* const pKACollection)
{
    auto weakCollection = xref::get_weakref(pKACollection);
    auto it = std::find_if(
        m_allLiveKeyboardAccelerators.begin(),
        m_allLiveKeyboardAccelerators.end(),
        KeyboardAcceleratorCollectionComparer(weakCollection)
    );

    if (it != m_allLiveKeyboardAccelerators.end())
    {
        it->second++;
    }
    else
    {
        m_allLiveKeyboardAccelerators.push_back(std::make_pair(weakCollection, 1));
    }
}

void CContentRoot::RemoveFromLiveKeyboardAccelerators(_In_ CKeyboardAcceleratorCollection* const pKACollection)
{
    auto weakCollection = xref::get_weakref(pKACollection);
    auto it = std::find_if(
        m_allLiveKeyboardAccelerators.begin(),
        m_allLiveKeyboardAccelerators.end(),
        KeyboardAcceleratorCollectionComparer(weakCollection)
    );

    if (it != m_allLiveKeyboardAccelerators.end())
    {
        int refCount = --it->second;
        if (refCount == 0)
        {
            m_allLiveKeyboardAccelerators.erase(it);
        }
    }
}

IInspectable* CContentRoot::GetOrCreateXamlRootNoRef()
{
    return m_visualTree.GetOrCreateXamlRootNoRef();
}

IInspectable* CContentRoot::GetXamlRootNoRef()
{
    return m_visualTree.GetXamlRootNoRef();
}

void CContentRoot::RaiseXamlRootChanged(CContentRoot::ChangeType changeType)
{
    AddPendingXamlRootChangedEvent(changeType);
    bool shouldRaiseWindowChangedEvent = (changeType != CContentRoot::ChangeType::Content) ? true : false;
    RaisePendingXamlRootChangedEventIfNeeded(shouldRaiseWindowChangedEvent);
}

void CContentRoot::RaisePendingXamlRootChangedEventIfNeeded(bool shouldRaiseWindowChangedEvent)
{
    if (m_hasPendingChangedEvent)
    {
        m_hasPendingChangedEvent = false;

        if (auto xamlRoot = GetXamlRootNoRef())
        {
            FxCallbacks::XamlRoot_RaiseChanged(xamlRoot);
        }

        if (shouldRaiseWindowChangedEvent)
        {
            const wf::Size size = m_visualTree.GetSize();
            m_visualTree.GetQualifierContext()->OnWindowChanged(
                static_cast<XUINT32>(lround(size.Width)),
                static_cast<XUINT32>(lround(size.Height)));
        }
    }
}

void CContentRoot::RaiseXamlRootInputActivationChanged()
{
    if (auto xamlRoot = GetXamlRootNoRef())
    {
        FxCallbacks::XamlRoot_RaiseInputActivationChanged(xamlRoot);
    }
}

HWND CContentRoot::GetOwnerWindow() const
{
    if (m_type == Type::CoreWindow)
    {
        const auto ownerWindow = static_cast<HWND>(m_coreServices.GetHostSite()->GetXcpControlWindow());
        return ownerWindow;
    }
    else if (m_type == Type::XamlIslandRoot)
    {
        if (m_xamlIslandRoot)
        {
            const auto hWndInputSite = m_xamlIslandRoot->GetInputHWND();
            ASSERT(hWndInputSite != nullptr);
            return hWndInputSite;
        }
    }
    else
    {
        XAML_FAIL_FAST();
    }
    return nullptr;
}

// Return true if and only if the contentRoot's top-level window is currently in the
// activated state.
bool CContentRoot::IsWindowActivated() const
{
    const HWND topLevelWindow = ::GetAncestor(GetOwnerWindow(), GA_ROOT);
    if (topLevelWindow != NULL)
    {
        return (topLevelWindow == ::GetForegroundWindow());
    }
    return false;
}

void CContentRoot::SetIsInputActive(bool isInputActive)
{
    m_isInputActive = isInputActive;
}

bool CContentRoot::GetIsInputActive() const
{
    return m_isInputActive;
}

wrl::ComPtr<ixp::IContentIslandEnvironment> CContentRoot::GetContentIslandEnvironment() const
{
    // We only support islands right now.  Always return null for UWP.
    if (m_type == Type::XamlIslandRoot && m_xamlIslandRoot)
    {
        return m_xamlIslandRoot->GetContentIslandEnvironment();
    }    
    return nullptr;
}