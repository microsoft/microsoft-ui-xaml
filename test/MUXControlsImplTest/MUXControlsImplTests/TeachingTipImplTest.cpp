#include "pch.h"
#include <CppUnitTest.h>
#include <TeachingTipImpl.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft {
    namespace VisualStudio {
        namespace CppUnitTestFramework {

            template<>
            std::wstring ToString<winrt::hstring>(const winrt::hstring& hstring) {
                return hstring.data();
            }

            template<>
            std::wstring ToString<TeachingTipLightDismissStates>(const TeachingTipLightDismissStates& state) {
                return TeachingTipTemplateHelpers::ToString(state).data();
            }

            template<>
            std::wstring ToString<TeachingTipButtonsStates>(const TeachingTipButtonsStates& state) {
                return TeachingTipTemplateHelpers::ToString(state).data();
            }

            template<>
            std::wstring ToString<TeachingTipContentStates>(const TeachingTipContentStates& state) {
                return TeachingTipTemplateHelpers::ToString(state).data();
            }

            template<>
            std::wstring ToString<TeachingTipCloseButtonLocations>(const TeachingTipCloseButtonLocations& state) {
                return TeachingTipTemplateHelpers::ToString(state).data();
            }

            template<>
            std::wstring ToString<TeachingTipIconStates>(const TeachingTipIconStates& state) {
                return TeachingTipTemplateHelpers::ToString(state).data();
            }
            template<>
            std::wstring ToString<TeachingTipHeroContentPlacementStates>(const TeachingTipHeroContentPlacementStates& state) {
                return TeachingTipTemplateHelpers::ToString(state).data();
            }
            template<>
            std::wstring ToString<TeachingTipPlacementStates>(const TeachingTipPlacementStates& state) {
                return TeachingTipTemplateHelpers::ToString(state).data();
            }

            template<>
            std::wstring ToString<TeachingTipTitleBlockStates>(const TeachingTipTitleBlockStates& state) {
                return TeachingTipTemplateHelpers::ToString(state).data();
            }

            template<>
            std::wstring ToString<TeachingTipSubtitleBlockStates>(const TeachingTipSubtitleBlockStates& state) {
                return TeachingTipTemplateHelpers::ToString(state).data();
            }
        }
    }
}

namespace UnitTests {
    TEST_CLASS(TeachingTipImplTests)
    {
    public:
        TeachingTipImplTests()
        {
            Logger::WriteMessage("In TeachingTipImplTests");
        }

        ~TeachingTipImplTests()
        {
            Logger::WriteMessage("In ~TeachingTipImplTests");
        }

        TEST_CLASS_INITIALIZE(TeachingTipImplTestsInitialize)
        {
            Logger::WriteMessage("In TeachingTipImplTests Initialize");
        }

        TEST_CLASS_CLEANUP(TeachingTipImplTestsCleanup)
        {
            Logger::WriteMessage("In TeachingTipImplTests Cleanup");
        }

        BEGIN_TEST_METHOD_ATTRIBUTE(GetPopupAutomationNameImpl)
            TEST_METHOD_ATTRIBUTE(L"ImplMethod", L"GetPopupAutomationNameImpl")
            TEST_DESCRIPTION(L"GetPopupAutomationNameImpl should return the automationName unless it is null or empty.")
            END_TEST_METHOD_ATTRIBUTE()

            TEST_METHOD(GetPopupAutomationNameImpl)
        {
            Assert::AreEqual(winrt::hstring{ L"automationName" }, TeachingTipImpl::GetPopupAutomationNameImpl(L"automationName", L"title"));
            Assert::AreEqual(winrt::hstring{ L"automationName" }, TeachingTipImpl::GetPopupAutomationNameImpl(L"automationName", L""));
            Assert::AreEqual(winrt::hstring{ L"title" }, TeachingTipImpl::GetPopupAutomationNameImpl(L"", L"title"));
            Assert::AreEqual(winrt::hstring{ L"" }, TeachingTipImpl::GetPopupAutomationNameImpl(L"", L""));
        }

        BEGIN_TEST_METHOD_ATTRIBUTE(GetContentStateImpl)
            TEST_METHOD_ATTRIBUTE(L"ImplMethod", L"GetContentStateImpl")
            TEST_DESCRIPTION(L"GetContentStateImpl should return the the content state if the content is non-null.")
            END_TEST_METHOD_ATTRIBUTE()

            TEST_METHOD(GetContentStateImpl)
        {
            Assert::AreEqual(TeachingTipContentStates::Content, TeachingTipImpl::GetContentStateImpl(winrt::box_value(1)));
            Assert::AreEqual(TeachingTipContentStates::NoContent, TeachingTipImpl::GetContentStateImpl(nullptr));
        }
    };
}
