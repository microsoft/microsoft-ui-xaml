// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace FocusRect {

// Helper to call CustomizeFocusRectangle for a specific type T
template<class T>
void
CallCustomizationFunctionIfExists(
    _In_ CDependencyObject* element,
    _Inout_ FocusRectangleOptions& options,
    _Inout_ bool* shouldDrawFocusRect)
{
    // Give ListViewBaseItemChrome a chance to tweak the render options if it wants
    if (element->OfTypeByIndex<DependencyObjectTraits<T>::Index>())
    {
        auto chrome = static_cast<T*>(element);
        IFCFAILFAST(chrome->CustomizeFocusRectangle(options, shouldDrawFocusRect));
    }
}

// Set Canvas.Left,Canvas.Top on element
void SetElementPosition(
    _In_ CUIElement* element,
    _In_ float x,
    _In_ float y)
{
    CValue leftValue;
    leftValue.SetDouble(x);
    IFCFAILFAST(element->SetValueByKnownIndex(KnownPropertyIndex::Canvas_Left, leftValue));

    CValue topValue;
    topValue.SetDouble(y);
    IFCFAILFAST(element->SetValueByKnownIndex(KnownPropertyIndex::Canvas_Top, topValue));
}

// Set width/height on an element for focus rect rendering
void SetElementSize(
    _In_ CFrameworkElement* fe,
    _In_ float width,
    _In_ float height,
    _Inout_ bool* sizeChanged)
{
    if (fe->m_eWidth == width && fe->m_eHeight == height)
    {
        *sizeChanged = false;
        return;
    }

    *sizeChanged = true;

    // TODO: I don't know why I need to set m_eWidth/m_eHeight and also call SetValue(Width/Height).
    // If I don't call SetValue(Width/Height), software rendering doesn't work because there's no layout storage.
    fe->m_eWidth = width;
    fe->m_eHeight = height;
    {
        CValue val;
        val.SetFloat(width);
        IFCFAILFAST(fe->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Width, val));
    }
    {
        CValue val;
        val.SetFloat(height);
        IFCFAILFAST(fe->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Height, val));
    }
}


// Set solidColorBrush if it's not the same as the original
bool SetBrush(
    _In_ CBrush** brushToSet,
    _In_ CBrush* newBrush)
{
    CSolidColorBrush* solidColorBrushToSet = do_pointer_cast<CSolidColorBrush>(*brushToSet);
    CSolidColorBrush* solidColorBrushNew = do_pointer_cast<CSolidColorBrush>(newBrush);

    if (solidColorBrushToSet && solidColorBrushNew)
    {
        if (solidColorBrushToSet->m_rgb == solidColorBrushNew->m_rgb
            && solidColorBrushToSet->m_eOpacity == solidColorBrushNew->m_eOpacity)
        {
            return false;
        }
    }
    ReplaceInterface(*brushToSet, newBrush);
    return true;
}

// Very specific to focus rect border scenario
xref_ptr<CBorder> EnsureFocusRectBorderAtPosition(
    _In_ CCoreServices* core,
    _In_ CUIElement* parent,
    _In_ unsigned int index)
{
    CUIElementCollection* collection = parent->GetChildren();
    if (collection && index < collection->GetCount())
    {
        return xref_ptr<CBorder>(static_cast<CBorder*>(collection->GetCollection()[index]));
    }

    // By convention, expect index to be pointing to the end of the collection
    ASSERT((!collection && index == 0) || (collection && index == collection->GetCount()));

    xref_ptr<CBorder> newBorder;
    CREATEPARAMETERS cp(core);
    IFCFAILFAST(CreateDO(newBorder.ReleaseAndGetAddressOf(), &cp));

    CValue hitTestVisible;
    hitTestVisible.SetBool(FALSE);
    IFCFAILFAST(newBorder->SetValueByKnownIndex(KnownPropertyIndex::UIElement_IsHitTestVisible, hitTestVisible));

    IFCFAILFAST(parent->AddChild(newBorder.get()));
    return newBorder;
}


void EnsureTranslateTransform(
    _In_ CUIElement* element,
    _In_ float x,
    _In_ float y)
{
    CValue val;
    IFCFAILFAST(element->GetValueByIndex(KnownPropertyIndex::UIElement_RenderTransform, &val));

    CDependencyObject* transform = val.AsObject();
    CTranslateTransform* translation = do_pointer_cast<CTranslateTransform>(transform);

    if (x == 0.0f && y == 0.0f)
    {
        if (transform)
        {
            IFCFAILFAST(element->SetValueByKnownIndex(KnownPropertyIndex::UIElement_RenderTransform, nullptr));
        }
    }
    else
    {
        if (!translation || translation->m_eX != x || translation->m_eY != y)
        {
            xref_ptr<CTranslateTransform> newTransform;
            CREATEPARAMETERS cp(element->GetContext());

            IFCFAILFAST(CreateDO(newTransform.ReleaseAndGetAddressOf(), &cp));

            newTransform->m_eX = x;
            newTransform->m_eY = y;

            IFCFAILFAST(element->SetValueByKnownIndex(KnownPropertyIndex::UIElement_RenderTransform, newTransform.get()));
        }
    }
}

}// namespace
