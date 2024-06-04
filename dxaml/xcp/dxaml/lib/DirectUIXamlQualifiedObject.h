// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents typed values.

#pragma once

#include <XamlTypeTokens.h>

namespace DirectUI
{
    // XamlQualifiedObject represents typed values by combining a CValue, a 
    // XamlTypeToken, and various lifetime flags.  Note that there's a
    // ::XamlQualifiedObject partially imported in from the core (but none of
    // its methods are usable) and our xaml::XamlQualifiedObject here to
    // simplify marshaling DOs to and from XamlQualifiedObjects.
    class XamlQualifiedObject
    {
    private:
        // Lifetime management flags
        enum QualifiedObjectFlags
        {
            qofNone = 0x0000,

            // When value.m_type == valueAny, (meaning a void*) this indicates
            // whether the underlying value is the *boxed* or *unboxed* instance
            // of the type.  If value.m_type is any other value, then this flag
            // is meaningless.
            qofBoxed = 1 << 0,

            // Indicates that what the value contains is neither the boxed, nor
            // unboxed representation  of that type, but rather a string that
            // has yet to be parsed.
            qofString = 1 << 1,
            
            //// Whoever supplied the buffer has relinquished  ownership of the
            //// buffer.
            //qofBufferRequiresDeletion = 1 << 2,
            
            // Whether the DO contained in the CValue needs to be released.
            // Since the DO creation is now opaque and no external entity can
            // hold it pre-creation, we need this otherwise in any error case
            // that the DO wasn't extracted it will leak and not be cleaned up
            // by the CValue
            qofContainedDORequiresRelease = 1 << 3,

            qofHasPeggedManagedPeer = 1 << 4,
        };

        XamlTypeToken m_typeToken;                  // Type of the value
        XamlBitSet<QualifiedObjectFlags> m_flags;   // Lifetime flags
        CValue m_value;                             // The value

    public:

        // Initializes a new instance of the XamlQualifiedObject class.
        XamlQualifiedObject() = default;

        // Get the value of the XamlQualifiedObject.
        const CValue& GetValue() const { return m_value; }
        CValue& GetValue() { return m_value; }

        // Get the type of the XamlQualifiedObject.
        const XamlTypeToken& GetToken() const { return m_typeToken; }
        XamlTypeToken& GetToken() { return m_typeToken; }

        // Get the value of a XamlQualifiedObject.
        _Check_return_ HRESULT GetValue(_Out_ IInspectable** ppValue, _In_ bool fPegNoRef = true ) const;

        // Set the value of a XamlQualifiedObject to a dependency object.
        _Check_return_ HRESULT SetValue(_In_ IInspectable* pValue, _In_ const CClassInfo* pValueType, bool fNewlyCreated = false);
    };
}
