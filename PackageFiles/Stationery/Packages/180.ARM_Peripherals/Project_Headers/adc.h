/**
 * @file     adc.h
 * @brief    ADC routines
 *
 * @version  V4.12.1.80
 * @date     13 April 2016
 */
#ifndef HEADER_ADC_H
#define HEADER_ADC_H

#include <stddef.h>
#include <assert.h>
#include "derivative.h"

/*
 * Default port information
 */
#ifndef FIXED_PORT_CLOCK_REG
#define FIXED_PORT_CLOCK_REG SCGC5
#endif

#ifndef FIXED_ADC_FN
#define FIXED_ADC_FN    (0)
#endif

#ifndef ADC0_CLOCK_MASK
#ifdef SIM_SCGC6_ADC0_MASK
#define ADC0_CLOCK_MASK SIM_SCGC6_ADC0_MASK
#define ADC0_CLOCK_REG  SCGC6
#endif
#ifdef SIM_SCGC3_ADC1_MASK
#define ADC1_CLOCK_MASK SIM_SCGC3_ADC1_MASK
#define ADC1_CLOCK_REG  SCGC3
#endif
#ifdef SIM_SCGC6_ADC1_MASK
#define ADC1_CLOCK_MASK SIM_SCGC6_ADC1_MASK
#define ADC1_CLOCK_REG  SCGC6
#endif
#ifdef SIM_SCGC6_ADC2_MASK
#define ADC2_CLOCK_MASK SIM_SCGC6_ADC2_MASK
#define ADC2_CLOCK_REG  SCGC6
#endif
#endif

namespace USBDM {

/**
 * @addtogroup AnalogueIO_Group Analogue Input
 * @{
 */

/**
 * PCR multiplexor value for analogue function
 */
static constexpr uint32_t    ADC_PORT_FN = PORT_PCR_MUX(FIXED_ADC_FN);
/**
 * Default PCR value for pins used as GPIO (including multiplexor value)
 */
static constexpr uint32_t    ADC_DEFAULT_PCR = ADC_PORT_FN;

/**
 * ADC Resolutions for use with AnalogueIO::setMode()
 */
enum Adc_Resolution {
   resolution_8bit_se    = ADC_CFG1_MODE(0),
   resolution_10bit_se   = ADC_CFG1_MODE(2),
   resolution_12bit_se   = ADC_CFG1_MODE(1),
   resolution_16bit_se   = ADC_CFG1_MODE(3),
   resolution_9bit_diff  = ADC_CFG1_MODE(0),
   resolution_11bit_diff = ADC_CFG1_MODE(2),
   resolution_12bit_diff = ADC_CFG1_MODE(1),
   resolution_16bit_diff = ADC_CFG1_MODE(3),
};

/**
 * Type definition for ADC interrupt call back
 *
 * @param value Conversion value from channel
 */
typedef void (*ADCCallbackFunction)(uint32_t value);

/**
 * Template class representing an ADC
 *
 * Example
 * @code
 *  // Instantiate ADC channel adc0_se8
 *  const AdcBase_T<adc0Info> Adc0;
 *
 *  // Initialise ADC
 *  Adc0.setMode(resolution_16bit_se);
 *  @endcode
 *
 * @tparam info Table of information describing ADC
 */
template<class Info>
class AdcBase_T {

protected:
   static constexpr volatile ADC_Type *adc      = reinterpret_cast<volatile ADC_Type *>(Info::basePtr);
   static constexpr volatile uint32_t *clockReg = reinterpret_cast<volatile uint32_t *>(Info::clockReg);

public:
   /**
    * Initialise ADC to default settings\n
    * Configures all ADC pins
    */
   static void enable() {
      // Configure pins
      Info::initPCRs();
      Info::InfoDP::initPCRs();
      Info::InfoDM::initPCRs();

      // Enable clock to ADC
      *clockReg  |= Info::clockMask;
      __DMB();

      // Set mode to default
      adc->CFG1 = Info::CFG1;
      adc->CFG2 = Info::CFG2;
      adc->SC2  = Info::SC2;

      if (Info::irqHandlerInstalled) {
         // Enable timer interrupts
         NVIC_EnableIRQ(Info::irqNums[0]);

         // Set priority level
         NVIC_SetPriority(Info::irqNums[0], Info::irqLevel);
      }
   }

   /**
    * Disables the ADC\n
    * Does not change ADC pin mapping
    */
   static void disable() {
      adc->CFG1 = 0;
      adc->CFG2 = 0;
      adc->SC2  = 0;
   }

   /**
    * Set conversion mode
    *
    * @param mode Mode for converter e.g resolution_16bit_se
    *
    * @note This affects all channels on the ADC
    */
   static void setMode(uint32_t mode = resolution_16bit_se) {
      adc->CFG1 = (Info::CFG1&~ADC_CFG1_MODE_MASK)|(mode&ADC_CFG1_MODE_MASK);
   }

   /**
    * Configure ADC
    */
   static void configure(uint32_t cfg1=Info::CFG1, uint32_t cfg2=Info::CFG2, uint32_t sc2=Info::SC2) {
      // Set mode
      adc->CFG1 = cfg1;
      adc->CFG2 = cfg2;
      adc->SC2  = sc2;
   }

