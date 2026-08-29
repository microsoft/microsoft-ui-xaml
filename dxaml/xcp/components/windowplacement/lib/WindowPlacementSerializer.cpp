// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "WindowPlacementSerializer.h"
#include "WindowPlacementFormat.h"

#include <windows.h>
#include <wincrypt.h>

namespace WindowPlacement
{
    namespace
    {
        void AppendU16(std::vector<uint8_t>& out, uint16_t value)
        {
            out.push_back(static_cast<uint8_t>(value & 0xFF));
            out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        }

        void AppendU32(std::vector<uint8_t>& out, uint32_t value)
        {
            out.push_back(static_cast<uint8_t>(value & 0xFF));
            out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
            out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
            out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
        }

        void AppendI32(std::vector<uint8_t>& out, int32_t value)
        {
            AppendU32(out, static_cast<uint32_t>(value));
        }

        void AppendRectField(std::vector<uint8_t>& out, WindowPlacementTag tag, const WindowPlacementRect& rect)
        {
            AppendU16(out, static_cast<uint16_t>(tag));
            AppendU16(out, c_rectValueLength);
            AppendI32(out, rect.left);
            AppendI32(out, rect.top);
            AppendI32(out, rect.right);
            AppendI32(out, rect.bottom);
        }

        void AppendU32Field(std::vector<uint8_t>& out, WindowPlacementTag tag, uint32_t value)
        {
            AppendU16(out, static_cast<uint16_t>(tag));
            AppendU16(out, c_uint32ValueLength);
            AppendU32(out, value);
        }

        void AppendGuidField(std::vector<uint8_t>& out, WindowPlacementTag tag, const GUID& guid)
        {
            AppendU16(out, static_cast<uint16_t>(tag));
            AppendU16(out, c_guidValueLength);
            AppendU32(out, guid.Data1);
            AppendU16(out, guid.Data2);
            AppendU16(out, guid.Data3);
            for (int i = 0; i < 8; ++i)
            {
                out.push_back(guid.Data4[i]);
            }
        }

        // Reads little-endian scalars out of a bounds-checked field value.
        uint16_t ReadU16(const uint8_t* p)
        {
            return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
        }

        uint32_t ReadU32(const uint8_t* p)
        {
            return static_cast<uint32_t>(p[0]) |
                   (static_cast<uint32_t>(p[1]) << 8) |
                   (static_cast<uint32_t>(p[2]) << 16) |
                   (static_cast<uint32_t>(p[3]) << 24);
        }

        int32_t ReadI32(const uint8_t* p)
        {
            return static_cast<int32_t>(ReadU32(p));
        }

        bool CoordinateInRange(int32_t v)
        {
            return v >= -c_maxCoordinateMagnitude && v <= c_maxCoordinateMagnitude;
        }

        bool ReadRect(const uint8_t* value, uint16_t length, WindowPlacementRect& rect)
        {
            if (length != c_rectValueLength)
            {
                return false; // known fixed-width tag with the wrong length rejects the blob
            }
            rect.left = ReadI32(value);
            rect.top = ReadI32(value + 4);
            rect.right = ReadI32(value + 8);
            rect.bottom = ReadI32(value + 12);
            return CoordinateInRange(rect.left) && CoordinateInRange(rect.top) &&
                   CoordinateInRange(rect.right) && CoordinateInRange(rect.bottom);
        }

        // Win32 SW_* show states, SW_HIDE (0) through SW_FORCEMINIMIZE (11). An out-of-range value
        // is treated as an absent tag rather than rejecting the whole blob.
        bool IsKnownShowCmd(uint32_t value)
        {
            return value <= SW_FORCEMINIMIZE;
        }
    }

