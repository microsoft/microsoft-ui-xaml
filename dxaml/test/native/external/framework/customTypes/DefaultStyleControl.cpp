// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "DefaultStyleControl.h"

using namespace Microsoft::UI::Xaml;
using namespace Tests::Native::External::Framework;

DefaultStyleControl::DefaultStyleControl()
{
    DefaultStyleKey = DefaultStyleControl::typeid->FullName;
}
