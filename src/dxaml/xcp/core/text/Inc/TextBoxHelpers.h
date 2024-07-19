// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef TEXT_BOX_HELPERS_H
#define TEXT_BOX_HELPERS_H

#include "text.h"
#include "TextAlignment.h"
#include "SelectionWordBreaker.h"

struct IJupiterTextSelection;
class CDependencyProperty;
class CStoryboard;
class CFrameworkElement;
class CTextPosition;
class CDoubleKeyFrameCollection;
namespace RichTextServices
{
    class TextFormatter;
    class TextFormatterCache;
}

enum class TagConversion
{
    None = 0,
    Default
};

//---------------------------------------------------------------------------
//
//  CTextBoxHelpers
//
//  Bunch of text box related methods that are helpful in 
//  TextBox/RichTextBox scenarios.
//
//---------------------------------------------------------------------------
class CTextBoxHelpers
{
public:
    ///
    /// Storyboard related helpers 
    ///

    static float GetCaretBlinkingPeriod();

    // Creates a storyboard for a blinking caret.
    static
    _Check_return_ HRESULT CreateCaretAnimationStoryboard(
        _Outptr_ CStoryboard                        **storyboard,
        _In_ CCoreServices                           *core,
        _In_ CDependencyObject                       *targetObject,
        _In_ const CDependencyProperty               *targetProperty,
        _In_ float                                    maxOpacity,
        _In_ float                                    blinkPeriodInSeconds
        );

    ///
    /// Pixel snapping related helpers
    ///

    // Gets pixel snapped rectangle.
    static
    _Check_return_ HRESULT GetPixelSnappedRectangle(
        _In_ const CMILMatrix    *pRenderTransform, 
        _In_ EditRectangleKind    rectKind,
        _In_ XFLOAT               strokeWidth,
        _Inout_ XRECTF           *pRect
        );

    // Indicates if pixel snapping is appropriate for the given transform.
    static
    _Check_return_ bool TransformAllowsSnapToPixels(
        _In_ const CMILMatrix *pTransform
        );

    // Snaps a selection, caret or focus rectangle to whole screen pixels.
    static
    _Check_return_ HRESULT SnapRectToPixel(
        _In_    const CMILMatrix  *pRenderTransform,
        _In_    XFLOAT             strokeWidth,
        _In_    EditRectangleKind  rectKind,
        _Inout_ XRECTF            *pRect
    );

    // Gets the text flow direction for the given DO.
    static
    _Check_return_ HRESULT GetFlowDirection(
        _In_  CFrameworkElement *pElement,
        _Out_ DirectUI::FlowDirection    *pFlowDirection
        );

    // Creates a zero-duration key frame Storyboard for temporarily "setting" values.
    static
    _Check_return_ HRESULT CreateZeroDurationStoryboard(
        _In_ CCoreServices *pCore,
        _In_ CDependencyObject *pTargetObject, 
        _In_ const CDependencyProperty *pTargetProperty,
        _In_ CValue *pTargetValue,
        _Outptr_ CStoryboard **ppStoryboard);

    // Update parameters of a Storyboard created by CreateZeroDurationStoryboard
    static
    _Check_return_ HRESULT UpdateZeroDurationStoryboard(
        _In_ CStoryboard *pStoryboard,
        _In_opt_ CDependencyObject *pTargetObject, 
        _In_opt_ const CDependencyProperty *pTargetProperty,
        _In_opt_ CValue *pTargetValue);

    static
    _Check_return_ HRESULT IsNotInSurrogateCRLF(
        _In_  ITextContainer *pContainer,
        _In_  XUINT32         offset,
        _Out_ bool          *pIsNotInSurrogateCRLF);

    static 
    _Check_return_ HRESULT VerifyPositionPair(
        _In_ ITextContainer *pTextContainer,
        _In_ XUINT32 iTextPosition1,
        _In_ XUINT32 iTextPosition2);

    static 
    _Check_return_ HRESULT GetText(
        _In_ ITextContainer *pTextContainer,
        _In_ XUINT32         charPosition1,
        _In_ XUINT32         charPosition2,
        _In_ bool           insertNewlines,
        _Out_ CString      **ppString);

