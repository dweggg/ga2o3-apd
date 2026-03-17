#include "bsp_sci.h"
#include "stdint.h"
#include "device.h"
#include "bsp_hal.h"
#include "serialDriver.h"

#pragma CODE_SECTION(SciaTxFIFOIsr, ".TI.ramfunc");
#pragma CODE_SECTION(SciaRxFIFOIsr, ".TI.ramfunc");

__interrupt void SciaTxFIFOIsr(void)
{
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_TXFF);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
}

__interrupt void SciaRxFIFOIsr(void)
{
    char rxChar = SCI_readCharBlockingFIFO(SCIA_BASE);
    SCI_clearOverflowStatus(SCIA_BASE);
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_RXFF);

    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
    serialDriverReceiveByte(rxChar);
}


HAL_StatusTypeDef bspInitSCI(void)
{
    //
    // GPIO28 is the SCI Rx pin.
    //
    GPIO_setMasterCore(43, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_43_SCIRXDA);
    GPIO_setDirectionMode(43, GPIO_DIR_MODE_IN);
    GPIO_setPadConfig(43, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(43, GPIO_QUAL_ASYNC);

    //
    // GPIO29 is the SCI Tx pin.
    //
    GPIO_setMasterCore(42, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_42_SCITXDA);
    GPIO_setDirectionMode(42, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(42, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(42, GPIO_QUAL_ASYNC);

    Interrupt_register(INT_SCIA_RX, SciaRxFIFOIsr);
    Interrupt_register(INT_SCIA_TX, SciaTxFIFOIsr);
    //
    // 8 char bits, 1 stop bit, no parity. Baud rate 115200.
    //
    SCI_setConfig(SCIA_BASE, DEVICE_LSPCLK_FREQ, 115200, (SCI_CONFIG_WLEN_8 |
                                                        SCI_CONFIG_STOP_ONE |
                                                        SCI_CONFIG_PAR_NONE));

    //F28x_usDelay(1000000);
    SCI_enableModule(SCIA_BASE);
    SCI_disableLoopback(SCIA_BASE);
    SCI_resetChannels(SCIA_BASE);
    SCI_enableFIFO(SCIA_BASE);

    //
    // RX and TX FIFO Interrupts Enabled
    //
    SCI_enableInterrupt(SCIA_BASE, (SCI_INT_RXFF | SCI_INT_TXFF));
    SCI_disableInterrupt(SCIA_BASE, SCI_INT_RXERR);

    SCI_setFIFOInterruptLevel(SCIA_BASE, SCI_FIFO_TX1, SCI_FIFO_RX1);
    SCI_performSoftwareReset(SCIA_BASE);

    SCI_resetTxFIFO(SCIA_BASE);
    SCI_resetRxFIFO(SCIA_BASE);
    
    Interrupt_enable(INT_SCIA_RX);
    //Interrupt_enable(INT_SCIA_TX);

    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);

    return HAL_OK;
}
