/*
 * niso_spi_command.c
 *
 *  Created on: 03.04.2019
 *      Author: VBKesha
 */

/******************************************************************************
 *                                                                             *
 * License Agreement                                                           *
 *                                                                             *
 * Copyright (c) 2003 Altera Corporation, San Jose, California, USA.           *
 * All rights reserved.                                                        *
 *                                                                             *
 * Permission is hereby granted, free of charge, to any person obtaining a     *
 * copy of this software and associated documentation files (the "Software"),  *
 * to deal in the Software without restriction, including without limitation   *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
 * and/or sell copies of the Software, and to permit persons to whom the       *
 * Software is furnished to do so, subject to the following conditions:        *
 *                                                                             *
 * The above copyright notice and this permission notice shall be included in  *
 * all copies or substantial portions of the Software.                         *
 *                                                                             *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
 * DEALINGS IN THE SOFTWARE.                                                   *
 *                                                                             *
 * This agreement shall be governed in all respects by the laws of the State   *
 * of California and by the laws of the United States of America.              *
 *                                                                             *
 ******************************************************************************/

#include <stdint.h>
#include "alt_types.h"

#include <altera_avalon_spi_regs.h>
#include <altera_avalon_spi.h>

/* This is a very simple routine which performs one SPI master transaction.
 * It would be possible to implement a more efficient version using interrupts
 * and sleeping threads but this is probably not worthwhile initially.
 */
#if 0
int nios_spi_command3(uint32_t base, uint32_t slave, uint32_t write_length,
		const uint8_t * write_data, uint32_t read_length, uint8_t * read_data,
		uint32_t flags) {

	const alt_u8 * write_end = write_data + write_length;
	alt_u8 * read_end = read_data + read_length;

	alt_u32 write_zeros = read_length;
	alt_u32 read_ignore = write_length;
	alt_u32 status;

	/* We must not send more than two bytes to the target before it has
	 * returned any as otherwise it will overflow. */
	/* Unfortunately the hardware does not seem to work with credits > 1,
	 * leave it at 1 for now. */
	alt_32 credits = 1;

	/* Warning: this function is not currently safe if called in a multi-threaded
	 * environment, something above must perform locking to make it safe if more
	 * than one thread intends to use it.
	 */

	IOWR_ALTERA_AVALON_SPI_SLAVE_SEL(base, 1 << slave);

	/* Set the SSO bit (force chipselect) only if the toggle flag is not set */
	if ((flags & ALT_AVALON_SPI_COMMAND_TOGGLE_SS_N) == 0) {
		IOWR_ALTERA_AVALON_SPI_CONTROL(base, ALTERA_AVALON_SPI_CONTROL_SSO_MSK);
	}

	/*
	 * Discard any stale data present in the RXDATA register, in case
	 * previous communication was interrupted and stale data was left
	 * behind.
	 */
	IORD_ALTERA_AVALON_SPI_RXDATA(base);

	/* Keep clocking until all the data has been processed. */
	for (;;) {

		do {
			status = IORD_ALTERA_AVALON_SPI_STATUS(base);
		} while (((status & ALTERA_AVALON_SPI_STATUS_TRDY_MSK) == 0
				|| credits == 0)
				&& (status & ALTERA_AVALON_SPI_STATUS_RRDY_MSK) == 0);

		if ((status & ALTERA_AVALON_SPI_STATUS_TRDY_MSK) != 0 && credits > 0) {
			credits--;

			if (write_data < write_end)
				IOWR_ALTERA_AVALON_SPI_TXDATA(base, *write_data++);
			else if (write_zeros > 0) {
				write_zeros--;
				IOWR_ALTERA_AVALON_SPI_TXDATA(base, 0xFF);
			} else
				credits = -1024;
		};

		if ((status & ALTERA_AVALON_SPI_STATUS_RRDY_MSK) != 0) {
			alt_u32 rxdata = IORD_ALTERA_AVALON_SPI_RXDATA(base);

			if (read_ignore > 0)
				read_ignore--;
			else
				*read_data++ = (alt_u8) rxdata;
			credits++;

			if (read_ignore == 0 && read_data == read_end)
				break;
		}

	}

	/* Wait until the interface has finished transmitting */
	do {
		status = IORD_ALTERA_AVALON_SPI_STATUS(base);
	} while ((status & ALTERA_AVALON_SPI_STATUS_TMT_MSK) == 0);

	/* Clear SSO (release chipselect) unless the caller is going to
	 * keep using this chip
	 */
	if ((flags & ALT_AVALON_SPI_COMMAND_MERGE) == 0)
		IOWR_ALTERA_AVALON_SPI_CONTROL(base, 0);

	return read_length;
}

