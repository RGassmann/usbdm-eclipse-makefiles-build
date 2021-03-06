/**
 * @file     dac.h (180.ARM_Peripherals/Project_Headers/dac.h)
 *
 * @brief    Abstraction layer for DAC interface
 *
 * @version  V4.12.1.80
 * @date     13 April 2016
 */

#ifndef PROJECT_HEADERS_DAC_H_
#define PROJECT_HEADERS_DAC_H_
 /*
 * *****************************
 * *** DO NOT EDIT THIS FILE ***
 * *****************************
 *
 * This file is generated automatically.
 * Any manual changes will be lost.
 */
#include "derivative.h"
#include "hardware.h"

namespace USBDM {

/**
 * @addtogroup DAC_Group DAC, Digital-to-Analogue Converter
 * @brief Pins used for Digital-to-Analogue Converter
 * @{
 */
/**
 * Type definition for DAC interrupt call back
 */
typedef void (*DACCallbackFunction)(void);

/**
 * @brief Class representing a Digital to Analogue Converter
 *
 * <b>Example</b>
 * @code
 * using dac = Dac_T<Dac0Info>;
 *
 *  vref::configure();
 *
 * @endcode
 */
template<class Info>
class Dac_T {
protected:

   /** Callback functions for ISRs */
   static DACCallbackFunction callback[Info::irqCount];

public:
   /** DAC interrupt handler -  Calls DAC0 callback */
   static void irqHandler() {
      // Clear interrupt flag
      if (Dac_T::callback[0] != 0) {
         Dac_T::callback[0]();
      }
   }

public:
   /**
    * Set callback for ISR
    *
    * @param channel  The DAC channel to modify
    * @param callback The function to call from stub ISR
    */
   static void setCallback(int channel, DACCallbackFunction callback) {
      Dac_T::callback[channel] = callback;
   }

protected:
   /** Hardware instance pointer */
   static __attribute__((always_inline)) volatile DAC_Type &dac() { return Info::dac(); }

   /** Clock register for peripheral */
   static __attribute__((always_inline)) volatile uint32_t &clockReg() { return Info::clockReg(); }

public:
   /**
    *  Configure the DAC with default settings
    *
    *  @param c0       Module Control Register 0
    *  @param c1       Module Control Register 1
    *  @param c2       Module Control Register 2
    */
   static void enable(uint32_t c0=Info::c0, uint32_t c1=Info::c1, uint32_t c2=Info::c2) {
      // Enable clock
      clockReg() |= Info::clockMask;
      __DMB();

      Info::initPCRs();

      // Enable timer
      dac().C0 = c0|DAC_C0_DACEN_MASK;
      dac().C1 = c1;
      dac().C2 = c2;
   }
   /**
    *   Disable the DAC channel
    */
   static void finalise(uint8_t channel) {
      // Enable timer
      dac().C0 = 0;
      dac().C1 = 0;
      clockReg() &= ~Info::clockMask;
   }
   /**
    * Set DAC output value
    *
    * @param value 12-bit value to write to DAC
    */
   static void setValue(uint16_t value) {
      dac().DATA[0] = DAC_DATA_DATA(value);
   }
};

/**
 * Callback table for programmatically set handlers
 */
template<class Info> DACCallbackFunction Dac_T<Info>::callback[] = {0};

#if defined(USBDM_DAC0_IS_DEFINED)
using Dac0 = Dac_T<Dac0Info>;
#endif

#if defined(USBDM_DAC1_IS_DEFINED)
using Dac1 = Dac_T<Dac1Info>;
#endif
/**
 * @}
 */
} // End namespace USBDM

#endif /* PROJECT_HEADERS_DAC_H_ */
