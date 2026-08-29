// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CDependencyObject;
class CDependencyProperty;

typedef _Check_return_ HRESULT(__stdcall CREATE_GROUP_FN)(
    _In_     CDependencyObject   *pObject,
    _In_opt_ const CDependencyProperty *pDp,
    _In_     bool forGetValue // If this is a GetValue call
    );
typedef CREATE_GROUP_FN *CREATEGROUPPFN;