// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

using namespace RichTextServices;

CTextPointerWrapper::CTextPointerWrapper(_In_ CCoreServices *pCore)
: CDependencyObject(pCore)
{
}

CTextPointerWrapper::~CTextPointerWrapper() 
{
}

void CTextPointerWrapper::SetPlainTextPosition(_In_ const CPlainTextPosition &plainTextPosition)
{
    m_plainTextPosition = CPlainTextPosition(plainTextPosition);
}
    
const CPlainTextPosition &CTextPointerWrapper::GetPlainTextPosition() const
{
    return m_plainTextPosition;
}

//------------------------------------------------------------------------
//  Summary:
//      Gets the LogicalDirection of the TextPointer
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextPointerWrapper::GetLogicalDirection(_Out_ XUINT32 *pDirectionEnumValue)
{
    RichTextServices::LogicalDirection::Enum direction;
    TextGravity gravity = LineForwardCharacterBackward;

    *pDirectionEnumValue = 0;
    
    // Plain text position is only queried if it's valid and we haven't outlived the TextContainer.
    if (CheckPositionAndContainerValid())
    {
        IFC_RETURN(m_plainTextPosition.GetGravity(&gravity));
        direction = CTextBoxHelpers::CharacterGravityBackward(gravity) ? RichTextServices::LogicalDirection::Backward : RichTextServices::LogicalDirection::Forward;
        *pDirectionEnumValue = direction;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Gets the offset of the TextPointer
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextPointerWrapper::GetOffset(_Out_ XINT32 *pOffset)
{
    XUINT32 offset = 0;
    *pOffset = 0;

    // Plain text position is only queried if it's valid and we haven't outlived the TextContainer.
    if (CheckPositionAndContainerValid())
    {
        IFC_RETURN(m_plainTextPosition.GetOffset(&offset));
        *pOffset = offset;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Gets the character rect representing the TextPointer
//      in requested direction
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextPointerWrapper::GetCharacterRect(
    _In_  RichTextServices::LogicalDirection::Enum  direction, 
    _Out_ XRECTF                                   *pRect)
{
    HRESULT hr = S_OK;
    XRECTF rect;
    TextGravity gravity = LineForwardCharacterForward;

    EmptyRectF(&rect);

    // Plain text position is only queried if it's valid and we haven't outlived the TextContainer.
    if (CheckPositionAndContainerValid())
    {
        if (direction == RichTextServices::LogicalDirection::Backward)
        {
            gravity = LineForwardCharacterBackward;
        }

        // Plain text path.
        IFC(m_plainTextPosition.GetCharacterRect(gravity, &rect));
    }

Cleanup:
    *pRect = rect;
    return hr;
}

//------------------------------------------------------------------------
//  Summary:
//      Gets the TextElement Parent enclosing the TextPointer.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextPointerWrapper::GetParent(_Outptr_ CDependencyObject **ppParent)
{
    *ppParent = nullptr;
    
    // Plain text position is only queried if it's valid and we haven't outlived the TextContainer.
    if (CheckPositionAndContainerValid())
    {
        IFC_RETURN(m_plainTextPosition.GetLogicalParent(ppParent));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Gets the TextElement Parent enclosing the TextPointer.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextPointerWrapper::GetVisualParent(_Outptr_ CFrameworkElement **ppParent)
{    
    *ppParent = nullptr;
    
    // Plain text position is only queried if it's valid and we haven't outlived the TextContainer.
    if (CheckPositionAndContainerValid())
    {
        IFC_RETURN(m_plainTextPosition.GetVisualParent(ppParent));
    }

    return S_OK;
}

_Check_return_ HRESULT CTextPointerWrapper::Create(
    _In_ CCoreServices *pCore, 
    _In_ const CPlainTextPosition  &plainTextPosition,
    _Outptr_ CTextPointerWrapper **ppTextPointerWrapper
    )
{
    CREATEPARAMETERS createParameters(pCore);
    xref_ptr<CTextPointerWrapper> textPointerWrapper;
    CDependencyObject *pDO = nullptr;
    *ppTextPointerWrapper = nullptr;

    // A TextPointerWrapper must always be created with a valid PlainTextPosition.
    IFCCATASTROPHIC_RETURN(plainTextPosition.IsValid());

    IFC_RETURN(CreateDO(textPointerWrapper.ReleaseAndGetAddressOf(), &createParameters));
    textPointerWrapper->SetPlainTextPosition(plainTextPosition);

    // Obtain the TextContainer associated with the plain text position and add a weak ref to it.
    // TextPointer in user code may outlive its TextContainer. Internally, PlainTextPosition usage
    // will never do this, so TextPointerWrapper is a good place to check for this case. Since PlainTextPosition is
    // required to be valid on creation of TextPointerWrapper, we can safely access the container here.
    pDO = plainTextPosition.GetTextContainer()->GetAsDependencyObject();
    IFCCATASTROPHIC_RETURN(pDO != nullptr);

    textPointerWrapper->m_pTextContainerRef = xref::get_weakref(pDO);

    *ppTextPointerWrapper = textPointerWrapper.detach();

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieves a TextPointer at the given (positive or negative) offset relative
//      to this position. Returns NULL if no such position exists.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextPointerWrapper::GetPositionAtOffset(
    _In_        XINT32                                     offset,
    _In_        RichTextServices::LogicalDirection::Enum   direction,
    _Outptr_    CTextPointerWrapper                      **ppTextPointerWrapper)
{
    TextGravity gravity = LineForwardCharacterBackward;
    CPlainTextPosition plainTextPosition;
    bool foundPosition = false;

    *ppTextPointerWrapper = nullptr;

    if (CheckPositionAndContainerValid())
    {
        gravity = (direction == RichTextServices::LogicalDirection::Forward) ? LineForwardCharacterForward : LineForwardCharacterBackward;

        IFC_RETURN(m_plainTextPosition.GetPositionAtOffset(offset, gravity, &foundPosition, &plainTextPosition)); 
        if (foundPosition)
        {
            IFC_RETURN(Create(GetContext(), plainTextPosition, ppTextPointerWrapper));
        } 
    }

    return S_OK;
}

// This API is used by UIA Text Patterns only. In case the current pointer is invalid which will happen 
// when current offset is more than the total position count, it is safe to return End Pointer of Document.
_Check_return_ HRESULT CTextPointerWrapper::Clone(_In_ CDependencyObject *pTextOwner, _Outptr_ CTextPointerWrapper **ppTextPointerWrapper)
{
    CPlainTextPosition plainTextPosition;
    xref_ptr<CTextPointerWrapper> pStartTextPointerWrapper;
    xref_ptr<CTextPointerWrapper> pTextPointerWrapper;

    *ppTextPointerWrapper = nullptr;

    if (CheckPositionAndContainerValid())
    {
        IFC_RETURN(m_plainTextPosition.Clone(&plainTextPosition));
        IFC_RETURN(Create(GetContext(), plainTextPosition, pTextPointerWrapper.ReleaseAndGetAddressOf()));
    }
    else
    {
        // if range is invalid as the Container is invaild we must fail here and let UIA know that this element is not available.
        if (!m_pTextContainerRef.lock())
        {
            IFC_RETURN(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
        }
        // This should happen only in case of when we are dealing with End Pointer of the whole document. 
        // It is safe to clone it End of the document so as to not break the continuous reading.
        IFC_RETURN(CTextAdapter::GetContentEndPointers(pTextOwner, pStartTextPointerWrapper.ReleaseAndGetAddressOf(), pTextPointerWrapper.ReleaseAndGetAddressOf()));
    }
    *ppTextPointerWrapper = pTextPointerWrapper.detach();

    return S_OK;
}
