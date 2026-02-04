// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"



#include <ObjectWriterFrameUnitTests.h>
#include <ObjectWriterFrame.h>

#include <XamlNamespace.h>
#include <XamlTypeNamespace.h>
#include <XamlType.h>
#include <XamlQualifiedObject.h>
#include <CDependencyObject.h>

class ObjectWriterFrame;
class XamlNamespace;
class XamlTypeNamespace;
class XamlType;
class CDependencyObject;
struct XamlQualifiedObject;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {

        void ObjectWriterFrameUnitTests::VerifyFindNamespaceByPrefix()
        {
            ObjectWriterFrame frame;
            HRESULT hr;

            DECLARE_CONST_STRING_IN_TEST_CODE(goodPrefix, L"Key");
            DECLARE_CONST_STRING_IN_TEST_CODE(badPrefix, L"Not a key");
            auto inNamespace = std::make_shared<XamlTypeNamespace>();

            auto dummyNamespace = frame.FindNamespaceByPrefix(goodPrefix);
            VERIFY_IS_NULL(dummyNamespace);

            hr = frame.AddNamespacePrefix(goodPrefix, inNamespace);

            VERIFY_ARE_EQUAL(hr, S_OK, L"Adding namespace with known key.");

            std::shared_ptr<XamlNamespace> outNamespace = frame.FindNamespaceByPrefix(goodPrefix);
            VERIFY_ARE_EQUAL(inNamespace, outNamespace);

            std::shared_ptr<XamlNamespace> badNamespace = frame.FindNamespaceByPrefix(badPrefix);
            VERIFY_IS_NULL(badNamespace);
            VERIFY_ARE_NOT_EQUAL(inNamespace, badNamespace);
        }

        void ObjectWriterFrameUnitTests::VerifyDirectives()
        {
            ObjectWriterFrame frame;
            HRESULT hr;

            VERIFY_IS_FALSE(frame.exists_Directives());

            std::shared_ptr<DirectiveProperty> dummyProperty;
            std::shared_ptr<DirectiveProperty> spProperty;
            auto spInstance = std::make_shared<XamlQualifiedObject>();

            hr = DirectiveProperty::Create(
                std::shared_ptr<XamlSchemaContext>(),
                xstring_ptr::EmptyString(),
                std::shared_ptr<XamlType>(),
                std::shared_ptr<XamlNamespace>(),
                std::shared_ptr<XamlTextSyntax>(),
                XamlDirectives(),
                spProperty);

            VERIFY_ARE_EQUAL(hr, S_OK);

            hr = frame.add_Directive(spProperty, spInstance);

            VERIFY_ARE_EQUAL(hr, S_OK);
            VERIFY_IS_TRUE(frame.exists_Directives());
            VERIFY_IS_TRUE(frame.contains_Directive(spProperty));
            VERIFY_IS_FALSE(frame.contains_Directive(dummyProperty));

            std::shared_ptr<XamlQualifiedObject> outInstance;
            hr = frame.tryget_Directive(spProperty, outInstance);

            VERIFY_ARE_EQUAL(hr, S_OK);
            VERIFY_ARE_EQUAL(spInstance, outInstance);

            auto dummyInstance = std::make_shared<XamlQualifiedObject>();
            VERIFY_ARE_NOT_EQUAL(dummyInstance, outInstance);
        }

        void ObjectWriterFrameUnitTests::VerifyType()
        {
            ObjectWriterFrame frame;

            VERIFY_IS_FALSE(frame.exists_Type());

            auto dummyType = frame.get_Type();
            VERIFY_IS_NULL(dummyType);

            auto spType = std::make_shared<XamlType>(
                std::shared_ptr<XamlSchemaContext>(),
                std::shared_ptr<XamlNamespace>(),
                XamlTypeToken(),
                xstring_ptr::EmptyString());
            VERIFY_ARE_NOT_EQUAL(dummyType, spType);

            frame.set_Type(spType);
            VERIFY_IS_TRUE(frame.exists_Type());

            VERIFY_ARE_EQUAL(frame.get_Type(), spType);
        }

        void ObjectWriterFrameUnitTests::VerifyMember()
        {
            ObjectWriterFrame frame;

            auto dummyMember = frame.get_Member();
            VERIFY_IS_NULL(dummyMember);
            VERIFY_IS_FALSE(frame.exists_Member());

            auto spMember = std::make_shared<XamlProperty>();
            VERIFY_ARE_NOT_EQUAL(dummyMember, spMember);

            frame.set_Member(spMember);
            VERIFY_IS_TRUE(frame.exists_Member());
            VERIFY_ARE_EQUAL(frame.get_Member(), spMember);

            frame.clear_Member();
            VERIFY_IS_NULL(frame.get_Member());
            VERIFY_IS_FALSE(frame.exists_Member());

            VERIFY_IS_FALSE(frame.get_IsObjectFromMember());

            frame.set_IsObjectFromMember(true);
            VERIFY_IS_TRUE(frame.get_IsObjectFromMember());
        }

