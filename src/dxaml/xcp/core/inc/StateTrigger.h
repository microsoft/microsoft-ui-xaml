// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CStateTrigger : public CStateTriggerBase 
{
public:
    // Creation method
    DECLARE_CREATE(CStateTrigger);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CStateTrigger>::Index;
    }

    static _Check_return_ HRESULT IsActive(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _In_reads_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Inout_ CValue *pResult);

private:
    CStateTrigger(_In_ CCoreServices *pCore) : CStateTriggerBase(pCore) {}
};
