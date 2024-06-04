// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "MetadataAPI.h"
#include "XamlProperty.h"
#include "XamlQualifiedObject.h"
#include "XamlSchemaContext.h"
#include "XamlType.h"

_Check_return_ HRESULT
CTargetPropertyPath::FromString(_In_ CREATEPARAMETERS *pCreate)
{
    xstring_ptr fullPath = pCreate->m_value.AsString();

    // Check if this is being used inside a Style or VisualState; permitted semantics are somewhat different
    std::shared_ptr<XamlType> xamlType;
    std::shared_ptr<XamlServiceProviderContext> serviceProviderContext = pCreate->m_spServiceProviderContext;
    {
        std::shared_ptr<XamlSchemaContext> schemaContext;
        IFC_RETURN(serviceProviderContext->GetSchemaContext(schemaContext));

        // Find Style.TargetType if it exists
        std::shared_ptr<XamlQualifiedObject> targetTypeValue;
        {
            std::shared_ptr<XamlType> styleXamlType;
            std::shared_ptr<XamlType> visualStateGroupCollectionXamlType;
            std::shared_ptr<XamlProperty> styleTargetTypeXamlProperty;
            IFC_RETURN(schemaContext->GetXamlType(
                XamlTypeToken(tpkNative, KnownTypeIndex::Style),
                styleXamlType));
            IFC_RETURN(schemaContext->GetXamlType(
                XamlTypeToken(tpkNative, KnownTypeIndex::VisualStateGroupCollection),
                visualStateGroupCollectionXamlType));
            IFC_RETURN(schemaContext->GetXamlProperty(
                XamlPropertyToken(tpkNative, KnownPropertyIndex::Style_TargetType),
                XamlTypeToken(tpkNative, KnownTypeIndex::TypeName),
                styleTargetTypeXamlProperty));

            IFC_RETURN(serviceProviderContext->GetAmbientValue(
                styleXamlType /* stop search if this type is encountered */,
                visualStateGroupCollectionXamlType /* stop search if this type is encountered */,
                styleTargetTypeXamlProperty /* return value of this property if it is encountered */,
                std::shared_ptr<XamlProperty>(),
                std::shared_ptr<XamlType>(),
                CompressedStackCacheHint::None,
                targetTypeValue));
        }
        if (targetTypeValue)
        {
            // An ambient Style.TargetType value indicates we're inside a Style
            KnownTypeIndex styleTargetTypeIndex = KnownTypeIndex::UnknownType;

            if (targetTypeValue->GetCValuePtr()->GetType() == valueObject)
            {
                CType* type = nullptr;
                IFC_RETURN(DoPointerCast<CType>(type, targetTypeValue->GetValue()));
                if (type)
                {
                    styleTargetTypeIndex = type->GetReferencedTypeIndex();
                }
            }
            else if (targetTypeValue->GetCValuePtr()->GetType() == valueTypeHandle)
            {
                styleTargetTypeIndex = targetTypeValue->GetCValuePtr()->AsTypeHandle();
            }

            if (styleTargetTypeIndex != KnownTypeIndex::UnknownType)
            {
                XamlTypeToken typeToken = XamlTypeToken::FromType(DirectUI::MetadataAPI::GetClassInfoByIndex(styleTargetTypeIndex));
                IFC_RETURN(schemaContext->GetXamlType(typeToken, xamlType));
            }
        }
    }

    bool isForStyleSetter = (xamlType != nullptr);
    xstring_ptr targetName;
    xstring_ptr pathString;
    {
        const unsigned int firstDotIndex = fullPath.FindChar(L'.');
        const bool hasDot = firstDotIndex != xstring_ptr_view::npos;

        if (isForStyleSetter)
        {
            xstring_ptr typeName;
            xstring_ptr propertyName;
            bool resolvedPropertyName = false;
            bool pathIsFullyQualifiedPropertyName = IsPathFullyQualifiedPropertyName(fullPath, firstDotIndex);

            // For a Style setter, there is no element name so entire string is the property path
            pathString = fullPath;
            if (pathIsFullyQualifiedPropertyName)
            {
                // pathIsFullyQualifiedPropertyName => hasDot
                ASSERT(hasDot);

                // Enclosed in parentheses and only one dot indicates direct (fully qualified) property
                // syntax, i.e. no element name; used by Style setter
                // We'll therefore use the first part of the specified path as the declaring type name, rather
                // than the Style's TargetType
                IFC_RETURN(fullPath.SubString(1, firstDotIndex, &typeName));
                IFC_RETURN(fullPath.SubString(firstDotIndex + 1, fullPath.GetCount() - 1, &propertyName));

                IFC_RETURN(serviceProviderContext->ResolveXamlType(typeName, xamlType));
            }
            else
            {
                if (hasDot)
                {
                    // TargetPropertyPath used by a Style Setter cannot have an explicit Target object
                    IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_STYLE_SETTER_EXPLICIT_TARGET_OBJECT));
                }

                // No dot, this indicates direct property syntax, i.e. no element name; used by Style setter
                propertyName = fullPath;
            }

            std::shared_ptr<XamlProperty> xamlProperty;
            if (xamlType && SUCCEEDED(xamlType->GetDependencyProperty(propertyName, xamlProperty)))
            {
                xref_ptr<CDependencyPropertyProxy> spResolvedProperty;
                IFC_RETURN(spResolvedProperty.init(new CDependencyPropertyProxy(GetContext())));
                IFC_RETURN(spResolvedProperty->FromXamlProperty(xamlProperty));

                CValue propAsValue;
                propAsValue.SetObjectNoRef(spResolvedProperty.detach());
                IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::TargetPropertyPath_CachedStyleSetterProperty, propAsValue));

                resolvedPropertyName = true;
            }

            if (!resolvedPropertyName)
            {
                // Report an error like "The property path %0 could not be resolved."
                xephemeral_string_ptr parameters[1];
                pathString.Demote(&parameters[0]);

                IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_TARGETPROPERTYPATH_CANT_RESOLVE_PATH, 1, parameters));
            }
        }
        else
        {
            // Suppose the input path is 'foo.bar.fizz.buzz'
            // The following statements are true:
            // 1) There is an element with the x:Name 'foo'
            // 2) Element 'foo' has a property named 'bar'
            // 3) The type of the value of 'foo.bar' has a property 'fizz'
            // 4) The type of the value of 'foo.bar.fizz' has a *dependency* property 'buzz'
            //
            // If any of these statements is false, then the path is incorrect.
            if (hasDot)
            {
                // For VSM Setter, part before first dot is element name, after first dot is property path
                // SubString() does not include the character at 'endIndex'
                IFC_RETURN(fullPath.SubString(0, firstDotIndex, &targetName));
                IFC_RETURN(fullPath.SubString(firstDotIndex + 1, fullPath.GetCount(), &pathString));
            }

            if (!pathString.IsNullOrEmpty())
            {
                // Resolve the Fully Qualified Properties while the XamlServiceProviderContext is available, this will store the resolved DPs by key and
                // when the full path is parsed we will pass the already resolved types.
                IFC_RETURN(ResolveFullyQualifiedPropertyNames(pathString, serviceProviderContext.get()));
            }
        }
    }

    if (!targetName.IsNullOrEmpty())
    {
        m_targetName = targetName;
        IFC_RETURN(SetPropertyIsLocal(GetPropertyByIndexInline(KnownPropertyIndex::TargetPropertyPath_Target)));
    }
    else if (!isForStyleSetter)
    {
        IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_TARGETPROPERTYPATH_REQUIRES_TARGET));
    }

    if (!pathString.IsNullOrEmpty())
    {
        IFC_RETURN(SetPropertyIsLocal(GetPropertyByIndexInline(KnownPropertyIndex::TargetPropertyPath_Path)));
        m_pathString = pathString;
    }
    else
    {
        // Property path must have a value
        IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_TARGETPROPERTYPATH_REQUIRES_PATH));
    }

    return S_OK;
}

