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

#ifndef SOURCES_PMD_USARTWITHDMA_CONFIG_CONTAINER_H_
#define SOURCES_PMD_USARTWITHDMA_CONFIG_CONTAINER_H_

static constexpr const std::array<const UsartWithDma, 1> Container =
{ {
      UsartWithDma(Factory<Usart>::get<Usart::MSCOM_IF>(), USART_DMAReq_Rx | USART_DMAReq_Tx,
                   &Factory<Dma>::get<Dma::USART2_TX>(), &Factory<Dma>::get<Dma::USART2_RX>())
  } };

#endif /* SOURCES_PMD_USART_CONFIG_CONTAINER_H_ */