void nios_spi_rw1(uint32_t base, uint32_t slave, uint32_t w_length,
		const uint32_t * write_data, uint32_t r_length, uint32_t * read_data,
		uint32_t flags) {

	/* Warning: this function is not currently safe if called in a multi-threaded
	 * environment, something above must perform locking to make it safe if more
	 * than one thread intends to use it.
	 */

	IOWR_ALTERA_AVALON_SPI_SLAVE_SEL(base, 1 << slave);

	/* Set the SSO bit (force chipselect) only if the toggle flag is not set */
	if ((flags & ALT_AVALON_SPI_COMMAND_TOGGLE_SS_N) == 0) {
		IOWR_ALTERA_AVALON_SPI_CONTROL(base, ALTERA_AVALON_SPI_CONTROL_SSO_MSK);
	}

	/*
	 * Discard any stale data present in the RXDATA register, in case
	 * previous communication was interrupted and stale data was left
	 * behind.
	 */
	IORD_ALTERA_AVALON_SPI_RXDATA(base);

	/* Keep clocking until all the data has been processed. */

	for (uint32_t i = 0; i < w_length; i++) {
		// Wait TX ready
		while ((IORD_ALTERA_AVALON_SPI_STATUS(base)
				& ALTERA_AVALON_SPI_STATUS_TRDY_MSK) == 0) {
		}
		IOWR_ALTERA_AVALON_SPI_TXDATA(base, write_data[i]);
	}

	while ((IORD_ALTERA_AVALON_SPI_STATUS(base)
			& ALTERA_AVALON_SPI_STATUS_TMT_MSK) == 0) {
	}

	uint32_t ret, ret2;
	if (r_length > 0) {
		IOWR_ALTERA_AVALON_SPI_TXDATA(base, 0xFFFFFFFF);
	}
	for (uint32_t i = 0; i < r_length; i++) {

		while ((IORD_ALTERA_AVALON_SPI_STATUS(base)
				& ALTERA_AVALON_SPI_STATUS_TMT_MSK) == 0) {
		}
		ret = IORD_ALTERA_AVALON_SPI_RXDATA(base);
		if (i + 1 < r_length) {
			IOWR_ALTERA_AVALON_SPI_TXDATA(base, 0xFFFFFFFF);
		}
		ret2 = 0;
		for (uint8_t j = 0; j < 4; j++) {
			ret2 = ret2 << 8;
			ret2 |= ret & 0xFF;
			ret = ret >> 8;
		}
		read_data[i] = ret2;
	}

	/* Clear SSO (release chipselect) unless the caller is going to
	 * keep using this chip
	 */
	if ((flags & ALT_AVALON_SPI_COMMAND_MERGE) == 0)
		IOWR_ALTERA_AVALON_SPI_CONTROL(base, 0);

}

uint8_t nios_spi_r1(uint32_t base, uint32_t slave, uint32_t r_length,
		uint8_t * read_data, uint32_t * data_end, uint8_t * data_pos,
		uint32_t flags) {

	/* Warning: this function is not currently safe if called in a multi-threaded
	 * environment, something above must perform locking to make it safe if more
	 * than one thread intends to use it.
	 */
	uint16_t wpos;
	uint32_t ret, ret2;
	uint32_t i;
	IOWR_ALTERA_AVALON_SPI_SLAVE_SEL(base, 1 << slave);

	/* Set the SSO bit (force chipselect) only if the toggle flag is not set */
	if ((flags & ALT_AVALON_SPI_COMMAND_TOGGLE_SS_N) == 0) {
		IOWR_ALTERA_AVALON_SPI_CONTROL(base, ALTERA_AVALON_SPI_CONTROL_SSO_MSK);
	}

	IORD_ALTERA_AVALON_SPI_RXDATA(base);
	while ((IORD_ALTERA_AVALON_SPI_STATUS(base)
			& ALTERA_AVALON_SPI_STATUS_TMT_MSK) == 0) {
	}

	if (r_length > 0) {
		IOWR_ALTERA_AVALON_SPI_TXDATA(base, 0xFFFFFFFF);
	}
	wpos = 0;
	ret2 = 0;
	for (i = 0; i < r_length; i += 4) {

		while ((IORD_ALTERA_AVALON_SPI_STATUS(base)
				& ALTERA_AVALON_SPI_STATUS_TMT_MSK) == 0) {
		}
		ret = IORD_ALTERA_AVALON_SPI_RXDATA(base);
		if (i + 4 < r_length) {
			IOWR_ALTERA_AVALON_SPI_TXDATA(base, 0xFFFFFFFF);
		}
		ret2 = ret;
		for (uint8_t j = 0; j < 4; j++) {
			read_data[wpos] = ((ret2 & 0xFF000000) >> 24);
			wpos++;
			if (wpos >= r_length) {
				break;
			}
			ret2 = ret2 << 8;
		}

	}
	for (uint8_t j = 0; j < 4; j++) {
		ret2 = ret2 << 8;
		ret2 |= ret & 0xFF;
		ret = ret >> 8;
	}
	*data_end = ret2;
	*data_pos = i - wpos;

	/* Clear SSO (release chipselect) unless the caller is going to
	 * keep using this chip
	 */
	if ((flags & ALT_AVALON_SPI_COMMAND_MERGE) == 0) {
		IOWR_ALTERA_AVALON_SPI_CONTROL(base, 0);
	}
	return 0;
}
#endif
typedef union {
	uint8_t  s[4];
	uint16_t m[2];
	uint32_t l;
}  uint32_u;

