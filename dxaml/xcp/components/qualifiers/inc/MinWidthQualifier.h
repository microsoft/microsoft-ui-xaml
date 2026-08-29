// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "IQualifier.h"

class MinWidthQualifier : public IQualifier
{
    public:
        MinWidthQualifier(_In_ int minWidth) : 
            m_minWidth(minWidth), 
            m_qualified(false) 
        { }

        bool IsQualified() override;
        XINT32 Score(_In_ QualifierFlags flags) override; 
        void Evaluate(_In_ QualifierContext*) override; 
        QualifierFlags Flags() override; 

    private:
        XUINT32 m_minWidth;
        bool m_qualified;
};
