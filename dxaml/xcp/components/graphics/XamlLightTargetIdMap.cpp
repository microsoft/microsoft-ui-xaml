// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLightTargetIdMap.h"
#include <XamlLight.h>
#include <UIElement.h>
#include <Brush.h>

bool XamlLightTargetIdMap::AddTargetElementAndId(_In_ CUIElement* target, _In_ const xstring_ptr& lightId)
{
    return AddTargetAndId(target, lightId);
}

bool XamlLightTargetIdMap::RemoveTargetElementAndId(_In_ CUIElement* target, _In_ const xstring_ptr& lightId)
{
    return RemoveTargetAndId(target, lightId);
}

bool XamlLightTargetIdMap::AddTargetBrushAndId(_In_ CBrush* target, _In_ const xstring_ptr& lightId)
{
    return AddTargetAndId(target, lightId);
}

bool XamlLightTargetIdMap::RemoveTargetBrushAndId(_In_ CBrush* target, _In_ const xstring_ptr& lightId)
{
    return RemoveTargetAndId(target, lightId);
}

bool XamlLightTargetIdMap::AddTargetAndId(_In_ CDependencyObject* target, _In_ const xstring_ptr& lightId)
{
    const auto& pair = m_map.find(target);
    if (pair != m_map.end())
    {
        const auto& insertResult = pair->second.insert(lightId);
        return insertResult.second;
    }
    else
    {
        StringSet set;
        set.insert(lightId);
        VERIFY_COND(m_map.emplace(target, set), .second);
        return true;
    }
}

bool XamlLightTargetIdMap::RemoveTargetAndId(_In_ CDependencyObject* target, _In_ const xstring_ptr& lightId)
{
    const auto& pair = m_map.find(target);
    if (pair != m_map.end())
    {
        int numberErased = static_cast<int>(pair->second.erase(lightId));

        if (pair->second.empty())
        {
            m_map.erase(pair);
        }

        ASSERT(numberErased == 0 || numberErased == 1);
        return numberErased == 1;
    }

    return false;
}

void XamlLightTargetIdMap::RemoveTarget(_In_ CDependencyObject* target)
{
    const auto& pair = m_map.find(target);
    if (pair != m_map.end())
    {
        m_map.erase(pair);
    }
}

bool XamlLightTargetIdMap::ContainsTarget(_In_ CDependencyObject* target) const
{
    const auto& pair = m_map.find(target);
    return pair != m_map.end();
}

bool XamlLightTargetIdMap::ContainsTarget(_In_ CDependencyObject* target, _In_ const xstring_ptr& lightId) const
{
    const auto& pair = m_map.find(target);
    if (pair != m_map.end())
    {
        const auto& entry = pair->second.find(lightId);
        return entry != pair->second.end();
    }
    return false;
}
