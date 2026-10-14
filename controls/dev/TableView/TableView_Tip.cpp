// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewSource.h"

#include <exception>
#include <utility>

TableView::ActionTip::ActionTip(
    TableView& owner, TableViewTip::Operation operation, TableViewTip::Stage stage, bool admitted) noexcept :
    m_owner(owner), m_state(owner.m_telemetrySession), m_stage(stage), m_lifetimeGeneration(owner.m_tipLifetimeGeneration),
    m_exceptions(std::uncaught_exceptions())
{
    if (admitted && owner.m_telemetrySession.active && !owner.m_rowsSourceDrained)
    {
        m_controlGenerationAtAdmission = owner.m_tip.operationGeneration;
        m_generation = TableViewTip::BeginOperation(m_state, operation);
        if (m_generation) { m_previousAction = std::exchange(owner.m_activeActionTip, this); }
    }
}

TableView::ActionTip::~ActionTip()
{
    if (!m_generation) { return; }
    m_owner.m_activeActionTip = m_previousAction;
    if (m_lifetimeGeneration != m_owner.m_tipLifetimeGeneration)
    {
        TableViewTip::IgnoreOperation(m_state, m_generation, m_owner.m_tipCancellationReason);
        return;
    }
    if (m_appFaultKind != TableViewTip::AppFaultKind::None)
    {
        TableViewTip::IgnoreOperation(m_state, m_generation, TableViewTip::IgnoreReason::ApplicationFault, m_error, m_appFaultKind);
        TableViewTelemetry::ReportError(m_state.session, m_state.operationSessionGeneration, m_state.operation, m_stage, true,
            TableViewTip::Attribution::Application, m_error, m_appFaultKind);
    }
    else if (m_failed)
    {
        TableViewTip::FailOperation(m_state, m_generation, m_stage, m_error);
    }
    else if (std::uncaught_exceptions() > m_exceptions)
    {
        TableViewTip::IgnoreOperation(m_state, m_generation, TableViewTip::IgnoreReason::UnattributedFault);
        TableViewTelemetry::ReportError(m_state.session, m_state.operationSessionGeneration, m_state.operation, m_stage, false);
    }
    else if (m_completed)
    {
        TableViewTip::CompleteOperation(m_state, m_generation);
    }
    else
    {
        TableViewTip::IgnoreOperation(m_state, m_generation, m_ignoreReason);
    }
}

void TableView::ActionTip::Fail(std::optional<HRESULT> error) noexcept
{
    if (!m_failed) { m_error = error; }
    m_failed = true;
}

void TableView::ActionTip::ApplicationFault(TableViewTip::AppFaultKind kind, std::optional<HRESULT> error) noexcept
{
    if (m_appFaultKind == TableViewTip::AppFaultKind::None)
    {
        m_appFaultKind = kind;
        m_error = error;
    }
}

TableView::AppCalloutTip::AppCalloutTip(
    TableView& owner, TableViewTip::Stage stage, TableViewTip::AppFaultKind kind) noexcept :
    m_owner(owner), m_action(owner.m_activeActionTip &&
        (!TableViewTip::IsOperationStarted(owner.m_tip) ||
            owner.m_activeActionTip->ControlGenerationAtAdmission() == owner.m_tip.operationGeneration) ?
        owner.m_activeActionTip : nullptr),
    m_operation(TableViewTip::IsOperationStarted(owner.m_tip) ?
        owner.m_tip.operation : TableViewTip::Operation::Setup),
    m_stage(stage), m_kind(kind), m_generation(owner.m_tip.operationGeneration),
    m_lifetime(owner.m_tipLifetimeGeneration)
{
}

void TableView::AppCalloutTip::Fault(std::optional<HRESULT> error) noexcept
{
    if (m_lifetime != m_owner.m_tipLifetimeGeneration) { return; }
    if (m_action)
    {
        m_action->ApplicationFault(m_kind, error);
    }
    else if (m_generation == m_owner.m_tip.operationGeneration)
    {
        TableViewTip::IgnoreOperation(m_owner.m_tip, m_generation,
            TableViewTip::IgnoreReason::ApplicationFault, error, m_kind);
        TableViewTip::IgnoreSetup(m_owner.m_tip, TableViewTip::IgnoreReason::ApplicationFault, error, m_kind);
        TableViewTelemetry::ReportError(m_owner.m_telemetrySession, 0, m_operation, m_stage, true,
            TableViewTip::Attribution::Application, error, m_kind);
    }
}

void TableView::IgnoreTip(TableViewTip::IgnoreReason reason) noexcept
{
    ++m_tipLifetimeGeneration;
    m_tipCancellationReason = reason;
    TableViewTip::IgnoreSetup(m_tip, reason);
    TableViewTip::IgnoreOperation(m_tip, m_tip.operationGeneration, reason);
}

uint64_t TableView::BeginTipOperation(TableViewTip::Operation operation, bool admitted) noexcept
{
    ++m_tipMutationDepth;
    if (!admitted) { return 0; }
    if (m_telemetrySession.active && !m_rowsSourceDrained)
    {
        return TableViewTip::BeginOperation(m_tip, operation);
    }
    return 0;
}

