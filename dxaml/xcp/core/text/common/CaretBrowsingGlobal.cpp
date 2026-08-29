// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CaretBrowsingGlobal.h"

bool g_caretBrowsingModeEnable = false;             // a status whether this mode is currently enabled
bool g_caretBrowsingDialogNotPopAgain = false;      // record if user don't want this dialog be pop up again
bool g_caretBrowsingF7Disabled = false;             // indicates whether F7 should no longer attempt to toggle caret browsing

void SetCaretBrowsingModeEnable(bool value)
{
    g_caretBrowsingModeEnable = value;
}
void SetCaretBrowsingDialogNotPopAgain(bool value)
{
    g_caretBrowsingDialogNotPopAgain = value;
}
void SetCaretBrowsingF7Disabled(bool value)
{
    g_caretBrowsingF7Disabled = value;
}
bool GetCaretBrowsingModeEnable()
{
    return g_caretBrowsingModeEnable;
}
bool GetCaretBrowsingDialogNotPopAgain()
{
    return g_caretBrowsingDialogNotPopAgain;
}
bool GetIsCaretBrowsingF7Disabled()
{
    return g_caretBrowsingF7Disabled;
}