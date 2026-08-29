// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "IQualifier.h"

class ExtensibleQualifier : public IQualifier
{
    public:
        ExtensibleQualifier(_In_ const bool* pTriggered);

        bool IsQualified() override;
        XINT32 Score(_In_ QualifierFlags flags) override; 
        void Evaluate(_In_ QualifierContext*) override; 
        QualifierFlags Flags() override; 

    private:
        const bool* m_pTriggered;
};
