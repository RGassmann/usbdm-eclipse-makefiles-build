/**
 * @file     adc.h (180.ARM_Peripherals/Project_Headers/adc.h)
 * @brief    ADC routines
 *
 * @version  V4.12.1.80
 * @date     13 April 2016
 */
#ifndef HEADER_ADC_H
#define HEADER_ADC_H
/*
 * *****************************
 * *** DO NOT EDIT THIS FILE ***
 * *****************************
 *
 * This file is generated automatically.
 * Any manual changes will be lost.
 */
#include <stddef.h>
#include <assert.h>
#include "derivative.h"

/*
 * Default port information
 */
#ifndef FIXED_PORT_CLOCK_REG
#define FIXED_PORT_CLOCK_REG SCGC5
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
 * @addtogroup ADC_Group ADC, Analogue Input
 * @brief Abstraction for Analogue Input
 */

/**
 * Default PCR value for pins used as GPIO (including multiplexor value)
 */
static constexpr PcrValue ADC_DEFAULT_PCR = pcrValue(
      PinPull_None, PinDriveStrength_Low, PinDriveMode_PushPull, PinIrq_None, PinFilter_None, PinSlewRate_Fast, PinMux_Analogue);

/**
 * ADC Resolutions for use with AnalogueIO::setMode()
 */
enum AdcResolution {
   AdcResolution_8bit_se    = ADC_CFG1_MODE(0),  //!<  8-bit unsigned for use with single-ended mode
   AdcResolution_10bit_se   = ADC_CFG1_MODE(2),  //!< 10-bit unsigned for use with single-ended mode
   AdcResolution_12bit_se   = ADC_CFG1_MODE(1),  //!< 12-bit unsigned for use with single-ended mode
   AdcResolution_16bit_se   = ADC_CFG1_MODE(3),  //!< 16-bit unsigned for use with single-ended mode
   AdcResolution_9bit_diff  = ADC_CFG1_MODE(0),  //!<  9-bit signed for use with differential mode
   AdcResolution_11bit_diff = ADC_CFG1_MODE(2),  //!< 11-bit signed for use with differential mode
   AdcResolution_12bit_diff = ADC_CFG1_MODE(1),  //!< 12-bit signed for use with differential mode
   AdcResolution_16bit_diff = ADC_CFG1_MODE(3),  //!< 16-bit signed for use with differential mode
};

/**
 * ADC Averaging
 */
enum AdcAveraging {
   AdcAveraging_off = 0,                                 //!< No averaging - single conversion
   AdcAveraging_4   = ADC_SC3_AVGE_MASK|ADC_SC3_AVGS(0), //!< Average across 4 conversions
   AdcAveraging_8   = ADC_SC3_AVGE_MASK|ADC_SC3_AVGS(1), //!< Average across 8 conversions
   AdcAveraging_16  = ADC_SC3_AVGE_MASK|ADC_SC3_AVGS(2), //!< Average across 16 conversions
   AdcAveraging_32  = ADC_SC3_AVGE_MASK|ADC_SC3_AVGS(3), //!< Average across 32 conversions
};

/**
 * ADC clock source
 */
enum AdcClockSource {
   AdcClockSource_Bus      = ADC_CFG1_ADICLK(0), //!< Bus Clock
   AdcClockSource_Busdiv2  = ADC_CFG1_ADICLK(1), //!< Bus Clock / 2
   AdcClockSource_Alt      = ADC_CFG1_ADICLK(2), //!< Alternate clock (ALTCLK)
   AdcClockSource_Asynch   = ADC_CFG1_ADICLK(3), //!< Asynchronous clock (ADACK Internal ADC clock source)
};

/**
 * ADC clock divider
 */
