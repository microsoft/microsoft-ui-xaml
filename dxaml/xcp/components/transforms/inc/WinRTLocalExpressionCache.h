// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// WinRTLocalExpressionCache is a simple container object responsible for caching the expression related objects that support
// the "LOCAL" transform expression - this expression is applied to the Primary visuals' TransformMatrix.
class WinRTLocalExpressionCache
{
public:
    void Reset()
    {
        m_localExpression.Reset();
        m_localExpressionComponentsPS.Reset();
        m_rtoExpression.Reset();
        m_transitionTargetRTOExpression.Reset();
        m_redirectionExpression.Reset();
        m_rtlExpression.Reset();
        m_dmanipTransformPS.Reset();
        m_transformFlags = 0;
    }

    // The final resulting expression (aka the UBER expression) which will target the primary visual's TransformMatrix
    wrl::ComPtr<WUComp::IExpressionAnimation> m_localExpression;

    // This CompositionPropertySet's members are the components making up the full local expression.
    // We will build an expression multiplying these (matrix) transformations in the right order to represent the overall transformation.
    wrl::ComPtr<WUComp::ICompositionPropertySet> m_localExpressionComponentsPS;

    // Transforms build and keep track of their own expressions. When they're given a RenderTransformOrigin, they'll wrap their
    // own expression in an outer expression to handle the RTO. That outer expression needs to be kept alive (WUC's StartAnimation
    // does not add a reference), but it can't live on the Transform, since multiple UIEs sharing that transform can all pass in
    // their own RTOs. We'll keep it alive in the builder, then store it in the comp node that makes use of the outer expression.
    wrl::ComPtr<WUComp::IExpressionAnimation> m_rtoExpression;
    wrl::ComPtr<WUComp::IExpressionAnimation> m_transitionTargetRTOExpression;
    // A similar case exists for redirection...
    wrl::ComPtr<WUComp::IExpressionAnimation> m_redirectionExpression;
    // ...and the RTL flip.
    wrl::ComPtr<WUComp::IExpressionAnimation> m_rtlExpression;

    wrl::ComPtr<WUComp::ICompositionPropertySet> m_dmanipTransformPS;

    UINT m_transformFlags = 0;                  // bit-mask of flags indicating which transform components are in use in the expression
};

