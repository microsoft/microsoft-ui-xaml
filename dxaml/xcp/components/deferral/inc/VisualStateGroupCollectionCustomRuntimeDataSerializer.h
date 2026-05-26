// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CustomRuntimeDataSerializer.h"
#include <StreamOffsetToken.h>

class XamlBinaryFormatSubWriter2;
class XamlBinaryFormatSubReader2;

class VisualStateEssence;
class VisualStateGroupEssence;
class VisualTransitionEssence;
class VisualTransitionTableOptimizedLookup;
class VisualStateGroupCollectionCustomRuntimeData;
struct VisualStateIndexes;

namespace CustomRuntimeDataSerializationHelpers
{
    template <>
     _Check_return_ HRESULT Serialize<VisualStateEssence>(
        _In_ const VisualStateEssence& target,
        _In_ XamlBinaryFormatSubWriter2* writer,
        _In_ const std::vector<unsigned int>& streamOffsetTokenTable);

    template <>
     _Check_return_ HRESULT Serialize<VisualStateGroupEssence>(
        _In_ const VisualStateGroupEssence& target,
        _In_ XamlBinaryFormatSubWriter2* writer,
        _In_ const std::vector<unsigned int>& streamOffsetTokenTable);

    template <>
     _Check_return_ HRESULT Serialize<VisualTransitionEssence>(
        _In_ const VisualTransitionEssence& target,
        _In_ XamlBinaryFormatSubWriter2* writer,
        _In_ const std::vector<unsigned int>& streamOffsetTokenTable);

    template <>
     _Check_return_ HRESULT Serialize<VisualTransitionTableOptimizedLookup>(
        _In_ const VisualTransitionTableOptimizedLookup& target,
        _In_ XamlBinaryFormatSubWriter2* writer,
        _In_ const std::vector<unsigned int>& streamOffsetTokenTable);

    template <>
     _Check_return_ HRESULT Serialize<VisualStateIndexes>(
        _In_ const VisualStateIndexes& target,
        _In_ XamlBinaryFormatSubWriter2* writer,
        _In_ const std::vector<unsigned int>& streamOffsetTokenTable);

    template <>
     _Check_return_ HRESULT Serialize<VisualStateGroupCollectionCustomRuntimeData>(
        _In_ const VisualStateGroupCollectionCustomRuntimeData& target,
        _In_ XamlBinaryFormatSubWriter2* writer,
        _In_ const std::vector<unsigned int>& streamOffsetTokenTable);

    template<>
    VisualStateGroupCollectionCustomRuntimeData Deserialize<VisualStateGroupCollectionCustomRuntimeData>(
        _In_  XamlBinaryFormatSubReader2* reader,
        _In_ CustomWriterRuntimeDataTypeIndex typeIndex);

    template<>
    VisualStateEssence Deserialize<VisualStateEssence>(
        _In_  XamlBinaryFormatSubReader2* reader,
        _In_ CustomWriterRuntimeDataTypeIndex typeIndex);

    template<>
    VisualStateGroupEssence Deserialize<VisualStateGroupEssence>(
        _In_  XamlBinaryFormatSubReader2* reader);

    template<>
    VisualTransitionEssence Deserialize<VisualTransitionEssence>(
        _In_  XamlBinaryFormatSubReader2* reader);

    template<>
    VisualTransitionTableOptimizedLookup Deserialize<VisualTransitionTableOptimizedLookup>(
        _In_  XamlBinaryFormatSubReader2* reader);

    template<>
    VisualStateIndexes Deserialize<VisualStateIndexes>(
        _In_  XamlBinaryFormatSubReader2* reader);
}

