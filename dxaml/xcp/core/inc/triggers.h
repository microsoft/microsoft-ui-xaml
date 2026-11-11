// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CStoryboard;
class CCoreServices;

//------------------------------------------------------------------------
//
//  Class:  CTriggerCollection
//
//  Synopsis:
//      Object created for <TriggerCollection> tag
//
//------------------------------------------------------------------------

class CTriggerCollection final : public CDOCollection
{
private:
    CTriggerCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}

public:
// Creation method

    DECLARE_CREATE(CTriggerCollection);

// CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTriggerCollection>::Index;
    }

// CCollection overrides
    bool ShouldEnsureNameResolution() override { return true; }

};


//------------------------------------------------------------------------
//
//  Class:  CTriggerActionCollection
//
//  Synopsis:
//      Object created for <TriggerActionCollection> tag
//
//------------------------------------------------------------------------

class CTriggerActionCollection final : public CDOCollection
{
private:
    CTriggerActionCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}

public:
// Creation method
    DECLARE_CREATE(CTriggerActionCollection);

// CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTriggerActionCollection>::Index;
    }

// CCollection overrides
    bool ShouldEnsureNameResolution() override { return true; }

};

//------------------------------------------------------------------------
//
//  Class:  CTriggerBase
//
//  Synopsis:
//      Base class for all Trigger types.
//
//------------------------------------------------------------------------

class CTriggerBase : public CDependencyObject
{
protected:
    CTriggerBase(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

public:
// Creation method
    DECLARE_CREATE(CTriggerBase);

// CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTriggerBase>::Index;
    }
};

//------------------------------------------------------------------------
//
//  Class:  CEventTrigger
//
//  Synopsis:
//      Object created for <EventTrigger> tag
//
//------------------------------------------------------------------------

class CEventTrigger final : public CTriggerBase
{
private:
    CEventTrigger(_In_ CCoreServices *pCore)
        : CTriggerBase(pCore)
    {}

   ~CEventTrigger() override;

public:
// Creation method

    DECLARE_CREATE(CEventTrigger);

// CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CEventTrigger>::Index;
    }

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

// CCollection overrides
    virtual bool ShouldEnsureNameResolution() { return true; }


//  fields

    xstring_ptr m_strRouted;
    CTriggerActionCollection *m_pTriggerActions = nullptr;
};

//------------------------------------------------------------------------
//
//  Class:  CConditionCollection
//
//  Synopsis:
//      Object created for <ConditionCollection> tag
//
//------------------------------------------------------------------------

class CConditionCollection final : public CDOCollection
{
private:
    CConditionCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}

public:
// Creation method
    DECLARE_CREATE(CConditionCollection);

// CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CConditionCollection>::Index;
    }

// CCollection overrides
    bool ShouldEnsureNameResolution() override { return true; }
};

//------------------------------------------------------------------------
//
//  Class:  CCondition
//
//  Synopsis:
//      Object created for <Condition> tag. A property name and the value it must equal.
//
//------------------------------------------------------------------------

class CCondition final : public CDependencyObject
{
private:
    CCondition(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

public:
// Creation method
    DECLARE_CREATE(CCondition);

// CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CCondition>::Index;
    }

//  fields
    xstring_ptr m_strProperty;
    CValue m_vValue;
};

class CSetterBaseCollection;

//------------------------------------------------------------------------
//
//  Class:  CTrigger
//
//  Synopsis:
//      Object created for <Trigger> tag. Applies its setters while one property has a given value.
//
//------------------------------------------------------------------------

class CTrigger final : public CTriggerBase
{
private:
    CTrigger(_In_ CCoreServices *pCore)
        : CTriggerBase(pCore)
    {}

   ~CTrigger() override;

public:
// Creation method
    DECLARE_CREATE(CTrigger);

// CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTrigger>::Index;
    }

//  fields
    xstring_ptr m_strProperty;
    CValue m_vValue;
    CSetterBaseCollection *m_pSetters = nullptr;
};

//------------------------------------------------------------------------
//
//  Class:  CMultiTrigger
//
//  Synopsis:
//      Object created for <MultiTrigger> tag. Applies its setters while all of its conditions hold.
//
//------------------------------------------------------------------------

class CMultiTrigger final : public CTriggerBase
{
private:
    CMultiTrigger(_In_ CCoreServices *pCore)
        : CTriggerBase(pCore)
    {}

   ~CMultiTrigger() override;

public:
// Creation method
    DECLARE_CREATE(CMultiTrigger);

// CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CMultiTrigger>::Index;
    }

//  fields
    CConditionCollection *m_pConditions = nullptr;
    CSetterBaseCollection *m_pSetters = nullptr;
};

//------------------------------------------------------------------------
//
//  Class:  CTriggerAction
//
//  Synopsis:
//      Base class for all the different actions possible within a trigger.
//
//------------------------------------------------------------------------

class CTriggerAction : public CDependencyObject
{
protected:
    CTriggerAction(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

public:
// Creation method
    DECLARE_CREATE(CTriggerAction);

// CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTriggerAction>::Index;
    }
};

//------------------------------------------------------------------------
//
//  Class:  CBeginStoryboard
//
//  Synopsis:
//      Object created for <BeginStoryboard> tag
//
//------------------------------------------------------------------------

class CBeginStoryboard final : public CTriggerAction
{
private:
    CBeginStoryboard(_In_ CCoreServices *pCore)
        : CTriggerAction(pCore)
    {}

   ~CBeginStoryboard() override;

public:
// Creation method

    DECLARE_CREATE(CBeginStoryboard);

// CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CBeginStoryboard>::Index;
    }

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

protected:
    // scoping.
    _Check_return_ HRESULT InvokeImpl(_In_ const CDependencyProperty *pdp, _In_opt_ CDependencyObject *pNamescopeOwner) override;


public:
// CBeginStoryboard fields
    CStoryboard *m_pStoryboard = nullptr;
};
