// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PersonPicture.h"

#include "PersonPictureAutomationPeer.g.h"

class PersonPictureAutomationPeer :
    public ReferenceTracker<PersonPictureAutomationPeer, winrt::implementation::PersonPictureAutomationPeerT>
{
public:
    PersonPictureAutomationPeer(winrt::PersonPicture const& owner);

    // IAutomationPeerOverrides 
    winrt::AutomationControlType GetAutomationControlTypeCore();
    winrt::hstring GetClassNameCore();
};
