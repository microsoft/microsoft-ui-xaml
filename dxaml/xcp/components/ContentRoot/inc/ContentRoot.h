// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "visualtree.h"
#include "focusmgr.h"
#include "inputservices.h"
#include "akexport.h"

#include "InputManager.h"

#include "EnumDefs.g.h"

#include "FocusAdapter.h"
#include "ContentRootEventListener.h"
#include <Microsoft.UI.Content.h>

/*
    +------------------------------------------------------------------------------------+
    |                                      +---------------+                             |
    |                                      | CCoreServices |                             |
    |                                      +-------+-------+                             |
    |                                              |                                     |
    |                                              |                                     |
    |                                        +-----v----------------+                    |
    |                              +---------+ContentRootCoordinator|                    |
    |                              |         +-----------------+----+                    |
    |                              |                           |                         |
    |                              |                           |                         |
    |                       +------v-----+            +--------v---+                     |
    |                       |CContentRoot|            |CContentRoot|"Main Content Root"  |
    |                       +--+---------+            +------+-----+                     |
    |                          |                             |                           |
    |      +--------------+    |                             |      +---------------+    |
    |      | CFocusManager<----+                             +------> CFocusManager |    |
    |      +--------------+    |                             |      +---------------+    |
    |      +--------------+    |                             |      +--------------+     |
    |      | CInputManager<----+                             +------> CInputManager|     |
    |      +--------------+    |                             |      +--------------+     |
    |      +------------+      |                             |      +-----------+        |
    |      | VisualTree <------+                             +------> VisualTree|        |
    |      +------------+                                           +-----------+        |
    |                                                                                    |
    |                                                                                    |
    +------------------------------------------------------------------------------------+
*/

class CInputServices;
class CCoreServices;
class CKeyboardAcceleratorCollection;
class CXamlIslandRoot;

typedef std::pair<xref::weakref_ptr<CKeyboardAcceleratorCollection>, unsigned int> KACollectionAndRefCountPair;
typedef std::vector<KACollectionAndRefCountPair> VectorOfKACollectionAndRefCountPair;

class CContentRoot
{
public:
    friend class ContentRootCoordinator;

    enum Type
    {
        CoreWindow,
        XamlIslandRoot
    };

    enum IslandType
    {
        Invalid,
        Raw,
        DesktopWindowContentBridge
    };

    enum class LifetimeState
    {
        Normal,
        PreparingToClose,
        Closing,
        Closed
    };

    XUINT32 AddRef();
    XUINT32 Release();
    xref::details::control_block* EnsureControlBlock()
    {
        return m_ref_count.ensure_control_block();
    }

    CContentRoot::Type GetType() const { return m_type; }
    CContentRoot::IslandType GetIslandType() const { return m_islandType; }
    CCoreServices& GetCoreServices() const { return m_coreServices; }

    CFocusManager* GetFocusManagerNoRef();
    VisualTree* GetVisualTreeNoRef() { return &m_visualTree; }
    CInputManager& GetInputManager();

    ixp::IContentIsland* GetCompositionContentNoRef() const { return m_compositionContent.Get(); }
    _Check_return_ HRESULT SetContentIsland(_In_ ixp::IContentIsland* compositionContent);

    AccessKeys::AccessKeyExport& GetAKExport() { return m_akExport; }

    CXamlIslandRoot* GetXamlIslandRootNoRef() const;
    void SetXamlIslandRoot(_In_ CXamlIslandRoot* xamlIslandRoot);
    void SetXamlIslandType(_In_ CContentRoot::IslandType islandType);

    HWND GetHostingHWND() const;

    // CKeyboardAcceleratorCollection can be added multiple times through initial Live Enters and then
    // Flyouts Open operations ( I can think of). We need reference counting on accelerator collection
    // so that closing flyout will only decrement the ref count and will not remove keyboard accelerator
    // collections which might be still alive. Collection will only be removed if ref count goes down to 0.
    // As of now Add/ Remove reference and then collection is triggered through Enter and Leave mechanism only.
    // If one has to update ref counts explicitly, please make sure to take care of life time of those collections.
    VectorOfKACollectionAndRefCountPair& GetAllLiveKeyboardAccelerators();
    void AddToLiveKeyboardAccelerators(_In_ CKeyboardAcceleratorCollection* const pKACollection);
    void RemoveFromLiveKeyboardAccelerators(_In_ CKeyboardAcceleratorCollection* const pKACollection);

