// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <BasePALResource.h>
#include <palexports.h>
#include <palmemory.h>
#include <PalResourceManager.h>
#include <shlwapi.h>

#include "FilePathResource.h"
#include "MRTKnownQualifierNames.h"
#include "MRTResource.h"

// Creates a CMRTResource for the given resource URI and MRT resource candidate.
_Check_return_ HRESULT
CMRTResource::Create(
    _In_ IPALUri* pResourceUri,
    _In_ IPALUri* pPhysicalResourceUri,
    _In_ const wrl::ComPtr<mwar::IResourceCandidate>& resourceCandidate,
    _Outptr_ IPALResource** ppResource)
{
    IFCPTRRC_RETURN(resourceCandidate, E_INVALIDARG);

    auto pResource = make_xref<CMRTResource>(pResourceUri, pPhysicalResourceUri);
    pResource->m_pResourceCandidate = resourceCandidate;
    IFC_RETURN(resourceCandidate->get_Kind(&(pResource->m_kind)));

    *ppResource = pResource.detach();

    return S_OK;
}

// Set the pPhysicalResourceUri in the base ctor to pResourceUri
// Set a unique value here, or in a CMRTResource setter
CMRTResource::CMRTResource(_In_ IPALUri* pResourceUri, _In_ IPALUri* pPhysicalResourceUri)
    : CBasePALResource(pResourceUri)
    , m_pPhysicalResourceUri(pPhysicalResourceUri)
{
}

CMRTResource::~CMRTResource()
{
    CoTaskMemFree(m_embeddedDataBuffer);
}

// See IPALResource::Load().
// CMRTResource implements Load() by first determining the type of the MRT resource
// this object represents. Then, depending on that type, a resource sub-object is created
// that matches the type, and Load() is delegating to the sub-object.
_Check_return_ HRESULT CMRTResource::Load(_Outptr_ IPALMemory** ppMemory)
{
    switch (m_kind)
    {
        case mwar::ResourceCandidateKind::ResourceCandidateKind_EmbeddedData:
            // For embedded data type resources, we must load via a stream.
            IFC_RETURN(EnsureEmbeddedDataResource());
            IFC_RETURN(GetPALMemoryServices()->CreatePALMemoryFromBuffer(
                m_embeddedDataBufferSize,
                m_embeddedDataBuffer,
                false /* OwnsBuffer */,
                ppMemory));
            break;
        case mwar::ResourceCandidateKind::ResourceCandidateKind_FilePath:
            // For path type resources, map the file into memory.
            IFC_RETURN(EnsureFilePathResource());
            IFC_RETURN(m_pFilePathResource->Load(ppMemory));
            break;
        default:
            // This is a MRT resource type that we don't support Load() for.
            IFC_RETURN(E_NOTIMPL);
    }

    return S_OK;
}

const WCHAR* CMRTResource::ToString()
{
    switch (m_kind)
    {
        case mwar::ResourceCandidateKind::ResourceCandidateKind_EmbeddedData:
            return L"EmbeddedData";
        case mwar::ResourceCandidateKind::ResourceCandidateKind_FilePath:
            if (SUCCEEDED(EnsureFilePathResource()))
            {
                return m_pFilePathResource->ToString();
            }
            break;
    }

    return CBasePALResource::ToString();
}

// See IPALResource::Exists().
// How the existence check is done varies depending on the type of MRT
// resource this object represents.
_Check_return_ HRESULT CMRTResource::Exists(_Out_ bool* pfExists)
{
    switch (m_kind)
    {
        case mwar::ResourceCandidateKind::ResourceCandidateKind_EmbeddedData:
            // EmbeddedData type resources always exist
            *pfExists = true;
            break;
        case mwar::ResourceCandidateKind::ResourceCandidateKind_FilePath:
            IFC_RETURN(EnsureFilePathResource());
            IFC_RETURN(m_pFilePathResource->Exists(pfExists));
            break;
        default:
            // This is a MRT resource type that we don't support Exists() for.
            IFC_RETURN(E_NOTIMPL);
    }

    return S_OK;
}

//  Returns URI to physical resource.
IPALUri* CMRTResource::GetPhysicalResourceUriNoRef()
{
    return m_pPhysicalResourceUri;
}