    std::vector<uint8_t> SerializePlacement(const WindowPlacementData& data)
    {
        std::vector<uint8_t> out;

        if (!data.hasNormalRect)
        {
            return out; // nothing to persist without a normal rectangle
        }

        // Build the field body first, then prepend the header with the final total length. Fields
        // are emitted once, in ascending tag order, so a given placement is byte-for-byte canonical.
        std::vector<uint8_t> body;

        AppendRectField(body, WindowPlacementTag::NormalRect, data.normalRect);

        if (data.hasWorkArea)
        {
            AppendRectField(body, WindowPlacementTag::WorkArea, data.workArea);
        }
        if (data.hasArrangeRect)
        {
            AppendRectField(body, WindowPlacementTag::ArrangeRect, data.arrangeRect);
        }
        if (data.hasDpi)
        {
            AppendU32Field(body, WindowPlacementTag::Dpi, data.dpi);
        }
        if (data.hasShowCmd)
        {
            AppendU32Field(body, WindowPlacementTag::ShowCmd, data.showCmd);
        }
        if (data.hasFlags)
        {
            AppendU32Field(body, WindowPlacementTag::Flags, data.flags & WindowPlacementFlags_KnownMask);
        }
        if (data.hasDeviceName && !data.deviceName.empty())
        {
            // Clamp to the maximum on-wire device-name length (31 UTF-16 code units). Real GDI
            // device names are well under this bound.
            size_t codeUnits = data.deviceName.size();
            const size_t maxCodeUnits = c_maxDeviceNameValueLength / sizeof(wchar_t);
            if (codeUnits > maxCodeUnits)
            {
                codeUnits = maxCodeUnits;
            }
            AppendU16(body, static_cast<uint16_t>(WindowPlacementTag::DeviceName));
            AppendU16(body, static_cast<uint16_t>(codeUnits * sizeof(wchar_t)));
            for (size_t i = 0; i < codeUnits; ++i)
            {
                wchar_t ch = data.deviceName[i];
                body.push_back(static_cast<uint8_t>(ch & 0xFF));
                body.push_back(static_cast<uint8_t>((ch >> 8) & 0xFF));
            }
        }
        if (data.hasVirtualDesktopId)
        {
            AppendGuidField(body, WindowPlacementTag::VirtualDesktopId, data.virtualDesktopId);
        }

        const uint32_t totalLength = static_cast<uint32_t>(c_headerLength + body.size());

        out.reserve(totalLength);
        for (size_t i = 0; i < c_headerMagicLength; ++i)
        {
            out.push_back(c_headerMagic[i]);
        }
        AppendU16(out, c_majorVersionV1);
        AppendU16(out, c_minorVersionV1);
        AppendU32(out, totalLength);
        out.insert(out.end(), body.begin(), body.end());

        return out;
    }

