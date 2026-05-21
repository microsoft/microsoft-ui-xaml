// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
namespace Microsoft { namespace UI { namespace Xaml {
            namespace Tests {
                namespace Foundation {
                    namespace Input {
                        namespace Focus {

                            class WindowLoseFocusTest : public WEX::TestClass<WindowLoseFocusTest>
                            {
                            private:
                                Platform::String^ GetResourcesPath() const;
                            public:
                                BEGIN_TEST_CLASS(WindowLoseFocusTest)
                                    TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                                    TEST_CLASS_PROPERTY(L"RunAs", L"Tailored")                                    
                                    TEST_CLASS_PROPERTY(L"Tailored:Host", L"Xaml")
                                    TEST_CLASS_PROPERTY(L"__ExecutionUnit", L";56BA1601-E088-49C5-BBB5-D90F33504A2F")
                                    TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                                END_TEST_CLASS()

                                TEST_CLASS_SETUP(ClassSetup)

                                TEST_METHOD_CLEANUP(TestCleanup)

                                BEGIN_TEST_METHOD(LoseFocusVisualsOnControlWhenWindowLosesFocus)
                                    TEST_METHOD_PROPERTY(L"Description", L"Ensures focus visuals are lost when window loses focus, and return when window regains focus.")
                                    TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                                END_TEST_METHOD()

                                BEGIN_TEST_METHOD(LoseFocusVisualsOnHyperlinkWhenWindowLosesFocus)
                                    TEST_METHOD_PROPERTY(L"Description", L"Ensures focus visuals on hyperlink are lost when window loses focus, and return when the window regains focus.")
                                    TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                                END_TEST_METHOD()
                            };
                        }
                    }
                }
            }
        }
    }
}
