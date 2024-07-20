// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ValueType.h"
#include "StaticAssertFalse.h"
#include "Indexes.g.h"

interface IInspectable;
struct XTABLE;
class CCustomClassInfo;
class CPropertyBase;
class CDependencyProperty;
class CCustomDependencyProperty;
class CCustomProperty;
class CSimpleProperty;
class CObjectDependencyProperty;
class CRenderDependencyProperty;
class CEnterDependencyProperty;
class CDependencyObject;
class CEventArgs;
class CCoreServices;
class CREATEPARAMETERS;
class CValue;
class CFrameworkElement;

enum class FocusVisualType : uint8_t;

namespace DirectUI { class DependencyObject; }

typedef _Check_return_ HRESULT(__stdcall *CREATEPFN)(_Outptr_ CDependencyObject** ppObject, _In_ CREATEPARAMETERS* pCreate);
typedef DirectUI::DependencyObject* (__stdcall *CREATEPFNFX)(_In_ IInspectable* const pOuter);
typedef HRESULT(__stdcall *METHODPFN)(_In_ CDependencyObject* pObject, _In_ UINT32 cArgs, _Inout_updates_(cArgs) CValue* pArgs, _In_opt_ IInspectable* pValueOuter, _Out_ CValue* pResult);

#include "CreateGroupFn.h"
#include "DirtyFlags.h"

enum class MetaDataTypeInfoFlags : UINT32
{
    None                                = 0x00000000,
    IsPublic                            = 0x00000001, // The type is public
    IsConstructible                     = 0x00000002, // The type can be constructed
    IsInterface                         = 0x00000004, // The type represents an interface
    IsCollection                        = 0x00000008, // The type implements ICollection
    IsDictionary                        = 0x00000010, // The type implements IDictionary
    IsMarkupExtension                   = 0x00000020, // The type represents a markup extension
    IsISupportInitialize                = 0x00000040, // The type implements ISupportInitialize
    IsValueType                         = 0x00000080, // The type is value type
    HasTypeConverter                    = 0x00000100, // The type has a type converter (from strings)
    IsWhitespaceSignificant             = 0x00000200, // Whitespace is significant when parsing this type
    TrimSurroundingWhitespace           = 0x00000400, // Surrounding whitespace should be trimmed when parsing this type
    IsEnum                              = 0x00000800, // The type is an enum
    IsCustomType                        = 0x00001000, // The type is a custom type
    IsPrimitive                         = 0x00002000, // The type is a primitive
    ExecutedClassConstructor            = 0x00004000, // The class constructor has executed in the current process.
    RequiresPeerActivation              = 0x00008000, // The type requires a framework peer to be activated because it has custom logic/state.
    IsStrict                            = 0x00010000, // The type is strict.
    IsCompactEnum                       = 0x00020000, // This is a compact (8-bit) enumeration
};
DEFINE_ENUM_FLAG_OPERATORS(MetaDataTypeInfoFlags);

enum class MetaDataPropertyInfoFlags : UINT32
{
    None                                = 0x00000000,
    IsSparse                            = 0x00000001,
    AffectMeasure                       = 0x00000002,
    AffectArrange                       = 0x00000004,
    IsAttached                          = 0x00000008,
    IsPublic                            = 0x00000010,
    IsReadOnlyProperty                  = 0x00000020,
    IsOnDemandProperty                  = 0x00000040,
    IsInheritedProperty                 = 0x00000080,
    IsPropMethodCall                    = 0x00000100,
    IsStorageGroup                      = 0x00000200,
    RequiresMultipleAssociationCheck    = 0x00000400,
    IsCustomProperty                    = 0x00000800, // Property is a regular property.
    IsCustomDependencyProperty          = 0x00001000,
    IsNullable                          = 0x00002000,
    NeedsInvoke                         = 0x00004000,
    IsExternalReadOnlyProperty          = 0x00008000,
    StoreDoubleAsFloat                  = 0x00010000,
    HadFieldInBlue                      = 0x00020000,
    /* unused bit                       = 0x00040000, */
    IsVisualTreeProperty                = 0x00080000,
    IsSimpleProperty                    = 0x00100000,
    IsStrictOnlyProperty                = 0x00200000, // Indicates the property is only allowed on strict types/objects
    IsNonStrictOnlyProperty             = 0x00400000, // Indicates the property is only allowed on non-strict objects
    IsInvalid                           = 0x80000000  // Runtime flag indicating associated metadata is not valid.
};
DEFINE_ENUM_FLAG_OPERATORS(MetaDataPropertyInfoFlags);

