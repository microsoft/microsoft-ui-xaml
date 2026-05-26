// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//
// Macros for defining the object Create method.  Use these on any parser or OM objects to ensure
// consistent behavior.
//
// Two versions are support:
//
//      DECLARE_CREATE_AND_TYPECONVERTER(T)
//          Defines Create with type converter support.
//          Assumes T::FromString is defined.
//
//      DECLARE_CREATE
//          Defines a simple Create with no type converter support
//

#define DECLARE_CREATE_WITH_TYPECONVERTER(T) \
static _Check_return_ HRESULT Create( \
    _Outptr_ CDependencyObject **ppObject, \
    _In_ CREATEPARAMETERS *pCreate \
    ) \
{ \
    HRESULT hr = S_OK; \
    \
    T* _this = new T(pCreate->m_pCore); \
    CDependencyObject *pTemp = NULL; \
    IFC(ValidateAndInit(_this, &pTemp)); \
    if (pCreate->m_value.GetType() == valueString) \
    { \
        IFC(_this->FromString(pCreate)); \
    } \
    *ppObject = pTemp; \
    _this = NULL; \
Cleanup: \
    if(pTemp) delete _this; \
    RRETURN(hr); \
}

#define DECLARE_CREATE(T)  \
static _Check_return_ HRESULT Create( \
    _Outptr_ CDependencyObject **ppObject, \
    _In_ CREATEPARAMETERS *pCreate \
    ) \
{ \
    HRESULT hr = S_OK; \
    \
    if (pCreate->m_value.GetType() == valueString) \
    { \
        IFC(E_NOTIMPL); \
    } \
    else \
    { \
        T *pObj = new T(pCreate->m_pCore);  \
        hr = ValidateAndInit(pObj, ppObject);   \
        if (FAILED(hr)) delete pObj; \
    } \
    \
Cleanup: \
    RRETURN(hr); \
}

#define DECLARE_CREATE_AP(T)  \
    static _Check_return_ HRESULT Create(\
    _Outptr_ CDependencyObject **ppObject, \
    _In_ CREATEPARAMETERS *pCreate \
    ) \
{ \
    HRESULT hr = S_OK; \
    T* pObject = NULL; \
    \
    IFCEXPECT(pCreate); \
    if (pCreate->m_value.GetType() != valueObject) \
    { \
        IFC(E_NOTIMPL); \
    } \
    else \
    { \
        pObject = new T(pCreate->m_pCore, pCreate->m_value); \
        hr = ValidateAndInit(pObject, ppObject); \
        if (FAILED(hr)) delete pObject; \
    } \
    \
Cleanup: \
    RRETURN(hr); \
}

// Macro template for CNoParentShareableDependencyObject::Clone() method
//  (1) Allocate new instance of the type using the cloning constructor
//  (2) IFCOOM() to see if the allocation was successful.
//  (3) IFC(hr) to see if the cloning constructor ran into other problems.
//  (4) All is well, give the clone back to caller via ppClone.

#define DECLARE_SHAREABLEDEPENDENCYOBJECT_CLONE(T) \
virtual _Check_return_ HRESULT Clone( _Outptr_ CNoParentShareableDependencyObject **ppClone ) \
{ \
    HRESULT hr = S_OK; \
    T *pLocalClone = NULL; \
    \
    pLocalClone = new T(*this, hr); \
    \
    IFC(hr); \
    \
    IFC(pLocalClone->ClonePropertySetField(this)); \
    \
    *ppClone = pLocalClone; \
    pLocalClone = NULL; \
    \
Cleanup: \
    ReleaseInterface(pLocalClone); \
    RRETURN(hr); \
}

// this is a dummy implementation used to ensure that the compiler doesn't
// fold any Create functions as these are used by the property system to
// uniquely identify the types
//
// c.f.: sakt[] in xcptypes.h
#define DECLARE_CREATE_RETURN(T, RETCODE) \
static _Check_return_ HRESULT Create( \
    _Outptr_ CDependencyObject **ppObject, \
    _In_ CREATEPARAMETERS * pCreate\
    ) \
{ \
    UNREFERENCED_PARAMETER(ppObject); \
    static bool dummy = pCreate->m_value.OwnsValue();\
\
    /* This function should *never* be called. */ \
    RRETURN(RETCODE); \
}