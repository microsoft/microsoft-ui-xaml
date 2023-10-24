// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <unordered_set>

class CUIElement;
class CXamlLight;

// A map of CUIElements to the CXamlLights that are targeting them
class XamlLightTargetMap
{
public:
    // Adds a visual associated with an element.
    void AddTargetVisual(_In_ CUIElement* target, _In_ WUComp::IVisual* visual, _In_ CXamlLight* light);

    // Remove a visual associated with an element.
    void RemoveTargetVisual(_In_ CUIElement* target, _In_ WUComp::IVisual* visual);

    // Removes an entry (e.g. for CUIElement 1234 to be target by XamlLight 5678) from the map.
    // Used when a light is destroyed.
    void RemoveTargetAndLight(_In_ CUIElement* target, _In_ CXamlLight* light);

    // Removes all entries associated with a target (e.g. CUIElement 1234) and notifies any lights that were targeting it.
    // Used when a UIElement leaves the tree.
    void RemoveTargetAndUnregisterElement(_In_ CUIElement* target);

public:
    containers::vector_map<
        CUIElement*,
        std::unordered_set<CXamlLight*>
        > m_map;
};
