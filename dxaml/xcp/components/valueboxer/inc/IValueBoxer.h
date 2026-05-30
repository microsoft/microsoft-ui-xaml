// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Value.h> // uses PropertyValue

namespace DirectUI
{
    class BoxerBuffer;

    class IValueBoxer
    {
    public:
        IValueBoxer() = delete;

        // Boxing

        // Box value to IReference<T>.
        template<class T>
        static _Check_return_ HRESULT BoxValue(
            _In_ IInspectable** box,
            _In_ T value)
        {
            return PropertyValue::CreateReference<T>(value, box);
        }

        // Box IReference<T>.
        template<class T>
        static _Check_return_ HRESULT BoxValue(
            _In_ IInspectable** box,
            _In_ wf::IReference<T>* value)
        {
            IFCPTR_RETURN(box);

            *box = value;
            AddRefInterface(value);

            return S_OK;
        }

        // Box intrinsic types.

        static _Check_return_ HRESULT BoxValue(
            _In_ IInspectable** box,
            _In_ BOOLEAN value);

        static _Check_return_ HRESULT BoxValue(
            _In_ IInspectable** box,
            _In_ INT value);

        static _Check_return_ HRESULT BoxValue(
            _In_ IInspectable** box,
            _In_ UINT value);

        static _Check_return_ HRESULT BoxValue(
            _In_ IInspectable** box,
            _In_ INT64 value);

        static _Check_return_ HRESULT BoxValue(
            _In_ IInspectable** box,
            _In_ UINT64 value);

        static _Check_return_ HRESULT BoxValue(
            _In_ IInspectable** box,
            _In_ DOUBLE value);

        static _Check_return_ HRESULT BoxValue(
            _In_ IInspectable** box,
            _In_ FLOAT value);

        static _Check_return_ HRESULT BoxValue(
            _In_ IInspectable** box,
            _In_opt_ HSTRING value);

        // Box ::Windows::Foundation types.

        static _Check_return_ HRESULT BoxValue(
            _In_ IInspectable** box,
            _In_ wf::DateTime value);

        static _Check_return_ HRESULT BoxValue(
            _In_ IInspectable** box,
            _In_ wf::TimeSpan value);

        static _Check_return_ HRESULT BoxValue(
            _In_ IInspectable** box,
            _In_ wf::Point value);

        static _Check_return_ HRESULT BoxValue(
            _In_ IInspectable** box,
            _In_ wf::Size value);

        static _Check_return_ HRESULT BoxValue(
            _In_ IInspectable** box,
            _In_ wf::Rect value);

        static _Check_return_ HRESULT BoxValue(
            _In_ IInspectable** box,
            _In_ wf::IUriRuntimeClass* value);

        // Box IInspectable*

        static _Check_return_ HRESULT BoxObjectValue(
            _In_ IInspectable** box,
            _In_ IInspectable* value);

        // Unboxing

        // Unbox IReference<T> value from IInspectable.
        template<class T>
        static _Check_return_ HRESULT UnboxValue(
            _In_ IInspectable* box,
            _Out_ T* value)
        {
            IFCPTR_RETURN(box);
            IFCPTR_RETURN(value);

            ctl::ComPtr<wf::IReference<T>> spObjAsRef;

            IFC_RETURN(ctl::do_query_interface(spObjAsRef, box));
            IFC_RETURN(spObjAsRef->get_Value(value));

            return S_OK;
        }

        // Unbox IReference<T> from IInspectable.
        template<class T>
        static _Check_return_ HRESULT UnboxValue(
            _In_ IInspectable* box,
            _Out_ wf::IReference<T>** value)
        {
            IFCPTR_RETURN(value);

            if (box)
            {
                IFC_RETURN(ctl::do_query_interface(*value, box));
            }
            else
            {
                *value = nullptr;
            }

            return S_OK;
        }

        // Unbox intrinsic types.

        static _Check_return_ HRESULT UnboxValue(
            _In_ IInspectable* box,
            _Out_ BOOLEAN* value);

        static _Check_return_ HRESULT UnboxValue(
            _In_ IInspectable* box,
            _Out_ INT* value);

        static _Check_return_ HRESULT UnboxValue(
            _In_ IInspectable* box,
            _Out_ UINT* value);

        static _Check_return_ HRESULT UnboxValue(
            _In_ IInspectable* box,
            _Out_ INT64* value);

        static _Check_return_ HRESULT UnboxValue(
            _In_ IInspectable* box,
            _Out_ UINT64* value);

        static _Check_return_ HRESULT UnboxValue(
            _In_ IInspectable* box,
            _Out_ DOUBLE* value);

        static _Check_return_ HRESULT UnboxValue(
            _In_ IInspectable* box,
            _Out_ FLOAT* value);

        static _Check_return_ HRESULT UnboxValue(
            _In_opt_ IInspectable* box,
            _Outptr_ HSTRING* value);

        // Unbox ::Windows::Foundation types.

        static _Check_return_ HRESULT UnboxValue(
            _In_ IInspectable* box,
            _Out_ wf::DateTime* value);

        static _Check_return_ HRESULT UnboxValue(
            _In_ IInspectable* box,
            _Out_ wf::TimeSpan* value);

        static _Check_return_ HRESULT UnboxValue(
            _In_ IInspectable* box,
            _Out_ wf::IUriRuntimeClass** value);

        // Unbox xaml::Interop types.

        static _Check_return_ HRESULT UnboxValue(
            _In_opt_ IInspectable* box,
            _Out_ wxaml_interop::TypeName* value);

        static _Check_return_ HRESULT UnboxObjectValue(
            _In_opt_ IInspectable* box,
            _In_ REFIID riid,
            _Out_ void** value);
    };
}
