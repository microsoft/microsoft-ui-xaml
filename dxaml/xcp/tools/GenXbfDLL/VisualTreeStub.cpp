// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "visualtree.h"
#include "contentroot.h"
#include <Panel.h>
#include <Canvas.h>
#include "xamlislandrootcollection.h"
#include "ConnectedAnimationRoot.h"
#include "RenderTargetBitmapRoot.h"
#include "FullWindowMediaRoot.h"
#include "TransitionRoot.h"
#include "PrintRoot.h"
#include "ContentPresenter.h"
#include "ScrollContentControl.h"
#include "Grid.h"
#include "Popup.h"
#include "RootVisual.h"
#include "RootScale.h"
#include "LayoutTransitionElement.h"

VisualTree::VisualTree(_In_ CCoreServices *pCore, _In_ XUINT32 backgroundColor, _In_opt_ CUIElement* rootElement, _In_ CContentRoot& coreContentRoot)
    : m_coreContentRoot(coreContentRoot)
{
}

VisualTree::~VisualTree()
{
}

_Check_return_ HRESULT VisualTree::ResetRoots(_Out_opt_ bool *pReleasePopup)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT VisualTree::Shutdown()
{
    return E_NOTIMPL;
}

XUINT32 VisualTree::Release()
{
    return 0;
}

void CPopupRoot::CloseAllPopupsForTreeReset(void)
{
}

CPopupRoot* VisualTree::GetPopupRoot(void)
{
    return nullptr;
}

IInspectable* VisualTree::GetXamlRootNoRef() const
{
    return nullptr;
}
