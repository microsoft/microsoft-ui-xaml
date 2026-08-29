// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Brush.h"
#include <MetadataAPI.h>
#include <DOPointerCast.h>
#include <Transform.h>
#include <DCompTreeHost.h>
#include <DependencyObject.h>
#include <XamlLight.g.h>
#include <XamlLight.h>

using namespace DirectUI;

xref_ptr<CTransform> CBrush::GetTransform() const
{
    CValue result;
    VERIFYHR(const_cast<CBrush*>(this)->GetEffectiveValueInSparseStorage(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Brush_Transform), &result));
    return checked_sp_cast<CTransform>(result.DetachObject());
}

xref_ptr<CTransform> CBrush::GetRelativeTransform() const
{
    CValue result;
    VERIFYHR(const_cast<CBrush*>(this)->GetEffectiveValueInSparseStorage(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Brush_RelativeTransform), &result));
    return checked_sp_cast<CTransform>(result.DetachObject());
}

FLOAT CBrush::GetOpacity() const
{
    return m_eOpacity;
}

void CBrush::AddLightTargetId(_In_ const xstring_ptr& lightId)
{
    auto& lightTargetIdMap = GetDCompTreeHost()->GetXamlLightTargetIdMap();
    bool hasMapChanged = lightTargetIdMap.AddTargetBrushAndId(this, lightId);

    if (hasMapChanged)
    {
        CBrush::SetLightTargetDirty(this, DirtyFlags::Render);
    }
}

void CBrush::RemoveLightTargetId(_In_ const xstring_ptr& lightId)
{
    auto& lightTargetIdMap = GetDCompTreeHost()->GetXamlLightTargetIdMap();
    bool hasMapChanged = lightTargetIdMap.RemoveTargetBrushAndId(this, lightId);

    if (hasMapChanged)
    {
        CBrush::SetLightTargetDirty(this, DirtyFlags::Render);
    }
}

bool CBrush::IsLightTarget()
{
    const auto& lightTargetIdMap = GetDCompTreeHost()->GetXamlLightTargetIdMap();
    return lightTargetIdMap.ContainsTarget(this);
}

bool CBrush::IsTargetedByLight(_In_ CXamlLight* light)
{
    const auto& lightTargetIdMap = GetDCompTreeHost()->GetXamlLightTargetIdMap();
    const auto& lightId = light->GetLightId();
    return lightTargetIdMap.ContainsTarget(this, lightId);
}

/* static */ void CBrush::SetLightTargetDirty(_In_ CDependencyObject* target, DirtyFlags flags)
{
    // This dirty flag method is manually called, and is only ever used with DirtyFlags::Render.
    ASSERT(flags == DirtyFlags::Render);

    // Call CMultiParentShareableDependencyObject tp propagate render dirty to all UIElements using this brush. As part of
    // regenerating their content, they will check for light targeting again.
    __super::NWSetRenderDirty(target, flags);
}

_Check_return_ HRESULT CBrush::StartAnimation(_In_ WUComp::ICompositionAnimationBase* animation)
{
    // This will be implemented by derived classes.
    return E_INVALIDARG;
}
_Check_return_ HRESULT CBrush::StopAnimation(_In_ WUComp::ICompositionAnimationBase* animation)
{
    // This will be implemented by derived classes.
    return E_INVALIDARG;
}
