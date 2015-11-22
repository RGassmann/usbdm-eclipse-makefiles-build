/**
 * @file FXOS8700CQ.cpp
 *
 *  Created on: 22/11/2013
 *      Author: podonoghue
 */
//#include <stdio.h>
#include "FXOS8700CQ.h"
#include "delay.h"

// Accelerometer registers
enum {
   STATUS,
   F_STATUS = STATUS,
   OUT_X_MSB,
   OUT_X_LSB,
   OUT_Y_MSB,
   OUT_Y_LSB,
   OUT_Z_MSB,
   OUT_Z_LSB,
   Reservedx07,
   Reservedx08,
   F_SETUP,
   TRIG_CFG,
   SYSMOD,
   INT_SOURCE,
   WHO_AM_I,
   XYZ_DATA_CFG,
   HP_FILTER_CUTOFF,
   PL_STATUS,
   PL_CFG,
   PL_COUNT,
   PL_BF_ZCOMP,
   P_L_THS_REG,
   A_FFMT_CFG,
   A_FFMT_SRC,
   A_FFMT_THS,
   A_FFMT_COUNT,
   reservedx19,
   reservedx1A,
   reservedx1B,
   reservedx1C,
   TRANSIENT_CFG,
   TRANSIENT_SCR,
   TRANSIENT_THS,
   TRANSIENT_COUNT,
   PULSE_CFG,
   PULSE_SRC,
   PULSE_THSX,
   PULSE_THSY,
   PULSE_THSZ,
   PULSE_TMLT,
   PULSE_LTCY,
   PULSE_WIND,
   ASLP_COUNT,
   CTRL_REG1,
   CTRL_REG2,
   CTRL_REG3,
   CTRL_REG4,
   CTRL_REG5,
   OFF_X,
   OFF_Y,
   OFF_Z,

   // Magnetometer registers (0x32 onwards)
   M_DR_STATUS,
   M_OUT_X_MSB,
   M_OUT_X_LSB,
   M_OUT_Y_MSB,
   M_OUT_Y_LSB,
   M_OUT_Z_MSB,
   M_OUT_Z_LSB,
   CMP_X_MSB,
   CMP_X_LSB,
   CMP_Y_MSB,
   CMP_Y_LSB,
   CMP_Z_MSB,
   CMP_Z_LSB,
   M_OFF_X_MSB,
   M_OFF_X_LSB,
   M_OFF_Y_MSB,
   M_OFF_Y_LSB,
   M_OFF_Z_MSB,
   M_OFF_Z_LSB,
   MAX_X_MSB,
   MAX_X_LSB,
   MAX_Y_MSB,
   MAX_Y_LSB,
   MAX_Z_MSB,
   MAX_Z_LSB,
   MIN_X_MSB,
   MIN_X_LSB,
   MIN_Y_MSB,
   MIN_Y_LSB,
   MIN_Z_MSB,
   MIN_Z_LSB,
   TEMP,
   M_THS_CFG,
   M_THS_SRC,
   M_THS_X_MSB,
   M_THS_X_LSB,
   M_THS_Y_MSB,
   M_THS_Y_LSB,
   M_THS_Z_MSB,
   M_THS_Z_LSB,
   M_THS_COUNT,
   M_CTRL_REG1,
   M_CTRL_REG2,
   M_CTRL_REG3,
   M_INT_SRC,
   A_VECM_CFG,
   A_VECM_THS_MSB,
   A_VECM_THS_LSB,
   A_VECM_CNT,
   A_VECM_INITX_MSB,
   A_VECM_INITX_LSB,
   A_VECM_INITY_MSB,
   A_VECM_INITY_LSB,
   A_VECM_INITZ_MSB,
   A_VECM_INITZ_LSB,
   M_VECM_CFG,
   M_VECM_THS_MSB,
   M_VECM_THS_LSB,
   M_VECM_CNT,
   M_VECM_INITX_MSB,
   M_VECM_INITX_LSB,
   M_VECM_INITY_MSB,
   M_VECM_INITY_LSB,
   M_VECM_INITZ_MSB,
   M_VECM_INITZ_LSB,
   A_FFMT_THS_X_MSB,
   A_FFMT_THS_X_LSB,
   A_FFMT_THS_Y_MSB,
   A_FFMT_THS_Y_LSB,
   A_FFMT_THS_Z_MSB,
   A_FFMT_THS_Z_LSB,
   Reservedx79,
};

