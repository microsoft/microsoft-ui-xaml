// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <CustomMetadataRegistrar.h>
#include <vector>

using namespace Microsoft::UI::Xaml::Controls;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Naming {

    class NamingIntegrationTests
    {
    
    public:
        BEGIN_TEST_CLASS(NamingIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"f62323d4-fd46-4c98-aa85-334db95ba8f6;27f34780-4ef6-4102-929f-29737dfda1b9;0a9cdf5f-1e1b-4b1d-9659-b354bf5f4ca6")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(LooseXamlTest)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify name scopes in XamlReader.Load")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(DuplicateNameTest)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that adding a duplicate name to tree causes an error on the append.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(DataTemplateTest)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify name scopes in data templates.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PopupTest)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify name scopes in popups")
        END_TEST_METHOD()
    
        BEGIN_TEST_METHOD(UserControlTest)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify name scopes with user controls")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(ControlTemplateTest)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify name scopes in control templates")
        END_TEST_METHOD()
    
        BEGIN_TEST_METHOD(ContentPresenterTest)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify name scopes in content presenters")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MidParseSetNameTest)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that a Name can be set by the app during a parse")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(NestedUserControlWithCustomContent)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that custom content on a nested UserControl will register its names with the nested UserControl.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StoryboardCanResolveNamescopeOwnerViaMentor)
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateNamedDataTemplateInDictionaryWithConnectionId)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that DataTemplates with x:Name and x:ConnectionId in a ResourceDictionary can be named and added to the dictionary.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UserControlWithDuplicateChildNameRegistrationPrecedence)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that a FindName call on a UserControl with a child element with a duplicate name as the definition name "
                L"of the UserControl returns the child elemnet and NOT the UserControl")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UserControlWithinDataTemplateCanStillFindItsElements)
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ItemsPanelTemplateNamescopeMembersDoRegisterTheirName)
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateNameRegistrationWithExisitingStandardNamescopeOwner)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateNameRegistrationWithoutExisitingStandardNamescopeOwner)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanDataBindNameProperty)
        END_TEST_METHOD()

    private:

        void ItemsPanelTemplateNamescopeMemberNamRegistrationTest(bool shouldRegister);
        DependencyObject^ CreateElementWithName(Platform::String^ type, Platform::String^ name);
    };




    
} } } } } }


// Test class used by MidParseSetNameTest
namespace Tests { namespace Native { namespace External { namespace Framework { namespace Naming {

    ref class MyPage : public Page, public Microsoft::UI::Xaml::Markup::IComponentConnector
    {        
        public:
        Microsoft::UI::Xaml::DataTemplate^ NamedDataTemplate() { return _namedDataTemplate; }
        virtual void Connect(int connectionId, ::Platform::Object^ target);
        virtual Microsoft::UI::Xaml::Markup::IComponentConnector^ GetBindingConnector(int /*connectionId*/, ::Platform::Object^ /*target*/) { return nullptr;  }

        private:
        Microsoft::UI::Xaml::DataTemplate^ _namedDataTemplate;
    };

    
}}}}}
