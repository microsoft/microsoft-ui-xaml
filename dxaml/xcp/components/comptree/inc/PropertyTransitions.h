// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CDependencyObject.h"

class DCompTreeHost;

class CScalarTransition final : public CDependencyObject
{
protected:
    CScalarTransition(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {
    }

    ~CScalarTransition() override;

public:
    DECLARE_CREATE(CScalarTransition);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::ScalarTransition;
    }

    WUComp::ICompositionAnimationBase* GetWUCAnimationNoRef(_In_ DCompTreeHost* dcompTreeHost, const wrl::Wrappers::HStringReference& propertyName, float finalValue);
};

class CVector3Transition final : public CDependencyObject
{
protected:
    CVector3Transition(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {
    }

    ~CVector3Transition() override;

public:
    DECLARE_CREATE(CVector3Transition);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::Vector3Transition;
    }

    WUComp::ICompositionAnimationBase* GetWUCAnimationNoRef(
        _In_ DCompTreeHost* dcompTreeHost,
        const wrl::Wrappers::HStringReference& propertyName,
        const wfn::Vector3& finalValue);
};

class CBrushTransition final : public CDependencyObject
{
protected:
    CBrushTransition(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {
    }

    ~CBrushTransition() override;

public:
    DECLARE_CREATE(CBrushTransition);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::BrushTransition;
    }

    wf::TimeSpan GetDuration() const;
};
