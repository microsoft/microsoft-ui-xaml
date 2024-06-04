// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlTypeTokens.h"
#include "TypeBits.h"
#include "CValue.h"
#include ".\CDependencyObject.h"

struct XamlQualifiedObject;
class CCoreServices;
class CDependencyObject;

// XamlQualifiedBuffer a typed value container to handle the fact that the 
// a tagged union is not adequate to describe a value that could be of any
// arbitrary type.  It is meant to contain any type of value that a CValue
// can contain.
struct XamlQualifiedObject
{
private:
    enum QualifiedObjectFlags
    {
        qofNone = 0x0000,
        // fBoxed
        //
        // when value.m_type == valueAny, (meaning a void*)
        // this indicates whether the underlying value
        // is the *boxed* or *unboxed* instance of
        // the type.
        //
        // if value.m_type is any other value, then this
        // flag is meaningless.
        qofBoxed = 1 << 0,
        // fStringValue 
        // 
        // indicates that what the value contains
        // is neither the boxed, nor unboxed representation 
        // of that type, but rather a string that has yet to 
        // be parsed.
        qofString = 1 << 1,
        //
        // Whoever supplied the buffer has relinquished 
        // ownership of the buffer.
        //
        ////qofBufferRequiresDeletion = 1 << 2,
        //
        // Whether the DO contained in the CValue needs to be released.
        // Since the DO creation is now opaque and no external
        // entity can hold it pre-creation, we need this
        // otherwise in any error case that the DO wasn't extracted
        // it will leak and not be cleaned up by the CValue
        //
        qofContainedDORequiresRelease = 1 << 3,

        qofHasPeggedManagedPeer = 1 << 4,

        qofIsXbfOptimized = 1 << 5,
    };

    XamlTypeToken m_typeToken;
    XamlBitSet<QualifiedObjectFlags> m_flags;
    CValue  m_value;

    HRESULT SetInternalData(
        const XamlTypeToken& inTypeToken,
        const CValue& inValue,
        const XamlBitSet<QualifiedObjectFlags>& inFlags);

    HRESULT AttachInternalData(
        const XamlTypeToken& inTypeToken,
        CValue&& inValue,
        const XamlBitSet<QualifiedObjectFlags>& inFlags);

public:
    XamlQualifiedObject()
    {
    }

    XamlQualifiedObject(_In_ XamlTypeToken typeToken)
        : m_typeToken(typeToken)
    {
    }

    static HRESULT Create(_In_ CCoreServices *pCore, _In_ const XamlTypeToken& typeToken, _Outptr_ XamlQualifiedObject **ppQO);

    static HRESULT Create(_In_ CCoreServices *pCore, _In_ const XamlTypeToken& typeToken, _Out_ std::shared_ptr<XamlQualifiedObject>& outQO);

    static HRESULT Create(_In_ CCoreServices *pCore, _In_ const XamlTypeToken& typeToken, _In_ CDependencyObject* pDO, _Out_ std::shared_ptr<XamlQualifiedObject>& outQO);

    // CreateNoAddRef
    //
    // Create a XamlQualifiedObject to wrap a DO, but don't AddRef.  This should be used
    // in the case that the XamlQualifiedObject is wrapping a newly-created DO, in which case
    // the ref-count will have already been bumped to 1 by its ctor.
    // 
    static HRESULT CreateNoAddRef(_In_ CCoreServices *pCore, _In_ const XamlTypeToken& typeToken, _In_ CDependencyObject* pDO, _Out_ std::shared_ptr<XamlQualifiedObject>& outQO);
       
    _Check_return_ HRESULT CreateFromXStringPtr(_In_ xstring_ptr& objectString,
        _Out_ std::shared_ptr<XamlQualifiedObject>& createdQO);

    XamlQualifiedObject(_In_ const XamlQualifiedObject &rhs) = delete;
    XamlQualifiedObject& operator=(_In_ const XamlQualifiedObject &rhs) = delete;

