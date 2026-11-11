// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Triggers.h"
#include "MetadataAPI.h"
#include "Storyboard.h"
#include "CValueUtil.h"
#include "FxCallbacks.h"
#include <Setter.h>
#include <SetterBaseCollection.h>
#include <OptimizedStyle.h>
#include <VisualStateSetterHelper.h>
#include <DOPointerCast.h>

using namespace DirectUI;

namespace
{
    // Compares a property's current value with a condition's expected value. Supports the value kinds
    // that conditional styling needs in practice: booleans, enums and numbers.
    bool ValuesMatch(_In_ const CValue& actual, _In_ const CValue& expected)
    {
        if (actual.IsNull() || expected.IsNull())
        {
            return actual.IsNull() && expected.IsNull();
        }

        if (expected.GetType() == valueBool)
        {
            return actual.GetType() == valueBool && actual.AsBool() == expected.AsBool();
        }

        if (expected.IsEnum())
        {
            return actual.IsEnum() && actual.AsEnum() == expected.AsEnum();
        }

        const auto toDouble = [](const CValue& value, double* result)
        {
            switch (value.GetType())
            {
            case valueFloat:    *result = value.AsFloat(); return true;
            case valueDouble:   *result = value.AsDouble(); return true;
            case valueSigned:   *result = value.AsSigned(); return true;
            case valueUnsigned: *result = value.AsUnsigned(); return true;
            default:            return false;
            }
        };

        double actualNumber = 0;
        double expectedNumber = 0;
        return toDouble(actual, &actualNumber) && toDouble(expected, &expectedNumber) && actualNumber == expectedNumber;
    }
}

// Evaluates the Trigger and MultiTrigger objects of one TriggerCollection while it is in the live tree.
//
// The conditions are tested against the templated parent of the element that owns the collection (or the
// element itself when it is not part of a template). When the set of active setters changes, the winner for
// every target property is recomputed from scratch: the last active setter in declaration order wins, and
// when it deactivates the next one (or the original value) shows through again.
//
// Values are applied with the same mechanism as VisualState setters, so they take precedence over local
// values and styles, and they can conflict with VisualStates or animations that set the same property.
class TriggerEvaluator
{
public:
    explicit TriggerEvaluator(_In_ CTriggerCollection* owner)
        : m_owner(owner)
    {}

    ~TriggerEvaluator()
    {
        Deactivate();
    }

    _Check_return_ HRESULT Activate()
    {
        Deactivate();

        // The collection is parented to its owning element as a non-public (property) parent.
        CDependencyObject* host = m_owner->GetParentInternal(false /* publicParentOnly */);
        if (!host)
        {
            return S_OK;
        }

        CDependencyObject* source = host->GetTemplatedParent();
        if (!source)
        {
            source = host;
        }
        m_source = xref::get_weakref(source);

        IFC_RETURN(BuildRules(source));
        if (m_rules.empty())
        {
            return S_OK;
        }

        m_active = true;

        // Any property of the source may be tested by a condition, so listen for all of them and filter.
        auto self = this;
        IFC_RETURN(FxCallbacks::DependencyObject_AddPropertyChangedHandler(
            source,
            [self](const CDependencyProperty* property) { return self->OnSourcePropertyChanged(property); },
            m_handlerToken.ReleaseAndGetAddressOf()));

        return Evaluate();
    }

    void Deactivate()
    {
        if (!m_active)
        {
            return;
        }
        m_active = false;

        if (m_handlerToken)
        {
            if (auto source = m_source.lock())
            {
                IGNOREHR(FxCallbacks::DependencyObject_RemovePropertyChangedHandler(source.get(), m_handlerToken.Get()));
            }
            m_handlerToken.Reset();
        }

        // Reveal the original values.
        for (auto& applied : m_applied)
        {
            if (auto target = applied.target.lock())
            {
                IGNOREHR(VisualStateSetterHelper::PerformAnimatedValueOperation(
                    VisualStateSetterHelper::SetterOperation::Unset, target, applied.property, CValue()));
            }
        }
        m_applied.clear();
        m_rules.clear();
    }

private:
    struct ConditionData
    {
        const CDependencyProperty* property = nullptr;
        CValue expected;
    };

    struct SetterData
    {
        xref_ptr<CSetter> setter;
        xref::weakref_ptr<CDependencyObject> target;
        const CDependencyProperty* targetProperty = nullptr;
        CValue value;
        bool resolved = false;
    };

    struct Rule
    {
        std::vector<ConditionData> conditions;
        std::vector<SetterData> setters;
    };

    struct Applied
    {
        xref::weakref_ptr<CDependencyObject> target;
        const CDependencyProperty* property = nullptr;
        const SetterData* setter = nullptr;
    };