_Check_return_ HRESULT
CTargetPropertyPath::Target(
    _In_ CDependencyObject* pObject,
    _In_ XUINT32 cArgs,
    _In_reads_(cArgs) CValue* pArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Inout_ CValue* pResult)
{
    CTargetPropertyPath* pThis = do_pointer_cast<CTargetPropertyPath>(pObject);

    if (cArgs == 0)
    {
        // GetValue
        if (!pThis->IsTargetNull())
        {
            xref_ptr<CDependencyObject> targetObject;
            IFC_RETURN(pThis->ResolveTargetObjectForVisualStateSetter(pThis, targetObject));

            pResult->SetObjectNoRef(targetObject.detach());
        }
        else
        {
            pResult->SetNull();
        }
    }
    else
    {
        // SetValue
        IFC_RETURN((pArgs->GetType() == valueObject || pArgs->GetType() == valueNull) ? S_OK : E_INVALIDARG);

        if (pArgs->AsObject())
        {
            pThis->m_manualTargetWeakRef = xref::get_weakref(pArgs->AsObject());
            IFC_RETURN(pThis->SetPropertyIsLocal(pThis->GetPropertyByIndexInline(KnownPropertyIndex::TargetPropertyPath_Target)));
        }
        else
        {
            pThis->m_manualTargetWeakRef.reset();
            pThis->SetPropertyIsDefault(pThis->GetPropertyByIndexInline(KnownPropertyIndex::TargetPropertyPath_Target));
        }
        IFC_RETURN(pThis->ClearValueByIndex(KnownPropertyIndex::TargetPropertyPath_CachedStyleSetterProperty));
    }

    return S_OK;
}

