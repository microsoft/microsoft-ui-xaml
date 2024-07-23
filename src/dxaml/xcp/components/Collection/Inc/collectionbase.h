// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CDependencyObject.h"

struct ICollectionChangeCallback;

// Abstract Jupiter collection base class defining interface for other
// collections. It has a design only a mother could love.
class CCollection
    : public CDependencyObject
{
public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);

    virtual XUINT32 GetCount() const = 0;

    CDependencyObject* const GetOwner() const;
    virtual _Check_return_ HRESULT SetOwner(_In_opt_ CDependencyObject *pOwner, _In_opt_ RENDERCHANGEDPFN pfnOwnerDirtyFlag = nullptr)
    {
        return SetOwner(pOwner, false, pfnOwnerDirtyFlag);
    }
    _Check_return_ HRESULT SetAndPropagateOwner(_In_opt_ CDependencyObject* pOwner, _In_opt_ RENDERCHANGEDPFN pfnOwnerDirtyFlag = nullptr);
    virtual _Check_return_ HRESULT PropagateOwnerInfo(_In_opt_ CDependencyObject*, _In_opt_ CDependencyObject*)
    {
        return S_OK;
    };

    virtual bool IsDOCollection() const { return false; }
    virtual bool ContainsNoRefItems() const { return false; }
    virtual _Check_return_ HRESULT Append(CValue& value, _Out_opt_ XUINT32 *pnIndex = NULL) = 0;
    virtual _Check_return_ HRESULT Insert(_In_ XUINT32 nIndex, _In_ CValue& value) = 0;
    virtual _Check_return_ void *RemoveAt(_In_ XUINT32 nIndex) = 0;
    virtual _Check_return_ void *GetItemWithAddRef(_In_ XUINT32 nIndex) = 0;
    virtual _Check_return_ HRESULT Neat(_In_ bool bBreak) = 0;
    virtual _Check_return_ HRESULT Reserve(_In_ XUINT32) { return S_OK; }
    virtual _Check_return_ HRESULT Clear();
    virtual _Check_return_ HRESULT IndexOf(CValue& value, _Out_ XINT32 *pIndex);
    virtual _Check_return_ HRESULT MoveInternal(_In_ XINT32 nIndex, _In_ XINT32 nPosition) = 0;

    void SetAffectsOwnerMeasure(XUINT32 value);
    void SetAffectsOwnerArrange(XUINT32 value);
    void Lock() { ASSERT(m_cLock < (0x1FFFFFFF-1)); if (m_cLock < (0x1FFFFFFF-1)) ++m_cLock; } // prevent overflow
    void Unlock() { ASSERT(m_cLock > 0); if (m_cLock > 0) --m_cLock; } // prevent underflow
    bool IsLocked() const { return m_cLock > 0; }
    _Check_return_ HRESULT FailIfLocked();

#pragma region Static Validation Methods
    // Common Validation code for Static members.  Individual functions may have additional
    // validation requirements.
    _Check_return_ HRESULT static ValidateParams(_In_ XUINT32 cArgsExpected, _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs, _In_reads_(cArgs) CValue *ppArgs);
    _Check_return_ HRESULT static GetItemCount(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(0) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);
    _Check_return_ HRESULT static Add(_In_ CDependencyObject *pObject, _In_ XUINT32 cArgs,
        _Inout_updates_(1) CValue *ppArgs, _Out_opt_ CValue *pResult);
    _Check_return_ HRESULT static Clear(_In_reads_bytes_(sizeof(CCollection)) CDependencyObject *pObject, _In_ XUINT32 cArgs, _Inout_updates_opt_(0) CValue *ppArgs, _Out_opt_ CValue *pResult);
    _Check_return_ HRESULT static Count(_In_ CDependencyObject *pObject, _In_ XUINT32 cArgs, _Inout_updates_opt_(0) CValue *ppArgs, _Inout_ CValue *pResult);
    _Check_return_ HRESULT static GetItem(_In_ CDependencyObject *pObject, _In_ XUINT32 index, _Out_ CValue *pResult);
    _Check_return_ HRESULT static InsertDO(_In_ CDependencyObject *pObject, _In_ XUINT32 nIndex, _In_ CValue& value);
    _Check_return_ HRESULT static IndexOf(_In_ CDependencyObject *pObject, _In_ XUINT32 cArgs,
        _Inout_updates_(1) CValue *ppArgs, _Out_ CValue *pResult);

    virtual void SetChangeCallback(
        _In_ const std::weak_ptr<ICollectionChangeCallback>& callback)
    {}
#pragma endregion

protected:
    CCollection(_In_ CCoreServices* pCore)
        : CDependencyObject(pCore)
        , m_cLock(0)
        , m_fOwnerIsFallback(FALSE)
        , m_fAffectsOwnerMeasure(FALSE)
        , m_fAffectsOwnerArrange(FALSE)
    {}

    ~CCollection() override;

    void ItemToCValue(_In_ CValue *pResult, _In_ void *item, bool fDeleteValueObject, bool fManaged);

    virtual _Check_return_ bool NeedsOwnerInfo()
    {
        return false;
    }

    virtual _Check_return_ HRESULT Destroy();

    // The owner can be set externally or internally. It's not always the parent of the DOCollection,
    // sometimes it's the namescope owner and othertimes it's whatever you can imagine it to be.
    virtual _Check_return_ HRESULT SetOwner(
        _In_opt_ CDependencyObject *pOwner,
        bool bOwnerIsFallback,
         _In_opt_ RENDERCHANGEDPFN pfnOwnerDirtyFlag = NULL);

    // CDependencyObject override
    void NWPropagateDirtyFlag(DirtyFlags flags) final;

    // These On... functions must be called by CCollection overrides if you wish to use them. Except
    // they aren't by the collection implementations today and are instead called manually by
    // calling code.
    virtual _Check_return_ HRESULT PreAddToCollection(_In_ CValue& newItem);
    virtual _Check_return_ HRESULT OnAddToCollection(_In_ const CValue&) { return S_OK; }
    virtual _Check_return_ HRESULT OnRemoveFromCollection(_In_ const CValue&, _In_ XINT32) { return S_OK; }
    virtual void OnRemovingDependencyObject(_In_ CDependencyObject* pDO) { pDO->SetAssociated(false, nullptr); }
    virtual _Check_return_ HRESULT OnClear() { return S_OK; }

    // This member should be above the ones below, to allow for better alignment of the data members while still leaving bitfields at the end
    // (this is convenient for the next person who adds a bitfield).
    xref::weakref_ptr<CDependencyObject> m_pOwner; // The Owner of the Collection.

    // TODO: MERGE: This should really be private and moved to DOCollection instead along with all the owner code since it's only used by that subclass.
    RENDERCHANGEDPFN m_pfnOwnerRenderChangedHandler = nullptr; // The dirty flag function to call on the owner when the collection changes

    XUINT32    m_fOwnerIsFallback    :  1;  // m_pOwner is currently set to the NamescopeOwner as a fallback measure.
    XUINT32    m_fAffectsOwnerMeasure:  1;  //indicates if the property this collection is a value of, is marked as affects measure
    XUINT32    m_fAffectsOwnerArrange:  1;  //indicates if the property this collection is a value of, is marked as affects arrange

private:
    XUINT32    m_cLock               : 29; // A lock count on this collection
};
