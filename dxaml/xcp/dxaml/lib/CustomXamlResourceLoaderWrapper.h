// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef CUSTOM_RESOURCE_LOADER_WRAPPER_H
#define CUSTOM_RESOURCE_LOADER_WRAPPER_H

#include "ICustomResourceLoader.h"
#include "refcounting.h"

//---------------------------------------------------------------------------
//
//  Synopsis:
//      This class implements the internal (core) ICustomResourceLoader interface, and
//      forwards its methods to an implementation of the external (DirectUI) ICustomXamlResourceLoader interface.
//
//  Notes:
//      The purpose of this class is to avoid having core depend on DirectUI (in the form of
//      the parser depending on ICustomXamlResourceLoader).
//
//---------------------------------------------------------------------------
class CustomXamlResourceLoaderWrapper final : public ICustomResourceLoader, public CReferenceCount
{
public:
    CustomXamlResourceLoaderWrapper(_In_ xaml::Resources::ICustomXamlResourceLoader *pWinrtLoader);
    ~CustomXamlResourceLoaderWrapper() override;

    FORWARD_ADDREF_RELEASE(CReferenceCount);

    _Check_return_ HRESULT GetResource(
        _In_ const xstring_ptr &resourceId,
        _In_ const xstring_ptr &objectType,
        _In_ const xstring_ptr &propertyName,
        _In_ const xstring_ptr &propertyType,
        _Out_ CValue *pValue
        ) override;

    _Check_return_ xaml::Resources::ICustomXamlResourceLoader* GetLoader();

private:
    CustomXamlResourceLoaderWrapper(const CustomXamlResourceLoaderWrapper &source);
    CustomXamlResourceLoaderWrapper& operator=(const CustomXamlResourceLoaderWrapper &right);

    xaml::Resources::ICustomXamlResourceLoader *m_pWinrtLoader;
};

#endif