_Check_return_ HRESULT
CTargetPropertyPath::Path(
    _In_ CDependencyObject* pObject,
    _In_ XUINT32 cArgs,
    _In_reads_(cArgs) CValue* pArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Inout_ CValue* pResult)
{
    CTargetPropertyPath* pThis = do_pointer_cast<CTargetPropertyPath>(pObject);

    if (cArgs == 0)
    {
        // GetValue
        IFC_RETURN(pThis->CreatePropertyPathObject());
        pResult->SetObjectAddRef(pThis->m_path.get());
    }
    else
    {
        // SetValue
        IFC_RETURN((pArgs->GetType() == valueObject || pArgs->GetType() == valueNull) ? S_OK : E_INVALIDARG);

        if (pArgs->AsObject())
        {
            CPropertyPath* newPropertyPath = nullptr;
            newPropertyPath = do_pointer_cast<CPropertyPath>(pArgs->AsObject());
            IFC_RETURN(newPropertyPath ? S_OK : E_INVALIDARG);
            pThis->m_path = newPropertyPath;
            IFC_RETURN(pThis->SetPropertyIsLocal(pThis->GetPropertyByIndexInline(KnownPropertyIndex::TargetPropertyPath_Path)));
            pThis->m_pathString = xstring_ptr::EmptyString();
        }
        else
        {
            pThis->m_path = nullptr;
            pThis->SetPropertyIsDefault(pThis->GetPropertyByIndexInline(KnownPropertyIndex::TargetPropertyPath_Path));
            pThis->m_pathString = xstring_ptr::EmptyString();
        }
        IFC_RETURN(pThis->ClearValueByIndex(KnownPropertyIndex::TargetPropertyPath_CachedStyleSetterProperty));
    }

    return S_OK;
}

