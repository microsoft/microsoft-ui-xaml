// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DCompTreeHelper.h"
#include "hwcompnode.h"
#include "corep.h"
#include "uielement.h"
#include "visualtree.h"
#include "DXamlServices.h"
#include <MetadataAPI.h>

namespace
{
    // Maps the small WUC enums to readable names for the XML dump (instead of raw ints).
    const WCHAR* CompositeModeName(WUComp::CompositionCompositeMode value)
    {
        switch (value)
        {
            case WUComp::CompositionCompositeMode::CompositionCompositeMode_Inherit:           return L"Inherit";
            case WUComp::CompositionCompositeMode::CompositionCompositeMode_SourceOver:        return L"SourceOver";
            case WUComp::CompositionCompositeMode::CompositionCompositeMode_DestinationInvert: return L"DestinationInvert";
            case WUComp::CompositionCompositeMode::CompositionCompositeMode_MinBlend:          return L"MinBlend";
            default:                                                                            return L"Unknown";
        }
    }

    const WCHAR* BorderModeName(WUComp::CompositionBorderMode value)
    {
        switch (value)
        {
            case WUComp::CompositionBorderMode::CompositionBorderMode_Inherit: return L"Inherit";
            case WUComp::CompositionBorderMode::CompositionBorderMode_Soft:    return L"Soft";
            case WUComp::CompositionBorderMode::CompositionBorderMode_Hard:    return L"Hard";
            default:                                                            return L"Unknown";
        }
    }

    const WCHAR* BackfaceVisibilityName(WUComp::CompositionBackfaceVisibility value)
    {
        switch (value)
        {
            case WUComp::CompositionBackfaceVisibility::CompositionBackfaceVisibility_Inherit: return L"Inherit";
            case WUComp::CompositionBackfaceVisibility::CompositionBackfaceVisibility_Visible: return L"Visible";
            case WUComp::CompositionBackfaceVisibility::CompositionBackfaceVisibility_Hidden:  return L"Hidden";
            default:                                                                            return L"Unknown";
        }
    }

    // Asks a WUC object for its own runtime class name (e.g. "CompositionColorBrush"),
    // stripping the leading "Microsoft.UI.Composition." namespace. Writes "" on failure.
    void GetWucClassName(_In_opt_ IInspectable* pObject, _Out_writes_(bufferSize) WCHAR* buffer, _In_ size_t bufferSize)
    {
        buffer[0] = L'\0';
        if (pObject == nullptr)
        {
            return;
        }

        xstring_ptr className;
        if (SUCCEEDED(DirectUI::MetadataAPI::GetRuntimeClassName(pObject, &className)) && !className.IsNullOrEmpty())
        {
            const WCHAR* pFull = className.GetBuffer();
            if (pFull != nullptr)
            {
                const WCHAR* pShort = wcsrchr(pFull, L'.');
                wcscpy_s(buffer, bufferSize, pShort != nullptr ? pShort + 1 : pFull);
            }
        }
    }

