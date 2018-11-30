// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// PersonPictureAutomationPeer.h
// Declaration of the PersonPictureAutomationPeer class.

#pragma once

#include "PersonPicture.h"

namespace Microsoft { namespace People { namespace Controls {

    ref class PersonPicture;

    /// <summary>
    /// Automation peer for SlideButton
    /// </summary>
    [Windows::Foundation::Metadata::WebHostHidden]
    private ref class PersonPictureAutomationPeer sealed : public Windows::UI::Xaml::Automation::Peers::FrameworkElementAutomationPeer
    {
    public:
        PersonPictureAutomationPeer(PersonPicture^ personPicture);

    protected:
        virtual Platform::String^ GetClassNameCore() override;
        virtual Windows::UI::Xaml::Automation::Peers::AutomationControlType GetAutomationControlTypeCore() override;
    };

}}} // namespace Microsoft { namespace People { namespace Controls {
