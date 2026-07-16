// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolBarStencilButton.g.h"
#include "InkToolBarStencilButton.properties.h"

#include "InkToolBarMenuButton.h"

class InkToolBarStencilButton :
    public winrt::implementation::InkToolBarStencilButtonT<InkToolBarStencilButton, InkToolBarMenuButton>, 
    public InkToolBarStencilButtonProperties
{
public:
    ForwardRefToBaseReferenceTracker(InkToolBarMenuButton)

    // These functions are ambiguous with InkToolBarMenuButton, disambiguate
    using InkToolBarStencilButtonProperties::EnsureProperties;
    using InkToolBarStencilButtonProperties::ClearProperties;

    winrt::InkPresenterRuler Ruler() { return m_ruler; }
    winrt::InkPresenterProtractor Protractor() { return m_protractor; }
    
    winrt::InkToolBarStencilKind SelectedStencil() { return m_selectedStencil; }
    void SelectedStencil(winrt::InkToolBarStencilKind value) { m_selectedStencil = value; }
    
    bool IsRulerItemVisible() { return m_isRulerItemVisible; }
    void IsRulerItemVisible(bool value) { m_isRulerItemVisible = value; }
    
    bool IsProtractorItemVisible() { return m_isProtractorItemVisible; }
    void IsProtractorItemVisible(bool value) { m_isProtractorItemVisible = value; }

    // Called to initialize ruler/protractor when InkPresenter is available
    void SetInkPresenter(winrt::Windows::UI::Input::Inking::InkPresenter const& inkPresenter)
    {
        if (inkPresenter)
        {
            m_ruler = winrt::InkPresenterRuler(inkPresenter);
            m_protractor = winrt::InkPresenterProtractor(inkPresenter);
        }
    }

private:
    winrt::InkPresenterRuler m_ruler{ nullptr };
    winrt::InkPresenterProtractor m_protractor{ nullptr };
    winrt::InkToolBarStencilKind m_selectedStencil{ winrt::InkToolBarStencilKind::Ruler };
    bool m_isRulerItemVisible{ true };
    bool m_isProtractorItemVisible{ true };
};