        void ObjectWriterFrameUnitTests::VerifyCollection() 
        {
            ObjectWriterFrame frame;

            auto dummyCollection = frame.get_Collection();
            VERIFY_IS_NULL(dummyCollection);

            auto qoCollection = std::make_shared<XamlQualifiedObject>();
            VERIFY_ARE_NOT_EQUAL(dummyCollection, qoCollection);

            frame.set_Collection(qoCollection);
            VERIFY_ARE_EQUAL(frame.get_Collection(), qoCollection);

            frame.clear_Collection();
            VERIFY_IS_NULL(frame.get_Collection());
        }

        void ObjectWriterFrameUnitTests::VerifyInstance() 
        {
            ObjectWriterFrame frame;

            auto dummyInstance = frame.get_Instance();
            VERIFY_IS_NULL(dummyInstance);
            VERIFY_IS_FALSE(frame.exists_Instance());

            auto qoInstance = std::make_shared<XamlQualifiedObject>();
            VERIFY_ARE_NOT_EQUAL(dummyInstance, qoInstance);

            frame.set_Instance(qoInstance);
            VERIFY_IS_TRUE(frame.exists_Instance());
            VERIFY_ARE_EQUAL(frame.get_Instance(), qoInstance);

            frame.clear_Instance();
            VERIFY_IS_NULL(frame.get_Instance());
            VERIFY_IS_FALSE(frame.exists_Instance());
        }

        void ObjectWriterFrameUnitTests::VerifyValue() 
        {
            ObjectWriterFrame frame;

            auto dummyValue = frame.get_Value();
            VERIFY_IS_NULL(dummyValue);
            VERIFY_IS_FALSE(frame.exists_Value());

            auto qoValue = std::make_shared<XamlQualifiedObject>();
            VERIFY_ARE_NOT_EQUAL(dummyValue, qoValue);

            frame.set_Value(qoValue);
            VERIFY_IS_TRUE(frame.exists_Value());
            VERIFY_ARE_EQUAL(frame.get_Value(), qoValue);
        }

        void ObjectWriterFrameUnitTests::VerifyWeakRef() 
        { 
            ObjectWriterFrame frame;
            VERIFY_IS_FALSE(frame.exists_WeakRefInstance());

            frame.set_Instance(std::make_shared<XamlQualifiedObject>());
            VERIFY_IS_TRUE(frame.exists_Instance());
            
            auto object = make_xref<CDependencyObject>();
            auto weakRef = xref::weakref_ptr<CDependencyObject>(object);
            frame.set_WeakRefInstance(weakRef);

            VERIFY_IS_FALSE(frame.exists_Instance());
            VERIFY_ARE_EQUAL(frame.get_WeakRefInstance(), weakRef);

            frame.clear_WeakRefInstance();
            VERIFY_IS_NULL(frame.get_WeakRefInstance());
        }

        void ObjectWriterFrameUnitTests::VerifyAssignedProperties()
        {
            ObjectWriterFrame frame;

            auto spProperty = std::make_shared<XamlProperty>();
            VERIFY_FAILED(spProperty->SetValue(
                std::make_shared<XamlQualifiedObject>(),
                std::make_shared<XamlQualifiedObject>(),
                TRUE));

            VERIFY_IS_FALSE(frame.get_IsPropertyAssigned(spProperty));

            frame.NotifyPropertyAssigned(spProperty);
            VERIFY_IS_TRUE(frame.get_IsPropertyAssigned(spProperty));

            auto dummyProperty = std::make_shared<XamlProperty>();
            VERIFY_IS_TRUE(frame.get_IsPropertyAssigned(dummyProperty));
        }

        void ObjectWriterFrameUnitTests::VerifyReplacementPropertyValues()
        {
            HRESULT hr;

            ObjectWriterFrame frame;
            VERIFY_IS_FALSE(frame.exists_ReplacementPropertyValues());

            auto replacementPropertyValues = std::make_shared<MapPropertyToQO>();
            auto spProperty = std::make_shared<XamlProperty>();
            auto spValue = std::make_shared<XamlQualifiedObject>();
            replacementPropertyValues->push_back(MapPropertyToQO::TPair(spProperty, spValue));

            frame.set_ReplacementPropertyValues(replacementPropertyValues);

            VERIFY_IS_TRUE(frame.exists_ReplacementPropertyValues());

            auto outReplacementPropertyValues = frame.get_ReplacementPropertyValues();
            VERIFY_ARE_EQUAL(replacementPropertyValues, outReplacementPropertyValues);

            std::shared_ptr<XamlQualifiedObject> outValue;
            hr = frame.GetAndRemoveReplacementPropertyValue(spProperty, outValue);
            VERIFY_ARE_EQUAL(hr, S_OK);
            VERIFY_ARE_EQUAL(spValue, outValue);
            VERIFY_IS_FALSE(frame.exists_ReplacementPropertyValues());
            VERIFY_IS_NULL(frame.get_ReplacementPropertyValues());
        }
        
        void ObjectWriterFrameUnitTests::VerifyCompressedStack() 
        {
            ObjectWriterFrame frame;

            VERIFY_IS_FALSE(frame.exists_CompressedStack());

            frame.ensure_CompressedStack();
            VERIFY_IS_TRUE(frame.exists_CompressedStack());

            VERIFY_IS_NOT_NULL(frame.get_CompressedStack());
        }
    }
} } } }
