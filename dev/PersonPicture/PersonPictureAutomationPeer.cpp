// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "PersonPicture.h"
#include "PersonPictureAutomationPeer.h"
#include "common.h"

PersonPictureAutomationPeer::PersonPictureAutomationPeer(winrt::PersonPicture  /*unused*/const& owner) :
    ReferenceTracker(owner)
{
}

//IAutomationPeerOverrides

winrt::AutomationControlType PersonPictureAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Text;
}

winrt::hstring PersonPictureAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::PersonPicture>();
}