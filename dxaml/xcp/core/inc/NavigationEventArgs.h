// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CNavigationEventArgs final : public CEventArgs
{
public:
    ~CNavigationEventArgs() override
    {
    }

    CNavigationEventArgs()
    {
    }

    void SetUrl(xstring_ptr strUrl)
    {
        m_strUrl = strUrl;
    }

    xstring_ptr m_strUrl;
};
