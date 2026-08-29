// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <MockClassInfo.h>
#include <ThreadLocalStorage.h>

using namespace Microsoft::UI::Xaml::Tests::Metadata;

const CREATEPFN CClassInfo::GetCoreConstructor() const
{
    auto mock = TlsProvider<ClassInfoCallbacks>::GetWrappedObject();
    return mock ? mock->GetCoreConstructor(this) : nullptr;
}

const CObjectDependencyProperty* CClassInfo::GetFirstObjectProperty() const
{
    return nullptr;
}

_Check_return_ HRESULT CClassInfo::RunClassConstructorIfNecessary()
{
    auto mock = TlsProvider<ClassInfoCallbacks>::GetWrappedObject();
    return mock ? mock->RunClassConstructorIfNecessary() : S_OK;
}