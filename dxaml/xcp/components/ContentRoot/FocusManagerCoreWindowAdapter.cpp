// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FocusManagerCoreWindowAdapter.h"
#include "ContentRoot.h"

#include "FocusObserver.h"

#include "corep.h"
#include "focusmgr.h"

#include "host.h"

#include <FxCallbacks.h>

using namespace ContentRootAdapters;

FocusManagerCoreWindowAdapter::FocusManagerCoreWindowAdapter(_In_ CContentRoot& contentRoot)
    : FocusAdapter(contentRoot)
{
}

void FocusManagerCoreWindowAdapter::SetFocus()
{
    CFocusManager* focusManager = m_contentRoot.GetFocusManagerNoRef();
    CDependencyObject* focusedElement = focusManager->GetFocusedElementNoRef();
    FocusObserver* focusObserver = focusManager->GetFocusObserverNoRef();
    const HWND coreWindowHwnd = static_cast<HWND>(m_contentRoot.GetCoreServices().GetHostSite()->GetXcpControlWindow());

    const bool coreWindowIsRoot = focusedElement != nullptr;
    const bool coreWindowHasFocus = coreWindowHwnd == ::GetFocus();
    const bool coreWindowIsActivated = focusObserver->IsActivated();

    const bool newFocusedElementIsPopup   = focusedElement && focusedElement->OfTypeByIndex<KnownTypeIndex::Popup>();

    const bool shouldRestoreCoreWindowFocus = coreWindowIsRoot &&
        !focusManager->IsPluginFocused() &&
        !newFocusedElementIsPopup &&
        coreWindowIsActivated &&
        !coreWindowHasFocus;

    if (shouldRestoreCoreWindowFocus)
    {
        FxCallbacks::JupiterWindow_SetFocus();
    }
}