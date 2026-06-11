// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <wil\resource.h>
#include <wil\winrt.h>
#include "XamlOM.WinUI.h"
#include <functional>

void inline CoDeallocPropertyChainSource(_Inout_ PropertyChainSource* source)
{
    SysFreeString(source->TargetType);
    SysFreeString(source->Name);
    SysFreeString(source->SrcInfo.FileName);
    SysFreeString(source->SrcInfo.Hash);
}

void inline CoDeallocPropertyChainValue(_Inout_ PropertyChainValue* value)
{
    SysFreeString(value->DeclaringType);
    SysFreeString(value->ItemType);
    SysFreeString(value->PropertyName);
    SysFreeString(value->Type);
    SysFreeString(value->Value);
    SysFreeString(value->ValueType);
}

void inline CoDeallocCollectionElementValue(_Inout_ CollectionElementValue* value)
{
    SysFreeString(value->Value);
    SysFreeString(value->ValueType);
}

void inline CoDeallocVisualElement(_Inout_ VisualElement* element)
{
    SysFreeString(element->SrcInfo.FileName);
    SysFreeString(element->SrcInfo.Hash);
    SysFreeString(element->Type);
    SysFreeString(element->Name);
}

void inline CoDeallocSourceInfo(_Inout_ SourceInfo* info)
{
    SysFreeString(info->Hash);
    SysFreeString(info->FileName);
}
// This file is meant to story any wil like helpers that we want to use.
namespace wil
{
    // Helpers for managing lifetime of XamlDiagnostics related structs
    typedef unique_struct<PropertyChainSource, decltype(&::CoDeallocPropertyChainSource), ::CoDeallocPropertyChainSource> unique_propertychainsource;
    typedef unique_struct<PropertyChainValue, decltype(&::CoDeallocPropertyChainValue), ::CoDeallocPropertyChainValue> unique_propertychainvalue;
    typedef unique_struct<CollectionElementValue, decltype(&::CoDeallocCollectionElementValue), ::CoDeallocCollectionElementValue> unique_collectionelementvalue;
    typedef unique_struct<VisualElement, decltype(&::CoDeallocVisualElement), ::CoDeallocVisualElement> unique_visualelement;
    typedef unique_struct<SourceInfo, decltype(&::CoDeallocSourceInfo), ::CoDeallocSourceInfo> unique_sourceinfo;
}

