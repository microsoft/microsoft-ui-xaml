// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif

#include "HWCompNode.h"
#include "DManipData.h"


HWCompNode::HWCompNode(
    _In_ CCoreServices *coreServices,
    _In_ CompositorTreeHost *pCompositorTreeHost)
    : CDependencyObject(coreServices)
{
}

HWCompNode::~HWCompNode()
{
}

_Check_return_ HRESULT HWCompNode::Remove()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT HWCompTreeNode::UpdateDManipData(_In_ CUIElement *)
{
    return E_NOTIMPL;
}

HWCompTreeNode::HWCompTreeNode(
    _In_ CCoreServices *coreServices,
    _In_ CompositorTreeHost *pCompositorTreeHost,
    bool isPlaceholderCompNode
    )
    : HWCompNode(coreServices, pCompositorTreeHost)
{
}

HWCompTreeNode::~HWCompTreeNode()
{
}

_Check_return_ HRESULT HWCompTreeNode::Remove()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT HWCompTreeNode::UpdateTreeVirtual(
    _In_opt_ DCompTreeHost *pDCompTreeHost,
    bool useDCompAnimations,
    bool disablePixelSnapping)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT HWCompTreeNode::UpdateTreeChildren(
    _In_opt_ DCompTreeHost *pDCompTreeHost,
    bool useDCompAnimations,
    bool disablePixelSnapping)
{
    return E_NOTIMPL;
}

void HWCompTreeNode::ClearVisualContent()
{
}

HWCompTreeNode* HWCompTreeNode::GetRedirectionWalkParent()
{
    return nullptr;
}

CTransformToRoot::CTransformToRoot()
{
}

CTransformToRoot::~CTransformToRoot()
{
}

DManipDataBase::DManipDataBase()
{
}

_Check_return_ HRESULT DManipDataBase::SetClipContent(_In_opt_ IObject *)
{
    return E_NOTIMPL;
}

XDMContentType DManipDataBase::GetManipulationContentType(void)const
{
    return XcpDMContentTypePrimary;
}

DManipDataWinRT::~DManipDataWinRT()
{
}

bool DManipDataWinRT::HasSharedManipulationTransform(bool)
{
    return false;
}

_Check_return_ HRESULT DManipDataWinRT::SetContentOffsets(_In_ float, _In_ float)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT DManipDataWinRT::EnsureOverallContentPropertySet(_In_ WUComp::ICompositor* pCompositor)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT DManipDataWinRT::SetSharedContentTransforms(
    _In_opt_ IUnknown* sharedPrimaryContentTransform,
    _In_opt_ IUnknown* sharedSecondaryContentTransform,
    _In_opt_ WUComp::ICompositor* pCompositor)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT DManipDataWinRT::SetSharedClipTransform(
    _In_opt_ IUnknown* sharedClipTransform,
    _In_opt_ WUComp::ICompositor* pCompositor)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT DManipDataWinRT::SetClipContent(_In_opt_ IObject* clipContent)
{
    return E_NOTIMPL;
}
