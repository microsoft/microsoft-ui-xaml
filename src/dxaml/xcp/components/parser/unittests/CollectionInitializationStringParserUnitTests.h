// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>
#include <CollectionInitializationStringParser.h>

class ParserErrorReporter;

namespace Microsoft {
    namespace UI {
        namespace Xaml {
            namespace Tests {
                namespace Framework {

                    class CollectionInitializationStringParserUnitTests : public WEX::TestClass<CollectionInitializationStringParserUnitTests>
                    {
                    public:
                        BEGIN_TEST_CLASS(CollectionInitializationStringParserUnitTests)
                            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                        END_TEST_CLASS()
                        
                        BEGIN_TEST_METHOD(VerifyValidInitializationStringParsing)
                            TEST_METHOD_PROPERTY(L"Description", L"Validates that an initialization string with valid syntax is parsed into a list of strings.")
                        END_TEST_METHOD()

                        BEGIN_TEST_METHOD(ThrowsInvalidArgumentForInvalidInitializationString)
                            TEST_METHOD_PROPERTY(L"Description", L"Validates that we throw an invalid argument for invalid initialization string.")
                        END_TEST_METHOD()

                    private:
                        void VerifyOutput(Jupiter::stack_vector<xstring_ptr, 8>& output, std::vector<xstring_ptr>& correctOutput);
                    };
                }
            }
        }
    }
}
