// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CCoreServices;
class VisualTree;
class CDependencyObject;
class CTransformGroup;
class CValue;
struct DisplayListener;
class CContentRoot;

#include <unordered_set>
#include <ImageReloadManager.h>
#include <Microsoft.UI.Content.h>

enum RootScaleConfig
{
    // Parent scale is identity or it expects the root visual tree to apply system DPI scale itself.
    ParentInvert,
    // Parent scale already applyes the system DPI scale, so need to apply in the internal root visual tree.
    ParentApply,
};

class RootScale
{
public:
    RootScale(RootScaleConfig config, CCoreServices* pCoreServices, VisualTree* pVisualTree);
    virtual ~RootScale();

    float GetEffectiveRasterizationScale();

    void AddDisplayListener(_In_ DisplayListener *displayListener);
    void RemoveDisplayListener(_In_ DisplayListener *displayListener);

    CImageReloadManager& GetImageReloadManager();

    _Check_return_ HRESULT SetSystemScale(float scale);
    _Check_return_ HRESULT SetTestOverride(float scale);
    _Check_return_ HRESULT ApplyScale();
    bool IsInitialized() const
    {
        return m_bInitialized;
    }

    static RootScale* GetRootScaleForContentRoot(_In_opt_ CContentRoot* coreContextRoot);
    static float GetRasterizationScaleForContentRoot(_In_opt_ CContentRoot* coreContextRoot)
    {
        if (auto rootScale = GetRootScaleForContentRoot(coreContextRoot))
        {
            return rootScale->GetEffectiveRasterizationScale();
        }
        return 1.0f;
    }

    static RootScale* GetRootScaleForElement(_In_ CDependencyObject* pDO);
    static float GetRasterizationScaleForElement(_In_ CDependencyObject* pDO)
    {
        if (auto rootScale = GetRootScaleForElement(pDO))
        {
            return rootScale->GetEffectiveRasterizationScale();
        }
        return 1.0f;
    }

    static RootScale* GetRootScaleForElementWithFallback(_In_opt_ CDependencyObject* pDO);
    static float GetRasterizationScaleForElementWithFallback(_In_ CDependencyObject* pDO)
    {
        auto rootScale = GetRootScaleForElementWithFallback(pDO);
        if (rootScale)
        {
            return rootScale->GetEffectiveRasterizationScale();
        }
        return 1.0f;
    }

    void SetContentIsland(_In_opt_ ixp::IContentIsland * content);
    _Check_return_ HRESULT UpdateSystemScale();

protected:

    float GetRootVisualScale();
    float GetSystemScale();

    virtual _Check_return_ HRESULT ApplyScaleProtected(bool scaleChanged) = 0;

    VisualTree* GetVisualTreeNoRef() const
    {
        return m_pVisualTree;
    }

private:
    enum ScaleKind
    {
        System,
        Test,
    };
    _Check_return_ HRESULT SetScale(float scale, ScaleKind kind);
    _Check_return_ HRESULT ApplyScale(bool scaleChanged);

private:
    RootScaleConfig m_config;
    // The system DPI, this is accumulated scale,
    // tipically this is control by the Display Settings app.
    float m_systemScale = 1.0f;
    // Used only for testing, it replaces the system DPI scale with a value
    // This can only be used when config is RootScaleConfig::ParentInvert
    float m_testOverrideScale = 0.0f;
    std::unordered_set<DisplayListener*> m_displayListeners;
    VisualTree* m_pVisualTree = nullptr;
    bool m_bInitialized = false;
    bool m_bUpdating = false;
    CImageReloadManager m_imageReloadManager;
    wrl::ComPtr<ixp::IContentIsland> m_content;  // IExpCompositionContent
protected:
    CCoreServices* m_pCoreServices = nullptr;
};
