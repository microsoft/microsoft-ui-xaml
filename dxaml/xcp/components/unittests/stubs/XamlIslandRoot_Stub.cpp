// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif

#include "Indexes.g.h"
#include "DependencyObject.h"
#include "XamlIslandRoot.h"


struct wf::Size __thiscall CXamlIslandRoot::GetSize(void)
{
    wf::Size size = {};
    return size;
}

ixp::IContentIsland * CXamlIslandRoot::GetContentIsland()
{
    return nullptr;
}

CUIElement* CXamlIslandRoot::GetPublicRootVisual()
{
    return nullptr;
}

CPopupRoot* CXamlIslandRoot::GetPopupRootNoRef()
{
    return nullptr;
}

void CXamlIslandRoot::ClientLogicalToScreenPhysical(_Inout_ POINT& pt) {}

void CXamlIslandRoot::ScreenPhysicalToClientLogical(_Inout_ POINT& pt) {}
