// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    class PropertyPathListener;
    class PropertyPathStep;
    class PropertyPathStepDescriptor;

    // Heap storage block for when we have more than 2 descriptors.
    struct HeapDescriptorStorage
    {
        // Pointer (and count) to array of descriptors on the heap
        size_t count = 0;
        PropertyPathStepDescriptor* descriptors = nullptr;

        PropertyPathStepDescriptor* begin() noexcept;
        PropertyPathStepDescriptor* end() noexcept;

        // Allocates a HeapDescriptorStorage with room for 'count' descriptors
        static HeapDescriptorStorage* Allocate(size_t count);
        // Frees the storage
        static void Free(HeapDescriptorStorage* storage) noexcept;
    };
    
    enum class PropertyPathStepDescriptorKind : XUINT8
    {
        None = 0,
        SourceAccess,
        PropertyAccess,
        IntIndexer,
        StringIndexer,
        DependencyProperty,
        HeapStorage,  // Points to HeapDescriptorStorage containing more descriptors
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
        static PropertyPathStepDescriptor CreatePropertyAccessShared(_In_z_ const WCHAR* szName) noexcept; // does NOT take ownership
        static PropertyPathStepDescriptor CreateIntIndexer(XUINT32 nIndex) noexcept;
        static PropertyPathStepDescriptor CreateStringIndexer(_In_z_ WCHAR* szIndex) noexcept; // takes ownership
        static PropertyPathStepDescriptor CreateDependencyProperty(_In_ const CDependencyProperty* pDP) noexcept;
        static PropertyPathStepDescriptor CreateHeapStorage(_In_ HeapDescriptorStorage* pStorage) noexcept; // takes ownership

        PropertyPathStepDescriptorKind GetKind() const noexcept { return m_kind; }
        HeapDescriptorStorage* GetHeapStorage() const noexcept { return m_pHeapStorage; }

        _Check_return_ HRESULT CreateStep(
            _In_ PropertyPathListener* pListener,
            bool fListenToChanges,
            _Outptr_ PropertyPathStep** ppStep) const;

    private:
        void Reset() noexcept;
        void MoveFrom(_Inout_ PropertyPathStepDescriptor& other) noexcept;

    private:
        PropertyPathStepDescriptorKind m_kind;
        bool m_ownsText = false; // Whether we own m_szText and should delete[] it
        union
        {
            const WCHAR* m_szText;
            XUINT32 m_nIndex;
            const CDependencyProperty* m_pDP;
            HeapDescriptorStorage* m_pHeapStorage;
        };
    };
}