_Check_return_ HRESULT
CTargetPropertyPath::ResolveTargetForVisualStateSetter(
    _In_opt_ CDependencyObject* namescopeReference,
    _Out_ xref_ptr<CDependencyObject>& targetObject,
    _Out_ xref_ptr<CDependencyObject>& targetPropertyOwner,
    _Outptr_result_maybenull_ const CDependencyProperty** targetProperty)
{
    targetPropertyOwner = nullptr;
    *targetProperty = nullptr;

    IFC_RETURN(ResolveTargetObjectForVisualStateSetter(namescopeReference, targetObject));

    if (targetObject)
    {
        // Resolve the target property using the target object we just resolved as the starting point
        IFC_RETURN(ResolveTargetPropertyForVisualStateSetter(targetObject, targetPropertyOwner, targetProperty));
    }

    return S_OK;
}

_Check_return_ HRESULT
CTargetPropertyPath::ResolveTargetForStyleSetter(
    _In_ KnownTypeIndex typeIndex,
    _Out_ KnownPropertyIndex* targetPropertyIndex)
{
    if (m_manualTargetWeakRef.lock() || !m_targetName.IsNullOrEmpty())
    {
        // TargetPropertyPath used by a Style Setter cannot have an explicit Target object
        IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_STYLE_SETTER_EXPLICIT_TARGET_OBJECT));
    }

    // Look up the cached setter property index
    KnownPropertyIndex cachedTargetPropertyIndex;
    IFC_RETURN(GetCachedTargetPropertyIndex(cachedTargetPropertyIndex));

    if (cachedTargetPropertyIndex == KnownPropertyIndex::UnknownType_UnknownProperty)
    {
        const CDependencyProperty* pDP = nullptr;
        if (IsPathFullyQualifiedPropertyName(m_pathString))
        {
            xstring_ptr fullyQualifiedPropertyName;
            IFC_RETURN(m_pathString.SubString(1, m_pathString.GetCount() - 1, &fullyQualifiedPropertyName));
            IGNOREHR(DirectUI::MetadataAPI::TryGetDependencyPropertyByFullyQualifiedName(fullyQualifiedPropertyName, nullptr, &pDP));
        }
        else
        {
            IFC_RETURN(DirectUI::MetadataAPI::TryGetDependencyPropertyByName(DirectUI::MetadataAPI::GetClassInfoByIndex(typeIndex), m_pathString, &pDP));
        }

        if (pDP)
        {
            cachedTargetPropertyIndex = pDP->GetIndex();

            xref_ptr<CDependencyPropertyProxy> spResolvedProperty = make_xref<CDependencyPropertyProxy>(GetContext());
            spResolvedProperty->SetDP(pDP);

            CValue propAsValue;
            propAsValue.SetObjectNoRef(spResolvedProperty.detach());
            IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::TargetPropertyPath_CachedStyleSetterProperty, propAsValue));
        }
        else
        {
            IFC_RETURN(E_FAIL);
        }
    }

    *targetPropertyIndex = cachedTargetPropertyIndex;

    return S_OK;
}

_Check_return_ HRESULT
CTargetPropertyPath::GenericResolveTargetProperty(_Outptr_ const CDependencyProperty** targetProperty)
{
    *targetProperty = nullptr;

    if (IsTargetNull())
    {
        KnownPropertyIndex propertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty;
        IFC_RETURN(ResolveTargetForStyleSetter(KnownTypeIndex::UnknownType, &propertyIndex));
        if (propertyIndex != KnownPropertyIndex::UnknownType_UnknownProperty)
        {
            *targetProperty = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(propertyIndex);
        }
    }
    else
    {
        xref_ptr<CDependencyObject> targetObject;
        xref_ptr<CDependencyObject> targetPropertyOwner;
        IFC_RETURN(ResolveTargetForVisualStateSetter(GetParent(), targetObject, targetPropertyOwner, targetProperty));
    }

    return S_OK;
}