// See IPALResource::TryGetFilePath().
// If this object represents a MRT path resource, first create a resource
// sub-object to represent the file path. Then delegate TryGetFilePath()
// to the sub-object.
_Check_return_ HRESULT CMRTResource::TryGetFilePath(_Out_ xstring_ptr* pstrFilePath)
{
    pstrFilePath->Reset();

    if (m_kind != mwar::ResourceCandidateKind::ResourceCandidateKind_FilePath)
    {
        // If our resource type is not Path, we can't supply a file path.
        return S_OK;
    }

    IFC_RETURN(EnsureFilePathResource());
    IFC_RETURN(m_pFilePathResource->TryGetFilePath(pstrFilePath));

    return S_OK;
}

// See IPALResource::TryGetRawStream().
// If this object represents a MRT embedded data resource, first ensure that
// the data has been read out into a buffer, then use the buffer to initialize
// an IStream
_Check_return_ HRESULT CMRTResource::TryGetRawStream(const PALResources::RawStreamType streamType, _Outptr_result_maybenull_ void** ppStream)
{
    *ppStream = nullptr;

    if (m_kind != mwar::ResourceCandidateKind::ResourceCandidateKind_EmbeddedData)
    {
        // If our resource type is not EmbeddedData, we can't supply a stream.
        return S_OK;
    }

    IFC_RETURN(EnsureEmbeddedDataResource());

    // We only support supplying an IStream.
    if (streamType == PALResources::RawStreamType::IStream)
    {
        IStream* pMemStream = SHCreateMemStream(m_embeddedDataBuffer, m_embeddedDataBufferSize);
        IFCOOM_RETURN(pMemStream);
        *ppStream = pMemStream;
    }

    return S_OK;
}

// See IPALResource::GetScalePercentage().
// Returns the scale associated with this resource, if any.
// If this resource has an MRT resource candidate that has a scale qualifier,
// the scale percentage value is returned. Otherwise, 0 is returned.
unsigned int CMRTResource::GetScalePercentage()
{
    wrl::ComPtr<wfc::IMapView<HSTRING, HSTRING>> qualifierValuesView;
    IFC_RETURN(m_pResourceCandidate->get_QualifierValues(qualifierValuesView.ReleaseAndGetAddressOf()));

    wrl_wrappers::HStringReference hScaleReference(MRTKnownQualifierNames::Scale);
    boolean hasKey;
    IFC_RETURN(qualifierValuesView->HasKey(hScaleReference.Get(), &hasKey));

    if (hasKey)
    {
        wrl_wrappers::HString scaleValue;
        IFC_RETURN(qualifierValuesView->Lookup(hScaleReference.Get(), scaleValue.GetAddressOf()));

        unsigned int length;
        return _wtoi(scaleValue.GetRawBuffer(&length));
    }
    else
    {
        return 0;
    }
}

// Private helper - ensures that the resource sub-object that represents a
// file path resource exists. Must only be called if this object represents
// a MRT resource of type Path.
_Check_return_ HRESULT CMRTResource::EnsureFilePathResource()
{
    ASSERT(m_embeddedDataBuffer == nullptr);

    if (m_pFilePathResource != nullptr)
    {
        return S_OK;
    }

    wrl_wrappers::HString filePathHstring;
    xstring_ptr strFilePath;
    IFC_RETURN(m_pResourceCandidate->get_ValueAsString(filePathHstring.GetAddressOf()));
    IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(filePathHstring.Get(), &strFilePath));
    IFC_RETURN(CFilePathResource::Create(GetResourceUriNoRef(), strFilePath, m_pFilePathResource.ReleaseAndGetAddressOf()));

    return S_OK;
}

// Private helper - ensures that the resource sub-object that represents a
// stream resource exists. Must only be called if this object represents
// a MRT resource of type EmbeddedData.
_Check_return_ HRESULT CMRTResource::EnsureEmbeddedDataResource()
{
    ASSERT(m_pFilePathResource == nullptr);

    if (m_embeddedDataBuffer == nullptr)
    {
        IFC_RETURN(m_pResourceCandidate->get_ValueAsBytes(
            &m_embeddedDataBufferSize,
            &m_embeddedDataBuffer));
    }

    return S_OK;
}

