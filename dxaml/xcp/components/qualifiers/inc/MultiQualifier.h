// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "IQualifier.h"

class MultiQualifier : public IQualifier
{
    public:
        _Check_return_ bool IsQualified() override;
        _Check_return_ XINT32 Score(_In_ QualifierFlags flags) override; 
        void Evaluate(_In_ QualifierContext*) override; 
        _Check_return_ QualifierFlags Flags() override; 
        void Add(_In_ std::shared_ptr<IQualifier> pQualifier);

    private:
        std::vector<std::shared_ptr<IQualifier>> m_qualifiers;
};


