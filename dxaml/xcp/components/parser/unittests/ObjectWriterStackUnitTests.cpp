// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"



#include <ObjectWriterStackUnitTests.h>
#include <ObjectWriterStack.h>
#include <ObjectWriterFrame.h>

#include <XamlTypeNamespace.h>
#include <XamlQualifiedObject.h>

class ObjectWriterStack;
class XamlTypeNamespace;
struct XamlQualifiedObject;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {

        void ObjectWriterStackUnitTests::VerifyPushPopCopy()
        {
            ObjectWriterStack stack;

            VERIFY_ARE_EQUAL(stack.size(), 0);

            stack.PushScope();
            VERIFY_ARE_EQUAL(stack.size(), 1);

            ObjectWriterStack outStack = stack.CopyStack();
            VERIFY_ARE_EQUAL(outStack.size(), 1);
            VerifyFramesAreEqual(stack.Current(), outStack.Current());

            stack.PopScope();
            VERIFY_ARE_EQUAL(stack.size(), 0);
        }

        void ObjectWriterStackUnitTests::VerifyUpdateIterators()
        {
            ObjectWriterStack stack;

            stack.PushScope();
            stack.Current().set_Instance(std::make_shared<XamlQualifiedObject>());
            auto first = stack.Current();

            stack.PushScope();
            stack.Current().set_IsObjectFromMember(true);
            auto second = stack.Current();

            VerifyFramesAreEqual(stack.Bottom(), first);
            VerifyFramesAreEqual(stack.Parent(), first);
            VerifyFramesAreEqual(stack.Current(), second);

            stack.PushScope();
            auto third = stack.Current();

            VerifyFramesAreEqual(stack.Bottom(), first);
            VerifyFramesAreEqual(stack.Parent(), second);
            VerifyFramesAreEqual(stack.Current(), third);

            stack.PopScope();
            VerifyFramesAreEqual(stack.Bottom(), first);
            VerifyFramesAreEqual(stack.Parent(), first);
            VerifyFramesAreEqual(stack.Current(), second);

            stack.PopScope();
            VerifyFramesAreEqual(stack.Current(), first);
        }

        void ObjectWriterStackUnitTests::VerifyFindNamespaceByPrefix()
        {
            ObjectWriterStack stack;
            HRESULT hr;

            DECLARE_CONST_STRING_IN_TEST_CODE(prefix1, L"Key 1");
            DECLARE_CONST_STRING_IN_TEST_CODE(prefix2, L"Key 2");
            DECLARE_CONST_STRING_IN_TEST_CODE(badPrefix, L"Not a key");
            auto namespace1 = std::make_shared<XamlTypeNamespace>();
            auto namespace2 = std::make_shared<XamlTypeNamespace>();
            auto dummyNamespace = stack.FindNamespaceByPrefix(prefix1);
            
            VERIFY_IS_NULL(dummyNamespace);

            stack.PushScope();
            hr = stack.AddNamespacePrefix(prefix1, namespace1);
            VERIFY_ARE_EQUAL(hr, S_OK, L"Adding namespace with known key.");

            stack.PushScope();
            hr = stack.AddNamespacePrefix(prefix2, namespace2);
            VERIFY_ARE_EQUAL(hr, S_OK, L"Adding namespace with known key.");

            std::shared_ptr<XamlNamespace> outNamespace = stack.FindNamespaceByPrefix(prefix1);
            VERIFY_ARE_EQUAL(namespace1, outNamespace, L"Verify we can retrieve namespace using existing key.");

            outNamespace = stack.FindNamespaceByPrefix(prefix2);
            VERIFY_ARE_EQUAL(namespace2, outNamespace, L"Verify we can retrieve namespace using existing key.");

            outNamespace = stack.FindNamespaceByPrefix(badPrefix);
            VERIFY_IS_NULL(outNamespace);
        }

        void ObjectWriterStackUnitTests::VerifyFramesAreEqual(const ObjectWriterFrame& frame1, const ObjectWriterFrame& frame2)
        {
            if (frame1.exists_Type())
            {
                VERIFY_ARE_EQUAL(frame1.get_Type(), frame2.get_Type());
            }

            if (frame1.exists_Member())
            {
                VERIFY_ARE_EQUAL(frame1.get_Member(), frame2.get_Member());
            }

            if (frame1.exists_Instance())
            {
                VERIFY_ARE_EQUAL(frame1.get_Instance(), frame2.get_Instance());
            }

            if (frame1.exists_WeakRefInstance())
            {
                VERIFY_ARE_EQUAL(frame1.get_WeakRefInstance(), frame2.get_WeakRefInstance());
            }

            if (frame1.exists_ReplacementPropertyValues())
            {
                VERIFY_ARE_EQUAL(frame1.get_ReplacementPropertyValues(), frame2.get_ReplacementPropertyValues());
            }

            if (frame1.exists_CompressedStack())
            {
                VERIFY_ARE_EQUAL(frame1.get_CompressedStack(), frame2.get_CompressedStack());
            }

            VERIFY_ARE_EQUAL(frame1.get_Collection(), frame2.get_Collection());

            if (frame1.exists_Value())
            {
                VERIFY_ARE_EQUAL(frame1.get_Value(), frame2.get_Value());
            }

            VERIFY_ARE_EQUAL(frame1.get_IsObjectFromMember(), frame2.get_IsObjectFromMember());
        }
    }
} } } }