enum class MetaDataEnterPropertyInfoFlags : UINT16
{
    None                                = 0x0000,
    DoNotEnterLeave                     = 0x0001,
    NeedsInvoke                         = 0x0002,
    IsObjectProperty                    = 0x0004
};
DEFINE_ENUM_FLAG_OPERATORS(MetaDataEnterPropertyInfoFlags);

struct MetaDataNamespaceNonAggregate
{
    KnownNamespaceIndex m_nIndex;
    xstring_ptr_storage m_strNameStorage;

    MetaDataNamespaceNonAggregate();
};

struct MetaDataTypeNonAggregate
{
    KnownTypeIndex            m_nIndex;
    KnownTypeIndex            m_nBaseTypeIndex;
    MetaDataTypeInfoFlags     m_flags;

    MetaDataTypeNonAggregate();
};

struct MetaDataPropertyNonAggregate
{
    KnownPropertyIndex          m_nIndex;
    KnownTypeIndex              m_nPropertyTypeIndex;
    KnownTypeIndex              m_nDeclaringTypeIndex;
    KnownTypeIndex              m_nTargetTypeIndex;
    MetaDataPropertyInfoFlags   m_flags;

    MetaDataPropertyNonAggregate();
};

struct MetaDataEnterPropertyNonAggregate
{
    UINT16                            m_nOffset;
    UINT16                            m_nGroupOffset;
    MetaDataEnterPropertyInfoFlags    m_flags;
    KnownPropertyIndex                m_nPropertyIndex;
    UINT16                            m_nNextEnterPropertyIndex;

    MetaDataEnterPropertyNonAggregate();
};

struct MetaDataObjectPropertyNonAggregate
{
    UINT16                            m_nOffset;
    UINT16                            m_nGroupOffset;
    KnownPropertyIndex                m_nPropertyIndex;
    UINT16                            m_nNextObjectPropertyIndex;

    MetaDataObjectPropertyNonAggregate();
};

struct MetaDataRenderPropertyNonAggregate
{
    UINT16                            m_nOffset;
    UINT16                            m_nGroupOffset;
    KnownPropertyIndex                m_nPropertyIndex;
    UINT16                            m_nNextRenderPropertyIndex;

    MetaDataRenderPropertyNonAggregate();
};

template <typename T>
inline bool IsFlagSetHelper(T flags, T flag)
{
    return (flags & flag) == flag;
}

class CNamespaceInfo : public MetaDataNamespaceNonAggregate
{
public:
    KnownNamespaceIndex GetIndex() const
    {
        return m_nIndex;
    }

    xstring_ptr GetName() const
    {
        return xstring_ptr(m_strNameStorage);
    }

    bool IsBuiltinNamespace() const
    {
        return (static_cast<UINT>(m_nIndex) < KnownNamespaceCount);
    }
};

class CClassInfo : public MetaDataTypeNonAggregate
{
protected:
    bool IsSet(MetaDataTypeInfoFlags flag) const
    {
        return IsFlagSetHelper(m_flags, flag);
    }

public:
    const CCustomClassInfo* AsCustomType() const
    {
        ASSERT(!IsBuiltinType());
        return reinterpret_cast<const CCustomClassInfo*>(this);
    }

    const CREATEPFN GetCoreConstructor() const;
    const CREATEPFNFX GetFrameworkConstructor() const;

    const CClassInfo* GetBaseType() const;

    const CDependencyProperty*          GetContentProperty() const;

    const CPropertyBase*                GetFirstProperty() const;
    const CEnterDependencyProperty*     GetFirstEnterProperty() const;
    const CObjectDependencyProperty*    GetFirstObjectProperty() const;
    const CRenderDependencyProperty*    GetFirstRenderProperty() const;

    KnownTypeIndex GetIndex() const
    {
        return m_nIndex;
    }

    const CNamespaceInfo* GetNamespace() const;

    REFIID GetGuid() const;

    xstring_ptr GetFullName() const;
    xstring_ptr GetName() const;

