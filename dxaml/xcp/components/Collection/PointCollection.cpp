// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CDependencyObject.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <TrimWhitespace.h>
#include <StringConversions.h>
#include <PointCollection.h>
#include <UIElement.h>
#include <collectionbase.h>
#include <DOCollection.h>
#include <DXamlServices.h>
#include <AutoReentrantReferenceLock.h>
#include "ICollectionChangeCallback.h"
#include <CValue.h>
#include <UIElement.h>
#include <double.h>
#include <Point.h>
#include <dopointercast.h>
#include <corep.h>
#include "ICollectionChangeCallback.h"
#include <NoParentShareableDependencyObject.h>
#include <Panel.h>
#include <Template.h>
#include <XAMLItemCollection.h>
#include <CControl.h>
#include <ItemsPresenter.h>
#include <MultiParentShareableDependencyObject.h>
#include <Layouttransition.h>
#include <TransitionCollection.h>
#include <ItemsControl.h>

_Check_return_ HRESULT
CPointCollection::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
)
{
    HRESULT hr = S_OK;
    CPointCollection *theClone;
    XPOINTF pt;
    CValue value;

    theClone = new CPointCollection(pCreate->m_pCore);

// If we're passed a string then call call the point type converter to read all
// of the points one at a time and append them to the collection.

    if (pCreate->m_value.GetType() == valueString)
    {
        UINT32 cString = 0;
        const WCHAR *pString = pCreate->m_value.AsEncodedString().GetBufferAndCount(&cString);

        while (cString)
        {
            IFC(ArrayFromString(cString, pString, &cString, &pString, 2, (XFLOAT *) &pt, FALSE));

        // Skip trailing debris including white space and optional comma before
        // the next point

            TrimWhitespace(cString, pString, &cString, &pString);

            if (cString && (L',' == *pString))
            {
                pString++;
                cString--;

                TrimWhitespace(cString, pString, &cString, &pString);
            }

            value.WrapPoint(&pt);
            IFC(theClone->SetValue(theClone->GetContentProperty(), value));
        }
    }
    else if (pCreate->m_value.GetType() == valuePointArray)
    {
        IFC(theClone->InitFromArray(
            pCreate->m_value.GetArrayElementCount(),
            pCreate->m_value.AsPointArray()));
    }

   *ppObject = (CDependencyObject *) theClone;
    theClone = NULL;

Cleanup:
    delete theClone;
    return hr;
}

KnownTypeIndex CPointCollection::GetTypeIndex() const
{
    return DependencyObjectTraits<CPointCollection>::Index;
}