/*
 * Constructor
 *
 * @param i2c  - The I2C interface to use
 * @param mode - Mode of operation (gain and filtering)
 */
FXOS8700CQ::FXOS8700CQ(I2C *i2c, AccelerometerMode mode) : i2c(i2c) {
   failedInit = false;
   if (readReg(WHO_AM_I) != WHO_AM_I_VALUE) {
      failedInit = true;
      return;
   }
   reset();

   writeReg(CTRL_REG3, 0x00);                                   // INT0/1 active low, open drain
   writeReg(CTRL_REG4, FXOS8700CQ_CTRL_REG4_INT_EN_DRDY_MASK);  // Enable DRDY Interrupt
   writeReg(CTRL_REG5, FXOS8700CQ_CTRL_REG5_INT_CFG_DRDY_MASK); // Route DRDY to INT1 pin (0=> INT2, 1=> INT1)

   writeReg(M_CTRL_REG2, FXOS8700CQ_M_CTRL_REG2_M_HYB_AUTOINC_MODE_MASK); // Hybrid auto-increment

   setAccelerometerMode(mode);

//   uint8_t buff[5] = {CTRL_REG1};
//   i2c->txRx(DEVICE_ADDRESS, buff, 1, sizeof(buff));
//   printf("FXOS8700CQ, ctrl = 0x%x,0x%x,0x%x,0x%x,0x%x\n", buff[0], buff[1], buff[2], buff[3], buff[4] );
}

/**
 * Enable accelerometer and/or magnetometer
 *
 * @param mode ACCEL_ONLY, MAG_ONLY or ACCEL_MAG
 */
void FXOS8700CQ::enable(Mode mode) {
   writeReg(M_CTRL_REG1, FXOS8700CQ_M_CTRL_REG1_M_OS(7)|FXOS8700CQ_M_CTRL_REG1_M_HMS(mode));
}

/**
 * Read Accelerometer register
 *
 * @param regNum  - Register number
 */
uint8_t FXOS8700CQ::readReg(uint8_t regNum) {
   uint8_t command[] = {regNum};

   i2c->txRx(DEVICE_ADDRESS, command, 1, sizeof(command));
   return command[0];
}

/**
 * Write Accelerometer register
 *
 * @param regNum  - Register number
 * @param value   - Value to write
 */
void FXOS8700CQ::writeReg(uint8_t regNum, uint8_t value) {
   uint8_t command[] = {regNum, value};

   i2c->transmit(DEVICE_ADDRESS, command, sizeof(command));
}

/**
 * Reset Accelerometer
 */
void FXOS8700CQ::reset(void) {

   writeReg(CTRL_REG2, FXOS8700CQ_CTRL_REG2_RST_MASK);

   // Device is not accessible after RESET
   waitUS(1000);
}

/**
 * Put accelerometer into Standby mode
 */
void FXOS8700CQ::standby() {

   writeReg(CTRL_REG1, readReg(CTRL_REG1)&~FXOS8700CQ_CTRL_REG1_ACTIVE_MASK);
}

/**
 * Put accelerometer into Active mode
 */
void FXOS8700CQ::active() {

   writeReg(CTRL_REG1, readReg(CTRL_REG1)|FXOS8700CQ_CTRL_REG1_ACTIVE_MASK);
}

/**
 * Obtains measurements from the accelerometer
 *
 * @param status  - Indicates status of x, y & z measurements
 * @param x       - X axis value
 * @param y       - Y axis value
 * @param z       - Z axis value
 */
void FXOS8700CQ::readAccelerometerXYZ(int *status, int16_t *x, int16_t *y, int16_t *z) {
   uint8_t dataXYZ[7] = {STATUS};

   // Receive 7 registers (status, X-high, X-low, Y-high, Y-low, Z-high & Z-low)
   i2c->txRx(DEVICE_ADDRESS, dataXYZ, 1, sizeof(dataXYZ));

   // Unpack data and return
   *status = dataXYZ[0];
   *x = ((int16_t)((dataXYZ[1]<<8)+dataXYZ[2]))>>2;
   *y = ((int16_t)((dataXYZ[3]<<8)+dataXYZ[4]))>>2;
   *z = ((int16_t)((dataXYZ[5]<<8)+dataXYZ[6]))>>2;
}

/**
 * Set accelerometer mode (gain and filtering)
 *
 * @param mode - one of ACCEL_2Gmode etc.
 */
