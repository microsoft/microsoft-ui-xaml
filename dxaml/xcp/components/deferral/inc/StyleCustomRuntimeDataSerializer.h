// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CustomRuntimeDataSerializer.h"
#include <StreamOffsetToken.h>

class XamlBinaryFormatSubWriter2;
class XamlBinaryFormatSubReader2;

class StyleSetterEssence;
class StyleCustomRuntimeData;

namespace CustomRuntimeDataSerializationHelpers
{
    template<>
     _Check_return_ HRESULT Serialize<StyleSetterEssence>(
        _In_ const StyleSetterEssence& target,
        _In_ XamlBinaryFormatSubWriter2* writer,
        _In_ const std::vector<unsigned int>& streamOffsetTokenTable);

    template<>
    StyleSetterEssence Deserialize<StyleSetterEssence>(
        _In_  XamlBinaryFormatSubReader2* reader);

    template<>
     _Check_return_ HRESULT Serialize<StyleCustomRuntimeData>(
        _In_ const StyleCustomRuntimeData& target,
        _In_ XamlBinaryFormatSubWriter2* writer,
        _In_ const std::vector<unsigned int>& streamOffsetTokenTable);

    template<>
    StyleCustomRuntimeData Deserialize<StyleCustomRuntimeData>(
        _In_  XamlBinaryFormatSubReader2* reader,
        _In_ CustomWriterRuntimeDataTypeIndex typeIndex);
}

