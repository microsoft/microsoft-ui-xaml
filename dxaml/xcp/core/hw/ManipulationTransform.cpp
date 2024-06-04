// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ManipulationTransform.h"

using namespace Microsoft::WRL::Wrappers;

static HRESULT CreateExpressionAnimation(
    _In_ WUComp::ICompositionObject *compositionObject,
    _In_z_ const wchar_t *expression,
    _Outptr_result_nullonfailure_ WUComp::ICompositionAnimation **result)
{
    *result = nullptr;

    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> expressionAnimation;
    Microsoft::WRL::ComPtr<WUComp::ICompositor> compositor;

    IFC_RETURN(compositionObject->get_Compositor(&compositor));
    IFC_RETURN(compositor->CreateExpressionAnimationWithExpression(HStringReference(expression).Get(), &expressionAnimation));
    IFC_RETURN(expressionAnimation.CopyTo(result));

    return S_OK;
}

// Connects the named property of the source composition object as an expression
// animation to the corresponding property of the manipulation composition object.
static HRESULT ConnectAnimation(
    _In_ WUComp::ICompositionObject *manipulationPropertySetCO,
    _In_ WUComp::ICompositionObject *sourceCO,
    _In_z_ const wchar_t *propertyName)
{
    const wchar_t *key = L"manipTransform";

    std::wstring expression;
    expression.reserve(128);
    expression.append(key);
    expression.append(L".");
    expression.append(propertyName);

    Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> compositionAnimation;
    IFC_RETURN(CreateExpressionAnimation(manipulationPropertySetCO, expression.c_str(), &compositionAnimation));
    IFC_RETURN(compositionAnimation->SetReferenceParameter(HStringReference(key).Get(), sourceCO));
    IFC_RETURN(manipulationPropertySetCO->StartAnimation(HStringReference(propertyName).Get(), compositionAnimation.Get()));

    return S_OK;
}

// Connects the named property of the source composition object as an expression
// animation to the corresponding property of the manipulation composition object.
// The propertyName should point to a Vector3 property.
// The additional offset is applied to the resulting value.
static HRESULT ConnectAnimationWithOffset(
    _In_ WUComp::ICompositionObject *manipulationPropertySetCO,
    _In_ WUComp::ICompositionObject *sourceCO,
    _In_ wfn::Vector3 offset,
    _In_z_ const wchar_t *propertyName)
{
    const wchar_t *sourceKey = L"manipTransform";
    const wchar_t *offsetKey = L"offset";

    std::wstring expression;
    expression.reserve(128);
    expression.append(sourceKey);
    expression.append(L".");
    expression.append(propertyName);
    expression.append(L"+");
    expression.append(offsetKey);

    Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> compositionAnimation;
    IFC_RETURN(CreateExpressionAnimation(manipulationPropertySetCO, expression.c_str(), &compositionAnimation));
    IFC_RETURN(compositionAnimation->SetReferenceParameter(HStringReference(sourceKey).Get(), sourceCO));
    IFC_RETURN(compositionAnimation->SetVector3Parameter(HStringReference(offsetKey).Get(), offset));
    IFC_RETURN(manipulationPropertySetCO->StartAnimation(HStringReference(propertyName).Get(), compositionAnimation.Get()));

    return S_OK;
}

// Connects the named property of the source composition object as an expression
// animation to the corresponding property of the manipulation composition object.
// The propertyName should point to a Matrix4x4 property.
// The resulting value is pre-multiplied with the specified matrix.
HRESULT ConnectAnimationWithPrependTransform(
    _In_ WUComp::ICompositionObject *manipulationPropertySetCO,
    _In_ WUComp::ICompositionObject *sourceCO,
    _In_ wfn::Matrix4x4 prependTransform,
    _In_z_ const wchar_t *propertyName)
{
    const wchar_t *sourceKey = L"manipTransform";
    const wchar_t *transformKey = L"transform";

    std::wstring expression;
    expression.reserve(128);
    expression.append(transformKey);
    expression.append(L"*");
    expression.append(sourceKey);
    expression.append(L".");
    expression.append(propertyName);

    Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> compositionAnimation;
    IFC_RETURN(CreateExpressionAnimation(manipulationPropertySetCO, expression.c_str(), &compositionAnimation));
    IFC_RETURN(compositionAnimation->SetReferenceParameter(HStringReference(sourceKey).Get(), sourceCO));
    IFC_RETURN(compositionAnimation->SetMatrix4x4Parameter(HStringReference(transformKey).Get(), prependTransform));
    IFC_RETURN(manipulationPropertySetCO->StartAnimation(HStringReference(propertyName).Get(), compositionAnimation.Get()));

    return S_OK;
}

