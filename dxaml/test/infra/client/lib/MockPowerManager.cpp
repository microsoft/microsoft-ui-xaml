// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MockPowerManager.h"

using namespace Microsoft::WRL::Wrappers;

namespace Private { namespace Infrastructure {

    void MockPowerManager::put_EnergySaverStatus(_In_ wsyp::EnergySaverStatus value)
    {
        m_energySaverStatus = value;
        LogThrow_IfFailed(m_energySaverStatusChanged.InvokeAll(nullptr, nullptr));
    }

    IFACEMETHODIMP MockPowerManager::get_EnergySaverStatus(_Out_ wsyp::EnergySaverStatus *value)
    {
        *value = m_energySaverStatus;
        return S_OK;
    }

    IFACEMETHODIMP MockPowerManager::add_EnergySaverStatusChanged(
        _In_ wf::IEventHandler<IInspectable *> *handler,
        _Out_ EventRegistrationToken *token)
    {
        return m_energySaverStatusChanged.Add(handler, token);
    }

    IFACEMETHODIMP MockPowerManager::remove_EnergySaverStatusChanged(EventRegistrationToken token)
    {
        return m_energySaverStatusChanged.Remove(token);
    }

    void MockPowerManager::put_BatteryStatus(_In_ wsyp::BatteryStatus value)
    {
        m_batteryStatus = value;
        LogThrow_IfFailed(m_batteryStatusChanged.InvokeAll(nullptr, nullptr));
    }

    IFACEMETHODIMP MockPowerManager::get_BatteryStatus(_Out_ wsyp::BatteryStatus *value)
    {
        *value = m_batteryStatus;
        return S_OK;
    }

    IFACEMETHODIMP MockPowerManager::add_BatteryStatusChanged(
            _In_ wf::IEventHandler<IInspectable *> *handler,
            _Out_ EventRegistrationToken *token)
    {
        return m_batteryStatusChanged.Add(handler, token);
    }

    IFACEMETHODIMP MockPowerManager::remove_BatteryStatusChanged(EventRegistrationToken token)
    {
        return m_batteryStatusChanged.Remove(token);
    }

    void MockPowerManager::put_PowerSupplyStatus(_In_ wsyp::PowerSupplyStatus value)
    {
        m_powerSupplyStatus = value;
        LogThrow_IfFailed(m_powerSupplyStatusChanged.InvokeAll(nullptr, nullptr));
    }

    IFACEMETHODIMP MockPowerManager::get_PowerSupplyStatus(_Out_ wsyp::PowerSupplyStatus *value)
    {
        *value = m_powerSupplyStatus;
        return S_OK;
    }

    IFACEMETHODIMP MockPowerManager::add_PowerSupplyStatusChanged(
        _In_ wf::IEventHandler<IInspectable *> *handler,
        _Out_ EventRegistrationToken *token)
    {
        return m_powerSupplyStatusChanged.Add(handler, token);
    }

    IFACEMETHODIMP MockPowerManager::remove_PowerSupplyStatusChanged(EventRegistrationToken token)
    {
        return m_powerSupplyStatusChanged.Remove(token);
    }

    void MockPowerManager::put_RemainingChargePercent(_In_ int value)
    {
        m_remainingChargePercent = value;
        LogThrow_IfFailed(m_remainingChargePercentChanged.InvokeAll(nullptr, nullptr));
    }

    IFACEMETHODIMP MockPowerManager::get_RemainingChargePercent(_Out_ int *value)
    {
        *value = m_remainingChargePercent;
        return S_OK;
    }

    IFACEMETHODIMP MockPowerManager::add_RemainingChargePercentChanged(
        _In_ wf::IEventHandler<IInspectable *> *handler,
        _Out_ EventRegistrationToken *token)
    {
        return m_remainingChargePercentChanged.Add(handler, token);
    }

    IFACEMETHODIMP MockPowerManager::remove_RemainingChargePercentChanged(EventRegistrationToken token)
    {
        return m_remainingChargePercentChanged.Remove(token);
    }

    void MockPowerManager::put_RemainingDischargeTime(_In_ wf::TimeSpan value)
    {
        m_remainingDischargeTime = value;
        LogThrow_IfFailed(m_remainingDischargeTimeChanged.InvokeAll(nullptr, nullptr));
    }

    IFACEMETHODIMP MockPowerManager::get_RemainingDischargeTime(_Out_ wf::TimeSpan *value)
    {
        *value = m_remainingDischargeTime;
        return S_OK;
    }

    IFACEMETHODIMP MockPowerManager::add_RemainingDischargeTimeChanged(
        _In_ wf::IEventHandler<IInspectable *> *handler,
        _Out_ EventRegistrationToken *token)
    {
        return m_remainingDischargeTimeChanged.Add(handler, token);
    }

    IFACEMETHODIMP MockPowerManager::remove_RemainingDischargeTimeChanged(EventRegistrationToken token)
    {
        return m_remainingDischargeTimeChanged.Remove(token);
    }

} }
