// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLightTargetMap.h"
#include "XamlLight.h"

void XamlLightTargetMap::AddTargetVisual(_In_ CUIElement* target, _In_ WUComp::IVisual* visual, _In_ CXamlLight* light)
{
    auto pair = m_map.find(target);
    if (pair != m_map.end())
    {
        pair->second.insert(light);
    }
    else
    {
        std::unordered_set<CXamlLight*> set;
        set.insert(light);
        VERIFY_COND(m_map.emplace(target, set), .second);
    }

    light->AddTargetVisual(target, visual);
}

void XamlLightTargetMap::RemoveTargetVisual(_In_ CUIElement* target, _In_ WUComp::IVisual* visual)
{
    const auto& pair = m_map.find(target);
    if (pair != m_map.end())
    {
        auto& lights = pair->second;
        for (auto iter = lights.begin(); iter != lights.end(); /* manual */)
        {
            auto& light = *iter;
            bool removed = light->RemoveTargetVisual(target, visual);

            // If the last visual for the target element had been removed, then the light is no longer targeting the element.
            if (removed && !light->TargetsElement(target))
            {
                iter = lights.erase(iter);
            }
            else
            {
                ++iter;
            }
        }

        if (lights.size() == 0)
        {
            m_map.erase(pair);
        }
    }
}

void XamlLightTargetMap::RemoveTargetAndLight(_In_ CUIElement* target, _In_ CXamlLight* light)
{
    const auto& pair = m_map.find(target);
    if (pair != m_map.end())
    {
        auto& lights = pair->second;

        lights.erase(light);

        if (lights.empty())
        {
            m_map.erase(pair);
        }
    }
}

void XamlLightTargetMap::RemoveTargetAndUnregisterElement(_In_ CUIElement* target)
{
    const auto& pair = m_map.find(target);
    if (pair != m_map.end())
    {
        for (const auto& light : pair->second)
        {
            light->RemoveTargetElement(target);
        }

        m_map.erase(pair);
    }
}