   /**
    * Initiates a conversion but does not wait for it to complete\n
    * Intended for use with interrupts
    *
    * @param sc1Value SC1 register value including the ADC channel to use
    */
   static void startConversion(const int sc1Value) {
#ifdef DEBUG_BUILD
      static_assert(Info::irqHandlerInstalled, "Interrupt handlers must be enabled when using this function");
#endif

      // Trigger conversion with interrupts enabled
      adc->SC1[0] = ADC_SC1_AIEN_MASK|(sc1Value&(ADC_SC1_ADCH_MASK|ADC_SC1_AIEN_MASK|ADC_SC1_DIFF_MASK));
   };

   /**
    * Initiates a conversion and waits for it to complete
    *
    * @param sc1Value SC1 register value including the ADC channel to use
    *
    * @return - the result of the conversion
    */
   static int readAnalogue(const int sc1Value) {

      // Trigger conversion
      adc->SC1[0] = (sc1Value&(ADC_SC1_ADCH_MASK|ADC_SC1_AIEN_MASK|ADC_SC1_DIFF_MASK));

      while ((adc->SC1[0]&ADC_SC1_COCO_MASK) == 0) {
         __asm__("nop");
      }
      return (int)adc->R[0];
   };
};

/**
 * Template class to provide ADC callback
 */
template<class Info>
class AdcIrq_T : public AdcBase_T<Info> {

protected:
   /** Callback function for ISR */
   static ADCCallbackFunction callback;

public:
   /**
    * IRQ handler
    */
   static void irqHandler() {
      if (callback != 0) {
         callback(AdcBase_T<Info>::adc->R[0]);
      }
      else {
         // Dummy read to clear interrupt
         (void)(AdcBase_T<Info>::adc->R[0]);
      }
   }

   /**
    * Set Callback function
    *
    * @param theCallback Callback function to execute on interrupt
    */
   static void setCallback(ADCCallbackFunction theCallback) {
      callback = theCallback;
   }
};

template<class Info> ADCCallbackFunction AdcIrq_T<Info>::callback = 0;

#ifdef USBDM_ADC0_IS_DEFINED
/**
 *
 * Template class representing a ADC0 channel
 *
 * Example
 * @code
 * // Instantiate the ADC channel (for ADC0 channel 6)
 * using Adc0_ch6 = USBDM::Adc0Channel<6>;
 *
 * // Set ADC resolution
 * Adc0_ch6.setMode(resolution_16bit_se);
 *
 * // Read ADC value
 * uint32_t value = Adc0_ch6.readAnalogue();
 * @endcode
 *
 * @tparam channel ADC channel
 */
template<int channel>
class Adc0Channel : public AdcBase_T<Adc0Info>, CheckSignal<Adc0Info, channel> {

public:
   /**
    * Initiates a conversion but does not wait for it to complete\n
    * Intended for use with interrupts
    *
    * @param channel The ADC channel to use
    */
   static void startConversion() {
      AdcBase_T::startConversion(ADC_SC1_ADCH(channel)&~ADC_SC1_DIFF_MASK);
   };
   /**
    * Initiates a conversion and waits for it to complete
    *
    * @return - the result of the conversion
    */
   static int readAnalogue() {
      return AdcBase_T::readAnalogue(ADC_SC1_ADCH(channel)&~ADC_SC1_DIFF_MASK);
   };
};

/**
 *
 * Template class representing a ADC0 differential channel
 *
 * Example
 * @code
 * // Instantiate the ADC channel (for ADC0 channel 6)
 * using Adc0_ch6 = USBDM::Adc0Channel<6>;
 *
 * // Set ADC resolution
 * Adc0_ch6.setMode(resolution_16bit_se);
 *
 * // Read ADC value
 * uint32_t value = Adc0_ch6.readAnalogue();
 * @endcode
 *
 * @tparam channel ADC channel
 */
template<int channel>
class Adc0DiffChannel : public AdcBase_T<Adc0Info>, CheckSignal<Adc0Info::InfoDP, channel>, CheckSignal<Adc0Info::InfoDM, channel> {

public:
   /**
    * Initiates a conversion but does not wait for it to complete\n
    * Intended for use with interrupts
    *
    * @param channel The ADC channel to use
    */
   static void startConversion() {
      AdcBase_T::startConversion(ADC_SC1_ADCH(channel)|ADC_SC1_DIFF_MASK);
   };
   /**
    * Initiates a conversion and waits for it to complete
    *
    * @return - the result of the conversion
    */
   static int readAnalogue() {
      return AdcBase_T::readAnalogue(ADC_SC1_ADCH(channel)|ADC_SC1_DIFF_MASK);
   };
};
/**
 * Class representing ADC0
 */
using Adc0 = AdcIrq_T<Adc0Info>;
#endif

#ifdef USBDM_ADC1_IS_DEFINED
/**
 *
 * Template class representing a ADC0 channel
 *
 * Refer @ref Adc0Channel
 *
 * @tparam channel ADC channel
 */
template<int channel>
class Adc1Channel : public AdcBase_T<Adc1Info>, CheckSignal<Adc1Info, channel> {

public:
   /**
    * Initiates a conversion and waits for it to complete
    *
    * @return - the result of the conversion
    */
   static int readAnalogue() {
      return AdcBase_T::readAnalogue(channel);
   };
};
/**
 * Class representing ADC1
 */
using Adc1 = AdcIrq_T<Adc1Info>;
#endif

/**
 * @}
 */
 
} // End namespace USBDM

#endif /* HEADER_ADC_H */

