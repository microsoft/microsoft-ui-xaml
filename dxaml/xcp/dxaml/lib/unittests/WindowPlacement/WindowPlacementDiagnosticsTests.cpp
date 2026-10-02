// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <Windows.h>
#include <WexTestClass.h>
#include <TraceLoggingProvider.h>
#include "WindowPlacementApplication.h"
#include "WindowPlacementStore.h"

// The isolated DLL does not link the product's registered provider.
TRACELOGGING_DEFINE_PROVIDER(g_hTraceProvider, "WinUI.WindowPlacement.Tests",
    (0xad9ac24c, 0xfba1, 0x469c, 0xa1, 0xdf, 0xf2, 0x91, 0xa8, 0x96, 0x2a, 0x28));

using namespace DirectUI::WindowPlacementPersistence;

namespace
{
    struct Reports
    {
        inline static thread_local unsigned Count{};
        inline static thread_local PlacementOperation Operation{};
        inline static thread_local PlacementFailure Failure{};
        PlacementFailureObserver Previous;

        Reports() : Previous(SetPlacementFailureObserverForTesting(Observe)) { Count = 0; }
        ~Reports() { SetPlacementFailureObserverForTesting(Previous); }

        static void Observe(PlacementOperation operation, PlacementFailureCategory category, HRESULT error) noexcept
        {
            ++Count;
            Operation = operation;
            Failure = {category, error};
        }

        static void Verify(PlacementOperation operation, PlacementFailureCategory category, HRESULT error)
        {
            VERIFY_ARE_EQUAL(1u, Count);
            VERIFY_IS_TRUE(Operation == operation);
            VERIFY_IS_TRUE(Failure.Category == category);
            VERIFY_ARE_EQUAL(error, Failure.Error);
        }
    };

    class Backend final : public IPlacementSettingsBackend
    {
    public:
        StorageOperationResult Read{StorageResult::Missing, S_OK};
        StorageOperationResult Write{StorageResult::Success, S_OK};
        StorageOperationResult Enumerate{StorageResult::Missing, S_OK};
        StoredValue Value;
        unsigned Writes{};

        StorageOperationResult ReadValue(const std::u16string&, const std::u16string&, StoredValue& value) override
        {
            value = Value;
            return Read;
        }
        StorageOperationResult ReplaceValue(
            const std::u16string&, const std::u16string&, const std::u16string&) override
        {
            ++Writes;
            return Write;
        }
        StorageOperationResult EnumerateValues(const std::u16string&, std::vector<StoredValue>&) override
        {
            return Enumerate;
        }
        StorageOperationResult DeleteValue(const std::u16string&, const std::u16string&) override
        {
            return {StorageResult::Success, S_OK};
        }
    };

    HRESULT Identity(std::u16string& id) noexcept
    {
        id = u"private application identity";
        return S_OK;
    }

    Snapshot Sample()
    {
        Snapshot placement;
        placement.NormalRect = {10, 10, 200, 100};
        placement.WorkArea = {0, 0, 1920, 1080};
        placement.Dpi = 96;
        placement.DisplayDeviceName = u"private device name";
        return placement;
    }

    HRESULT Load(Backend& backend, LoadResult& result)
    {
        return ReadPersistPlacement(backend, u"private app", 11, u"private id", 10, result);
    }

    bool Save(Backend& backend, const Snapshot* placement)
    {
        return SavePersistPlacement(backend, placement, u"private id", 10, Identity);
    }

    class TestWindow
    {
    public:
        TestWindow()
        {
            WNDCLASSW wc{};
            wc.lpfnWndProc = WindowProc;
            wc.hInstance = ::GetModuleHandleW(nullptr);
            wc.lpszClassName = L"WindowPlacementDiagnosticsTests";
            ::RegisterClassW(&wc);
            Handle = ::CreateWindowExW(0, wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW,
                100, 100, 400, 300, nullptr, nullptr, wc.hInstance, this);
            VERIFY_IS_NOT_NULL(Handle);
        }
        ~TestWindow() { if (::IsWindow(Handle)) ::DestroyWindow(Handle); }
        HWND Handle{};
        bool DestroyOnConstraints{};

    private:
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
        {
            auto self = reinterpret_cast<TestWindow*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (message == WM_NCCREATE)
            {
                self = static_cast<TestWindow*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
                ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
            if (message == WM_GETMINMAXINFO && self && self->DestroyOnConstraints)
            {
                ::DestroyWindow(hwnd);
                return 0;
            }
            return ::DefWindowProcW(hwnd, message, wParam, lParam);
        }
    };

    long Missing(const char16_t*, size_t, LoadResult& result)
    {
        result.Status = LoadStatus::Missing;
        result.Error = S_OK;
        return S_OK;
    }

    long FailedRead(const char16_t*, size_t, LoadResult& result)
    {
        result.Status = LoadStatus::Unexpected;
        result.Category = PlacementFailureCategory::Read;
        result.Error = RO_E_CLOSED;
        return RO_E_CLOSED;
    }
}

class WindowPlacementDiagnosticsTests
{
public:
    TEST_CLASS(WindowPlacementDiagnosticsTests);

