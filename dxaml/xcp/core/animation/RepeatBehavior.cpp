// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "RepeatBehavior.h"
#include <StringConversions.h>
#include <Clock.h>
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"

_Check_return_ HRESULT CRepeatBehavior::Create(
    _Outptr_ CDependencyObject **result,
    _In_ CREATEPARAMETERS *createParams)
{
    xref_ptr<CRepeatBehavior> newObj;
    newObj.init(new CRepeatBehavior(createParams->m_pCore));

    double durationInSec                = RepeatBehaviorVO::s_defaultDurationInSec;
    DirectUI::RepeatBehaviorType type   = RepeatBehaviorVO::s_defaultRepeatBehaviorType;
    float count                         = RepeatBehaviorVO::s_defaultCount;

    switch (createParams->m_value.GetType())
    {
        case valueString:
            IFC_RETURN(RepeatBehaviorFromString(
                createParams->m_value.As<valueString>(),
                &type,
                &durationInSec,
                &count));
            break;

        case valueDouble:
            type = DirectUI::RepeatBehaviorType::Duration;
            durationInSec = createParams->m_value.As<valueDouble>();
            break;

        case valueTimeSpan:
            {
                wf::TimeSpan ts = createParams->m_value.As<valueTimeSpan>();
                type = DirectUI::RepeatBehaviorType::Duration;
                // Convert wf::TimeSpan into fractional seconds.
                durationInSec = TimeSpanUtil::ToSeconds(ts);
            }
            break;

        case valueVO:
            {
                Flyweight::PropertyValueObjectBase* wrapper = createParams->m_value.As<valueVO>();
                IFCEXPECT_RETURN(wrapper->GetTypeIndex() == RepeatBehaviorVO::s_typeIndex);
                RepeatBehaviorVO::Wrapper* specificWrapper = static_cast<RepeatBehaviorVO::Wrapper*>(wrapper);
                durationInSec = specificWrapper->Value().GetDurationInSec();
                type = specificWrapper->Value().GetRepeatBehaviorType();
                count = specificWrapper->Value().GetCount();
            }
            break;

        default:
            ASSERT(FALSE);
             // Fall-through
        case valueAny:
            break;
    }

    newObj->SetRepeatBehaviorType(type);
    newObj->SetDurationInSec(durationInSec);
    newObj->SetCount(count);

    *result = newObj.detach();

    return S_OK;
}

KnownTypeIndex CRepeatBehavior::GetTypeIndex() const
{
    return DependencyObjectTraits<CRepeatBehavior>::Index;
}