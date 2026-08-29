// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "IsApiContractNotPresent.g.h"
#include "IsApiContractPresent.g.h"
#include "IsPropertyNotPresent.g.h"
#include "IsPropertyPresent.g.h"
#include "IsTypeNotPresent.g.h"
#include "IsTypePresent.g.h"

namespace
{
    _Check_return_ HRESULT
    PrepareXamlPredicateArguments(
        _In_ wfc::IVectorView<HSTRING>* pArguments,
        _Out_ std::vector<xstring_ptr>& predicateArgs)
    {
        unsigned int count = 0;
        IFC_RETURN(pArguments->get_Size(&count));
        predicateArgs.clear();
        predicateArgs.reserve(count);
        for (unsigned int i = 0; i < count; i++)
        {
            HSTRING argAsHstring;
            IFC_RETURN(pArguments->GetAt(i, &argAsHstring));

            xstring_ptr argAsXstr;
            IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(argAsHstring, &argAsXstr));

            predicateArgs.push_back(argAsXstr);
        }

        return S_OK;
    }
}

namespace DirectUI
{
    _Check_return_ HRESULT IsApiContractNotPresent::EvaluateImpl(_In_ wfc::IVectorView<HSTRING>* pArguments, _Out_ BOOLEAN* pReturnValue)
    {
        std::vector<xstring_ptr> predicateArgs;
        IFC_RETURN(PrepareXamlPredicateArguments(pArguments, predicateArgs));

        try
        {
            *pReturnValue = (static_cast<CIsApiContractNotPresentPredicate*>(GetHandle())->Evaluate(predicateArgs)) ? TRUE : FALSE;
        }
        CATCH_RETURN();

        return S_OK;
    }

    _Check_return_ HRESULT IsApiContractPresent::EvaluateImpl(_In_ wfc::IVectorView<HSTRING>* pArguments, _Out_ BOOLEAN* pReturnValue)
    {
        std::vector<xstring_ptr> predicateArgs;
        IFC_RETURN(PrepareXamlPredicateArguments(pArguments, predicateArgs));

        try
        {
            *pReturnValue = (static_cast<CIsApiContractPresentPredicate*>(GetHandle())->Evaluate(predicateArgs)) ? TRUE : FALSE;
        }
        CATCH_RETURN();

        return S_OK;
    }

    _Check_return_ HRESULT IsPropertyNotPresent::EvaluateImpl(_In_ wfc::IVectorView<HSTRING>* pArguments, _Out_ BOOLEAN* pReturnValue)
    {
        std::vector<xstring_ptr> predicateArgs;
        IFC_RETURN(PrepareXamlPredicateArguments(pArguments, predicateArgs));

        try
        {
            *pReturnValue = (static_cast<CIsPropertyNotPresentPredicate*>(GetHandle())->Evaluate(predicateArgs)) ? TRUE : FALSE;
        }
        CATCH_RETURN();

        return S_OK;
    }

    _Check_return_ HRESULT IsPropertyPresent::EvaluateImpl(_In_ wfc::IVectorView<HSTRING>* pArguments, _Out_ BOOLEAN* pReturnValue)
    {
        std::vector<xstring_ptr> predicateArgs;
        IFC_RETURN(PrepareXamlPredicateArguments(pArguments, predicateArgs));

        try
        {
            *pReturnValue = (static_cast<CIsPropertyPresentPredicate*>(GetHandle())->Evaluate(predicateArgs)) ? TRUE : FALSE;
        }
        CATCH_RETURN();

        return S_OK;
    }

    _Check_return_ HRESULT IsTypeNotPresent::EvaluateImpl(_In_ wfc::IVectorView<HSTRING>* pArguments, _Out_ BOOLEAN* pReturnValue)
    {
        std::vector<xstring_ptr> predicateArgs;
        IFC_RETURN(PrepareXamlPredicateArguments(pArguments, predicateArgs));

        try
        {
            *pReturnValue = (static_cast<CIsTypeNotPresentPredicate*>(GetHandle())->Evaluate(predicateArgs)) ? TRUE : FALSE;
        }
        CATCH_RETURN();

        return S_OK;
    }

    _Check_return_ HRESULT IsTypePresent::EvaluateImpl(_In_ wfc::IVectorView<HSTRING>* pArguments, _Out_ BOOLEAN* pReturnValue)
    {
        std::vector<xstring_ptr> predicateArgs;
        IFC_RETURN(PrepareXamlPredicateArguments(pArguments, predicateArgs));

        try
        {
            *pReturnValue = (static_cast<CIsTypePresentPredicate*>(GetHandle())->Evaluate(predicateArgs)) ? TRUE : FALSE;
        }
        CATCH_RETURN();

        return S_OK;
    }
}