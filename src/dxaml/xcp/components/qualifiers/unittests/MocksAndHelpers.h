// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "precomp.h"
#include "IntegrationTests.h"
#include "XamlLogging.h"
#include <WexTestClass.h>
#include <functional>
#include <cstdint>
#include "VariantMap.h"

// IQualifier test classes

class TestQualifier : public IQualifier
{
        public:
            TestQualifier() : m_qualified(false), m_width(-1), m_height(-1), m_flags((QualifierFlags)0),
                m_evaluateCalled(false) { };
            bool IsQualified() override { return m_qualified; };
            XINT32 Score(QualifierFlags flags) override
            {
                if((flags & QualifierFlags::Height) != QualifierFlags::None) return m_height;
                else if((flags & QualifierFlags::Width) != QualifierFlags::None) return m_width;

                return -1;
            };
            void Evaluate(QualifierContext* qualfierContext) override { m_evaluateCalled = true; };
            QualifierFlags Flags() { return m_flags; };

            bool m_qualified;
            bool m_evaluateCalled;
            XINT32 m_width;
            XINT32 m_height;
            QualifierFlags m_flags;
};

// QualifierContext test classes

// Class has a callback and a 'HasBeenCalledBack' bool
// bool is set when the class is called back
class QualifierContextTestListener : public IQualifierContextCallback
{
    public:
        QualifierContextTestListener() : HasBeenCalledBack(false) { };
        bool HasBeenCalledBack;
        HRESULT OnQualifierContextChanged() { HasBeenCalledBack = true; return S_OK; };
};