_Check_return_ HRESULT CTargetPropertyPath::ResolveTargetObjectForVisualStateSetter(
    _In_opt_ CDependencyObject* namescopeReference,
    _Out_ xref_ptr<CDependencyObject>& targetObject)
{
    // First check for manually set Target object
    targetObject = m_manualTargetWeakRef.lock();

    if (targetObject)
    {
        return S_OK;
    }
    else if (!m_targetName.IsNullOrEmpty())
    {
        // Resolve the target object by name
        targetObject = GetContext()->TryGetElementByName(
            m_targetName, (namescopeReference) ? namescopeReference : this);
        if (!targetObject)
        {
            // TODO: Debugger check is (hopefully) a temporary measure that will be removed at a later date
            if (IsDebuggerPresent())
            {
                // Report an error like "The target object with name '%0' could not be resolved."
                xephemeral_string_ptr parameters[1];
                m_targetName.Demote(&parameters[0]);

                IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_TARGETPROPERTYPATH_CANT_RESOLVE_TARGET, 1, parameters));
            }
            else
            {
                IFC_RETURN(E_FAIL);
            }
        }
    }
    else
    {
        IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_TARGETPROPERTYPATH_REQUIRES_TARGET));
    }

    return S_OK;
}


_Check_return_ HRESULT CTargetPropertyPath::ResolveTargetPropertyForVisualStateSetter(
    _In_ const xref_ptr<CDependencyObject> targetObject,
    _Out_ xref_ptr<CDependencyObject>& targetPropertyOwner,
    _Outptr_ const CDependencyProperty** targetProperty)
{
    CDependencyObject* owner = targetObject.get();
    xstring_ptr pathString = (m_path) ? m_path->m_strPath : m_pathString;

    HRESULT hr = GetContext()->ParsePropertyPath(
        &owner,
        targetProperty,
        pathString,
        m_partialPathTypes);
    if (FAILED(hr))
    {
        // TODO: Debugger check is (hopefully) a temporary measure that will be removed at a later date
        if (IsDebuggerPresent())
        {
            // Report an error like "The property path '%0' could not be resolved."
            xephemeral_string_ptr parameters[1];
            pathString.Demote(&parameters[0]);

            IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_TARGETPROPERTYPATH_CANT_RESOLVE_PATH, 1, parameters));
        }
        else
        {
            IFC_RETURN(hr);
        }
    }

    targetPropertyOwner = owner;

    return S_OK;
}

_Check_return_ HRESULT CTargetPropertyPath::CreatePropertyPathObject()
{
    if (!m_path)
    {
        xstring_ptr pathString = m_pathString;

        if (pathString.IsNullOrEmpty())
        {
            KnownPropertyIndex cachedTargetPropertyIndex;
            IFC_RETURN(GetCachedTargetPropertyIndex(cachedTargetPropertyIndex));
            if (cachedTargetPropertyIndex != KnownPropertyIndex::UnknownType_UnknownProperty)
            {
                const CDependencyProperty* cachedProperty = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(cachedTargetPropertyIndex);
                xstring_ptr propertyName = cachedProperty->GetName();

                if (cachedProperty->IsAttached())
                {
                    // Names of attached properties need to be enclosed in parentheses
                    // and have the declaring type's name e.g. "(Canvas.Left)"
                    xstring_ptr declaringTypeName = cachedProperty->GetDeclaringType()->GetName();
                    XStringBuilder stringBuilder;
                    IFC_RETURN(stringBuilder.Initialize(propertyName.GetCount() + 2));
                    IFC_RETURN(stringBuilder.AppendChar(L'('));
                    IFC_RETURN(stringBuilder.Append(declaringTypeName));
                    IFC_RETURN(stringBuilder.AppendChar(L'.'));
                    IFC_RETURN(stringBuilder.Append(propertyName));
                    IFC_RETURN(stringBuilder.AppendChar(L')'));

                    IFC_RETURN(stringBuilder.DetachString(&pathString));
                }
                else
                {
                    pathString = propertyName;
                }
            }
        }

        if (!pathString.IsNullOrEmpty())
        {
            // Create the target property path
            CREATEPARAMETERS cp(GetContext(), pathString);
            IFC_RETURN(CreateDO(m_path.ReleaseAndGetAddressOf(), &cp));
        }
    }
    return S_OK;
}

