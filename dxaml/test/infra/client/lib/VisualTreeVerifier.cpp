// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <XamlTailored.h>

#include "VisualTreeVerifier.h"
#include "VisualDebugTags.h"
#include "IXamlTestHooks-win.h"
#include <windowscollections.h>

using namespace WEX::Common;
using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Private::Infrastructure {

// Note: Manually keep this in sync with the enum class in VisualDebugTags.h.
// Used by test code to convert the enum value back to a string.
static const wchar_t* VisualDebugTagsNames[] =
{
    L"WindowedPopup_ContentIslandRootVisual",
    L"WindowedPopup_DebugVisual",
    L"WindowedPopup_AnimationRootVisual",
    L"WindowedPopup_SystemBackdropPlacementVisual",
    L"WindowedPopup_PublicRootVisual",
    L"CompNode_PrependVisual",
    L"CompNode_PrimaryVisual",
    L"CompNode_ContentVisual",
    L"CompNode_RoundedCornerClipVisual",
    L"CompNode_DropShadowVisual",
};

HRESULT VisualTreeVerifierStatics::CreateFromElement(_In_ xaml::IUIElement* element, _Outptr_ test_infra::IVisualTreeVerifier** value)
{
    wrl::ComPtr<wfci_::Vector<IInspectable*>> vector;
    LogThrow_IfFailed(wfci_::Vector<IInspectable*>::Make(&vector));

    wrl::ComPtr<wfc::IVector<IInspectable*>> visuals;
    LogThrow_IfFailed(vector.As(&visuals));

    wrl::ComPtr<xaml::IDependencyObject> dependencyObject;
    LogThrow_IfFailed(element->QueryInterface(IID_PPV_ARGS(&dependencyObject)));

    wrl::ComPtr<msy::IDispatcherQueue> dispatcherQueue;
    LogThrow_IfFailed(dependencyObject->get_DispatcherQueue(&dispatcherQueue));

    wrl::ComPtr<msy::IDispatcherQueue2> dq2;
    LogThrow_IfFailed(dispatcherQueue.As(&dq2));

    boolean hasThreadAccess = false;
    LogThrow_IfFailed(dq2->get_HasThreadAccess(&hasThreadAccess));
    WEX::Common::Throw::IfFalse(hasThreadAccess, E_FAIL, L"VisualTreeVerifier can only be used on the UI thread.");

    wrl::ComPtr<xaml::IDxamlCoreTestHooksStatics> testHooksStatics;
    LogThrow_IfFailed(wf::GetActivationFactory(wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_DxamlCoreTestHooks).Get(), &testHooksStatics));

    wrl::ComPtr<xaml::IDxamlCoreTestHooks> testHooks;
    RunOnDispatcherThread(dispatcherQueue, [&]
    {
        LogThrow_IfFailed(testHooksStatics->GetForCurrentThread(&testHooks));

        wrl::ComPtr<IXamlTestHooks> xamlTestHooks;
        LogThrow_IfFailed(testHooks.As(&xamlTestHooks));

        xamlTestHooks->GetElementRenderedVisuals(element, visuals.Get());
    });

    UINT size;
    LogThrow_IfFailed(visuals->get_Size(&size));
    if (size != 1)
    {
        LOG_OUTPUT(L"  vtv> CreateFromElement failed - expected 1 visual, got %u.", size);
        WEX::Common::Throw::Exception(E_FAIL, L"Element must have a single Visual.");
    }

    wrl::ComPtr<IInspectable> inspectable;
    LogThrow_IfFailed(visuals->GetAt(0, &inspectable));

    wrl::ComPtr<ixp::IVisual> visual;
    LogThrow_IfFailed(inspectable.As(&visual));

    return wrl::MakeAndInitialize<VisualTreeVerifier>(value, visual.Get(), dispatcherQueue.Get());
}

HRESULT VisualTreeVerifierStatics::CreateFromVisual(_In_ ixp::IVisual* visual, _Outptr_ test_infra::IVisualTreeVerifier** value)
{
    wrl::ComPtr<msy::IDispatcherQueueStatics> dqStatics;
    LogThrow_IfFailed(wf::GetActivationFactory(wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueue).Get(), &dqStatics));

    wrl::ComPtr<msy::IDispatcherQueue> dispatcherQueue;
    LogThrow_IfFailed(dqStatics->GetForCurrentThread(&dispatcherQueue));

    return wrl::MakeAndInitialize<VisualTreeVerifier>(value, visual, dispatcherQueue.Get());
}