void TableView::EndTipOperation() noexcept
{
    --m_tipMutationDepth;
}

void TableView::SupersedeTipOperation() noexcept
{
    if (!TableViewTip::IsOperationStarted(m_tip)) { return; }
    // Control-owned sort writes notify inside their own scope; a Sorted handler can reshape
    // after that scope ends but before the enclosing TiP operation unwinds.
    if (m_tipMutationDepth &&
        (m_tip.operation != TableViewTip::Operation::Sort ||
            m_isApplyingControlInitiatedSort || m_tipSourceMutationDepth))
    {
        return;
    }
    TableViewTip::IgnoreOperation(m_tip, m_tip.operationGeneration, TableViewTip::IgnoreReason::Superseded);
}

void TableView::IgnoreUnattributedTipOperation(
    TableViewTip::Operation operation, uint64_t generation, TableViewTip::Stage stage, std::optional<HRESULT> error) noexcept
{
    if (!generation || generation != m_tip.operationGeneration) { return; }
    if (TableViewTip::IsOperationHandled(m_tip, operation, generation)) { return; }
    TableViewTip::IgnoreOperation(m_tip, generation, TableViewTip::IgnoreReason::UnattributedFault, error);
    TableViewTelemetry::ReportError(m_telemetrySession, 0, operation, stage, false, TableViewTip::Attribution::Unknown, error);
}

void TableView::CompleteTipOperation(uint64_t generation) noexcept
{
    if (!generation || generation != m_tip.operationGeneration ||
        !TableViewTip::IsOperationStarted(m_tip)) { return; }
    TableViewTip::CompleteOperation(m_tip, generation);
}

void TableView::AttachSourceTip(winrt::TableViewSource const& source)
{
    auto const sourceImpl = winrt::get_self<::TableViewSource>(source);
    auto const weakThis = get_weak();
    auto const weakSource = winrt::make_weak(source);
    sourceImpl->SetShapingTipHandlers(*this,
        [weakThis, weakSource](TableViewTip::Operation operation, bool admitted) noexcept -> uint64_t
        {
            if (auto self = weakThis.get())
            {
                ++self->m_tipSourceMutationDepth;
                if (self->m_activeSource.get() != weakSource.get())
                {
                    ++self->m_tipMutationDepth;
                    return 0;
                }
                if (operation == TableViewTip::Operation::Sort &&
                    (self->m_isApplyingControlInitiatedSort || self->m_controlSortTipGeneration))
                {
                    ++self->m_tipMutationDepth;
                    return admitted && self->m_controlSortTipGeneration &&
                        self->m_controlSortTipGeneration != (std::numeric_limits<uint64_t>::max)() &&
                        self->m_controlSortTipGeneration == self->m_tip.operationGeneration &&
                        self->m_tip.operation == operation &&
                        TableViewTip::IsOperationStarted(self->m_tip) ?
                        self->m_controlSortTipGeneration : 0;
                }
                return self->BeginTipOperation(operation, admitted);
            }
            return 0;
        },
        [weakThis, weakSource](TableViewTip::Operation operation, uint64_t generation,
            std::optional<TableViewTip::IgnoreReason> reason, std::optional<HRESULT> error,
            TableViewTip::AppFaultKind kind) noexcept
        {
            if (auto self = weakThis.get())
            {
                auto completeScope = gsl::finally([&self]() noexcept
                {
                    --self->m_tipSourceMutationDepth;
                    self->EndTipOperation();
                });
                if (!generation || self->m_activeSource.get() != weakSource.get() ||
                    generation != self->m_tip.operationGeneration) { return; }
                auto const stage = operation == TableViewTip::Operation::Filter ? TableViewTip::Stage::Filter :
                    operation == TableViewTip::Operation::Sort ? TableViewTip::Stage::Sort : TableViewTip::Stage::Grouping;
                if (reason)
                {
                    if (*reason == TableViewTip::IgnoreReason::DeferredOrUnproven &&
                        operation == TableViewTip::Operation::Sort && self->m_isApplyingControlInitiatedSort)
                    {
                        return;
                    }
                    // A callout that already closed this operation has reported the fault.
                    if (TableViewTip::IsOperationHandled(self->m_tip, operation, generation)) { return; }
                    TableViewTip::IgnoreOperation(self->m_tip, generation, *reason, error, kind);
                    if (*reason == TableViewTip::IgnoreReason::ApplicationFault ||
                        *reason == TableViewTip::IgnoreReason::UnattributedFault)
                    {
                        TableViewTelemetry::ReportError(self->m_telemetrySession, 0, operation, stage, false,
                            kind == TableViewTip::AppFaultKind::None ? TableViewTip::Attribution::Unknown :
                                TableViewTip::Attribution::Application, error, kind);
                    }
                }
                else if (operation == TableViewTip::Operation::Sort && self->m_isApplyingControlInitiatedSort)
                {
                    self->m_controlSortPublishedGeneration = generation;
                }
                else { self->CompleteTipOperation(generation); }
            }
        });
}
