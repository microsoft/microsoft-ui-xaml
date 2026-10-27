// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewSource.h"

#include <exception>
#include <utility>

TableView::ActionTip::ActionTip(
    TableView& owner, TableViewTip::Operation operation, TableViewTip::Stage stage, bool admitted, bool withTest) noexcept :
    m_owner(owner), m_stage(stage)
{
    // An action that is not tracked does no TiP work at all.
    if (admitted && owner.TracksTipActions())
    {
        m_state.emplace(owner.m_telemetrySession);
        m_lifetimeGeneration = owner.m_tipLifetimeGeneration;
        m_exceptions = std::uncaught_exceptions();
        m_controlGenerationAtAdmission = owner.m_tip.operationGeneration;
        m_generation = TableViewTip::BeginOperation(*m_state, operation,
            withTest ? owner.TipMetricsInputs(operation) : TableViewTip::MetricsInputs{}, withTest);
        if (m_generation) { m_previousAction = std::exchange(owner.m_activeActionTip, this); }
    }
}

TableView::ActionTip::~ActionTip()
{
    if (!m_generation) { return; }
    m_owner.m_activeActionTip = m_previousAction;
    auto& state = *m_state;
    if (m_lifetimeGeneration != m_owner.m_tipLifetimeGeneration)
    {
        TableViewTip::IgnoreOperation(state, m_generation, m_owner.m_tipCancellationReason);
        return;
    }

    // An exception leaving the action is the app's when app code threw it (Escaping records it in a catch).
    auto const escaping = std::uncaught_exceptions() > m_exceptions;
    auto const appEscaped = escaping && m_escaping && m_escaping->appFault != TableViewTip::AppFaultKind::None;
    // Diagnostics keep one report per operation, attribution and recoverability, so the app fault that decided
    // the outcome goes first. A fault that stopped the action (its exception left it, or its write did not
    // land) is not recoverable.
    if (appEscaped) { ReportAppFault(m_escaping->appFault, m_escaping->error, false); }
    if (m_appFaultKind != TableViewTip::AppFaultKind::None) { ReportAppFault(m_appFaultKind, m_appFaultError, false); }
    if (m_recoveredKind != TableViewTip::AppFaultKind::None) { ReportAppFault(m_recoveredKind, m_recoveredError, true); }

    // An app fault never overrides TableView's own failure in the same action.
    if (m_failed || (escaping && !appEscaped))
    {
        auto const escapingError = m_escaping ? std::optional{ m_escaping->error } : std::nullopt;
        TableViewTip::FailOperation(state, m_generation, m_stage, m_failError ? m_failError : escapingError);
    }
    else if (appEscaped)
    {
        TableViewTip::IgnoreOperation(state, m_generation, TableViewTip::IgnoreReason::ApplicationFault,
            m_escaping->error, m_escaping->appFault);
    }
    else if (m_appFaultKind != TableViewTip::AppFaultKind::None)
    {
        TableViewTip::IgnoreOperation(state, m_generation, TableViewTip::IgnoreReason::ApplicationFault,
            m_appFaultError, m_appFaultKind);
    }
    else if (m_completed)
    {
        TableViewTip::CompleteOperation(state, m_generation);
    }
    else
    {
        TableViewTip::IgnoreOperation(state, m_generation, m_ignoreReason);
    }
}

void TableView::ActionTip::ReportAppFault(TableViewTip::AppFaultKind kind, std::optional<HRESULT> error, bool recoverable) noexcept
{
    auto& state = *m_state;
    TableViewTelemetry::ReportError(state.session, state.operationSessionGeneration, state.operation, m_stage, recoverable,
        TableViewTip::Attribution::Application, error, kind);
}

void TableView::ActionTip::Fail(std::optional<HRESULT> error) noexcept
{
    if (!m_failed) { m_failError = error; }
    m_failed = true;
}

void TableView::ActionTip::ApplicationFault(TableViewTip::AppFaultKind kind, std::optional<HRESULT> error) noexcept
{
    if (m_appFaultKind == TableViewTip::AppFaultKind::None)
    {
        m_appFaultKind = kind;
        m_appFaultError = error;
    }
}

void TableView::ActionTip::RecoveredAppFault(TableViewTip::AppFaultKind kind, std::optional<HRESULT> error) noexcept
{
    if (m_recoveredKind == TableViewTip::AppFaultKind::None)
    {
        m_recoveredKind = kind;
        m_recoveredError = error;
    }
}

// The action an app callout runs under, unless a control operation started after that action did.
TableView::ActionTip* TableView::CalloutAction() const noexcept
{
    return m_activeActionTip &&
        (!TableViewTip::IsOperationStarted(m_tip) ||
            m_activeActionTip->ControlGenerationAtAdmission() == m_tip.operationGeneration) ?
        m_activeActionTip : nullptr;
}

