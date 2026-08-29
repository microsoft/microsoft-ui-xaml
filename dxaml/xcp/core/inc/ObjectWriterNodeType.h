// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <cstdint>

// Node type descriptions for the 
// the operations of the object writer.
// These values are written into the XBF so
// MUST REMAIN STATIC.
enum class ObjectWriterNodeType : std::uint8_t
{
    None = 0,

    // stack manipulation
    PushScope = 1,
    PopScope = 2,

    // tree creation
    AddNamespace = 3,
    PushConstant = 4,
    PushResolvedType = 5,
    PushResolvedProperty = 6,
    SetValue = 7,
    AddToCollection = 8,
    AddToDictionary = 9,
    AddToDictionaryWithKey = 10,

    // type specific operations
    CheckPeerType = 11,
    SetConnectionId = 12,
    SetName = 13,
    GetResourcePropertyBag = 14,

    // custom data blob
    SetCustomRuntimeData = 15,
    SetResourceDictionaryItems = 16,
    SetDeferredProperty = 17,

    // aggregate operations
    PushScopeAddNamespace = 18,
    PushScopeGetValue = 19,
    PushScopeCreateTypeBeginInit = 20,
    PushScopeCreateTypeWithConstantBeginInit = 21,
    PushScopeCreateTypeWithTypeConvertedConstantBeginInit = 22,
    CreateTypeBeginInit = 23,
    CreateTypeWithConstantBeginInit = 24,
    CreateTypeWithTypeConvertedConstantBeginInit = 25,
    SetValueConstant = 26,
    SetValueTypeConvertedConstant = 27,
    SetValueTypeConvertedResolvedProperty = 28,
    SetValueTypeConvertedResolvedType = 29,
    SetValueFromStaticResource = 30,
    SetValueFromTemplateBinding = 31,
    SetValueFromMarkupExtension = 32,
    EndInitPopScope = 33,
    ProvideStaticResourceValue = 34,
    ProvideThemeResourceValue = 35,
    SetValueFromThemeResource = 36,

    // sentinel
    EndOfStream = 37,

    // Conditional XAML nodes
    BeginConditionalScope = 38,
    EndConditionalScope = 39,

    // more aggregate operations
    EndInitProvideValuePopScope = 40,

    ///////////////////////// temporary states during optimization   
    CreateType = 128,
    CreateTypeWithInitialValue = 129,
    BeginInit = 130,
    EndInit = 131,
    GetValue = 132,
    TypeConvertValue = 133,
    PushScopeCreateType = 134,
    PushScopeCreateTypeWithConstant = 135,
    PushScopeCreateTypeWithTypeConvertedConstant = 136,
    CreateTypeWithConstant = 137,
    CreateTypeWithTypeConvertedConstant = 138,
    ProvideValue = 139,
    ProvideTemplateBindingValue = 140,
    SetDirectiveProperty = 141,
    StreamOffsetMarker = 142,
};
