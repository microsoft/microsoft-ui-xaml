// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "QualifierFlags.h"
#include "IQualifier.h"
#include <enumdefs.h>

class QualifierFactory
{
    public:
        static std::shared_ptr<IQualifier> Create(_In_ QualifierFlags flags, _In_ const XINT32& value);
        static std::shared_ptr<IQualifier> 
            Create(
                QualifierFlags flags);

        static std::shared_ptr<IQualifier> Create(
                _In_ const int& width,
                _In_ const int& height);

        static std::shared_ptr<IQualifier> Create(_In_ const std::vector<int>& serializedQualifier);

        static std::shared_ptr<IQualifier> Create(_In_ const bool* pBool); 
};