VisualTreeVerifier::~VisualTreeVerifier()
{
}

HRESULT VisualTreeVerifier::RuntimeClassInitialize(_In_ ixp::IVisual* visual, _In_ msy::IDispatcherQueue* dispatcherQueue)
{
    m_visual = visual;
    m_dispatcherQueue = dispatcherQueue;
    return S_OK;
}

void VisualTreeVerifier::VerifyIsUIThread()
{
    wrl::ComPtr<msy::IDispatcherQueue2> dq2;
    LogThrow_IfFailed(m_dispatcherQueue.As(&dq2));

    boolean hasThreadAccess = false;
    LogThrow_IfFailed(dq2->get_HasThreadAccess(&hasThreadAccess));

    WEX::Common::Throw::IfFalse(hasThreadAccess, E_FAIL, L"VisualTreeVerifier can only be used on the UI thread.");
}

wrl::ComPtr<wfc::IIterator<ixp::Visual*>> VisualTreeVerifier::GetChildVisualIterator()
{
    wrl::ComPtr<ixp::IContainerVisual> containerVisual;
    LogThrow_IfFailed(m_visual.As(&containerVisual));

    wrl::ComPtr<ixp::IVisualCollection> visualCollection;
    LogThrow_IfFailed(containerVisual->get_Children(&visualCollection))

    wrl::ComPtr<wfc::IIterable<ixp::Visual*>> iterable;
    LogThrow_IfFailed(visualCollection.As(&iterable));

    wrl::ComPtr<wfc::IIterator<ixp::Visual*>> it;
    LogThrow_IfFailed(iterable->First(&it));

    return it;
}

bool VisualTreeVerifier::DoesVisualHaveTag(const wrl::ComPtr<ixp::IVisual>& visual, UINT tag)
{
    wrl::ComPtr<ixp::ICompositionObject> compositionObject;
    LogThrow_IfFailed(visual.As(&compositionObject));

    wrl::ComPtr<ixp::ICompositionPropertySet> compositionPropertySet;
    LogThrow_IfFailed(compositionObject->get_Properties(&compositionPropertySet));

    ixp::CompositionGetValueStatus status;
    float propertyValue;
    LogThrow_IfFailed(compositionPropertySet->TryGetScalar(wrl_wrappers::HStringReference(debugTagPropertyName).Get(), &propertyValue, &status));

    if (status == ixp::CompositionGetValueStatus_Succeeded
        && propertyValue == static_cast<float>(tag))
    {
        return true;
    }

    return false;
}

bool VisualTreeVerifier::WalkToFirstTaggedChildHelper(UINT tag)
{
    wrl::ComPtr<wfc::IIterator<ixp::Visual*>> it = GetChildVisualIterator();

    WEX::Common::Throw::IfNull(it.Get(), L"Visual does not have any children.");

    boolean hasCurrent = false;
    LogThrow_IfFailed(it->get_HasCurrent(&hasCurrent));

    while (hasCurrent)
    {
        wrl::ComPtr<ixp::IVisual> visual;
        LogThrow_IfFailed(it->get_Current(&visual));

        if (DoesVisualHaveTag(visual, tag))
        {
            m_visual = visual;
            return true;
        }

        LogThrow_IfFailed(it->MoveNext(&hasCurrent));
    }

    return false;
}

// Note: This mutates the VisualTreeVerifier, which is associated with a particular Visual. The intent is for the caller
// to chain together WalkToAsdf calls to walk it down a chain of Visuals.
HRESULT VisualTreeVerifier::WalkToTaggedChild(UINT tag, _Outptr_ IVisualTreeVerifier** value)
{
    VerifyIsUIThread();

    LOG_OUTPUT(L"  vtv> Walking to child visual with tag %s.", VisualDebugTagsNames[tag]);

    if (WalkToFirstTaggedChildHelper(tag))
    {
        this->QueryInterface(IID_PPV_ARGS(value));
        return S_OK;
    }
    else
    {
        WEX::Common::Throw::Exception(E_FAIL, L"Did not find any children with the specified tag.");
    }
}

