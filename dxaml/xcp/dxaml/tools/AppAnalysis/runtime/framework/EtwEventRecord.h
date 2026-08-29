// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {

// Arbitrarily large buffer, equivalent to a 2000 character length string.
// We shouldn't need anything larger than this.
const size_t LARGE_BUFFER = 4096;

////////////////////////////////////////////////////////////////////////////////
//
// The EtwEventRecord class is the implementation of appanalysis::IEtwEventRecord. It wraps an EVENT_RECORD,
// and provides COM compatible access to event metadata (through TDH). The reason
// PEVENT_RECORD is not passed at construction time is because we can't afford to 
// create a new object every time there's a new EVENT_RECORD. The object instance 
// is used internally by the EventProcessor and is reused for the lifetime of the
// EventProcessor
//

    class EtwEventRecord 
        : public wrl::RuntimeClass<appanalysis::IEtwEventRecord, wrl::FtmBase>
    {

        InspectableClass(
            RuntimeClass_Microsoft_Diagnostics_AppAnalysis_EtwEventRecord, TrustLevel::FullTrust
            );

    public:
        EtwEventRecord();
        virtual ~EtwEventRecord();

        HRESULT RuntimeClassInitialize();

        void SetEventRecord(
            _In_opt_ PEVENT_RECORD pValue
            );
        //
        // IEtwEventRecordRecord
        //

        IFACEMETHOD(get_Timestamp)(
            _Out_ INT64* pValue
            ) override;

        IFACEMETHOD(get_ThreadId)(
            _Out_ UINT32* pValue
            ) override;

        IFACEMETHOD(get_EventId)(
            _Out_ UINT16* pValue
            ) override;

        IFACEMETHOD(get_EventVersion)(
            _Out_ BYTE* pValue
            ) override;

        IFACEMETHOD(get_ProviderId)(
            _Out_ GUID* pValue
            ) override;

        IFACEMETHOD(GetUnicodeStringProperty)(
            _In_ HSTRING propertyName,
            _Out_ HSTRING* pValue
            ) override;

        IFACEMETHOD(GetBooleanProperty)(
            _In_ HSTRING propertyName,
            _Out_ boolean* pValue
            ) override;

        IFACEMETHOD(GetInt32Property)(
            _In_ HSTRING propertyName,
            _Out_ INT32* pValue
            ) override;

        IFACEMETHOD(GetUInt32Property)(
            _In_ HSTRING propertyName,
            _Out_ UINT32* pValue
            ) override;

        IFACEMETHOD(GetInt64Property)(
            _In_ HSTRING propertyName,
            _Out_ INT64* pValue
            ) override;

        IFACEMETHOD(GetUInt64Property)(
            _In_ HSTRING propertyName,
            _Out_ UINT64* pValue
            ) override;

        IFACEMETHOD(GetFloatProperty)(
            _In_ HSTRING propertyName,
            _Out_ FLOAT* pValue
            ) override;

        IFACEMETHOD(GetDoubleProperty)(
            _In_ HSTRING propertyName,
            _Out_ double* pValue
            ) override;

    private:

        PEVENT_RECORD m_pEventRecord;
        BYTE m_pLargeBuffer[LARGE_BUFFER]{};
        unsigned short m_pointerSize;
    };
} } }
