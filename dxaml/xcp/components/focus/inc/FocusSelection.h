// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "enumdefs.g.h"
#include <Metadataapi.h>
#include <CValue.h>
#include "XYFocus.h"
#include "FocusMovement.h"

namespace Focus
{
    class FocusSelection
    {
    public:
        template<class Element>
        static bool ShouldUpdateFocus(_In_ Element* const element, _In_ const DirectUI::FocusState focusState)
        {
            return !(focusState == DirectUI::FocusState::Pointer && GetAllowFocusOnInteraction(element) == false);
        }

        template<class Element>
        static bool GetAllowFocusOnInteraction(_In_ Element* const element)
        {
            CValue value;
            KnownPropertyIndex index = KnownPropertyIndex::FrameworkElement_AllowFocusOnInteraction;

            if (element->template OfTypeByIndex<KnownTypeIndex::TextElement>())
            {
                index = KnownPropertyIndex::TextElement_AllowFocusOnInteraction;
            }
            else if(element->template OfTypeByIndex<KnownTypeIndex::FlyoutBase>())
            {
                index = KnownPropertyIndex::FlyoutBase_AllowFocusOnInteraction;
            }

            auto dp = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(index);

            if (SUCCEEDED(element->GetValueInherited(dp, &value)))
            {
                return value.AsBool() == TRUE;
            }

            return true;
        }

        static DirectUI::FocusNavigationDirection GetNavigationDirection(_In_ wsy::VirtualKey key);
        static DirectUI::FocusNavigationDirection GetNavigationDirectionForKeyboardArrow(_In_ wsy::VirtualKey key);

        struct DirectionalFocusInfo
        {
            DirectionalFocusInfo() : handled(false), shouldBubble(true), focusCandidateFound(false), directionalFocusEnabled(false){}
            bool handled;
            bool shouldBubble;
            bool focusCandidateFound;
            bool directionalFocusEnabled;
        };

        template<class Options = Focus::XYFocusOptions, class FocusManager, class Element=CDependencyObject>
        static DirectionalFocusInfo TryDirectionalFocus(_In_ FocusManager* const focusMgr, _In_ DirectUI::FocusNavigationDirection direction, _In_ Element* const searchScope)
        {
            CValue value;

            DirectionalFocusInfo info;

            if (direction == DirectUI::FocusNavigationDirection::Next || direction == DirectUI::FocusNavigationDirection::Previous ||
                direction == DirectUI::FocusNavigationDirection::None) { return info; }

            // We do not want to process direction focus if the element is not a UIElement (ie. Hyperlink)
            if (searchScope->template OfTypeByIndex<KnownTypeIndex::UIElement>() == false) { return info; }

            if (SUCCEEDED(searchScope->GetValueByIndex(KnownPropertyIndex::UIElement_XYFocusKeyboardNavigation, &value)))
            {
                DirectUI::XYFocusKeyboardNavigationMode mode = static_cast<DirectUI::XYFocusKeyboardNavigationMode>(value.AsEnum());

                if (mode == DirectUI::XYFocusKeyboardNavigationMode::Disabled)
                {
                    info.shouldBubble = false;
                }
                else if(mode == DirectUI::XYFocusKeyboardNavigationMode::Enabled)
                {
                    info.directionalFocusEnabled = true;
                    Options xyFocusOptions;
                    xyFocusOptions.searchRoot = searchScope;
                    xyFocusOptions.shouldConsiderXYFocusKeyboardNavigation = true;

                    Element* const candidate = focusMgr->FindNextFocus(direction, xyFocusOptions, nullptr, true /*updateManifold*/);

                    if (candidate)
                    {
                        const Focus::FocusMovementResult result = focusMgr->SetFocusedElement(Focus::FocusMovement(candidate, direction, DirectUI::FocusState::Keyboard));
                        VERIFYHR(result.GetHResult());
                        info.handled = result.WasMoved();
                        info.focusCandidateFound = true;
                    }
                }
            }

            return info;
        }
    };
}
