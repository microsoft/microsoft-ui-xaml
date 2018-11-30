// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// PersonPictureAutomationPeer.cpp
// Implementation of the PersonPictureAutomationPeer class.

#include "pch.h"
#include "PersonPictureAutomationPeer.h"

using namespace Platform;
using namespace Windows::UI::Xaml;

PersonPictureAutomationPeer::PersonPictureAutomationPeer(PersonPicture^ personPicture) :
    FrameworkElementAutomationPeer(personPicture)
{
}

String^ PersonPictureAutomationPeer::GetClassNameCore()
{
    return "Microsoft.People.Controls.PersonPicture";
}

Automation::Peers::AutomationControlType PersonPictureAutomationPeer::GetAutomationControlTypeCore()
{
    return Automation::Peers::AutomationControlType::Text;
}