    bool HasTypeConverter() const          { return IsSet(MetaDataTypeInfoFlags::HasTypeConverter); }
    bool IsBindable() const;
    bool IsBuiltinType() const             { return !IsSet(MetaDataTypeInfoFlags::IsCustomType); }
    bool IsCollection() const              { return IsSet(MetaDataTypeInfoFlags::IsCollection); }
    bool IsConstructible() const           { return IsSet(MetaDataTypeInfoFlags::IsConstructible); }
    bool IsDictionary() const              { return IsSet(MetaDataTypeInfoFlags::IsDictionary); }
    bool IsInterface() const               { return IsSet(MetaDataTypeInfoFlags::IsInterface); }
    bool IsEnum() const                    { return (m_nIndex == KnownTypeIndex::Enumerated) || IsSet(MetaDataTypeInfoFlags::IsEnum); }
    bool IsCompactEnum() const             { return IsSet(MetaDataTypeInfoFlags::IsCompactEnum); }
    bool IsISupportInitialize() const      { return IsSet(MetaDataTypeInfoFlags::IsISupportInitialize); }
    bool IsMarkupExtension() const         { return IsSet(MetaDataTypeInfoFlags::IsMarkupExtension); }
    bool IsNullable() const                { return !IsValueType(); }
    bool IsNumericType() const
    {
        // Enum is not really a numeric value, even though it is stored as a number
        switch (m_nIndex)
        {
            case KnownTypeIndex::Int32:
            case KnownTypeIndex::Char16:
            case KnownTypeIndex::Int16:
            case KnownTypeIndex::UInt16:
            case KnownTypeIndex::UInt32:
            case KnownTypeIndex::Double:
            case KnownTypeIndex::Int64:
            case KnownTypeIndex::UInt64:
            case KnownTypeIndex::Float:
                return true;
            default:
                return false;
        }
    }
    bool IsPrimitive() const               { return IsSet(MetaDataTypeInfoFlags::IsPrimitive); }
    bool IsPublic() const                  { return IsSet(MetaDataTypeInfoFlags::IsPublic); }
    bool IsUnknown() const                 { return (m_flags == MetaDataTypeInfoFlags::IsCustomType); }
    bool IsValueType() const               { return IsSet(MetaDataTypeInfoFlags::IsValueType); }
    bool IsWhitespaceSignificant() const   { return IsSet(MetaDataTypeInfoFlags::IsWhitespaceSignificant); }
    bool RequiresPeerActivation() const    { return IsSet(MetaDataTypeInfoFlags::RequiresPeerActivation) || IsISupportInitialize(); }
    bool TrimSurroundingWhitespace() const { return IsSet(MetaDataTypeInfoFlags::TrimSurroundingWhitespace); }
    bool IsStrict() const                  { return IsSet(MetaDataTypeInfoFlags::IsStrict); }

    _Check_return_ HRESULT RunClassConstructorIfNecessary();
};

#if !defined(EXP_CLANG)

// Make sure that the binary layout of CClassInfo doesn't change for compatibility reasons with SDK tools.
static_assert(offsetof(CClassInfo, m_nIndex) == 0, "Sadly, you cannot change the binary layout of CClassInfo!");
static_assert(offsetof(CClassInfo, m_nBaseTypeIndex) == sizeof(KnownTypeIndex), "Sadly, you cannot change the binary layout of CClassInfo!");
static_assert(offsetof(CClassInfo, m_flags) == (offsetof(CClassInfo, m_nBaseTypeIndex) + sizeof(KnownTypeIndex)), "Sadly, you cannot change the binary layout of CClassInfo!");

// This static assert is for the overall size of the CClassInfo, please see the owners of any XAML Tools that are shipped in the SDK to make sure this is ok (Visual Diagnostics, GenXbf, etc)
// If this won't cause compat issues, or if the tooling can be updated to accommodate this change, then update this static_assert along with your change.
// This value should always be offsetof(lastMember) + sizeof(lastMember).
static_assert(sizeof(CClassInfo) == (offsetof(CClassInfo, m_flags) + sizeof(MetaDataTypeInfoFlags)), "Please see owners of any XAML tools shipped in SDK to makes sure that this change won't break compatibility");

#endif // EXP_CLANG

// These static asserts are for the sizes of the members of CClassInfo. These are necessary because if these were to change, the above static asserts would pass and that would be undesired.
static_assert(sizeof(KnownTypeIndex) == sizeof(unsigned short), "Please see owners of any XAML tools shipped in SDK to makes sure that this change won't break compatibility");
static_assert(sizeof(MetaDataTypeInfoFlags) == sizeof(int), "Please see owners of any XAML tools shipped in SDK to makes sure that this change won't break compatibility");

