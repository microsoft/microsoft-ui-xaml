// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct IQualifierContextCallback
{
    virtual _Check_return_ HRESULT OnQualifierContextChanged() = 0; 
};

