// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewSource.h"

#include <exception>
#include <utility>

TableView::ActionTelemetry::ActionTelemetry(
    TableView& owner, TableViewTelemetry::Operation operation, TableViewTelemetry::Stage stage, bool admitted) noexcept :
    m_owner(owner), m_state(owner.m_telemetry), m_stage(stage), m_lifetimeGeneration(owner.m_telemetryLifetimeGeneration),
    m_exceptions(std::uncaught_exceptions())
{
    if (admitted && owner.m_telemetry.session.active && !owner.m_rowsSourceDrained)
    {
        m_state.origin = owner.m_telemetry.origin;
        m_controlGenerationAtAdmission = owner.m_telemetry.operationGeneration;
        m_generation = TableViewTelemetry::BeginOperation(m_state, operation);
        if (m_generation) { m_previousAction = std::exchange(owner.m_activeActionTelemetry, this); }
    }
}

TableView::ActionTelemetry::~ActionTelemetry()
{
    if (!m_generation) { return; }
    m_owner.m_activeActionTelemetry = m_previousAction;
    if (m_lifetimeGeneration != m_owner.m_telemetryLifetimeGeneration)
    {
        TableViewTelemetry::IgnoreOperation(m_state, m_generation, m_owner.m_telemetryCancellationReason);
        return;
    }
    if (m_appFaultKind != TableViewTelemetry::AppFaultKind::None)
    {
        TableViewTelemetry::IgnoreOperation(m_state, m_generation, TableViewTelemetry::IgnoreReason::ApplicationFault, m_error, m_appFaultKind);
        TableViewTelemetry::ReportError(m_state, m_state.operation, m_stage, true,
            TableViewTelemetry::Attribution::Application, m_error, m_appFaultKind);
    }
    else if (m_failed)
    {
        TableViewTelemetry::FailOperation(m_state, m_state.operation, m_generation, m_stage, m_error);
    }
    else if (std::uncaught_exceptions() > m_exceptions)
    {
        TableViewTelemetry::IgnoreOperation(m_state, m_generation, TableViewTelemetry::IgnoreReason::UnattributedFault);
        TableViewTelemetry::ReportError(m_state, m_state.operation, m_stage, false);
    }
    else if (m_completed)
    {
        TableViewTelemetry::CompleteOperation(m_state, m_generation, TableViewTelemetry::Result::Success);
    }
    else
    {
        TableViewTelemetry::IgnoreOperation(m_state, m_generation, m_ignoreReason);
    }
}

void TableView::ActionTelemetry::Fail(std::optional<HRESULT> error) noexcept
{
    if (!m_failed) { m_error = error; }
    m_failed = true;
}

void TableView::ActionTelemetry::ApplicationFault(TableViewTelemetry::AppFaultKind kind, std::optional<HRESULT> error) noexcept
{
    if (m_appFaultKind == TableViewTelemetry::AppFaultKind::None)
    {
        m_appFaultKind = kind;
        m_error = error;
    }
}

TableView::ApplicationCalloutTelemetry::ApplicationCalloutTelemetry(
    TableView& owner, TableViewTelemetry::Stage stage, TableViewTelemetry::AppFaultKind kind) noexcept :
    m_owner(owner), m_action(owner.m_activeActionTelemetry &&
        (!TableViewTelemetry::IsOperationStarted(owner.m_telemetry) ||
            owner.m_activeActionTelemetry->ControlGenerationAtAdmission() == owner.m_telemetry.operationGeneration) ?
        owner.m_activeActionTelemetry : nullptr),
    m_operation(TableViewTelemetry::IsOperationStarted(owner.m_telemetry) ?
        owner.m_telemetry.operation : TableViewTelemetry::Operation::InitialLayout),
    m_stage(stage), m_kind(kind), m_generation(owner.m_telemetry.operationGeneration),
    m_lifetime(owner.m_telemetryLifetimeGeneration)
{
}

