// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "Duration.h"
#include <StringConversions.h>
#include <Clock.h>
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"

_Check_return_ HRESULT CDuration::Create(
    _Outptr_ CDependencyObject **result,
    _In_ CREATEPARAMETERS *createParams)
{
    xref_ptr<CDuration> newObj;
    newObj.init(new CDuration(createParams->m_pCore));

    double timeSpanInSec            = DurationVO::s_defaultTimeSpanInSec;
    DirectUI::DurationType type     = DurationVO::s_defaultDurationType;

    switch (createParams->m_value.GetType())
    {
        case valueString:
            IFC_RETURN(DurationFromString(
                createParams->m_value.As<valueString>(),
                &type,
                &timeSpanInSec));
            break;

        case valueFloat:
            // for compatibility with XBF
            type = DirectUI::DurationType::TimeSpan;
            timeSpanInSec = createParams->m_value.As<valueFloat>();
            break;

        case valueDouble:
            type = DirectUI::DurationType::TimeSpan;
            timeSpanInSec = createParams->m_value.As<valueDouble>();
            break;

        case valueTimeSpan:
            {
                wf::TimeSpan ts = createParams->m_value.As<valueTimeSpan>();
                type = DirectUI::DurationType::TimeSpan;
                // Convert wf::TimeSpan into fractional seconds.
                timeSpanInSec = TimeSpanUtil::ToSeconds(ts);
            }
            break;

        case valueVO:
            {
                Flyweight::PropertyValueObjectBase* wrapper = createParams->m_value.As<valueVO>();
                IFCEXPECT_RETURN(wrapper->GetTypeIndex() == DurationVO::s_typeIndex);
                DurationVO::Wrapper* specificWrapper = static_cast<DurationVO::Wrapper*>(wrapper);
                type = specificWrapper->Value().GetDurationType();
                timeSpanInSec = specificWrapper->Value().GetTimeSpanInSec();
            }
            break;

        default:
            ASSERT(FALSE);
             // Fall-through
        case valueAny:
            break;
    }

    newObj->SetDurationType(type);
    newObj->SetTimeSpanInSec(timeSpanInSec);

    *result = newObj.detach();

    return S_OK;
}

KnownTypeIndex CDuration::GetTypeIndex() const
{
    return DependencyObjectTraits<CDuration>::Index;
}