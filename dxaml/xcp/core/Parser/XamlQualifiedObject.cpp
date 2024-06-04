// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

XamlQualifiedObject::~XamlQualifiedObject()
{
    Tidy();
}

void 
XamlQualifiedObject::SetDependencyObjectNoAddRef(CDependencyObject *pdo)
{
    m_value.SetObjectNoRef(pdo);
}

_Check_return_ HRESULT XamlQualifiedObject::SetDependencyObject(CDependencyObject *pdo)
{
    m_value.SetObjectAddRef(pdo);
    RRETURN(S_OK);
}

// TODO: create a stack-helper to manage conversion and lifetime.
_Check_return_ HRESULT XamlQualifiedObject::ConvertForManaged(XamlQualifiedObject& outManagedQualifiedObject) const
{
    // Since "Pegging" a managed peer is not ref-counted, it's best to keep
    // the Peg with only one XamlQualifedObject.  
    XamlBitSet<QualifiedObjectFlags> inFlags = m_flags;
    inFlags.ClearBit(qofHasPeggedManagedPeer);

    IFC_RETURN(outManagedQualifiedObject.SetInternalData(m_typeToken, m_value, inFlags));
    
    return S_OK;
}

HRESULT XamlQualifiedObject::SetInternalData(
            const XamlTypeToken& inTypeToken, 
            const CValue& inValue, 
            const XamlBitSet<QualifiedObjectFlags>& inFlags)
{
    m_typeToken = inTypeToken;
    m_flags = inFlags;
    RRETURN(m_value.CopyConverted(inValue));
}

HRESULT XamlQualifiedObject::AttachInternalData(
    const XamlTypeToken& inTypeToken,
    CValue&& inValue,
    const XamlBitSet<QualifiedObjectFlags>& inFlags)
{
    m_value = std::move(inValue);
    m_typeToken = inTypeToken;
    m_flags = inFlags;
    RRETURN(S_OK);
}

_Check_return_ HRESULT XamlQualifiedObject::SetValue(const XamlTypeToken& inTypeToken, _In_ CDependencyObject* pDO)
{
    XamlBitSet<QualifiedObjectFlags> flags;

    if (ShouldSetPegFlagDuringSetValue(pDO))
    {
        flags.SetBit(qofHasPeggedManagedPeer);
    }

    m_flags = flags;
    m_typeToken = inTypeToken;
    return SetDependencyObject(pDO);
}

_Check_return_ HRESULT XamlQualifiedObject::SetValue(_In_ const CValue &rValue)
{
    RRETURN(SetValue(XamlTypeToken(), rValue));
}

_Check_return_ HRESULT XamlQualifiedObject::SetValue(
            const XamlTypeToken& inTypeToken, 
            _In_ const CValue &rValue)
{
    XamlBitSet<QualifiedObjectFlags> flags;

    if (ShouldSetPegFlagDuringSetValue(rValue))
    {
        flags.SetBit(qofHasPeggedManagedPeer);
    }

    // by default set the qofHasPeggedManagedPeer flag so that we will attempt to 
    // un-peg in the dtor.  This means that *not* attempting to unpeg is a choice that 
    // is made elsewhere. 
    IFC_RETURN(SetInternalData(inTypeToken, rValue,  flags));

    return S_OK;
}

_Check_return_ HRESULT XamlQualifiedObject::AttachValue(_In_ CValue&& rValue)
{
    return AttachValue(XamlTypeToken(), std::move(rValue));
}

_Check_return_ HRESULT XamlQualifiedObject::AttachValue(
    const XamlTypeToken& inTypeToken,
    _In_ CValue&& rValue)
{
    XamlBitSet<QualifiedObjectFlags> flags;

    if (ShouldSetPegFlagDuringSetValue(rValue))
    {
        flags.SetBit(qofHasPeggedManagedPeer);
    }

    // by default set the qofHasPeggedManagedPeer flag so that we will attempt to 
    // un-peg in the dtor.  This means that *not* attempting to unpeg is a choice that 
    // is made elsewhere. 
    IFC_RETURN(AttachInternalData(inTypeToken, std::move(rValue), flags));

    return S_OK;
}

