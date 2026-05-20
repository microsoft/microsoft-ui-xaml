// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <collection.h>

using namespace ::Windows::Foundation::Collections;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace Patterns {

    ref class MockCustomNavigationProviderControlAutomationPeer sealed : public xaml_automation_peers::ListViewItemAutomationPeer,
        public Microsoft::UI::Xaml::Automation::Provider::ICustomNavigationProvider
    {
    public:
        MockCustomNavigationProviderControlAutomationPeer(xaml_controls::ListViewItem^ owner, int positionInSet, int sizeOfSet, int level) :xaml_automation_peers::ListViewItemAutomationPeer(owner)
        {
            _positionInSet = positionInSet;
            _sizeOfSet = sizeOfSet;
            _level = level;

            // initialize default values
            if (_positionInSet == -1)
            {
                _positionInSet = 0;
            }
            if (_sizeOfSet == -1)
            {
                _sizeOfSet = 3;
            }
            if (_level == -1)
            {
                _level = 0;
            }
        };

        virtual ~MockCustomNavigationProviderControlAutomationPeer()
        {
        };

    protected:
        Platform::Object^ GetPatternCore(Microsoft::UI::Xaml::Automation::Peers::PatternInterface patternInterface) override
        {
            if (patternInterface == xaml_automation_peers::PatternInterface::CustomNavigation)
            {
                return this;
            }

            return xaml_automation_peers::ListViewItemAutomationPeer::GetPatternCore(patternInterface);
        }

        Microsoft::UI::Xaml::Automation::Peers::AutomationControlType GetAutomationControlTypeCore() override
        {
            return Microsoft::UI::Xaml::Automation::Peers::AutomationControlType::ListItem;
        }

        int GetPositionInSetCore() override
        {
            return _positionInSet;
        }

        int GetSizeOfSetCore() override
        {
            return _sizeOfSet;
        }

        int GetLevelCore() override
        {
            return _level;
        }

        Platform::String^ GetClassNameCore() override
        {
            return "TestMock";
        }
    public:
        // ICustomNavigationProvider
        virtual Platform::Object^ NavigateCustom(Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection direction);

    private:
        int _positionInSet;
        int _sizeOfSet;
        int _level;
    };

    ref class MockCustomNavigationProviderControl sealed : public xaml_controls::ListViewItem
    {
    public:
        MockCustomNavigationProviderControl(int positionInSet, int sizeOfSet, int level)
        {
            DefaultStyleKey = "Microsoft.UI.Xaml.Test.Automation.TextRange.MockCustomNavigationProviderControl";
            _positionInSet = positionInSet;
            _sizeOfSet = sizeOfSet;
            _level = level;
        }

        virtual ~MockCustomNavigationProviderControl()
        {
        }

    protected:
        virtual xaml_automation_peers::AutomationPeer^ OnCreateAutomationPeer() override
        {
            if (_peer == nullptr)
            {
                _peer = ref new MockCustomNavigationProviderControlAutomationPeer(this, _positionInSet, _sizeOfSet, _level);
            }
            return _peer;
        }
    public:
        int GetPositionInSet()
        {
            return _positionInSet;
        }
        int GetSizeOfSet()
        {
            return _sizeOfSet;
        }
        int GetLevel()
        {
            return _level;
        }

    private:
        MockCustomNavigationProviderControlAutomationPeer^ _peer;
        int _positionInSet;
        int _sizeOfSet;
        int _level;
    };


    Platform::Object^ MockCustomNavigationProviderControlAutomationPeer::NavigateCustom(Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection direction)
    {
        xaml_automation_peers::AutomationPeer^ retValue = nullptr;
        MockCustomNavigationProviderControl^ retElement = nullptr;
        if (direction == Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection::FirstChild)
        {
            retElement = ref new MockCustomNavigationProviderControl(0, 3, _level + 1);
        }
        else if (direction == Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection::NextSibling)
        {
            if (_positionInSet < 2)
            {
                retElement = ref new MockCustomNavigationProviderControl(_positionInSet + 1, 3, _level);
            }
        }
        else if (direction == Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection::PreviousSibling)
        {
            if (_positionInSet > 0)
            {
                retElement = ref new MockCustomNavigationProviderControl(_positionInSet - 1, 3, _level);
            }
        }
        else if (direction == Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection::Parent)
        {
            if (_level > 0)
            {
            }
        }
        else if (direction == Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection::LastChild && _level < 3)
        {
            retElement = ref new MockCustomNavigationProviderControl(2, 3, _level + 1);
        }

        if (retElement)
        {
            xaml::Controls::Grid^ container = (xaml::Controls::Grid^)TestServices::WindowHelper->WindowContent;
            container->Children->Append(retElement);

            retValue = FrameworkElementAutomationPeer::CreatePeerForElement(retElement);
        }
        return retValue;
    }

} } } } } }
