// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <corep.h>
#include "AKExport.h"

#include <DXamlServices.h>
#include "AccessKeysEvents.Specializations.h"

#include <CDependencyObject.h>
#include <UIElement.h>
#include <CControl.h>

#include <AutomationPeer.h>

#include <UIElementCollection.h>
#include <Framework.h>
#include <Popup.h>
#include <FlyoutBase.h>

#include "focusmgr.h"
#include "VisualTreeAdapter.h"

#include "InputInterceptor.h"
#include "ScopeTree.h"
#include "ScopeBuilder.h"
#include "AccessKey.h"
#include "AccessKeysParser.h"
#include "ModeContainer.h"
#include "Scope.h"
#include "TreeAnalyzer.h"

using namespace AccessKeys;

typedef AKTreeAnalyzer<CDOCollection, VisualTree, AKVisualTreeFinder> TreeAnalyzer;
typedef AKScopeBuilder<TreeAnalyzer, AKParser, CDependencyObject, Scope> ScopeBuilder;
typedef AKScopeTree<Scope, ScopeBuilder, AKModeContainer, CDependencyObject, TreeAnalyzer> ScopeTree;
typedef AKInputInterceptor<AKModeContainer,ScopeTree, TreeAnalyzer> InputInterceptor;

struct AccessKeys::AccessKeyExportImpl
{
    AccessKeyExportImpl(_In_ CCoreServices* const core) :
        m_treeLibrary(core),
        m_treeAnalyzer(m_treeLibrary),
        m_scopeBuilder(m_treeAnalyzer),
        m_scopeTree(m_scopeBuilder, m_treeAnalyzer, m_container),
        m_inputInterceptor(m_container, m_scopeTree, m_treeAnalyzer),
        m_isVisualTreeValid(false),
        m_isFocusManagerValid(false) {}

    bool IsValid() const { return m_isFocusManagerValid && m_isVisualTreeValid; }

    AKVisualTreeFinder m_treeLibrary;
    AKModeContainer m_container;
    TreeAnalyzer m_treeAnalyzer;
    ScopeBuilder m_scopeBuilder;
    ScopeTree m_scopeTree;
    InputInterceptor m_inputInterceptor;
    bool m_isVisualTreeValid;
    bool m_isFocusManagerValid;
};

AccessKeyExport::AccessKeyExport(_In_ CCoreServices* const core) : impl(std::make_unique<AccessKeyExportImpl>(core)) {};
AccessKeyExport::~AccessKeyExport() = default;

_Check_return_ HRESULT AccessKeyExport::TryProcessInputForAccessKey(_In_ const InputMessage* const inputMessage, _Out_ bool* keyProcessed) const
{
    if (impl->IsValid())
    {
        // We anticipate input messages to arrive with XCP_CHAR only when we are not hosted inside a CoreWindow
        // In all other cases, these should be coming in/getting handled by TryProcessInputForCharacterReceived
        if (inputMessage->m_msgID == XCP_CHAR)
        {
            ASSERT(inputMessage->m_hCoreWindow == NULL);
        }
        IFC_RETURN(impl->m_inputInterceptor.TryProcessInputForAccessKey(inputMessage, keyProcessed));
    }

    return S_OK;
}

_Check_return_ HRESULT AccessKeyExport::UpdateScope() const
{
    if (impl->IsValid())
    {
        IFC_RETURN(impl->m_scopeTree.UpdateScope());
    }

    return S_OK;
}

_Check_return_ HRESULT AccessKeyExport::ProcessPointerInput(_In_ const InputMessage* const inputMessage) const
{
    if (impl->IsValid())
    {
        IFC_RETURN(impl->m_inputInterceptor.ProcessPointerInput(inputMessage));
    }

    return S_OK;
}

_Check_return_ HRESULT AccessKeyExport::TryProcessInputForCharacterReceived(_In_ mui::ICharacterReceivedEventArgs* args, _Out_ bool* keyProcessed) const
{
    if (impl->IsValid())
    {
        IFC_RETURN(impl->m_inputInterceptor.TryProcessInputForCharacterReceived(args, keyProcessed));
    }

    return S_OK;
}

_Check_return_ HRESULT AccessKeyExport::AddElementToAKMode(_In_ CDependencyObject* const element) const
{
    if (impl->IsValid())
    {
        IFC_RETURN(impl->m_scopeTree.AddElement(element));
    }

    return S_OK;
}

_Check_return_ HRESULT AccessKeyExport::RemoveElementFromAKMode(_In_ CDependencyObject* const element) const
{
    if (impl->IsValid())
    {
        IFC_RETURN(impl->m_scopeTree.RemoveElement(element));
    }

    return S_OK;
}

_Check_return_ HRESULT AccessKeyExport::OnIsEnabledChanged(_In_ CDependencyObject* const element, bool isEnabled) const
{
    if(impl->IsValid())
    {
        IFC_RETURN(impl->m_scopeTree.OnIsEnabledChanged(element, isEnabled));
    }

    return S_OK;
}

_Check_return_ HRESULT AccessKeyExport::OnVisibilityChanged(_In_ CDependencyObject* const element, const DirectUI::Visibility& visibility) const
{
    if (impl->IsValid())
    {
        IFC_RETURN(impl->m_scopeTree.OnVisibilityChanged(element, visibility));
    }

    return S_OK;
}

bool AccessKeyExport::IsActive() const
{
    return impl->m_container.GetIsActive();
}

AKModeContainer& AccessKeyExport::GetModeContainer() const
{
    return impl->m_container;
}

void AccessKeyExport::SetVisualTree(_In_ VisualTree* tree) const
{
    impl->m_treeLibrary.SetVisualTree(tree);
    impl->m_isVisualTreeValid = tree != nullptr;
}

void AccessKeyExport::SetFocusManager(_In_ CFocusManager* focusManager) const
{
    impl->m_scopeTree.SetFocusManager(focusManager);
    impl->m_container.SetFocusManager(focusManager);
    impl->m_isFocusManagerValid = focusManager != nullptr;
}

_Check_return_ HRESULT AccessKeyExport::CleanupAndExitCurrentScope() const
{
    if (impl->IsValid())
    {
        IFC_RETURN(impl->m_scopeTree.ExitScope(IsActive()));
    }

    return S_OK;
}

_Check_return_ HRESULT AccessKeyExport::ExitAccessKeyMode() const
{
    IFC_RETURN(CleanupAndExitCurrentScope());
    impl->m_container.SetIsActive(false);
    return S_OK;
}

_Check_return_ HRESULT AccessKeyExport::EnterAccessKeyMode() const
{
    if (impl->m_container.GetIsActive())
    {
        return S_OK;
    }
    impl->m_container.SetIsActive(true);
    impl->m_scopeTree.EnterScope();
    return S_OK;
}