// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TimingCollection.h"

class CCoreServices;
enum class CompositionAnimationConversionResult : byte;
class CompositionAnimationConversionContext;

// Collection that holds keyframes for keyframe animation types
class CKeyFrameCollection : public CTimingCollection
{
private:
    void OriginateInvalidKeyFrameError();

protected:
    CKeyFrameCollection(_In_ CCoreServices *pCore)
        : CTimingCollection(pCore)
        , m_isSorted(false)
    {}

public:
    bool ShouldEnsureNameResolution() final { return true; }

    virtual _Check_return_ HRESULT InitializeKeyFrames(_In_ XFLOAT rOneIterationDuration);

    virtual const CDOCollection::storage_type& GetSortedCollection();

#pragma region ::Windows::UI::Composition

    CompositionAnimationConversionResult AddCompositionKeyFrames(
        _In_ CompositionAnimationConversionContext* context,
        _Inout_ WUComp::IKeyFrameAnimation* animation);

#pragma endregion

protected:
    bool m_isSorted : 1;
};
