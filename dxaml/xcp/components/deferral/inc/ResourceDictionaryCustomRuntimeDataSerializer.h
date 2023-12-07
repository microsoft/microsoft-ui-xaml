// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CustomRuntimeDataSerializer.h"
#include <StreamOffsetToken.h>

class XamlBinaryFormatSubWriter2;
class XamlBinaryFormatSubReader2;
class ResourceDictionaryCustomRuntimeData;

namespace CustomRuntimeDataSerializationHelpers
{
    template<>
     _Check_return_ HRESULT Serialize<ResourceDictionaryCustomRuntimeData>(
        _In_ const ResourceDictionaryCustomRuntimeData& target,
        _In_ XamlBinaryFormatSubWriter2* writer,
        _In_ const std::vector<unsigned int>& streamOffsetTokenTable);

    template<>
    ResourceDictionaryCustomRuntimeData Deserialize<ResourceDictionaryCustomRuntimeData>(
        _In_ XamlBinaryFormatSubReader2* reader,
        _In_ CustomWriterRuntimeDataTypeIndex typeIndex);
}
