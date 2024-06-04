// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <vector_map.h>
#include <NamespaceAliases.h>
#include <indexes.g.h>
#include <FacadeReferenceWrapper.h>

#define DUAL_NAMESPACE Microsoft
#include <microsoft.ui.composition.h>
#include <microsoft.ui.composition.experimental.h>

class CUIElement;
class CDependencyObject;


static const wchar_t FacadeProperty_Translation[] = L"Translation";
static const wchar_t FacadeProperty_Translation_Comp[] = L"Offset"; // Translation corresponds to Offset in Composition
static const wchar_t FacadeProperty_Rotation[] = L"Rotation";
static const wchar_t FacadeProperty_Rotation_Comp[] = L"RotationAngleInDegrees";  // Rotation corresponds to RotationAngleInDegrees in Composition
static const wchar_t FacadeProperty_Scale[] = L"Scale";
static const wchar_t FacadeProperty_TransformMatrix[] = L"TransformMatrix";
static const wchar_t FacadeProperty_CenterPoint[] = L"CenterPoint";
static const wchar_t FacadeProperty_RotationAxis[] = L"RotationAxis";
static const wchar_t FacadeProperty_Opacity[] = L"Opacity";
static const wchar_t FacadeProperty_ActualOffset[] = L"ActualOffset";
static const wchar_t FacadeProperty_ActualSize[] = L"ActualSize";

// Although we only have the one listener now (PropertySet), we will eventually have listeners for
// brushes and possibly geometery.  But for now, our only abstraction for this will be an interface
// so that it can be stored and used without having to know what the
// object is underneath.  At some point we will probably want to break the common parts of
// PropertySetListener into a CompositionPropertyListener base class that will be used for things other
// than property sets.
interface __declspec(uuid("3afa597f-2e53-4235-98b4-33e43d94477b")) IFacadePropertyListener : public IInspectable
{
    virtual void OnFacadePropertyInserted(KnownPropertyIndex facadeID) = 0;
    virtual void OnAnimationStarted(_In_ WUComp::ICompositionObject* backingPropertySet, KnownPropertyIndex facadeID) = 0;
    virtual void OnAnimationCompleted(_In_ WUComp::ICompositionObject* backingPropertySet, KnownPropertyIndex facadeID) = 0;
};

class DOFacadeAnimationInfo
{
public:
    DOFacadeAnimationInfo() {}
    ~DOFacadeAnimationInfo() {}

    void UnregisterCompletionHandler();

    KnownPropertyIndex m_facadeID = KnownPropertyIndex::UnknownType_UnknownProperty;
    wrl::ComPtr<WUComp::ICompositionAnimationBase> m_animation;
    wrl::ComPtr<WUComp::ICompositionScopedBatch> m_scopedBatch;
    EventRegistrationToken m_token = {0};
    bool m_animationRunning = false;

    // Implicit animations are not stopped when the property is set to the same value again. Explicit animations are stopped.
    bool m_isImplicitAnimation = false;
};

typedef containers::vector_map<KnownPropertyIndex, DOFacadeAnimationInfo> DOFacadeAnimationMap;

struct DOFacadeStorage
{
    wrl::ComPtr<WUComp::ICompositionObject> m_backingCO;
    wrl::ComPtr<IFacadePropertyListener> m_listener;
    wrl::ComPtr<FacadeReferenceWrapper> m_referenceWrapper;
    DOFacadeAnimationMap m_doFacadeAnimationMap;
    bool m_isDeferringReferences = false;
};
typedef containers::vector_map<const CDependencyObject*, DOFacadeStorage> DOFacadeMap;

class FacadeStorage
{
public:
    FacadeStorage() {}
    ~FacadeStorage() {}

    wrl::ComPtr<WUComp::ICompositionObject> GetBackingCompositionObject(_In_ const CDependencyObject* object) const;

    void SetBackingCompositionObject(_In_ const CDependencyObject* object, _In_ WUComp::ICompositionObject* backingCO);

    wrl::ComPtr<IFacadePropertyListener> GetBackingListener(_In_ const CDependencyObject* object) const;

    void SetBackingListener(_In_ const CDependencyObject* object, _In_ IFacadePropertyListener* listener);

    wrl::ComPtr<FacadeReferenceWrapper> GetReferenceWrapper(_In_ const CDependencyObject* object) const;
    void SetReferenceWrapper(_In_ const CDependencyObject* object, _In_ FacadeReferenceWrapper* referenceWrapper);

    void SetIsDeferringReferences(_In_ const CDependencyObject* object, bool isDeferringReferences);
    bool IsDeferringReferences(_In_ const CDependencyObject* object) const;

    DOFacadeAnimationInfo& EnsureFacadeAnimationInfo(_In_ const CDependencyObject* object, KnownPropertyIndex facadeID);

    DOFacadeAnimationInfo* TryGetFacadeAnimationInfo(_In_ const CDependencyObject* object, KnownPropertyIndex facadeID) const;
    bool HasAnimation(_In_ const CDependencyObject* object, KnownPropertyIndex facadeID) const;
    bool HasExplicitAnimation(_In_ const CDependencyObject* object, KnownPropertyIndex facadeID) const;

    DOFacadeAnimationInfo* GetFacadeAnimationInfoForScopedBatch(_In_ const CDependencyObject* object, _In_ WUComp::ICompositionScopedBatch* scopedBatch);

    void ClearFacadeAnimationInfo(_In_ const CDependencyObject* object, KnownPropertyIndex facadeID);

    bool HasAnyFacadeAnimations(_In_ const CDependencyObject* object);

    KnownPropertyIndex FindFirstStrictAnimatingFacade(_In_ const CDependencyObject* object);

    wrl::ComPtr<WUComp::IExpressionAnimation> GetFacadeGlueExpression(KnownPropertyIndex facadeID) const;
    void SetFacadeGlueExpression(KnownPropertyIndex facadeID, _In_ WUComp::IExpressionAnimation* glueExpression);

    void OnDODestroyed(_In_ const CDependencyObject* object);

    void ShrinkToFit();

    void CleanupCompositorResources();

private:
    DOFacadeStorage& EnsureDOFacadeStorage(_In_ const CDependencyObject* object);

    DOFacadeStorage* TryGetDOFacadeStorage(_In_ const CDependencyObject* object) const;

    void ClearDOFacadeStorage(_In_ const CDependencyObject* object);

private:
    // Maps a DO to information about the DO, including backing CompositionObject and running animations
    DOFacadeMap m_doFacadeMap;

    // Maps a facadeID to a cached expression that acts as "glue" to hook up backing CompositionObject with final destination
    typedef containers::vector_map<KnownPropertyIndex, wrl::ComPtr<WUComp::IExpressionAnimation>> FacadeGlueExpressionMap;
    FacadeGlueExpressionMap m_facadeGlueExpressionMap;
};

// Helper RAII object to set/clear the "deferred references" flag
class DeferReferenceGuard
{
public:
    DeferReferenceGuard(_In_ FacadeStorage& storage, _In_ CDependencyObject* object)
    : m_storage(storage)
    , m_object(object)
    {
        m_storage.SetIsDeferringReferences(m_object, true);
    }

    ~DeferReferenceGuard()
    {
        m_storage.SetIsDeferringReferences(m_object, false);
    }

    // Noncopyable, nonmovable
    DeferReferenceGuard(const DeferReferenceGuard&) = delete;
    DeferReferenceGuard& operator=(const DeferReferenceGuard&) = delete;

private:
    FacadeStorage& m_storage;
    CDependencyObject* m_object;
};