    _Check_return_ HRESULT ResolveCondition(
        _In_ CDependencyObject* source,
        _In_ const xstring_ptr& name,
        _In_ const CValue& expectedValue,
        _Out_ ConditionData* condition)
    {
        const CDependencyProperty* property = nullptr;
        if (name.FindChar(L'.') != xstring_ptr_view::npos)
        {
            // An attached property such as RowContext.PreviousRelation.
            IFC_RETURN(MetadataAPI::TryGetAttachedPropertyByName(name, &property));
        }
        else
        {
            IFC_RETURN(MetadataAPI::TryGetDependencyPropertyByName(
                MetadataAPI::GetClassInfoByIndex(source->GetTypeIndex()), name, &property));
        }
        IFCEXPECT_RETURN(property != nullptr);

        // Convert the XAML text (for example "True" or "SameGroup") to the property's type.
        CValue expected;
        IFC_RETURN(expected.CopyConverted(expectedValue));
        bool createdObject = false;
        IFC_RETURN(OptimizedStyle::ResolveValue(source->GetContext(), property, expected, &createdObject));

        condition->property = property;
        condition->expected = std::move(expected);
        return S_OK;
    }

    _Check_return_ HRESULT AddSetters(_In_opt_ CSetterBaseCollection* setters, _Inout_ Rule* rule)
    {
        if (!setters)
        {
            return S_OK;
        }

        for (const auto& item : setters->GetCollection())
        {
            if (auto setter = do_pointer_cast<CSetter>(item))
            {
                SetterData data;
                data.setter = xref_ptr<CSetter>(setter);
                rule->setters.push_back(std::move(data));
            }
        }
        return S_OK;
    }

    _Check_return_ HRESULT BuildRules(_In_ CDependencyObject* source)
    {
        for (const auto& item : m_owner->GetCollection())
        {
            Rule rule;

            if (auto trigger = do_pointer_cast<CTrigger>(item))
            {
                ConditionData condition;
                IFC_RETURN(ResolveCondition(source, trigger->m_strProperty, trigger->m_vValue, &condition));
                rule.conditions.push_back(std::move(condition));
                IFC_RETURN(AddSetters(trigger->m_pSetters, &rule));
            }
            else if (auto multiTrigger = do_pointer_cast<CMultiTrigger>(item))
            {
                if (multiTrigger->m_pConditions)
                {
                    for (const auto& conditionItem : multiTrigger->m_pConditions->GetCollection())
                    {
                        if (auto condition = do_pointer_cast<CCondition>(conditionItem))
                        {
                            ConditionData data;
                            IFC_RETURN(ResolveCondition(source, condition->m_strProperty, condition->m_vValue, &data));
                            rule.conditions.push_back(std::move(data));
                        }
                    }
                }
                IFC_RETURN(AddSetters(multiTrigger->m_pSetters, &rule));
            }
            else
            {
                // Other kinds of triggers (EventTrigger) are not evaluated here.
                continue;
            }

            // A rule without conditions would always apply; it is not valid.
            IFCEXPECT_RETURN(!rule.conditions.empty());
            m_rules.push_back(std::move(rule));
        }
        return S_OK;
    }

    // Targets are resolved by name in the template's namescope, which may not be complete yet when the
    // collection enters the tree, so unresolved setters are retried on every evaluation.
    void ResolveSetters()
    {
        for (auto& rule : m_rules)
        {
            for (auto& data : rule.setters)
            {
                if (data.resolved)
                {
                    continue;
                }

                xref_ptr<CDependencyObject> target;
                const CDependencyProperty* targetProperty = nullptr;
                CValue value;
                if (SUCCEEDED(VisualStateSetterHelper::ResolveSetterAnimationTargets(data.setter.get(), target, &targetProperty, value))
                    && target && targetProperty)
                {
                    data.target = xref::get_weakref(target.get());
                    data.targetProperty = targetProperty;
                    data.value = std::move(value);
                    data.resolved = true;
                }
            }
        }
    }

    bool RuleIsActive(_In_ CDependencyObject* source, _In_ const Rule& rule) const
    {
        for (const auto& condition : rule.conditions)
        {
            CValue actual;
            if (FAILED(source->GetValue(condition.property, &actual)) || !ValuesMatch(actual, condition.expected))
            {
                return false;
            }
        }
        return true;
    }

    _Check_return_ HRESULT OnSourcePropertyChanged(_In_ const CDependencyProperty* property)
    {
        if (!m_active)
        {
            return S_OK;
        }

        for (const auto& rule : m_rules)
        {
            for (const auto& condition : rule.conditions)
            {
                if (condition.property->GetIndex() == property->GetIndex())
                {
                    return Evaluate();
                }
            }
        }
        return S_OK;
    }

