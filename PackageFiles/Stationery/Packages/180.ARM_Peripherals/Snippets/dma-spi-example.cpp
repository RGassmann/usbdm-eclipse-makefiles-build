/**
 ============================================================================
 * @file    dma-spi-example.cpp (180.ARM_Peripherals/Snippets)
 * @brief   DMA example using SPI
 *
 *  Created on: 10/1/2016
 *      Author: podonoghue
 ============================================================================
 */
/**
 * This example uses DMA to transfer data from a buffer to the SPI for transmission.
 * At the same time another DMA channel is used to transfer receive data from the SPI to a buffer.
 * The transmission is not continuous but may be restarted without setting up the TCD again:
 * - Clears the DREQ on transfer complete
 * - Arranging SLAST/DLAST to return the transfer addresses to starting value after each major-loop.
 *
 * This example requires DMA and SPI interrupts.
 *
 * It is necessary to enable these in Configure.usbdmProject
 * under the "Peripheral Parameters"->DMA0 and "Peripheral Parameters"->SPI tabs
 * Select irqHandlingMethod option (Class Method - Software ...)
 */
#include <string.h>
#include "hardware.h"
#include "dma.h"
#include "spi.h"

using namespace USBDM;

// Connection - change as required
using Led = GpioC<3, ActiveLow>;  // = PTA2 = D9 = Blue LED

// SPI to use
Spi0 spi;

// Which SPI PCS signal to assert
static constexpr SpiPeripheralSelect spiSelect = SpiPeripheralSelect_2;

// Used to indicate complete transfer
static volatile bool complete;

/**
 * DMA complete callback
 *
 * Sets flag to indicate sequence complete.
 */
static void dmaCallback(DmaChannelNum channel) {
   Dma0::clearInterruptRequest(channel);
   complete = true;
}

/**
 * Base value for transmit data
 */
static constexpr uint32_t PUSH_BASE =
      SPI_PUSHR_CTAS(0)|   // CTAR 0
      SPI_PUSHR_EOQ(0)|    // Not end of Queue
      SPI_PUSHR_CTCNT(0)|  // Don't clear transfer counter
      spiSelect;           // Peripheral to select

/**
 * Transmit Data + Control information.
 * Continuously asserts peripheral select during transfer
 */
const uint32_t txBuffer[]= {
      PUSH_BASE|SPI_PUSHR_CONT(1)|SPI_PUSHR_TXDATA(1),
      PUSH_BASE|SPI_PUSHR_CONT(1)|SPI_PUSHR_TXDATA(2),
      PUSH_BASE|SPI_PUSHR_CONT(1)|SPI_PUSHR_TXDATA(3),
      PUSH_BASE|SPI_PUSHR_CONT(1)|SPI_PUSHR_TXDATA(4),
      PUSH_BASE|SPI_PUSHR_CONT(1)|SPI_PUSHR_TXDATA(5),
      PUSH_BASE|SPI_PUSHR_CONT(1)|SPI_PUSHR_TXDATA(6),
      PUSH_BASE|SPI_PUSHR_CONT(1)|SPI_PUSHR_TXDATA(7),
      PUSH_BASE|SPI_PUSHR_CONT(1)|SPI_PUSHR_TXDATA(8),
      PUSH_BASE|SPI_PUSHR_CONT(1)|SPI_PUSHR_TXDATA(9),
      PUSH_BASE|SPI_PUSHR_CONT(1)|SPI_PUSHR_TXDATA(10),
      PUSH_BASE|SPI_PUSHR_CONT(1)|SPI_PUSHR_TXDATA(11),
      PUSH_BASE|SPI_PUSHR_CONT(1)|SPI_PUSHR_TXDATA(12),
      PUSH_BASE|SPI_PUSHR_CONT(1)|SPI_PUSHR_TXDATA(13),
      PUSH_BASE|SPI_PUSHR_CONT(0)|SPI_PUSHR_TXDATA(14)|SPI_PUSHR_EOQ(1),
};

static uint8_t rxBuffer[sizeof(txBuffer)/sizeof(txBuffer[0])];
static const uint8_t rxTestBuffer[sizeof(txBuffer)/sizeof(txBuffer[0])] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14};

/**
 * Configure DMA from Memory-to-SPI-to-Memory
 */