HRESULT XamlQualifiedObject::GetCopyAsString(_Out_ xstring_ptr* pstrOutssString) const
{
    if (m_value.GetType() == valueString)
    {
        *pstrOutssString = m_value.AsString();
    }
    else
    {
        CString* pString = do_pointer_cast<CString>(m_value.AsObject());
        if (pString)
        {
            *pstrOutssString = pString->m_strString;
        }
        else
        {
            pstrOutssString->Reset();
        }
    }

    return S_OK; // RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if XamlQualifiedObject::SetValue should have a bias 
//      towards attempting to un-peg the object that is tracked by this 
//      XamlQualifiedObject.
//  
//  Notes:
//      Normally this would be true due to the fact that a DO can get a 
//      pegged managed peer outside of the control of XamlQualifiedObject
//      or the parser.
//
//      This exception for CTextElement is a work around for the text stack
//      not establishing a gcroot by the time parsing finishes.
//
//      TODO: bug 100114: Remove this workaround and ensure that CTextElement
//      has a gcroot by the time parsing ends.
//
//------------------------------------------------------------------------
bool XamlQualifiedObject::ShouldSetPegFlagDuringSetValue(const CValue& rValue)
{
    return (do_pointer_cast<CTextElement>(rValue) == NULL);
}

bool XamlQualifiedObject::ShouldSetPegFlagDuringSetValue(CDependencyObject* pDO)
{
    return (do_pointer_cast<CTextElement>(pDO) == NULL);
}

_Check_return_ HRESULT XamlQualifiedObject::Create(_In_ CCoreServices *, _In_ const XamlTypeToken& typeToken, _Outptr_ XamlQualifiedObject **ppQO)
{
    *ppQO = new XamlQualifiedObject();
    (*ppQO)->SetTypeToken(typeToken);
    return S_OK;
}

_Check_return_ HRESULT XamlQualifiedObject::Create(_In_ CCoreServices *, _In_ const XamlTypeToken& typeToken, _Out_ std::shared_ptr<XamlQualifiedObject>& outQO)
{
    outQO = std::make_shared<XamlQualifiedObject>();
    outQO->SetTypeToken(typeToken);
    return S_OK;
}

_Check_return_ HRESULT XamlQualifiedObject::Create(_In_ CCoreServices *, _In_ const XamlTypeToken& typeToken, _In_ CDependencyObject* pDO, _Out_ std::shared_ptr<XamlQualifiedObject>& outQO)
{
    auto qo = std::make_shared<XamlQualifiedObject>();
    qo->SetTypeToken(typeToken);
    IFC_RETURN(qo->SetDependencyObject(pDO));
    outQO = std::move(qo);
    return S_OK;
}

_Check_return_ HRESULT XamlQualifiedObject::CreateNoAddRef(_In_ CCoreServices *, _In_ const XamlTypeToken& typeToken, _In_ CDependencyObject* pDO, _Out_ std::shared_ptr<XamlQualifiedObject>& outQO)
{
    outQO = std::make_shared<XamlQualifiedObject>();
    outQO->SetTypeToken(typeToken);
    outQO->SetDependencyObjectNoAddRef(pDO);
    return S_OK;
}

_Check_return_ HRESULT XamlQualifiedObject::CreateFromXStringPtr(_In_ xstring_ptr& inputString, _Out_ std::shared_ptr<XamlQualifiedObject>& createdQO)
{
    std::shared_ptr<XamlSchemaContext> spSchemaContext;
    CValue boxedString;

    boxedString.SetString(inputString);
    IFC_RETURN(createdQO->SetValue(boxedString));

    return S_OK;
}

std::shared_ptr<CDependencyObject> XamlQualifiedObject::GetOwnedDependencyObject()
{
    if (CDependencyObject* asObject = m_value.AsObject())
    {
        asObject->AddRef();

        // If the object has been pegged we want to transfer a ref to that peg
        // to the caller of this API too, and pass a guarantee that the peg will
        // be unpegged when the smart pointer goes out of scope.
        if (GetHasPeggedManagedPeer() && asObject->HasManagedPeer())
        {
            VERIFYHR(asObject->PegManagedPeer());

            return std::shared_ptr<CDependencyObject>(
                asObject, 
                [](CDependencyObject* obj) {
                    obj->UnpegManagedPeer();
                    obj->Release(); });
        }
        else
        {
            return std::shared_ptr<CDependencyObject>(
                asObject,
                [](CDependencyObject* obj) { obj->Release(); });
        }
    }

    return std::shared_ptr<CDependencyObject>();
}

std::shared_ptr<CDependencyObject> XamlQualifiedObject::GetAndTransferDependencyObjectOwnership()
{
    ClearHasPeggedManagedPeer();
    return GetOwnedDependencyObject();
}

void XamlQualifiedObject::Tidy()
{
    // When we create a managed peer, we get it back with a strong ref
    // (there's a strong ref on the peer from the ManagedPeerTable).  That's 
    // necessary, because during the parse of the object, the only managed reference on
    // it is from the MPT.  But now that we're done parsing the object, assuming it's
    // not the root object, its lifetime is being properly managed in some way.  E.g.,
    // if it's a Rectangle in a Canvas, it's being tracked by the visual tree.
    // And so we should weaken the reference.
    //
    // The some exceptions to that:
    // *  If the object ControlsManagedPeerLifetime, then all the logic is in the ref-counting,
    //    and we don't need to do anything special here.
    //
    if (GetHasPeggedManagedPeer())
    {
        CDependencyObject *pdo = GetDependencyObject();

        if (pdo)
        {
            if (pdo->HasManagedPeer()
                && !pdo->ControlsManagedPeerLifetime())
            {
                pdo->UnpegManagedPeerNoRef();
            }
        }
    }
}
