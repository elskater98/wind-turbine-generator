#include "ch.h"
#include "hal.h"
#include "chprintf.h"

static const uint8_t i2c_address = 0x04;
static uint8_t result = 0;

MUTEX_DECL(mtx1);

void lcdPrintf(int x, int y, char text[], int value)
{

  sdPut(&SD1, (uint8_t)0x7C);
  sdPut(&SD1, (uint8_t)0x18);
  sdPut(&SD1, (uint8_t)x);
  chThdSleepMilliseconds(10);

  sdPut(&SD1, (uint8_t)0x7C);
  sdPut(&SD1, (uint8_t)0x19);
  sdPut(&SD1, (uint8_t)y);
  chThdSleepMilliseconds(10);

  chprintf((BaseSequentialStream *)&SD1, text, value);
  chThdSleepMilliseconds(10);
}

void clearScreen()
{
  sdPut(&SD1, (uint8_t)0x7C);
  sdPut(&SD1, (uint8_t)0);
}

static WORKING_AREA(waThread_LCD, 128);
static msg_t Thread_LCD(void *p)
{
  (void)p;
  chRegSetThreadName("SerialPrint");
  while (TRUE)
  {
    sdPut(&SD1, (uint8_t)0x7C);
    sdPut(&SD1, (uint8_t)0x18);
    sdPut(&SD1, (uint8_t)0x20);
    chThdSleepMilliseconds(10);

    sdPut(&SD1, (uint8_t)0x7C);
    sdPut(&SD1, (uint8_t)0x19);
    sdPut(&SD1, (uint8_t)0x20);
    chThdSleepMilliseconds(10);

    //chprintf((BaseSequentialStream *)&SD1, "Iter.: %u", iteration);
    chThdSleepMilliseconds(2000);
  }
  return 0;
}

static WORKING_AREA(waThread_I2C, 512);
static msg_t Thread_I2C(void *p)
{
  (void)p;

  chRegSetThreadName("SerialPrintI2C");

  uint8_t request = 0;
  msg_t status;

  // Some time to allow slaves initialization
  chThdSleepMilliseconds(2000);

  while (TRUE)
  {

    // Request values
    //msg_t i2cMsg = i2cMasterReceiveTimeout(&I2C0, i2c_address, result, 1, MS2ST(1000));
    chMtxLock(&mtx1);
    msg_t i2cMsg = i2cMasterTransmitTimeout(&I2C0, i2c_address, &request, 1, &result, 1, MS2ST(1000));
    chThdSleepMilliseconds(10);
    chMtxUnlock();
    
    lcdPrintf(30, 8, "%u", request);
    lcdPrintf(59, 8, "%u", result);

    /*if (i2cMsg == Q_OK)
    {
      //lcdPrintf(30, 8, "%u", result[0]);
      lcdPrintf(59, 8, "%c", 'O');
    }
    else
    {
      lcdPrintf(59, 8, "%c", 'F');
    }*/

    request++;

    chThdSleepMilliseconds(5000);
    clearScreen();
  }

  return 0;
}

int main(void)
{
  halInit();
  chSysInit();

  // Initialize Serial Port
  sdStart(&SD1, NULL);

  chprintf((BaseSequentialStream *)&SD1, "Main (SD1 started)");

  // Coordinates
  sdPut(&SD1, (uint8_t)0x7C);
  sdPut(&SD1, (uint8_t)0x18);
  sdPut(&SD1, (uint8_t)0x00);
  chThdSleepMilliseconds(10);

  sdPut(&SD1, (uint8_t)0x7C);
  sdPut(&SD1, (uint8_t)0x19);
  sdPut(&SD1, (uint8_t)0x0A);
  chThdSleepMilliseconds(10);

  // LCD Thread
  //chThdCreateStatic(waThread_LCD, sizeof(waThread_LCD), NORMALPRIO, Thread_LCD, NULL);

  // I2C Thread
  I2CConfig i2cConfig;
  i2cStart(&I2C0, &i2cConfig);

  chThdCreateStatic(waThread_I2C, sizeof(waThread_I2C), HIGHPRIO, Thread_I2C, NULL);

  // Blocks until finish
  chThdWait(chThdSelf());

  return 0;
}