static void initDma(DmaChannelNum dmaTransmitChannel, DmaChannelNum dmaReceiveChannel) {

   /**
    * @verbatim
    * +------------------------------+            Simple DMA mode (MLNO = Minor Loop Mapping Disabled)
    * | Major Loop =                 |            ==================================================
    * |    CITER x Minor Loop        |
    * |                              |            Each DMA request triggers a minor-loop transfer sequence.
    * | +--------------------------+ |<-DMA Req.  The minor loops are counted in the major-loop.
    * | | Minor Loop               | |
    * | | Each transfer            | |            The following are used during a minor loop:
    * | |   SADDR->DADDR           | |             - SADDR Source address
    * | |   SADDR += SOFF          | |             - SOFF  Adjustment applied to SADDR after each transfer
    * | |   DADDR += DOFF          | |             - DADDR Destination address
    * | | Total transfer is NBYTES | |             - DOFF  Adjustment applied to DADDR after each transfer
    * | +--------------------------+ |             - NBYTES Number of bytes to transfer
    * | +--------------------------+ |<-DMA Req.   - Attributes
    * | | Minor Loop               | |               - ATTR_SSIZE, ATTR_DSIZE Source and destination transfer sizes
    * |..............................|               - ATTR_SMOD, ATTR_DMOD Modulo
    * | |                          | |
    * | +--------------------------+ |             The number of reads and writes done will depend on NBYTES, SSIZE and DSIZE
    * | +--------------------------+ |<-DMA Req.   For example: NBYTES=12, SSIZE=16-bits, DSIZE=32-bits => 6 reads, 3 writes
    * | | Minor Loop               | |             NBYTES must be an even multiple of SSIZE and DSIZE in bytes.
    * | | Each transfer            | |
    * | |   SADDR->DADDR           | |            The following are used by the major loop
    * | |   SADDR += SOFF          | |             - SLAST Adjustment applied to SADDR after major loop
    * | |   DADDR += DOFF          | |             - DLAST Adjustment applied to DADDR after major loop
    * | | Total transfer is NBYTES | |             - CITER Major loop counter - counts how many minor loops
    * | +--------------------------+ |
    * |                              |            SLAST and DLAST may be used to reset the addresses to the initial value or
    * | At end of Major Loop         |            link to the next transfer.
    * |    SADDR += SLAST            |            The total transferred for the entire sequence is CITER x NBYTES.
    * |    DADDR += DLAST            |
    * |                              |            Important options in the CSR:
    * | Total transfer =             |              - DMA_CSR_INTMAJOR = Generate interrupt at end of Major-loop
    * |    CITER*NBYTES              |              - DMA_CSR_DREQ     = Clear hardware request at end of Major-loop
    * +------------------------------+              - DMA_CSR_START    = Start transfer. Used for software transfers. Automatically cleared.
    * @endverbatim
    */
   /**
    * Structure to define the Transmit DMA transfer
    *
    * Note: This uses a 32-bit transfer even though the transmit data is only 8-bit
    */
   static constexpr DmaTcd txTcd {
      /* uint32_t  SADDR        Source address              */ (uint32_t)(txBuffer),                     // Source array
      /* uint16_t  SOFF         SADDR offset                */ sizeof(txBuffer[0]),                      // SADDR advances 1 element for each request
      /* DmaSize   DSIZE        Destination size            */ DmaSize_32bit,                            // 32-bit write to DADDR (PUSHR)
      /* DmaModulo DMOD         Destination modulo          */ DmaModulo_Disabled,                       // No modulo
      /* DmaSize   SSIZE        Source size                 */ dmaSize(txBuffer[0]),                     // 32-bit read from SADDR (buffer)
      /* DmaModulo SMOD         Source modulo               */ DmaModulo_Disabled,                       // No modulo
      /* uint32_t  NBYTES       Minor loop byte count       */ sizeof(txBuffer[0]),                      // Total transfer in one minor-loop
      /* uint32_t  SLAST        Last SADDR adjustment       */ -sizeof(txBuffer),                        // Reset SADDR to start of array on completion
      /* uint32_t  DADDR        Destination address         */ spi.spiPUSHR(),                           // Destination is SPI PUSH data register
      /* uint16_t  DOFF         DADDR offset                */ 0,                                        // DADDR doesn't change
      /* uint16_t  CITER        Major loop count            */ DMA_CITER_ELINKNO_ELINK(0)|               // No ELINK
      /*                                                    */ ((sizeof(txBuffer))/sizeof(txBuffer[0])), // Transfer entire txBuffer
      /* uint32_t  DLAST        Last DADDR adjustment       */ 0,                                        // DADDR doesn't change
      /* bool      START;       Channel Start               */ false,                                    // Don't start (triggered by hardware)
      /* bool      INTMAJOR;    Interrupt on major complete */ false,                                    // No interrupt
      /* bool      INTHALF;     Interrupt on half complete  */ false,                                    // No interrupt
      /* bool      DREQ;        Disable Request             */ true,                                     // Clear hardware request when complete major loop
      /* bool      ESG;         Enable Scatter/Gather       */ false,                                    // Disabled
      /* bool      MAJORELINK;  Enable channel linking      */ false,                                    // Disabled
      /* bool      ACTIVE;      Channel Active              */ false,
      /* bool      DONE;        Channel Done                */ false,
      /* unsigned  MAJORLINKCH; Link Channel Number         */ 0,                                        // N/A
      /* DmaSpeed  BWC;         Bandwidth (speed) Control   */ DmaSpeed_NoStalls,                        // Full speed
   };

   /**
    * Structure to define the Receive DMA transfer
    *
    * Note: The transfer size used here is 8-bits only
    */
   static constexpr DmaTcd rxTcd {
      /* uint32_t  SADDR        Source address              */ spi.spiPOPR(),                            // Source is SPI POPR data register
      /* uint16_t  SOFF         SADDR offset                */ 0,                                        // SADDR adoesn't change
      /* DmaSize   DSIZE        Destination size            */ dmaSize(rxBuffer[0]),                     // 8-bit write to DADDR (buffer)
      /* DmaModulo DMOD         Destination modulo          */ DmaModulo_Disabled,                       // No modulo
      /* DmaSize   SSIZE        Source size                 */ DmaSize_8bit,                             // 8-bit read from SADDR (=POPR)
      /* DmaModulo SMOD         Source modulo               */ DmaModulo_Disabled,                       // No modulo
      /* uint32_t  NBYTES       Minor loop byte count       */ sizeof(rxBuffer[0]),                      // Total transfer in one minor-loop
      /* uint32_t  SLAST        Last SADDR adjustment       */ 0,                                        // SADDR doesn't change
      /* uint32_t  DADDR        Destination address         */ (uint32_t)(rxBuffer),                     // Destination array
      /* uint16_t  DOFF         DADDR offset                */ sizeof(rxBuffer[0]),                      // DADDR advances 1 element for each request
      /* uint16_t  CITER        Major loop count            */ DMA_CITER_ELINKNO_ELINK(0)|               // No ELINK
      /*                                                    */ ((sizeof(rxBuffer))/sizeof(rxBuffer[0])), // Transfer entire txBuffer
      /* uint32_t  DLAST        Last DADDR adjustment       */ -sizeof(rxBuffer),                        // Reset DADDR to start of array on completion
      /* bool      START;       Channel Start               */ false,                                    // Don't start (triggered by hardware)
      /* bool      INTMAJOR;    Interrupt on major complete */ true,                                     // Generate interrupt on completion of Major-loop
      /* bool      INTHALF;     Interrupt on half complete  */ false,                                    // No interrupt
      /* bool      DREQ;        Disable Request             */ true,                                     // Clear hardware request when complete major loop
      /* bool      ESG;         Enable Scatter/Gather       */ false,                                    // Disabled
      /* bool      MAJORELINK;  Enable channel linking      */ false,                                    // Disabled
      /* bool      ACTIVE;      Channel Active              */ false,
      /* bool      DONE;        Channel Done                */ false,
      /* unsigned  MAJORLINKCH; Link Channel Number         */ 0,                                        // N/A
      /* DmaSpeed  BWC;         Bandwidth (speed) Control   */ DmaSpeed_NoStalls,                        // Full speed
   };

   // Sequence not complete yet
   complete = false;

   // Enable DMAC with default settings
   Dma0::configure();

   // Set callback (Interrupts are enabled in TCD)
   Dma0::setCallback(dmaTransmitChannel, dmaCallback);
   Dma0::enableNvicInterrupts(dmaTransmitChannel);

   // Set callback (Interrupts are enabled in TCD)
   Dma0::setCallback(dmaReceiveChannel, dmaCallback);
   Dma0::enableNvicInterrupts(dmaReceiveChannel);

   // Connect DMA channel to SPI Tx
   DmaMux0::configure(dmaTransmitChannel, Dma0Slot_SPI0_Tx, DmaMuxEnable_Continuous);

   // Connect DMA channel to SPI Rx
   DmaMux0::configure(dmaReceiveChannel, Dma0Slot_SPI0_Rx, DmaMuxEnable_Continuous);

   // Configure the Tx transfer
   Dma0::configureTransfer(dmaTransmitChannel, txTcd);

   // Configure the Rx transfer
   Dma0::configureTransfer(dmaReceiveChannel, rxTcd);
}