    static
    _Check_return_ HRESULT SelectWordFromTextPosition(
        _In_  ITextContainer *pTextContainer,
        _In_  CTextPosition   hitPosition,
        _In_  IJupiterTextSelection *pTextSelection,
        _In_  FindBoundaryType forwardFindBoundaryType,
        _In_  TagConversion     tagConversion);

    static
    _Check_return_ HRESULT GetClosestNonWhitespaceWordBoundary(
        _In_  ITextContainer *pTextContainer,
        _In_  CTextPosition   hitPosition,
        _In_  TagConversion   tagConversion,
        _Out_ CTextPosition  *pClosestPosition);

    static
    _Check_return_ HRESULT GetAdjacentWordBoundaryPosition(
        _In_opt_  ITextContainer   *pTextContainer,
        _In_      CTextPosition     textPosition,
        _In_      FindBoundaryType  findType,
        _In_      TagConversion     tagConversion,
        _In_      HRESULT (*GetAdjacentBoundaryPosition)(CTextPosition,
                                                         ISimpleTextBackend*,
                                                         FindBoundaryType,
                                                         CTextPosition*),
        _Out_     CTextPosition    *pAdjacentPosition,
        _Out_opt_ TextGravity      *peAdjacentGravity);

    static 
    _Check_return_ HRESULT GetAdjacentWordNavigationBoundaryPosition(
        _In_opt_  ITextContainer   *pTextContainer,
        _In_      CTextPosition     textPosition,
        _In_      FindBoundaryType  findType,
        _In_      TagConversion     tagConversion,
        _Out_     CTextPosition    *pAdjacentPosition,
        _Out_opt_ TextGravity      *peAdjacentGravity);

   static 
    _Check_return_ HRESULT GetAdjacentWordSelectionBoundaryPosition(
        _In_opt_  ITextContainer   *pTextContainer,
        _In_      CTextPosition     textPosition,
        _In_      FindBoundaryType  findType,
        _In_      TagConversion     tagConversion,
        _Out_     CTextPosition    *pAdjacentPosition,
        _Out_opt_ TextGravity      *peAdjacentGravity);

    //
    // Gravity flag tests
    //

    static
    bool LineGravityBackward     (TextGravity eGravity) {return  (eGravity & LineBackward) == LineBackward;}

    static
    bool LineGravityForward      (TextGravity eGravity) {return !(eGravity & LineBackward);}

    static
    bool CharacterGravityBackward(TextGravity eGravity) {return  (eGravity & CharacterBackward) == CharacterBackward;}

    static
    bool CharacterGravityForward (TextGravity eGravity) {return !(eGravity & CharacterBackward);}

    // Translates from ::TextAlignment to RichTextServices::TextAlignment.
    static
    RichTextServices::TextAlignment::Enum GetRichTextServicesTextAlignment(_In_ DirectUI::TextAlignment alignment);

    // Frame rate for the caret animation
    static 
    const XFLOAT CARET_ANIMATION_RAMP_FRAME_RATE;

private:
    static
    _Check_return_ HRESULT AppendDoubleDiscreteKeyFrame(
        _In_ CCoreServices                 *pCore,
        _In_ XFLOAT                         rValue,
        _In_ XFLOAT                         rEndTime,
        _Inout_ CDoubleKeyFrameCollection  *pKeyFramesCollection
        );
};


//---------------------------------------------------------------------------
//
//  Text Formatter Factory
//
//---------------------------------------------------------------------------
class CTextFormatterFactory
{

public:

    static
    _Check_return_ HRESULT GetTextFormatter(
        _In_ CCoreServices *pCore,
        _Outptr_ RichTextServices::TextFormatter **ppTextFormatter
        );

    static 
    void ReleaseTextFormatter(
        _In_ CCoreServices *pCore,
        _In_opt_ RichTextServices::TextFormatter *pTextFormatter
        );

private:
    static
    _Check_return_ HRESULT GetTextFormatterCache(
        _In_ CCoreServices *pCore,
        _Outptr_ RichTextServices::TextFormatterCache **ppTextFormatterCache
        );

};

#endif // TEXT_BOX_HELPERS_H

