// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "InkToolbarTrace.h"
#include "Utils.h"

// Match the UWP color-name lookup using five bits per channel for the built-in palette colors.
struct ColorNames
{
    winrt::hstring GetColorName(winrt::Windows::UI::Color const& color, bool& isGenericFormat)
    {
        const uint16_t key = PackColor(color.R, color.G, color.B);

        for (auto const& entry : c_colorNames)
        {
            if (PackColor(entry.r, entry.g, entry.b) == key)
            {
                if (auto name = TryGetLocalizedString(entry.resourceId); !name.empty())
                {
                    isGenericFormat = false;
                    return name;
                }
                break;
            }
        }

        isGenericFormat = true;
        auto format = TryGetLocalizedString(SR_InkToolbarColorRgbFormat);
        if (format.empty())
        {
            format = L"RGB %1!u!, %2!u!, %3!u!";
        }
        return StringUtil::FormatString(
            format, static_cast<unsigned>(color.R), static_cast<unsigned>(color.G), static_cast<unsigned>(color.B));
    }

private:
    static winrt::hstring TryGetLocalizedString(std::wstring_view resourceName)
    {
        try
        {
            return ResourceAccessor::GetLocalizedStringResource(resourceName);
        }
        catch (winrt::hresult_error const& e)
        {
            InkToolbarLogHResult(e.code(), L"palette color string lookup");
            return {};
        }
    }

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
        { 0x44, 0xC8, 0xF5, SR_InkToolbarColorNameLightBlue },
        { 0xEC, 0x00, 0x8C, SR_InkToolbarColorNamePink },
    };
};