void TableView::ApplicationCalloutTelemetry::Fault(std::optional<HRESULT> error) noexcept
{
    if (m_lifetime != m_owner.m_telemetryLifetimeGeneration) { return; }
    if (m_action)
    {
        m_action->ApplicationFault(m_kind, error);
    }
    else if (m_generation == m_owner.m_telemetry.operationGeneration)
    {
        TableViewTelemetry::IgnoreOperation(m_owner.m_telemetry, m_generation,
            TableViewTelemetry::IgnoreReason::ApplicationFault, error, m_kind);
        TableViewTelemetry::IgnoreInitial(m_owner.m_telemetry, TableViewTelemetry::IgnoreReason::ApplicationFault, m_stage, error, m_kind);
        TableViewTelemetry::ReportError(m_owner.m_telemetry, m_operation, m_stage, true,
            TableViewTelemetry::Attribution::Application, error, m_kind);
    }
}

void TableView::IgnoreTelemetry(TableViewTelemetry::IgnoreReason reason, TableViewTelemetry::Stage stage) noexcept
{
    ++m_telemetryLifetimeGeneration;
    m_telemetryCancellationReason = reason;
    TableViewTelemetry::IgnoreInitial(m_telemetry, reason, stage);
    TableViewTelemetry::IgnoreOperation(m_telemetry, m_telemetry.operationGeneration, reason);
}

uint64_t TableView::BeginOperationTelemetry(TableViewTelemetry::Operation operation, bool admitted) noexcept
{
    ++m_telemetryMutationDepth;
    if (!admitted) { return 0; }
    if (m_telemetry.session.active && !m_rowsSourceDrained)
    {
        return TableViewTelemetry::BeginOperation(m_telemetry, operation);
    }
    return 0;
}

void TableView::EndOperationTelemetry() noexcept
{
    --m_telemetryMutationDepth;
}

void TableView::InvalidateOperationTelemetry() noexcept
{
    if (!TableViewTelemetry::IsOperationStarted(m_telemetry)) { return; }
    // Control-owned sort writes notify inside their own scope; a Sorted handler can reshape
    // after that scope ends but before the enclosing telemetry operation unwinds.
    if (m_telemetryMutationDepth &&
        (m_telemetry.operation != TableViewTelemetry::Operation::Sort ||
            m_isApplyingControlInitiatedSort || m_telemetrySourceMutationDepth))
    {
        return;
    }
    TableViewTelemetry::IgnoreOperation(m_telemetry, m_telemetry.operationGeneration, TableViewTelemetry::IgnoreReason::Superseded);
}

void TableView::IgnoreUnattributedTelemetry(
    TableViewTelemetry::Operation operation, uint64_t generation, TableViewTelemetry::Stage stage, std::optional<HRESULT> error) noexcept
{
    if (generation != m_telemetry.operationGeneration &&
        !(operation == TableViewTelemetry::Operation::InitialLayout && generation == 0)) { return; }
    if (TableViewTelemetry::IsOperationHandled(m_telemetry, operation, generation)) { return; }
    TableViewTelemetry::IgnoreOperation(m_telemetry, generation, TableViewTelemetry::IgnoreReason::UnattributedFault, error);
    TableViewTelemetry::IgnoreInitial(m_telemetry, TableViewTelemetry::IgnoreReason::UnattributedFault, stage, error);
    TableViewTelemetry::ReportError(m_telemetry, operation, stage, false, TableViewTelemetry::Attribution::Unknown, error);
}

void TableView::CompleteOperationTelemetry(uint64_t generation) noexcept
{
    if (!generation || generation != m_telemetry.operationGeneration ||
        !TableViewTelemetry::IsOperationStarted(m_telemetry)) { return; }
    TableViewTelemetry::CompleteOperation(m_telemetry, generation, TableViewTelemetry::Result::Success);
}

