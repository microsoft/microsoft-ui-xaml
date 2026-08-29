// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "D2DFactoryUnitTests.h"
#include "D2DAccelerated.h"
#include "MockD2D1.h"
#include "XamlLogging.h"

using namespace ::Windows::UI::Xaml::Tests::Graphics;

/* ------------------------------------------------------------------------------------------
    D2DFactory Unit Tests
-------------------------------------------------------------------------------------------*/

void D2DFactoryUnitTests::Creation()
{
    CreateD2D1FactoryDetour d2d1Detour;
    xref_ptr<CD2DFactory> factory;
    VERIFY_SUCCEEDED(CD2DFactory::Create(factory.ReleaseAndGetAddressOf()));
    VERIFY_IS_NOT_NULL(factory->GetFactory());
}

