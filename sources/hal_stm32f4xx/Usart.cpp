/* Copyright (C) 2018  Nils Weiss and Henning Mende
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

#include "Usart.h"
#include "trace.h"

static const int __attribute__((unused)) g_DebugZones = ZONE_ERROR | ZONE_WARNING | ZONE_VERBOSE | ZONE_INFO;

using hal::Factory;
using hal::Usart;

void Usart::initialize() const
{
    USART_DeInit(reinterpret_cast<USART_TypeDef*>(mPeripherie));
    USART_Init(reinterpret_cast<USART_TypeDef*>(mPeripherie), &mConfiguration);

    USART_Cmd(reinterpret_cast<USART_TypeDef*>(mPeripherie), ENABLE);
    mInitalized = true;

    // Initialize Interrupts
    // TODO Maybe also TX and not only RX will be needed.
    mNvic.setPriority(0xf);

    mNvic.registerGetInterruptStatusProcedure([this](void) -> bool {
        return USART_GetITStatus(reinterpret_cast<USART_TypeDef*>(this->
                                                                  mPeripherie),
                                 USART_IT_RXNE) == SET;
    });

    mNvic.registerClearInterruptProcedure([this](void) -> void {
        USART_ClearITPendingBit(reinterpret_cast<USART_TypeDef*>(this->
                                                                 mPeripherie),
                                USART_IT_RXNE);
    });

    mNvic.registerInterruptCallback([this](void) -> void {
        uint8_t databyte =
            static_cast<uint8_t>(USART_ReceiveData(reinterpret_cast<USART_TypeDef*>(
                                                                                    this
                                                                                    ->
                                                                                    mPeripherie)));
        Usart::ReceiveInterruptCallbacks[this->mDescription](databyte);
    });

    mNvic.enable();
}

//IRQn Usart::getIRQn(void) const
//{
//    switch (mPeripherie) {
//    case USART1_BASE:
//        return IRQn::USART1_IRQn;
//
//    case USART2_BASE:
//        return IRQn::USART2_IRQn;
//
//    case USART3_BASE:
//        return IRQn::USART3_IRQn;
//
//    case UART4_BASE:
//        return IRQn::UART4_IRQn;
//
//    case UART5_BASE:
//        return IRQn::UART5_IRQn;
//    }
//
//    return IRQn::UsageFault_IRQn;
//}

bool Usart::isInitalized(void) const
{
    return mInitalized;
}

void Usart::setBaudRate(const size_t baudRate) const
{
    USART_InitTypeDef configuration = mConfiguration;
    configuration.USART_BaudRate = baudRate;

    USART_Init(reinterpret_cast<USART_TypeDef*>(mPeripherie), &configuration);
    USART_Cmd(reinterpret_cast<USART_TypeDef*>(mPeripherie), ENABLE);
}

void Usart::enableNonBlockingReceive(std::function<void(uint8_t)> callback) const
{
    ReceiveInterruptCallbacks[mDescription] = callback;

    USART_ClearITPendingBit(reinterpret_cast<USART_TypeDef*>(mPeripherie), USART_IT_RXNE);
    USART_ITConfig(reinterpret_cast<USART_TypeDef*>(mPeripherie), USART_IT_RXNE, ENABLE);
}

void Usart::disableNonBlockingReceive(void) const
{
    ReceiveInterruptCallbacks[mDescription] = nullptr;

    USART_ITConfig(reinterpret_cast<USART_TypeDef*>(mPeripherie), USART_IT_RXNE, DISABLE);
}

void Usart::send(const uint16_t data) const
{
    USART_SendData(reinterpret_cast<USART_TypeDef*>(mPeripherie), data);
}

size_t Usart::send(uint8_t const* const data, const size_t length) const
{
    if (data == nullptr) {
        return 0;
    }

    size_t bytesSend = 0;
    while (bytesSend < length) {
        if (this->isReadyToSend()) {
            this->send(data[bytesSend]);
            bytesSend++;
        }
    }
    return bytesSend;
}

size_t Usart::send(uint16_t const* const data, const size_t length) const
{
    if (data == nullptr) {
        return 0;
    }

    size_t bytesSend = 0;
    while (bytesSend < length) {
        if (this->isReadyToSend()) {
            this->send(data[bytesSend]);
            bytesSend++;
        }
    }
    return bytesSend;
}

bool Usart::isReadyToReceive(void) const
{
    return USART_GetFlagStatus(reinterpret_cast<USART_TypeDef*>(mPeripherie), USART_FLAG_RXNE) == SET ? true : false;
}

uint16_t Usart::receive(void) const
{
    return USART_ReceiveData(reinterpret_cast<USART_TypeDef*>(mPeripherie));
}

size_t Usart::receive(uint8_t* const data, const size_t length) const
{
    if (data == nullptr) {
        return 0;
    }

    size_t bytesReceived = 0;
    while (bytesReceived < length) {
        if (this->isReadyToReceive()) {
            data[bytesReceived] = (uint8_t)this->receive();
            bytesReceived++;
        }
    }
    return bytesReceived;
}

size_t Usart::receive(uint16_t* const data, const size_t length) const
{
    if (data == nullptr) {
        return 0;
    }

    size_t bytesReceived = 0;
    while (bytesReceived < length) {
        if (this->isReadyToReceive()) {
            data[bytesReceived] = (uint8_t)this->receive();
            bytesReceived++;
        }
    }
    return bytesReceived;
}

size_t Usart::receiveAvailableData(uint8_t* const data, const size_t length) const
{
    if (data == nullptr) {
        return 0;
    }

    size_t bytesReceived = 0;
    while (bytesReceived < length) {
        if (this->isReadyToReceive()) {
            data[bytesReceived] = (uint8_t)receive();
            bytesReceived++;
        } else {
            return bytesReceived;
        }
    }
    return bytesReceived;
}

bool Usart::isReadyToSend(void) const
{
    return USART_GetFlagStatus(reinterpret_cast<USART_TypeDef*>(mPeripherie), USART_FLAG_TXE) == SET ? true : false;
}

bool Usart::hasOverRunError(void) const
{
    return USART_GetFlagStatus(reinterpret_cast<USART_TypeDef*>(mPeripherie), USART_FLAG_ORE) == SET ? true : false;
}

bool Usart::hasNoiseError(void) const
{
    return USART_GetFlagStatus(reinterpret_cast<USART_TypeDef*>(mPeripherie), USART_FLAG_NE) == SET ? true : false;
}

bool Usart::hasFramingError(void) const
{
    return USART_GetFlagStatus(reinterpret_cast<USART_TypeDef*>(mPeripherie), USART_FLAG_FE) == SET ? true : false;
}

bool Usart::hasParityError(void) const
{
    return USART_GetFlagStatus(reinterpret_cast<USART_TypeDef*>(mPeripherie), USART_FLAG_PE) == SET ? true : false;
}

void Usart::clearOverRunError(void) const
{
    USART_ClearFlag(reinterpret_cast<USART_TypeDef*>(mPeripherie), USART_FLAG_ORE);
}

void Usart::clearNoiseError(void) const
{
    USART_ClearFlag(reinterpret_cast<USART_TypeDef*>(mPeripherie), USART_FLAG_NE);
}

void Usart::clearFramingError(void) const
{
    USART_ClearFlag(reinterpret_cast<USART_TypeDef*>(mPeripherie), USART_FLAG_FE);
}

void Usart::clearParityError(void) const
{
    USART_ClearFlag(reinterpret_cast<USART_TypeDef*>(mPeripherie), USART_FLAG_PE);
}

Usart::ReceiveCallbackArray Usart::ReceiveInterruptCallbacks;

constexpr const std::array<const Usart, Usart::__ENUM__SIZE> Factory<Usart>::Container;
constexpr const std::array<const uint32_t, Usart::__ENUM__SIZE> Factory<Usart>::Clocks;