    bool TryParsePlacement(const uint8_t* bytes, size_t length, WindowPlacementData& data)
    {
        data = WindowPlacementData{};

        if (bytes == nullptr || length < c_headerLength || length > c_maxBlobLength)
        {
            return false;
        }

        // Header.
        for (size_t i = 0; i < c_headerMagicLength; ++i)
        {
            if (bytes[i] != c_headerMagic[i])
            {
                return false;
            }
        }
        const uint16_t major = ReadU16(bytes + 4);
        if (major != c_majorVersionV1)
        {
            return false; // must-understand: an unknown major rejects the blob
        }
        // minor at bytes + 6 is tolerated and ignored.
        const uint32_t totalLength = ReadU32(bytes + 8);
        if (totalLength != length)
        {
            return false; // total length must equal the decoded byte count
        }

        WindowPlacementData parsed;

        size_t offset = c_headerLength;
        while (offset < totalLength)
        {
            if (offset + c_fieldHeaderLength > totalLength)
            {
                return false; // a partial field header
            }
            const uint16_t tag = ReadU16(bytes + offset);
            const uint16_t valueLength = ReadU16(bytes + offset + 2);
            const size_t valueOffset = offset + c_fieldHeaderLength;
            if (valueOffset + valueLength > totalLength)
            {
                return false; // value would run past the blob
            }
            const uint8_t* value = bytes + valueOffset;

            switch (static_cast<WindowPlacementTag>(tag))
            {
            case WindowPlacementTag::NormalRect:
                if (!ReadRect(value, valueLength, parsed.normalRect)) { return false; }
                parsed.hasNormalRect = true;
                break;

            case WindowPlacementTag::WorkArea:
                if (!ReadRect(value, valueLength, parsed.workArea)) { return false; }
                parsed.hasWorkArea = true;
                break;

            case WindowPlacementTag::ArrangeRect:
                if (!ReadRect(value, valueLength, parsed.arrangeRect)) { return false; }
                parsed.hasArrangeRect = true;
                break;

            case WindowPlacementTag::Dpi:
                if (valueLength != c_uint32ValueLength) { return false; }
                parsed.dpi = ReadU32(value);
                parsed.hasDpi = true;
                break;

            case WindowPlacementTag::ShowCmd:
                if (valueLength != c_uint32ValueLength) { return false; }
                {
                    const uint32_t showCmd = ReadU32(value);
                    if (IsKnownShowCmd(showCmd))
                    {
                        parsed.showCmd = showCmd;
                        parsed.hasShowCmd = true;
                    }
                    // An unknown show command is treated as an absent tag.
                }
                break;

            case WindowPlacementTag::Flags:
                if (valueLength != c_uint32ValueLength) { return false; }
                parsed.flags = ReadU32(value) & WindowPlacementFlags_KnownMask;
                parsed.hasFlags = true;
                break;

            case WindowPlacementTag::DeviceName:
                if ((valueLength % sizeof(wchar_t)) != 0 || valueLength > c_maxDeviceNameValueLength)
                {
                    return false; // device-name length must be even and within bounds
                }
                {
                    std::wstring name;
                    name.reserve(valueLength / sizeof(wchar_t));
                    for (uint16_t i = 0; i < valueLength; i += static_cast<uint16_t>(sizeof(wchar_t)))
                    {
                        name.push_back(static_cast<wchar_t>(ReadU16(value + i)));
                    }
                    parsed.deviceName = std::move(name);
                    parsed.hasDeviceName = true;
                }
                break;

            case WindowPlacementTag::VirtualDesktopId:
                if (valueLength != c_guidValueLength) { return false; }
                parsed.virtualDesktopId.Data1 = ReadU32(value);
                parsed.virtualDesktopId.Data2 = ReadU16(value + 4);
                parsed.virtualDesktopId.Data3 = ReadU16(value + 6);
                for (int i = 0; i < 8; ++i)
                {
                    parsed.virtualDesktopId.Data4[i] = value[8 + i];
                }
                parsed.hasVirtualDesktopId = true;
                break;

            default:
                // Unknown tag: skip using its encoded length.
                break;
            }

            offset = valueOffset + valueLength;
        }

        if (offset != totalLength)
        {
            return false; // the final field must end exactly at the total-length boundary
        }
        if (!parsed.hasNormalRect)
        {
            return false; // a placement without a normal rectangle is not usable
        }

        data = std::move(parsed);
        return true;
    }

    std::wstring SerializePlacementToBase64(const WindowPlacementData& data)
    {
        const std::vector<uint8_t> blob = SerializePlacement(data);
        if (blob.empty())
        {
            return std::wstring();
        }

        DWORD charCount = 0;
        if (!CryptBinaryToStringW(
                blob.data(),
                static_cast<DWORD>(blob.size()),
                CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                nullptr,
                &charCount) ||
            charCount == 0)
        {
            return std::wstring();
        }

        std::wstring result(charCount, L'\0');
        if (!CryptBinaryToStringW(
                blob.data(),
                static_cast<DWORD>(blob.size()),
                CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                result.data(),
                &charCount))
        {
            return std::wstring();
        }

        // charCount excludes the terminating NUL that CryptBinaryToStringW does not count here;
        // trim any trailing NULs the buffer picked up.
        result.resize(wcslen(result.c_str()));
        return result;
    }

    bool TryParsePlacementFromBase64(const std::wstring& base64, WindowPlacementData& data)
    {
        data = WindowPlacementData{};
        if (base64.empty())
        {
            return false;
        }

        DWORD byteCount = 0;
        if (!CryptStringToBinaryW(
                base64.c_str(),
                static_cast<DWORD>(base64.size()),
                CRYPT_STRING_BASE64,
                nullptr,
                &byteCount,
                nullptr,
                nullptr) ||
            byteCount == 0 || byteCount > c_maxBlobLength)
        {
            return false;
        }

        std::vector<uint8_t> blob(byteCount);
        if (!CryptStringToBinaryW(
                base64.c_str(),
                static_cast<DWORD>(base64.size()),
                CRYPT_STRING_BASE64,
                blob.data(),
                &byteCount,
                nullptr,
                nullptr))
        {
            return false;
        }
        blob.resize(byteCount);

        return TryParsePlacement(blob, data);
    }
}