void FXOS8700CQ::setAccelerometerMode(AccelerometerMode mode) {
   // Make inactive
   writeReg(CTRL_REG1, 0x00);

   // Change mode
   writeReg(XYZ_DATA_CFG, FXOS8700CQ_XYZ_DATA_CFG_FS(mode));

   // Make active etc
   writeReg(CTRL_REG1,
         FXOS8700CQ_CTRL_REG1_ASLP_RATE(0) | /* 50 Hz auto-sleep rate */
         FXOS8700CQ_CTRL_REG1_DR(2) |        /* 200 Hz update rate    */
         FXOS8700CQ_CTRL_REG1_ACTIVE_MASK);  /* Active     */
}

/*
 * Obtains measurements from the Magnetometer
 *
 * @param status  - Indicates status of x, y & z measurements
 * @param x       - X axis value
 * @param y       - Y axis value
 * @param z       - Z axis value
 */
void FXOS8700CQ::readMagnetometerXYZ(int *status, int16_t *x, int16_t *y, int16_t *z) {
   uint8_t dataXYZ[7] = {M_DR_STATUS};

   // Receive 7 registers (status, X-high, X-low, Y-high, Y-low, Z-high & Z-low)
   i2c->txRx(DEVICE_ADDRESS, dataXYZ, 1, sizeof(dataXYZ));

   // Unpack data and return (data is sign extended)
   *status = dataXYZ[0];
   *x = ((dataXYZ[1]<<8)+dataXYZ[2]);
   *y = ((dataXYZ[3]<<8)+dataXYZ[4]);
   *z = ((dataXYZ[5]<<8)+dataXYZ[6]);
}

/**
 * Set magnetometer mode (gain and filtering)
 *
 * @param mode - one of 2Gmode etc.
 */
void FXOS8700CQ::setMagnetometerMode(ControlReg2Mode mode) {
   writeReg(M_CTRL_REG2, mode|FXOS8700CQ_M_CTRL_REG2_M_HYB_AUTOINC_MODE_MASK);
}

/*
 * Obtains measurements from the Accelerometer & Magnetometer
 *
 * @param data  Reference to structure to contain values read
 */
void FXOS8700CQ::readAll(Data &data) {
   uint8_t dataXYZ[13] = {XYZ_DATA_CFG};

   // Receive 14 registers (accelerometerStatus, X-high/low, Y-high/low, Z-high/low,
   //                       magnetometerStatus, X-high/low, Y-high/low, Z-high/low)
   i2c->txRx(DEVICE_ADDRESS, dataXYZ, 1, sizeof(dataXYZ));
   data.accelerometerStatus = dataXYZ[0];
   data.accelerometer_X     = ((dataXYZ[1]<<8)+dataXYZ[2]);
   data.accelerometer_Y     = ((dataXYZ[3]<<8)+dataXYZ[4]);
   data.accelerometer_Z     = ((dataXYZ[5]<<8)+dataXYZ[6]);
   data.magnetometerStatus = dataXYZ[7];
   data.magnetometer_X      = ((dataXYZ[7]<<8)+dataXYZ[8]);
   data.magnetometer_Y      = ((dataXYZ[9]<<8)+dataXYZ[10]);
   data.magnetometer_Z      = ((dataXYZ[10]<<8)+dataXYZ[11]);
}

/*!
 * Read ID from accelerometer
 *
 * @return ID value as 8-bit number (0x1A for MMA8451Q)
 */
uint32_t FXOS8700CQ::readID(void) {
   uint8_t values[] = {WHO_AM_I};
   i2c->txRx(DEVICE_ADDRESS, values, 1, sizeof(values));
   return values[0];
}

/**
 * Calibrate accelerometer
 */
