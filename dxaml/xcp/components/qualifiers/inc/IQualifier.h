// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "QualifierFlags.h"
#include "QualifierContext.h"

struct IQualifier
{
        virtual ~IQualifier() {};
        virtual bool IsQualified() = 0;
        virtual XINT32 Score(QualifierFlags flags) = 0;
        virtual void Evaluate(QualifierContext* qualifierContext) = 0;
        virtual QualifierFlags Flags() = 0;
};

