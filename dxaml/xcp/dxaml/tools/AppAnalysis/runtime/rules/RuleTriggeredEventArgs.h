// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "helpers.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis { 

class RuleTriggeredEventArgsFactory
    : public wrl::AgileActivationFactory<appanalysis::IRuleTriggeredEventArgsFactory>
{
public:
    // IRuleTriggeredEventArgsFactory
    IFACEMETHOD(CreateInstance)(
        _In_ UINT64 elementId,
        _In_ appanalysis::TimelineInfo rangeInfo,
        _In_ appanalysis::Measurement measurement,
        _In_ appanalysis::IResourceString* description,
        _In_ appanalysis::IResourceString* solution,
        _COM_Outptr_ appanalysis::IRuleTriggeredEventArgs** ppInstance
        ) override;
};
////////////////////////////////////////////////////////////////////////////////
//
// The RuleTriggeredEventArgs class defines objects that a rule will use to report issues
// back to the host app.
//
class RuleTriggeredEventArgs
    : public wrl::RuntimeClass<appanalysis::IRuleTriggeredEventArgs, wrl::FtmBase>
{
    InspectableClass(
        RuntimeClass_Microsoft_Diagnostics_AppAnalysis_RuleTriggeredEventArgs,
        BaseTrust
        );

public:
    RuleTriggeredEventArgs();
    virtual ~RuleTriggeredEventArgs();

    struct CreateParams {
        CreateParams()
            : timeline({ 0 })
            , measurement({ 0 })
            , elementId(0)
        {
        }

        ~CreateParams()
        {
        }

        wil::shared_sourceinfo sourceInfo;
        UINT64 elementId;
        wrl::ComPtr<ResourceString> description;
        wrl::ComPtr<ResourceString> solution;
        appanalysis::Measurement measurement;
        appanalysis::TimelineInfo timeline;

    };

    // CreateInstance is used internally because we don't have to reallocate the strings
    static HRESULT CreateInstance(
        _In_ const CreateParams& params,
        _COM_Outptr_ appanalysis::IRuleTriggeredEventArgs** instance
        );

    HRESULT Initialize(
        _In_ UINT64 elementId,
        _In_ appanalysis::TimelineInfo timelineInfo,
        _In_ appanalysis::Measurement measurement,
        _In_ appanalysis::IResourceString* description,
        _In_ appanalysis::IResourceString* solution
        );

    //
    // IRuleTriggeredEventArgs
    //
    IFACEMETHOD(get_TimelineStart)(
        _Out_ INT64* timelineStart
        ) override;

    IFACEMETHOD(get_TimelineStop)(
        _Out_ INT64* timelineStop
        ) override;

    IFACEMETHOD(get_FileName)(
        _Out_ HSTRING* sourceInfo
        ) override;

    IFACEMETHOD(get_LineNumber)(
        _Out_ UINT32* lineNumber
        ) override;

    IFACEMETHOD(get_ColumnNumber)(
        _Out_ UINT32* columnNumber
        ) override;

    IFACEMETHOD(get_FileHash)(
        _Out_ HSTRING* hash
        ) override;

    IFACEMETHOD(get_ElementId)(
        _Out_ UINT64* elementId
        ) override;

    IFACEMETHOD(get_MeasurementUnit)(
        _Out_ appanalysis::MeasurementUnit* measurementUnit
        ) override;

    IFACEMETHOD(get_MeasurementValue)(
        _Out_ DOUBLE* measurementValue
        ) override;

    IFACEMETHOD(get_Description)(
        _COM_Outptr_ appanalysis::IResourceStringView** description
        ) override;

    IFACEMETHOD(get_Solution)(
        _COM_Outptr_ appanalysis::IResourceStringView** description
        ) override;
private:

    HRESULT GetSourceInfo(_In_ UINT64 elementId, _Out_ appanalysis::SourceInfo* sourceInfo, _Out_ UINT64* visualTreeId);

private:

    appanalysis::Measurement m_measurement;
    appanalysis::TimelineInfo m_timelineInfo;
    UINT64 m_elementId{};

    wrl::ComPtr<appanalysis::IResourceStringView> m_description;
    wrl::ComPtr<appanalysis::IResourceStringView> m_solution;

    wil::shared_sourceinfo m_sourceInfo;

};
} } }