/**
 * SPI callback
 *
 * Used for debug timing checks.
 * LED toggles on each SPI event
 *
 * @param status Interrupt status value from SPI->SR
 */
static void spiCallback(uint32_t status) {
   (void)status;
   Led::toggle();
}

/**
 * Configure SPI
 */
static void configureSpi() {
   spi.setSpeed(24000000);
   spi.setPeripheralSelect(spiSelect, ActiveLow, SpiSelectMode_Idle);
   spi.setCallback(spiCallback);
   spi.configureInterrupts(
         SpiTxCompleteInterrupt_Enabled,
         SpiEndOfQueueInterrupt_Enable,
         SpiFifoUnderflowInterrupt_Enabled,
         SpiFifoOverflowInterrupt_Enabled);
   spi.configureFifoRequests(SpiFifoTxRequest_Dma, SpiFifoRxRequest_Dma);
}

/**
 * Start transfer
 */
static void startTransfer(DmaChannelNum dmaTransmitChannel, DmaChannelNum dmaReceiveChannel) {
   complete = false;

   spi.getStatus();
   spi.enableTransfer();

   // Enable Rx hardware requests
   Dma0::enableRequests(dmaReceiveChannel);

   // Enable Tx hardware requests
   Dma0::enableRequests(dmaTransmitChannel);
}

