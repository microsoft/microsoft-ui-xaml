// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <WexTestClass.h>

class XamlLightTargetIdMap;
class CUIElement;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Graphics {

class XamlLightTargetIdMapUnitTests : public WEX::TestClass<XamlLightTargetIdMapUnitTests>
{
public:
    BEGIN_TEST_CLASS(XamlLightTargetIdMapUnitTests)
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    TEST_METHOD(ValidateMap)

private:
    void ValidateEntryCount(int expected, _In_ XamlLightTargetIdMap& map, _In_ CDependencyObject* key);
};

} } } } }
