// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CLayoutElement : public CUIElement
{
protected:
    CLayoutElement(_In_ CCoreServices *core);
    ~CLayoutElement() override = default;

public:
    // Getter/Setter for ActualWidth.
    static _Check_return_ HRESULT
        ActualWidth(
            _In_ CDependencyObject *object,
            _In_ unsigned int numArgs,
            _In_reads_(numArgs) CValue *args,
            _In_opt_ IInspectable* valueOuter,
            _Out_ CValue *result);

    // Getter/Setter for ActualHeight
    static _Check_return_ HRESULT
        ActualHeight(
            _In_ CDependencyObject *object,
            _In_ unsigned int numArgs,
            _In_reads_(numArgs) CValue *args,
            _In_opt_ IInspectable* valueOuter,
            _Out_ CValue *result);

public:
    // Field backed properties
    float m_width{ static_cast<float>(XDOUBLE_NAN) };
    float m_minWidth{ 0 };
    float m_maxWidth{ XFLOAT_INF };

    float m_height{ static_cast<float>(XDOUBLE_NAN) };
    float m_minHeight{ 0 };
    float m_maxHeight{ XFLOAT_INF };

    DirectUI::HorizontalAlignment m_horizontalAlignment{ DirectUI::HorizontalAlignment::Stretch };
    DirectUI::VerticalAlignment m_verticalAlignment{ DirectUI::VerticalAlignment::Stretch };

    XTHICKNESS m_margin{};

#if WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
public:
    bool CanHaveChildren() const final { return true; }
    bool GetIsLayoutElement() const final { return true; }

protected:
    virtual KnownPropertyIndex GetWidthProperty() const = 0;
    virtual KnownPropertyIndex GetHeightProperty() const = 0;

private:
    _Check_return_ HRESULT MeasureCore(XSIZEF availableSize, _Out_ XSIZEF& desiredSize) final;
    _Check_return_ HRESULT ArrangeCore(XRECTF finalRect) final;

    // Default implementations of measure and arrange if no attached layout is provided.
    // Implements an algorithm equivalent to a 1 cell grid.
    _Check_return_ HRESULT MeasureWorker(XSIZEF availableSize, _Out_ XSIZEF& desiredSize);
    _Check_return_ HRESULT ArrangeWorker(XSIZEF finalSize, XSIZEF& newFinalSize);

    float GetActualWidth();
    float GetActualHeight();

    void GetMinMaxSize(float& minWidth, float& maxWidth, float& minHeight, float& maxHeight);
#endif // WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
};