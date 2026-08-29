// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef ICUSTOM_RESOURCE_LOADER_H
#define ICUSTOM_RESOURCE_LOADER_H

#include "refcounting.h"

//---------------------------------------------------------------------------
//
//  Synopsis:
//      This interface is used by the parser when {CustomResource} markup extensions
//      are encountered.
//
//---------------------------------------------------------------------------
struct ICustomResourceLoader : public IObject
{
    virtual ~ICustomResourceLoader() {};

    //---------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Retrieve the resource with the given key to be set on the given property.
    //
    //  Returns:
    //      The property value.
    //
    //  Notes:
    //      objectType and propertyType are the full type names, e.g. "Microsoft.UI.Xaml.TextBlock"
    //
    //--------------------------------------------------------------------------- 
    virtual HRESULT GetResource(
        _In_ const xstring_ptr& resourceId,
        _In_ const xstring_ptr& objectType,
        _In_ const xstring_ptr& propertyName,
        _In_ const xstring_ptr& propertyType,
        _Out_ CValue *pValue
        ) = 0;
};

#endif