void TableView::AttachSourceTelemetry(winrt::TableViewSource const& source)
{
    auto const sourceImpl = winrt::get_self<::TableViewSource>(source);
    auto const weakThis = get_weak();
    auto const weakSource = winrt::make_weak(source);
    sourceImpl->SetShapingOperationHandlers(*this,
        [weakThis, weakSource](TableViewTelemetry::Operation operation, bool admitted) noexcept -> uint64_t
        {
            if (auto self = weakThis.get())
            {
                ++self->m_telemetrySourceMutationDepth;
                if (self->m_activeSource.get() != weakSource.get())
                {
                    ++self->m_telemetryMutationDepth;
                    return 0;
                }
                if (operation == TableViewTelemetry::Operation::Sort &&
                    (self->m_isApplyingControlInitiatedSort || self->m_controlSortTelemetryGeneration))
                {
                    ++self->m_telemetryMutationDepth;
                    return admitted && self->m_controlSortTelemetryGeneration &&
                        self->m_controlSortTelemetryGeneration != (std::numeric_limits<uint64_t>::max)() &&
                        self->m_controlSortTelemetryGeneration == self->m_telemetry.operationGeneration &&
                        self->m_telemetry.operation == operation &&
                        TableViewTelemetry::IsOperationStarted(self->m_telemetry) ?
                        self->m_controlSortTelemetryGeneration : 0;
                }
                return self->BeginOperationTelemetry(operation, admitted);
            }
            return 0;
        },
        [weakThis, weakSource](TableViewTelemetry::Operation operation, uint64_t generation,
            std::optional<TableViewTelemetry::IgnoreReason> reason, std::optional<HRESULT> error,
            TableViewTelemetry::AppFaultKind kind) noexcept
        {
            if (auto self = weakThis.get())
            {
                auto completeScope = gsl::finally([&self]() noexcept
                {
                    --self->m_telemetrySourceMutationDepth;
                    self->EndOperationTelemetry();
                });
                if (!generation || self->m_activeSource.get() != weakSource.get() ||
                    generation != self->m_telemetry.operationGeneration) { return; }
                auto const stage = operation == TableViewTelemetry::Operation::Filter ? TableViewTelemetry::Stage::Filter :
                    operation == TableViewTelemetry::Operation::Sort ? TableViewTelemetry::Stage::Sort : TableViewTelemetry::Stage::Grouping;
                if (reason)
                {
                    if (*reason == TableViewTelemetry::IgnoreReason::DeferredOrUnproven &&
                        operation == TableViewTelemetry::Operation::Sort && self->m_isApplyingControlInitiatedSort)
                    {
                        return;
                    }
                    TableViewTelemetry::IgnoreOperation(self->m_telemetry, generation, *reason, error, kind);
                    if (*reason == TableViewTelemetry::IgnoreReason::ApplicationFault ||
                        *reason == TableViewTelemetry::IgnoreReason::UnattributedFault)
                    {
                        TableViewTelemetry::IgnoreInitial(self->m_telemetry, *reason, stage, error, kind);
                        TableViewTelemetry::ReportError(self->m_telemetry, operation, stage, false,
                            kind == TableViewTelemetry::AppFaultKind::None ? TableViewTelemetry::Attribution::Unknown :
                                TableViewTelemetry::Attribution::Application, error, kind);
                    }
                }
                else if (operation == TableViewTelemetry::Operation::Sort && self->m_isApplyingControlInitiatedSort)
                {
                    self->m_controlSortPublishedGeneration = generation;
                }
                else { self->CompleteOperationTelemetry(generation); }
            }
        },
        [weakThis, weakSource](TableViewTelemetry::Operation operation, TableViewTelemetry::AppFaultKind kind, HRESULT error) noexcept
        {
            if (auto self = weakThis.get(); self && self->m_activeSource.get() == weakSource.get())
            {
                TableViewTelemetry::ReportError(self->m_telemetry, operation, TableViewTelemetry::Stage::Source, false,
                    kind == TableViewTelemetry::AppFaultKind::None ? TableViewTelemetry::Attribution::Unknown :
                        TableViewTelemetry::Attribution::Application, error, kind);
            }
        });
}