//------------------------------------------------------------------------
//
//  Method:   Append
//
//  Synopsis:
//      Append a point value to the collection.  Because this class
//      is value-based rather than object-based, we need to make a copy of
//      the incoming CPoint.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CPointCollection::Append(CValue& value, _Out_opt_ UINT32 *pnIndex)
{
    CValue vValue;

    // We only accept points
    IFC_RETURN(EnsureValue(value, vValue));

    ASSERT(vValue.GetType() == valuePoint);
    m_items.push_back(*vValue.AsPoint());

    if (pnIndex)
    {
        *pnIndex = m_items.size();
    }

    IFC_RETURN(OnAddToCollection(vValue));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Insert
//
//  Synopsis:
//      Insert a point value into the collection.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CPointCollection::Insert(_In_ UINT32 nIndex, CValue& value)
{
    CValue vValue;

    // We only accept points
    IFC_RETURN(EnsureValue(value, vValue));

    // Redirect calls that would generate an append operation
    if (nIndex >= m_items.size())
    {
        return Append(vValue);
    }

    // Put the new object in its slot.

    ASSERT(vValue.GetType() == valuePoint);
    m_items.insert(nIndex, *vValue.AsPoint());

    IFC_RETURN(OnAddToCollection(vValue));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   RemoveAt
//
//  Synopsis:
//      Remove a value from the collection.
//
//------------------------------------------------------------------------
_Check_return_ void *
CPointCollection::RemoveAt(_In_ UINT32 nIndex)
{
    HRESULT hr = S_OK;

    // Bail out quickly if the index is out of range

    if (nIndex >= m_items.size())
    {
        return nullptr;
    }

    //allocate memory for the return value;
    std::unique_ptr<XPOINTF> removed = std::make_unique<XPOINTF>(m_items[nIndex]);
    m_items.erase(nIndex);

    {
        CValue valueRemoved;
        valueRemoved.WrapPoint(removed.get());
        IFC(OnRemoveFromCollection(valueRemoved, nIndex));
    }

Cleanup:
    if (SUCCEEDED(hr))
    {
        return removed.release();
    }
    else
    {
        return nullptr;
    }
}

//------------------------------------------------------------------------
//
//  Method:   GetItem
//
//  Synopsis:
//      Retrevies a value from the collection.
//
//------------------------------------------------------------------------

_Check_return_ void *
CPointCollection::GetItemWithAddRef(_In_ UINT32 nIndex)
{
    XPOINTF *value = NULL;

// Bail out quickly if the index is out of range

    if (nIndex >= m_items.size())
    {
        return nullptr;
    }

// Like the insertion operation the GetItem operation is best handled in the
// flattened form.  We waste some memory for performance reasons.

    value = new XPOINTF(m_items[nIndex]);

    return value;
}

//------------------------------------------------------------------------
//
//  Method:   IndexOf
//
//  Synopsis:
//      Retreive the index of a value in the collection.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CPointCollection::IndexOf(CValue& value, _Out_ INT32 *pIndex)
{
    INT32 iObject = 0;
    XPOINTF point;
    CValue vValue;

    // We only accept points
    IFC_RETURN(EnsureValue(value, vValue));

    ASSERT(vValue.GetType() == valuePoint);
    point = *vValue.AsPoint();

    // Walk the collection and try to find the object.
    iObject = m_items.size() - 1;

    // Scan the flattened collection for our object
    while (iObject >= 0)
    {
        if (m_items[iObject] == point)
        {
            break;
        }
        iObject--;
    }

    if (iObject >= 0)
    {
        *pIndex = iObject;
    }
    else
    {
        // this item is not in the collection
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   InitFromArray
//
//  Synopsis:
//      Create a flattened array from an input array
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CPointCollection::InitFromArray(
    _In_ UINT32 cPoints,
    _In_reads_(cPoints) XPOINTF *pPoints
)
{
    if (!cPoints)
    {
        IFC_RETURN(E_UNEXPECTED);
    }

    IFC_RETURN(Clear());

    IFC_RETURN(m_items.resize(cPoints));
    for (UINT i = 0; i < cPoints; i++)
    {
        m_items[i] = pPoints[i];
    }

    OnPointsChanged();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: OnPointsChanged
//
//  Synopsis:
//      When the collection is changed, flag ourselves dirty so the parent
//      knows to pull out new data and redraw if needed.
//
//------------------------------------------------------------------------
void
CPointCollection::OnPointsChanged()
{

    // The collection itself doesn't store any dirty state so just notify parents.
    NWPropagateDirtyFlag(DirtyFlags::None);

    if (IsActive())
    {
        CUIElement *pParent = NULL;

        if (SUCCEEDED(DoPointerCast(pParent, GetParentInternal(false))) && pParent)
        {
            pParent->InvalidateMeasure();
        }
    }
}

_Check_return_ HRESULT
CPointCollection::OnAddToCollection(_In_ const CValue& value)
{
    OnPointsChanged();
    RRETURN(S_OK);
}

_Check_return_ HRESULT
CPointCollection::OnRemoveFromCollection(_In_ const CValue& value, _In_ INT32 iPreviousIndex)
{
    OnPointsChanged();
    RRETURN(S_OK);
}

_Check_return_ HRESULT
CPointCollection::OnClear()
{
    OnPointsChanged();
    RRETURN(S_OK);
}


//------------------------------------------------------------------------
//
//  Method:   EnsureValue
//
//  Synopsis:
//      Helper for functions which accept parameters of type CValue.
//
//      Ensures that a parameter is of the expected type either as a CValue type
//      or a DO that is the "boxed" representation of that type.  If not, then the
//      function will fail with E_INVALIDARG
//
//      ***n.b.: The coerced CValue is for use on the stack and no private
//      copy is made.  If the target valueType requires that the m_peValue be set,
//      for example, the vCoerced may point into the vOriginal.
//------------------------------------------------------------------------
_Check_return_ HRESULT
CPointCollection::EnsureValue(_In_ const CValue& vOriginal, _Out_ CValue& vCoerced)
{
    ASSERT(&vOriginal != &vCoerced);

    if (vOriginal.GetType() == valuePoint)
    {
        vCoerced.CopyConverted(vOriginal);
    }
    else
    {
        const CPoint* pPoint = do_pointer_cast<CPoint>(vOriginal);

        if (pPoint)
        {
            vCoerced.WrapPoint(&pPoint->m_pt);
        }
        else
        {
            IFC_RETURN(E_INVALIDARG);
        }
    }

    return S_OK;
}