_Check_return_ HRESULT ConnectComplexAnimationWithPrependTransform(
    _In_ WUComp::ICompositionObject *manipulationPropertySetCO,
    _In_ WUComp::ICompositionObject *sourceCOPrimary,
    _In_ WUComp::ICompositionObject *sourceCOSecondary,
    _In_ wfn::Matrix4x4 prependTransform,
    _In_z_ const wchar_t *propertyName)
{
    const wchar_t *sourceKeyPrimary = L"manipTransformPrimary";
    const wchar_t *sourceKeySecondary = L"manipTransformSecondary";
    const wchar_t *transformKey = L"transform";

    std::wstring expression;
    expression.reserve(128);
    expression.append(transformKey);
    expression.append(L"*");
    expression.append(sourceKeySecondary);
    expression.append(L".");
    expression.append(propertyName);
    expression.append(L"*");
    expression.append(sourceKeyPrimary);
    expression.append(L".");
    expression.append(propertyName);

    Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> compositionAnimation;
    IFC_RETURN(CreateExpressionAnimation(manipulationPropertySetCO, expression.c_str(), &compositionAnimation));
    IFC_RETURN(compositionAnimation->SetReferenceParameter(HStringReference(sourceKeyPrimary).Get(), sourceCOPrimary));
    IFC_RETURN(compositionAnimation->SetReferenceParameter(HStringReference(sourceKeySecondary).Get(), sourceCOSecondary));
    IFC_RETURN(compositionAnimation->SetMatrix4x4Parameter(HStringReference(transformKey).Get(), prependTransform));
    IFC_RETURN(manipulationPropertySetCO->StartAnimation(HStringReference(propertyName).Get(), compositionAnimation.Get()));

    return S_OK;
}

static HRESULT SetDefaultValues(_In_ WUComp::ICompositionPropertySet *manipulationPropertySet)
{
    IFC_RETURN(manipulationPropertySet->InsertVector3(HStringReference(L"Translation").Get(), { 0 }));
    IFC_RETURN(manipulationPropertySet->InsertVector3(HStringReference(L"Pan").Get(), { 0 }));
    IFC_RETURN(manipulationPropertySet->InsertVector3(HStringReference(L"Scale").Get(), { 1, 1, 1 }));
    IFC_RETURN(manipulationPropertySet->InsertVector3(HStringReference(L"CenterPoint").Get(), { 0 }));
    IFC_RETURN(manipulationPropertySet->InsertMatrix4x4(HStringReference(L"Matrix").Get(), { 1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1 }));
    return S_OK;
}

// Creates a composition property set filled with properties found in the
// manipulation transform. The returned object is ready to be passed to
// ConnectManipulationPropertySetToTransform to connect with the manipulation transform instance.
_Check_return_ HRESULT CreateManipulationTransformPropertySet(
    _In_ WUComp::ICompositor *compositor,
    _Outptr_result_nullonfailure_ WUComp::ICompositionPropertySet **result)
{
    *result = nullptr;

    Microsoft::WRL::ComPtr<WUComp::ICompositionPropertySet> manipulationPropertySet;
    IFC_RETURN(compositor->CreatePropertySet(&manipulationPropertySet));
    IFC_RETURN(SetDefaultValues(manipulationPropertySet.Get()));

    *result = manipulationPropertySet.Detach();

    return S_OK;
}

// Sets the legacy DComp transform as an expression animation to the manipulation transform property set.
// Pass NULL source to disconnect the previously connected transform.
_Check_return_ HRESULT ConnectManipulationPropertySetToTransform(
    _In_ WUComp::ICompositionPropertySet *manipulationPropertySet,
    _In_ IUnknown* transform,
    float contentOffsetX,
    float contentOffsetY,
    _Outptr_ WUComp::ICompositionObject** manipulationTransformCO)
{
    Microsoft::WRL::ComPtr<WUComp::ICompositionObject> manipulationPropertySetCO;
    IFC_RETURN(manipulationPropertySet->QueryInterface(IID_PPV_ARGS(&manipulationPropertySetCO)));

    // TODO: do not recreate this ICompositionObject if only offset values change
    Microsoft::WRL::ComPtr<WUComp::ICompositionObject> legacyTransformCO;
    {
        IFC_RETURN(transform->QueryInterface(IID_PPV_ARGS(legacyTransformCO.ReleaseAndGetAddressOf())));
    }

    wfn::Vector3 offset = { contentOffsetX, contentOffsetY, 0 };
    wfn::Matrix4x4 prependTransform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, contentOffsetX, contentOffsetY, 0, 1 };

    IFC_RETURN(ConnectAnimationWithOffset(manipulationPropertySetCO.Get(), legacyTransformCO.Get(), offset, L"Translation"));
    IFC_RETURN(ConnectAnimationWithOffset(manipulationPropertySetCO.Get(), legacyTransformCO.Get(), offset, L"Pan"));
    IFC_RETURN(ConnectAnimationWithOffset(manipulationPropertySetCO.Get(), legacyTransformCO.Get(), offset, L"CenterPoint"));
    IFC_RETURN(ConnectAnimation(manipulationPropertySetCO.Get(), legacyTransformCO.Get(), L"Scale"));
    IFC_RETURN(ConnectAnimationWithPrependTransform(manipulationPropertySetCO.Get(), legacyTransformCO.Get(), prependTransform, L"Matrix"));

    *manipulationTransformCO = legacyTransformCO.Detach();

    return S_OK;
}

_Check_return_ HRESULT DisconnectManipulationPropertySet(_In_ WUComp::ICompositionPropertySet *manipulationPropertySet)
{
    IFC_RETURN(SetDefaultValues(manipulationPropertySet));
    return S_OK;
}


