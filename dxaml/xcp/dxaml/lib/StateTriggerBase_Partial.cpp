// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "StateTriggerBase.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT StateTriggerBase::SetActiveImpl(_In_ BOOLEAN bTriggerValue)
{
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::StateTriggerBase_TriggerState, bTriggerValue));
}