enum AdcClockDivider {
   AdcClockDivider_1       = ADC_CFG1_ADIV(0), //!< Clock divide by 1
   AdcClockDivider_2       = ADC_CFG1_ADIV(1), //!< Clock divide by 2
   AdcClockDivider_4       = ADC_CFG1_ADIV(2), //!< Clock divide by 3
   AdcClockDivider_8       = ADC_CFG1_ADIV(3), //!< Clock divide by 4
};

enum AdcInterrupt {
   AdcInterrupt_disable = ADC_SC1_AIEN(0), //!< No interrupt on conversion complete
   AdcInterrupt_enable  = ADC_SC1_AIEN(1), //!< No interrupt on conversion complete
};

/**
 * Type definition for ADC interrupt call back
 *
 * @param[in] value Conversion value from channel
 * @param[in] value Channel number for the conversion
 */
typedef void (*ADCCallbackFunction)(uint32_t value, int channel);

/**
 * Template class representing an ADC
 *
 * Example
 * @code
 *  // Instantiate ADC channel adc0_se8
 *  const AdcBase_T<adc0Info> Adc0;
 *
 *  // Initialise ADC
 *  Adc0.setMode(AdcResolution_16bit_se);
 *  @endcode
 *
 * @tparam info Table of information describing ADC
 */
template<class Info>
class AdcBase_T {

private:
   /**
    * This class is not intended to be instantiated
    */
   AdcBase_T() = delete;
   AdcBase_T(const AdcBase_T&) = delete;
   AdcBase_T(AdcBase_T&&) = delete;

protected:
   /** Clock register for peripheral */
   static constexpr volatile uint32_t *clockReg = Info::clockReg;

   /** Callback to catch unhandled interrupt */
   static void errorCallback(uint32_t, int) {
      setAndCheckErrorCode(E_NO_HANDLER);
   }
public:
   /** Peripheral hardware instance */
   static constexpr volatile ADC_Type *adc      = Info::adc;

   /**
    * Basic enable of ADC\n
    * Includes configuring all pins
    */
   static void enable() {
      // Configure pins
      Info::initPCRs();
      Info::InfoDP::initPCRs();
      Info::InfoDM::initPCRs();

      // Enable clock to ADC
      *clockReg  |= Info::clockMask;
      __DMB();
   }

   /**
    * Initialise ADC to default settings\n
    * Configures all ADC pins
    */
   static void configure() {
      enable();
      
      // Set mode to default
      adc->CFG1 = Info::cfg1;
      adc->CFG2 = Info::cfg2;
      adc->SC2  = Info::sc2;

      enableNvicInterrupts();
   }