// We need a static assert of the size of KnownPropertyIndex as well, we cast some integers to this type and pass down to InternalDebugInterop... :(
static_assert(sizeof(KnownPropertyIndex) == sizeof(unsigned short), "Please see owners of any XAML tools shipped in SDK to makes sure that this change won't break compatibility");

class CPropertyBase : protected MetaDataPropertyNonAggregate
{
protected:
    CPropertyBase(
        KnownPropertyIndex index,
        KnownTypeIndex propertyTypeIndex,
        KnownTypeIndex declaringTypeIndex,
        KnownTypeIndex targetTypeIndex,
        MetaDataPropertyInfoFlags flags)
    {
        m_nIndex = index;
        m_flags = flags;
        m_nPropertyTypeIndex = propertyTypeIndex;
        m_nDeclaringTypeIndex = declaringTypeIndex;
        m_nTargetTypeIndex = targetTypeIndex;
    }

    bool IsSet(MetaDataPropertyInfoFlags flag) const
    {
        return IsFlagSetHelper(m_flags, flag);
    }

public:
    template <typename T>
    const T* AsOrNull() const
    {
        if (Is<T>())
        {
            return static_cast<const T*>(this);
        }
        else
        {
            return nullptr;
        }
    }

    template <typename T> bool Is() const
    {
        static_assert_false("override it");
    }

    template <> bool Is<CCustomProperty>() const                { return IsSet(MetaDataPropertyInfoFlags::IsCustomProperty); }
    template <> bool Is<CCustomDependencyProperty>() const      { return IsSet(MetaDataPropertyInfoFlags::IsCustomDependencyProperty); }
    template <> bool Is<CSimpleProperty>() const                { return IsSet(MetaDataPropertyInfoFlags::IsSimpleProperty); }
    template <>
    bool Is<CDependencyProperty>() const
    {
        return IsKnownDependencyPropertyIndexHelper() || Is<CCustomProperty>() || Is<CCustomDependencyProperty>();
    }

    bool IsReadOnly() const                             { return IsSet(MetaDataPropertyInfoFlags::IsReadOnlyProperty); }
    bool IsPublic() const                               { return IsSet(MetaDataPropertyInfoFlags::IsPublic); }
    bool IsDirective() const                            { return (GetIndex() == KnownPropertyIndex::DependencyObject_Name); }
    bool IsStrictOnly() const                           { return IsSet(MetaDataPropertyInfoFlags::IsStrictOnlyProperty); }
    bool IsNonStrictOnly() const                        { return IsSet(MetaDataPropertyInfoFlags::IsNonStrictOnlyProperty); }

    xstring_ptr GetName() const;

    KnownPropertyIndex GetIndex() const
    {
        return m_nIndex;
    }

    KnownTypeIndex GetDeclaringTypeIndex() const
    {
        return m_nDeclaringTypeIndex;
    }

    KnownTypeIndex GetPropertyTypeIndex() const
    {
        return m_nPropertyTypeIndex;
    }

    KnownTypeIndex GetTargetTypeIndex() const
    {
        return m_nTargetTypeIndex;
    }

    const CClassInfo* GetDeclaringType() const;
    const CClassInfo* GetPropertyType() const;
    const CClassInfo* GetTargetType() const;

    const CPropertyBase* GetNextProperty() const;

    bool IsKnownDependencyPropertyIndexHelper() const;

#if defined(__XAML_UNITTESTS__)
protected:
    CPropertyBase() = default;

public:
    void SetIndex(KnownPropertyIndex index)             { m_nIndex = index; }
    void SetPropertyTypeIndex(KnownTypeIndex index)     { m_nPropertyTypeIndex = index; }
    void SetDeclaringTypeIndex(KnownTypeIndex index)    { m_nDeclaringTypeIndex = index; }
    void SetFlags(MetaDataPropertyInfoFlags flags)      { m_flags = flags; }
#endif
};

