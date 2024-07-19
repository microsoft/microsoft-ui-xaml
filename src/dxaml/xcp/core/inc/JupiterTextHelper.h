// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <IXamlTestHooks-win.h>

class JupiterTextHelper
{
public:
    static HRESULT GetGripperData(
        _In_  void*                element,
        _Out_ JupiterGripperData*  data);

private:
    static HRESULT SetGripperData(
        _In_  TextSelectionManager* selectionManager,
        _Out_ JupiterGripperData*  data);
};