    _Check_return_ HRESULT Evaluate()
    {
        if (!m_active)
        {
            return S_OK;
        }

        // Applying a value can change a property that a condition tests; finish this pass first.
        if (m_evaluating)
        {
            m_reevaluate = true;
            return S_OK;
        }
        m_evaluating = true;
        auto done = wil::scope_exit([this] { m_evaluating = false; });

        do
        {
            m_reevaluate = false;
            IFC_RETURN(EvaluateOnce());
        } while (m_reevaluate);

        return S_OK;
    }

    _Check_return_ HRESULT EvaluateOnce()
    {
        auto source = m_source.lock();
        if (!source)
        {
            return S_OK;
        }

        ResolveSetters();

        // The last active setter for each target property wins.
        std::vector<Applied> winners;
        for (auto& rule : m_rules)
        {
            if (!RuleIsActive(source.get(), rule))
            {
                continue;
            }

            for (auto& data : rule.setters)
            {
                if (!data.resolved)
                {
                    continue;
                }

                auto target = data.target.lock();
                auto existing = std::find_if(winners.begin(), winners.end(), [&](const Applied& winner)
                {
                    return winner.target.lock() == target && winner.property->GetIndex() == data.targetProperty->GetIndex();
                });

                if (existing != winners.end())
                {
                    existing->setter = &data;
                }
                else
                {
                    winners.push_back({ data.target, data.targetProperty, &data });
                }
            }
        }

        // Reveal values that no longer have a winner.
        for (const auto& applied : m_applied)
        {
            const bool stillWins = std::any_of(winners.begin(), winners.end(), [&](const Applied& winner)
            {
                return winner.target.lock() == applied.target.lock() && winner.property->GetIndex() == applied.property->GetIndex();
            });

            if (!stillWins)
            {
                if (auto target = applied.target.lock())
                {
                    IFC_RETURN(VisualStateSetterHelper::PerformAnimatedValueOperation(
                        VisualStateSetterHelper::SetterOperation::Unset, target, applied.property, CValue()));
                }
            }
        }

        // Apply new or changed winners.
        for (const auto& winner : winners)
        {
            const bool unchanged = std::any_of(m_applied.begin(), m_applied.end(), [&](const Applied& applied)
            {
                return applied.setter == winner.setter
                    && applied.target.lock() == winner.target.lock()
                    && applied.property->GetIndex() == winner.property->GetIndex();
            });

            if (!unchanged)
            {
                if (auto target = winner.target.lock())
                {
                    IFC_RETURN(VisualStateSetterHelper::PerformAnimatedValueOperation(
                        VisualStateSetterHelper::SetterOperation::Set, target, winner.property, winner.setter->value,
                        xref_ptr<CDependencyObject>(winner.setter->setter.get())));
                }
            }
        }

        m_applied = std::move(winners);
        return S_OK;
    }

    CTriggerCollection* m_owner;
    xref::weakref_ptr<CDependencyObject> m_source;
    std::vector<Rule> m_rules;
    std::vector<Applied> m_applied;
    Microsoft::WRL::ComPtr<IUnknown> m_handlerToken;
    bool m_active = false;
    bool m_evaluating = false;
    bool m_reevaluate = false;
};

// Defined here, where TriggerEvaluator is a complete type.
CTriggerCollection::CTriggerCollection(_In_ CCoreServices *pCore)
    : CDOCollection(pCore)
{}

CTriggerCollection::~CTriggerCollection() = default;

_Check_return_ HRESULT CTriggerCollection::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ EnterParams params)
{
    IFC_RETURN(CDOCollection::EnterImpl(pNamescopeOwner, params));


    if (params.fIsLive && GetCount() > 0)
    {
        m_evaluator = std::make_unique<TriggerEvaluator>(this);
        IFC_RETURN(m_evaluator->Activate());
    }

    return S_OK;
}

_Check_return_ HRESULT CTriggerCollection::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ LeaveParams params)
{
    if (params.fIsLive && m_evaluator)
    {
        m_evaluator->Deactivate();
        m_evaluator.reset();
    }

    IFC_RETURN(CDOCollection::LeaveImpl(pNamescopeOwner, params));
    return S_OK;
}

CBeginStoryboard::~CBeginStoryboard()
{
    ReleaseInterface(m_pStoryboard);
}

// Override SetValue to hookup storyboard to the time manager.
_Check_return_ HRESULT CBeginStoryboard::SetValue(_In_ const SetValueParams& args)
{
    // We don't want two Storyboard's from the parser, but we will allow managed
    // to overwrite the Storyboard property
    if (m_pStoryboard != NULL && !GetContext()->IsSettingValueFromManaged(this))
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFC_RETURN(CDependencyObject::SetValue(args));

    return S_OK;
}