int main() {
   console.writeln("Starting");
   console.write("spiPtr      = ").writeln((unsigned)&spi.spiPtr(), Radix_16);
   console.write("spiBase     = ").writeln(spi.spiBase(), Radix_16);
   console.write("spiCR       = ").writeln(spi.spiCR(), Radix_16);
   console.write("spiCTAR[1]  = ").writeln(spi.spiCTAR(1), Radix_16);
   console.write("spiMCR      = ").writeln(spi.spiMCR(), Radix_16);
   console.write("spiPOPR     = ").writeln(spi.spiPOPR(), Radix_16);
   console.write("spiPUSHR    = ").writeln(spi.spiPUSHR(), Radix_16);
   console.write("spiSR       = ").writeln(spi.spiSR(), Radix_16);

   Led::setOutput();

   // DMA channel number to use for transmission
   static const DmaChannelNum dmaTransmitChannel = Dma0::allocatePeriodicChannel();
   if (dmaTransmitChannel == DmaChannelNum_None) {
      console.writeln("Failed to allocate Transmit DMA channel");
      checkError();
   }
   console.write("Allocated Transmit DMA channel  #").writeln(dmaTransmitChannel);

   // DMA channel number to use for reception
   static const DmaChannelNum dmaReceiveChannel = Dma0::allocatePeriodicChannel();
   if (dmaReceiveChannel == DmaChannelNum_None) {
      console.writeln("Failed to allocate Receive DMA channel");
      checkError();
   }
   console.write("Allocated Receive DMA channel  #").writeln(dmaReceiveChannel);

   // Set up DMA transfer from memory -> SPI -> memory
   initDma(dmaTransmitChannel, dmaReceiveChannel);
   configureSpi();

   unsigned failureCount = 0;
   for(;;) {
      // Clear Rx buffer
      memset(rxBuffer, 0, sizeof(rxBuffer));

      console.writeln("Starting Transfer");

      // Start transfer
      startTransfer(dmaTransmitChannel, dmaReceiveChannel);

      // Wait for completion of 1 Major-loop = 1 txBuffer
      while (!complete) {
         __asm__("nop");
      }
      // Check expected Rx data
      if (memcmp(rxBuffer, rxTestBuffer, sizeof(rxBuffer)) != 0) {
         failureCount++;
         console.write("Failed   - fail count = ").writeln(failureCount);
      }
      else {
         console.write("Verified - fail count =").writeln(failureCount);
      }
   }
   return 0;
}
