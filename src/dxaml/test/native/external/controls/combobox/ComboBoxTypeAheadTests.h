// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <CommonInputHelper.h>
#include <ComboBoxHelper.h>
#include <Collection.h>

#include "ComboBoxTypeAheadTestsTypes.h"

using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ComboBox {
    // Specifying these number of items ensures that:
    // Typing "K" to start a search will always result in Kangaroo
    // Typing "N" to start a search will always result in Newt
    // Typing "Q" to start a search will result in no hits found (Quail omitted from animal list)
    // These values are carefully chosen; ensure any future values maintain this invariant. 
    enum class NumberOfItems
    {
        Twenty = 20, // short
        TwoHundred = 200 // tall/all
    };

    enum class ComboBoxSetupType
    {
        Default, // Just a plain 'ol ComboBox
        StackPanelItemsPanel, // ComboBox with retemplated ItemsPanel
        InsideControlThatHandlesSpace, // ComboBox within a Button, which handles space
        TextSearchIsDisabled // ComboBox.IsTextSearchEnabled = false, don't search
    };

    enum class ComboBoxItemType
    {
        String, // Strings representing animal objects
        StringWithLeadingSpaces, // Same as above, except with spaces preceding the string
        ComboBoxItem, // ComboBoxItems containing content = name of animals
        PersonObject // Person objects with a FirstName, and a Pet. 
    };

    class TypeAheadRunnerParameters 
    {
        public:
            TypeAheadRunnerParameters()
            {
                _numberOfItems = ComboBox::NumberOfItems::Twenty;
                _keySequences = ref new Platform::Collections::Vector<Platform::String^>();
                _expectedStrings = ref new Platform::Collections::Vector<Platform::String^>();
                _startIndex = -1;
                _shouldOpenDropDown = false;
                _setupType = ComboBox::ComboBoxSetupType::Default;
                _itemType = ComboBox::ComboBoxItemType::String;
                _displayMemberPaths = ref new Platform::Collections::Vector<Platform::String^>();
                _timeBetweenKeyPressesInMs = 0;
            }
        public:
            void SetNumberOfItems(ComboBox::NumberOfItems value)
            {
                _numberOfItems = value;
            }
            ComboBox::NumberOfItems GetNumberOfItems()
            {
                return _numberOfItems;
            }
            void AppendKeySequence(Platform::String^ value)
            {
                _keySequences->Append(value);
            }
            void SetKeySequences(Platform::Collections::Vector<Platform::String^>^ value)
            {
                _keySequences = value;
            }
            Platform::Collections::Vector<Platform::String^>^ GetKeySequences()
            {
                return _keySequences;
            }
            void AppendExpectedString(Platform::String^ value)
            {
                _expectedStrings->Append(value);
            }
            void SetExpectedStrings(Platform::Collections::Vector<Platform::String^>^ value)
            {
                _expectedStrings = value;
            }
            Platform::Collections::Vector<Platform::String^>^ GetExpectedStrings()
            {
                return _expectedStrings;
            }
            void SetStartIndex(int value)
            {
                _startIndex = value;
            }
            int GetStartIndex()
            {
                return _startIndex;
            }
            void SetShouldOpenDropDown(bool value)
            {
                _shouldOpenDropDown = value;
            }
            bool GetShouldOpenDropDown()
            {
                return _shouldOpenDropDown;
            }
            void SetSetupType(ComboBox::ComboBoxSetupType value)
            {
                _setupType = value;
            }
            ComboBox::ComboBoxSetupType GetSetupType()
            {
                return _setupType;
            }
            void SetItemType(ComboBox::ComboBoxItemType value)
            {
                _itemType = value;
            }
            ComboBox::ComboBoxItemType GetItemType()
            {
                return _itemType;
            }
            void AppendDisplayMemberPaths(Platform::String^ value)
            {
                _displayMemberPaths->Append(value);
            }
            Platform::Collections::Vector<Platform::String^>^ GetDisplayMemberPaths()
            {
                return _displayMemberPaths;
            }
            void SetTimeBetweenKeyPressesInMs(int value)
            {
                _timeBetweenKeyPressesInMs = value;
            }
            int GetTimeBetweenKeyPressesInMs()
            {
                return _timeBetweenKeyPressesInMs;
            }
        private:
            ComboBox::NumberOfItems _numberOfItems;
            Platform::Collections::Vector<Platform::String^>^ _keySequences;
            Platform::Collections::Vector<Platform::String^>^ _expectedStrings;
            int _startIndex;
            bool _shouldOpenDropDown;
            ComboBox::ComboBoxSetupType _setupType;
            ComboBox::ComboBoxItemType _itemType;
            Platform::Collections::Vector<Platform::String^>^ _displayMemberPaths;
            int _timeBetweenKeyPressesInMs;
    };

    class ComboBoxTypeAheadTests : public WEX::TestClass<ComboBoxTypeAheadTests>
    {
    public:
        BEGIN_TEST_CLASS(ComboBoxTypeAheadTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"d96ffe50-c538-42ef-918b-517ad455f1e0;70a0f79e-de5f-46d3-bd45-a84dcb6e99df;cc5953d4-6553-42e5-8c02-80720aa9d842")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        // Short = 20 items; Aardvark, Arrow crab, Blue bird, Carp, Condor, Donkey, Finch, Goldfish, Harrier, Kangaroo, Lizard, Mastodon, Newt, Peafowl, Praying mantis, Rooster, Skink, Stingray, Tiger shark, Water buffalo
        // Long = 200 items; Aardvark, African buffalo, African leopard, Alpaca, American robin, Angelfish, Ant, Antelope, Ape, Aphid, Arrow crab, Baboon, Bald eagle, Barracuda, Bass, Bear, Bedbug, Beetle, Bison, Black panther, Blue bird, Blue whale, Boar, Bobolink, Box jellyfish, Buffalo, Butterfly, Camel, Cape buffalo, Cardinal, Carp, Catshark, Catfish, Centipede, Chameleon, Chickadee, Chimpanzee, Chipmunk, Clownfish, Cockroach, Condor, Coral, Coyote, Crane, Crawdad, Cricket, Crow, Damselfly, Dingo, Dog, Donkey, Dove, Duck, Eagle, Earwig, Eel, Elephant, Elk, English pointer, Falcon, Finch, Fish, Flea, Fowl, Frog, Gamefowl, Gecko, Giant panda, Gibbon, Giraffe, Goldfish, Gopher, Grasshopper, Great white shark, Ground shark, Guanaco, Guinea pig, Guppy, Halibut, Hamster, Harrier, Hedgehog, Heron, Hippopotamus, Hornet, Hoverfly, Hyena, Impala, Jaguar, Jellyfish, Kangaroo, Kingfisher, Kiwi, Koi, Ladybug, Landfowl, Lark, Lemming, Leopard, Limpet, Lizard, Lobster, Loon, Lungfish, Macaw, Magpie, Manatee, Manta ray, Marmoset, Marsupial, Mastodon, Meerkat, Minnow, Mockingbird, Mollusk, Monitor lizard, Moose, Moth, Mouse, Muskox, Newt, Nightingale, Octopus, Orangutan, Ostrich, Owl, Panda, Panthera hybrid, Parrot, Partridge, Peafowl, Penguin, Peregrine falcon, Pig, Pike, Piranha, Platypus, Pony, Porpoise, Prairie dog, Praying mantis, Puffin, Python, Rabbit cottontail, Rainbow trout, Rattlesnake, Ray, Reindeer, Rhinoceros, Rodent, Rooster, Saber-toothed cat, Salamander, Sawfish, Scallop, Seahorse, Sea slug, Shark, Shrew, Silkworm, Skink, Sloth, Smelt, Snake, Snow leopard, Sole, Spider, Spoonbill, Squirrel, Star-nosed mole, Stingray, Stork, Sugar glider, Swan, Swordfish, Tahr, Tarantula, Tasmanian devil, Tern, Tick, Tiger shark, Tortoise, Trapdoor spider, Trout, Turkey, Urial, Vampire squid, Vole, Wallaby, Wasp, Water buffalo, Whale, Whooping crane, Wildebeest, Wolf, Wombat, Worm, Xerinae, Yak, Zebra
        // People = 20 items; Ace, Breck, Cleffa, Demetruis, Eunice, Frank, Gary, Herb, Igor, Jace, Kif, Leon, Matt, Neo, Orion, Percy, Quint, Reese, Steven, Tubbs

        // Basic selection changed functionality cases:
        // NOTE: These tests do not change selection via TypeAhead; instead, they ensure that put_selectedIndex does the right thing
        // in a variety of cases (namely ensuring that setting SelectedIndex to indices whose items were visually virtualized away still works).
        BEGIN_TEST_METHOD(CanChangeSelectionWhileComboBoxIsOpenedShort)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that changing selection works when the combobox is opened, with no virtualization")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeSelectionWhileComboBoxIsOpenedLong)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that changing selection works when the combobox is opened, with virtualization")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeSelectionWhileComboBoxIsClosedShort)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that changing selection works when the combobox is closed, with no virtualization")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeSelectionWhileComboBoxIsClosedLong)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that changing selection works when the combobox is closed, with virtualization")
        END_TEST_METHOD()

        // One letter cases:
        // No results found cases:
        BEGIN_TEST_METHOD(NoResultsFoundOpenedShort)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that typing 'Q' results in no hits in an opened box with no virutalization")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(NoResultsFoundClosedShort)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that typing 'Q' results in no hits in a closed box with no virutalization")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(NoResultsFoundOpenedLong)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that typing 'Q' results in no hits in an opened box with virutalization")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(NoResultsFoundClosedLong)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that typing 'Q' results in no hits in a closed box with virutalization")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(NoResultsFoundComboBoxItems)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that typing 'Q' results in no hits with ComboBoxItems")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(NoResultsFoundStackPanelItemsPanel)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that typing 'Q' results in no hits with Strings in a StackPanel ItemsPanel")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(NoResultsFoundBecauseSearchIsDisabled)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that typing 'K' results in no hits when the ComboBox has IsTextSearchEnabled is false.")
        END_TEST_METHOD()

        // Results found cases (looking for Kangaroo or Newt)
        // Hit the first result without looping (ie. go from Index 0 to Index N, where N >= 0)
        BEGIN_TEST_METHOD(CanHitFirstResultOpenedShort)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that typing 'K' results in hitting 'Kangaroo' in an opened box with no virtualization.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(CanHitFirstResultClosedShort)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that typing 'K' results in hitting 'Kangaroo' in a closed box with no virtualization.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanHitFirstResultOpenedLong)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that typing 'K' results in hitting 'Kangaroo' in an opened box with virtualization.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(CanHitFirstResultClosedLong)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that typing 'K' results in hitting 'Kangaroo' in a closed box with virtualization.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanHitFirstResultComboBoxItems)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that typing 'K' results in hitting 'Kangaroo' in a box with ComboBoxItems")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanHitFirstResultStackPanelItemsPanel)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that typing 'K' results in hitting 'Kangaroo' in a box with Strings in a StackPanel ItemsPanel")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        // Hit the first result after looping (ie. go from Index=last to Index N, where N < last index). 
        // This flexes the part of the search that loops the search from the beginning if we're at the end of the list.  
        BEGIN_TEST_METHOD(CanHitFirstResultLoopingOpenedShort)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that typing 'K' results in hitting 'Kangaroo' in an opened box with no virtualization.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(CanHitFirstResultLoopingClosedShort)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that typing 'K' results in hitting 'Kangaroo' in a closed box with no virtualization.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanHitFirstResultLoopingOpenedLong)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that typing 'K' results in hitting 'Kangaroo' in an opened box with virtualization.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(CanHitFirstResultLoopingClosedLong)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that typing 'K' results in hitting 'Kangaroo' in a closed box with virtualization.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanHitFirstResultLoopingStackPanelItemsPanel)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that typing 'K' results in hitting 'Kangaroo' in a box with a StackPanel ItemsPanel.")
        END_TEST_METHOD()

        // Two letter cases:
        // Type one letter to jump to the first match, then add a new letter to jump to the next match
        BEGIN_TEST_METHOD(CanHitFirstResultThenHitSecondResultSequentiallyAfterOpenedShort)
            TEST_METHOD_PROPERTY(L"Description", L"Type 'a' to go to Aardvark, then 'r' to go to Arrow Crab. Non-virtualized list, opened.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(CanHitFirstResultThenHitSecondResultSequentiallyAfterClosedShort)
            TEST_METHOD_PROPERTY(L"Description", L"Type 'a' to go to Aardvark, then 'r' to go to Arrow Crab. Non-virtualized list, closed.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanHitFirstResultThenHitSecondResultSequentiallyAfterOpenedLong)
            TEST_METHOD_PROPERTY(L"Description", L"Type 'a' to go to Aardvark, then 'r' to go to Arrow Crab. Virtualized list, opened.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(CanHitFirstResultThenHitSecondResultSequentiallyAfterClosedLong)
            TEST_METHOD_PROPERTY(L"Description", L"Type 'a' to go to Aardvark, then 'r' to go to Arrow Crab. Virtualized list, closed.")
        END_TEST_METHOD()

        // Stay at the same item once no match is found
        BEGIN_TEST_METHOD(CanHitFirstAndSecondResultAndStayAfterThirdLetterNoResultsFoundOpened)
            TEST_METHOD_PROPERTY(L"Description", L"Type 'a' to go to Aardvark, then 'r' to go to Arrow Crab, then 'q'. Stay on Arrow Crab. Virtualized list, opened.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanHitFirstAndSecondResultAndStayAfterThirdLetterNoResultsFoundClosed)
            TEST_METHOD_PROPERTY(L"Description", L"Type 'a' to go to Aardvark, then 'r' to go to Arrow Crab, then 'q'. Stay on Arrow Crab. Virtualized list, closed.")
        END_TEST_METHOD()

        // Space cases:
        BEGIN_TEST_METHOD(EnsureLeadingSpaceOpensClosesComboBox)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that pressing space without entering Text Search mode opens/closes the box.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanHitFirstResultLeadingSpaces)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that typing 'K' results in hitting '  Kangaroo' in a box with Strings containing leading spaces")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanHitResultIncludingInteriorSpaces)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that the Space char is included in the search string by going from 'African buffalo' to 'African leopard'")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(EnsureLeadingSpaceOpensClosesComboBoxInsideControlThatHandlesSpace)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that pressing space without entering Text Search mode opens/closes the box, even in a parent that would otherwise eat Space. ")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(EnsureTypeAheadWithSpacesDoesntTriggerButtonClickOpened)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that the Space char is included in the search string and not bubbled up to parent control if in search mode; popup open.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(EnsureTypeAheadWithSpacesDoesntTriggerButtonClickClosed)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that the Space char is included in the search string and not bubbled up to parent control if in search mode; popup closed.")
        END_TEST_METHOD()

        // ComboBox with custom objects instead of strings cases:
        BEGIN_TEST_METHOD(CanFindTextInObjectWithNullDisplayMemberPath)
            TEST_METHOD_PROPERTY(L"Description", L"Given PersonObjects and a null DisplayMemberPath, ensure we can hit '' after typing ''.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanFindTextInObjectWithSimpleDisplayMemberPath)
            TEST_METHOD_PROPERTY(L"Description", L"Given PersonObjects and a DisplayMemberPath of 'FirstName', ensure we can hit 'Matt' after typing 'M'.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanFindTextInObjectWithComplexDisplayMemberPath)
            TEST_METHOD_PROPERTY(L"Description", L"Given PersonObjects and a DisplayMemberPath of 'PetAnimal.CommonName', ensure we can hit 'Matt' after typing 'Newt' (since Matt's pet is a newt)")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanFindTextInObjectAfterResettingDisplayMemberPath)
            TEST_METHOD_PROPERTY(L"Description", L"Given PersonObjects and an empty DMP, ensure we hit 'Matt'. Then set DMP to 'PetAnimal.CommonName', and hit 'Cleffa' after typing 'B' (since Cleffa's pet is a Blue Bird)")
        END_TEST_METHOD()

        // Timeout cases:
        BEGIN_TEST_METHOD(CanFindFirstStringThenNextAfterTimeout)
            TEST_METHOD_PROPERTY(L"Description", L"Type 'a' to go to 'Aardvark' then a again to go to 'Arrow Crab'.")
        END_TEST_METHOD()

    private:
        // Test runners:
        void CanChangeSelectionRunner(TypeAheadRunnerParameters* runnerParams, int endingIndex);

        void TypeAheadRunner(TypeAheadRunnerParameters* runnerParams);

        void LeadingSpaceRunner(ComboBox::ComboBoxSetupType setupType);

        // Setup methods:
        xaml_controls::ComboBox^ SetupTypeAheadComboBoxTest(TypeAheadRunnerParameters* runnerParams);
        xaml_controls::ComboBox^ CreateStackPanelComboBox();
        xaml_controls::Button^ CreateComboBoxInsideControlThatHandlesSpace();

        void AddComboBoxItems(xaml_controls::ComboBox^ comboBox, ComboBox::NumberOfItems numberOfItems);

        Platform::Collections::Vector<Platform::String^>^ GetListOfAnimalStrings(ComboBox::NumberOfItems numberOfItems, Platform::String^ prefix = nullptr);
        Platform::Collections::Vector<PetOwnerObject^>^ GetListOfPersonObjects(); 

        // Verification helpers:
        void VerifySelectedValueMatchesExpectedString(Platform::Object^ selectedValue, Platform::String^ expectedString);

        UINT ConvertNumberOfItemsEnumToInt(ComboBox::NumberOfItems numberOfItemsEnum);
    };

} } } } } }
