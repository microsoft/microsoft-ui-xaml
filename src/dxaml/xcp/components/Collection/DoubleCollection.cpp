// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CDependencyObject.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <TrimWhitespace.h>
#include <StringConversions.h>
#include <DoubleCollection.h>
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

_Check_return_ HRESULT
CDoubleCollection::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
)
{
    HRESULT hr = S_OK;
    CDoubleCollection *pDoubleCollection = new CDoubleCollection(pCreate->m_pCore);

    ASSERT((pCreate->m_value.GetType() == valueString) ||
           (pCreate->m_value.GetType() == valueAny) ||
           (pCreate->m_value.GetType() == valueObject && pCreate->m_value.AsObject() != nullptr && pCreate->m_value.AsObject()->OfTypeByIndex<KnownTypeIndex::Double>()));

    if (pCreate->m_value.GetType() == valueString)
    {
        XUINT32 cString = 0;
        const WCHAR* pString = pCreate->m_value.AsEncodedString().GetBufferAndCount(&cString);
        IFC(pDoubleCollection->InitFromString(
            cString,
            pString
            ));
    }

   *ppObject = pDoubleCollection;
   pDoubleCollection = NULL;

Cleanup:
    delete pDoubleCollection;
    return hr;
}

KnownTypeIndex CDoubleCollection::GetTypeIndex() const
{
    return DependencyObjectTraits<CDoubleCollection>::Index;
}

//------------------------------------------------------------------------
//
//  Method:   Append
//
//  Synopsis:
//      Append a double value to the collection.  Because this class
//      is value-based rather than object-based, we need to make a copy of
//      the incoming XFLOAT.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CDoubleCollection::Append(CValue& value, _Out_opt_ XUINT32 *pnIndex)
{
    CValue vValue;

    // We only accept floats
    IFC_RETURN(EnsureValue(value, vValue));

// Store the object pointer in the current block at the current index and hold
// a reference to the object and increment the last valid index

    ASSERT(vValue.GetType() == valueFloat);
    m_items.push_back(vValue.AsFloat());

    if(pnIndex)
    {
       *pnIndex = static_cast<XUINT32>(m_items.size()-1);
    }

    IFC_RETURN(OnAddToCollection(vValue));

    return S_OK;
}

_Check_return_ HRESULT CDoubleCollection::Insert(_In_ XUINT32 nIndex, _In_ CValue& value)
{
    CValue vValue;

    // We only accept floats
    IFC_RETURN(EnsureValue(value, vValue));

    // Redirect calls that would generate an append operation

    if (nIndex >= m_items.size())
    {
        return Append(vValue);
    }

    ASSERT(value.GetType() == valueFloat);
    m_items.insert(m_items.begin() + nIndex, value.AsFloat());

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
CDoubleCollection::RemoveAt(_In_ XUINT32 nIndex)
{
    XFLOAT *pRemove = NULL;
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

// Bail out quickly if the index is out of range

    if (nIndex >= m_items.size())
    {
        return nullptr;
    }

    pRemove = new XFLOAT;
    *pRemove = m_items[nIndex];

    m_items.erase(m_items.begin() + nIndex);

    {
        CValue valueRemoved;
        valueRemoved.SetFloat(*pRemove);
        IFC(OnRemoveFromCollection(valueRemoved, nIndex));
    }

Cleanup:
    return pRemove;
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
CDoubleCollection::GetItemWithAddRef(_In_ XUINT32 nIndex)
{
    XFLOAT *pItem = NULL;
// Bail out quickly if the index is out of range

    if (nIndex >= m_items.size())
    {
        return nullptr;
    }

// Like the insertion operation the GetItem operation is best handled in the
// flattened form.  We waste some memory for performance reasons.

    pItem = &m_items[nIndex];

    return pItem;
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
CDoubleCollection::IndexOf(CValue& value, _Out_ XINT32 *pIndex)
{
    XINT32 iObject = -1;
    XFLOAT flt;
    CValue vValue;

    // We only accept floats
    IFC_RETURN(EnsureValue(value, vValue));

    ASSERT(vValue.GetType() == valueFloat);

    flt = vValue.AsFloat();

    for (unsigned int i=0; i<m_items.size(); i++)
    {
        if (m_items[i] == flt)
        {
            iObject = i;
            break;
        }
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
//  Method:   CDoubleCollection::InitFromString
//
//  Synopsis:
//      Mini-language converter for a double collection
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDoubleCollection::InitFromString(
    _In_ XUINT32 cString,
    _In_reads_(cString) const WCHAR *pString
    )
{
    XFLOAT   rValue;
    CValue   value;

    while (cString)
    {
    // Consume white space

        TrimWhitespace(cString, pString, &cString, &pString);

        if (cString == 0)
        {
            break;
        }

    // Parse the number and add it ot the collection
        const XUINT32 cPrevious = cString;
        IFC_RETURN(FloatFromString(cString, pString, &cString, &pString, &rValue));
        if (cString == cPrevious)
        {
            IFC_RETURN(E_INVALIDARG);
        }

        value.SetFloat(rValue);
        IFC_RETURN(Append(value));

    // We might be have some debris including white space and comma

        TrimWhitespace(cString, pString, &cString, &pString);

        if (cString && (L',' == *pString))
        {
            cString--;
            pString++;

            TrimWhitespace(cString, pString, &cString, &pString);
        }
    }

    SetChangedFlags();

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Trivial implementations of CCollection::OnXYZ overrides, only purpose of which
//      is so we know about any changes and set our dirty flags accordingly.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CDoubleCollection::OnAddToCollection(_In_ const CValue& value)
{
    SetChangedFlags();
    RRETURN(S_OK);
}

_Check_return_ HRESULT
CDoubleCollection::OnRemoveFromCollection(_In_ const CValue& value, _In_ XINT32 iPreviousIndex)
{
    SetChangedFlags();
    RRETURN(S_OK);
}

_Check_return_ HRESULT
CDoubleCollection::OnClear()
{
    SetChangedFlags();
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  When the collection is changed, flag ourselves dirty so the parent knows
//   to pull out new data and redraw if needed.
//
//------------------------------------------------------------------------
void
CDoubleCollection::SetChangedFlags()
{
    // The collection itself doesn't store any dirty state so just notify parents.
    NWPropagateDirtyFlag(DirtyFlags::None);
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
CDoubleCollection::EnsureValue(_In_ const CValue& vOriginal, _Out_ CValue& vCoerced)
{
    ASSERT(&vOriginal != &vCoerced);

    if (vOriginal.IsFloatingPoint())
    {
        vCoerced.SetFloat(static_cast<FLOAT>(vOriginal.AsDouble()));
    }
    else
    {
        const CDouble* pDouble = do_pointer_cast<CDouble>(vOriginal);

        if (pDouble)
        {
            vCoerced.SetFloat(pDouble->m_eValue);
        }
        else
        {
            IFC_RETURN(E_INVALIDARG);
        }
    }

    return S_OK;
}