void FXOS8700CQ::calibrateAccelerometer() {

   uint8_t originalControlReg1Value = readReg(CTRL_REG1);
   uint8_t originalXYXDataConfigValue = readReg(XYZ_DATA_CFG);

   // Make inactive so setting can be modified
   writeReg(CTRL_REG1, 0x00);

   // Clear existing offsets
   writeReg(OFF_X, 0);
   writeReg(OFF_Y, 0);
   writeReg(OFF_Z, 0);

   int mode = (originalXYXDataConfigValue&FXOS8700CQ_XYZ_DATA_CFG_FS_MASK)>>FXOS8700CQ_XYZ_DATA_CFG_FS_OFF;

   static const int calibration2Gs[]     = {4096*8, 2048*8, 1024*8};
   static const int calibrationFactors[] = {8*8, 4*8, 2*8};

   int calibration2G     = calibration2Gs[mode];
   int calibrationFactor = calibrationFactors[mode];

   writeReg(CTRL_REG1,
         FXOS8700CQ_CTRL_REG1_ASLP_RATE(0) | /* 50 Hz auto-sleep rate */
         FXOS8700CQ_CTRL_REG1_DR(2) |        /* 200 Hz update rate    */
         FXOS8700CQ_CTRL_REG1_ACTIVE_MASK);  /* Active     */

   int16_t Xout_Accel_14_bit, Yout_Accel_14_bit, Zout_Accel_14_bit;
   int     Xout_Accel=0, Yout_Accel=0, Zout_Accel=0;

   // Average 8 samples to reduce noise
   for (int i=0; i<8; i++) {
      int status;
      do {
         readAccelerometerXYZ(&status, &Xout_Accel_14_bit, &Yout_Accel_14_bit, &Zout_Accel_14_bit);
      } while ((status & FXOS8700CQ_STATUS_XYZDR_MASK) == 0);
      Xout_Accel += Xout_Accel_14_bit;
      Yout_Accel += Yout_Accel_14_bit;
      Zout_Accel += Zout_Accel_14_bit;
   }

   // Make inactive so setting can be modified
   writeReg(CTRL_REG1, 0x00);

   char X_Accel_offset = -(Xout_Accel / calibrationFactor);                    // Compute X-axis offset correction value
   char Y_Accel_offset = -(Yout_Accel / calibrationFactor);                    // Compute Y-axis offset correction value
   char Z_Accel_offset = -((Zout_Accel - calibration2G) / calibrationFactor);  // Compute Z-axis offset correction value

   writeReg(OFF_X, X_Accel_offset);
   writeReg(OFF_Y, Y_Accel_offset);
   writeReg(OFF_Z, Z_Accel_offset);

   // Restore original settings
   writeReg(CTRL_REG1, originalControlReg1Value);
}

/**
 * Simple calibration of magnetometer
 * Requires user to rotate the board in all dimensions
 *
 * @param time How long to run calibration in seconds
 */
void FXOS8700CQ::calibrateMagnetometer(int time) {

   uint8_t originalMControlReg1Value = readReg(M_CTRL_REG1);
   uint8_t originalControlReg1Value  = readReg(CTRL_REG1);

//   uint8_t buff[6] = {M_OFF_X_MSB};
//   i2c->txRx(DEVICE_ADDRESS, buff, 1, sizeof(buff));
//   printf("FXOS8700CQ, m_offset = 0x%02x%02x, 0x%02x%02x, 0x%02x%02x,\n", buff[0], buff[1], buff[2], buff[3], buff[4], buff[5] );

   // Make inactive so setting can be changed
   writeReg(CTRL_REG1, 0x00);
   writeReg(CTRL_REG1,
         FXOS8700CQ_CTRL_REG1_ASLP_RATE(0) | // 50 Hz auto-sleep rate
         FXOS8700CQ_CTRL_REG1_DR(6) |        // 6.25 Hz update rate (assuming mag only)
         FXOS8700CQ_CTRL_REG1_ACTIVE_MASK);  // Active
   writeReg(M_CTRL_REG1,
         FXOS8700CQ_M_CTRL_REG1_M_ACAL_MASK|       // Magnetic hard-iron offset auto-calibration enabled
         FXOS8700CQ_M_CTRL_REG1_M_OS(7)|           // Maximum over-sample
         FXOS8700CQ_M_CTRL_REG1_M_HMS(MAG_ONLY));  // Magnetometer only

   // Samples @ 6.35 Hz
   waitMS(time*1000);

//   buff[0] = M_OFF_X_MSB;
//   i2c->txRx(DEVICE_ADDRESS, buff, 1, sizeof(buff));
//   printf("FXOS8700CQ, m_offset = 0x%02x%02x, 0x%02x%02x, 0x%02x%02x,\n", buff[0], buff[1], buff[2], buff[3], buff[4], buff[5] );

   // Make inactive so setting can be changed
   writeReg(CTRL_REG1, 0x00);
   // Restore original settings
   writeReg(M_CTRL_REG1, originalMControlReg1Value);
   writeReg(CTRL_REG1, originalControlReg1Value);
}
