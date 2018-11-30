// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
class VectorChangedEventArgs : public winrt::implements<VectorChangedEventArgs, winrt::IVectorChangedEventArgs>
{
public:
    VectorChangedEventArgs(winrt::CollectionChange action, unsigned int index)
    {
        m_action = action;
        m_index = index;
    }

    winrt::CollectionChange CollectionChange() { return m_action; }
    uint32_t Index() { return m_index; }

private:
    winrt::CollectionChange m_action;
    unsigned int m_index;
};