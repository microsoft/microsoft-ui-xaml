// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class PredictableDManipEnablerServer
{
public:
    PredictableDManipEnablerServer();
    ~PredictableDManipEnablerServer();
private:
    HKEY m_hKey = nullptr;
};