bool CTargetPropertyPath::IsForStyleSetter() const
{
    return IsTargetNull();
}

_Check_return_ HRESULT CTargetPropertyPath::GetCachedTargetPropertyIndex(_Out_ KnownPropertyIndex& cachedTargetPropertyIndex) const
{
    // Look up the cached setter property index
    cachedTargetPropertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty;

    CValue propertyAsCValue;
    IFC_RETURN(GetValueByIndex(KnownPropertyIndex::TargetPropertyPath_CachedStyleSetterProperty, &propertyAsCValue));
    if (CDependencyPropertyProxy* setterPropertyProxy = do_pointer_cast<CDependencyPropertyProxy>(propertyAsCValue))
    {
        cachedTargetPropertyIndex = setterPropertyProxy->GetPropertyIndex();
    }

    return S_OK;
}

bool CTargetPropertyPath::IsTargetNull() const
{
    return (!m_manualTargetWeakRef.lock() && m_targetName.IsNullOrEmpty());
}

bool CTargetPropertyPath::IsPathFullyQualifiedPropertyName(const xstring_ptr& path, unsigned int firstDotIndex) const
{
    unsigned int firstDotIndexActual = firstDotIndex;
    if (firstDotIndexActual == 0)
    {
        firstDotIndexActual = path.FindChar(L'.');
        if (firstDotIndexActual == xstring_ptr_view::npos)
        {
            return false;
        }
    }

    return (path.GetCount() >= 5 /* minimum possible name is '(a.b)' */
        && path.GetChar(0) == L'('
        && path.GetChar(path.GetCount() - 1) == L')'
        && path.FindChar(L'.', firstDotIndexActual) == xstring_ptr_view::npos);
}

HRESULT CTargetPropertyPath::ResolveFullyQualifiedPropertyNames(const xstring_ptr& path, XamlServiceProviderContext* serviceProviderContext)
{
    unsigned int startIndex = 0;
    unsigned int endIndex = 0;

    while (true)
    {
        startIndex = path.FindChar(L'(', startIndex);

        if (startIndex == xstring_ptr_view::npos)
        {
            break;
        }

        endIndex = path.FindChar(L')', startIndex);

        if (endIndex == xstring_ptr_view::npos)
        {
            break;
        }

        xstring_ptr fullyQualifiedPropertyName;
        IFC_RETURN(path.SubString(startIndex + 1, endIndex, &fullyQualifiedPropertyName));
        startIndex = endIndex + 1;

        xstring_ptr fullyQualifiedPropertyNameTrimmed;
        XUINT32 cString;
        const WCHAR* pString = fullyQualifiedPropertyName.GetBufferAndCount(&cString);

        IFC_RETURN(xstring_ptr::CloneBufferTrimWhitespace(pString, cString, &fullyQualifiedPropertyNameTrimmed));

        const CDependencyProperty* pDP = nullptr;

        // Skip if we already resolved this type.
        if ((m_partialPathTypes.find(fullyQualifiedPropertyNameTrimmed)) == m_partialPathTypes.end())
        {
            // By using the XamlServiceProviderContext this API automatically resolves any xmlns prefix present.
            IGNOREHR(DirectUI::MetadataAPI::TryGetDependencyPropertyByFullyQualifiedName(fullyQualifiedPropertyNameTrimmed, serviceProviderContext, &pDP));

            // If the DP was found store in the map for later use.
            if (pDP)
            {
                m_partialPathTypes.insert(std::pair<xstring_ptr, const CDependencyProperty*>(fullyQualifiedPropertyNameTrimmed, pDP));
            }
        }
    }

    return S_OK;
}