class CDependencyProperty : public CPropertyBase
{
public:
    bool AffectsArrange() const                         { return IsSet(MetaDataPropertyInfoFlags::AffectArrange); }
    bool AffectsMeasure() const                         { return IsSet(MetaDataPropertyInfoFlags::AffectMeasure); }
    bool RequiresMultipleAssociationCheck() const       { return IsSet(MetaDataPropertyInfoFlags::RequiresMultipleAssociationCheck); }
    bool HadFieldInBlue() const                         { return IsSet(MetaDataPropertyInfoFlags::HadFieldInBlue); }
    bool IsAttached() const                             { return IsSet(MetaDataPropertyInfoFlags::IsAttached); }
    bool IsNullable() const                             { return IsSet(MetaDataPropertyInfoFlags::IsNullable); }
    bool IsOnDemandProperty() const                     { return IsSet(MetaDataPropertyInfoFlags::IsOnDemandProperty); }
    bool IsPropMethodCall() const                       { return IsSet(MetaDataPropertyInfoFlags::IsPropMethodCall); }
    bool IsExternalReadOnly() const                     { return IsSet(MetaDataPropertyInfoFlags::IsExternalReadOnlyProperty); }
    bool IsSparse() const                               { return IsSet(MetaDataPropertyInfoFlags::IsSparse); }
    bool IsInherited() const                            { return IsSet(MetaDataPropertyInfoFlags::IsInheritedProperty); }
    bool IsStorageGroup() const                         { return IsSet(MetaDataPropertyInfoFlags::IsStorageGroup); }
    bool IsVisualTreeProperty() const                   { return IsSet(MetaDataPropertyInfoFlags::IsVisualTreeProperty); }
    bool NeedsInvoke() const                            { return IsSet(MetaDataPropertyInfoFlags::NeedsInvoke); }
    bool StoreDoubleAsFloat() const                     { return IsSet(MetaDataPropertyInfoFlags::StoreDoubleAsFloat); }
    bool IsValid() const                                { return !IsSet(MetaDataPropertyInfoFlags::IsInvalid); }

    bool IsContentProperty() const                      { return (this == GetDeclaringType()->GetContentProperty()); }
    bool IsInheritedAttachedPropertyInStorageGroup() const
    {
        return IsSet(MetaDataPropertyInfoFlags::IsInheritedProperty | MetaDataPropertyInfoFlags::IsAttached | MetaDataPropertyInfoFlags::IsStorageGroup);
    }

    // Meant to check if a custom DP is initialized.
    bool IsInitialized() const                          { return (GetPropertyTypeIndex() != KnownTypeIndex::UnknownType); }

    bool IsStatic() const                               { return IsAttached(); }
    bool IsGridLengthProperty() const
    {
        return (GetIndex() == KnownPropertyIndex::FrameworkElement_Width) ||
               (GetIndex() == KnownPropertyIndex::FrameworkElement_Height) ||
               (GetIndex() == KnownPropertyIndex::SplitView_OpenPaneLength);
    }
    bool PreserveThemeResourceExtension() const         { return GetIndex() == KnownPropertyIndex::Setter_Value; }

    // If the source property of a Binding/TemplateBinding is an on-demand property,
    // then the simple act of the Binding querying the source property will cause
    // the default value to be created (asuming that it didn't already have a value set on it).
    // In RS5, we fixed our TransitionCollection properties so that they were marked as
    // on-demand properties, but this also resulted in empty TransitionCollections being created
    // in places where they previously were not. To avoid the perf impact, those properties were
    // special-cased so that Binding/TemplateBinding would *not* be the reason they were created
    // on-demand. (Of course, if the create-on-demand nature of the property in question is disabled
    // via quirk, then 'false' should be returned because the proeprty isn't create-on-demand)
    const bool ShouldBindingGetValueUseCheckOnDemandProperty() const
    {
        auto const index = GetIndex();

        switch (index)
        {
            case KnownPropertyIndex::Panel_ChildrenTransitions:
            case KnownPropertyIndex::ItemsControl_ItemContainerTransitions:
            case KnownPropertyIndex::Border_ChildTransitions:
            case KnownPropertyIndex::ContentControl_ContentTransitions:
            case KnownPropertyIndex::ContentPresenter_ContentTransitions:
            case KnownPropertyIndex::ItemsPresenter_HeaderTransitions:
            case KnownPropertyIndex::ItemsPresenter_FooterTransitions:
            case KnownPropertyIndex::ListViewBase_HeaderTransitions:
            case KnownPropertyIndex::ListViewBase_FooterTransitions:
            case KnownPropertyIndex::Popup_ChildTransitions:
            case KnownPropertyIndex::UIElement_Transitions:
            {
                return true;
            }

            default:
            {
                return false;
            }
        }
    }