    TEST_METHOD(DetachedIdentityFailureReportsOriginalErrorOnce)
    {
        Reports reports;
        LoadResult result;
        VERIFY_ARE_EQUAL(S_OK, ReadPersistPlacement(u"private id", 10, result));
        VERIFY_IS_TRUE(result.Status == LoadStatus::Unavailable);
        VERIFY_IS_TRUE(result.Error == HRESULT_FROM_WIN32(APPMODEL_ERROR_NO_PACKAGE) ||
            result.Error == HRESULT_FROM_WIN32(APPMODEL_ERROR_NO_APPLICATION));
        Reports::Verify(PlacementOperation::Load, PlacementFailureCategory::Identity, result.Error);
    }

    TEST_METHOD(DetachedReadFailureReportsAndPropagatesOriginalError)
    {
        Reports reports;
        Backend backend;
        backend.Read = {StorageResult::Failure, RO_E_CLOSED};
        LoadResult result;
        VERIFY_ARE_EQUAL(RO_E_CLOSED, Load(backend, result));
        Reports::Verify(PlacementOperation::Load, PlacementFailureCategory::Read, RO_E_CLOSED);
    }

    TEST_METHOD(UnavailableStoreReportsWithoutPropagating)
    {
        Reports reports;
        Backend backend;
        backend.Read = {StorageResult::Unavailable, E_ACCESSDENIED};
        LoadResult result;
        VERIFY_ARE_EQUAL(S_OK, Load(backend, result));
        VERIFY_IS_TRUE(result.Status == LoadStatus::Unavailable);
        Reports::Verify(PlacementOperation::Load, PlacementFailureCategory::Read, E_ACCESSDENIED);
    }

    TEST_METHOD(UnexpectedBackendFailureWithoutErrorStillPropagates)
    {
        Reports reports;
        Backend backend;
        backend.Read = {StorageResult::Failure, S_OK};
        LoadResult result;
        VERIFY_ARE_EQUAL(E_FAIL, Load(backend, result));
        Reports::Verify(PlacementOperation::Load, PlacementFailureCategory::Read, E_FAIL);
    }

    TEST_METHOD(DecodeValidationReportsCategoryWithoutFabricatingError)
    {
        Reports reports;
        Backend backend;
        backend.Read = {StorageResult::Success, S_OK};
        backend.Value.Text = u"private invalid record content";
        LoadResult result;
        VERIFY_ARE_EQUAL(S_OK, Load(backend, result));
        VERIFY_IS_TRUE(result.Status == LoadStatus::Invalid);
        Reports::Verify(PlacementOperation::Load, PlacementFailureCategory::Decode, S_OK);
    }

    TEST_METHOD(WrongTypeRetainsBackendError)
    {
        Reports reports;
        Backend backend;
        backend.Read = {StorageResult::WrongType, DISP_E_TYPEMISMATCH};
        LoadResult result;
        VERIFY_ARE_EQUAL(S_OK, Load(backend, result));
        Reports::Verify(PlacementOperation::Load, PlacementFailureCategory::Decode, DISP_E_TYPEMISMATCH);
    }

    TEST_METHOD(MissingRecordReportsNothing)
    {
        Reports reports;
        Backend backend;
        LoadResult result;
        VERIFY_ARE_EQUAL(S_OK, Load(backend, result));
        VERIFY_IS_TRUE(result.Status == LoadStatus::Missing);
        VERIFY_ARE_EQUAL(0u, Reports::Count);
    }

    TEST_METHOD(SuccessfulReadAndWriteReportNothing)
    {
        Reports reports;
        Backend backend;
        const auto placement = Sample();
        VERIFY_IS_TRUE(Save(backend, &placement));
        backend.Read = {StorageResult::Success, S_OK};
        VERIFY_IS_TRUE(EncodeRecordText({placement, 1}, backend.Value.Text));
        LoadResult result;
        VERIFY_ARE_EQUAL(S_OK, Load(backend, result));
        VERIFY_IS_TRUE(result.Status == LoadStatus::Loaded);
        VERIFY_ARE_EQUAL(0u, Reports::Count);
    }

    TEST_METHOD(SaveWithoutCaptureReportsOnce)
    {
        Reports reports;
        Backend backend;
        VERIFY_IS_FALSE(Save(backend, nullptr));
        VERIFY_ARE_EQUAL(0u, backend.Writes);
        Reports::Verify(PlacementOperation::Save, PlacementFailureCategory::Capture, S_OK);
    }

    TEST_METHOD(SaveIdentityFailureReportsOriginalError)
    {
        Reports reports;
        Backend backend;
        const auto placement = Sample();
        VERIFY_IS_FALSE(SavePersistPlacement(backend, &placement, u"id", 2,
            [](std::u16string&) noexcept -> long { return E_UNEXPECTED; }));
        VERIFY_ARE_EQUAL(0u, backend.Writes);
        Reports::Verify(PlacementOperation::Save, PlacementFailureCategory::Identity, E_UNEXPECTED);
    }