    // Returns the concrete WUC visual type name via a QI ladder (most-derived first,
    // since SpriteVisual/LayerVisual/ShapeVisual/RedirectVisual all derive from
    // ContainerVisual and would otherwise match IContainerVisual first).
    const WCHAR* GetWucVisualTypeName(_In_ WUComp::IVisual* pVisual)
    {
        if (pVisual == nullptr)
        {
            return L"null";
        }

        xref_ptr<WUComp::ISpriteVisual> spSprite;
        if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(spSprite.ReleaseAndGetAddressOf()))))
        {
            return L"SpriteVisual";
        }
        xref_ptr<WUComp::ILayerVisual> spLayer;
        if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(spLayer.ReleaseAndGetAddressOf()))))
        {
            return L"LayerVisual";
        }
        xref_ptr<WUComp::IShapeVisual> spShape;
        if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(spShape.ReleaseAndGetAddressOf()))))
        {
            return L"ShapeVisual";
        }
        xref_ptr<WUComp::IRedirectVisual> spRedirect;
        if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(spRedirect.ReleaseAndGetAddressOf()))))
        {
            return L"RedirectVisual";
        }
        xref_ptr<WUComp::IContainerVisual> spContainer;
        if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(spContainer.ReleaseAndGetAddressOf()))))
        {
            return L"ContainerVisual";
        }
        return L"Visual";
    }

    // Returns the number of direct WUC children for a visual, or -1 if it isn't a
    // container (and thus has no Children collection).
    int GetWucVisualChildCount(_In_ WUComp::IVisual* pVisual)
    {
        xref_ptr<WUComp::IContainerVisual> spContainer;
        if (pVisual == nullptr ||
            FAILED(pVisual->QueryInterface(IID_PPV_ARGS(spContainer.ReleaseAndGetAddressOf()))) ||
            spContainer == nullptr)
        {
            return -1;
        }
        xref_ptr<WUComp::IVisualCollection> spChildren;
        if (FAILED(spContainer->get_Children(spChildren.ReleaseAndGetAddressOf())) || spChildren == nullptr)
        {
            return -1;
        }
        int count = 0;
        if (FAILED(spChildren->get_Count(&count)))
        {
            return -1;
        }
        return count;
    }

    // Reads every render property off a WUC visual and appends them as XML attributes to
    // 'line' starting at offset *pN (advancing *pN). The final (type-specific) group ends
    // right after its closing quote - no trailing space and no '>' - so the caller appends
    // either ">" or " />". Safe with pVisual == null (emits default values). Shared by the
    // <WucVisual> visual-tree nodes; mirrors the attribute set the <CompNode> tag emits.
    void AppendVisualPropertyAttributes(
        _In_opt_ WUComp::IVisual* pVisual,
        _Inout_ WCHAR* line,
        _Inout_ int* pN,
        _In_ int cap)
    {
        // Base render properties.
        float offsetX = 0, offsetY = 0, offsetZ = 0;
        float sizeW = 0, sizeH = 0;
        float opacity = 1.0f;
        bool isVisible = true;

        // Type-specific properties.
        int wucChildCount = -1;               // -1 = not a container-derived visual
        int shapeCount = -1;                  // ShapeVisual only
        bool hasBrush = false, hasShadow = false, hasEffect = false, hasViewBox = false, hasClip = false;
        WCHAR brushType[128] = L"";           // kind of Brush  (SpriteVisual)
        WCHAR shadowType[128] = L"";          // kind of Shadow (Sprite/Layer)
        WCHAR clipType[128] = L"";            // kind of Clip   (any Visual)
        void* pSourceVisualRaw = nullptr;     // RedirectVisual.Source

        // Extended base Visual transform properties (IVisual / IVisual2 / IVisual3 / IVisual4).
        float anchorX = 0, anchorY = 0;
        float centerX = 0, centerY = 0, centerZ = 0;
        float scaleX = 1, scaleY = 1, scaleZ = 1;
        float rotationDegrees = 0;
        float rotationAxisX = 0, rotationAxisY = 0, rotationAxisZ = 1;
        float orientX = 0, orientY = 0, orientZ = 0, orientW = 1;
        wfn::Matrix4x4 matrix = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
        float relOffsetX = 0, relOffsetY = 0, relOffsetZ = 0;
        float relSizeX = 0, relSizeY = 0;
        const WCHAR* compositeMode = L"Inherit";
        const WCHAR* borderMode = L"Inherit";
        const WCHAR* backfaceVisibility = L"Inherit";
        bool isHitTestVisible = true;
        bool isPixelSnappingEnabled = false;

        if (pVisual != nullptr)
        {
            // Base composition render properties.
            wfn::Vector3 offset = {};
            wfn::Vector2 size = {};
            boolean visible = TRUE;
            if (SUCCEEDED(pVisual->get_Offset(&offset))) { offsetX = offset.X; offsetY = offset.Y; offsetZ = offset.Z; }
            if (SUCCEEDED(pVisual->get_Size(&size)))     { sizeW = size.X; sizeH = size.Y; }
            pVisual->get_Opacity(&opacity);
            if (SUCCEEDED(pVisual->get_IsVisible(&visible))) { isVisible = !!visible; }

            // Extended transform properties (still on the base IVisual).
            wfn::Vector2 anchor = {};
            if (SUCCEEDED(pVisual->get_AnchorPoint(&anchor))) { anchorX = anchor.X; anchorY = anchor.Y; }
            wfn::Vector3 center = {};
            if (SUCCEEDED(pVisual->get_CenterPoint(&center))) { centerX = center.X; centerY = center.Y; centerZ = center.Z; }
            wfn::Vector3 scale = {};
            if (SUCCEEDED(pVisual->get_Scale(&scale))) { scaleX = scale.X; scaleY = scale.Y; scaleZ = scale.Z; }
            pVisual->get_RotationAngleInDegrees(&rotationDegrees);
            wfn::Vector3 axis = {};
            if (SUCCEEDED(pVisual->get_RotationAxis(&axis))) { rotationAxisX = axis.X; rotationAxisY = axis.Y; rotationAxisZ = axis.Z; }
            wfn::Quaternion orient = {};
            if (SUCCEEDED(pVisual->get_Orientation(&orient))) { orientX = orient.X; orientY = orient.Y; orientZ = orient.Z; orientW = orient.W; }
            pVisual->get_TransformMatrix(&matrix);

            WUComp::CompositionCompositeMode compMode = {};
            if (SUCCEEDED(pVisual->get_CompositeMode(&compMode))) { compositeMode = CompositeModeName(compMode); }
            WUComp::CompositionBorderMode bMode = {};
            if (SUCCEEDED(pVisual->get_BorderMode(&bMode))) { borderMode = BorderModeName(bMode); }
            WUComp::CompositionBackfaceVisibility bfVis = {};
            if (SUCCEEDED(pVisual->get_BackfaceVisibility(&bfVis))) { backfaceVisibility = BackfaceVisibilityName(bfVis); }

            xref_ptr<WUComp::ICompositionClip> spClip;
            if (SUCCEEDED(pVisual->get_Clip(spClip.ReleaseAndGetAddressOf())) && spClip != nullptr)
            {
                hasClip = true;
                GetWucClassName(spClip.get(), clipType, _countof(clipType));
            }

            // Versioned base interfaces (IVisual2/3/4) - present on newer builds only.
            xref_ptr<WUComp::IVisual2> spVisual2;
            if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(spVisual2.ReleaseAndGetAddressOf()))))
            {
                wfn::Vector3 relOffset = {};
                if (SUCCEEDED(spVisual2->get_RelativeOffsetAdjustment(&relOffset))) { relOffsetX = relOffset.X; relOffsetY = relOffset.Y; relOffsetZ = relOffset.Z; }
                wfn::Vector2 relSize = {};
                if (SUCCEEDED(spVisual2->get_RelativeSizeAdjustment(&relSize))) { relSizeX = relSize.X; relSizeY = relSize.Y; }
            }
            xref_ptr<WUComp::IVisual3> spVisual3;
            if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(spVisual3.ReleaseAndGetAddressOf()))))
            {
                boolean htv = TRUE;
                if (SUCCEEDED(spVisual3->get_IsHitTestVisible(&htv))) { isHitTestVisible = !!htv; }
            }
            xref_ptr<WUComp::IVisual4> spVisual4;
            if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(spVisual4.ReleaseAndGetAddressOf()))))
            {
                boolean pse = FALSE;
                if (SUCCEEDED(spVisual4->get_IsPixelSnappingEnabled(&pse))) { isPixelSnappingEnabled = !!pse; }
            }

            // Type-specific properties via QI ladder (most-derived first, since the four
            // leaf types all derive from ContainerVisual and would match IContainerVisual too).
            xref_ptr<WUComp::ISpriteVisual> spSprite;
            xref_ptr<WUComp::ILayerVisual> spLayer;
            xref_ptr<WUComp::IShapeVisual> spShape;
            xref_ptr<WUComp::IRedirectVisual> spRedirect;
            xref_ptr<WUComp::IContainerVisual> spContainer;

            if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(spSprite.ReleaseAndGetAddressOf()))))
            {
                xref_ptr<WUComp::ICompositionBrush> spBrush;
                if (SUCCEEDED(spSprite->get_Brush(spBrush.ReleaseAndGetAddressOf())) && spBrush != nullptr)
                {
                    hasBrush = true;
                    GetWucClassName(spBrush.get(), brushType, _countof(brushType));
                }
                xref_ptr<WUComp::ISpriteVisual2> spSprite2;
                if (SUCCEEDED(spSprite->QueryInterface(IID_PPV_ARGS(spSprite2.ReleaseAndGetAddressOf()))))
                {
                    xref_ptr<WUComp::ICompositionShadow> spShadow;
                    if (SUCCEEDED(spSprite2->get_Shadow(spShadow.ReleaseAndGetAddressOf())) && spShadow != nullptr)
                    {
                        hasShadow = true;
                        GetWucClassName(spShadow.get(), shadowType, _countof(shadowType));
                    }
                }
            }
            else if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(spLayer.ReleaseAndGetAddressOf()))))
            {
                xref_ptr<WUComp::ICompositionEffectBrush> spEffect;
                if (SUCCEEDED(spLayer->get_Effect(spEffect.ReleaseAndGetAddressOf())) && spEffect != nullptr)
                {
                    hasEffect = true;
                }
                xref_ptr<WUComp::ILayerVisual2> spLayer2;
                if (SUCCEEDED(spLayer->QueryInterface(IID_PPV_ARGS(spLayer2.ReleaseAndGetAddressOf()))))
                {
                    xref_ptr<WUComp::ICompositionShadow> spShadow;
                    if (SUCCEEDED(spLayer2->get_Shadow(spShadow.ReleaseAndGetAddressOf())) && spShadow != nullptr)
                    {
                        hasShadow = true;
                        GetWucClassName(spShadow.get(), shadowType, _countof(shadowType));
                    }
                }
            }
            else if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(spShape.ReleaseAndGetAddressOf()))))
            {
                xref_ptr<wfc::IVector<WUComp::CompositionShape*>> spShapes;
                if (SUCCEEDED(spShape->get_Shapes(spShapes.ReleaseAndGetAddressOf())) && spShapes != nullptr)
                {
                    unsigned int count = 0;
                    if (SUCCEEDED(spShapes->get_Size(&count))) { shapeCount = static_cast<int>(count); }
                }
                xref_ptr<WUComp::ICompositionViewBox> spViewBox;
                if (SUCCEEDED(spShape->get_ViewBox(spViewBox.ReleaseAndGetAddressOf())) && spViewBox != nullptr)
                {
                    hasViewBox = true;
                }
            }
            else if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(spRedirect.ReleaseAndGetAddressOf()))))
            {
                xref_ptr<WUComp::IVisual> spSource;
                if (SUCCEEDED(spRedirect->get_Source(spSource.ReleaseAndGetAddressOf())))
                {
                    pSourceVisualRaw = spSource.get();
                }
            }

            // WUC child count for any container-derived visual (incl. the leaf types,
            // which inherit Children from ContainerVisual).
            if (spContainer == nullptr)
            {
                pVisual->QueryInterface(IID_PPV_ARGS(spContainer.ReleaseAndGetAddressOf()));
            }
            if (spContainer != nullptr)
            {
                xref_ptr<WUComp::IVisualCollection> spChildren;
                if (SUCCEEDED(spContainer->get_Children(spChildren.ReleaseAndGetAddressOf())) && spChildren != nullptr)
                {
                    int count = 0;
                    if (SUCCEEDED(spChildren->get_Count(&count))) { wucChildCount = count; }
                }
            }
        }

        int n = *pN;

        // Base Visual transform / render properties.
        n += swprintf_s(line + n, cap - n,
            L"offsetX=\"%.1f\" offsetY=\"%.1f\" offsetZ=\"%.1f\" width=\"%.1f\" height=\"%.1f\" "
            L"anchorPointX=\"%.2f\" anchorPointY=\"%.2f\" "
            L"centerPointX=\"%.2f\" centerPointY=\"%.2f\" centerPointZ=\"%.2f\" "
            L"scaleX=\"%.3f\" scaleY=\"%.3f\" scaleZ=\"%.3f\" "
            L"rotationAngleInDegrees=\"%.2f\" rotationAxisX=\"%.2f\" rotationAxisY=\"%.2f\" rotationAxisZ=\"%.2f\" "
            L"orientationX=\"%.3f\" orientationY=\"%.3f\" orientationZ=\"%.3f\" orientationW=\"%.3f\" ",
            offsetX, offsetY, offsetZ, sizeW, sizeH,
            anchorX, anchorY,
            centerX, centerY, centerZ,
            scaleX, scaleY, scaleZ,
            rotationDegrees, rotationAxisX, rotationAxisY, rotationAxisZ,
            orientX, orientY, orientZ, orientW);

        // Transform matrix + modes (readable enum names) + relative adjustments + flags.
        n += swprintf_s(line + n, cap - n,
            L"transformMatrix=\"%.3f,%.3f,%.3f,%.3f, %.3f,%.3f,%.3f,%.3f, %.3f,%.3f,%.3f,%.3f, %.3f,%.3f,%.3f,%.3f\" "
            L"opacity=\"%.3f\" isVisible=\"%s\" isHitTestVisible=\"%s\" isPixelSnappingEnabled=\"%s\" "
            L"compositeMode=\"%s\" borderMode=\"%s\" backfaceVisibility=\"%s\" hasClip=\"%s\" clipType=\"%s\" "
            L"relativeOffsetAdjustmentX=\"%.2f\" relativeOffsetAdjustmentY=\"%.2f\" relativeOffsetAdjustmentZ=\"%.2f\" "
            L"relativeSizeAdjustmentX=\"%.2f\" relativeSizeAdjustmentY=\"%.2f\" ",
            matrix.M11, matrix.M12, matrix.M13, matrix.M14,
            matrix.M21, matrix.M22, matrix.M23, matrix.M24,
            matrix.M31, matrix.M32, matrix.M33, matrix.M34,
            matrix.M41, matrix.M42, matrix.M43, matrix.M44,
            opacity,
            isVisible ? L"true" : L"false",
            isHitTestVisible ? L"true" : L"false",
            isPixelSnappingEnabled ? L"true" : L"false",
            compositeMode, borderMode, backfaceVisibility,
            hasClip ? L"true" : L"false", clipType,
            relOffsetX, relOffsetY, relOffsetZ,
            relSizeX, relSizeY);

        // Type-specific properties. Ends right after the final closing quote (no trailing
        // space, no '>') so the caller can append either ">" or " />".
        n += swprintf_s(line + n, cap - n,
            L"wucChildCount=\"%d\" shapeCount=\"%d\" hasBrush=\"%s\" brushType=\"%s\" "
            L"hasShadow=\"%s\" shadowType=\"%s\" hasEffect=\"%s\" hasViewBox=\"%s\" sourceVisual=\"0x%p\"",
            wucChildCount, shapeCount,
            hasBrush ? L"true" : L"false", brushType,
            hasShadow ? L"true" : L"false", shadowType,
            hasEffect ? L"true" : L"false",
            hasViewBox ? L"true" : L"false",
            pSourceVisualRaw);

        *pN = n;
    }

    // Recursively emits the pure WUC visual tree rooted at pVisual: one <WucVisual> element
    // for pVisual itself (with the full property set), then a nested <WucVisual> for each of
    // its children. A ContainerVisual can contain other ContainerVisuals (incl. the
    // Sprite/Shape/Layer/Redirect leaf types, which all derive from ContainerVisual), so we
    // walk Children -> IIterable -> IIterator and recurse into each child.
    void DumpVisualTreeXml(
        _In_opt_ WUComp::IVisual* pVisual,
        _In_ int indentLevel,
        _In_ int depth,
        _In_ int maxDepth)
    {
        if (pVisual == nullptr || depth > maxDepth)
        {
            return;
        }

        // Indent string (2 spaces per level), matching the <CompNode> pretty-printing.
        WCHAR indent[256] = {0};
        for (int i = 0; i < indentLevel && i < 60; i++)
        {
            wcscat_s(indent, _countof(indent), L"  ");
        }

        const WCHAR* type = GetWucVisualTypeName(pVisual);
        const int childCount = GetWucVisualChildCount(pVisual);
        const bool willRecurse = (childCount > 0) && (depth < maxDepth);

        WCHAR line[4096];
        int n = 0;
        n += swprintf_s(line + n, _countof(line) - n,
            L"%s<WucVisual type=\"%s\" wucVisual=\"0x%p\" ",
            indent,
            type,
            reinterpret_cast<void*>(pVisual));
        AppendVisualPropertyAttributes(pVisual, line, &n, _countof(line));
        n += swprintf_s(line + n, _countof(line) - n,
            L"%s>\n",
            willRecurse ? L"" : L" /");
        OutputDebugStringW(line);

        if (willRecurse)
        {
            xref_ptr<WUComp::IContainerVisual> spContainer;
            if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(spContainer.ReleaseAndGetAddressOf()))) && spContainer != nullptr)
            {
                xref_ptr<WUComp::IVisualCollection> spChildren;
                if (SUCCEEDED(spContainer->get_Children(spChildren.ReleaseAndGetAddressOf())) && spChildren != nullptr)
                {
                    xref_ptr<wfc::IIterable<WUComp::Visual*>> spIterable;
                    if (SUCCEEDED(spChildren->QueryInterface(IID_PPV_ARGS(spIterable.ReleaseAndGetAddressOf()))) && spIterable != nullptr)
                    {
                        xref_ptr<wfc::IIterator<WUComp::Visual*>> spIterator;
                        if (SUCCEEDED(spIterable->First(spIterator.ReleaseAndGetAddressOf())) && spIterator != nullptr)
                        {
                            boolean hasCurrent = FALSE;
                            if (SUCCEEDED(spIterator->get_HasCurrent(&hasCurrent)))
                            {
                                while (hasCurrent)
                                {
                                    xref_ptr<WUComp::IVisual> spChild;
                                    if (SUCCEEDED(spIterator->get_Current(spChild.ReleaseAndGetAddressOf())) && spChild != nullptr)
                                    {
                                        DumpVisualTreeXml(spChild.get(), indentLevel + 1, depth + 1, maxDepth);
                                    }
                                    if (FAILED(spIterator->MoveNext(&hasCurrent)))
                                    {
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            WCHAR closeLine[300];
            swprintf_s(closeLine, _countof(closeLine), L"%s</WucVisual>\n", indent);
            OutputDebugStringW(closeLine);
        }
    }
}

// =========================================================================
// Tree Navigation
// =========================================================================

HWCompTreeNode*
DCompTreeHelper::GetParent(_In_ HWCompNode* pCompNode)
{
    if (pCompNode == nullptr)
    {
        return nullptr;
    }

    CDependencyObject* pParent = pCompNode->GetParentInternal(false /*publicParentOnly*/);
    
    if (pParent != nullptr && pParent->OfTypeByIndex<KnownTypeIndex::HWCompTreeNode>())
    {
        return static_cast<HWCompTreeNode*>(pParent);
    }
    
    return nullptr;
}

HWCompNode*
DCompTreeHelper::GetChild(_In_ HWCompTreeNode* pCompNode, _In_ int index)
{
    if (pCompNode == nullptr || index < 0)
    {
        return nullptr;
    }

    int currentIndex = 0;
    for (auto it = pCompNode->GetChildrenBegin(); it != pCompNode->GetChildrenEnd(); ++it)
    {
        if (currentIndex == index)
        {
            return *it;
        }
        currentIndex++;
    }
    
    return nullptr; // Index out of range
}

int
DCompTreeHelper::GetChildrenCount(_In_ HWCompTreeNode* pCompNode)
{
    if (pCompNode == nullptr)
    {
        return 0;
    }

    int count = 0;
    for (auto it = pCompNode->GetChildrenBegin(); it != pCompNode->GetChildrenEnd(); ++it)
    {
        count++;
    }
    return count;
}

// =========================================================================
// XAML <-> DComp Bridge
// =========================================================================

CUIElement*
DCompTreeHelper::GetUIElement(_In_ HWCompTreeNode* pCompNode)
{
    if (pCompNode == nullptr)
    {
        return nullptr;
    }
    return pCompNode->GetUIElementPeer();
}

HWCompTreeNode*
DCompTreeHelper::GetCompNode(_In_ CUIElement* pUIElement)
{
    if (pUIElement == nullptr)
    {
        return nullptr;
    }
    return pUIElement->GetCompositionPeer();
}

// =========================================================================
// Debug / Diagnostic Methods
// =========================================================================

void
DCompTreeHelper::DumpMainTree(_In_ CCoreServices* pCore)
{
    if (pCore == nullptr)
    {
        OutputDebugStringW(L"[DCompTreeHelper] DumpMainTree: CCoreServices is null\n");
        return;
    }

    OutputDebugStringW(L"\n");
    OutputDebugStringW(L"================================================================================\n");
    OutputDebugStringW(L"                         DCOMP TREE DUMP                                       \n");
    OutputDebugStringW(L"                    (Composition Visual Tree)                                  \n");
    OutputDebugStringW(L"================================================================================\n");
    OutputDebugStringW(L"\n");

    // Get the main visual tree
    VisualTree* pVisualTree = pCore->GetMainVisualTree();
    if (pVisualTree == nullptr)
    {
        OutputDebugStringW(L"[DCompTreeHelper] No main visual tree found\n");
        return;
    }

    // Get the root element
    CUIElement* pRootElement = pVisualTree->GetRootElementNoRef();
    if (pRootElement == nullptr)
    {
        OutputDebugStringW(L"[DCompTreeHelper] No root element in visual tree\n");
        return;
    }

    // Get the composition peer
    HWCompTreeNode* pRootCompNode = pRootElement->GetCompositionPeer();
    if (pRootCompNode == nullptr)
    {
        OutputDebugStringW(L"[DCompTreeHelper] Root element has no composition peer\n");
        OutputDebugStringW(L"[DCompTreeHelper] (Element may not have been rendered yet)\n");
        return;
    }

    // Now dump the tree
    DumpTreeRecursive(pRootCompNode, 0, 50, 0);

    OutputDebugStringW(L"\n");
    OutputDebugStringW(L"================================================================================\n");
    OutputDebugStringW(L"\n");
}

void
DCompTreeHelper::DumpMainTree()
{
    // Grab the current core ourselves so callers (e.g. the Immediate Window)
    // don't need a CCoreServices pointer in scope.
    if (!DirectUI::DXamlServices::IsDXamlCoreInitialized())
    {
        OutputDebugStringW(L"[DCompTreeHelper] DumpMainTree: DXaml core not initialized\n");
        return;
    }

    CCoreServices* pCore = DirectUI::DXamlServices::GetSafeHandle();
    DumpMainTree(pCore);
}

void
DCompTreeHelper::DumpTree(_In_ HWCompTreeNode* pRootCompNode, _In_ int maxDepth)
{
    if (pRootCompNode == nullptr)
    {
        OutputDebugStringW(L"[DCompTreeHelper] DumpTree: Root node is null\n");
        return;
    }

    OutputDebugStringW(L"\n");
    OutputDebugStringW(L"================ DCOMP TREE ================\n");
    
    DumpTreeRecursive(pRootCompNode, 0, maxDepth, 0);
    
    OutputDebugStringW(L"============================================\n\n");
}

void
DCompTreeHelper::DumpTreeForElement(_In_ CUIElement* pUIElement, _In_ int maxDepth)
{
    if (pUIElement == nullptr)
    {
        OutputDebugStringW(L"[DCompTreeHelper] DumpTreeForElement: UIElement is null\n");
        return;
    }

    HWCompTreeNode* pCompNode = pUIElement->GetCompositionPeer();
    if (pCompNode == nullptr)
    {
        OutputDebugStringW(L"[DCompTreeHelper] Element has no composition peer (not rendered yet?)\n");
        return;
    }

    DumpTree(pCompNode, maxDepth);
}

void
DCompTreeHelper::GetNodeDescription(
    _In_ HWCompNode* pCompNode,
    _Out_writes_(bufferSize) WCHAR* buffer,
    _In_ size_t bufferSize)
{
    if (pCompNode == nullptr)
    {
        swprintf_s(buffer, bufferSize, L"<null>");
        return;
    }

    // Get type name
    const WCHAR* typeName = L"HWCompNode";
    KnownTypeIndex typeIndex = pCompNode->GetTypeIndex();
    
    if (typeIndex == KnownTypeIndex::HWCompTreeNode)
    {
        typeName = L"TreeNode";
    }
    else if (typeIndex == KnownTypeIndex::HWCompLeafNode)
    {
        typeName = L"LeafNode";
    }
    else if (typeIndex == KnownTypeIndex::HWCompRenderDataNode)
    {
        typeName = L"RenderData";
    }
    else if (typeIndex == KnownTypeIndex::HWCompMediaNode)
    {
        typeName = L"MediaNode";
    }

    // Check if it's a tree node and get UIElement info
    if (pCompNode->OfTypeByIndex<KnownTypeIndex::HWCompTreeNode>())
    {
        HWCompTreeNode* pTreeNode = static_cast<HWCompTreeNode*>(pCompNode);
        CUIElement* pUIElement = pTreeNode->GetUIElementPeer();
        int childCount = GetChildrenCount(pTreeNode);
        
        if (pUIElement != nullptr)
        {
            xstring_ptr elementTypeName = pUIElement->GetClassName();
            const WCHAR* elemName = elementTypeName.GetBuffer();
            if (elemName == nullptr) elemName = L"?";
            
            // Check for WUC visual
            xref_ptr<WUComp::IVisual> spVisual = pCompNode->GetWUCVisual();
            const WCHAR* hasVisual = spVisual ? L" [WUC]" : L"";
            
            swprintf_s(buffer, bufferSize, 
                L"%s [%s] (children:%d)%s",
                typeName, elemName, childCount, hasVisual);
        }
        else
        {
            swprintf_s(buffer, bufferSize, 
                L"%s [no UIElement] (children:%d)",
                typeName, childCount);
        }
    }
    else
    {
        swprintf_s(buffer, bufferSize, L"%s (leaf)", typeName);
    }
}

void
DCompTreeHelper::DumpTreeRecursive(
    _In_ HWCompNode* pCompNode,
    _In_ int currentDepth,
    _In_ int maxDepth,
    _In_ int indentLevel)
{
    if (pCompNode == nullptr || currentDepth > maxDepth)
    {
        return;
    }

    // Build indent string
    WCHAR indent[256] = {0};
    for (int i = 0; i < indentLevel && i < 60; i++)
    {
        wcscat_s(indent, _countof(indent), L"  ");
    }

    // Get node description
    WCHAR nodeDesc[512];
    GetNodeDescription(pCompNode, nodeDesc, _countof(nodeDesc));

    // Build and output the line
    WCHAR outputLine[1024];
    if (currentDepth == 0)
    {
        swprintf_s(outputLine, _countof(outputLine), L"%s%s\n", indent, nodeDesc);
    }
    else
    {
        swprintf_s(outputLine, _countof(outputLine), L"%s+-- %s\n", indent, nodeDesc);
    }
    OutputDebugStringW(outputLine);

    // If it's a tree node, recurse into children
    if (pCompNode->OfTypeByIndex<KnownTypeIndex::HWCompTreeNode>())
    {
        HWCompTreeNode* pTreeNode = static_cast<HWCompTreeNode*>(pCompNode);
        
        for (auto it = pTreeNode->GetChildrenBegin(); it != pTreeNode->GetChildrenEnd(); ++it)
        {
            DumpTreeRecursive(*it, currentDepth + 1, maxDepth, indentLevel + 1);
        }
    }
}

void
DCompTreeHelper::DumpTreeRecursiveXml(
    _In_ HWCompNode* pCompNode,
    _In_ int currentDepth,
    _In_ int maxDepth,
    _In_ int indentLevel)
{
    if (pCompNode == nullptr || currentDepth > maxDepth)
    {
        return;
    }

    // Build indent string (2 spaces per level) for pretty-printed XML.
    WCHAR indent[256] = {0};
    for (int i = 0; i < indentLevel && i < 60; i++)
    {
        wcscat_s(indent, _countof(indent), L"  ");
    }

    // Gather node attributes.
    const WCHAR* typeName = L"HWCompNode";
    KnownTypeIndex typeIndex = pCompNode->GetTypeIndex();
    if (typeIndex == KnownTypeIndex::HWCompTreeNode)        typeName = L"TreeNode";
    else if (typeIndex == KnownTypeIndex::HWCompLeafNode)   typeName = L"LeafNode";
    else if (typeIndex == KnownTypeIndex::HWCompRenderDataNode) typeName = L"RenderData";
    else if (typeIndex == KnownTypeIndex::HWCompMediaNode)  typeName = L"MediaNode";

    const bool isTreeNode = pCompNode->OfTypeByIndex<KnownTypeIndex::HWCompTreeNode>();

    WCHAR elementName[256] = L"";
    WCHAR xName[256] = L"";               // x:Name of the element, if it has one
    int childCount = 0;
    bool hasVisual = false;
    CUIElement* pUIElement = nullptr;     // address of the owning XAML element (if any)
    void* pVisualRaw = nullptr;           // address of the underlying WUC visual (if any)
    xref_ptr<WUComp::IVisual> spVisual;    // the node's WUC visual (spine container), hoisted so the WUC child walk below can reuse it

    // WUC visual render properties (filled in when the node has a visual).
    float offsetX = 0, offsetY = 0, offsetZ = 0;
    float sizeW = 0, sizeH = 0;
    float opacity = 1.0f;
    bool isVisible = true;

    // Concrete WUC visual type (discovered via QI) plus its type-specific properties.
    WCHAR visualType[64] = L"Visual";
    int wucChildCount = -1;               // -1 = node has no WUC container visual
    int shapeCount = -1;                  // ShapeVisual only
    bool hasBrush = false, hasShadow = false, hasEffect = false, hasViewBox = false, hasClip = false;
    WCHAR brushType[128] = L"";           // kind of Brush  (SpriteVisual)
    WCHAR shadowType[128] = L"";          // kind of Shadow (Sprite/Layer)
    WCHAR clipType[128] = L"";            // kind of Clip   (any Visual)
    void* pSourceVisualRaw = nullptr;     // RedirectVisual.Source

    // Extended base Visual transform properties (IVisual / IVisual2 / IVisual3 / IVisual4).
    float anchorX = 0, anchorY = 0;
    float centerX = 0, centerY = 0, centerZ = 0;
    float scaleX = 1, scaleY = 1, scaleZ = 1;
    float rotationDegrees = 0;
    float rotationAxisX = 0, rotationAxisY = 0, rotationAxisZ = 1;
    float orientX = 0, orientY = 0, orientZ = 0, orientW = 1;
    wfn::Matrix4x4 matrix = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
    float relOffsetX = 0, relOffsetY = 0, relOffsetZ = 0;
    float relSizeX = 0, relSizeY = 0;
    const WCHAR* compositeMode = L"Inherit";
    const WCHAR* borderMode = L"Inherit";
    const WCHAR* backfaceVisibility = L"Inherit";
    bool isHitTestVisible = true;
    bool isPixelSnappingEnabled = false;

    if (isTreeNode)
    {
        HWCompTreeNode* pTreeNode = static_cast<HWCompTreeNode*>(pCompNode);
        pUIElement = pTreeNode->GetUIElementPeer();
        childCount = GetChildrenCount(pTreeNode);

        spVisual = pCompNode->GetWUCVisual();
        hasVisual = (spVisual != nullptr);
        pVisualRaw = spVisual.get();

        if (spVisual != nullptr)
        {
            // Read the base composition render properties off the WUC visual.
            wfn::Vector3 offset = {};
            wfn::Vector2 size = {};
            boolean visible = TRUE;
            if (SUCCEEDED(spVisual->get_Offset(&offset))) { offsetX = offset.X; offsetY = offset.Y; offsetZ = offset.Z; }
            if (SUCCEEDED(spVisual->get_Size(&size)))     { sizeW = size.X; sizeH = size.Y; }
            spVisual->get_Opacity(&opacity);
            if (SUCCEEDED(spVisual->get_IsVisible(&visible))) { isVisible = !!visible; }

            // Extended transform properties (still on the base IVisual).
            wfn::Vector2 anchor = {};
            if (SUCCEEDED(spVisual->get_AnchorPoint(&anchor))) { anchorX = anchor.X; anchorY = anchor.Y; }
            wfn::Vector3 center = {};
            if (SUCCEEDED(spVisual->get_CenterPoint(&center))) { centerX = center.X; centerY = center.Y; centerZ = center.Z; }
            wfn::Vector3 scale = {};
            if (SUCCEEDED(spVisual->get_Scale(&scale))) { scaleX = scale.X; scaleY = scale.Y; scaleZ = scale.Z; }
            spVisual->get_RotationAngleInDegrees(&rotationDegrees);
            wfn::Vector3 axis = {};
            if (SUCCEEDED(spVisual->get_RotationAxis(&axis))) { rotationAxisX = axis.X; rotationAxisY = axis.Y; rotationAxisZ = axis.Z; }
            wfn::Quaternion orient = {};
            if (SUCCEEDED(spVisual->get_Orientation(&orient))) { orientX = orient.X; orientY = orient.Y; orientZ = orient.Z; orientW = orient.W; }
            spVisual->get_TransformMatrix(&matrix);

            WUComp::CompositionCompositeMode compMode = {};
            if (SUCCEEDED(spVisual->get_CompositeMode(&compMode))) { compositeMode = CompositeModeName(compMode); }
            WUComp::CompositionBorderMode bMode = {};
            if (SUCCEEDED(spVisual->get_BorderMode(&bMode))) { borderMode = BorderModeName(bMode); }
            WUComp::CompositionBackfaceVisibility bfVis = {};
            if (SUCCEEDED(spVisual->get_BackfaceVisibility(&bfVis))) { backfaceVisibility = BackfaceVisibilityName(bfVis); }

            xref_ptr<WUComp::ICompositionClip> spClip;
            if (SUCCEEDED(spVisual->get_Clip(spClip.ReleaseAndGetAddressOf())) && spClip != nullptr)
            {
                hasClip = true;
                GetWucClassName(spClip.get(), clipType, _countof(clipType));
            }

            // Versioned base interfaces (IVisual2/3/4) - present on newer builds only.
            xref_ptr<WUComp::IVisual2> spVisual2;
            if (SUCCEEDED(spVisual->QueryInterface(IID_PPV_ARGS(spVisual2.ReleaseAndGetAddressOf()))))
            {
                wfn::Vector3 relOffset = {};
                if (SUCCEEDED(spVisual2->get_RelativeOffsetAdjustment(&relOffset))) { relOffsetX = relOffset.X; relOffsetY = relOffset.Y; relOffsetZ = relOffset.Z; }
                wfn::Vector2 relSize = {};
                if (SUCCEEDED(spVisual2->get_RelativeSizeAdjustment(&relSize))) { relSizeX = relSize.X; relSizeY = relSize.Y; }
            }
            xref_ptr<WUComp::IVisual3> spVisual3;
            if (SUCCEEDED(spVisual->QueryInterface(IID_PPV_ARGS(spVisual3.ReleaseAndGetAddressOf()))))
            {
                boolean htv = TRUE;
                if (SUCCEEDED(spVisual3->get_IsHitTestVisible(&htv))) { isHitTestVisible = !!htv; }
            }
            xref_ptr<WUComp::IVisual4> spVisual4;
            if (SUCCEEDED(spVisual->QueryInterface(IID_PPV_ARGS(spVisual4.ReleaseAndGetAddressOf()))))
            {
                boolean pse = FALSE;
                if (SUCCEEDED(spVisual4->get_IsPixelSnappingEnabled(&pse))) { isPixelSnappingEnabled = !!pse; }
            }

            // Concrete type detection via QI ladder (most-derived first, since the four
            // leaf types all derive from ContainerVisual and would match IContainerVisual too).
            xref_ptr<WUComp::ISpriteVisual> spSprite;
            xref_ptr<WUComp::ILayerVisual> spLayer;
            xref_ptr<WUComp::IShapeVisual> spShape;
            xref_ptr<WUComp::IRedirectVisual> spRedirect;
            xref_ptr<WUComp::IContainerVisual> spContainer;

            if (SUCCEEDED(spVisual->QueryInterface(IID_PPV_ARGS(spSprite.ReleaseAndGetAddressOf()))))
            {
                wcscpy_s(visualType, _countof(visualType), L"SpriteVisual");
                xref_ptr<WUComp::ICompositionBrush> spBrush;
                if (SUCCEEDED(spSprite->get_Brush(spBrush.ReleaseAndGetAddressOf())) && spBrush != nullptr)
                {
                    hasBrush = true;
                    GetWucClassName(spBrush.get(), brushType, _countof(brushType));
                }
                xref_ptr<WUComp::ISpriteVisual2> spSprite2;
                if (SUCCEEDED(spSprite->QueryInterface(IID_PPV_ARGS(spSprite2.ReleaseAndGetAddressOf()))))
                {
                    xref_ptr<WUComp::ICompositionShadow> spShadow;
                    if (SUCCEEDED(spSprite2->get_Shadow(spShadow.ReleaseAndGetAddressOf())) && spShadow != nullptr)
                    {
                        hasShadow = true;
                        GetWucClassName(spShadow.get(), shadowType, _countof(shadowType));
                    }
                }
            }
            else if (SUCCEEDED(spVisual->QueryInterface(IID_PPV_ARGS(spLayer.ReleaseAndGetAddressOf()))))
            {
                wcscpy_s(visualType, _countof(visualType), L"LayerVisual");
                xref_ptr<WUComp::ICompositionEffectBrush> spEffect;
                if (SUCCEEDED(spLayer->get_Effect(spEffect.ReleaseAndGetAddressOf())) && spEffect != nullptr)
                {
                    hasEffect = true;
                }
                xref_ptr<WUComp::ILayerVisual2> spLayer2;
                if (SUCCEEDED(spLayer->QueryInterface(IID_PPV_ARGS(spLayer2.ReleaseAndGetAddressOf()))))
                {
                    xref_ptr<WUComp::ICompositionShadow> spShadow;
                    if (SUCCEEDED(spLayer2->get_Shadow(spShadow.ReleaseAndGetAddressOf())) && spShadow != nullptr)
                    {
                        hasShadow = true;
                        GetWucClassName(spShadow.get(), shadowType, _countof(shadowType));
                    }
                }
            }
            else if (SUCCEEDED(spVisual->QueryInterface(IID_PPV_ARGS(spShape.ReleaseAndGetAddressOf()))))
            {
                wcscpy_s(visualType, _countof(visualType), L"ShapeVisual");
                xref_ptr<wfc::IVector<WUComp::CompositionShape*>> spShapes;
                if (SUCCEEDED(spShape->get_Shapes(spShapes.ReleaseAndGetAddressOf())) && spShapes != nullptr)
                {
                    unsigned int count = 0;
                    if (SUCCEEDED(spShapes->get_Size(&count))) { shapeCount = static_cast<int>(count); }
                }
                xref_ptr<WUComp::ICompositionViewBox> spViewBox;
                if (SUCCEEDED(spShape->get_ViewBox(spViewBox.ReleaseAndGetAddressOf())) && spViewBox != nullptr)
                {
                    hasViewBox = true;
                }
            }
            else if (SUCCEEDED(spVisual->QueryInterface(IID_PPV_ARGS(spRedirect.ReleaseAndGetAddressOf()))))
            {
                wcscpy_s(visualType, _countof(visualType), L"RedirectVisual");
                xref_ptr<WUComp::IVisual> spSource;
                if (SUCCEEDED(spRedirect->get_Source(spSource.ReleaseAndGetAddressOf())))
                {
                    pSourceVisualRaw = spSource.get();
                }
            }
            else if (SUCCEEDED(spVisual->QueryInterface(IID_PPV_ARGS(spContainer.ReleaseAndGetAddressOf()))))
            {
                wcscpy_s(visualType, _countof(visualType), L"ContainerVisual");
            }

            // WUC child count for any container-derived visual (incl. the leaf types,
            // which inherit Children from ContainerVisual).
            if (spContainer == nullptr)
            {
                spVisual->QueryInterface(IID_PPV_ARGS(spContainer.ReleaseAndGetAddressOf()));
            }
            if (spContainer != nullptr)
            {
                xref_ptr<WUComp::IVisualCollection> spChildren;
                if (SUCCEEDED(spContainer->get_Children(spChildren.ReleaseAndGetAddressOf())) && spChildren != nullptr)
                {
                    int count = 0;
                    if (SUCCEEDED(spChildren->get_Count(&count))) { wucChildCount = count; }
                }
            }
        }

        if (pUIElement != nullptr)
        {
            xstring_ptr elementTypeName = pUIElement->GetClassName();
            const WCHAR* elemName = elementTypeName.GetBuffer();
            if (elemName != nullptr)
            {
                wcscpy_s(elementName, _countof(elementName), elemName);
            }

            // x:Name (the developer-assigned name), read from the backing field.
            if (!pUIElement->m_strName.IsNullOrEmpty())
            {
                const WCHAR* pName = pUIElement->m_strName.GetBuffer();
                if (pName != nullptr)
                {
                    wcscpy_s(xName, _countof(xName), pName);
                }
            }
        }
    }

    // Emit the opening tag. Self-close when there are no children to recurse.
    // Addresses are emitted as hex so you can correlate nodes across snapshots
    // and look them up in the debugger. Render properties (offset/size/opacity/
    // visibility/transform) come straight off the underlying WUC visual, and the
    // concrete WUC type (SpriteVisual/ShapeVisual/etc.) is discovered via QI above.
    const bool willRecurse = isTreeNode && childCount > 0;

    WCHAR line[4096];
    int n = 0;

    // Identity + tree structure + concrete visual type.
    n += swprintf_s(line + n, _countof(line) - n,
        L"%s<CompNode type=\"%s\" element=\"%s\" name=\"%s\" children=\"%d\" hasVisual=\"%s\" visualType=\"%s\" "
        L"compNode=\"0x%p\" uiElement=\"0x%p\" wucVisual=\"0x%p\" ",
        indent,
        typeName,
        elementName[0] != L'\0' ? elementName : L"",
        xName,
        childCount,
        hasVisual ? L"true" : L"false",
        visualType,
        reinterpret_cast<void*>(pCompNode),
        reinterpret_cast<void*>(pUIElement),
        pVisualRaw);

    // Base Visual transform / render properties.
    n += swprintf_s(line + n, _countof(line) - n,
        L"offsetX=\"%.1f\" offsetY=\"%.1f\" offsetZ=\"%.1f\" width=\"%.1f\" height=\"%.1f\" "
        L"anchorPointX=\"%.2f\" anchorPointY=\"%.2f\" "
        L"centerPointX=\"%.2f\" centerPointY=\"%.2f\" centerPointZ=\"%.2f\" "
        L"scaleX=\"%.3f\" scaleY=\"%.3f\" scaleZ=\"%.3f\" "
        L"rotationAngleInDegrees=\"%.2f\" rotationAxisX=\"%.2f\" rotationAxisY=\"%.2f\" rotationAxisZ=\"%.2f\" "
        L"orientationX=\"%.3f\" orientationY=\"%.3f\" orientationZ=\"%.3f\" orientationW=\"%.3f\" ",
        offsetX, offsetY, offsetZ, sizeW, sizeH,
        anchorX, anchorY,
        centerX, centerY, centerZ,
        scaleX, scaleY, scaleZ,
        rotationDegrees, rotationAxisX, rotationAxisY, rotationAxisZ,
        orientX, orientY, orientZ, orientW);

    // Transform matrix + modes (readable enum names) + relative adjustments + flags.
    n += swprintf_s(line + n, _countof(line) - n,
        L"transformMatrix=\"%.3f,%.3f,%.3f,%.3f, %.3f,%.3f,%.3f,%.3f, %.3f,%.3f,%.3f,%.3f, %.3f,%.3f,%.3f,%.3f\" "
        L"opacity=\"%.3f\" isVisible=\"%s\" isHitTestVisible=\"%s\" isPixelSnappingEnabled=\"%s\" "
        L"compositeMode=\"%s\" borderMode=\"%s\" backfaceVisibility=\"%s\" hasClip=\"%s\" clipType=\"%s\" "
        L"relativeOffsetAdjustmentX=\"%.2f\" relativeOffsetAdjustmentY=\"%.2f\" relativeOffsetAdjustmentZ=\"%.2f\" "
        L"relativeSizeAdjustmentX=\"%.2f\" relativeSizeAdjustmentY=\"%.2f\" ",
        matrix.M11, matrix.M12, matrix.M13, matrix.M14,
        matrix.M21, matrix.M22, matrix.M23, matrix.M24,
        matrix.M31, matrix.M32, matrix.M33, matrix.M34,
        matrix.M41, matrix.M42, matrix.M43, matrix.M44,
        opacity,
        isVisible ? L"true" : L"false",
        isHitTestVisible ? L"true" : L"false",
        isPixelSnappingEnabled ? L"true" : L"false",
        compositeMode, borderMode, backfaceVisibility,
        hasClip ? L"true" : L"false", clipType,
        relOffsetX, relOffsetY, relOffsetZ,
        relSizeX, relSizeY);

    // Type-specific properties (only meaningful for the matching visualType).
    n += swprintf_s(line + n, _countof(line) - n,
        L"wucChildCount=\"%d\" shapeCount=\"%d\" hasBrush=\"%s\" brushType=\"%s\" "
        L"hasShadow=\"%s\" shadowType=\"%s\" hasEffect=\"%s\" hasViewBox=\"%s\" sourceVisual=\"0x%p\"%s>\n",
        wucChildCount, shapeCount,
        hasBrush ? L"true" : L"false", brushType,
        hasShadow ? L"true" : L"false", shadowType,
        hasEffect ? L"true" : L"false",
        hasViewBox ? L"true" : L"false",
        pSourceVisualRaw,
        willRecurse ? L"" : L" /");   // self-closing tag when no children
    OutputDebugStringW(line);

    if (willRecurse)
    {
        HWCompTreeNode* pTreeNode = static_cast<HWCompTreeNode*>(pCompNode);
        for (auto it = pTreeNode->GetChildrenBegin(); it != pTreeNode->GetChildrenEnd(); ++it)
        {
            DumpTreeRecursiveXml(*it, currentDepth + 1, maxDepth, indentLevel + 1);
        }

        // Closing tag.
        WCHAR closeLine[300];
        swprintf_s(closeLine, _countof(closeLine), L"%s</CompNode>\n", indent);
        OutputDebugStringW(closeLine);
    }
}

// =========================================================================
// Auto Snapshot (throttled full-tree dump)
// =========================================================================

// Controls whether the automatic throttled snapshot runs. On by default in DBG.
#if DBG
static bool g_dcompAutoSnapshotEnabled = true;
#else
static bool g_dcompAutoSnapshotEnabled = false;
#endif

// Minimum milliseconds between automatic snapshots, so the output stays readable
// instead of dumping the whole tree every single frame.
static const ULONGLONG c_dcompSnapshotThrottleMs = 2500;

void
DCompTreeHelper::EnableAutoSnapshot(_In_ bool enable)
{
    g_dcompAutoSnapshotEnabled = enable;
}

bool
DCompTreeHelper::IsAutoSnapshotEnabled()
{
    return g_dcompAutoSnapshotEnabled;
}

void
DCompTreeHelper::MaybeDumpTreeSnapshot(_In_ HWCompTreeNode* pRootCompNode)
{
    if (!g_dcompAutoSnapshotEnabled || pRootCompNode == nullptr)
    {
        return;
    }

    // Throttle: only dump if enough time has passed since the last snapshot.
    static ULONGLONG s_lastDumpTime = 0;
    const ULONGLONG now = GetTickCount64();
    if (s_lastDumpTime != 0 && (now - s_lastDumpTime) < c_dcompSnapshotThrottleMs)
    {
        return;
    }
    s_lastDumpTime = now;

    // Emit the snapshot as XML. The <DCompTreeSnapshot> / </DCompTreeSnapshot>
    // markers let the viewer script detect a complete tree and write it to a .xml file.
    // Two sibling trees are emitted inside the snapshot:
    //   <CompTree>   - the sparse HWCompNode tree (as before).
    //   <VisualTree> - the pure WUC visual tree, rooted at the root comp node's container
    //                  visual and walked recursively through each visual's Children.
    OutputDebugStringW(L"<DCompTreeSnapshot>\n");

    OutputDebugStringW(L"<CompTree>\n");
    DumpTreeRecursiveXml(pRootCompNode, 0, 50, 2);
    OutputDebugStringW(L"</CompTree>\n");

    OutputDebugStringW(L"<VisualTree>\n");
    xref_ptr<WUComp::IVisual> spRootVisual = pRootCompNode->GetWUCVisual();
    DumpVisualTreeXml(spRootVisual.get(), 2 /*indentLevel*/, 0 /*depth*/, 256 /*maxDepth*/);
    OutputDebugStringW(L"</VisualTree>\n");

    OutputDebugStringW(L"</DCompTreeSnapshot>\n");
}

// =========================================================================
// Live Logging (Poor Man's Live Visual Tree)
// =========================================================================

// Global flag controlling whether per-change comp tree mutations are logged.
// Defaults OFF - the per-mutation stream is noisy and hard to read as a tree.
// The throttled auto-snapshot (below) is the preferred way to see the tree.
// Toggle at runtime from the Immediate Window: DCompTreeHelper::EnableLiveLogging(true)
static bool g_dcompLiveLoggingEnabled = false;

void
DCompTreeHelper::EnableLiveLogging(_In_ bool enable)
{
    g_dcompLiveLoggingEnabled = enable;
    if (enable)
    {
        OutputDebugStringW(L"[DComp] Live logging ENABLED - comp tree changes will be printed here\n");
    }
    else
    {
        OutputDebugStringW(L"[DComp] Live logging DISABLED\n");
    }
}

bool
DCompTreeHelper::IsLiveLoggingEnabled()
{
    return g_dcompLiveLoggingEnabled;
}

void
DCompTreeHelper::LogCompNodeInserted(_In_ HWCompNode* pParent, _In_ HWCompNode* pChild)
{
    if (!g_dcompLiveLoggingEnabled)
    {
        return;
    }

    WCHAR parentDesc[512] = L"<null>";
    WCHAR childDesc[512] = L"<null>";

    if (pParent != nullptr)
    {
        GetNodeDescription(pParent, parentDesc, _countof(parentDesc));
    }
    if (pChild != nullptr)
    {
        GetNodeDescription(pChild, childDesc, _countof(childDesc));
    }

    WCHAR line[1100];
    swprintf_s(line, _countof(line), L"[DComp +] ADD    %s  -->  under  %s\n", childDesc, parentDesc);
    OutputDebugStringW(line);
}

void
DCompTreeHelper::LogCompNodeRemoved(_In_ HWCompNode* pParent, _In_ HWCompNode* pChild)
{
    if (!g_dcompLiveLoggingEnabled)
    {
        return;
    }

    WCHAR parentDesc[512] = L"<null>";
    WCHAR childDesc[512] = L"<null>";

    if (pParent != nullptr)
    {
        GetNodeDescription(pParent, parentDesc, _countof(parentDesc));
    }
    if (pChild != nullptr)
    {
        GetNodeDescription(pChild, childDesc, _countof(childDesc));
    }

    WCHAR line[1100];
    swprintf_s(line, _countof(line), L"[DComp -] REMOVE %s  -->  from   %s\n", childDesc, parentDesc);
    OutputDebugStringW(line);
}
