// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "MonochromaticOverlayPresenter.g.h"
#include "MonochromaticOverlayPresenter.properties.h"

#pragma warning(push)
#pragma warning(disable: 6101)  // Returning uninitialized memory '<value>'.  A successful path through the function does not set the named _Out_ parameter.
#include "microsoft.ui.private.composition.effects_impl.h"
#pragma warning(pop)

class MonochromaticOverlayPresenter :
    public ReferenceTracker<MonochromaticOverlayPresenter, winrt::implementation::MonochromaticOverlayPresenterT>,
    public MonochromaticOverlayPresenterProperties
{

public:
    MonochromaticOverlayPresenter();
    ~MonochromaticOverlayPresenter() {}

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    void UpdateBrush();
    void InvalidateBrush();

    winrt::CompositionEffectFactory _effectFactory{ nullptr };
    winrt::Color _replacementColor{};
    bool _needsBrushUpdate{};
};
