// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Abstract base class for Grid Row/Column Definitions.
class CDefinitionBase : public CDependencyObject
{
protected:
    CDefinitionBase(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

    _Check_return_ HRESULT FromString(_In_ CREATEPARAMETERS* pCreate);

public:
    float GetUserSizeValue() const
    {
        return m_pUserSize.value;
    }

    DirectUI::GridUnitType GetUserSizeType() const
    {
        return (DirectUI::GridUnitType)m_pUserSize.type;
    }

    float GetUserMaxSize() const
    {
        return m_eUserMaxSize;
    }

    float GetUserMinSize() const
    {
        return m_eUserMinSize;
    }

    float GetEffectiveMinSize() const
    {
        return m_effectiveMinSize;
    }

    void SetEffectiveMinSize(float value)
    {
        m_effectiveMinSize = value;
    }

    float GetMeasureArrangeSize() const
    {
        return m_measureArrangeSize;
    }

    void SetMeasureArrangeSize(float value)
    {
        m_measureArrangeSize = value;
    }

    float GetSizeCache() const
    {
        return m_sizeCache;
    }

    void SetSizeCache(float value)
    {
        m_sizeCache = value;
    }

    float GetFinalOffset() const
    {
        return m_finalOffset;
    }

    void SetFinalOffset(float value)
    {
        m_finalOffset = value;
    }

    DirectUI::GridUnitType GetEffectiveUnitType() const
    {
        return m_effectiveUnitType;
    }

    void SetEffectiveUnitType(DirectUI::GridUnitType type)
    {
        m_effectiveUnitType = type;
    }

    float GetPreferredSize() const
    {
        return
            (m_effectiveUnitType != DirectUI::GridUnitType::Auto
                && m_effectiveMinSize < m_measureArrangeSize)
                ? m_measureArrangeSize
                : m_effectiveMinSize;
    }

    void UpdateEffectiveMinSize(float newValue)
    {
        m_effectiveMinSize = std::max(m_effectiveMinSize, newValue);
    }

    _Check_return_ static HRESULT ActualSize(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _In_reads_(cArgs) CValue *pArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);

private:

    float GetActualSize() const;

public:
    // Backing fields for public properties.
    XGRIDLENGTH m_pUserSize     = { DirectUI::GridUnitType::Star, {}, 1.0f };
    XFLOAT m_eUserMinSize       = 0.0f;
    XFLOAT m_eUserMaxSize       = std::numeric_limits<float>::infinity();

private:
    float m_effectiveMinSize                    = 0.0f;
    float m_measureArrangeSize                  = 0.0f;
    float m_sizeCache                           = 0.0f;
    float m_finalOffset                         = 0.0f;
    DirectUI::GridUnitType m_effectiveUnitType  = DirectUI::GridUnitType::Auto;
};

class CRowDefinition final : public CDefinitionBase
{
private:
    CRowDefinition(_In_ CCoreServices *pCore)
        : CDefinitionBase(pCore)
    {}

public:
    DECLARE_CREATE_WITH_TYPECONVERTER(CRowDefinition);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CRowDefinition>::Index;
    }

protected:
    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;
};

class CColumnDefinition final : public CDefinitionBase
{
private:
    CColumnDefinition(_In_ CCoreServices *pCore)
        : CDefinitionBase(pCore)
    {}

public:
    DECLARE_CREATE_WITH_TYPECONVERTER(CColumnDefinition);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CColumnDefinition>::Index;
    }

protected:
    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;
};
