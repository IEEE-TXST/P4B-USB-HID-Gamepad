/*
 * P4-B pin mux: UART0 console (from the mouse example this project is
 * based on, and from P0), plus the accelerometer's I2C0 pins, SW1, and
 * the TSI touch slider electrode, all reused verbatim from P1. No USB
 * pins here: the USB D+/D- lines are dedicated, not part of the general
 * PORT mux system, confirmed by this same absence in NXP's own mouse
 * example's pin_mux.c.
 */
#include "fsl_common.h"
#include "fsl_port.h"
#include "pin_mux.h"

#define PIN1_IDX 1u
#define PIN2_IDX 2u
#define PIN3_IDX 3u
#define PIN16_IDX 16u
#define PIN17_IDX 17u
#define PIN24_IDX 24u
#define PIN25_IDX 25u

#define SOPT5_UART0RXSRC_UART_RX 0x00u
#define SOPT5_UART0TXSRC_UART_TX 0x00u

void BOARD_InitPins(void)
{
    CLOCK_EnableClock(kCLOCK_PortA);
    CLOCK_EnableClock(kCLOCK_PortB);
    CLOCK_EnableClock(kCLOCK_PortC);
    CLOCK_EnableClock(kCLOCK_PortE);

    /* UART0 debug console. */
    PORT_SetPinMux(PORTA, PIN1_IDX, kPORT_MuxAlt2); /* PTA1 = UART0_RX */
    PORT_SetPinMux(PORTA, PIN2_IDX, kPORT_MuxAlt2); /* PTA2 = UART0_TX */
    SIM->SOPT5 = ((SIM->SOPT5 & (~(SIM_SOPT5_UART0TXSRC_MASK | SIM_SOPT5_UART0RXSRC_MASK))) |
                  SIM_SOPT5_UART0TXSRC(SOPT5_UART0TXSRC_UART_TX) | SIM_SOPT5_UART0RXSRC(SOPT5_UART0RXSRC_UART_RX));

    /* SW1 push button, internal pull-up, same as P1/P2. */
    const port_pin_config_t sw1_config = {
        kPORT_PullUp,
        kPORT_FastSlewRate,
        kPORT_PassiveFilterDisable,
        kPORT_LowDriveStrength,
        kPORT_MuxAsGpio,
    };
    PORT_SetPinConfig(PORTC, PIN3_IDX, &sw1_config); /* PTC3 = SW1 */

    /* TSI0 touch slider electrode. */
    PORT_SetPinMux(PORTB, PIN16_IDX, kPORT_PinDisabledOrAnalog); /* PTB16 = TSI0_CH9 */
    PORT_SetPinMux(PORTB, PIN17_IDX, kPORT_PinDisabledOrAnalog); /* PTB17 = TSI0_CH10 */

    /* I2C0 to the on-board FXOS8700CQ accelerometer. */
    PORT_SetPinMux(PORTE, PIN24_IDX, kPORT_MuxAlt5); /* PTE24 = I2C0_SCL */
    PORT_SetPinMux(PORTE, PIN25_IDX, kPORT_MuxAlt5); /* PTE25 = I2C0_SDA */
}
