// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "KeyFrame.h"
#include <KeyFrameCollection.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>

class CCoreServices;
class CTimingCollection;

// Class that sets a target value based on the current position of the primary pointer
// relative to the bounds of a reference object.
class CPointerKeyFrame : public CKeyFrame
{
protected:
    CPointerKeyFrame( _In_ CCoreServices *pCore ) : CKeyFrame(pCore)
    {
        m_value = 0.0f;
        m_pointerValue = 0.0f;
    }

public:
    DECLARE_CREATE(CPointerKeyFrame);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPointerKeyFrame>::Index;
    }

    bool IsDiscrete() override { return false; }

    float GetEffectiveProgress(float rCurrentProgress) override
    {
        return rCurrentProgress;
    }

public:
    XFLOAT m_value;
    XFLOAT m_pointerValue;
};

class CPointerKeyFrameCollection : public CKeyFrameCollection
{
private:
    CPointerKeyFrameCollection(_In_ CCoreServices *pCore)
        : CKeyFrameCollection(pCore)
    {}

public:
    DECLARE_CREATE(CPointerKeyFrameCollection);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPointerKeyFrameCollection>::Index;
    }

    _Check_return_ HRESULT InitializeKeyFrames(_In_ XFLOAT rOneIterationDuration) override;

    const CDOCollection::storage_type& GetSortedCollection() override;
};