void TableView::NoteRecoveredAppFault(TableViewTip::Operation operation, TableViewTip::Stage stage,
    TableViewTip::AppFaultKind kind, std::optional<HRESULT> error) noexcept
{
    if (auto const action = CalloutAction())
    {
        action->RecoveredAppFault(kind, error);
        return;
    }
    // Outside an action: the control operation in flight, else initialization, else the callout's own operation.
    auto const reported = TableViewTip::IsOperationStarted(m_tip) ? m_tip.operation :
        m_tip.initializationPending ? TableViewTip::Operation::Setup : operation;
    TableViewTelemetry::ReportError(m_telemetrySession, 0, reported, stage, true,
        TableViewTip::Attribution::Application, error, kind);
}

void TableView::NoteColumnFault(ActionTip* tip, TableViewTip::Operation operation, HRESULT error,
    std::optional<TableViewTip::AppFaultKind> appFault) noexcept
{
    if (appFault)
    {
        if (tip) { tip->RecoveredAppFault(*appFault, error); }
        else { NoteRecoveredAppFault(operation, TableViewTip::Stage::Editing, *appFault, error); }
    }
    else if (tip) { tip->Fail(error); }
    else
    {
        TableViewTelemetry::ReportError(m_telemetrySession, 0, operation, TableViewTip::Stage::Editing, true,
            TableViewTip::Attribution::Product, error);
    }
}

void TableView::IgnoreTip(TableViewTip::IgnoreReason reason) noexcept
{
    ++m_tipLifetimeGeneration;
    m_tipCancellationReason = reason;
    TableViewTip::IgnoreInitialization(m_tip, reason);
    TableViewTip::IgnoreOperation(m_tip, m_tip.operationGeneration, reason);
}

// The metrics bucket's inputs, for a TiP test about to start.
TableViewTip::MetricsInputs TableView::TipMetricsInputs() noexcept
{
    TableViewTip::MetricsInputs metrics;
    try
    {
        metrics.rightToLeft = FlowDirection() == winrt::FlowDirection::RightToLeft;
        // Source row counts are disabled while the source is moved behind ITableViewSource, which does not expose them.
        // if (auto const source = m_activeSource.get())
        // {
        //     auto const* const sourceImpl = winrt::get_self<::TableViewSource>(source);
        //     metrics.sourceRows = sourceImpl->SourceRowCount();
        //     metrics.dataRows = sourceImpl->DataRowCount();
        // }
    }
    catch (...)
    {
        // The bucket is context only; the test still runs.
    }
    return metrics;
}

// Read only for an operation with a TiP test.
TableViewTip::MetricsInputs TableView::TipMetricsInputs(TableViewTip::Operation operation) noexcept
{
    return TableViewTip::OperationTestIdentity(operation) ? TipMetricsInputs() : TableViewTip::MetricsInputs{};
}

void TableView::StartTipInitialization() noexcept
{
    if (m_tip.initializationPending || m_tip.initializationDone) { return; }
    TableViewTip::StartInitialization(m_tip, TipMetricsInputs());
}

void TableView::CompleteTipInitializationIfEmpty() noexcept
{
    if (!m_tip.initializationPending) { return; }
    try
    {
        auto const source = m_activeSource.get();
        auto const view = source ? GetTableViewSource(source)->GetItemsSourceView() : nullptr;
        if (!view || view.Count() == 0) { TableViewTip::CompleteInitialization(m_tip); }
    }
    catch (...)
    {
        // Left running: the first realized row, or unload, decides.
    }
}

void TableView::NoteTipInitializationSource() noexcept
{
    if (m_templateApplied && m_activeSource.get()) { StartTipInitialization(); }
    CompleteTipInitializationIfEmpty();
}

void TableView::FailTipInitialization(TableViewTip::Stage stage, TableViewTip::Fault fault) noexcept
{
    StartTipInitialization();
    if (fault.appFault != TableViewTip::AppFaultKind::None)
    {
        TableViewTip::IgnoreInitialization(m_tip, TableViewTip::IgnoreReason::ApplicationFault, fault.error, fault.appFault);
        TableViewTelemetry::ReportError(m_telemetrySession, 0, TableViewTip::Operation::Setup, stage,
            false, TableViewTip::Attribution::Application, fault.error, fault.appFault);
    }
    else
    {
        TableViewTip::FailInitialization(m_tip, stage, fault.error);
    }
}

// The first realized row or group header arms this once; its layout pass completes Initialize.
void TableView::ArmTipInitializationLayout() noexcept
{
    try
    {
        auto weakThis = get_weak();
        m_tipInitializationLayoutToken = LayoutUpdated(
            [weakThis](winrt::IInspectable const&, winrt::IInspectable const&)
            {
                if (auto strongThis = weakThis.get())
                {
                    strongThis->RevokeTipInitializationLayout();
                    TableViewTip::CompleteInitialization(strongThis->m_tip);
                }
            });
    }
    catch (...)
    {
        // Left unarmed: the next prepared element retries, or unload closes Initialize.
    }
}

