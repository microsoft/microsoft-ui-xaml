// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {

        // Covers ResourceDictionary through its IVector<IKeyValuePair<Object, Object>> interface (available
        // via QueryInterface) and the key forms accepted by the IMap interface.
        class ResourceDictionaryVectorInterfaceTests : public WEX::TestClass<ResourceDictionaryVectorInterfaceTests>
        {
        public:
            BEGIN_TEST_CLASS(ResourceDictionaryVectorInterfaceTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(VectorAppendAddsKeyValuePair)
                TEST_METHOD_PROPERTY(L"Description", L"IVector.Append inserts the pair's key and value, replaces an existing key, and ignores null.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VectorRemoveAtRemovesEntryAtIndex)
                TEST_METHOD_PROPERTY(L"Description", L"IVector.RemoveAt removes exactly the entry reported at that index.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VectorRemoveAtOutOfRangeFails)
                TEST_METHOD_PROPERTY(L"Description", L"IVector.RemoveAt with index >= Size fails with E_BOUNDS and leaves the dictionary unchanged.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VectorRemoveAtEndRemovesLastEntry)
                TEST_METHOD_PROPERTY(L"Description", L"IVector.RemoveAtEnd removes the last entry and is a no-op on an empty dictionary.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DataTemplateKeyIsAcceptedAsKey)
                TEST_METHOD_PROPERTY(L"Description", L"A DataTemplateKey with a TypeName data type can be inserted, found, looked up and removed, and is kept distinct from a string key equal to the type name.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DataTemplateKeyWithoutDataTypeIsRejected)
                TEST_METHOD_PROPERTY(L"Description", L"A DataTemplateKey whose DataType is not set is rejected as a key and never reported as present.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EmptyStringKeyIsRejected)
                TEST_METHOD_PROPERTY(L"Description", L"An empty string key is rejected (Insert fails) without changing the dictionary and is never reported as present.")
            END_TEST_METHOD()
        };

} } } } }
