// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "QualifierFlags.h"
#include "QualifierContext.h"

struct IQualifier
{
        virtual ~IQualifier() {};
        virtual _Check_return_ bool IsQualified() = 0;
        virtual _Check_return_ XINT32 Score(QualifierFlags flags) = 0;
        virtual void Evaluate(_In_ QualifierContext* qualifierContext) = 0;
        virtual _Check_return_ QualifierFlags Flags() = 0;
};

