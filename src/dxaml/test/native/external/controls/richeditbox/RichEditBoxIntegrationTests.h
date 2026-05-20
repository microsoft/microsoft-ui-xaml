// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace RichEditBox {

    class RichEditBoxIntegrationTests : public WEX::TestClass<RichEditBoxIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(RichEditBoxIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"a62e3c8d-69d4-44de-95b5-a62be5062286;57e0de30-efb3-4001-9ccc-b38032fd1974;cbb6c59f-3ce2-4ed3-8eaa-f598566c2755")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyContextMenuRaisesCutCopyPasteEvents)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that using the context menu buttons to cut, copy, and paste raises the associated events.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySelectingTextWithTouchShowsSelectionFlyout)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that selecting text with touch shows the selection flyout.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()
    };

} } } } } }