// Note: This mutates the VisualTreeVerifier, which is associated with a particular Visual. The intent is for the caller
// to chain together WalkToAsdf calls to walk it down a chain of Visuals.
HRESULT VisualTreeVerifier::WalkThroughSimpleCompNode(_Outptr_ IVisualTreeVerifier** value)
{
    VerifyIsUIThread();

    LOG_OUTPUT(L"  vtv> Walking to the bottom of a simple comp node - expecting Prepend, Primary, Content visuals.");

    if (!WalkToFirstTaggedChildHelper(static_cast<UINT>(VisualDebugTags::CompNode_PrependVisual)))
    {
        WEX::Common::Throw::Exception(E_FAIL, L"Did not find Prepend visual.");
    }

    if (!WalkToFirstTaggedChildHelper(static_cast<UINT>(VisualDebugTags::CompNode_PrimaryVisual)))
    {
        WEX::Common::Throw::Exception(E_FAIL, L"Did not find Primary visual.");
    }

    if (!WalkToFirstTaggedChildHelper(static_cast<UINT>(VisualDebugTags::CompNode_ContentVisual)))
    {
        WEX::Common::Throw::Exception(E_FAIL, L"Did not find Content visual.");
    }

    this->QueryInterface(IID_PPV_ARGS(value));

    return S_OK;
}

// Note: This mutates the VisualTreeVerifier, which is associated with a particular Visual. The intent is for the caller
// to chain together WalkToAsdf calls to walk it down a chain of Visuals.
HRESULT VisualTreeVerifier::WalkToChildAtIndex(UINT childIndex, UINT expectedChildCount, _Outptr_ IVisualTreeVerifier** value)
{
    VerifyIsUIThread();

    LOG_OUTPUT(L"  vtv> Walking to child visual [%u] of %u.", childIndex, expectedChildCount);

    wrl::ComPtr<wfc::IIterator<ixp::Visual*>> it = GetChildVisualIterator();

    if (!it)
    {
        WEX::Common::Throw::Exception(E_FAIL, L"No children found.");
    }

    boolean hasCurrent = false;
    LogThrow_IfFailed(it->get_HasCurrent(&hasCurrent));

    UINT actualChildCount = 0;
    bool found = false;
    while (hasCurrent)
    {
        if (actualChildCount == childIndex)
        {
            LogThrow_IfFailed(it->get_Current(m_visual.ReleaseAndGetAddressOf()));
            found = true;
        }

        actualChildCount++;
        LogThrow_IfFailed(it->MoveNext(&hasCurrent));
    }

    WEX::Common::Throw::IfFalse(expectedChildCount == actualChildCount, E_FAIL, L"Child count did not match expected value.");
    WEX::Common::Throw::IfFalse(found, E_FAIL, L"Child at specified index was not found.");

    this->QueryInterface(IID_PPV_ARGS(value));

    return S_OK;
}

HRESULT VisualTreeVerifier::VerifyNoTaggedChild(UINT tag)
{
    VerifyIsUIThread();

    LOG_OUTPUT(L"  vtv> Verifying no child visual with tag %s.", VisualDebugTagsNames[tag]);

    wrl::ComPtr<wfc::IIterator<ixp::Visual*>> it = GetChildVisualIterator();

    if (!it)
    {
        return S_OK;
    }

    boolean hasCurrent = false;
    LogThrow_IfFailed(it->get_HasCurrent(&hasCurrent));

    while (hasCurrent)
    {
        wrl::ComPtr<ixp::IVisual> visual;
        LogThrow_IfFailed(it->get_Current(&visual));

        if (DoesVisualHaveTag(visual, tag))
        {
            LogThrow_IfFailed(E_FAIL);
        }

        LogThrow_IfFailed(it->MoveNext(&hasCurrent));
    }

    return S_OK;
}

HRESULT VisualTreeVerifier::VerifyChildCount(UINT childCount)
{
    VerifyIsUIThread();

    LOG_OUTPUT(L"  vtv> Verifying child count of %u.", childCount);

    wrl::ComPtr<wfc::IIterator<ixp::Visual*>> it = GetChildVisualIterator();

    if (!it)
    {
        return S_OK;
    }

    boolean hasCurrent = false;
    LogThrow_IfFailed(it->get_HasCurrent(&hasCurrent));

    UINT actualChildCount = 0;
    while (hasCurrent)
    {
        actualChildCount++;
        LogThrow_IfFailed(it->MoveNext(&hasCurrent));
    }

    LOG_OUTPUT(L"  vtv> Expected %u child%s. Actual count was %u.", childCount, childCount == 1 ? L"" : L"ren", actualChildCount);
    WEX::Common::Throw::IfFalse(childCount == actualChildCount, E_FAIL, L"Child count did not match expected value.");

    return S_OK;
}

bool VisualTreeVerifier::AreEqualWithinEpsilon(const float expected, const float actual)
{
    const bool areEqual = std::abs(expected - actual) <= 0.0001f;
    if (!areEqual)
    {
        LOG_OUTPUT(L"  vtv> Float comparison failed - expected %.2f, got %.2f.", expected, actual);
    }
    return areEqual;
}