void TableView::RevokeTipInitializationLayout() noexcept
{
    if (m_tipInitializationLayoutToken.value)
    {
        try { LayoutUpdated(m_tipInitializationLayoutToken); }
        catch (...) {}
        m_tipInitializationLayoutToken = {};
    }
}

void TableView::IgnoreTipInitializationUnloadedBeforeRender() noexcept
{
    // Still waiting, the table laid out no row: a realized row or group header only arms the callback, and the layout
    // pass that runs it completes Initialize (ArmTipInitializationLayout).
    if (!m_tip.initializationPending) { return; }
    try
    {
        auto const source = m_activeSource.get();
        auto const view = source ? GetTableViewSource(source)->GetItemsSourceView() : nullptr;
        if (!view || view.Count() == 0) { return; }
        if (HasRowsViewport()) { TableViewTip::IgnoreInitialization(m_tip, TableViewTip::IgnoreReason::UnloadedBeforeRender); }
    }
    catch (...)
    {
        // Left to the unload that follows.
    }
}

uint64_t TableView::BeginTipOperation(TableViewTip::Operation operation, bool admitted) noexcept
{
    ++m_tipMutationDepth;
    if (!admitted) { return 0; }
    // Before the first Loaded only a source swap is tracked: that is when apps usually set ItemsSource.
    auto const beforeFirstLoad = m_telemetrySession.generation == 0 && operation == TableViewTip::Operation::ReplaceSource;
    if ((m_telemetrySession.active || beforeFirstLoad) && !m_rowsSourceDrained)
    {
        return TableViewTip::BeginOperation(m_tip, operation, TipMetricsInputs(operation));
    }
    return 0;
}

void TableView::FailTipOperation(uint64_t generation, TableViewTip::Stage stage, std::optional<TableViewTip::Fault> fault) noexcept
{
    if (!generation || generation != m_tip.operationGeneration || !TableViewTip::IsOperationStarted(m_tip)) { return; }
    if (fault && fault->appFault != TableViewTip::AppFaultKind::None)
    {
        auto const operation = m_tip.operation;
        auto const startedIn = m_tip.operationSessionGeneration;
        TableViewTip::IgnoreOperation(m_tip, generation, TableViewTip::IgnoreReason::ApplicationFault,
            fault->error, fault->appFault);
        TableViewTelemetry::ReportError(m_telemetrySession, startedIn, operation, stage, false,
            TableViewTip::Attribution::Application, fault->error, fault->appFault);
    }
    else { TableViewTip::FailOperation(m_tip, generation, stage, fault ? std::optional{ fault->error } : std::nullopt); }
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

void TableView::CompleteTipOperation(uint64_t generation) noexcept
{
    if (!generation || generation != m_tip.operationGeneration ||
        !TableViewTip::IsOperationStarted(m_tip)) { return; }
    TableViewTip::CompleteOperation(m_tip, generation);
}

// Disabled while the source is moved behind ITableViewSource, which does not expose SetShapingTipHandlers.
#if 0
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
        [weakThis, weakSource](TableViewTip::Operation operation, uint64_t generation, bool failed,
            std::optional<TableViewTip::IgnoreReason> reason, std::optional<HRESULT> error,
            TableViewTip::AppFaultKind kind, TableViewTip::ContainedFaults contained) noexcept
        {
            if (auto self = weakThis.get())
            {
                auto completeScope = gsl::finally([&self]() noexcept
                {
                    --self->m_tipSourceMutationDepth;
                    self->EndTipOperation();
                });
                if (self->m_activeSource.get() != weakSource.get()) { return; }
                auto const stage = operation == TableViewTip::Operation::Filter ? TableViewTip::Stage::Filter :
                    operation == TableViewTip::Operation::Sort ? TableViewTip::Stage::Sort : TableViewTip::Stage::Grouping;
                // The verb carried on past app delegates that threw; whether or not it has a test, that is the app's.
                if (contained.count)
                {
                    TableViewTelemetry::ReportError(self->m_telemetrySession, 0, operation, stage, true,
                        TableViewTip::Attribution::Application, contained.firstError, TableViewTip::AppFaultKind::Callback,
                        contained.count);
                }
                if (!generation || generation != self->m_tip.operationGeneration) { return; }
                // Already closed under this generation (unload, retemplate, supersession or the control's own
                // scope); that outcome stands.
                if (TableViewTip::IsOperationHandled(self->m_tip, operation, generation)) { return; }
                if (failed)
                {
                    TableViewTip::FailOperation(self->m_tip, generation, stage, error);
                }
                else if (reason)
                {
                    if (*reason == TableViewTip::IgnoreReason::DeferredOrUnproven &&
                        operation == TableViewTip::Operation::Sort && self->m_isApplyingControlInitiatedSort)
                    {
                        return;
                    }
                    TableViewTip::IgnoreOperation(self->m_tip, generation, *reason, error, kind);
                    if (*reason == TableViewTip::IgnoreReason::ApplicationFault)
                    {
                        TableViewTelemetry::ReportError(self->m_telemetrySession, 0, operation, stage, false,
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
#endif
