// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RuleTriggeredEventArgs.h"
#include "RuleServiceProvider.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {

    IFACEMETHODIMP 
    RuleTriggeredEventArgsFactory::CreateInstance(
        _In_ UINT64 elementId,
        _In_ appanalysis::TimelineInfo timelineInfo,
        _In_ appanalysis::Measurement measurement,
        _In_ appanalysis::IResourceString* description,
        _In_ appanalysis::IResourceString* solution,
        _COM_Outptr_ appanalysis::IRuleTriggeredEventArgs** instance
        )
    {
        wrl::ComPtr<RuleTriggeredEventArgs> args = wrl::Make<RuleTriggeredEventArgs>();

        IFC_RETURN(args->Initialize(
            elementId,
            timelineInfo,
            measurement,
            description,
            solution
            ));

        *instance = args.Detach();
        return S_OK;
    }

RuleTriggeredEventArgs::RuleTriggeredEventArgs()
: m_measurement({0})
, m_timelineInfo({0})
{
}

RuleTriggeredEventArgs::~RuleTriggeredEventArgs()
{
}

HRESULT
RuleTriggeredEventArgs::Initialize(
    _In_ UINT64 elementId,
    _In_ appanalysis::TimelineInfo timelineInfo,
    _In_ appanalysis::Measurement measurement,
    _In_ appanalysis::IResourceString* description,
    _In_ appanalysis::IResourceString* solution
    )
{
    IFC_RETURN(GetSourceInfo(elementId, &m_sourceInfo, &m_elementId));
    m_timelineInfo = timelineInfo;
    m_measurement = measurement;

    IFC_RETURN(wrl::MakeAndInitialize<ResourceStringView>(&m_solution, solution));
    IFC_RETURN(wrl::MakeAndInitialize<ResourceStringView>(&m_description, description));

    return S_OK;
}

HRESULT
RuleTriggeredEventArgs::CreateInstance(
    _In_ const RuleTriggeredEventArgs::CreateParams& params,
    _COM_Outptr_ appanalysis::IRuleTriggeredEventArgs** instance
    )
{
    wrl::ComPtr<RuleTriggeredEventArgs> args = wrl::Make<RuleTriggeredEventArgs>();

    args->m_sourceInfo = params.sourceInfo;
    IFC_RETURN(wrl::MakeAndInitialize<ResourceStringView>(&args->m_solution, params.solution.Get()));
    IFC_RETURN(wrl::MakeAndInitialize<ResourceStringView>(&args->m_description, params.description.Get()));

    args->m_timelineInfo = params.timeline;
    args->m_measurement = params.measurement;
    args->m_elementId = params.elementId;

    *instance = args.Detach();
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// IRuleTriggeredEventArgs::get_TimelineStart
//
IFACEMETHODIMP
RuleTriggeredEventArgs::get_TimelineStart(
    _Out_ INT64* timelineStart
    )
{
    ARG_VALIDRETURNPOINTER(timelineStart);
    *timelineStart = m_timelineInfo.Start;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// IRuleTriggeredEventArgs::get_TimelineStop
//
IFACEMETHODIMP
RuleTriggeredEventArgs::get_TimelineStop(
    _Out_ INT64* timelineStop
    )
{
    ARG_VALIDRETURNPOINTER(timelineStop);
    *timelineStop = m_timelineInfo.Stop;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// IRuleTriggeredEventArgs::get_FileName
//
IFACEMETHODIMP
RuleTriggeredEventArgs::get_FileName(
    _Out_ HSTRING* fileName
    )
{
    ARG_VALIDRETURNPOINTER(fileName);
    IFC_RETURN(WindowsDuplicateString(m_sourceInfo.FileName, fileName));

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// IRuleTriggeredEventArgs::get_LineNumber
//
IFACEMETHODIMP
RuleTriggeredEventArgs::get_LineNumber(
    _Out_ UINT32* lineNumber
    )
{
    ARG_VALIDRETURNPOINTER(lineNumber);

    *lineNumber = m_sourceInfo.LineNumber;
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// IRuleTriggeredEventArgs::get_ColumnNumber
//
IFACEMETHODIMP 
RuleTriggeredEventArgs::get_ColumnNumber(
    _Out_ UINT32* columnNumber
    )
{
    ARG_VALIDRETURNPOINTER(columnNumber);

    *columnNumber = m_sourceInfo.ColumnNumber;
    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
// IRuleTriggeredEventArgs::get_FileHash
//
IFACEMETHODIMP
RuleTriggeredEventArgs::get_FileHash(
    _Out_ HSTRING* hash
)
{
    ARG_VALIDRETURNPOINTER(hash);
    IFC_RETURN(WindowsDuplicateString(m_sourceInfo.FileHash, hash));

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// IRuleTriggeredEventArgs::get_ElementId
//
IFACEMETHODIMP
RuleTriggeredEventArgs::get_ElementId(
    _Out_ UINT64* elementId
    )
{
    ARG_VALIDRETURNPOINTER(elementId);

    *elementId = m_elementId;
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// IRuleTriggeredEventArgs::get_MeasurementUnit
//
IFACEMETHODIMP
RuleTriggeredEventArgs::get_MeasurementUnit(
    _Out_ appanalysis::MeasurementUnit* measurementUnit
    )
{
    ARG_VALIDRETURNPOINTER(measurementUnit);

    *measurementUnit = m_measurement.Unit;
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// IRuleTriggeredEventArgs::get_MeasurementValue
//
IFACEMETHODIMP
RuleTriggeredEventArgs::get_MeasurementValue(
    _Out_ DOUBLE* measurementValue
    )
{
    ARG_VALIDRETURNPOINTER(measurementValue);

    *measurementValue = m_measurement.Value;
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// IRuleTriggeredEventArgs::get_Description
//
IFACEMETHODIMP
RuleTriggeredEventArgs::get_Description(
    _COM_Outptr_ appanalysis::IResourceStringView** description
    )
{
    ARG_VALIDRETURNPOINTER(description);
    *description = nullptr;

    IFC_RETURN(m_description.CopyTo(description));

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// IRuleTriggeredEventArgs::get_Solution
//
IFACEMETHODIMP
RuleTriggeredEventArgs::get_Solution(
    _COM_Outptr_ appanalysis::IResourceStringView** solution
    )
{
    ARG_VALIDRETURNPOINTER(solution);
    *solution = nullptr;

    IFC_RETURN(m_solution.CopyTo(solution));

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// Private Methods
//
HRESULT 
RuleTriggeredEventArgs::GetSourceInfo(_In_ UINT64 elementId, _Out_ appanalysis::SourceInfo* sourceInfo, _Out_ UINT64* visualTreeId)
{
    wrl::ComPtr<appanalysis::IRuleServiceProviderStatics> serviceProvider;
    IFC_RETURN(RuleServiceProvider::GetProvider(&serviceProvider));
    wrl::ComPtr<appanalysis::IRuleService> ruleService;
    IFC_RETURN(serviceProvider->GetService(__uuidof(appanalysis::ISourceInfoRuleService), &ruleService));
    
    wrl::ComPtr<appanalysis::ISourceInfoRuleService> sourceInfoService;
    IFC_RETURN(ruleService.As(&sourceInfoService));

    IFC_RETURN(sourceInfoService->GetSourceInfo(elementId, sourceInfo));
    IFC_RETURN(sourceInfoService->GetVisualTreeId(elementId, visualTreeId));
    
    return S_OK;
}

} } }
