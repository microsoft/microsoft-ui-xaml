// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "KeyTip.h"

KeyTip::~KeyTip()
{
    Reset();
}

void KeyTip::Reset()
{
    if (Popup)
    {
        VERIFYHR(Popup->SetValueByKnownIndex(KnownPropertyIndex::Popup_IsOpen, false));
    }
    Popup = nullptr;
    Object = nullptr;
}