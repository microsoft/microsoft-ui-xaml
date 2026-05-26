// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.system.power.h>
#include <Private.Infrastructure.h>

namespace Private { namespace Infrastructure {

    class MockPowerManager : public Microsoft::WRL::AgileActivationFactory<wsyp::IPowerManagerStatics> {

        InspectableClassStatic(RuntimeClass_Windows_System_Power_PowerManager,
            BaseTrust);

    public:

        // IPowerManager methods

        void put_EnergySaverStatus(_In_ wsyp::EnergySaverStatus energyStatus);

        IFACEMETHOD(get_EnergySaverStatus) (
            _Out_ wsyp::EnergySaverStatus *Value
            );

        IFACEMETHOD(add_EnergySaverStatusChanged) (
            _In_ wf::IEventHandler<IInspectable *> *Handler,
            _Out_ EventRegistrationToken *Token
            );

        IFACEMETHOD(remove_EnergySaverStatusChanged) (
            EventRegistrationToken Token
            );

        void put_BatteryStatus(_In_ wsyp::BatteryStatus powerStatus);

        IFACEMETHOD(get_BatteryStatus) (
            _Out_ wsyp::BatteryStatus *Value
            );

        IFACEMETHOD(add_BatteryStatusChanged) (
            _In_ wf::IEventHandler<IInspectable *> *Handler,
            _Out_ EventRegistrationToken *Token
            );

        IFACEMETHOD(remove_BatteryStatusChanged) (
            EventRegistrationToken Token
            );

        void put_PowerSupplyStatus(_In_ wsyp::PowerSupplyStatus powerStatus);

        IFACEMETHOD(get_PowerSupplyStatus) (
            _Out_ wsyp::PowerSupplyStatus *Value
            );

        IFACEMETHOD(add_PowerSupplyStatusChanged) (
            _In_ wf::IEventHandler<IInspectable *> *Handler,
            _Out_ EventRegistrationToken *Token
            );

        IFACEMETHOD(remove_PowerSupplyStatusChanged) (
            EventRegistrationToken Token
            );

        void put_RemainingChargePercent(_In_ int value);

        IFACEMETHOD(get_RemainingChargePercent) (
            _Out_ int *Value
            );

        IFACEMETHOD(add_RemainingChargePercentChanged) (
            _In_ wf::IEventHandler<IInspectable *> *Handler,
            _Out_ EventRegistrationToken *Token
            );

        IFACEMETHOD(remove_RemainingChargePercentChanged) (
            EventRegistrationToken Token
            );

        void put_RemainingDischargeTime(_In_ wf::TimeSpan value);
        IFACEMETHOD(get_RemainingDischargeTime) (
            _Out_ wf::TimeSpan *Value
            );

        IFACEMETHOD(add_RemainingDischargeTimeChanged) (
            _In_ wf::IEventHandler<IInspectable *> *Handler,
            _Out_ EventRegistrationToken *Token
            );

        IFACEMETHOD(remove_RemainingDischargeTimeChanged) (
            EventRegistrationToken Token
            );

    private:

        wsyp::BatteryStatus m_batteryStatus = wsyp::BatteryStatus_NotPresent;
        Microsoft::WRL::AgileEventSource<wf::IEventHandler<IInspectable *>> m_batteryStatusChanged;

        wsyp::EnergySaverStatus m_energySaverStatus = wsyp::EnergySaverStatus_Disabled;
        Microsoft::WRL::AgileEventSource<wf::IEventHandler<IInspectable *>> m_energySaverStatusChanged;

        wsyp::PowerSupplyStatus m_powerSupplyStatus = wsyp::PowerSupplyStatus_Adequate;
        Microsoft::WRL::AgileEventSource<wf::IEventHandler<IInspectable *>> m_powerSupplyStatusChanged;

        int m_remainingChargePercent = 100;
        Microsoft::WRL::AgileEventSource<wf::IEventHandler<IInspectable *>> m_remainingChargePercentChanged;

        wf::TimeSpan m_remainingDischargeTime = { MAXLONGLONG };
        Microsoft::WRL::AgileEventSource<wf::IEventHandler<IInspectable *>> m_remainingDischargeTimeChanged;
    };

} }

