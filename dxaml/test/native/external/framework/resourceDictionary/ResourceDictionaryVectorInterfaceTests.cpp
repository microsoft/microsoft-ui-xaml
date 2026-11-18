// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <StringUtilities.h>
#include "ResourceDictionaryVectorInterfaceTests.h"

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {

    namespace
    {
        typedef IKeyValuePair<Object^, Object^> ResourcePair;
        typedef IVector<ResourcePair^> ResourcePairVector;

        const HRESULT c_eBounds = static_cast<HRESULT>(0x8000000B);

        // ResourceDictionary answers QueryInterface for IVector<IKeyValuePair<Object, Object>>.
        ResourcePairVector^ AsPairVector(ResourceDictionary^ dictionary)
        {
            auto vector = dynamic_cast<ResourcePairVector^>(static_cast<Object^>(dictionary));
            VERIFY_IS_NOT_NULL(vector, L"ResourceDictionary must expose IVector<IKeyValuePair<Object, Object>>.");
            return vector;
        }

        // Builds a dictionary with string keys "Key0".."Key{count-1}" mapped to distinct brushes.
        ResourceDictionary^ CreateDictionary(unsigned int count)
        {
            auto dictionary = ref new ResourceDictionary();
            for (unsigned int i = 0; i < count; i++)
            {
                dictionary->Insert(ref new String(L"Key") + i.ToString(), ref new SolidColorBrush());
            }
            VERIFY_ARE_EQUAL(count, dictionary->Size);
            return dictionary;
        }

        Object^ BoxTypeName(Platform::Type^ type)
        {
            return ::Windows::UI::Xaml::Interop::TypeName(type);
        }

        // Runs an action that must fail and returns its HRESULT.
        template <typename TAction>
        HRESULT CaptureFailure(TAction action)
        {
            try
            {
                action();
            }
            catch (Exception^ e)
            {
                return static_cast<HRESULT>(e->HResult);
            }
            return S_OK;
        }

        bool SetupXaml()
        {
            TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool CleanupXaml()
        {
            TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }
    }

    // ---------------------------------------------------------------------------------------------
    // ResourceDictionaryVectorInterfaceTests
    // ---------------------------------------------------------------------------------------------

    bool ResourceDictionaryVectorInterfaceTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ResourceDictionaryVectorInterfaceTests::TestSetup() { return SetupXaml(); }
    bool ResourceDictionaryVectorInterfaceTests::TestCleanup() { return CleanupXaml(); }

    void ResourceDictionaryVectorInterfaceTests::VectorAppendAddsKeyValuePair()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto appendedBrush = ref new SolidColorBrush();
            auto source = ref new ResourceDictionary();
            source->Insert(L"AppendedKey", appendedBrush);
            ResourcePair^ pair = source->First()->Current;
            VERIFY_ARE_STRINGS_EQUAL(L"AppendedKey", safe_cast<String^>(pair->Key)->Data());

            auto existingBrush = ref new SolidColorBrush();
            auto target = ref new ResourceDictionary();
            target->Insert(L"ExistingKey", existingBrush);
            auto vector = AsPairVector(target);

            vector->Append(pair);
            VERIFY_ARE_EQUAL(2u, target->Size);
            VERIFY_ARE_EQUAL(2u, vector->Size);
            VERIFY_IS_TRUE(target->HasKey(L"AppendedKey"));
            VERIFY_IS_TRUE(safe_cast<SolidColorBrush^>(target->Lookup(L"AppendedKey")) == appendedBrush, L"Append must store the pair's value.");
            VERIFY_IS_TRUE(safe_cast<SolidColorBrush^>(target->Lookup(L"ExistingKey")) == existingBrush, L"Append must not disturb other entries.");

            // Appending a pair whose key already exists replaces the value rather than adding an entry.
            auto replacementBrush = ref new SolidColorBrush();
            auto replacementSource = ref new ResourceDictionary();
            replacementSource->Insert(L"ExistingKey", replacementBrush);
            vector->Append(replacementSource->First()->Current);
            VERIFY_ARE_EQUAL(2u, target->Size);
            VERIFY_IS_TRUE(safe_cast<SolidColorBrush^>(target->Lookup(L"ExistingKey")) == replacementBrush);

            // A null pair is ignored.
            vector->Append(nullptr);
            VERIFY_ARE_EQUAL(2u, target->Size);
        });
    }

    void ResourceDictionaryVectorInterfaceTests::VectorRemoveAtRemovesEntryAtIndex()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto dictionary = CreateDictionary(3);
            auto vector = AsPairVector(dictionary);

            auto keyAt0 = safe_cast<String^>(vector->GetAt(0)->Key);
            auto keyAt1 = safe_cast<String^>(vector->GetAt(1)->Key);
            auto keyAt2 = safe_cast<String^>(vector->GetAt(2)->Key);

            vector->RemoveAt(1);
            VERIFY_ARE_EQUAL(2u, dictionary->Size);
            VERIFY_IS_FALSE(dictionary->HasKey(keyAt1), L"The entry at the removed index must be gone.");
            VERIFY_IS_TRUE(dictionary->HasKey(keyAt0));
            VERIFY_IS_TRUE(dictionary->HasKey(keyAt2));

            auto firstKey = safe_cast<String^>(vector->GetAt(0)->Key);
            vector->RemoveAt(0);
            VERIFY_ARE_EQUAL(1u, dictionary->Size);
            VERIFY_IS_FALSE(dictionary->HasKey(firstKey));
        });
    }

    void ResourceDictionaryVectorInterfaceTests::VectorRemoveAtOutOfRangeFails()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto dictionary = CreateDictionary(2);
            auto vector = AsPairVector(dictionary);

            VERIFY_ARE_EQUAL(c_eBounds, CaptureFailure([&]() { vector->RemoveAt(2); }));
            VERIFY_ARE_EQUAL(2u, dictionary->Size);
            VERIFY_IS_TRUE(dictionary->HasKey(L"Key0"));
            VERIFY_IS_TRUE(dictionary->HasKey(L"Key1"));

            auto empty = AsPairVector(ref new ResourceDictionary());
            VERIFY_ARE_EQUAL(c_eBounds, CaptureFailure([&]() { empty->RemoveAt(0); }));
            VERIFY_ARE_EQUAL(0u, empty->Size);
        });
    }

    void ResourceDictionaryVectorInterfaceTests::VectorRemoveAtEndRemovesLastEntry()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto dictionary = CreateDictionary(2);
            auto vector = AsPairVector(dictionary);

            auto firstKey = safe_cast<String^>(vector->GetAt(0)->Key);
            auto lastKey = safe_cast<String^>(vector->GetAt(1)->Key);

            vector->RemoveAtEnd();
            VERIFY_ARE_EQUAL(1u, dictionary->Size);
            VERIFY_IS_FALSE(dictionary->HasKey(lastKey), L"RemoveAtEnd must remove the entry at the last index.");
            VERIFY_IS_TRUE(dictionary->HasKey(firstKey));

            vector->RemoveAtEnd();
            VERIFY_ARE_EQUAL(0u, dictionary->Size);

            // Removing from an empty dictionary is a no-op.
            VERIFY_ARE_EQUAL(S_OK, CaptureFailure([&]() { vector->RemoveAtEnd(); }));
            VERIFY_ARE_EQUAL(0u, dictionary->Size);
        });
    }

    void ResourceDictionaryVectorInterfaceTests::DataTemplateKeyIsAcceptedAsKey()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto dictionary = ref new ResourceDictionary();
            auto buttonTemplate = ref new DataTemplate();

            dictionary->Insert(ref new DataTemplateKey(BoxTypeName(Button::typeid)), buttonTemplate);
            VERIFY_ARE_EQUAL(1u, dictionary->Size);

            // Keys are matched by data type, not by DataTemplateKey instance.
            VERIFY_IS_TRUE(dictionary->HasKey(ref new DataTemplateKey(BoxTypeName(Button::typeid))));
            VERIFY_IS_FALSE(dictionary->HasKey(ref new DataTemplateKey(BoxTypeName(TextBlock::typeid))));
            VERIFY_IS_TRUE(safe_cast<DataTemplate^>(dictionary->Lookup(ref new DataTemplateKey(BoxTypeName(Button::typeid)))) == buttonTemplate);

            // A DataTemplateKey is an implicit (type) key, kept apart from an ordinary string key with the same
            // text: the type's full name is not found as a string key, and inserting it adds a distinct entry.
            String^ typeNameAsString = Button::typeid->FullName;
            VERIFY_IS_FALSE(dictionary->HasKey(typeNameAsString));
            auto stringKeyedBrush = ref new SolidColorBrush();
            dictionary->Insert(typeNameAsString, stringKeyedBrush);
            VERIFY_ARE_EQUAL(2u, dictionary->Size);
            VERIFY_IS_TRUE(safe_cast<SolidColorBrush^>(dictionary->Lookup(typeNameAsString)) == stringKeyedBrush);
            VERIFY_IS_TRUE(safe_cast<DataTemplate^>(dictionary->Lookup(ref new DataTemplateKey(BoxTypeName(Button::typeid)))) == buttonTemplate);

            // Removing the DataTemplateKey removes only the implicit entry.
            dictionary->Remove(ref new DataTemplateKey(BoxTypeName(Button::typeid)));
            VERIFY_ARE_EQUAL(1u, dictionary->Size);
            VERIFY_IS_FALSE(dictionary->HasKey(ref new DataTemplateKey(BoxTypeName(Button::typeid))));
            VERIFY_IS_TRUE(dictionary->HasKey(typeNameAsString));
        });
    }

    void ResourceDictionaryVectorInterfaceTests::DataTemplateKeyWithoutDataTypeIsRejected()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto dictionary = ref new ResourceDictionary();
            auto keyWithoutType = ref new DataTemplateKey();
            VERIFY_IS_NULL(keyWithoutType->DataType);

            // DataType not set: the key has no type to resolve, so Insert/Lookup/Remove reject it.
            VERIFY_ARE_EQUAL(E_POINTER, CaptureFailure([&]()
            {
                dictionary->Insert(keyWithoutType, ref new DataTemplate());
            }));
            VERIFY_ARE_EQUAL(0u, dictionary->Size);
            VERIFY_ARE_EQUAL(E_POINTER, CaptureFailure([&]() { dictionary->Lookup(keyWithoutType); }));
            VERIFY_ARE_EQUAL(E_POINTER, CaptureFailure([&]() { dictionary->Remove(keyWithoutType); }));
            VERIFY_IS_FALSE(dictionary->HasKey(keyWithoutType));
        });
    }

    void ResourceDictionaryVectorInterfaceTests::EmptyStringKeyIsRejected()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto dictionary = CreateDictionary(1);

            // A Platform::String^ "" is a null HSTRING (treated as a null key), so box an explicit empty string.
            Object^ emptyKey = PropertyValue::CreateString(L"");
            VERIFY_IS_NOT_NULL(emptyKey);

            // Only failure is asserted: the product currently surfaces E_FAIL, and the exact code is
            // not a documented contract.
            HRESULT hr = CaptureFailure([&]()
            {
                dictionary->Insert(emptyKey, ref new SolidColorBrush());
            });
            VERIFY_IS_TRUE(FAILED(hr), L"Inserting an empty string key must fail.");
            VERIFY_ARE_EQUAL(1u, dictionary->Size);
            VERIFY_IS_FALSE(dictionary->HasKey(emptyKey));
        });
    }

} } } } }