    bool IsShuttingDown() const;
    _Check_return_ HRESULT Close();

    ContentRootAdapters::FocusAdapter& GetFocusAdapter();

    IInspectable* GetOrCreateXamlRootNoRef();

    // Support for the public XamlRootChanged event
    enum ChangeType
    {
        Size,
        RasterizationScale,
        IsVisible,
        Content
    };
    void RaiseXamlRootChanged(ChangeType changeType);
    void AddPendingXamlRootChangedEvent(ChangeType /*ignored for now*/) { m_hasPendingChangedEvent = true; }
    void RaisePendingXamlRootChangedEventIfNeeded(bool shouldRaiseWindowChangedEvent);
    bool HasPendingXamlRootChangedEvent() { return m_hasPendingChangedEvent; }

    void RaiseXamlRootInputActivationChanged();

    bool ShouldUseVisualRelativePixels();

    void PrepareToClose();

    void SetIsInputActive(bool isInputActive);
    bool GetIsInputActive() const;

    wrl::ComPtr<ixp::IContentIslandEnvironment> GetContentIslandEnvironment() const;

// CONTENT-TODO: Lifted IXP doesn't support OneCoreTransforms UIA yet.
#if false
    UINT64 GetVisualIdentifier();
#endif

private:
    CContentRoot(_In_ CContentRoot::Type type, _In_ XUINT32 backgroundColor, _In_opt_ CUIElement* rootElement, _In_ CCoreServices& coreServices);
    ~CContentRoot();

    _Check_return_ HRESULT OnAutomationProviderRequested(
        ixp::IContentIsland* content,
        ixp::IContentIslandAutomationProviderRequestedEventArgs* args);
    _Check_return_ HRESULT OnStateChanged();
    _Check_return_ HRESULT RegisterCompositionContent(_In_ ixp::IContentIsland* compositionContent);
    _Check_return_ HRESULT ResetCompositionContent();

    IInspectable* GetXamlRootNoRef();

    xref::details::optional_ref_count m_ref_count;
    LifetimeState m_lifetimeState = LifetimeState::Normal;

    CContentRoot::Type m_type = CContentRoot::Type::CoreWindow;
    CContentRoot::IslandType m_islandType = CContentRoot::IslandType::Invalid;

    CCoreServices& m_coreServices;
    VisualTree m_visualTree;
    CFocusManager m_focusManager;

    Microsoft::WRL::ComPtr<ixp::IContentIsland> m_compositionContent;
    EventRegistrationToken m_compositionContentStateChangedToken = {};
    EventRegistrationToken m_automationProviderRequestedToken = {};

    xref_ptr<CXamlIslandRoot> m_xamlIslandRoot;

    CInputManager m_inputManager;

    AccessKeys::AccessKeyExport m_akExport;

    VectorOfKACollectionAndRefCountPair m_allLiveKeyboardAccelerators;

    std::unique_ptr<ContentRootAdapters::FocusAdapter> m_focusAdapter;
    ContentRootAdapters::ContentRootEventListener m_contentRootEventListener;

    bool m_hasPendingChangedEvent = false;

    bool m_isInputActive{};
};

struct XamlRootStateChangedRaiiHelper
{
    XamlRootStateChangedRaiiHelper(_In_opt_ CContentRoot* contentRoot) : m_contentRoot(contentRoot) {}
    ~XamlRootStateChangedRaiiHelper()
    {
        if (m_contentRoot)
        {
            m_contentRoot->RaisePendingXamlRootChangedEventIfNeeded(true /*shouldRaiseWindowChangedEvent*/ );
        }
    }

    CContentRoot* m_contentRoot;
};
