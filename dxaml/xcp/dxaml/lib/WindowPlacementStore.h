// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Reads and writes persisted window placement text in the app's local settings.
//
// See docs/design-notes/Window-PlacementPersistence.md sections 3.1 and 3.3. The
// stored shape is:
//
//     <LocalSettings root>\Microsoft.UI.Xaml.WindowPlacement
//     wp1_<readable-slug>_<hash> = <base64 placement blob>
//
// Value names come from WindowPlacementValueName.h. The base64 text comes from
// WindowPlacementBlob.h. This file only moves that text in and out of the store.
//
// Every entry point is fail-safe and returns bool rather than HRESULT. An
// unreachable settings store, a missing value, and a store that does not exist
// yet are all normal: they mean "no placement to restore". A window must still
// open when its placement cannot be read, and must still close when its
// placement cannot be written.

#include <string>

namespace WindowPlacementPersistence {

class Store
{
public:
    // Fills `text` with the stored value. Returns false when the value is
    // absent, empty, or unreadable.
    static bool TryLoad(_In_ const std::wstring& valueName, _Out_ std::wstring& text);

    // Writes `text`, creating the placement container if needed. Returns false
    // when the value could not be written.
    static bool TrySave(_In_ const std::wstring& valueName, _In_ const std::wstring& text);

    // Removes any stored value. Returns true when the value is gone afterwards,
    // including when there was nothing stored to begin with.
    static bool TryRemove(_In_ const std::wstring& valueName);

    // True when the app can reach a local settings store at all. Unpackaged apps
    // on a Windows App SDK without unpackaged ApplicationData support cannot, and
    // silently do not participate in placement persistence.
    static bool IsAvailable();
};

} // namespace WindowPlacementPersistence
