// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WeakReferenceSourceNoThreadId.h"

namespace DirectUI
{
    class DependencyObject
        : public xaml::IDependencyObject
        , public xaml::ISourceInfoPrivate
        , public ctl::WeakReferenceSourceNoThreadId
    {

    };
}