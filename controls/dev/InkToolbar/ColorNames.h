// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "Utils.h"

// Faithful C++/WinRT port of onecoreuap\...\inkcontrols\lib\ColorNames.{h,cpp}.
//
// UWP maps a color to a localized name through a 15-bit packed resource id
//     IDS_INKTOOLBAR_COLORNAME_BASE + ((R>>3)<<10 | (G>>3)<<5 | (B>>3))
// into a ~32K-entry string table hosted by the OS binary Windows.UI.Xaml.InkControls.dll. That table is
// not reachable from the lift, so the same packing is used here as the match key against the colors the
// InkToolbar palettes actually ship - the 30 default pen colors (InkToolbarPenButton) plus the two
// highlighter-only colors (InkToolbarHighlighterButton) - whose names live in our own Resources.resw.
// Dropping the low 3 bits of each channel is what makes near-identical colors share a name, exactly as in
// UWP. A color with no entry falls back to UWP's generic "RGB color r, g, b" format and reports
// isGenericFormat=true, which is the same behavior UWP has for an id with no string.
struct ColorNames
{
    winrt::hstring GetColorName(winrt::Windows::UI::Color const& color, bool& isGenericFormat)
    {
        const uint16_t key = PackColor(color.R, color.G, color.B);

        for (auto const& entry : c_colorNames)
        {
            if (PackColor(entry.r, entry.g, entry.b) == key)
            {
                auto name = TryGetLocalizedString(entry.resourceId);
                if (!name.empty())
                {
                    isGenericFormat = false;
                    return name;
                }
                break;
            }
        }

        isGenericFormat = true;

        // StringUtil::FormatString is FormatMessage-based, so inserts are %1!u! and not printf %u.
        auto format = TryGetLocalizedString(SR_InkToolbarColorRgbFormat);
        if (format.empty())
        {
            format = winrt::hstring{ L"RGB color %1!u!, %2!u!, %3!u!" };
        }

        return StringUtil::FormatString(
            format, static_cast<unsigned>(color.R), static_cast<unsigned>(color.G), static_cast<unsigned>(color.B));
    }

private:
    // ResourceAccessor throws ERROR_NOT_FOUND for an absent name, which an app carrying an older merged
    // PRI than the framework will hit. UWP logged and carried on (LOG_IF_FAILED), so do the same: a
    // missing color name degrades to the generic RGB text rather than taking the app down.
    static winrt::hstring TryGetLocalizedString(std::wstring_view resourceName)
    {
        try
        {
            return ResourceAccessor::GetLocalizedStringResource(resourceName);
        }
        catch (...)
        {
            return {};
        }
    }
    // UWP ColorNames::ColorToResourceId, minus the resource base: 5 bits per channel, red most significant.
    static constexpr uint16_t PackColor(uint8_t r, uint8_t g, uint8_t b)
    {
        return static_cast<uint16_t>(((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3));
    }

    struct ColorNameEntry
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        const wchar_t* resourceId;
    };

    // Kept in the same order as the UWP IDS_INKTOOLBAR_COLORNAME_* table in inkcontrols\lib\Resource.h.
    static constexpr ColorNameEntry c_colorNames[] = {
        { 0x00, 0x00, 0x00, SR_InkToolbarColorNameBlack },
        { 0xFF, 0xFF, 0xFF, SR_InkToolbarColorNameWhite },
        { 0xD1, 0xD3, 0xD4, SR_InkToolbarColorNameSilver },
        { 0xA7, 0xA9, 0xAC, SR_InkToolbarColorNameGray },
        { 0x80, 0x82, 0x85, SR_InkToolbarColorNameDarkGray },
        { 0x58, 0x59, 0x5B, SR_InkToolbarColorNameCharcoal },
        { 0xB3, 0x15, 0x64, SR_InkToolbarColorNameMagenta },
        { 0xE6, 0x1B, 0x1B, SR_InkToolbarColorNameRed },
        { 0xFF, 0x55, 0x00, SR_InkToolbarColorNameRedOrange },
        { 0xFF, 0xAA, 0x00, SR_InkToolbarColorNameOrange },
        { 0xFF, 0xCE, 0x00, SR_InkToolbarColorNameGold },
        { 0xFF, 0xE6, 0x00, SR_InkToolbarColorNameYellow },
        { 0xA2, 0xE6, 0x1B, SR_InkToolbarColorNameGrassGreen },
        { 0x26, 0xE6, 0x00, SR_InkToolbarColorNameGreen },
        { 0x00, 0x80, 0x55, SR_InkToolbarColorNameDarkGreen },
        { 0x00, 0xAA, 0xCC, SR_InkToolbarColorNameTeal },
        { 0x00, 0x4D, 0xE6, SR_InkToolbarColorNameBlue },
        { 0x3D, 0x00, 0xB8, SR_InkToolbarColorNameIndigo },
        { 0x66, 0x00, 0xCC, SR_InkToolbarColorNameViolet },
        { 0x60, 0x00, 0x80, SR_InkToolbarColorNamePurple },
        { 0xF7, 0xD7, 0xC4, SR_InkToolbarColorNameBeige },
        { 0xBB, 0x91, 0x67, SR_InkToolbarColorNameLightBrown },
        { 0x8E, 0x56, 0x2E, SR_InkToolbarColorNameBrown },
        { 0x61, 0x3D, 0x30, SR_InkToolbarColorNameDarkBrown },
        { 0xFF, 0x80, 0xFF, SR_InkToolbarColorNamePastelPink },
        { 0xFF, 0xC6, 0x80, SR_InkToolbarColorNamePastelOrange },
        { 0xFF, 0xFF, 0x80, SR_InkToolbarColorNamePastelYellow },
        { 0x80, 0xFF, 0x9E, SR_InkToolbarColorNamePastelGreen },
        { 0x80, 0xD6, 0xFF, SR_InkToolbarColorNamePastelBlue },
        { 0xBC, 0xB3, 0xFF, SR_InkToolbarColorNamePastelPurple },
        // Highlighter-only colors.
        { 0x44, 0xC8, 0xF5, SR_InkToolbarColorNameLightBlue },
        { 0xEC, 0x00, 0x8C, SR_InkToolbarColorNamePink },
    };
};
