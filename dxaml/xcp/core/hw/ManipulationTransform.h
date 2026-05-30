// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

_Check_return_ HRESULT CreateManipulationTransformPropertySet(
    _In_ WUComp::ICompositor *compositor,
    _Outptr_result_nullonfailure_ WUComp::ICompositionPropertySet** result);

_Check_return_ HRESULT ConnectAnimationWithPrependTransform(
    _In_ WUComp::ICompositionObject *manipulationPropertySetCO,
    _In_ WUComp::ICompositionObject *sourceCO,
    _In_ wfn::Matrix4x4 prependTransform,
    _In_z_ const wchar_t *propertyName);

_Check_return_ HRESULT ConnectComplexAnimationWithPrependTransform(
    _In_ WUComp::ICompositionObject *manipulationPropertySetCO,
    _In_ WUComp::ICompositionObject *sourceCOPrimary,
    _In_ WUComp::ICompositionObject *sourceCOSecondary,
    _In_ wfn::Matrix4x4 prependTransform,
    _In_z_ const wchar_t *propertyName);

_Check_return_ HRESULT ConnectManipulationPropertySetToTransform(
    _In_ WUComp::ICompositionPropertySet *manipulationPropertySet,
    _In_ IUnknown *transform,
    float contentOffsetX,
    float contentOffsetY,
    _Outptr_ WUComp::ICompositionObject** manipulationTransformCO);

_Check_return_ HRESULT DisconnectManipulationPropertySet(
    _In_ WUComp::ICompositionPropertySet *manipulationPropertySet);

