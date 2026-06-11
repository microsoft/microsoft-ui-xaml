// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "IQualifier.h"

class MinHeightQualifier : public IQualifier
{
    public:
        MinHeightQualifier(_In_ XUINT32 minHeight) : 
            m_minHeight(minHeight), 
            m_qualified(false) 
        {}

        bool IsQualified() override;
        XINT32 Score(_In_ QualifierFlags flags) override; 
        void Evaluate(_In_ QualifierContext*) override; 
        QualifierFlags Flags() override; 

    private:
        XUINT32 m_minHeight;
        bool m_qualified;
};
