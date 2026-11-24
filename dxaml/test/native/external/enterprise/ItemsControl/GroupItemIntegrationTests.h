// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <collection.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace ItemsControl {

    // Validates GroupItem behavior for grouped ItemsControl/ListView instances that use a legacy
    // (ItemContainerGenerator based) items panel such as StackPanel or VirtualizingStackPanel.
    // Modern panels (ItemsStackPanel/ItemsWrapGrid) use ListViewBaseHeaderItem and are not covered here.
    class GroupItemIntegrationTests : public WEX::TestClass<GroupItemIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(GroupItemIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(HidesIfEmptyCollapsesAndRestoresGroupItem)
            TEST_METHOD_PROPERTY(L"Description", L"With GroupStyle.HidesIfEmpty the GroupItem of an empty group is collapsed and becomes visible once the group gets an item.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RemovingGroupClearsGroupItemContainer)
            TEST_METHOD_PROPERTY(L"Description", L"Removing a group clears the generated Content and header ContentTemplate of its GroupItem and leaves the other groups intact.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ContainerStyleAndSelectorAppliedToGroupItems)
            TEST_METHOD_PROPERTY(L"Description", L"GroupStyle.ContainerStyle, or GroupStyle.ContainerStyleSelector when no ContainerStyle is set, is applied to every GroupItem.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UserDeclaredGroupItemIsNotGeneratorPrepared)
            TEST_METHOD_PROPERTY(L"Description", L"A GroupItem added directly as an item keeps its own Content when it is prepared and when it is removed.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GroupItemCreatesGroupItemAutomationPeer)
            TEST_METHOD_PROPERTY(L"Description", L"A GroupItem exposes a GroupItemAutomationPeer whose children are the peers of the group's items.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GroupHeaderKeyDownIsNotConsumedOutsideSemanticZoom)
            TEST_METHOD_PROPERTY(L"Description", L"A key pressed in a group header of a ListView that is not hosted in a SemanticZoom is not marked handled by the group header.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RecycledGroupItemsRestoreState)
            TEST_METHOD_PROPERTY(L"Description", L"GroupItems recycled by a VirtualizingStackPanel show the correct group, items and HidesIfEmpty visibility after being reused.")
        END_TEST_METHOD()

    private:
        static Platform::Collections::Vector<Platform::Object^>^ MakeGroup(std::initializer_list<const wchar_t*> items);
        static xaml_controls::GroupItem^ FindGroupItem(xaml::FrameworkElement^ itemsControl, Platform::Object^ group);
        static unsigned int CountGroupItems(xaml::FrameworkElement^ itemsControl);
        static xaml::FrameworkElement^ LoadGroupedContent(
            Platform::String^ xamlString,
            Platform::Collections::Vector<Platform::Object^>^ groups);
    };

} } } } } }
