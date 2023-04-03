/* sd_spi.simple.c
Copyright 2021 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use
this file except in compliance with the License. You may obtain a copy of the
License at

   http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.
*/
/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
//
/* FreeRTOS includes. */
#include "FreeRTOS.h"
//#include "FreeRTOSFATConfig.h" // for DBG_PRINTF
//
#include "sd_card.h"
#include "sd_spi.h"
#include "spi.h"
//

//#define TRACE_PRINTF(fmt, args...)
#define TRACE_PRINTF task_printf

void sd_spi_go_high_frequency(sd_card_t *pSD) {
    uint actual = spi_set_baudrate(pSD->spi->hw_inst, 6 * 1000 * 1000);
    TRACE_PRINTF("%s: Actual frequency: %lu\n", __FUNCTION__, actual);
}
void sd_spi_go_low_frequency(sd_card_t *pSD) {
    uint actual = spi_set_baudrate(pSD->spi->hw_inst, 100 * 1000);
    TRACE_PRINTF("%s: Actual frequency: %lu\n", __FUNCTION__, actual);
}

static void sd_spi_lock(sd_card_t *pSD) {
    configASSERT(pSD->spi->mutex);
    xSemaphoreTakeRecursive(pSD->spi->mutex, portMAX_DELAY);
    pSD->spi->owner = xTaskGetCurrentTaskHandle();
}
static void sd_spi_unlock(sd_card_t *pSD) {
    pSD->spi->owner = 0;
    xSemaphoreGiveRecursive(pSD->spi->mutex);
}

static void sd_spi_select(sd_card_t *pSD) {
    asm volatile("nop \n nop \n nop");  // FIXME
    gpio_put(pSD->ss_gpio, 0);
    asm volatile("nop \n nop \n nop");  // FIXME
}

static void sd_spi_deselect(sd_card_t *pSD) {
    asm volatile("nop \n nop \n nop");  // FIXME
    gpio_put(pSD->ss_gpio, 1);
    asm volatile("nop \n nop \n nop");  // FIXME
    /*
    MMC/SDC enables/disables the DO output in synchronising to the SCLK. This
    means there is a posibility of bus conflict with MMC/SDC and another SPI
    slave that shares an SPI bus. Therefore to make MMC/SDC release the MISO
    line, the master device needs to send a byte after the CS signal is
    deasserted.
    */
    uint8_t fill = SPI_FILL_CHAR;
    spi_write_blocking(pSD->spi->hw_inst, &fill, 1);
}

void sd_spi_acquire(sd_card_t *pSD) {
    sd_spi_lock(pSD);
    sd_spi_select(pSD);
}

void sd_spi_release(sd_card_t *pSD) {
    sd_spi_deselect(pSD);
    sd_spi_unlock(pSD);
}

// SPI Transfer: Read & Write (simultaneously) on SPI bus
//   If the data that will be received is not important, pass NULL as rx.
//   If the data that will be transmitted is not important,
//     pass NULL as tx and then the SPI_FILL_CHAR is sent out as each data
//     element.
bool sd_spi_transfer(sd_card_t *pSD, const uint8_t *tx, uint8_t *rx,
                     size_t length) {
    //  TRACE_PRINTF("%s\n", __FUNCTION__);
    BaseType_t rc;

    configASSERT(512 == length);
    configASSERT(tx || rx);

    int num = 0;
    if (tx && rx) {
        num = spi_write_read_blocking(pSD->spi->hw_inst, tx, rx, length);
    } else if (tx) {
        num = spi_write_blocking(pSD->spi->hw_inst, tx, length);
    } else if (rx) {
        num = spi_read_blocking(pSD->spi->hw_inst, SPI_FILL_CHAR, rx, length);
    }
    configASSERT(num == length);

    return true;
}

uint8_t sd_spi_write(sd_card_t *pSD, const uint8_t value) {
    // TRACE_PRINTF("%s\n", __FUNCTION__);
    u_int8_t received = SPI_FILL_CHAR;

    int num = spi_write_read_blocking(pSD->spi->hw_inst, &value, &received, 1);
    configASSERT(1 == num);

    return received;
}

/* [] END OF FILE */