    TEST_METHOD(WriteFailureReportsOriginalErrorOnce)
    {
        Reports reports;
        Backend backend;
        backend.Write = {StorageResult::Failure, HRESULT_FROM_WIN32(ERROR_DISK_FULL)};
        const auto placement = Sample();
        VERIFY_IS_FALSE(Save(backend, &placement));
        VERIFY_ARE_EQUAL(1u, backend.Writes);
        Reports::Verify(PlacementOperation::Save, PlacementFailureCategory::Write, backend.Write.Error);
    }

    TEST_METHOD(EnumerationFailureReportsOriginalErrorWithoutWriting)
    {
        Reports reports;
        Backend backend;
        backend.Enumerate = {StorageResult::Unavailable, E_ACCESSDENIED};
        const auto placement = Sample();
        VERIFY_IS_FALSE(Save(backend, &placement));
        VERIFY_ARE_EQUAL(0u, backend.Writes);
        Reports::Verify(PlacementOperation::Save, PlacementFailureCategory::Write, E_ACCESSDENIED);
    }

    TEST_METHOD(ApplyValidationReportsCategoryWithoutFabricatingError)
    {
        Reports reports;
        TestWindow window;
        PlacementPass pass{};
        pass.Request.Placement = Snapshot{};
        WindowPlacementCapture capture;
        VERIFY_IS_FALSE(TryApplyPlacement(window.Handle, pass, capture));
        Reports::Verify(PlacementOperation::Apply, PlacementFailureCategory::Apply, S_OK);
    }

    TEST_METHOD(AutomaticMissingRecordReportsNothingAfterFallback)
    {
        Reports reports;
        TestWindow window;
        PlacementPass pass{};
        pass.PlacementId = u"private id";
        pass.AutomaticPersistenceOptIn = true;
        WindowPlacementCapture capture;
        VERIFY_IS_FALSE(TryApplyPlacement(window.Handle, pass, capture, Missing));
        VERIFY_IS_NOT_NULL(capture.TryGetPlacement());
        VERIFY_ARE_EQUAL(0u, Reports::Count);
    }

    TEST_METHOD(AutomaticApplyFailureReportsOnceAtLoadBoundary)
    {
        Reports reports;
        TestWindow window;
        PlacementPass pass{};
        pass.PlacementId = u"private id";
        pass.AutomaticPersistenceOptIn = true;
        window.DestroyOnConstraints = true;
        WindowPlacementCapture capture;
        VERIFY_IS_FALSE(TryApplyPlacement(window.Handle, pass, capture,
            [](const char16_t*, size_t, LoadResult& result) -> long
            {
                result.Status = LoadStatus::Loaded;
                result.Error = S_OK;
                result.Placement.Placement = Sample();
                return S_OK;
            }));
        VERIFY_IS_FALSE(!!::IsWindow(window.Handle));
        Reports::Verify(PlacementOperation::Load, PlacementFailureCategory::Apply, S_OK);
    }

    TEST_METHOD(AutomaticReadFailureIsNonfatalAndReportsOnce)
    {
        Reports reports;
        TestWindow window;
        PlacementPass pass{};
        pass.PlacementId = u"private id";
        pass.AutomaticPersistenceOptIn = true;
        WindowPlacementCapture capture;
        VERIFY_IS_FALSE(TryApplyPlacement(window.Handle, pass, capture, FailedRead));
        VERIFY_IS_NOT_NULL(capture.TryGetPlacement());
        VERIFY_IS_TRUE(!!::IsWindowVisible(window.Handle));
        Reports::Verify(PlacementOperation::Load, PlacementFailureCategory::Read, RO_E_CLOSED);
    }

    TEST_METHOD(AutomaticReadAndFallbackFailuresProduceOnlyOriginalReport)
    {
        Reports reports;
        TestWindow window;
        PlacementPass pass{};
        pass.PlacementId = u"private id";
        pass.AutomaticPersistenceOptIn = true;
        WindowPlacementCapture capture;
        VERIFY_IS_TRUE(capture.TryCapture(window.Handle, PresenterKind::Overlapped));
        capture.Detach(); // Makes fallback capture fail without an unrelated HWND error.
        VERIFY_IS_FALSE(TryApplyPlacement(window.Handle, pass, capture, FailedRead));
        Reports::Verify(PlacementOperation::Load, PlacementFailureCategory::Read, RO_E_CLOSED);
    }

    TEST_METHOD(AutomaticIdentityFailureDoesNotDoubleReport)
    {
        Reports reports;
        TestWindow window;
        PlacementPass pass{};
        pass.PlacementId = u"private id";
        pass.AutomaticPersistenceOptIn = true;
        WindowPlacementCapture capture;
        VERIFY_IS_FALSE(TryApplyPlacement(window.Handle, pass, capture));
        VERIFY_ARE_EQUAL(1u, Reports::Count);
        VERIFY_IS_TRUE(Reports::Operation == PlacementOperation::Load);
        VERIFY_IS_TRUE(Reports::Failure.Category == PlacementFailureCategory::Identity);
    }
};
