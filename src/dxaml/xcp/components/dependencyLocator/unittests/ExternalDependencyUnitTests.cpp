// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ExternalDependencyUnitTests.h"
#include <ExternalDependency.h>
#include <unittests\mocks\MockExternalDependency.h>
#include "LoadLibraryAbs.h"

void STDAPICALLTYPE LoadStub(_In_opt_ const HMODULE hModule, _In_ const wchar_t * modulename)
{
}

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {
        class __declspec(uuid("34D3F1B7-2000-4E5D-ABF2-A4DE3520B8E6")) MockObject
            : public wrl::RuntimeClass<IInspectable, wrl::FtmBase>
        {
            InspectableClass(
                L"Mocks.MockObject",
                BaseTrust
            );

        public:
            MockObject()
            {
            }

            ~MockObject()
            {
            }
        };

        std::wstring ExternalDependencyUnitTests::m_mockDependencyPath{};
        
        bool ExternalDependencyUnitTests::ClassSetup()
        {
            m_mockDependencyPath = GetMuxAbsPath(L"Test\\ExternalTestMocks.dll");

            return true;
        }

        void ExternalDependencyUnitTests::CanLoadStorageFromLibrary()
        {
            DependencyLocator::InitializeProcess();

            DependencyLocator::ExternalDependency<ExternalSimpleObject1> dep1(m_mockDependencyPath.c_str(), LoadStub);

            VERIFY_ARE_EQUAL(dep1->ReturnTheNumberOne(), 1);

            VERIFY_ARE_EQUAL(dep1->GetLoadedCount(), 1);

            DependencyLocator::UninitializeProcess();
        }

        void ExternalDependencyUnitTests::LoadsStorageOnce()
        {
            DependencyLocator::InitializeProcess();

            DependencyLocator::ExternalDependency<ExternalSimpleObject1>
                dep11(m_mockDependencyPath.c_str(), LoadStub);
            DependencyLocator::ExternalDependency<ExternalSimpleObject1>
                dep12(m_mockDependencyPath.c_str(), LoadStub);
            DependencyLocator::ExternalDependency<ExternalSimpleObject2>
                dep21(m_mockDependencyPath.c_str(), LoadStub);
            DependencyLocator::ExternalDependency<ExternalSimpleObject2>
                dep22(m_mockDependencyPath.c_str(), LoadStub);

            VERIFY_ARE_EQUAL(dep11->ReturnTheNumberOne(), 1);
            VERIFY_ARE_EQUAL(dep12->ReturnTheNumberOne(), 1);

            VERIFY_ARE_EQUAL(dep12->GetLoadedCount(), 1);

            VERIFY_ARE_EQUAL(dep21->ReturnTheNumberTwo(), 2);
            VERIFY_ARE_EQUAL(dep22->ReturnTheNumberTwo(), 2);

            VERIFY_ARE_EQUAL(dep12->GetLoadedCount(), 1);

            DependencyLocator::UninitializeProcess();
        }

        void ExternalDependencyUnitTests::UnloadingReleasesReferences()
        {
            DependencyLocator::InitializeProcess();

            DependencyLocator::ExternalDependency<ExternalSimpleObject2>
                dep2(m_mockDependencyPath.c_str(), LoadStub);

            wrl::WeakRef wkObj;
            {
                auto obj = wrl::Make<MockObject>();
                VERIFY_SUCCEEDED(obj.AsWeak(&wkObj));
                dep2->SetItem(obj.Get());
            }

            {
                auto handle = GetModuleHandle(m_mockDependencyPath.c_str());
                VERIFY_IS_NOT_NULL(handle);

                wrl::ComPtr<IInspectable> resolved;
                VERIFY_SUCCEEDED(wkObj.As(&resolved));

                VERIFY_IS_NOT_NULL(resolved);
            }

            DependencyLocator::UninitializeProcess();

            {
                auto handle = GetModuleHandle(m_mockDependencyPath.c_str());
                VERIFY_IS_NULL(handle);

                wrl::ComPtr<IInspectable> resolved;
                VERIFY_SUCCEEDED(wkObj.As(&resolved));

                VERIFY_IS_NULL(resolved);
            }
        }
    }
} } } }

