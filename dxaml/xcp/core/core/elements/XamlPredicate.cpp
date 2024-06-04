// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XamlPredicate.h"

#include <string>
#include <windows.foundation.metadata.h>

#pragma region Evaluation helper methods

bool IsApiContractPresent_Evaluate(std::vector<xstring_ptr> args)
{
    ctl::ComPtr<wf::Metadata::IApiInformationStatics> apiInformationStatics;
    THROW_IF_FAILED(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_Metadata_ApiInformation).Get(),
        apiInformationStatics.GetAddressOf()));
    boolean isPresent = FALSE;

    if (args.size() == 2)
    {
        auto contractName = wrl_wrappers::HStringReference(args[0].GetBuffer(), args[0].GetCount());

        auto tmp = std::wstring(args[1].GetBuffer());
        auto majorVersion = static_cast<unsigned short>(std::stoul(tmp));

        auto hr = apiInformationStatics->IsApiContractPresentByMajor(contractName.Get(), majorVersion, &isPresent);
        if(FAILED(hr))
        {
            // ApiInformation doesn't always work on downlevel machines for unpackaged apps.
            // For that case, assume it doesn't exist.
            hr = S_OK;
            isPresent = FALSE;
        }
    }
    else if (args.size() == 3)
    {
        auto contractName = wrl_wrappers::HStringReference(args[0].GetBuffer(), args[0].GetCount());

        auto tmp = std::wstring(args[1].GetBuffer());
        auto majorVersion = static_cast<unsigned short>(std::stoul(tmp));

        tmp = std::wstring(args[2].GetBuffer());
        auto minorVersion = static_cast<unsigned short>(std::stoul(tmp));

        auto hr = apiInformationStatics->IsApiContractPresentByMajorAndMinor(contractName.Get(), majorVersion, minorVersion, &isPresent);
        if(FAILED(hr))
        {
            // ApiInformation doesn't always work on downlevel machines for unpackaged apps.
            // For that case, assume it doesn't exist.
            hr = S_OK;
            isPresent = FALSE;
        }
    }
    else
    {
        THROW_HR(E_INVALIDARG);
    }

    return !!isPresent;
}

bool IsPropertyPresent_Evaluate(std::vector<xstring_ptr> args)
{
    ctl::ComPtr<wf::Metadata::IApiInformationStatics> apiInformationStatics;
    THROW_IF_FAILED(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_Metadata_ApiInformation).Get(),
        apiInformationStatics.GetAddressOf()));
    boolean isPresent = FALSE;

    if (args.size() == 2)
    {
        auto typeName = wrl_wrappers::HStringReference(args[0].GetBuffer(), args[0].GetCount());
        auto propertyName = wrl_wrappers::HStringReference(args[1].GetBuffer(), args[1].GetCount());

        auto hr = apiInformationStatics->IsPropertyPresent(typeName.Get(), propertyName.Get(), &isPresent);
        if(FAILED(hr))
        {
            // ApiInformation doesn't always work on downlevel machines for unpackaged apps.
            // For that case, assume it doesn't exist.
            hr = S_OK;
            isPresent = FALSE;
        }
    }
    else
    {
        THROW_HR(E_INVALIDARG);
    }

    return !!isPresent;
}

bool IsTypePresent_Evaluate(std::vector<xstring_ptr> args)
{
    ctl::ComPtr<wf::Metadata::IApiInformationStatics> apiInformationStatics;
    THROW_IF_FAILED(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_Metadata_ApiInformation).Get(),
        apiInformationStatics.GetAddressOf()));
    boolean isPresent = FALSE;

    if (args.size() == 1)
    {
        auto typeName = wrl_wrappers::HStringReference(args[0].GetBuffer(), args[0].GetCount());

        auto hr = apiInformationStatics->IsTypePresent(typeName.Get(), &isPresent);
        if(FAILED(hr))
        {
            // ApiInformation doesn't always work on downlevel machines for unpackaged apps.
            // For that case, assume it doesn't exist.
            hr = S_OK;
            isPresent = FALSE;
        }
    }
    else
    {
        THROW_HR(E_INVALIDARG);
    }

    return !!isPresent;
}

#pragma endregion Evaluation helper methods

bool CIsApiContractPresentPredicate::Evaluate(_In_ std::vector<xstring_ptr>& args)
{
    return IsApiContractPresent_Evaluate(args);
}

bool CIsApiContractNotPresentPredicate::Evaluate(_In_ std::vector<xstring_ptr>& args)
{
    return !IsApiContractPresent_Evaluate(args);
}

bool CIsPropertyPresentPredicate::Evaluate(_In_ std::vector<xstring_ptr>& args)
{
    return IsPropertyPresent_Evaluate(args);
}

bool CIsPropertyNotPresentPredicate::Evaluate(_In_ std::vector<xstring_ptr>& args)
{
    return !IsPropertyPresent_Evaluate(args);
}

bool CIsTypePresentPredicate::Evaluate(_In_ std::vector<xstring_ptr>& args)
{
    return IsTypePresent_Evaluate(args);
}

bool CIsTypeNotPresentPredicate::Evaluate(_In_ std::vector<xstring_ptr>& args)
{
    return !IsTypePresent_Evaluate(args);
}