    XamlQualifiedObject(XamlQualifiedObject&& other)
        : m_typeToken(std::move(other.m_typeToken))
        , m_flags(std::move(other.m_flags))
        , m_value(std::move(other.m_value))
    {
        other.Clear();
    }
    XamlQualifiedObject& operator=(XamlQualifiedObject&& other) noexcept
    {
        if (this != &other)
        {
            std::swap(m_flags, other.m_flags);
            std::swap(m_typeToken, other.m_typeToken);
            std::swap(m_value, other.m_value);
            other.Clear();
        }
        return *this;
    }

    ~XamlQualifiedObject();

    bool IsEmpty() const { return !!m_value.IsNullOrUnset(); }
    bool IsUnset() const { return !!m_value.IsUnset(); }
    void Clear()
    {
        Tidy();
        m_flags.ClearAllBits();
        m_typeToken = XamlTypeToken();
        m_value.Unset();
    }
    
    // Flag accessors
    ////bool GetBufferRequiresDeletion() const       { return m_flags.IsBitSet(qofBufferRequiresDeletion);     }
    bool GetIsStringValue() const                  { return m_flags.IsBitSet(qofString);                     }
    bool GetIsBoxed() const                        { return m_flags.IsBitSet(qofBoxed);                      }
    bool GetContainedDORequiredRelease() const     { return m_flags.IsBitSet(qofContainedDORequiresRelease); }
    bool GetHasPeggedManagedPeer() const           { return m_flags.IsBitSet(qofHasPeggedManagedPeer);       }
    bool GetIsXbfOptimized() const                 { return m_flags.IsBitSet(qofIsXbfOptimized);             }
    void SetIsXbfOptimized()                       { return m_flags.SetBit(qofIsXbfOptimized);               }
    void  SetHasPeggedManagedPeer()                { return m_flags.SetBit(qofHasPeggedManagedPeer);         }

   
    // Accessors
    const CValue& GetValue() const              { return m_value;                              }
    CValue& GetValue()                          { return m_value;                              }
    CValue* GetCValuePtr()                      { return &m_value;                             }
    const XamlTypeToken& GetTypeToken() const   { return m_typeToken;                          }
    const XamlBitSet<QualifiedObjectFlags>& GetFlags() const { return m_flags;                 }

    // Mutators
    void SetTypeToken(XamlTypeToken inToken) { m_typeToken = inToken;    }

    // Proxied methods onto CValue.
    CDependencyObject* GetDependencyObject() const {  return m_value.AsObject(); }

    // Returns a pointer to a CDependencyObject with the ability to be released when
    // needed.
    std::shared_ptr<CDependencyObject> GetOwnedDependencyObject();

    // Returns a pointer to a CDependencyObject with ownership given to the caller.
    std::shared_ptr<CDependencyObject> GetAndTransferDependencyObjectOwnership();

    // Helper for cases where we want a shared pointer string representation.
    HRESULT GetCopyAsString(_Out_ xstring_ptr* pstrOutssString) const;

    _Check_return_ HRESULT ConvertForManaged(XamlQualifiedObject& outManagedQualifiedObject) const;
    _Check_return_ HRESULT SetDependencyObject(CDependencyObject *pdo);
    void SetDependencyObjectNoAddRef(CDependencyObject *pdo);

    _Check_return_ HRESULT SetValue(_In_ const CValue &rValue);
    _Check_return_ HRESULT SetValue(const XamlTypeToken& inTypeToken, _In_ const CValue &rValue);
    _Check_return_ HRESULT SetValue(const XamlTypeToken& inTypeToken, _In_ CDependencyObject* pDO);

    _Check_return_ HRESULT AttachValue(_In_ CValue&& rValue);
    _Check_return_ HRESULT AttachValue(const XamlTypeToken& inTypeToken, _In_ CValue&& rValue);
    
    void ClearHasPeggedManagedPeer()                {   m_flags.ClearBit(qofHasPeggedManagedPeer);  }

private:
    void Tidy();
    
    bool ShouldSetPegFlagDuringSetValue(const CValue& rValue);
    bool ShouldSetPegFlagDuringSetValue(CDependencyObject* pDO);
};


