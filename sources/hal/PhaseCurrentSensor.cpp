/* Copyright (C) 2015  Nils Weiss
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include "PhaseCurrentSensor.h"
#include "trace.h"
#include "log2.h"
#include "medianFilter.h"
#include <algorithm>

static const int __attribute__((unused)) g_DebugZones = ZONE_ERROR | ZONE_WARNING | ZONE_VERBOSE | ZONE_INFO;

using hal::AdcWithDma;
using hal::Factory;
using hal::HalfBridge;
using hal::PhaseCurrentSensor;
using hal::Tim;

void PhaseCurrentSensor::setPulsWidthForTriggerPerMill(uint32_t value) const
{
    static constexpr uint32_t maxValue = 1000;
    if (value > maxValue) {
        value = maxValue;
    }

    static const float scale = static_cast<float>(mHBridge.mTim.mConfiguration.TIM_Period) /
                               static_cast<float>(maxValue);

    value = static_cast<uint32_t>(static_cast<float>(value) * scale);

    const uint32_t sampleTime = mAdcWithDma.mAdcChannel.mSampleTime; // TODO prepare Value that it is correct

    TIM_SetCompare4(mHBridge.mTim.getBasePointer(),
                    static_cast<uint32_t>(std::max(
                                                   static_cast<int32_t>((value >> 1) - sampleTime),
                                                   static_cast<int32_t>(1))));
}

void PhaseCurrentSensor::updateCurrentValue(void) const
{
    const auto& array = MeasurementValueBuffer[mDescription];

    uint32_t sum = 0;
    for (size_t i = 0; i < array.size(); i++) {
        sum += array[i];
    }
    mPhaseCurrentValue = static_cast<float>(sum) / static_cast<float>(array.size());
}

void PhaseCurrentSensor::registerValueAvailableSemaphore(os::Semaphore* valueAvailable) const
{
    mAdcWithDma.mDma.registerInterruptSemaphore(valueAvailable, hal::Dma::InterruptSource::TC);
}

void PhaseCurrentSensor::unregisterValueAvailableSemaphore(void) const
{
    mAdcWithDma.mDma.unregisterInterruptSemaphore(hal::Dma::InterruptSource::TC);
}

void PhaseCurrentSensor::enable(void) const
{
    mAdcWithDma.startConversion(MeasurementValueBuffer[mDescription], [&] {this->updateCurrentValue();
                                });
}

void PhaseCurrentSensor::disable(void) const
{
    mAdcWithDma.stopConversion();
}

void PhaseCurrentSensor::setOffset(const float offset) const
{
    this->mOffsetVoltage = offset;
}

float PhaseCurrentSensor::getPhaseCurrent(void) const
{
    static constexpr const float SHUNT_CONDUCTANCE = 1 / SHUNT_RESISTANCE;
    return (mAdcWithDma.getVoltage(mPhaseCurrentValue) -
            mOffsetVoltage) *
           SHUNT_CONDUCTANCE / MEASUREMENT_GAIN;
}

void PhaseCurrentSensor::initialize(void) const
{
    TIM_OC4Init(mHBridge.mTim.getBasePointer(), &mAdcTrgoConfiguration);
    TIM_OC4PreloadConfig(mHBridge.mTim.getBasePointer(), TIM_OCPreload_Enable);

    TIM_SelectMasterSlaveMode(mHBridge.mTim.getBasePointer(), TIM_MasterSlaveMode_Enable);

    /* Channel 4 output compare signal is connected to TRGO */
    TIM_SelectOutputTrigger(mHBridge.mTim.getBasePointer(), (uint16_t)TIM_TRGOSource_OC4Ref);
}

constexpr const std::array<const PhaseCurrentSensor,
                           PhaseCurrentSensor::Description::__ENUM__SIZE> Factory<PhaseCurrentSensor>::Container;
std::array<std::array<uint16_t,
                      PhaseCurrentSensor::NUMBER_OF_MEASUREMENTS_FOR_AVG>,
           PhaseCurrentSensor::Description::__ENUM__SIZE> PhaseCurrentSensor::MeasurementValueBuffer;