    METHODPFN           GetPropertyMethod() const;
    UINT16              GetOffset() const;
    UINT16              GetGroupOffset() const;
    CREATEGROUPPFN      GetGroupCreator() const;
    RENDERCHANGEDPFN    GetRenderChangedHandler() const;

    bool IsAssignable(_In_ const CValue& value) const;

    bool AllowsObjects() const
    {
        ValueType type = GetStorageType();
        return (type == valueObject || type == valueAny);
    }

    bool ShouldPreserveObjectIdentity() const
    {
        switch (m_nIndex)
        {
            case KnownPropertyIndex::RelativePanel_LeftOf:
            case KnownPropertyIndex::RelativePanel_Above:
            case KnownPropertyIndex::RelativePanel_RightOf:
            case KnownPropertyIndex::RelativePanel_Below:
            case KnownPropertyIndex::RelativePanel_AlignLeftWith:
            case KnownPropertyIndex::RelativePanel_AlignTopWith:
            case KnownPropertyIndex::RelativePanel_AlignRightWith:
            case KnownPropertyIndex::RelativePanel_AlignBottomWith:
            case KnownPropertyIndex::RelativePanel_AlignHorizontalCenterWith:
            case KnownPropertyIndex::RelativePanel_AlignVerticalCenterWith:
                return false;

            default:
                return true;
        }
    }

    ValueType GetStorageType() const
    {
        switch (GetPropertyTypeIndex())
        {
            case KnownTypeIndex::String:
            case KnownTypeIndex::Uri:
                return valueString;

            case KnownTypeIndex::Int32:
                return valueSigned;

            case KnownTypeIndex::Double:
                if (StoreDoubleAsFloat())
                {
                    return valueFloat;
                }
                return valueDouble;

            case KnownTypeIndex::Float:
                return valueFloat;

            case KnownTypeIndex::Boolean:
                return valueBool;

            case KnownTypeIndex::Color:
                return valueColor;

            case KnownTypeIndex::Thickness:
                return valueThickness;

            case KnownTypeIndex::Point:
                return valuePoint;

            case KnownTypeIndex::Rect:
                return valueRect;

            case KnownTypeIndex::Size:
                return valueSize;

            case KnownTypeIndex::CornerRadius:
                return valueCornerRadius;

            case KnownTypeIndex::GridLength:
                return valueGridLength;

            case KnownTypeIndex::DateTime:
                return valueDateTime;

            case KnownTypeIndex::FontWeight:
                return valueEnum;

            case KnownTypeIndex::TypeName:
                return valueTypeHandle;

            case KnownTypeIndex::Object:
                return valueAny;

            case KnownTypeIndex::Duration:
            case KnownTypeIndex::RepeatBehavior:
            case KnownTypeIndex::KeyTime:
                return valueVO;

            default:
                if (GetPropertyType()->IsEnum())
                {
                    if (GetPropertyType()->IsCompactEnum())
                    {
                        return valueEnum8;
                    }
                    else
                    {
                        return valueEnum;
                    }
                }

                // Anything else is expected to be a DO (valueObject should really be called valueDependencyObject).
                return valueObject;
        }
    }

    _Check_return_ HRESULT GetDefaultValue(
        _In_ CCoreServices* core,
        _In_opt_ CDependencyObject* referenceObject,
        _In_ const CClassInfo* type,
        _Out_ CValue* defaultValue) const;

    _Check_return_ HRESULT CreateDefaultValueObject(
        _In_ CCoreServices* core,
        _Out_ CValue* defaultValue) const;

    _Check_return_ HRESULT CreateDefaultVO(
        _In_ CCoreServices* core,
        _Out_ CValue* defaultValue) const;

    _Check_return_ HRESULT GetDefaultInheritedPropertyValue(
        _In_ CCoreServices* core,
        _Out_ CValue* defaultValue) const;

    _Check_return_ HRESULT GetDefaultValueFromPeer(
        _In_opt_ CDependencyObject* referenceObject,
        _Out_ CValue* defaultValue) const;

    _Check_return_ HRESULT ValidateType(
        _In_ const CClassInfo* type) const;

    static _Check_return_ HRESULT GetBooleanThemeResourceValue(
        _In_ CCoreServices* core,
        _In_ const xstring_ptr_view& key,
        _Out_ bool* value,
        _Out_opt_ bool* resourceExists = nullptr);

