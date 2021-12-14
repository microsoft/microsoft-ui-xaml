#include "pch.h"
#include <CppUnitTest.h>
#include <InfoBadgeImpl.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft {
    namespace VisualStudio {
        namespace CppUnitTestFramework {

            template<>
            std::wstring ToString<winrt::Size>(const winrt::Size& size) {
                return L"{" + std::to_wstring(size.Width) + L", " + std::to_wstring(size.Height) + L"}";
            }

            template<>
            std::wstring ToString<InfoBadgeDisplayKindStates>(const InfoBadgeDisplayKindStates& state) {
                return InfoBadgeTemplateHelpers::ToString(state).data();
            }

            template<>
            std::wstring ToString<std::tuple<double, double, double, double>>(const std::tuple<double, double, double, double>& values) {
                return L"(" + std::to_wstring(std::get<0>(values)) + L", "
                    + std::to_wstring(std::get<1>(values)) + L", "
                    + std::to_wstring(std::get<2>(values)) + L", "
                    + std::to_wstring(std::get<3>(values)) + L")";
            }
        }
    }
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

    BEGIN_TEST_METHOD_ATTRIBUTE(ValidateValuePropertyImpl_LessThanNegativeOne)
        TEST_METHOD_ATTRIBUTE(L"ImplMethod", L"ValidateValuePropertyImpl")
        TEST_DESCRIPTION(L"ValidateValuePropertyImpl throws when the value is less than -1.")
        END_TEST_METHOD_ATTRIBUTE()

        TEST_METHOD(ValidateValuePropertyImpl_LessThanNegativeOne)
    {
        auto const invalidValueCall = [] { InfoBadgeImpl::ValidateValuePropertyImpl(-2); };
        Assert::ExpectException<winrt::hresult_out_of_bounds>(invalidValueCall);
    }

    BEGIN_TEST_METHOD_ATTRIBUTE(ValidateValuePropertyImpl_NegativeOneAndGreater)
        TEST_METHOD_ATTRIBUTE(L"ImplMethod", L"ValidateValuePropertyImpl")
        TEST_DESCRIPTION(L"ValidateValuePropertyImpl no opts for -1 and higher")
        END_TEST_METHOD_ATTRIBUTE()

        TEST_METHOD(ValidateValuePropertyImpl_NegativeOneAndGreater)
    {
        InfoBadgeImpl::ValidateValuePropertyImpl(-1);
        InfoBadgeImpl::ValidateValuePropertyImpl(0);
        InfoBadgeImpl::ValidateValuePropertyImpl(1);
    }


    BEGIN_TEST_METHOD_ATTRIBUTE(CalculateAppropriateDisplayKindStateImpl_StandardInputs)
        TEST_METHOD_ATTRIBUTE(L"ImplMethod", L"CalculateAppropriateDisplayKindStateImpl")
        TEST_DESCRIPTION(L"CalculateAppropriateDisplayKindStateImpl Prioritizes Value if we have a non-sentinal value, followed by an icon state if we have an icon, followed by the Dot state if we have neither.")
        END_TEST_METHOD_ATTRIBUTE()

        TEST_METHOD(CalculateAppropriateDisplayKindStateImpl_StandardInputs)
    {
        Assert::AreEqual(InfoBadgeDisplayKindStates::Dot, InfoBadgeImpl::CalculateAppropriateDisplayKindStateImpl(-1, false, false));
        Assert::AreEqual(InfoBadgeDisplayKindStates::Icon, InfoBadgeImpl::CalculateAppropriateDisplayKindStateImpl(-1, true, false));
        Assert::AreEqual(InfoBadgeDisplayKindStates::FontIcon, InfoBadgeImpl::CalculateAppropriateDisplayKindStateImpl(-1, true, true));
        Assert::AreEqual(InfoBadgeDisplayKindStates::Value, InfoBadgeImpl::CalculateAppropriateDisplayKindStateImpl(0, false, false));
        Assert::AreEqual(InfoBadgeDisplayKindStates::Value, InfoBadgeImpl::CalculateAppropriateDisplayKindStateImpl(0, true, false));
        Assert::AreEqual(InfoBadgeDisplayKindStates::Value, InfoBadgeImpl::CalculateAppropriateDisplayKindStateImpl(0, true, true));
        Assert::AreEqual(InfoBadgeDisplayKindStates::Value, InfoBadgeImpl::CalculateAppropriateDisplayKindStateImpl(1, false, false));
        Assert::AreEqual(InfoBadgeDisplayKindStates::Value, InfoBadgeImpl::CalculateAppropriateDisplayKindStateImpl(1, true, false));
        Assert::AreEqual(InfoBadgeDisplayKindStates::Value, InfoBadgeImpl::CalculateAppropriateDisplayKindStateImpl(1, true, true));
    }

    BEGIN_TEST_METHOD_ATTRIBUTE(CalculateAppropriateDisplayKindStateImpl_NonValidValueInput)
        TEST_METHOD_ATTRIBUTE(L"ImplMethod", L"CalculateAppropriateDisplayKindStateImpl")
        TEST_DESCRIPTION(L"CalculateAppropriateDisplayKindStateImpl should not receive a value parameter less than -1, but if it does, it is treated the same as the sentinal value.")
        END_TEST_METHOD_ATTRIBUTE()

        TEST_METHOD(CalculateAppropriateDisplayKindStateImpl_NonValidValueInput)
    {
        Assert::AreEqual(InfoBadgeDisplayKindStates::FontIcon, InfoBadgeImpl::CalculateAppropriateDisplayKindStateImpl(-2, true, true));
        Assert::AreEqual(InfoBadgeDisplayKindStates::Icon, InfoBadgeImpl::CalculateAppropriateDisplayKindStateImpl(-2, true, false));
        Assert::AreEqual(InfoBadgeDisplayKindStates::FontIcon, InfoBadgeImpl::CalculateAppropriateDisplayKindStateImpl(-2, true, true));
    }

    BEGIN_TEST_METHOD_ATTRIBUTE(CalculateAppropriateDisplayKindStateImpl_NonValidBooleanInputs)
        TEST_METHOD_ATTRIBUTE(L"ImplMethod", L"CalculateAppropriateDisplayKindStateImpl")
        TEST_DESCRIPTION(L"CalculateAppropriateDisplayKindStateImpl should not receive a false for its iconExists parameter but true for its iconIsFontIcon parameter, but if it does, the iconIsFontIcon parameter is ignored.")
        END_TEST_METHOD_ATTRIBUTE()

        TEST_METHOD(CalculateAppropriateDisplayKindStateImpl_NonValidBooleanInputs)
    {
        Assert::AreEqual(InfoBadgeDisplayKindStates::Dot, InfoBadgeImpl::CalculateAppropriateDisplayKindStateImpl(-2, false, true));
        Assert::AreEqual(InfoBadgeDisplayKindStates::Dot, InfoBadgeImpl::CalculateAppropriateDisplayKindStateImpl(-1, false, true));
        Assert::AreEqual(InfoBadgeDisplayKindStates::Value, InfoBadgeImpl::CalculateAppropriateDisplayKindStateImpl(0, false, true));
        Assert::AreEqual(InfoBadgeDisplayKindStates::Value, InfoBadgeImpl::CalculateAppropriateDisplayKindStateImpl(1, false, true));
    }

    BEGIN_TEST_METHOD_ATTRIBUTE(GetFullyRoundedCornerRadiusValueImpl_ValidHeight)
        TEST_METHOD_ATTRIBUTE(L"ImplMethod", L"GetFullyRoundedCornerRadiusValueImpl")
        TEST_DESCRIPTION(L"InfoBadge fully rounds its corners based off its height, so GetFullyRoundedCornerRadiusValueImpl should devide the provided height by 2 and push that value to each corner.")
        END_TEST_METHOD_ATTRIBUTE()

        TEST_METHOD(GetFullyRoundedCornerRadiusValueImpl_ValidHeight)
    {
        Assert::AreEqual(std::make_tuple(5.0, 5.0, 5.0, 5.0), InfoBadgeImpl::GetFullyRoundedCornerRadiusValueImpl(10));
        Assert::AreEqual(std::make_tuple(0.0, 0.0, 0.0, 0.0), InfoBadgeImpl::GetFullyRoundedCornerRadiusValueImpl(0));
    }

    BEGIN_TEST_METHOD_ATTRIBUTE(GetFullyRoundedCornerRadiusValueImpl_InvalidHeight)
        TEST_METHOD_ATTRIBUTE(L"ImplMethod", L"GetFullyRoundedCornerRadiusValueImpl")
        TEST_DESCRIPTION(L"It shouldn't be possible for GetFullyRoundedCornerRadiusValueImpl to be passed a negative height, but the implementation treats it as normal.")
        END_TEST_METHOD_ATTRIBUTE()

        TEST_METHOD(GetFullyRoundedCornerRadiusValueImpl_InvalidHeight)
    {
        Assert::AreEqual(std::make_tuple(-5.0, -5.0, -5.0, -5.0), InfoBadgeImpl::GetFullyRoundedCornerRadiusValueImpl(-10));
    }
};
