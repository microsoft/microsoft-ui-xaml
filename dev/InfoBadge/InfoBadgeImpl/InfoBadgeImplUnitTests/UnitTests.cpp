// USAGE EXAMPLE
#include "pch.h"
#include <CppUnitTest.h>
#include <InfoBadgeImpl.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft {
    namespace VisualStudio {
        namespace CppUnitTestFramework {

            template<>
            static std::wstring ToString<winrt::Size>(const winrt::Size& size) {
                return L"{" + std::to_wstring(size.Width) + L", " + std::to_wstring(size.Height) + L"}";
            }

        }
    }
}

BEGIN_TEST_MODULE_ATTRIBUTE()
TEST_MODULE_ATTRIBUTE(L"Control", L"InfoBadge")
END_TEST_MODULE_ATTRIBUTE()

TEST_MODULE_INITIALIZE(ModuleInitialize)
{
    Logger::WriteMessage("In InfoBadge Module Initialize");
}

TEST_MODULE_CLEANUP(ModuleCleanup)
{
    Logger::WriteMessage("In InfoBadge Module Cleanup");
}

TEST_CLASS(InfoBadgeImplTests)
{

public:

    InfoBadgeImplTests()
    {
        Logger::WriteMessage("In InfoBadgeImplTests");
    }

    ~InfoBadgeImplTests()
    {
        Logger::WriteMessage("In ~InfoBadgeImplTests");
    }

    TEST_CLASS_INITIALIZE(InfoBadgeImplTestsInitialize)
    {
        Logger::WriteMessage("In InfoBadgeImplTests Initialize");
    }

    TEST_CLASS_CLEANUP(InfoBadgeImplTestsCleanup)
    {
        Logger::WriteMessage("In InfoBadgeImplTests Cleanup");
    }

    BEGIN_TEST_METHOD_ATTRIBUTE(MeasureOverrideImpl_EqualHeightAndWidth)
        TEST_METHOD_ATTRIBUTE(L"ImplMethod", L"MeasureOverrideImpl")
        TEST_DESCRIPTION(L"MeasureOverrideImpl should not change values that are equal")
        END_TEST_METHOD_ATTRIBUTE()

    TEST_METHOD(MeasureOverrideImpl_EqualHeightAndWidth)
    {
        Assert::AreEqual(winrt::Size{ 0,0 }, InfoBadgeImpl::MeasureOverrideImpl(winrt::Size{ 0,0 }));
        Assert::AreEqual(winrt::Size{ 10,10 }, InfoBadgeImpl::MeasureOverrideImpl(winrt::Size{ 10,10 }));
        Assert::AreEqual(winrt::Size{ -10,-10 }, InfoBadgeImpl::MeasureOverrideImpl(winrt::Size{ -10,-10 }));
    }

    BEGIN_TEST_METHOD_ATTRIBUTE(MeasureOverrideImpl_GreaterHeight)
        TEST_METHOD_ATTRIBUTE(L"ImplMethod", L"MeasureOverrideImpl")
        TEST_DESCRIPTION(L"MeasureOverrideImpl should change the width to be as large as the height when it is higher so we can fully round the rectangle.")
        END_TEST_METHOD_ATTRIBUTE()

        TEST_METHOD(MeasureOverrideImpl_GreaterHeight)
    {
        Assert::AreEqual(winrt::Size{ 10,10 }, InfoBadgeImpl::MeasureOverrideImpl(winrt::Size{ 0,10 }));
        Assert::AreEqual(winrt::Size{ 10,10 }, InfoBadgeImpl::MeasureOverrideImpl(winrt::Size{ 5,10 }));
        Assert::AreEqual(winrt::Size{ 10,10 }, InfoBadgeImpl::MeasureOverrideImpl(winrt::Size{ -10,10 }));
    }

    BEGIN_TEST_METHOD_ATTRIBUTE(MeasureOverrideImpl_GreaterWidth)
        TEST_METHOD_ATTRIBUTE(L"ImplMethod", L"MeasureOverrideImpl")
        TEST_DESCRIPTION(L"MeasureOverrideImpl should not change values when width is larger because we can still round the rectangle in that case.")
        END_TEST_METHOD_ATTRIBUTE()

        TEST_METHOD(MeasureOverrideImpl_GreaterWidth)
    {
        Assert::AreEqual(winrt::Size{ 10,0 }, InfoBadgeImpl::MeasureOverrideImpl(winrt::Size{ 10,0 }));
        Assert::AreEqual(winrt::Size{ 10,5 }, InfoBadgeImpl::MeasureOverrideImpl(winrt::Size{ 10,5 }));
        Assert::AreEqual(winrt::Size{ 10,-10 }, InfoBadgeImpl::MeasureOverrideImpl(winrt::Size{ 10,-10 }));
    }

    TEST_METHOD(Method2)
    {
        Assert::Fail(L"Fail");
    }
};