    // Returns the default FocusVisualSecondaryBrush or FocusVisualSecondaryBrush brush for the provided targetObject
    // FrameworkElement, according to the forFocusVisualSecondaryBrush flag.
    // There are two successive fallback mechanisms in case the SystemControlFocusVisualSecondary/PrimaryBrush brush
    // cannot be found in the current resources dictionary: First the SystemAltMediumColor/SystemBaseHighColor
    // resource is retrieved respectively. In the unexpected case that cannot be accessed either, the default
    // black/white color is used. All those colors are flipped as needed in light themes to maintain a good
    // contrast.
    static _Check_return_ HRESULT GetDefaultFocusVisualBrush(
        _In_ FocusVisualType forFocusVisualType,
        _In_ CCoreServices* core,
        _In_opt_ CDependencyObject* targetObject,
        _Outptr_ CDependencyObject** ppBrush);

    static _Check_return_ HRESULT GetDefaultFontIconFontFamily(
        _In_ CCoreServices* core,
        _Outptr_ CDependencyObject** fontFamily);

    static _Check_return_ HRESULT GetDefaultTextControlContextFlyout(
        _In_ CCoreServices* core,
        _Outptr_ CDependencyObject** flyout);

    static _Check_return_ HRESULT GetDefaultTextControlSelectionFlyout(
        _In_ CCoreServices* core,
        _Outptr_ CDependencyObject** flyout);

protected:
    template <typename... Args>
    CDependencyProperty(Args&&... args)
        : CPropertyBase(std::forward<Args>(args)...)
    {}

private:
    // Returns a brush picked in this order:
    // 1. Based on the provided targetObject element and provided brush resource key in its current theme dictionary.
    // 2. Based on the provided targetObject element and provided color resource key in its current theme dictionary.
    // 3. Based on the provided fallback color.
    struct FocusVisualResourceData
    {
        xstring_ptr BrushThemeKey;
        xstring_ptr ColorThemeKey;
        uint32_t FallbackColor;
    };

    static _Check_return_ HRESULT GetDefaultFocusVisualBrush(
        _In_ FocusVisualType forFocusVisualType,
        _In_ CCoreServices* core,
        _In_opt_ CDependencyObject* targetObject,
        _In_ const FocusVisualResourceData& strBrushThemeKey,
        _Outptr_ CDependencyObject** ppBrush);

    static FocusVisualResourceData GetFocusVisualResourceData(
        _In_ FocusVisualType forFocusVisualType,
        _In_opt_ CFrameworkElement* targetObject);

#if defined(__XAML_UNITTESTS__)
public:
    CDependencyProperty() = default;
#endif
};

class CEnterDependencyProperty : public MetaDataEnterPropertyNonAggregate
{
    bool IsSet(MetaDataEnterPropertyInfoFlags flag) const
    {
        return IsFlagSetHelper(m_flags, flag);
    }

public:
    const CEnterDependencyProperty* GetNextProperty() const;

    bool DoNotEnterLeave() const    { return IsSet(MetaDataEnterPropertyInfoFlags::DoNotEnterLeave); }
    bool IsObjectProperty() const   { return IsSet(MetaDataEnterPropertyInfoFlags::IsObjectProperty); }
    bool NeedsInvoke() const        { return IsSet(MetaDataEnterPropertyInfoFlags::NeedsInvoke); }
};

class CObjectDependencyProperty : public MetaDataObjectPropertyNonAggregate
{
public:
    const CObjectDependencyProperty* GetNextProperty() const;
};

class CRenderDependencyProperty : public MetaDataRenderPropertyNonAggregate
{
public:
    const CRenderDependencyProperty* GetNextProperty() const;
};

class CSimpleProperty : public CPropertyBase
{
    // TODO: for now this is a placeholder for a class representing non-dp properties.
};

struct TypeCheckData
{
    constexpr TypeCheckData(
        UINT64 handleMask,
        UINT64 handle)
        : m_handleMask(handleMask)
        , m_handle(handle)
    {}

    constexpr UINT64 GetHandleMask() const
    {
        return m_handleMask;
    }

    constexpr UINT64 GetHandle() const
    {
        return m_handle;
    }

    constexpr bool IsHandleValid() const
    {
        return m_handle != 0;
    }

    constexpr bool IsNotLeaf() const
    {
        return (m_handleMask & c_isLeafHandleMask) == 0;
    }

private:
    static constexpr UINT64 c_isLeafHandleMask = 0x800000000000;

    UINT64 m_handleMask;
    UINT64 m_handle;
};
