// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "MRTResourceUnitTests.h"
#include "ParserUtilities.h"

#include <MRTResource.h>
#include <PALMemoryProxy.h>
#include <XcpAllocationDebug.h>
#include <memorybuffer.h>
#include <winuri.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <thread>
#include <vector>
#include <wrl.h>
#include <xref_ptr.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    namespace
    {
        constexpr std::array<BYTE, 8> expectedBytes = { 0x58, 0x42, 0x46, 0x00, 0x12, 0x34, 0x56, 0x78 };

        struct CandidateState
        {
            BYTE* buffer = nullptr;
            UINT32 valueAsMemoryBufferCalls = 0;
            HRESULT loadResult = S_OK;
            HRESULT createReferenceResult = S_OK;
            HRESULT getBufferResult = S_OK;
            std::atomic<UINT32> buffersAlive{0};
            std::atomic<UINT32> referencesAlive{0};
            std::atomic<DWORD> referenceDestructionThreadId{0};
            std::atomic<bool> destroyed{false};
            std::atomic<DWORD> destructionThreadId{0};
        };

        class EmbeddedDataBufferReference final
            : public wrl::RuntimeClass<wrl::RuntimeClassFlags<wrl::WinRtClassicComMix>,
                wf::IMemoryBufferReference, wf_::IMemoryBufferByteAccess, wrl::FtmBase>
        {
            InspectableClass(L"Microsoft.UI.Xaml.Tests.EmbeddedDataBufferReference", BaseTrust);

        public:
            explicit EmbeddedDataBufferReference(const std::shared_ptr<CandidateState>& state)
                : m_state(state)
            {
                ++m_state->referencesAlive;
            }

            ~EmbeddedDataBufferReference() override
            {
                m_state->referenceDestructionThreadId = GetCurrentThreadId();
                --m_state->referencesAlive;
            }

            HRESULT STDMETHODCALLTYPE GetBuffer(BYTE** buffer, UINT32* size) override
            {
                *buffer = nullptr;
                *size = 0;
                IFC_RETURN(m_state->getBufferResult);
                *buffer = m_bytes.data();
                *size = static_cast<UINT32>(m_bytes.size());
                m_state->buffer = *buffer;
                return S_OK;
            }

            HRESULT STDMETHODCALLTYPE get_Capacity(UINT32* value) override
            {
                *value = static_cast<UINT32>(m_bytes.size());
                return S_OK;
            }

            HRESULT STDMETHODCALLTYPE add_Closed(
                wf::ITypedEventHandler<wf::IMemoryBufferReference*, IInspectable*>*,
                EventRegistrationToken*) override
            {
                IFC_RETURN(E_NOTIMPL);
                return S_OK;
            }

            HRESULT STDMETHODCALLTYPE remove_Closed(EventRegistrationToken) override
            {
                IFC_RETURN(E_NOTIMPL);
                return S_OK;
            }

        private:
            std::shared_ptr<CandidateState> m_state;
            std::array<BYTE, expectedBytes.size()> m_bytes = expectedBytes;
        };

        class EmbeddedDataBuffer final
            : public wrl::RuntimeClass<wrl::RuntimeClassFlags<wrl::WinRt>, wf::IMemoryBuffer, wrl::FtmBase>
        {
            InspectableClass(L"Microsoft.UI.Xaml.Tests.EmbeddedDataBuffer", BaseTrust);

        public:
            explicit EmbeddedDataBuffer(const std::shared_ptr<CandidateState>& state)
                : m_state(state)
            {
                ++m_state->buffersAlive;
            }

            ~EmbeddedDataBuffer() override
            {
                --m_state->buffersAlive;
            }

            HRESULT STDMETHODCALLTYPE CreateReference(wf::IMemoryBufferReference** value) override
            {
                *value = nullptr;
                IFC_RETURN(m_state->createReferenceResult);
                auto reference = wrl::Make<EmbeddedDataBufferReference>(m_state);
                IFCOOM_RETURN(reference);
                *value = reference.Detach();
                return S_OK;
            }

        private:
            std::shared_ptr<CandidateState> m_state;
        };

        class EmbeddedDataCandidate final
            : public wrl::RuntimeClass<wrl::RuntimeClassFlags<wrl::WinRt>,
                mwar::IResourceCandidate, mwar::IResourceCandidate2, wrl::FtmBase>
        {
            InspectableClass(L"Microsoft.UI.Xaml.Tests.EmbeddedDataCandidate", BaseTrust);

        public:
            explicit EmbeddedDataCandidate(const std::shared_ptr<CandidateState>& state)
                : m_state(state)
            {
            }

            ~EmbeddedDataCandidate() override
            {
                m_state->destructionThreadId = GetCurrentThreadId();
                m_state->destroyed = true;
            }

            HRESULT STDMETHODCALLTYPE get_Kind(mwar::ResourceCandidateKind* value) override
            {
                *value = mwar::ResourceCandidateKind_EmbeddedData;
                return S_OK;
            }

            HRESULT STDMETHODCALLTYPE get_ValueAsBytes(UINT32* size, BYTE** value) override
            {
                *size = 0;
                *value = nullptr;
                VERIFY_FAIL(L"Embedded data should use the memory buffer without copying bytes.");
                IFC_RETURN(E_NOTIMPL);
                return S_OK;
            }

            HRESULT STDMETHODCALLTYPE ValueAsMemoryBuffer(wf::IMemoryBuffer** value) override
            {
                *value = nullptr;
                ++m_state->valueAsMemoryBufferCalls;
                IFC_RETURN(m_state->loadResult);

                auto buffer = wrl::Make<EmbeddedDataBuffer>(m_state);
                IFCOOM_RETURN(buffer);
                *value = buffer.Detach();
                return S_OK;
            }

            HRESULT STDMETHODCALLTYPE get_ValueAsString(HSTRING* value) override
            {
                *value = nullptr;
                VERIFY_FAIL(L"Embedded data should not request a string.");
                return E_NOTIMPL;
            }

            HRESULT STDMETHODCALLTYPE get_QualifierValues(wfc::IMapView<HSTRING, HSTRING>** value) override
            {
                *value = nullptr;
                VERIFY_FAIL(L"Loading embedded bytes should not request qualifiers.");
                return E_NOTIMPL;
            }

        private:
            std::shared_ptr<CandidateState> m_state;
        };

        xref_ptr<IPALResource> CreateResource(const std::shared_ptr<CandidateState>& state)
        {
            wrl::ComPtr<mwar::IResourceCandidate> candidate = wrl::Make<EmbeddedDataCandidate>(state);
            VERIFY_IS_NOT_NULL(candidate.Get());

            constexpr WCHAR uriString[] = L"ms-resource:///Files/Embedded.xaml";
            xref_ptr<IPALUri> uri;
            VERIFY_SUCCEEDED(CWinUriFactory::Create(
                ARRAYSIZE(uriString) - 1, uriString, uri.ReleaseAndGetAddressOf()));

            xref_ptr<IPALResource> resource;
            VERIFY_SUCCEEDED(CMRTResource::Create(uri.get(), uri.get(), candidate, resource.ReleaseAndGetAddressOf()));
            return resource;
        }

        void VerifyBytes(const BYTE* buffer)
        {
            VERIFY_IS_TRUE(std::equal(expectedBytes.begin(), expectedBytes.end(), buffer));
        }
    }

    bool MRTResourceUnitTests::ClassSetup()
    {
#if XCP_MONITOR
        InitCheckedMemoryChainLock();
#endif
        return true;
    }

    bool MRTResourceUnitTests::ClassCleanup()
    {
#if XCP_MONITOR
        DeleteCheckedMemoryChainLock();
#endif
        return true;
    }

    void MRTResourceUnitTests::EmbeddedDataMemoryRetainsResource()
    {
        auto state = std::make_shared<CandidateState>();
        auto resource = CreateResource(state);
        xref_ptr<IPALMemory> memory;
        VERIFY_SUCCEEDED(resource->Load(memory.ReleaseAndGetAddressOf()));

        VERIFY_ARE_EQUAL(expectedBytes.size(), static_cast<size_t>(memory->GetSize()));
        VERIFY_IS_TRUE(state->buffer == memory->GetAddress());
        VERIFY_ARE_EQUAL(1u, state->valueAsMemoryBufferCalls);

        resource.reset();

        // Check lifetime before dereferencing, so the old implementation fails without a use-after-free.
        VERIFY_IS_FALSE(state->destroyed.load());
        VERIFY_ARE_EQUAL(1u, state->buffersAlive.load());
        VERIFY_ARE_EQUAL(1u, state->referencesAlive.load());
        VerifyBytes(static_cast<const BYTE*>(memory->GetAddress()));

        memory.reset();
        VERIFY_IS_TRUE(state->destroyed.load());
        VERIFY_ARE_EQUAL(0u, state->buffersAlive.load());
        VERIFY_ARE_EQUAL(0u, state->referencesAlive.load());
    }

    void MRTResourceUnitTests::RepeatedLoadsShareEmbeddedData()
    {
        auto state = std::make_shared<CandidateState>();
        auto resource = CreateResource(state);
        xref_ptr<IPALMemory> first;
        xref_ptr<IPALMemory> second;
        VERIFY_SUCCEEDED(resource->Load(first.ReleaseAndGetAddressOf()));
        VERIFY_SUCCEEDED(resource->Load(second.ReleaseAndGetAddressOf()));
        VERIFY_IS_TRUE(first->GetAddress() == second->GetAddress());
        VERIFY_IS_TRUE(state->buffer == first->GetAddress());
        VERIFY_ARE_EQUAL(1u, state->valueAsMemoryBufferCalls);

        auto alias = second;
        resource.reset();
        first.reset();
        second.reset();
        VERIFY_IS_FALSE(state->destroyed.load());
        VerifyBytes(static_cast<const BYTE*>(alias->GetAddress()));

        alias.reset();
        VERIFY_IS_TRUE(state->destroyed.load());
    }

    void MRTResourceUnitTests::StreamRetainsEmbeddedDataMemory()
    {
        auto state = std::make_shared<CandidateState>();
        auto resource = CreateResource(state);
        xref_ptr<IPALMemory> memory;
        VERIFY_SUCCEEDED(resource->Load(memory.ReleaseAndGetAddressOf()));

        ParserUtilities parserUtils;
        auto stream = parserUtils.GetPALStreamFromPALMemory(memory);
        resource.reset();
        memory.reset();
        VERIFY_IS_FALSE(state->destroyed.load());

        std::array<BYTE, expectedBytes.size()> actualBytes{};
        XUINT32 bytesRead = 0;
        VERIFY_SUCCEEDED(stream->Read(actualBytes.data(), static_cast<XUINT32>(actualBytes.size()), &bytesRead));
        VERIFY_ARE_EQUAL(static_cast<XUINT32>(actualBytes.size()), bytesRead);
        VerifyBytes(actualBytes.data());

        stream.reset();
        VERIFY_IS_TRUE(state->destroyed.load());
    }

    void MRTResourceUnitTests::ImageMemoryReleasesResourceOnWorkerThread()
    {
        auto state = std::make_shared<CandidateState>();
        auto resource = CreateResource(state);
        xref_ptr<IPALMemory> memory;
        VERIFY_SUCCEEDED(resource->Load(memory.ReleaseAndGetAddressOf()));

        auto proxy = std::make_unique<PALMemoryProxy>(memory.get());
        resource.reset();
        memory.reset();
        VERIFY_IS_FALSE(state->destroyed.load());

        HRESULT initializeResult = E_FAIL;
        bool bytesMatch = false;
        std::thread worker([proxy = std::move(proxy), &initializeResult, &bytesMatch]() mutable
        {
            initializeResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            if (SUCCEEDED(initializeResult))
            {
                bytesMatch = proxy->GetSize() == expectedBytes.size()
                    && std::equal(expectedBytes.begin(), expectedBytes.end(), proxy->GetData());
                proxy.reset();
                CoUninitialize();
            }
        });
        worker.join();

        VERIFY_SUCCEEDED(initializeResult);
        VERIFY_IS_TRUE(bytesMatch);
        VERIFY_IS_TRUE(state->destroyed.load());
        VERIFY_ARE_NOT_EQUAL(GetCurrentThreadId(), state->destructionThreadId.load());
        VERIFY_ARE_EQUAL(0u, state->buffersAlive.load());
        VERIFY_ARE_EQUAL(0u, state->referencesAlive.load());
        VERIFY_ARE_EQUAL(state->destructionThreadId.load(), state->referenceDestructionThreadId.load());
    }

    void MRTResourceUnitTests::ConcurrentMemoryReleases()
    {
        auto state = std::make_shared<CandidateState>();
        auto resource = CreateResource(state);
        std::array<std::vector<xref_ptr<IPALMemory>>, 4> batches;
        for (auto& batch : batches)
        {
            for (size_t index = 0; index < 256; ++index)
            {
                xref_ptr<IPALMemory> memory;
                THROW_IF_FAILED(resource->Load(memory.ReleaseAndGetAddressOf()));
                batch.push_back(std::move(memory));
            }
        }

        std::atomic<bool> start{false};
        std::vector<std::thread> workers;
        for (auto& batch : batches)
        {
            workers.emplace_back([batch = std::move(batch), &start]() mutable
            {
                while (!start.load())
                {
                    std::this_thread::yield();
                }
                batch.clear();
            });
        }
        start = true;
        for (auto& worker : workers)
        {
            worker.join();
        }

        const auto references = resource->AddRef();
        resource->Release();
        VERIFY_ARE_EQUAL(2u, references);
        VERIFY_IS_FALSE(state->destroyed.load());
        VERIFY_ARE_EQUAL(1u, state->valueAsMemoryBufferCalls);

        resource.reset();
        VERIFY_IS_TRUE(state->destroyed.load());
    }

    void MRTResourceUnitTests::FailedLoadClearsOutput()
    {
        auto state = std::make_shared<CandidateState>();
        auto resource = CreateResource(state);
        xref_ptr<IPALMemory> validMemory;
        VERIFY_SUCCEEDED(resource->Load(validMemory.ReleaseAndGetAddressOf()));

        auto failingState = std::make_shared<CandidateState>();
        failingState->loadResult = E_FAIL;
        auto failingResource = CreateResource(failingState);
        IPALMemory* output = validMemory.get();
        VERIFY_ARE_EQUAL(E_FAIL, failingResource->Load(&output));
        VERIFY_IS_NULL(output);

        failingResource.reset();
        VERIFY_IS_TRUE(failingState->destroyed.load());
    }

    void MRTResourceUnitTests::FailedInitializationCanBeRetried()
    {
        for (auto failure : { &CandidateState::loadResult,
            &CandidateState::createReferenceResult, &CandidateState::getBufferResult })
        {
            auto state = std::make_shared<CandidateState>();
            state.get()->*failure = E_FAIL;
            auto resource = CreateResource(state);
            xref_ptr<IPALMemory> memory;
            VERIFY_ARE_EQUAL(E_FAIL, resource->Load(memory.ReleaseAndGetAddressOf()));
            VERIFY_IS_NULL(memory.get());
            VERIFY_ARE_EQUAL(0u, state->buffersAlive.load());
            VERIFY_ARE_EQUAL(0u, state->referencesAlive.load());

            state.get()->*failure = S_OK;
            VERIFY_SUCCEEDED(resource->Load(memory.ReleaseAndGetAddressOf()));
            VERIFY_ARE_EQUAL(expectedBytes.size(), static_cast<size_t>(memory->GetSize()));
            VerifyBytes(static_cast<const BYTE*>(memory->GetAddress()));
            VERIFY_ARE_EQUAL(2u, state->valueAsMemoryBufferCalls);

            xref_ptr<IPALMemory> second;
            VERIFY_SUCCEEDED(resource->Load(second.ReleaseAndGetAddressOf()));
            VERIFY_IS_TRUE(memory->GetAddress() == second->GetAddress());
            VERIFY_ARE_EQUAL(2u, state->valueAsMemoryBufferCalls);
        }
    }

} } } } }