HRESULT VisualTreeVerifier::VerifyVisualOffset(const float x, const float y)
{
    VerifyIsUIThread();

    LOG_OUTPUT(L"  vtv> Verifying visual offset to be (%.2f, %.2f, 0).", x, y);

    wfn::Vector3 actualOffset = {};
    LogThrow_IfFailed(m_visual->get_Offset(&actualOffset));

    LOG_OUTPUT(L"  vtv> Actual visual offset is (%.2f, %.2f, 0).", actualOffset.X, actualOffset.Y);

    WEX::Common::Throw::IfFalse(AreEqualWithinEpsilon(x, actualOffset.X), E_FAIL, L"Visual offset.X did not match expected value.");
    WEX::Common::Throw::IfFalse(AreEqualWithinEpsilon(y, actualOffset.Y), E_FAIL, L"Visual offset.Y did not match expected value.");

    return S_OK;
}

HRESULT VisualTreeVerifier::GetVisualOffset(_Out_ wfn::Vector3* offset)
{
    VerifyIsUIThread();

    LogThrow_IfFailed(m_visual->get_Offset(offset));

    LOG_OUTPUT(L"  vtv> Returning visual offset (%.2f, %.2f, %.2f).", offset->X, offset->Y, offset->Z);

    return S_OK;
}

HRESULT VisualTreeVerifier::VerifyVisualSize(const float width, const float height)
{
    VerifyIsUIThread();

    LOG_OUTPUT(L"  vtv> Verifying visual size to be %.2f x %.2f.", width, height);

    wfn::Vector2 actualSize = {};
    LogThrow_IfFailed(m_visual->get_Size(&actualSize));

    LOG_OUTPUT(L"  vtv> Actual visual size is %.2f x %.2f.", actualSize.X, actualSize.Y);

    WEX::Common::Throw::IfFalse(AreEqualWithinEpsilon(width, actualSize.X), E_FAIL, L"Visual width did not match expected value.");
    WEX::Common::Throw::IfFalse(AreEqualWithinEpsilon(height, actualSize.Y), E_FAIL, L"Visual height did not match expected value.");

    return S_OK;
}

HRESULT VisualTreeVerifier::VerifyRoundedCornerClip(const float left, const float top, const float right, const float bottom, const float cornerRadius)
{
    VerifyIsUIThread();

    LOG_OUTPUT(L"  vtv> Verifying Visual.Clip to be a rounded corner clip with LTRB = {%.2f, %.2f, %.2f, %.2f} and CornerRadius = %.2f.", left, top, right, bottom, cornerRadius);

    wrl::ComPtr<ixp::ICompositionClip> clip;
    LogThrow_IfFailed(m_visual->get_Clip(&clip));

    wrl::ComPtr<ixp::IRectangleClip> rectangleClip;
    LogThrow_IfFailed(clip.As(&rectangleClip));

    float actualLeft;
    LogThrow_IfFailed(rectangleClip->get_Left(&actualLeft));
    WEX::Common::Throw::IfFalse(AreEqualWithinEpsilon(left, actualLeft), E_FAIL, L"RectangleClip.Left did not match expected value.");

    float actualTop;
    LogThrow_IfFailed(rectangleClip->get_Top(&actualTop));
    WEX::Common::Throw::IfFalse(AreEqualWithinEpsilon(top, actualTop), E_FAIL, L"RectangleClip.Top did not match expected value.");

    float actualRight;
    LogThrow_IfFailed(rectangleClip->get_Right(&actualRight));
    WEX::Common::Throw::IfFalse(AreEqualWithinEpsilon(right, actualRight), E_FAIL, L"RectangleClip.Right did not match expected value.");

    float actualBottom;
    LogThrow_IfFailed(rectangleClip->get_Bottom(&actualBottom));
    WEX::Common::Throw::IfFalse(AreEqualWithinEpsilon(bottom, actualBottom), E_FAIL, L"RectangleClip.Bottom did not match expected value.");

    wfn::Vector2 actualCornerRadius = {};

    LogThrow_IfFailed(rectangleClip->get_TopLeftRadius(&actualCornerRadius));
    WEX::Common::Throw::IfFalse(AreEqualWithinEpsilon(cornerRadius, actualCornerRadius.X) && AreEqualWithinEpsilon(cornerRadius, actualCornerRadius.Y), E_FAIL, L"RectangleClip.TopLeftRadius did not match expected value.");
    LogThrow_IfFailed(rectangleClip->get_TopRightRadius(&actualCornerRadius));
    WEX::Common::Throw::IfFalse(AreEqualWithinEpsilon(cornerRadius, actualCornerRadius.X) && AreEqualWithinEpsilon(cornerRadius, actualCornerRadius.Y), E_FAIL, L"RectangleClip.TopRightRadius did not match expected value.");
    LogThrow_IfFailed(rectangleClip->get_BottomLeftRadius(&actualCornerRadius));
    WEX::Common::Throw::IfFalse(AreEqualWithinEpsilon(cornerRadius, actualCornerRadius.X) && AreEqualWithinEpsilon(cornerRadius, actualCornerRadius.Y), E_FAIL, L"RectangleClip.BottomLeftRadius did not match expected value.");
    LogThrow_IfFailed(rectangleClip->get_BottomRightRadius(&actualCornerRadius));
    WEX::Common::Throw::IfFalse(AreEqualWithinEpsilon(cornerRadius, actualCornerRadius.X) && AreEqualWithinEpsilon(cornerRadius, actualCornerRadius.Y), E_FAIL, L"RectangleClip.BottomRightRadius did not match expected value.");

    return S_OK;
}

