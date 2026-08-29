// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "PropertyValueWrapper.h"
#include <WexTestClass.h>
#include <XamlLogging.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {

ctl::ComPtr<wf::IPropertyValueStatics> PropertyValueWrapper::s_spValueFactory;

PropertyValueWrapper::~PropertyValueWrapper()
{
    if (s_spValueFactory)
    {
        s_spValueFactory.Reset();
        wf::Uninitialize();
    }
}

wf::IPropertyValueStatics* PropertyValueWrapper::GetPropertyValueStatics()
{
    if (!s_spValueFactory)
    {
        LogThrow_IfFailed(wf::Initialize(RO_INIT_MULTITHREADED));
        LogThrow_IfFailed(ctl::GetActivationFactory(wrl_wrappers::HStringReference(STR_LEN_PAIR(RuntimeClass_Windows_Foundation_PropertyValue)).Get(), &s_spValueFactory));
    }

    return s_spValueFactory.Get();
}

}}}}
