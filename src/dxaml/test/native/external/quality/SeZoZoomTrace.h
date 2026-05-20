// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

using namespace Microsoft::UI::Xaml::Controls;

namespace Microsoft { namespace UI { namespace Xaml {
            namespace Tests {
                namespace Quality  {
                    class SeZoZoomTrace
                    {
                    public:
                        BEGIN_TEST_CLASS(SeZoZoomTrace)
                            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                        END_TEST_CLASS()

                        TEST_CLASS_SETUP(ClassSetup)

                        TEST_METHOD_CLEANUP(TestCleanup)

                        BEGIN_TEST_METHOD(VerifyZoomingTraceWheel)
                            TEST_METHOD_PROPERTY(L"Description", L"Validates that the correct event is fired when Semantic Zoom zooms in or out with the mouse wheel.")
                            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                        END_TEST_METHOD()

                        BEGIN_TEST_METHOD(VerifyZoomingTracePinch)
                            TEST_METHOD_PROPERTY(L"Description", L"Validates that the correct event is fired when Semantic Zoom zooms in or out with pinch.")
                            TEST_METHOD_PROPERTY(L"Ignore", L"True") //Disabling this test, it is proving to be unstable in CINCH runs.
                            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                        END_TEST_METHOD()

                        BEGIN_TEST_METHOD(VerifyZoomingTraceTap)
                            TEST_METHOD_PROPERTY(L"Description", L"Validates that the correct event is fired when Semantic Zoom zooms in with tap.")
                            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                        END_TEST_METHOD()

                        void SetUpPage(Grid^ &grid, SemanticZoom^ &seZo, GridView^ &innerView, GridView^ &outerView, GridViewItem^ &innerItem, GridViewItem^ &outerItem);
                    };
                }
            }
        }
    }
}
