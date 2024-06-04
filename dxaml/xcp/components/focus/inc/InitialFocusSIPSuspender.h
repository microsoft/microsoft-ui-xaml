// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Focusmgr.h"

/* Some devices will pop up a SIP when a TextBox / RichEditBox / PasswordBox gets focus, 
 * but this can be undesirable on devices with small screens because the SIP takes up so much space.
 *
 * An example of where this is undesirable is the Calendar app on phone - 
 * when you tap "Create an appointment" it navigates you to a new page and the first control on that page 
 * is a TextBox for the name of the appointment.If that becomes focused normally, then the SIP opens and 
 * takes up about half of the screen, obscuring a bunch of the appointment entry details.
 *
 * Thus we have this class - use this around calls to set focus which will be like an "initial focus" - 
 * window getting focus, page being loaded, popup opening, and so on.  It will temporarily suppress opening 
 * the SIP (by pretending as if PreventKeyboardDisplayOnProgrammaticFocus = true) for the duration of that one focus operation.
 */

class InitialFocusSIPSuspender
{
public:
    InitialFocusSIPSuspender(
        _In_ CFocusManager *pFocusManager) : m_focusManager(pFocusManager)
    {
        m_focusManager->SetInitialFocus(true);
    }
    ~InitialFocusSIPSuspender()
    {
        m_focusManager->SetInitialFocus(false);
    }

private:
    CFocusManager *m_focusManager;
};
