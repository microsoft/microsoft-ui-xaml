// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "precomp.h"
#include "NamespaceAliases.h"
#include <Microsoft.UI.Composition.h>

class MockEasingFunction
    : public ixp::ICompositionEasingFunction
    , public ixp::ILinearEasingFunction
    , public ixp::ICubicBezierEasingFunction
{
public:
    MockEasingFunction() : m_isLinear(true) {}
    MockEasingFunction(wfn::Vector2 cp1, wfn::Vector2 cp2) : m_isLinear(false), m_cp1(cp1), m_cp2(cp2) {}
    ~MockEasingFunction() {}

    STDMETHOD(QueryInterface)(REFIID iid, PVOID *pVoid)
    {
        if (iid == __uuidof(ixp::ICompositionEasingFunction))
        {
            *pVoid = static_cast<ixp::ICompositionEasingFunction*>(this);
            AddRef();
            return S_OK;
        }
        else if (m_isLinear && iid == __uuidof(ixp::ILinearEasingFunction))
        {
            *pVoid = static_cast<ixp::ILinearEasingFunction*>(this);
            AddRef();
            return S_OK;
        }
        else if (!m_isLinear && iid == __uuidof(ixp::ICubicBezierEasingFunction))
        {
            *pVoid = static_cast<ixp::ICubicBezierEasingFunction*>(this);
            AddRef();
            return S_OK;
        }

        return E_NOTIMPL;
    }

    STDMETHOD_(ULONG, AddRef)(THIS)
    {
        return InterlockedIncrement(&m_refCount);
    }

    STDMETHOD_(ULONG, Release)(THIS)
    {
        ULONG newRefCount = InterlockedDecrement(&m_refCount);
        if (newRefCount == 0) { delete this; }
        return newRefCount;
    }

    STDMETHOD(GetIids)(ULONG *iidCount, IID **iids) { return E_NOTIMPL; }
    STDMETHOD(GetTrustLevel)(TrustLevel *trustLvl) { return E_NOTIMPL; }
    STDMETHOD(GetRuntimeClassName)(HSTRING *pClassName) { return E_NOTIMPL; }

    STDMETHOD(get_ControlPoint1)(wfn::Vector2* controlPoint) { *controlPoint = m_cp1; return S_OK; }
    STDMETHOD(get_ControlPoint2)(wfn::Vector2* controlPoint) { *controlPoint = m_cp2; return S_OK; }

public:
    ULONG m_refCount { 1 };
    float m_isLinear;
    wfn::Vector2 m_cp1{};
    wfn::Vector2 m_cp2{};
};

class MockEasingFunctionStatics
    : public ixp::ICompositionEasingFunctionStatics
{
public:
    MockEasingFunctionStatics() {}
    virtual ~MockEasingFunctionStatics() {}

    // IInspectable
    STDMETHOD(QueryInterface)(REFIID iid, PVOID *pVoid) { return E_NOTIMPL; }
    STDMETHOD_(ULONG, AddRef)(THIS) { return InterlockedIncrement(&m_refCount); }
    STDMETHOD_(ULONG, Release)(THIS)
    {
        ULONG newRefCount = InterlockedDecrement(&m_refCount);
        if (newRefCount == 0) { delete this; }
        return newRefCount;
    }

    STDMETHOD(GetIids)(ULONG *iidCount, IID **iids) { return E_NOTIMPL; }
    STDMETHOD(GetTrustLevel)(TrustLevel *trustLvl) { return E_NOTIMPL; }
    STDMETHOD(GetRuntimeClassName)(HSTRING *pClassName) { return E_NOTIMPL; }

    STDMETHOD(CreateCubicBezierEasingFunction)(
        ixp::ICompositor * owner,
        wfn::Vector2 controlPoint1,
        wfn::Vector2 controlPoint2,
        ixp::ICubicBezierEasingFunction * * result)
    {
        *result = new MockEasingFunction(controlPoint1, controlPoint2);
        return S_OK;
    }

    STDMETHOD(CreateLinearEasingFunction)(
        ixp::ICompositor * owner,
        ixp::ILinearEasingFunction * * result)
    {
        *result = new MockEasingFunction();
        return S_OK;
    }

    STDMETHOD(CreateStepEasingFunction)(
        ixp::ICompositor * owner,
        ixp::IStepEasingFunction * * result) { return E_NOTIMPL; }

    STDMETHOD(CreateStepEasingFunctionWithStepCount)(
        ixp::ICompositor * owner,
        INT32 stepCount,
        ixp::IStepEasingFunction * * result) { return E_NOTIMPL; }

    STDMETHOD(CreateBackEasingFunction)(
        ixp::ICompositor * owner,
        ixp::CompositionEasingFunctionMode mode,
        FLOAT amplitude,
        ixp::IBackEasingFunction * * result) { return E_NOTIMPL; }

    STDMETHOD(CreateBounceEasingFunction)(
        ixp::ICompositor * owner,
        ixp::CompositionEasingFunctionMode mode,
        INT32 bounces,
        FLOAT bounciness,
        ixp::IBounceEasingFunction * * result) { return E_NOTIMPL; }

    STDMETHOD(CreateCircleEasingFunction)(
        ixp::ICompositor * owner,
        ixp::CompositionEasingFunctionMode mode,
        ixp::ICircleEasingFunction * * result) { return E_NOTIMPL; }

    STDMETHOD(CreateElasticEasingFunction)(
        ixp::ICompositor * owner,
        ixp::CompositionEasingFunctionMode mode,
        INT32 oscillations,
        FLOAT springiness,
        ixp::IElasticEasingFunction * * result) { return E_NOTIMPL; }

    STDMETHOD(CreateExponentialEasingFunction)(
        ixp::ICompositor * owner,
        ixp::CompositionEasingFunctionMode mode,
        FLOAT exponent,
        ixp::IExponentialEasingFunction * * result) { return E_NOTIMPL; }

    STDMETHOD(CreatePowerEasingFunction)(
        ixp::ICompositor * owner,
        ixp::CompositionEasingFunctionMode mode,
        FLOAT power,
        ixp::IPowerEasingFunction * * result) { return E_NOTIMPL; }

    STDMETHOD(CreateSineEasingFunction)(
        ixp::ICompositor * owner,
        ixp::CompositionEasingFunctionMode mode,
        ixp::ISineEasingFunction * * result) { return E_NOTIMPL; }

public:
    ULONG m_refCount { 1 };
};
