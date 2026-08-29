// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ComTemplates.h>

#include "CDependencyObject.h"
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"

class IXamlPredicate
    : public CDependencyObject
{
protected:
    IXamlPredicate(_In_ CCoreServices* core)
        : CDependencyObject(core)
    {}

public:
    virtual bool Evaluate(_In_ std::vector<xstring_ptr>& args) = 0;
};

class CIsApiContractPresentPredicate final
    : public IXamlPredicate
{
public:
    // Creation method
    DECLARE_CREATE(CIsApiContractPresentPredicate);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CIsApiContractPresentPredicate>::Index;
    }

    bool Evaluate(_In_ std::vector<xstring_ptr>& args) override;

private:
    CIsApiContractPresentPredicate(_In_ CCoreServices* core)
        : IXamlPredicate(core)
    {}
};

class CIsApiContractNotPresentPredicate final
    : public IXamlPredicate
{
public:
    // Creation method
    DECLARE_CREATE(CIsApiContractNotPresentPredicate);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CIsApiContractNotPresentPredicate>::Index;
    }

    bool Evaluate(_In_ std::vector<xstring_ptr>& args) override;

private:
    CIsApiContractNotPresentPredicate(_In_ CCoreServices* core)
        : IXamlPredicate(core)
    {}
};

class CIsPropertyPresentPredicate final
    : public IXamlPredicate
{
public:
    // Creation method
    DECLARE_CREATE(CIsPropertyPresentPredicate);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CIsPropertyPresentPredicate>::Index;
    }

    bool Evaluate(_In_ std::vector<xstring_ptr>& args) override;

private:
    CIsPropertyPresentPredicate(_In_ CCoreServices* core)
        : IXamlPredicate(core)
    {}
};

class CIsPropertyNotPresentPredicate final
    : public IXamlPredicate
{
public:
    // Creation method
    DECLARE_CREATE(CIsPropertyNotPresentPredicate);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CIsPropertyNotPresentPredicate>::Index;
    }

    bool Evaluate(_In_ std::vector<xstring_ptr>& args) override;

private:
    CIsPropertyNotPresentPredicate(_In_ CCoreServices* core)
        : IXamlPredicate(core)
    {}
};

class CIsTypePresentPredicate final
    : public IXamlPredicate
{
public:
    // Creation method
    DECLARE_CREATE(CIsTypePresentPredicate);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CIsTypePresentPredicate>::Index;
    }

    bool Evaluate(_In_ std::vector<xstring_ptr>& args) override;

private:
    CIsTypePresentPredicate(_In_ CCoreServices* core)
        : IXamlPredicate(core)
    {}
};

class CIsTypeNotPresentPredicate final
    : public IXamlPredicate
{
public:
    // Creation method
    DECLARE_CREATE(CIsTypeNotPresentPredicate);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CIsTypeNotPresentPredicate>::Index;
    }

    bool Evaluate(_In_ std::vector<xstring_ptr>& args) override;

private:
    CIsTypeNotPresentPredicate(_In_ CCoreServices* core)
        : IXamlPredicate(core)
    {}
};