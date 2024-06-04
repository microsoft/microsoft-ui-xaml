// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <cvalue.h>
#include <vector>
#include <CDependencyObject.h>
#include <Setter.h>

class CCoreServices;
class CDependencyObject;
class CDependencyProperty;
class CSetter;

// Helpers useful for implementing VisualState.Setters
namespace VisualStateSetterHelper
{
    enum class SetterOperation
    {
        Set,
        Unset,
    };

    // Little helper class to store the important data needed to apply a VisualState Setter
    // without having to store the entire Setter.
    class ResolvedVisualStateSetter
    {
    public:
        ResolvedVisualStateSetter(
            xref::weakref_ptr<CDependencyObject> targetObject,
            const CDependencyProperty* targetProperty,
            CValue value)
            : m_targetObject(targetObject)
            , m_targetProperty(targetProperty)
            , m_value(value)
        {
        }
        
        ResolvedVisualStateSetter(
            xref::weakref_ptr<CDependencyObject> targetObject,
            const CDependencyProperty* targetProperty,
            CValue value,
            xref_ptr<CDependencyObject> setter)
            : m_targetObject(targetObject)
            , m_targetProperty(targetProperty)
            , m_value(value)
            , m_originalSetter(setter)
        {
        }
        
        ResolvedVisualStateSetter(const ResolvedVisualStateSetter&) = default;
        ResolvedVisualStateSetter(ResolvedVisualStateSetter&&) noexcept = default;

        // move assignment operator
        ResolvedVisualStateSetter& operator=(ResolvedVisualStateSetter&&) = default;

        // copy assignment operator
        ResolvedVisualStateSetter& operator=(const ResolvedVisualStateSetter &other)
        {
            if (this != &other)
            {
                m_targetObject = other.m_targetObject;
                m_targetProperty = other.m_targetProperty;
                VERIFYHR(m_value.CopyConverted(other.m_value));
            }

            return (*this);
        }

        const xref::weakref_ptr<CDependencyObject> get_TargetObject() const
        {
            return m_targetObject;
        }

        const CDependencyProperty* get_TargetProperty() const
        {
            return m_targetProperty;
        }

        const CValue get_Value() const
        {
            return m_value;
        }

        const xref_ptr<CDependencyObject> get_OriginalSetter() const
        {
            return m_originalSetter;
        }

    private:
        xref::weakref_ptr<CDependencyObject> m_targetObject;
        const CDependencyProperty* m_targetProperty;
        CValue m_value;
        xref_ptr<CDependencyObject> m_originalSetter; //Only used when XamlDiagnostics is enabled to keep a strong reference to the original setter for source info purposes
    };

    typedef std::vector<ResolvedVisualStateSetter> ResolvedVisualStateSetterCollection;

    _Check_return_ HRESULT ResolveSetterAnimationTargets(
        _In_ CSetter* setter,
        _Out_ xref_ptr<CDependencyObject>& modifiedObject,
        _Outptr_ const CDependencyProperty** modifiedProperty,
        _Out_ CValue& value);

    // Performs the specified operation (set/unset) given the provided target object,
    // target property, and desired value. Set operations should not provide an unset value;
    // for unset operations, the provided value is mostly ignored. By default, this follows 
    // general animation semantics, i.e. the [Set,Clear]AnimatedValue is performed synchronously
    // if it is a built-in property, otherwise it is deferred to the next tick. For some
    // scenarios (e.g. refreshing the applied value after a theme change notification)
    // it is necessary to force all operations to be deferred to the next tick.
    _Check_return_ HRESULT PerformAnimatedValueOperation(
        _In_ SetterOperation operation,
        _In_ xref_ptr<CDependencyObject> targetObject,
        _In_ const CDependencyProperty* targetProperty,
        _In_ const CValue value,
        _In_ xref_ptr<CDependencyObject> originalSetter = nullptr, //Used by XamlDiagnostics for source info
        _In_ const bool forceDeferOperation = false);

    // Performs the operation for the setter (eventually calls through to PerformAnimatedValueOperation), but
    // only if the visual state is currently active. This is for operations like adding/removing setters from 
    // the currently active visual state.
    void PerformSetterOperationIfStateActive(
        _In_ CVisualState* visualState,
        _In_ CSetter* setter, 
        _In_ VisualStateSetterHelper::SetterOperation operation);

    _Check_return_ CVisualState* GetVisualStateSetterVisualState(_In_ CSetter* setter);
};
