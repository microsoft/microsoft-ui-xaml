// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CCoreServices;
class CVisualTransition;
class CStoryboard;
class CFrameworkElement;
class CVisualState;
class ResolvedVisualStateSetterCollection;

namespace DynamicTransitionStoryboardGenerator {
    _Check_return_ HRESULT
    GenerateDynamicTransitionAnimations(
        _In_ CFrameworkElement* targetElement,
        _In_ CVisualTransition* transition,
        _In_ const std::vector<CStoryboard*>& activeStoryboards,
        _In_opt_ CStoryboard* destinationStoryboard,
        _Outptr_ CStoryboard** dynamicStoryboard);

    // Creates a new Storyboard that transitions smoothly
    // from activeStoryboards to destinationStoryboard.
    _Check_return_ HRESULT GenerateDynamicTransitionAnimations(
        _In_ CFrameworkElement* targetElement,
        _In_ CVisualTransition* transition,
        _In_ const std::vector<CStoryboard*>& activeStoryboards,
        _In_opt_ CStoryboard* destinationStoryboard,
        _In_ const VisualStateSetterHelper::ResolvedVisualStateSetterCollection& activeSetters,
        _In_ const VisualStateSetterHelper::ResolvedVisualStateSetterCollection& destinationSetters,
        _Outptr_ CStoryboard** dynamicStoryboard);
}