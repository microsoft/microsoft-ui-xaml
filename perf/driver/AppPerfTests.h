//  Copyright (c) Microsoft Corporation.  All rights reserved.

#pragma once

#include <WexTestClass.h>

namespace Microsoft::UI::Xaml::Tests::AppPerf
{
    class AppPerfTests : public WEX::TestClass<AppPerfTests>
    {
    public:
        BEGIN_TEST_CLASS(AppPerfTests)
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)

        BEGIN_TEST_METHOD(RunAppFromParameters)
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"Classification", L"Performance")
        END_TEST_METHOD()
    };
}
