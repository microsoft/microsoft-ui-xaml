// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#define SC_SYSTEMTYPE 1

// Defines the RC id and type to store XamTypeName[]
#define XAMLTYPENAMES 1060
#define XAMLTYPENAME 1061
#define RT_XAMLTYPENAMES MAKEINTRESOURCE(XAMLTYPENAMES)

// Defines the RC id and type to store UserTypeInfo[]
#define USERTYPES 1050
#define USERTYPEINFO 1051
#define RT_USERTYPES MAKEINTRESOURCE(USERTYPES)

// Defines the RC id and type to store UserMemberInfo[]
#define USERMEMBERS 1040
#define USERMEMBERINFO 1041
#define RT_USERMEMBERS MAKEINTRESOURCE(USERMEMBERS)

// Defines the RC id and type to store UserEnumValueInfo[]
#define USERENUMVALUEINFO 1030
#define RT_USERENUMVALUEINFO MAKEINTRESOURCE(USERENUMVALUEINFO)

// Defines the RC id and type to store the array of indices into UserMemberInfo[]
#define MEMBERINDICES 1070
#define MEMBERINDICE  1071
#define RT_MEMBERINDICES MAKEINTRESOURCE(MEMBERINDICES)

// Defines the RC id and type to store UserEnumInfo[]
#define USERENUMS 1020
#define USERENUMINFO 1021
#define RT_USERENUMS MAKEINTRESOURCE(USERENUMS)

// Defines the RC id and type to store metadadta strings
#define XAMLSTRINGS 1080
#define XAMLSTRING 1081
#define RT_XAMLSTRINGS MAKEINTRESOURCE(XAMLSTRINGS)

namespace Private
{
    struct XamlTypeName
    {
        UINT16 Xamlndex;
        UINT16 idsName;
        INT16 iUserType;
    };

    struct UserTypeInfo
    {
        UINT16 Xamlndex;
        UINT16 idsName;
        INT16  iContentProperty;
        INT16  iBaseType;
        INT16  iEnumIndex;

        // Flags
        UINT16 isArray            : 1;
        UINT16 IsCollection       : 1;
        UINT16 isConstructible    : 1;
        UINT16 isDictionary       : 1;
        UINT16 isMarkupExtension  : 1;

        INT32  iMembers;

        UINT16 activatorId;
        UINT16 enumBoxerId;
        UINT16 addToVectorId;
        UINT16 addToMapId;
    };

    struct UserMemberInfo
    {
        UINT16 XamlIndex;
        UINT16 idsName;
        UINT16 iTargetType;
        UINT16 iType;

        // Flags
        UINT16 isAttachable         : 1;
        UINT16 isDependencyProperty : 1;
        UINT16 isReadOnly           : 1;

        UINT16 setFuncId;
        UINT16 getFuncId;
    };

    // Represents a valid value for an enumeration.
    struct UserEnumValueInfo
    {
        UINT16           idsName;    // Named constant.
        INT16            iValue;     // Actual enum value.
    };

    // Provides information about an enum.
    struct UserEnumInfo
    {
        UINT16                      nTypeIndex;  // Index of the enum
        UINT16                      cValues;     // How many values are in the enum.
        UINT16                      valuesId;    // Values table.
    };

    class __declspec(uuid("11701cf4-670a-4dcf-b085-d6dcbab4b592")) XamlRuntimeType
    {
    public:
        virtual _Check_return_ HRESULT ActivateInstance(_In_ UINT16 typeId, _Outptr_ IInspectable **instance) const;

        virtual _Check_return_ HRESULT GetValue(_In_ UINT16 getFuncId, _In_ IInspectable* instance, _Outptr_result_maybenull_ IInspectable **value) const;

        virtual _Check_return_ HRESULT SetValue(_In_ UINT16 setFuncId, _In_ IInspectable* instance, _In_ IInspectable *value) const;

        virtual _Check_return_ HRESULT BoxEnum(_In_ UINT16 enumBoxerId, _In_ UINT32 enumValue, _Outptr_ IInspectable **ppBoxedEnum) const;

        virtual _Check_return_ HRESULT AddToVector(_In_ UINT16 addToVectorId, _In_ IInspectable* instance, _In_ IInspectable* value) const;

        virtual _Check_return_ HRESULT AddToMap(_In_ UINT16 addToMapId, _In_ IInspectable* instance, _In_ IInspectable* key, _In_ IInspectable* value) const;
        
        virtual _Check_return_ HRESULT EnsureDependencyProperties(_In_ UINT16 typeId) const;

        virtual void ResetDependencyProperties() const;
    };
}

