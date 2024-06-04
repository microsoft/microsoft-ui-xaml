// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <unordered_set>

class CUIElement;
class CBrush;
class CXamlLight;

// A map of CUIElements to a list of IDs of the lights that should be targeting them
class XamlLightTargetIdMap
{
public:
    // Adds an entry (e.g. for CUIElement 1234's subtree to be target by XamlLight with ID "asdf") to the map. Returns whether the map was actually changed.
    // According to our implementation, any DO can be targeted, but we restrict it to just UIElements and brushes.
    bool AddTargetElementAndId(_In_ CUIElement* target, _In_ const xstring_ptr& lightId);

    // Removes an entry (e.g. for CUIElement 1234's subtree to be target by XamlLight with ID "asdf") from the map. Returns whether the map was actually changed.
    // According to our implementation, any DO can be targeted, but we restrict it to just UIElements and brushes.
    bool RemoveTargetElementAndId(_In_ CUIElement* target, _In_ const xstring_ptr& lightId);

    // Adds an entry (e.g. for CBrush 1234 to be target by XamlLight with ID "asdf") to the map. Returns whether the map was actually changed.
    // According to our implementation, any DO can be targeted, but we restrict it to just UIElements and brushes.
    bool AddTargetBrushAndId(_In_ CBrush* target, _In_ const xstring_ptr& lightId);

    // Removes an entry (e.g. for CBrush 1234 to be target by XamlLight with ID "asdf") from the map. Returns whether the map was actually changed.
    // According to our implementation, any DO can be targeted, but we restrict it to just UIElements and brushes.
    bool RemoveTargetBrushAndId(_In_ CBrush* target, _In_ const xstring_ptr& lightId);

    // Removes all entries associated with a target (e.g. CUIElement 1234).
    void RemoveTarget(_In_ CDependencyObject* target);

    // Returns whether the given object wants to be targeted by any lights at all - i.e. checks for an entry with the target as the key
    bool ContainsTarget(_In_ CDependencyObject* target) const;

    // Returns whether the given object is targeted by the given light - i.e. checks for an entry with the target and the light's Id.
    bool ContainsTarget(_In_ CDependencyObject* target, _In_ const xstring_ptr& lightId) const;

    typedef std::unordered_set<
            xstring_ptr,
            xstrCaseSensitiveHasher,
            xstrCaseSensitiveEqual,
            std::allocator<xstring_ptr>
            > StringSet;

    typedef containers::vector_map<CDependencyObject*, StringSet> StringSetMap;

private:
    // Adds an entry (e.g. for CDependencyObject 1234 to be target by XamlLight with ID "asdf") to the map. Returns whether the map was actually changed.
    bool AddTargetAndId(_In_ CDependencyObject* target, _In_ const xstring_ptr& lightId);

    // Removes an entry (e.g. for CDependencyObject 1234 to be target by XamlLight with ID "asdf") from the map. Returns whether the map was actually changed.
    bool RemoveTargetAndId(_In_ CDependencyObject* target, _In_ const xstring_ptr& lightId);

public:
    StringSetMap m_map;
};