_Check_return_ HRESULT CBeginStoryboard::InvokeImpl(_In_ const CDependencyProperty *pdp, _In_opt_ CDependencyObject *pNamescopeOwner)
{
    // The only property we need to check for is 'Storyboard' either as a content
    // or explicit property.
    //
    // If the Storyboad is entering the live tree (either because an ancestor is entering
    // or because of a SetValue()), call BeginPrivate().
    if (IsActive()
        && (pdp->GetIndex() == KnownPropertyIndex::BeginStoryboard_Storyboard)
        && (m_pStoryboard))
    {
        // Only top-level Storyboards enter the tree as part of the Triggers tag
        IFC_RETURN(m_pStoryboard->BeginPrivate( TRUE /* fIsTopLevel */ ));
    }
    else
    {
        IFC_RETURN(CDependencyObject::InvokeImpl(pdp, pNamescopeOwner));
    }

    return S_OK;
}

CEventTrigger::~CEventTrigger()
{
    ReleaseInterface(m_pTriggerActions);
}

CTrigger::~CTrigger()
{
    ReleaseInterface(m_pSetters);
}

CMultiTrigger::~CMultiTrigger()
{
    ReleaseInterface(m_pConditions);
    ReleaseInterface(m_pSetters);
}

// Override to base SetValue to do value validation for some properties.
_Check_return_ HRESULT CEventTrigger::SetValue(_In_ const SetValueParams& args)
{
    if (args.m_pDP->GetIndex() == KnownPropertyIndex::EventTrigger_RoutedEvent)
    {
        //
        // The passed value stands for a valid event name on a targe Element.
        // The value must be a string in format:  "ElementType.EventType",
        // Such as "Canvas.Loaded",  "Rectangle.Loaded" etc.
        //
        const CClassInfo *pElementClass = NULL;

        const WCHAR *pElementName = NULL;
        XUINT32     cElementName = 0;
        const WCHAR *pEventName = NULL;
        XUINT32     cEventName = 0;

        //
        // The type of RoutedEvent property in CEventTrigger is CString.
        // The accepted type of value is either one of below :
        //     1> valueString
        //     2> valueObject with STRING value in pdoValue.
        //
        xephemeral_string_ptr strStringValue;
        switch (args.m_value.GetType())
        {
            case valueString:
                // This is valid value.
                CValueUtil::GetEphemeralString(
                    args.m_value,
                    strStringValue);
                break;

            case valueObject:
            {
                const CString* pdoString = do_pointer_cast<CString>(args.m_value);

                // Valid only when its value type is String.
                if (pdoString != nullptr &&
                    !pdoString->m_strString.IsNull())
                {
                    pdoString->m_strString.Demote(&strStringValue);
                }
                else
                {
                    IFC_RETURN(E_INVALIDARG);
                }
                break;
            }

            default:
                IFC_RETURN(E_INVALIDARG);
                break;
        }

        //
        // Return E_INVALIDARG for below scenarios:
        //
        //  1. The text string doesn't contain a separator ".", (return value is XUINT32(~0)).
        //  2. Contains the "." separator at wrong position,
        //     such as at the beginning or at the end.
        //

        const auto cOffsetSeparator = strStringValue.FindChar(L'.');
        if (cOffsetSeparator == xstring_ptr_view::npos
            || cOffsetSeparator == 0 || (cOffsetSeparator == strStringValue.GetCount() -1))
        {
            IFC_RETURN(E_INVALIDARG);
        }

        //
        // There is a separator '.' inside the value string.
        // The first part will be treated as element name, the second part is the event name.
        //
        pElementName = strStringValue.GetBuffer();
        pEventName = strStringValue.GetBuffer() + cOffsetSeparator + 1;

        cElementName = cOffsetSeparator;
        cEventName = strStringValue.GetCount() - cOffsetSeparator - 1;

        pElementClass = DirectUI::MetadataAPI::GetBuiltinClassInfoByName(XSTRING_PTR_EPHEMERAL2(pElementName, cElementName));

        // if the element part is not for a valid type, returns failure here.
        if (pElementClass == NULL)
        {
            IFC_RETURN(E_INVALIDARG);
        }

        // Verify the type derives from FrameworkElement, because we currently only support the FrameworkElement.Loaded event.
        if (!DirectUI::MetadataAPI::IsAssignableFrom(
            DirectUI::MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::FrameworkElement),
            pElementClass))
        {
            IFC_RETURN(E_INVALIDARG);
        }

        //
        // For now, only Loaded event is accepted as EventTrigger.
        // When we support more events as EventTrigger, remove the below condition.
        //
        if (!XSTRING_PTR_EPHEMERAL2(pEventName, cEventName).Equals(STR_LEN_PAIR(L"Loaded")))
        {
            IFC_RETURN(E_INVALIDARG);
        }
    }

    // Just call the base class to set the value
    IFC_RETURN(CDependencyObject::SetValue(args));

    return S_OK;
}