   /**
    * Enable/disable interrupts in NVIC
    *
    * @param[in] enable true to enable, false to disable
    */
   static __attribute__((always_inline)) void enableNvicInterrupts(bool enable=true) {

      if (enable) {
         // Enable interrupts
         NVIC_EnableIRQ(Info::irqNums[0]);

         // Set priority level
         NVIC_SetPriority(Info::irqNums[0], Info::irqLevel);
      }
      else {
         // Disable interrupts
         NVIC_DisableIRQ(Info::irqNums[0]);
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
    * @param[in] adcResolution Resolution for converter e.g. AdcResolution_16bit_se
    *
    * @note This affects all channels on the ADC
    */
   static __attribute__((always_inline)) void setResolution(AdcResolution adcResolution = AdcResolution_16bit_se) {
      adc->CFG1 = (adc->CFG1&~ADC_CFG1_MODE_MASK)|adcResolution;
   }

   /**
    * Set clock source
    *
    * @param[in] adcClockSource  Clock source e.g. AdcClockSource_Asynch
    * @param[in] adcClockDivider Clock divider e.g. AdcClockDivider_4
    *
    * @note This affects all channels on the ADC.\n
    * The resulting ADC clock rate should be restricted to the following ranges:\n
    *  [2..12MHz] for 16-bit conversion modes  \n
    *  [1..18MHz] for other conversion modes
    */
   static __attribute__((always_inline)) void setClockSource(AdcClockSource adcClockSource = AdcClockSource_Asynch, AdcClockDivider adcClockDivider=AdcClockDivider_1) {
      adc->CFG1 = (adc->CFG1&~(ADC_CFG1_ADIV_MASK|ADC_CFG1_ADICLK_MASK))|adcClockSource|adcClockDivider;
   }

   /**
    * Enable ADC internal asynchronous clock source
    *
    * @param[in] enable true to enable clock, false to disable
    *
    * @note It is not necessary to enable the internal clock to use it as a clock source.\n
    *       If the internal clock is selected, it will be automatically enabled when an ADC conversion is initiated.\n
    *       However, enabling it beforehand will reduce the latency of the 1st conversion in a sequence.
    */
   static __attribute__((always_inline)) void enableAsynchronousClock(bool enable=true) {
      if (enable) {
         adc->CFG1 |= ADC_CFG2_ADACKEN_MASK;
      }
      else {
         adc->CFG1 &= ~ADC_CFG2_ADACKEN_MASK;
      }
   }

   /**
    * Set averaging mode
    *
    * @param[in] adcAveraging Mode for averaging e.g. AdcAveraging_4 etc
    *
    * @note This affects all channels on the ADC
    */
   static __attribute__((always_inline)) void setAveraging(AdcAveraging adcAveraging = AdcAveraging_4) {
      adc->SC3 = (adc->SC3&~(ADC_SC3_AVGE_MASK|ADC_SC3_AVGS_MASK))|adcAveraging;
   }

protected:
   /**
    * Enables hardware trigger mode of operation and configures a channel
    *
    * @param[in] hardwareTrigger Hardware trigger to use for this channel\n
    *                            This corresponds to pre-triggers in the PDB channels and SC1[n] register setups
    * @param[in] sc1Value        SC1 register value including the ADC channel to use
    */
   static void enableHardwareConversion(int hardwareTrigger, int sc1Value) {
      // Set hardware triggers
      adc->SC2 |= ADC_SC2_ADTRG(1);
      // Configure channel for hardware trigger input
      adc->SC1[hardwareTrigger] = sc1Value;
   }

   /**
    * Initiates a conversion but does not wait for it to complete.\n
    * Intended for use with interrupts so ADC interrupts are enabled
    *
    * @param[in] sc1Value     SC1 register value including the ADC channel to use
    * @param[in] continuous   If true select continuous conversion mode
    *
    * @return E_NO_ERROR on success
    */
   static __attribute__((always_inline)) ErrorCode startConversion(const int sc1Value, bool continuous) {
      if (!Info::irqHandlerInstalled) {
         return setErrorCode(E_NO_HANDLER);
      }
      if (continuous) {
         adc->SC3 |= ADC_SC3_ADCO_MASK;
      }
      else {
         adc->SC3 &= ~ADC_SC3_ADCO_MASK;
      }
      // Trigger conversion with interrupts enabled
      adc->SC1[0] = ADC_SC1_AIEN_MASK|(sc1Value&(ADC_SC1_ADCH_MASK|ADC_SC1_AIEN_MASK|ADC_SC1_DIFF_MASK));

      return E_NO_ERROR;
   };

   /**
    * Initiates a conversion and waits for it to complete
    *
    * @param[in] sc1Value SC1 register value including the ADC channel to use
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

   using AdcInfo = Info;

   /**
    * IRQ handler
    */
   static void irqHandler() {
      if (AdcBase_T<Info>::adc->SC1[0] & ADC_SC1_COCO_MASK) {
         callback(AdcBase_T<Info>::adc->R[0], AdcBase_T<Info>::adc->SC1[0]&ADC_SC1_ADCH_MASK);
      }
      if (AdcBase_T<Info>::adc->SC1[1] & ADC_SC1_COCO_MASK) {
         callback(AdcBase_T<Info>::adc->R[1], AdcBase_T<Info>::adc->SC1[1]&ADC_SC1_ADCH_MASK);
      }
   }

   /**
    * Set Callback function
    *
    * @param[in] theCallback Callback function to execute on interrupt.\n
    *            nullptr to remove handler
    */
   static void setCallback(ADCCallbackFunction theCallback) {
      if (theCallback == nullptr) {
         callback = AdcBase_T<Info>::errorCallback;
         return;
      }
      callback = theCallback;
   }
};

template<class Info> ADCCallbackFunction AdcIrq_T<Info>::callback = AdcBase_T<Info>::errorCallback;

/**
 * Template class representing an ADC channel
 *
 * Example
 * @code
 * // Instantiate the ADC channel (for ADC0 channel 6)
 * using Adc0_ch6 = USBDM::AdcChannel<Adc0Info, 6>;
 *
 * // Set ADC resolution
 * Adc0_ch6.setMode(AdcResolution_16bit_se);
 *
 * // Read ADC value
 * uint32_t value = Adc0_ch6.readAnalogue();
 * @endcode
 *
 * @tparam info    Table of information describing ADC
 * @tparam channel ADC channel
 */
template<class Info, int channel>
class AdcChannel : public AdcBase_T<Info>, CheckSignal<Info, channel> {

public:
   /** Channel number */
   static constexpr int CHANNEL=channel;

   /**
    * Enables hardware trigger mode of operation and configures a channel
    *
    * @param[in] hardwareTrigger Hardware trigger to use for this channel\n
    *                            This corresponds to pre-triggers in the PDB channels and SC1[n] register setups
    * @param[in] enableInterrupt Whether to generate interrupt when complete
    */
   static void enableHardwareConversion(int hardwareTrigger, AdcInterrupt enableInterrupt=AdcInterrupt_disable) {
      AdcBase_T<Info>::enableHardwareConversion(hardwareTrigger, ADC_SC1_ADCH(channel)|enableInterrupt);
   }

   /**
    * Initiates a conversion but does not wait for it to complete.\n
    * Intended for use with interrupts so ADC interrupts are enabled
    *
    * @param[in] continuous   If true select continuous conversion mode
    */
   static __attribute__((always_inline)) void startConversion(bool continuous=false) {
      AdcBase_T<Info>::startConversion(ADC_SC1_ADCH(channel), continuous);
   };
   /**
    * Initiates a conversion and waits for it to complete
    *
    * @return - the result of the conversion
    */
   static __attribute__((always_inline)) uint32_t readAnalogue() {
      return AdcBase_T<Info>::readAnalogue(ADC_SC1_ADCH(channel));
   };
};

/**
 * Template class representing an ADC differential channel
 *
 * Example
 * @code
 * // Instantiate the differential ADC channels (for ADC_DM0, ADC_DP0)
 * using Adc0 = USBDM::Adc0DiffChannel<Adc0Info,0>;
 *
 * // Set ADC resolution
 * Adc0.setMode(AdcResolution_11bit_diff );
 *
 * // Read ADC value
 * uint32_t value = Adc0.readAnalogue();
 * @endcode
 *
 * @tparam info    Table of information describing ADC
 * @tparam channel ADC channel
 */
template<class Info, int channel>
class AdcDiffChannel : public AdcBase_T<Info>, CheckSignal<typename Info::InfoDP, channel>, CheckSignal<typename Info::InfoDM, channel> {

public:
   /** Channel number */
   static constexpr int CHANNEL=channel;

   /**
    * Enables hardware trigger mode of operation and configures a channel
    *
    * @param[in] hardwareTrigger Hardware trigger to use for this channel\n
    *                            This corresponds to pre-triggers in the PDB channels and SC1[n] register setups
    * @param[in] enableInterrupt Whether to generate interrupt when complete
    */
   static void enableHardwareConversion(int hardwareTrigger, AdcInterrupt enableInterrupt=AdcInterrupt_disable) {
      AdcBase_T<Info>::enableHardwareConversion(ADC_SC1_ADCH(channel)|ADC_SC1_DIFF_MASK, channel|enableInterrupt);
   }

   /**
    * Initiates a conversion but does not wait for it to complete.\n
    * Intended for use with interrupts so ADC interrupts are enabled
    *
    * @param[in] continuous   If true select continuous conversion mode
    */
   static __attribute__((always_inline)) void startConversion(bool continuous=false) {
      AdcBase_T<Info>::startConversion(ADC_SC1_ADCH(channel)|ADC_SC1_DIFF_MASK, continuous);
   };
   /**
    * Initiates a conversion and waits for it to complete
    *
    * @return - the result of the conversion
    */
   static __attribute__((always_inline)) int32_t readAnalogue() {
      return AdcBase_T<Info>::readAnalogue(ADC_SC1_ADCH(channel)|ADC_SC1_DIFF_MASK);
   };
};

#ifdef USBDM_ADC0_IS_DEFINED
/**
 * Template class representing an ADC0 channel
 *
 * Example
 * @code
 * // Instantiate the ADC channel (for ADC0 channel 6)
 * using Adc = USBDM::Adc0Channel<6>;
 *
 * // Set ADC resolution
 * Adc.setMode(AdcResolution_16bit_se);
 *
 * // Read ADC value
 * uint32_t value = Adc.readAnalogue();
 * @endcode
 *
 * @tparam channel ADC channel
 */
template<int channel>
class Adc0Channel : public AdcChannel<Adc0Info, channel> {
};
//template<int channel> using Adc0Channel = AdcChannel<Adc0Info, channel>;

/**
 * Template class representing an ADC0 differential channel
 *
 * Example
 * @code
 * // Instantiate the differential ADC0 channels (for ADC0_DM0, ADC0_DP0)
 * using Adc = USBDM::Adc0DiffChannel<0>;
 *
 * // Set ADC resolution
 * Adc.setMode(AdcResolution_11bit_diff );
 *
 * // Read ADC value
 * uint32_t value = Adc0.readAnalogue();
 * @endcode
 *
 * @tparam channel ADC channel
 */
template<int channel>
class Adc0DiffChannel : public AdcDiffChannel<Adc0Info, channel> {
};
//template<int channel> using Adc0DiffChannel = AdcDiffChannel<Adc0Info, channel>;

/**
 * Class representing ADC0
 */
using Adc0 = AdcIrq_T<Adc0Info>;
#endif

#ifdef USBDM_ADC1_IS_DEFINED
/**
 * Template class representing an ADC1 channel
 *
 * Example
 * @code
 * // Instantiate the ADC channel (for ADC1 channel 6)
 * using Adc = USBDM::Adc1Channel<6>;
 *
 * // Set ADC resolution
 * Adc.setMode(AdcResolution_16bit_se);
 *
 * // Read ADC value
 * uint32_t value = Adc.readAnalogue();
 * @endcode
 *
 * @tparam channel ADC channel
 */
template<int channel>
class Adc1Channel : public AdcChannel<Adc1Info, channel> {
};
//template<int channel> using Adc1Channel = AdcChannel<Adc1Info, channel>;

/**
 * Template class representing an ADC1 differential channel
 *
 * Example
 * @code
 * // Instantiate the differential ADC1 channels (for ADC1_DM0, ADC1_DP0)
 * using Adc = USBDM::Adc1DiffChannel<0>;
 *
 * // Set ADC resolution
 * Adc.setMode(AdcResolution_11bit_diff );
 *
 * // Read ADC value
 * uint32_t value = Adc.readAnalogue();
 * @endcode
 *
 * @tparam channel ADC channel
 */
template<int channel>
class Adc1DiffChannel : public AdcDiffChannel<Adc1Info, channel> {
};
//template<int channel> using Adc1DiffChannel = AdcDiffChannel<Adc01Info, channel>;

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