HRESULT VisualTreeVerifier::VerifyRoundedCornerClipCorners(const float topLeft, const float topRight, const float bottomRight, const float bottomLeft)
{
    VerifyIsUIThread();

    LOG_OUTPUT(L"  vtv> Verifying Visual.Clip to be a rounded corner clip with corners {TL, TR, BR, BL} = {%.2f, %.2f, %.2f, %.2f}.", topLeft, topRight, bottomRight, bottomLeft);

    wrl::ComPtr<ixp::ICompositionClip> clip;
    LogThrow_IfFailed(m_visual->get_Clip(&clip));

    wrl::ComPtr<ixp::IRectangleClip> rectangleClip;
    LogThrow_IfFailed(clip.As(&rectangleClip));

    wfn::Vector2 actualCornerRadius = {};

    LogThrow_IfFailed(rectangleClip->get_TopLeftRadius(&actualCornerRadius));
    WEX::Common::Throw::IfFalse(AreEqualWithinEpsilon(topLeft, actualCornerRadius.X) && AreEqualWithinEpsilon(topLeft, actualCornerRadius.Y), E_FAIL, L"RectangleClip.TopLeftRadius did not match expected value.");

    LogThrow_IfFailed(rectangleClip->get_TopRightRadius(&actualCornerRadius));
    WEX::Common::Throw::IfFalse(AreEqualWithinEpsilon(topRight, actualCornerRadius.X) && AreEqualWithinEpsilon(topRight, actualCornerRadius.Y), E_FAIL, L"RectangleClip.TopRightRadius did not match expected value.");

    LogThrow_IfFailed(rectangleClip->get_BottomRightRadius(&actualCornerRadius));
    WEX::Common::Throw::IfFalse(AreEqualWithinEpsilon(bottomRight, actualCornerRadius.X) && AreEqualWithinEpsilon(bottomRight, actualCornerRadius.Y), E_FAIL, L"RectangleClip.BottomRightRadius did not match expected value.");

    LogThrow_IfFailed(rectangleClip->get_BottomLeftRadius(&actualCornerRadius));
    WEX::Common::Throw::IfFalse(AreEqualWithinEpsilon(bottomLeft, actualCornerRadius.X) && AreEqualWithinEpsilon(bottomLeft, actualCornerRadius.Y), E_FAIL, L"RectangleClip.BottomLeftRadius did not match expected value.");

    return S_OK;
}

HRESULT VisualTreeVerifier::VerifyTransformMatrix_Scale(const float x, const float y)
{
    VerifyIsUIThread();

    LOG_OUTPUT(L"  vtv> Verifying Visual.TransformMatrix to have a scale of (%.2fx, %.2fx).", x, y);

    wfn::Matrix4x4 actualTransformMatrix = {};
    LogThrow_IfFailed(m_visual->get_TransformMatrix(&actualTransformMatrix));
    WEX::Common::Throw::IfFalse(AreEqualWithinEpsilon(x, actualTransformMatrix.M11), E_FAIL, L"Visual TransformMatrix.M11 did not match expected value.");
    WEX::Common::Throw::IfFalse(AreEqualWithinEpsilon(y, actualTransformMatrix.M22), E_FAIL, L"Visual TransformMatrix.M22 did not match expected value.");

    return S_OK;
}

}
