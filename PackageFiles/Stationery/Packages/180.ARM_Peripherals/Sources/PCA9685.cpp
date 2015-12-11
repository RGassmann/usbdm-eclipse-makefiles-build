/**
 * @file pca9685.cpp
 *
 *  Created on: 12/12/2015
 *      Author: podonoghue
 */
#include "pca9685.h"
#include <assert.h>

namespace USBDM {

constexpr uint32_t I2C_SWRST_ADDRESS      =  (0x00<<1); // General I2C Software reset address

//============== PCA9685 register addresses
constexpr uint32_t PCA9685_RESET          =  (0x06);   // Used in conjunction with I2C_SWRST_ADDR

constexpr uint32_t PCA9685_MODE1          =  (0x00);   // READ/WRITE Mode register 1
constexpr uint32_t PCA9685_MODE2          =  (0x01);   // READ/WRITE Mode register 2
constexpr uint32_t PCA9685_SUBADR1        =  (0x02);   // READ/WRITE I2C-bus sub-address 1
constexpr uint32_t PCA9685_SUBADR2        =  (0x03);   // READ/WRITE I2C-bus sub-address 2
constexpr uint32_t PCA9685_SUBADR3        =  (0x04);   // READ/WRITE I2C-bus sub-address 3
constexpr uint32_t PCA9685_ALLCALLADR     =  (0x05);   // READ/WRITE OUT All Call I2C-bus address
constexpr uint32_t PCA9685_OUT_START      =  (0x06);   // READ/WRITE OUT0  output and brightness control byte 0
//constexpr uint32_t PCA9685_OUT0_ON_L       = (0x06);   // READ/WRITE OUT0  output and brightness control byte 0
//constexpr uint32_t PCA9685_OUT0_ON_H       = (0x07);   // READ/WRITE OUT0  output and brightness control byte 1
//constexpr uint32_t PCA9685_OUT0_OFF_L      = (0x08);   // READ/WRITE OUT0  output and brightness control byte 2
//constexpr uint32_t PCA9685_OUT0_OFF_H      = (0x09);   // READ/WRITE OUT0  output and brightness control byte 3
//...
constexpr uint32_t PCA9685_ALL_OUT_ON_L   =  (0xFA);   // READ 0/WRITE load all the OUTn_ON  registers, byte 0
constexpr uint32_t PCA9685_ALL_OUT_ON_H   =  (0xFB);   // READ 0/WRITE load all the OUTn_ON  registers, byte 1
constexpr uint32_t PCA9685_ALL_OUT_OFF_L  =  (0xFC);   // READ 0/WRITE load all the OUTn_OFF registers, byte 2
constexpr uint32_t PCA9685_ALL_OUT_OFF_H  =  (0xFD);   // READ 0/WRITE load all the OUTn_OFF registers, byte 3

constexpr uint32_t PCA9685_PRE_SCALE      =  (0xFE);   // READ/WRITE pre-scaler for output frequency
constexpr uint32_t PCA9685_TestMode       =  (0xFF);   // READ/WRITE defines the test mode to be entered

// Calculates base register address for pin register set
constexpr inline int PCA9685_PIN_REG(int pinNum) {
   return ((uint8_t)(PCA9685_OUT_START+4*(pinNum)));
}

// OUTx_ON_L
//             7:0 OUTx_ON count for OUTx, 8 LSBs
constexpr uint32_t OUTx_ON_L_COUNT_MASK = (0xFF<<0);
// OUTx_ON_H
constexpr uint32_t OUTx_ON_H_FULL_MASK  = (1<<4);
constexpr uint32_t OUTx_ON_H_COUNT_MASK = (0xF<<0);
//               4 OUTx full ON
//             3:0 OUTx_ON count for OUTx, 4 MSBs
// OUTx_OFF_L
constexpr uint32_t OUTx_OFF_L_COUNT_MASK = (0xFF<<0);
//             7:0 OUTx_OFF count for OUTx, 8 LSBs
// OUTx_OFF_H
constexpr uint32_t OUTx_OFF_H_FULL_MASK  = (1<<4);
constexpr uint32_t OUTx_OFF_H_COUNT_MASK = (0xF<<0);
//               4 OUTx full OFF
//             3:0 OUTx_OFF count for OUTx, 4 MSBs

constexpr uint32_t ALL_CALL_ADDDRESS  =  (0xE0);

inline void assert_helper( bool test ) {
  assert(test);
}
inline constexpr bool constexpr_assert( bool test ) {
  return test?true:(assert_helper(test),false);
}

/**
 * Constructor with default values
 *
 * @param deviceAddress    Device I2C address
 * @param prescaleValue    Prescale value for the internal clock
 * @param mode1Value       Mode register 1 value
 * @param mode2Value       Mode Register 2 value
 *
 */
PCA9685::PCA9685(I2c *i2cInterface, uint8_t deviceAddress, uint8_t prescaleValue, uint8_t mode1Value, uint8_t mode2Value) {
   slaveAddress = deviceAddress;

   this->i2cInterface = i2cInterface;

   assert((PCA9685_DEFAULT_OSC_PRESCALE & ~0xFFU) == 0); // Check if OSC_PRESCALE is too large

//   uint8_t resetdata[] = {PCA9685_RESET};
//   i2c_transmit(I2C_SWRST_ADDRESS, resetdata, sizeof(resetdata));
   //
   const uint8_t mode1data[] = {
       /* register address */   PCA9685_MODE1,
       /* mode1            */   mode1Value,
       /* mode2            */   mode2Value,
       /* subaddr1         */   0,
       /* subaddr2         */   0,
       /* subaddr3         */   0,
       /* all call address */   ALL_CALL_ADDDRESS
   };
   i2cInterface->transmit(slaveAddress, mode1data, sizeof(mode1data));
   //
   const uint8_t prescaleData[] = {
      PCA9685_PRE_SCALE,
      prescaleValue   // Prescale value for clock
   };
   i2cInterface->transmit(slaveAddress, prescaleData, sizeof(prescaleData));
   // All outputs off
   allHigh();
}

/**
 * Set all outputs high
 */
void PCA9685::allHigh(void) {
   const uint8_t data[] = {
      /* register */ (uint8_t) PCA9685_ALL_OUT_ON_L,
      /* onLow    */ (uint8_t) 0,
      /* onHigh   */ (uint8_t) 0,
      /* offLow   */ (uint8_t) 0,
      /* offHigh  */ (uint8_t) OUTx_OFF_H_FULL_MASK,
   };
   i2cInterface->transmit(ALL_CALL_ADDDRESS, data, sizeof(data));
}

/**
 * Set all outputs low
 */
void PCA9685::allLow(void) {
   const uint8_t data[] = {
      /* register */ (uint8_t) PCA9685_ALL_OUT_ON_L,
      /* onLow    */ (uint8_t) 0,
      /* onHigh   */ (uint8_t) OUTx_ON_H_FULL_MASK,
      /* offLow   */ (uint8_t) 0,
      /* offHigh  */ (uint8_t) 0,
   };
   i2cInterface->transmit(ALL_CALL_ADDDRESS, data, sizeof(data));
}

/**
 * Sets the dutyCycle of the given pin
 *
 * @param pinNum     Pin to modify
 * @param dutyCycle  Duty-cycle to set. Expressed as a value [0..4095]
 *
 */
void PCA9685::set_pin_pwm(unsigned pinNum, unsigned dutyCycle) {
   if (dutyCycle>MAX_PWM) {
      dutyCycle = MAX_PWM;
   }
   const uint16_t onCount  = 0;
   const uint16_t offCount = dutyCycle;
   const uint8_t data[] = {
      /* register */ (uint8_t) PCA9685_PIN_REG(pinNum),
      /* onLow    */ (uint8_t) onCount,
      /* onHigh   */ (uint8_t) ((onCount>>8)&OUTx_ON_H_COUNT_MASK),
      /* offLow   */ (uint8_t) offCount,
      /* offHigh  */ (uint8_t) ((offCount>>8)&OUTx_OFF_H_COUNT_MASK),
   };
   i2cInterface->transmit(slaveAddress, data, sizeof(data));
}

/**
 * Set given pin low
 *
 * @param pinNum Pin to change
 */
void PCA9685::set_pin_low(unsigned pinNum) {
   const uint8_t data[] = {
      /* register */ (uint8_t) PCA9685_PIN_REG(pinNum),
      /* onLow    */ (uint8_t) 0,
      /* onHigh   */ (uint8_t) OUTx_ON_H_FULL_MASK,
      /* offLow   */ (uint8_t) 0,
      /* offHigh  */ (uint8_t) 0,
   };
   i2cInterface->transmit(slaveAddress, data, sizeof(data));
}

/**
 * Set given pin high
 *
 * @param pinNum Pin to change
 */
void PCA9685::set_pin_high(unsigned pinNum) {
   const uint8_t data[] = {
      /* register */ (uint8_t) PCA9685_PIN_REG(pinNum),
      /* onLow    */ (uint8_t) 0,
      /* onHigh   */ (uint8_t) 0,
      /* offLow   */ (uint8_t) 0,
      /* offHigh  */ (uint8_t) OUTx_OFF_H_FULL_MASK,
   };
   i2cInterface->transmit(slaveAddress, data, sizeof(data));
}

} // End namespace USBDM
