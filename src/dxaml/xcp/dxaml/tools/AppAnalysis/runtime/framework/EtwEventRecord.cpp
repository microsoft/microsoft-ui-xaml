// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include "EtwEventRecord.h"
#include <memory>
#include "wil\resource.h"
#include "EtwEventInfo.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {
    ////////////////////////////////////////////////////////////////////////////////
    //
    template <typename T>
    HRESULT
    GetFixedProperty(
        _In_ EVENT_RECORD* pEventRecord,
        _In_ HSTRING propertyName,
        _In_ UINT pointerSize,
        _Out_ T* pValue
        )
    {
        PROPERTY_DATA_DESCRIPTOR prop = {
            reinterpret_cast<ULONGLONG>(WindowsGetStringRawBuffer(propertyName, nullptr)),
            ULONG_MAX,
            0
        };
        IFCPTR_RETURN(pEventRecord);

        // If the pointer size is greater than 0, that means that this is an event from a classic 
        // provider and we need to provide a TDHContext object;
        if (pointerSize > 0)
        {
            TDH_CONTEXT context = { 0 };
            context.ParameterValue = pointerSize;
            context.ParameterType = TDH_CONTEXT_POINTERSIZE;
            context.ParameterSize = 0;
            IFCSTATUS_RETURN(TdhGetProperty(pEventRecord, 1, &context, 1, &prop, sizeof(T), (BYTE*)pValue));
        }
        else
        {
            IFCSTATUS_RETURN(TdhGetProperty(pEventRecord, 0, nullptr, 1, &prop, sizeof(T), (BYTE*)pValue));
        }

        return S_OK;
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    EtwEventRecord::EtwEventRecord()
        : m_pEventRecord(nullptr)
        , m_pointerSize(0)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    EtwEventRecord::~EtwEventRecord()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    HRESULT EtwEventRecord::RuntimeClassInitialize()
    {
        return S_OK;
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    void
    EtwEventRecord::SetEventRecord(
        _In_opt_ PEVENT_RECORD eventRecord
        )
    {
        m_pEventRecord = eventRecord;
        // if this is a classic event, we store the pointer size for the call into TdhGetProperty inside of GetFixedProperty
        // and GetUnicodeStringProperty
        if (m_pEventRecord && (m_pEventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_CLASSIC_HEADER) == EVENT_HEADER_FLAG_CLASSIC_HEADER)
        {
            if ((m_pEventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_32_BIT_HEADER) == EVENT_HEADER_FLAG_32_BIT_HEADER)
            {
                m_pointerSize = 4;
            }
            else if ((m_pEventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_64_BIT_HEADER) == EVENT_HEADER_FLAG_64_BIT_HEADER)
            {
                m_pointerSize = 8;
            }
        }
        else
        {
            // reset pointer size to 0 for next event
            m_pointerSize = 0;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    IFACEMETHODIMP
    EtwEventRecord::get_Timestamp(
        _Out_ INT64* pValue
        )
    {
        HRESULT hr = S_OK;

        ARG_VALIDRETURNPOINTER(pValue);
        IFCPTR(m_pEventRecord);

        *pValue = m_pEventRecord->EventHeader.TimeStamp.QuadPart;

    Cleanup:
        return hr;
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    IFACEMETHODIMP
    EtwEventRecord::get_ThreadId(
        _Out_ UINT32* pValue
        )
    {
        ARG_VALIDRETURNPOINTER(pValue);
        IFCPTR_RETURN(m_pEventRecord);

        *pValue = m_pEventRecord->EventHeader.ThreadId;

        return S_OK;
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    IFACEMETHODIMP
    EtwEventRecord::get_EventId(
        _Out_ UINT16* pValue
        )
    {
        ARG_VALIDRETURNPOINTER(pValue);
        IFCPTR_RETURN(m_pEventRecord);

        // Classic (MOF) providers store the Event ID in the Opcode.
        if ((m_pEventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_CLASSIC_HEADER) == EVENT_HEADER_FLAG_CLASSIC_HEADER)
        {
            *pValue = m_pEventRecord->EventHeader.EventDescriptor.Opcode;
        }
        else
        {
            *pValue = m_pEventRecord->EventHeader.EventDescriptor.Id;
        }

        return S_OK;
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    IFACEMETHODIMP
    EtwEventRecord::get_EventVersion(
        _Out_ BYTE* pValue
        )
    {
        ARG_VALIDRETURNPOINTER(pValue);
        IFCPTR_RETURN(m_pEventRecord);

        *pValue = m_pEventRecord->EventHeader.EventDescriptor.Version;

        return S_OK;
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    IFACEMETHODIMP
    EtwEventRecord::get_ProviderId(
        _Out_ GUID* pValue
        )
    {
        ARG_VALIDRETURNPOINTER(pValue);
        IFCPTR_RETURN(m_pEventRecord);

        *pValue = m_pEventRecord->EventHeader.ProviderId;

        return S_OK;
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    IFACEMETHODIMP
    EtwEventRecord::GetUnicodeStringProperty(
        _In_ HSTRING propertyName,
        _Out_ HSTRING* pValue
        )
    {
        ULONG uPropertySize = 0;

        PROPERTY_DATA_DESCRIPTOR prop = { 
            // This API requires this cast.
            reinterpret_cast<ULONGLONG>(WindowsGetStringRawBuffer(propertyName, nullptr)),
            ULONG_MAX, 
            0 
        };    

        ZeroMemory(m_pLargeBuffer, sizeof(m_pLargeBuffer));

        ARG_VALIDRETURNPOINTER(pValue);
        IFCPTR_RETURN(m_pEventRecord);

        // If the pointer size is greater than 0, that means that this is an event from a classic 
        // provider and we need to provide a TDHContext object for Tdh
        if (m_pointerSize > 0)
        {
            TDH_CONTEXT context = { 0 };
            context.ParameterValue = m_pointerSize;
            context.ParameterType = TDH_CONTEXT_POINTERSIZE;
            context.ParameterSize = 0;
            IFCSTATUS_RETURN(TdhGetPropertySize(m_pEventRecord, 1, &context, 1, &prop, &uPropertySize));
            IFCSTATUS_RETURN(TdhGetProperty(m_pEventRecord, 1, &context, 1, &prop, sizeof(m_pLargeBuffer), m_pLargeBuffer));
        }
        else
        {
            IFCSTATUS_RETURN(TdhGetPropertySize(m_pEventRecord, 0, nullptr, 1, &prop, &uPropertySize));
            IFCSTATUS_RETURN(TdhGetProperty(m_pEventRecord, 0, nullptr, 1, &prop, sizeof(m_pLargeBuffer), m_pLargeBuffer));
        }

        // The UnicodeString property in the ETW payload is guaranteed to be null terminated, so we can divide by two to get the the string length and 
        // subtract one for the null terminator.
        UINT32 stringBufferSize = (uPropertySize / 2) - 1;
        // This API requires this cast.
        //
        IFC_RETURN(WindowsCreateString(reinterpret_cast<LPWSTR>(m_pLargeBuffer), stringBufferSize, pValue));
        return S_OK;
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    IFACEMETHODIMP 
    EtwEventRecord::GetBooleanProperty(
        _In_ HSTRING propertyName,
        _Out_ boolean* pValue
        )
    {
        // the win:Boolean type used in event manifests is 32 bit. the boolean type is only 8 bit, so we need
        // to use the old-school BOOL type
        BOOL propertyValue = FALSE;
        IFC_RETURN(GetFixedProperty<BOOL>(m_pEventRecord, propertyName, m_pointerSize, &propertyValue));
        *pValue = static_cast<boolean>(propertyValue);
        return S_OK;
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    IFACEMETHODIMP 
    EtwEventRecord::GetInt32Property(
        _In_ HSTRING propertyName,
        _Out_ INT32* pValue
        )
    {
        return GetFixedProperty<INT32>(m_pEventRecord, propertyName, m_pointerSize, pValue);
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    IFACEMETHODIMP
    EtwEventRecord::GetUInt32Property(
        _In_ HSTRING propertyName,
        _Out_ UINT32* pValue
        )
    {
        return GetFixedProperty<UINT32>(m_pEventRecord, propertyName, m_pointerSize, pValue);
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    IFACEMETHODIMP
    EtwEventRecord::GetInt64Property(
        _In_ HSTRING propertyName,
        _Out_ INT64* pValue
        )
    {
        return GetFixedProperty<INT64>(m_pEventRecord, propertyName, m_pointerSize, pValue);
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    IFACEMETHODIMP
    EtwEventRecord::GetUInt64Property(
        _In_ HSTRING propertyName,
        _Out_ UINT64* pValue
        )
    {
        return GetFixedProperty<UINT64>(m_pEventRecord, propertyName, m_pointerSize, pValue);
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    IFACEMETHODIMP
    EtwEventRecord::GetFloatProperty(
        _In_ HSTRING propertyName,
        _Out_ FLOAT* pValue
        )
    {
        return GetFixedProperty<FLOAT>(m_pEventRecord, propertyName, m_pointerSize, pValue);
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    IFACEMETHODIMP
        EtwEventRecord::GetDoubleProperty(
            _In_ HSTRING propertyName,
            _Out_ double* pValue
            )
    {
        return GetFixedProperty<double>(m_pEventRecord, propertyName, m_pointerSize, pValue);
    }

} } }