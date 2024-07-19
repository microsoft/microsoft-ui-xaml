// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Microsoft.Windows.ApplicationModel.Resources.h>

#include <PalResourceManager.h>
#include <BasePALResource.h>

// CMRTResource is an IPALResource implementation that represents a resource
// supplied by MRT.
class CMRTResource : public CBasePALResource
{
    template <typename Ty, typename... Types> friend xref_ptr<Ty> make_xref(Types&&... Args);

public:
    static _Check_return_ HRESULT Create(
        _In_ IPALUri* pResourceUri,
        _In_ IPALUri* pPhysicalResourceUri,
        _In_ const wrl::ComPtr<mwar::IResourceCandidate>& pResourceCandidate,
        _Outptr_ IPALResource** ppResource);

    // IPALResource
    _Check_return_ HRESULT Load(_Outptr_ IPALMemory** ppMemory) override;
    const WCHAR* ToString() override;
    _Check_return_ HRESULT Exists(_Out_ bool* pfExists) override;
    IPALUri* GetPhysicalResourceUriNoRef() override;
    _Check_return_ HRESULT TryGetFilePath(_Out_ xstring_ptr* pstrFilePath) override;
    _Check_return_ HRESULT TryGetRawStream(const PALResources::RawStreamType streamType, _Outptr_result_maybenull_ void** ppStream) override;
    unsigned int GetScalePercentage() override;

private:
    CMRTResource(_In_ IPALUri* pResourceUri, _In_ IPALUri* pPhysicalResourceUri);
    ~CMRTResource() override;

    _Check_return_ HRESULT EnsureFilePathResource();
    _Check_return_ HRESULT EnsureEmbeddedDataResource();

    mwar::ResourceCandidateKind m_kind = mwar::ResourceCandidateKind::ResourceCandidateKind_Unknown;
    wrl::ComPtr<mwar::IResourceCandidate> m_pResourceCandidate;
    xref_ptr<IPALResource> m_pFilePathResource;
    xref_ptr<IPALUri> m_pPhysicalResourceUri;
    BYTE* m_embeddedDataBuffer = nullptr;
    std::uint32_t m_embeddedDataBufferSize = 0;
};
