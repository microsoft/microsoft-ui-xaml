// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls { namespace Primitives
{

LoopingSelectorPanel::LoopingSelectorPanel()
    : _snapPointOffset(0.0),
      _snapPointSpacing(0.0)
{}

LoopingSelectorPanel::~LoopingSelectorPanel()
{}

_Check_return_
HRESULT
LoopingSelectorPanel::InitializeImpl()
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_controls::ICanvasFactory> spInnerFactory;
    wrl::ComPtr<xaml_controls::ICanvas> spInnerInstance;
    wrl::ComPtr<IInspectable> spInnerInspectable;

    IFC(LoopingSelectorPanelGenerated::InitializeImpl());

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Canvas).Get(),
        &spInnerFactory));

    IFC(spInnerFactory->CreateInstance(
        static_cast<IInspectable*>(static_cast<ILoopingSelectorPanel*>(this)),
        &spInnerInspectable,
        &spInnerInstance));

    IFC(SetComposableBasePointers(
            spInnerInspectable.Get(),
            spInnerFactory.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT LoopingSelectorPanel::get_AreHorizontalSnapPointsRegularImpl(_Out_ boolean* pValue)
{
    *pValue = TRUE;
    RRETURN(S_OK);
}

_Check_return_
HRESULT LoopingSelectorPanel::get_AreVerticalSnapPointsRegularImpl(_Out_ boolean* pValue)
{
    *pValue = TRUE;
    RRETURN(S_OK);
}


_Check_return_
HRESULT LoopingSelectorPanel::GetIrregularSnapPointsImpl(_In_ xaml_controls::Orientation orientation, _In_ xaml_controls::Primitives::SnapPointsAlignment alignment, _Outptr_ wfc::IVectorView<FLOAT>** returnValue)
{
    // NOTE: This method should never be called, both
    // horizontal and vertical SnapPoints are ALWAYS regular.
    UNREFERENCED_PARAMETER(orientation);
    UNREFERENCED_PARAMETER(alignment);
    UNREFERENCED_PARAMETER(returnValue);
    RRETURN(E_NOTIMPL);
}

_Check_return_
HRESULT LoopingSelectorPanel::GetRegularSnapPointsImpl(_In_ xaml_controls::Orientation orientation, _In_ xaml_controls::Primitives::SnapPointsAlignment alignment, _Out_ FLOAT* offset, _Out_ FLOAT* returnValue)
{
    HRESULT hr = S_OK;
    // For now the LoopingSelectorPanel will simply return a evenly spaced grid,
    // the vertical and horizontal snap points will be identical.
    UNREFERENCED_PARAMETER(orientation);
    UNREFERENCED_PARAMETER(alignment);

    IFCPTR(offset);
    IFCPTR(returnValue);

    *offset = _snapPointOffset;
    *returnValue = _snapPointSpacing;

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT LoopingSelectorPanel::SetOffsetInPixels(_In_ FLOAT offset)
{
    if (_snapPointOffset != offset)
    {
        _snapPointOffset = offset;
        return RaiseSnapPointsChangedEvents();
    }
    return S_OK;
}

_Check_return_
HRESULT LoopingSelectorPanel::SetSizeInPixels(_In_ FLOAT size)
{
    if (_snapPointSpacing != size)
    {
        _snapPointSpacing = size;
        return RaiseSnapPointsChangedEvents();
    }
    return S_OK;
}

_Check_return_
HRESULT LoopingSelectorPanel::RaiseSnapPointsChangedEvents()
{
    HRESULT hr = S_OK;

    wrl::ComPtr<IInspectable> spThisAsInspectable;

    IFC(QueryInterface(
        __uuidof(IInspectable),
        &spThisAsInspectable));

    IFC(_snapPointsChangedEventSource.InvokeAll(
        spThisAsInspectable.Get(),
        spThisAsInspectable.Get()));

Cleanup:
    RRETURN(hr);
}

} } } } } XAML_ABI_NAMESPACE_END
