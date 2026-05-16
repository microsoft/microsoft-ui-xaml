// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    class PropertyPathListener;
    class PropertyPathStep;
    
    enum class PropertyPathStepDescriptorKind : XUINT8
    {
        None = 0,
        SourceAccess,
        PropertyAccess,
        IntIndexer,
        StringIndexer,
        DependencyProperty,
    };

    class PropertyPathStepDescriptor
    {
    public:
        PropertyPathStepDescriptor() noexcept;
        ~PropertyPathStepDescriptor() noexcept;

        // This is going in a vector and we want it to be move-only.
        PropertyPathStepDescriptor(const PropertyPathStepDescriptor&) = delete;
        PropertyPathStepDescriptor& operator=(const PropertyPathStepDescriptor&) = delete;

        PropertyPathStepDescriptor(PropertyPathStepDescriptor&& other) noexcept;
        PropertyPathStepDescriptor& operator=(PropertyPathStepDescriptor&& other) noexcept;

        static PropertyPathStepDescriptor CreateSourceAccess() noexcept;
        static PropertyPathStepDescriptor CreatePropertyAccess(_In_z_ WCHAR* szName) noexcept; // takes ownership
        static PropertyPathStepDescriptor CreateIntIndexer(XUINT32 nIndex) noexcept;
        static PropertyPathStepDescriptor CreateStringIndexer(_In_z_ WCHAR* szIndex) noexcept; // takes ownership
        static PropertyPathStepDescriptor CreateDependencyProperty(_In_ const CDependencyProperty* pDP) noexcept;

        _Check_return_ HRESULT CreateStep(
            _In_ PropertyPathListener* pListener,
            bool fListenToChanges,
            _Outptr_ PropertyPathStep** ppStep) const;

    private:
        void Reset() noexcept;
        void MoveFrom(_Inout_ PropertyPathStepDescriptor& other) noexcept;

    private:
        PropertyPathStepDescriptorKind m_kind;
        union
        {
            WCHAR* m_szText;
            XUINT32 m_nIndex;
            const CDependencyProperty* m_pDP;
        };
    };
}
