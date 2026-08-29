// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Windows { namespace UI { namespace Xaml { namespace Tests {

    class PropertyValueWrapper
    {
    public:
        ~PropertyValueWrapper();
        static wf::IPropertyValueStatics* GetPropertyValueStatics();

    private:
        static ctl::ComPtr<wf::IPropertyValueStatics> s_spValueFactory;
    };
}}}}
