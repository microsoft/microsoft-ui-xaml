// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PagerControlSelectedIndexChangedEventArgs.g.h"

class PagerControlSelectedIndexChangedEventArgs :
    public winrt::implementation::PagerControlSelectedIndexChangedEventArgsT<PagerControlSelectedIndexChangedEventArgs>
{
public:
    PagerControlSelectedIndexChangedEventArgs(const int oldIndex,const int newIndex) :
        m_PreviousPageIndex(oldIndex), m_NewPageIndex(newIndex) {};

    int NewPageIndex() { return m_NewPageIndex; };
    int PreviousPageIndex() { return m_PreviousPageIndex; };

private:

    int m_NewPageIndex = -1;
    int m_PreviousPageIndex = -1;

};
