// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ScopeTree.h"
#include "ScopeTreeUnitTests.h"
#include <CxxMockTaef.h>
#include <XamlLogging.h>
#include <UIAEnums.h> // Core Automation peer enums
#include "Mocks.h"

#include "FocusMovement.h"

using namespace Focus;
using namespace CxxMock;
using namespace AccessKeys;

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace AccessKeys {

        MOCK_CLASS(MockCDOWithGetValue, MockCDO)

            HRESULT GetValueByIndex(KnownPropertyIndex i, CValue* value)
            {
                if (i == KnownPropertyIndex::UIElement_ExitDisplayModeOnAccessKeyInvoked)
                {
                    Expect(*value, AsBool)
                        .ReturnValue(false);
                }

                return S_OK;
            }

            bool IsFocusable()
            {
                return true;
            }
        END_MOCK

        MOCK_CLASS(MockScope, Base)
            bool IsDeleted;
            MockScope() : IsDeleted(false) {}

            STUB_METHOD(HRESULT, Invoke, 3(const wchar_t, bool, AKInvokeReturnParams<MockCDOWithGetValue>*))

            STUB_METHOD(HRESULT, ShowAccessKeys, 0)
            STUB_METHOD(HRESULT, HideAccessKeys, 0)

            STUB_METHOD(MockCDOWithGetValue*, GetElement, 0)

            STUB_METHOD(HRESULT, RemoveFromAccessKeyOwner, 1(MockCDOWithGetValue*))
            STUB_METHOD(HRESULT, AddToAccessKeyOwner, 1(MockCDOWithGetValue*))

            STUB_METHOD(xref::weakref_ptr<MockCDOWithGetValue>, GetScopeParent, 0)

            STUB_METHOD(bool, ShouldElementEnteringTreeUpdateScope, 1(MockCDOWithGetValue*))

            STUB_METHOD(bool, IsScopeFilteringInput, 0)
            STUB_METHOD(HRESULT, ProcessEscapeKey, 0)
            ~MockScope() {}
        END_MOCK

        MOCK_CLASS(MockScopeBuilder, Base)

            static void VefifyDeleter(MockScope *mockScope)
            {
                VERIFY_IS_FALSE(mockScope->IsDeleted);
                mockScope->IsDeleted = true;
            }

            HRESULT ConstructScope(MockCDOWithGetValue* const parent, std::shared_ptr<MockScope>& newScope)
            {
                MockScope* pScope = nullptr;
                HRESULT hr = ConstructScope(parent, &pScope);
                newScope = std::shared_ptr<MockScope>(pScope, VefifyDeleter);
                return hr;
            }
            STUB_METHOD(HRESULT, ConstructScope, 2(MockCDOWithGetValue* const, MockScope**))
        END_MOCK

        MOCK_CLASS(MockModeContainer, Base)
            STUB_METHOD(bool, HasAKModeChanged, 0)
            STUB_METHOD(bool, GetIsActive, 0)
            STUB_METHOD(HRESULT, SetIsActive, 1(bool))
            STUB_METHOD(bool, ShouldForciblyExitAKMode, 0)
        END_MOCK

        MOCK_CLASS(MockTreeAnalyzer, Base)
            STUB_METHOD(MockCDOWithGetValue*, GetScopeOwner, 1(MockCDOWithGetValue*))
            STUB_METHOD(bool, IsScopeOwner, 1(MockCDOWithGetValue*))
            STUB_METHOD(bool, IsValidAKElement, 1(MockCDOWithGetValue*))
            STUB_METHOD(bool, IsAccessKey, 1(MockCDOWithGetValue*))
        END_MOCK

        MOCK_CLASS(MockFocusManager, Base)
            STUB_METHOD(MockCDOWithGetValue*, GetFocusedElementNoRef, 0)
            STUB_METHOD(FocusMovementResult, SetFocusedElement, 1(const FocusMovement&))
        END_MOCK

         MOCK_CLASS(MockPropertyChangedParams, Base)
            CDependencyProperty* m_pDP;
            CValue* m_pNewValue;
        END_MOCK

        typedef AKScopeTree<
            MockScope,
            MockScopeBuilder,
            MockModeContainer,
            MockCDOWithGetValue,
            MockTreeAnalyzer,
            MockFocusManager> ScopeTree;

        void ScopeTreeUnitTests::RootScopeIsCreated()
        {
            MockCDOWithGetValue element;

            MockScope scope;
            Expect(scope, GetElement)
                .ReturnValue(&element);
            Expect(scope, ShowAccessKeys)
                .Once()
                .ReturnValue(S_OK);

            MockScopeBuilder scopeBuilder;
            Expect(scopeBuilder, ConstructScope)
                .SetOutValue<1>(&scope)
                .ReturnValue(S_OK);

            MockModeContainer container;
            Expect(container, ShouldForciblyExitAKMode)
                .ReturnValue(false);

            BeginOrderedCalls();
                Expect(container, GetIsActive)
                    .ReturnValue(true);
                Expect(container, HasAKModeChanged)
                    .ReturnValue(true);
            EndOrderedCalls();

            MockTreeAnalyzer treeAnalyzer;

            ScopeTree tree(scopeBuilder, treeAnalyzer, container);

            bool wasInvoked = false;
            VERIFY_SUCCEEDED_WITHMOCKS(tree.ProcessCharacter(ALT, &wasInvoked));
            VERIFY_IS_FALSE(wasInvoked);

            VERIFY_EXPECTATIONS(container);
            VERIFY_EXPECTATIONS(scope);
            VERIFY_EXPECTATIONS(scopeBuilder);
            VERIFY_EXPECTATIONS(treeAnalyzer);
        }

        void ScopeTreeUnitTests::VerifyScopeWasInvoked()
        {
            MockCDOWithGetValue element;

            MockScope scope;
            Expect(scope, GetElement)
                .ReturnValue(&element);
            Expect(scope, Invoke)
                .Once()
                .SetOutValue<2>(&AKInvokeReturnParams<MockCDOWithGetValue>::invokeAttempted, true)
                .ReturnValue(S_OK);
            Expect(scope, ShowAccessKeys)
                .Once()
                .ReturnValue(S_OK);

            MockScopeBuilder scopeBuilder;
            Expect(scopeBuilder, ConstructScope)
                .SetOutValue<1>(&scope)
                .ReturnValue(S_OK);

            MockModeContainer container;
            Expect(container, ShouldForciblyExitAKMode)
                .ReturnValue(false);

            MockTreeAnalyzer treeAnalyzer;

            ScopeTree tree(scopeBuilder, treeAnalyzer, container);

            BeginOrderedCalls();
                Expect(container, GetIsActive)
                    .ReturnValue(true);  // Situation like pressing an alt key, should cause accesskeys to show.
                Expect(container, HasAKModeChanged)
                    .ReturnValue(true);
            EndOrderedCalls();

            bool wasInvoked = false;
            VERIFY_SUCCEEDED_WITHMOCKS(tree.ProcessCharacter((wchar_t)L'A', &wasInvoked));
            VERIFY_IS_TRUE(wasInvoked);

            VERIFY_EXPECTATIONS(container);
            VERIFY_EXPECTATIONS(scope);
            VERIFY_EXPECTATIONS(scopeBuilder);
        }

        void ScopeTreeUnitTests::ScopeAcceptsMulipleInputBeforeCreatingNewScope()
        {
            MockScope scope;
            Expect(scope, Invoke)
                .With(eq<wchar_t>(L'M'), eq<bool>(true))
                .SetOutValue<2>(&AKInvokeReturnParams<MockCDOWithGetValue>::invokeAttempted, (bool)false)
                .ReturnValue(S_OK);
            Expect(scope, Invoke)
                .With(eq<wchar_t>(L'H'), eq<bool>(true))
                .SetOutValue<2>(&AKInvokeReturnParams<MockCDOWithGetValue>::invokeAttempted, (bool)true)
                .ReturnValue(S_OK);
            Expect(scope, ShowAccessKeys)
                .Once()
                .ReturnValue(S_OK);

            MockScopeBuilder scopeBuilder;
            Expect(scopeBuilder, ConstructScope)
                .SetOutValue<1>(&scope)
                .Once()
                .ReturnValue(S_OK);

            MockModeContainer container;
            Expect(container, ShouldForciblyExitAKMode)
                .ReturnValue(false);

            Expect(container, GetIsActive)
                .ReturnValue(true);
            Expect(container, HasAKModeChanged)
                .ReturnValue(true)
                .ReturnValue(false)
                .ReturnValue(false);

            MockTreeAnalyzer treeAnalyzer;
            Expect(treeAnalyzer, IsScopeOwner)
                .ReturnValue(true);
            Expect(treeAnalyzer, IsValidAKElement)
                .ReturnValue(true);
            Expect(treeAnalyzer, IsAccessKey)
                .ReturnValue(true);
            ScopeTree tree(scopeBuilder, treeAnalyzer, container);

            bool wasInvoked = false;
            VERIFY_SUCCEEDED_WITHMOCKS(tree.ProcessCharacter((wchar_t)ALT, &wasInvoked));
            VERIFY_IS_FALSE(wasInvoked);

            VERIFY_SUCCEEDED_WITHMOCKS(tree.ProcessCharacter((wchar_t)L'M', &wasInvoked));
            VERIFY_IS_FALSE(wasInvoked);

            VERIFY_SUCCEEDED_WITHMOCKS(tree.ProcessCharacter((wchar_t)L'H', &wasInvoked));
            VERIFY_IS_TRUE(wasInvoked);

            VERIFY_EXPECTATIONS(container);
            VERIFY_EXPECTATIONS(scope);
            VERIFY_EXPECTATIONS(scopeBuilder);
            VERIFY_EXPECTATIONS(treeAnalyzer);
        }

        void ScopeTreeUnitTests::ScopeIsInvokedThroughHotkey()
        {
            MockScope scope;
            Expect(scope, Invoke)
                .With(eq<wchar_t>(L's'), eq<bool>(false))
                .SetOutValue<2>(&AKInvokeReturnParams<MockCDOWithGetValue>::invokeAttempted, true)
                .ReturnValue(S_OK);

            MockScopeBuilder scopeBuilder;
            Expect(scopeBuilder, ConstructScope)
                .SetOutValue<1>(&scope)
                .ReturnValue(S_OK);

            MockModeContainer container;
            Expect(container, ShouldForciblyExitAKMode)
                .ReturnValue(false);

            Expect(container, GetIsActive)
                .ReturnValue(false);
            Expect(container, HasAKModeChanged)
                .ReturnValue(true);

            MockTreeAnalyzer treeAnalyzer;

            ScopeTree tree(scopeBuilder, treeAnalyzer, container);

            bool handled = false;
            VERIFY_SUCCEEDED_WITHMOCKS(tree.ProcessCharacter(L's', &handled));
            VERIFY_IS_TRUE(handled);

            VERIFY_EXPECTATIONS(container);
            VERIFY_EXPECTATIONS(scope);
            VERIFY_EXPECTATIONS(scopeBuilder);
            VERIFY_EXPECTATIONS(treeAnalyzer);
        }

        void ScopeTreeUnitTests::ModeContainerMustBeActiveToUpdateScope()
        {
            MockScope scope;

            MockScopeBuilder scopeBuilder;
            Expect(scopeBuilder, ConstructScope)
                .SetOutValue<1>(&scope)
                .ReturnValue(S_OK);
            MockModeContainer container;
            Expect(container, ShouldForciblyExitAKMode)
                .ReturnValue(false);

            Expect(container, GetIsActive)
                .Once()
                .ReturnValue(false);

            MockTreeAnalyzer treeAnalyzer;

            ScopeTree tree(scopeBuilder, treeAnalyzer, container);

            VERIFY_SUCCEEDED_WITHMOCKS(tree.UpdateScope());

            VERIFY_EXPECTATIONS(container);
            VERIFY_EXPECTATIONS(scopeBuilder);
            VERIFY_EXPECTATIONS(scope);
        }

        void ScopeTreeUnitTests::UpdateScopeExitsAndEnters()
        {
            MockScope rootScope;
            Expect(rootScope, ShowAccessKeys)
                .Once()
                .ReturnValue(S_OK);
            Expect(rootScope, HideAccessKeys)
                .Once()
                .ReturnValue(S_OK);
            Expect(rootScope, GetScopeParent)
                .ReturnValue(xref::weakref_ptr<MockCDOWithGetValue>());

            MockScope elementScope;
            Expect(elementScope, ShowAccessKeys)
                .Once()
                .ReturnValue(S_OK);
            Expect(elementScope, HideAccessKeys)
                .CallCount(0)
                .ReturnValue(E_FAIL);

            MockCDOWithGetValue element;

            MockScopeBuilder scopeBuilder;
            MockTreeAnalyzer treeAnalyzer;

            MockModeContainer container;
            Expect(container, ShouldForciblyExitAKMode)
                .ReturnValue(false);

            Expect(container, GetIsActive)
                .ReturnValue(true);

            Expect(scopeBuilder, ConstructScope)
                .With(eq<MockCDOWithGetValue* const>(nullptr))
                .SetOutValue<1>(&rootScope)
                .ReturnValue(S_OK);
            Expect(treeAnalyzer, GetScopeOwner)
                .With(eq<MockCDOWithGetValue*>(nullptr))
                .ReturnValue(nullptr);

            ScopeTree tree(scopeBuilder, treeAnalyzer, container);

            LOG_OUTPUT(L"Entering root scope");
            VERIFY_SUCCEEDED_WITHMOCKS(tree.UpdateScope());

            Expect(scopeBuilder, ConstructScope)
                .With(eq<MockCDOWithGetValue* const>(&element))
                .SetOutValue<1>(&elementScope)
                .ReturnValue(S_OK);
            Expect(treeAnalyzer, GetScopeOwner)
                .With(eq<MockCDOWithGetValue*>(&element))
                .ReturnValue(&element);

            MockFocusManager focusManager;
            tree.SetFocusManager(&focusManager);
            Expect(focusManager, GetFocusedElementNoRef)
                .ReturnValue(&element);

            LOG_OUTPUT(L"Entering element scope");
            VERIFY_SUCCEEDED_WITHMOCKS(tree.UpdateScope());

            LOG_OUTPUT(L"Entered element scope");

            VERIFY_EXPECTATIONS(rootScope);
            VERIFY_EXPECTATIONS(elementScope);
            VERIFY_EXPECTATIONS(container);
            VERIFY_EXPECTATIONS(scopeBuilder);
            VERIFY_EXPECTATIONS(element);
            VERIFY_EXPECTATIONS(focusManager);
        }

        void ScopeTreeUnitTests::MnemonicsExitedOnEscapeInRootScope()
        {
            MockCDOWithGetValue element;

            MockScope rootScope;
            Expect(rootScope, GetElement)
                .ReturnValue(&element);
            Expect(rootScope, GetScopeParent)
                .ReturnValue(xref::weakref_ptr<MockCDOWithGetValue>())
                .AtLeastOnce();
            Expect(rootScope, HideAccessKeys)
                .ReturnValue(S_OK)
                .Once();
            Expect(rootScope, ShowAccessKeys)
                .ReturnValue(S_OK)
                .Once();
            Expect(rootScope, IsScopeFilteringInput)
                .ReturnValue(false);


            MockScopeBuilder scopeBuilder;
            Expect(scopeBuilder, ConstructScope)
                .With(eq<MockCDOWithGetValue* const>(nullptr))
                .SetOutValue<1>(&rootScope)
                .ReturnValue(S_OK);

            MockTreeAnalyzer treeAnalyzer;

            MockModeContainer container;
            Expect(container, ShouldForciblyExitAKMode)
                .ReturnValue(false);

            BeginOrderedCalls();
                Expect(container, GetIsActive)
                    .ReturnValue(true)
                    .ReturnValue(true)
                    .ReturnValue(true)
                    .ReturnValue(false);
                Expect(container, HasAKModeChanged)
                    .ReturnValue(true)
                    .ReturnValue(false);
            EndOrderedCalls();
            Expect(container, SetIsActive)
                .With(false)
                .ReturnValue(S_OK)
                .Once();

            ScopeTree tree(scopeBuilder, treeAnalyzer, container);
            bool wasInvoked = false;

            VERIFY_SUCCEEDED_WITHMOCKS(tree.ProcessCharacter(ALT, &wasInvoked));
            VERIFY_IS_FALSE(wasInvoked);

            // InputInterceptor will only send escape to scope tree if already in mnemonics mode (otherwise it just ignores the character).
            VERIFY_SUCCEEDED_WITHMOCKS(tree.ProcessCharacter(ESC, &wasInvoked));
            VERIFY_IS_FALSE(wasInvoked);

            VERIFY_EXPECTATIONS(rootScope);
            VERIFY_EXPECTATIONS(scopeBuilder);
            VERIFY_EXPECTATIONS(container);
            VERIFY_EXPECTATIONS(element);
        }

        void ScopeTreeUnitTests::ExitingWithAltClosesKeytips()
        {
            MockCDOWithGetValue element;

            MockScope scope;
            Expect(scope, GetElement)
                .ReturnValue(&element);
            Expect(scope, HideAccessKeys)
                .ReturnValue(S_OK)
                .Once();
            Expect(scope, ShowAccessKeys)
                .ReturnValue(S_OK)
                .Once();

            MockScopeBuilder scopeBuilder;
            Expect(scopeBuilder, ConstructScope)
                .SetOutValue<1>(&scope)
                .ReturnValue(S_OK);

            MockModeContainer container;
            Expect(container, ShouldForciblyExitAKMode)
                .ReturnValue(false);

            BeginOrderedCalls();
            Expect(container, GetIsActive)
                .ReturnValue(true)
                .ReturnValue(true)
                .ReturnValue(false)
                .ReturnValue(false);
            Expect(container, HasAKModeChanged)
                .ReturnValue(true)
                .ReturnValue(true);
            EndOrderedCalls();

            MockTreeAnalyzer treeAnalyzer;

            ScopeTree tree(scopeBuilder, treeAnalyzer, container);

            bool wasInvoked = false;
            VERIFY_SUCCEEDED_WITHMOCKS(tree.ProcessCharacter(ALT, &wasInvoked));
            VERIFY_IS_FALSE(wasInvoked);

            VERIFY_SUCCEEDED_WITHMOCKS(tree.ProcessCharacter(ALT, &wasInvoked));
            VERIFY_IS_FALSE(wasInvoked);

            VERIFY_EXPECTATIONS(container);
            VERIFY_EXPECTATIONS(scope);
            VERIFY_EXPECTATIONS(scopeBuilder);
        }

        class Callback : public ICommand
        {
            std::function<void ()> m_callback;
        public:
            Callback(std::function<void ()> callback) : m_callback(callback)
            {
            }

            virtual void Exec()
            {
                m_callback();
            }
        };

        void ScopeTreeUnitTests::InvokeCanEnterScope()
        {
            MockCDOWithGetValue rootScopeElement;

            MockScope rootScope;

            MockScopeBuilder scopeBuilder;
            Expect(scopeBuilder, ConstructScope)
                .With(eq<MockCDOWithGetValue*>(&rootScopeElement))
                .SetOutValue<1>(&rootScope)
                .ReturnValue(S_OK);

            MockModeContainer container;
            Expect(container, ShouldForciblyExitAKMode)
                .ReturnValue(false);

            Expect(container, GetIsActive)
                .ReturnValue(true);
            Expect(container, HasAKModeChanged)
                .ReturnValue(false);

            MockTreeAnalyzer treeAnalyzer;
            Expect(treeAnalyzer, GetScopeOwner)
                .With(eq<MockCDOWithGetValue*>(&rootScopeElement))
                .ReturnValue(&rootScopeElement);

            ScopeTree tree(scopeBuilder, treeAnalyzer, container);

            Expect(rootScope, ShowAccessKeys)
                .ReturnValue(S_OK);

            MockFocusManager focusManager;
            tree.SetFocusManager(&focusManager);

            MockCDOWithGetValue innerScopeElement;
            Expect(innerScopeElement, Release)
                .Once();

            Expect(focusManager, GetFocusedElementNoRef)
                .ReturnValue(&rootScopeElement)
                .ReturnValue(&innerScopeElement);

            VERIFY_SUCCEEDED_WITHMOCKS(tree.UpdateScope());

            MockScope subScope;

            Expect(scopeBuilder, ConstructScope)
                .With(eq<MockCDOWithGetValue*>(&innerScopeElement))
                .SetOutValue<1>(&subScope)
                .ReturnValue(S_OK);
            Expect(rootScope, GetScopeParent)
                .ReturnValue(xref::weakref_ptr<MockCDOWithGetValue>());
            Expect(rootScope, HideAccessKeys)
                .ReturnValue(S_OK);
            Expect(treeAnalyzer, GetScopeOwner)
                .With(eq<MockCDOWithGetValue*>(&innerScopeElement))
                .ReturnValue(&innerScopeElement);
            Expect(rootScope, Invoke)
                .SetOutValue<2>(&AKInvokeReturnParams<MockCDOWithGetValue>::invokeAttempted, (bool)true)
                .SetOutValue<2>(&AKInvokeReturnParams<MockCDOWithGetValue>::invokedElement, xref::weakref_ptr<MockCDOWithGetValue>(&innerScopeElement))
                .ReturnValue(S_OK);
            Expect(treeAnalyzer, IsScopeOwner)
                .ReturnValue(true);
            Expect(treeAnalyzer, IsValidAKElement)
                .ReturnValue(true);
            Expect(treeAnalyzer, IsAccessKey)
                .ReturnValue(true);

            Expect(subScope, ShowAccessKeys)
                .ReturnValue(S_OK);

            bool wasInvoked = false;

            VERIFY_IS_FALSE(rootScope.IsDeleted);
            VERIFY_IS_FALSE(subScope.IsDeleted);
            VERIFY_SUCCEEDED_WITHMOCKS(tree.ProcessCharacter(L'S', &wasInvoked));
            VERIFY_IS_TRUE(rootScope.IsDeleted);
            VERIFY_IS_FALSE(subScope.IsDeleted);
        }

        void ScopeTreeUnitTests::PropertyChangedAddsToTree()
        {
            MockCDOWithGetValue element;

            MockScope scope;
            Expect(scope, AddToAccessKeyOwner)
                .Once()
                .ReturnValue(S_OK);
            Expect(scope, GetScopeParent)
                .ReturnValue(xref::weakref_ptr<MockCDOWithGetValue>());
            Expect(scope, ShowAccessKeys)
                .ReturnValue(S_OK);
            Expect(scope, ShouldElementEnteringTreeUpdateScope)
                .ReturnValue(false);

            MockScopeBuilder scopeBuilder;
            Expect(scopeBuilder, ConstructScope)
                .With(eq<MockCDOWithGetValue*>(nullptr))
                .SetOutValue<1>(&scope)
                .ReturnValue(S_OK);

            MockTreeAnalyzer treeAnalyzer;
            Expect(treeAnalyzer, GetScopeOwner)
                .ReturnValue(nullptr);
            Expect(treeAnalyzer, IsValidAKElement)
                .ReturnValue(true);
            Expect(treeAnalyzer, IsAccessKey)
                .ReturnValue(true);

            MockFocusManager focusManager;
            Expect(focusManager, GetFocusedElementNoRef)
                .ReturnValue(nullptr);

            MockModeContainer container;
            Expect(container, GetIsActive)
                .ReturnValue(true);

            ScopeTree tree(scopeBuilder, treeAnalyzer, container);
            tree.SetFocusManager(&focusManager);

            //We need to set the scope first
            VERIFY_SUCCEEDED_WITHMOCKS(tree.UpdateScope());

            VERIFY_SUCCEEDED_WITHMOCKS(tree.OnVisibilityChanged(&element, DirectUI::Visibility::Visible));
            VERIFY_EXPECTATIONS(scope);
        }

        void ScopeTreeUnitTests::PropertyChangedRemovesFromTree()
        {
            MockCDOWithGetValue element;

            MockScope scope;
            Expect(scope, RemoveFromAccessKeyOwner)
                .Once()
                .ReturnValue(S_OK);
            Expect(scope, GetScopeParent)
                .ReturnValue(xref::weakref_ptr<MockCDOWithGetValue>());
            Expect(scope, ShowAccessKeys)
                .ReturnValue(S_OK);

            MockScopeBuilder scopeBuilder;
            Expect(scopeBuilder, ConstructScope)
                .With(eq<MockCDOWithGetValue*>(nullptr))
                .SetOutValue<1>(&scope)
                .ReturnValue(S_OK);

            MockTreeAnalyzer treeAnalyzer;
            Expect(treeAnalyzer, GetScopeOwner)
                .ReturnValue(nullptr);
            Expect(treeAnalyzer, IsValidAKElement)
                .ReturnValue(true);
            Expect(treeAnalyzer, IsAccessKey)
                .ReturnValue(true);

            MockFocusManager focusManager;
            Expect(focusManager, GetFocusedElementNoRef)
                .ReturnValue(nullptr);

            MockModeContainer container;
            Expect(container, GetIsActive)
                .ReturnValue(true);

            ScopeTree tree(scopeBuilder, treeAnalyzer, container);
            tree.SetFocusManager(&focusManager);

            //We need to set the scope first
            VERIFY_SUCCEEDED_WITHMOCKS(tree.UpdateScope());

            VERIFY_SUCCEEDED_WITHMOCKS(tree.OnIsEnabledChanged(&element, false));
            VERIFY_EXPECTATIONS(scope);
        }
    }
}}}}