int nios_spi32_wr(const uint32_t base, const uint32_t slave, const uint32_t write_length,	const uint8_t * write_data, const uint32_t read_length, uint8_t * read_data, const uint32_t flags) {

	uint32_u tx_data;

	static uint32_u rx_data;
	static uint32_t wp;

	if ((flags & ALT_AVALON_SPI_COMMAND_TOGGLE_SS_N) == 0) {
		IOWR_ALTERA_AVALON_SPI_CONTROL(base, ALTERA_AVALON_SPI_CONTROL_SSO_MSK);
	}
/*
	IORD_ALTERA_AVALON_SPI_RXDATA(base);
	while ((IORD_ALTERA_AVALON_SPI_STATUS(base) & ALTERA_AVALON_SPI_STATUS_TMT_MSK) == 0) {} // wait ready
*/
	if(write_length > 0){
		wp = 0;
		while(wp < write_length ){
			register uint8_t i = 4;
			do{
				tx_data.s[--i] = (wp < write_length) ? write_data[wp] : 0xFF;
				wp++;
			} while(i>0);

			while ((IORD_ALTERA_AVALON_SPI_STATUS(base) & ALTERA_AVALON_SPI_STATUS_TRDY_MSK) == 0) {} // wait ready transmited
			IOWR_ALTERA_AVALON_SPI_TXDATA(base, tx_data.l);
		}
		wp -= write_length;
		if(wp > 0){
			while ((IORD_ALTERA_AVALON_SPI_STATUS(base) & ALTERA_AVALON_SPI_STATUS_TMT_MSK) == 0) {} // wait ready receive
			rx_data.l = IORD_ALTERA_AVALON_SPI_RXDATA(base);
			//printf("SPI: doread data %04X\r\n", rx_data.l);
		}
	}

	if(read_length > 0){
		register uint32_t rd = 0;
		while(rd < read_length){
			if(rd + wp < read_length){
				while ((IORD_ALTERA_AVALON_SPI_STATUS(base) & ALTERA_AVALON_SPI_STATUS_TRDY_MSK) == 0) {} // wait ready receive
				IOWR_ALTERA_AVALON_SPI_TXDATA(base, 0xFFFFFFFF);
				//printf("SPI: Preget new data\r\n");
			}

			while((wp > 0) && (rd < read_length)){
				//printf("SPI: Read data\r\n");
				wp--;
				read_data[rd++] = rx_data.s[wp];
			}

			if(rd < read_length){
				while ((IORD_ALTERA_AVALON_SPI_STATUS(base) & ALTERA_AVALON_SPI_STATUS_TMT_MSK) == 0) {} // wait ready receive
				rx_data.l = IORD_ALTERA_AVALON_SPI_RXDATA(base);
				wp = 4;
				//printf("SPI: Wait data %04X\r\n", rx_data.l);
			}
		}
	}

	if ((flags & ALT_AVALON_SPI_COMMAND_MERGE) == 0) {
		IOWR_ALTERA_AVALON_SPI_CONTROL(base, 0);
	}

	//printf("SPI WP[%d]: %d\r\n", write_length, wp);
	return 0;
}

void nios_spi_cs_off(uint32_t base){
	IOWR_ALTERA_AVALON_SPI_CONTROL(base, 0);
}

