// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "KeyFrame.h"
#include "KeyFrameCollection.h"

class CCoreServices;
class CTimingCollection;

class CObjectKeyFrame : public CKeyFrame
{
protected:
    CObjectKeyFrame( _In_ CCoreServices *pCore )
        : CKeyFrame(pCore)
    {
        m_vValue.SetObjectNoRef(nullptr);
    }

    ~CObjectKeyFrame() override;

public:
    DECLARE_CREATE_RETURN(CObjectKeyFrame, E_UNEXPECTED);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CObjectKeyFrame>::Index;
    }

    XUINT32 ParticipatesInManagedTreeInternal() final
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

public:
    CValue m_vValue;
};

class CDiscreteObjectKeyFrame final : public CObjectKeyFrame
{
protected:
    CDiscreteObjectKeyFrame( _In_ CCoreServices *pCore )
        : CObjectKeyFrame(pCore)
    {}

public:
    DECLARE_CREATE(CDiscreteObjectKeyFrame);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CDiscreteObjectKeyFrame>::Index;
    }

    bool IsDiscrete() override { return true; }

    float GetEffectiveProgress(float rCurrentProgress) override
    {
        // Discrete keyframes clamp to the starting value of the time segment
        return 0;
    }
};

class CObjectKeyFrameCollection final : public CKeyFrameCollection
{
private:
    CObjectKeyFrameCollection(_In_ CCoreServices *pCore)
        : CKeyFrameCollection(pCore)
    {}

public:
    DECLARE_CREATE(CObjectKeyFrameCollection);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CObjectKeyFrameCollection>::Index;
    }
};
