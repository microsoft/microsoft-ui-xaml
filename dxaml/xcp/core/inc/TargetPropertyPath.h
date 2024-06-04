// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CPropertyPath.h>

class CCoreServices;
class CDependencyObject;

class CTargetPropertyPath final
    : public CDependencyObject
{
protected:
    CTargetPropertyPath(CCoreServices *pCore)
        : CDependencyObject(pCore)
    {
    }

public:
    DECLARE_CREATE_WITH_TYPECONVERTER(CTargetPropertyPath);

    _Check_return_ HRESULT FromString(_In_ CREATEPARAMETERS *pCreate);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTargetPropertyPath>::Index;
    }

    static _Check_return_ HRESULT Target(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _In_reads_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Inout_ CValue *pResult);

    static _Check_return_ HRESULT Path(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _In_reads_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Inout_ CValue *pResult);

    // Resolves the target object, target property, and the object that actually owns the target property.
    // Because the resolution is done every time this method is called, if the TargetPropertyPath
    // was created from a string the namescope doesn't actually have to contain an object with the
    // target name at TargetPropertyPath creation time.
    // If the namescopeReference is null, then 'this' will be used.
    _Check_return_ HRESULT ResolveTargetForVisualStateSetter(
        _In_opt_ CDependencyObject* namescopeReference,
        _Out_ xref_ptr<CDependencyObject>& targetObject,
        _Out_ xref_ptr<CDependencyObject>& targetPropertyOwner,
        _Outptr_result_maybenull_ const CDependencyProperty** targetProperty);

    // Resolve the target property for the provided KnownTypeIndex. This is used for the case
    // where the TargetPropertyPath belongs to a Style Setter. The result is cached and returned
    // for subsequent calls, since Style semantics guarantee that nothing about the target
    // can change.
    _Check_return_ HRESULT ResolveTargetForStyleSetter(
        _In_ KnownTypeIndex typeIndex,
        _Out_ KnownPropertyIndex* targetPropertyIndex);

    _Check_return_ HRESULT GenericResolveTargetProperty(_Outptr_ const CDependencyProperty** targetProperty);

    // Returns whether or not the TargetPropertyPath is being used by a Style setter
    // Currently just a passthrough to IsTargetNull() since that's how the semantics
    // are currently defined, but this could conceivably change in the future
    bool IsForStyleSetter() const;

private:
    _Check_return_ HRESULT ResolveTargetObjectForVisualStateSetter(
        _In_opt_ CDependencyObject* namescopeReference,
        _Out_ xref_ptr<CDependencyObject>& targetObject);

    _Check_return_ HRESULT ResolveTargetPropertyForVisualStateSetter(
        _In_ const xref_ptr<CDependencyObject> targetObject,
        _Out_ xref_ptr<CDependencyObject>& targetPropertyOwner,
        _Outptr_ const CDependencyProperty** targetProperty);

    _Check_return_ HRESULT CreatePropertyPathObject();

    _Check_return_ HRESULT GetCachedTargetPropertyIndex(_Out_ KnownPropertyIndex& cachedTargetPropertyIndex) const;

    _Check_return_ HRESULT ResolveFullyQualifiedPropertyNames(const xstring_ptr& path, XamlServiceProviderContext* serviceProviderContext);

    bool IsTargetNull() const;
    bool IsPathFullyQualifiedPropertyName(const xstring_ptr& path, unsigned int firstDotIndex = 0) const;

    xref::weakref_ptr<CDependencyObject> m_manualTargetWeakRef;
    xstring_ptr m_targetName;
    xref_ptr<CPropertyPath> m_path;
    xstring_ptr m_pathString;
    std::map<xstring_ptr, const CDependencyProperty*> m_partialPathTypes;
};
