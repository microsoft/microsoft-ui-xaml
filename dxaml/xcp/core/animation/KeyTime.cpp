// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "KeyTime.h"
#include <StringConversions.h>
#include <Clock.h>
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"

_Check_return_ HRESULT CKeyTime::Create(
    _Outptr_ CDependencyObject **result,
    _In_ CREATEPARAMETERS *createParams)
{
    xref_ptr<CKeyTime> newObj;
    newObj.init(new CKeyTime(createParams->m_pCore));

    double timeSpanInSec        = KeyTimeVO::s_defaultTimeSpanInSec;
    float percent               = KeyTimeVO::s_defaultPercent;

    switch (createParams->m_value.GetType())
    {
        case valueString:
            IFC_RETURN(TimeSpanFromString(
                createParams->m_value.As<valueString>(),
                &timeSpanInSec));
            break;

        case valueFloat:
            timeSpanInSec = createParams->m_value.As<valueFloat>();
            break;

        case valueDouble:
            timeSpanInSec = createParams->m_value.As<valueDouble>();
            break;

        case valueTimeSpan:
            {
                // Convert wf::TimeSpan into fractional seconds.
                timeSpanInSec = TimeSpanUtil::ToSeconds(createParams->m_value.As<valueTimeSpan>());
            }
            break;

        case valueVO:
            {
                Flyweight::PropertyValueObjectBase* wrapper = createParams->m_value.As<valueVO>();
                IFCEXPECT_RETURN(wrapper->GetTypeIndex() == KeyTimeVO::s_typeIndex);
                KeyTimeVO::Wrapper* specificWrapper = static_cast<KeyTimeVO::Wrapper*>(wrapper);
                timeSpanInSec = specificWrapper->Value().GetTimeSpanInSec();
            }
            break;

        default:
            ASSERT(FALSE);
             // Fall-through
        case valueAny:
            break;
    }

    newObj->SetPercent(percent);
    newObj->SetTimeSpanInSec(timeSpanInSec);

    *result = newObj.detach();

    return S_OK;
}

KnownTypeIndex CKeyTime::GetTypeIndex() const
{
    return DependencyObjectTraits<CKeyTime>::Index;
}