//-----------------------------------------------------------------------------
// \file    evmc6748.c
// \brief   implementation of initialization functions for C6748.
//
//-----------------------------------------------------------------------------
#include "VSK_6748.h"

#ifndef NULL
#define NULL 0
 #endif
#ifdef DEBUG
#include "stdio.h"
 #endif
//-----------------------------------------------------------------------------
// Private Defines and Macros
//-----------------------------------------------------------------------------
// pinmux defines.
#define PINMUX_MCASP_REG_0        (0)
#define PINMUX_MCASP_MASK_0       (0x00FFFFFF)
#define PINMUX_MCASP_VAL_0        (0x00111111)
#define PINMUX_MCASP_REG_2        (2)
#define PINMUX_MCASP_MASK_2       (0xFFFFFFFF)
#define PINMUX_MCASP_VAL_2        (0x11111111)
//-----------------------------------------------------------------------------
// Private Defines and Macros
//-----------------------------------------------------------------------------
#define TIMER_DIV       (12)
#define TICKS_PER_US    (2)
#define I2C_PORT_AIC3106         (I2C0)
//-----------------------------------------------------------------------------
// Private Defines and Macros
//-----------------------------------------------------------------------------
#define I2C_PORT_GPIO         (I2C0)
#define I2C_GPIO_PIN_MAX      (16)
// config input/output pins (1 -> input, 0 -> output).
#define I2C_GPIO_CONFIG0_EX   (0x3F)
#define I2C_GPIO_CONFIG1_EX   (0xFF)
#define I2C_GPIO_CONFIG0_UI   (0x0F)
#define I2C_GPIO_CONFIG1_UI   (0xFF)
// TCA6416 command byte defines.
#define CMD_BYTE_INPUT0       (0x00)
#define CMD_BYTE_INPUT1       (0x01)
#define CMD_BYTE_OUTPUT0      (0x02)
#define CMD_BYTE_OUTPUT1      (0x03)
#define CMD_BYTE_POLARITY0    (0x04)
#define CMD_BYTE_POLARITY1    (0x05)
#define CMD_BYTE_CONFIG0      (0x06)
#define CMD_BYTE_CONFIG1      (0x07)
#define PINMUX_GPIO_UI_IO_EXP_REG   (6)
#define PINMUX_GPIO_UI_IO_EXP_MASK  (0x0000000F)
#define PINMUX_GPIO_UI_IO_EXP_VAL   (0x00000008)
#define GPIO_UI_IO_EXP_BANK         (2)
#define GPIO_UI_IO_EXP_PIN          (7)
#define PINMUX_I2C0_REG       (4)
#define PINMUX_I2C0_MASK      (0x0000FF00)
#define PINMUX_I2C0_VAL       (0x00002200)
#define PINMUX_I2C1_REG       (4)
#define PINMUX_I2C1_MASK      (0x00FF0000)
#define PINMUX_I2C1_VAL       (0x00440000)
// i2c bus timeout.
#define I2C_TIMEOUT           (500000)
//-----------------------------------------------------------------------------
// Global Variable Initializations
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Static Variable Declarations
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Function Prototypes
//-----------------------------------------------------------------------------
//static uint32_t init_psc(void);
//static uint32_t init_clocks(void);
static i2c_clk_e g_clock_rate;
//static uint32_t testAudioLineOut(void);
static uint32_t testAudioLineIn(void);


//-----------------------------------------------------------------------------
// prints a chunk of flash data in a readable format.
//-----------------------------------------------------------------------------
#ifdef DEBUG
void UTIL_printMem(uint32_t begin_addr, uint8_t *buffer, uint32_t length, uint8_t continuation)
{
#define BYTES_PER_LINE  16
   uint32_t i, j, line_end;

   if (!continuation)
   {
      printf("\r\n\r\nPrint Data\r\n");
      printf("----------\r\n");

      // print idices across the top.
      printf("address     ");
      for (i = 0; i < BYTES_PER_LINE; i++)
      {
         printf("%02X ", i);
      }
      printf("\r\n");
   }

   // print data.
   for (i = 0; i < length; i += BYTES_PER_LINE)
   {
      if (length > (i + BYTES_PER_LINE))
      {
         line_end = (i + BYTES_PER_LINE);
      }
      else
      {
         line_end = length;
      }

      printf("\n%08X    ", (begin_addr + i));
      for (j = i; j < line_end; j++)
         printf("%02X ", buffer[j]);
   }

   printf("\r\n");
}
#endif

//-----------------------------------------------------------------------------
// looks for the UI gpio expander to see if UI board is attached.
//-----------------------------------------------------------------------------
uint8_t UTIL_isUIBoardAttached(void)
{
   if (I2CGPIO_init(I2C_ADDR_GPIO_UI) == ERR_NO_ERROR)
      return (1);
   else
      return (0);
}


uint32_t MCASP_init(void)
{
   // enable the psc and config pinmux for mcasp.
   EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_MCASP0, PSC_ENABLE);
   EVMC6748_pinmuxConfig(PINMUX_MCASP_REG_0, PINMUX_MCASP_MASK_0, PINMUX_MCASP_VAL_0);
   EVMC6748_pinmuxConfig(PINMUX_MCASP_REG_2, PINMUX_MCASP_MASK_2, PINMUX_MCASP_VAL_2);

   // reset mcasp.
   MCASP->GBLCTL  = 0;

   // configure receive registers.
   MCASP->RMASK      = 0xFFFFFFFF;
   MCASP->RFMT       = 0x00008078;
   MCASP->AFSRCTL    = 0x00000112;
   MCASP->ACLKRCTL   = 0x000000AF;
   MCASP->AHCLKRCTL  = 0x00000000;
   MCASP->RTDM       = 0x00000003;
   MCASP->RINTCTL    = 0x00000000;
   MCASP->RCLKCHK    = 0x00FF0008;

   // configure transmit registers.
   MCASP->XMASK      = 0xFFFFFFFF;
   MCASP->XFMT       = 0x00008078;
   MCASP->AFSXCTL    = 0x00000112;
   MCASP->ACLKXCTL   = 0x000000AF;
   MCASP->AHCLKXCTL  = 0x00000000;
   MCASP->XTDM       = 0x00000003;
   MCASP->XINTCTL    = 0x00000000;
   MCASP->XCLKCHK    = 0x00FF0008;

   // config serializers (11 = xmit, 12 = rcv).
 //  MCASP->SRCTL11    = 0x000D;
 //  MCASP->SRCTL12    = 0x000E;
   MCASP->SRCTL3    = 0x000D;
   MCASP->SRCTL4    = 0x000E;

   // config pin function and direction.
   MCASP->PFUNC      = 0;
  //  MCASP->PDIR       = 0x14000800;

   MCASP->PDIR       = 0x14000008;

   //
   MCASP->DITCTL     = 0x00000000;
   MCASP->DLBCTL     = 0x00000000;
   MCASP->AMUTE      = 0x00000000;

   MCASP->XSTAT = 0x0000FFFF;        // Clear all
   MCASP->RSTAT = 0x0000FFFF;        // Clear all

   return (ERR_NO_ERROR);
}

//-----------------------------------------------------------------------------
// Private Function Definitions
//-----------------------------------------------------------------------------
uint32_t AIC3106_init(void)
{
   // select page 0 and reset codec.
   AIC3106_writeRegister(AIC3106_REG_PAGESELECT, 0);
   AIC3106_writeRegister(AIC3106_REG_RESET, 0x80);

   // config codec regs. please see AIC3106 documentation for explination.
   // Document Num: TLV320AIC3106
   AIC3106_writeRegister(3, 0x22);
   AIC3106_writeRegister(4, 0x20);
   AIC3106_writeRegister(5, 0x6E);
   AIC3106_writeRegister(6, 0x23);
   AIC3106_writeRegister(7, 0x0A);
   AIC3106_writeRegister(8, 0x00);
   AIC3106_writeRegister(9, 0x00);
   AIC3106_writeRegister(10, 0x00);
   AIC3106_writeRegister(15, 0x17);
   AIC3106_writeRegister(16, 0x17);
   AIC3106_writeRegister(17, 0x0F);
   AIC3106_writeRegister(18, 0xF0);
   AIC3106_writeRegister(19, 0x7C);
   AIC3106_writeRegister(22, 0x7C);
   AIC3106_writeRegister(25, 0x40);
   AIC3106_writeRegister(27, 0);
   AIC3106_writeRegister(30, 0);
   AIC3106_writeRegister(37, 0xE0);
   AIC3106_writeRegister(38, 0x10);
   AIC3106_writeRegister(43, 0);
   AIC3106_writeRegister(44, 0);
   AIC3106_writeRegister(47, 0x80);
   AIC3106_writeRegister(51, 0x09); // 51           HPLOUT Output         <- [Mute=OFF][Power=ON]
   AIC3106_writeRegister(58, 0);
   AIC3106_writeRegister(64, 0x80); // 64 DAC_R1 to HPROUT Volume         <- [Routed]
   AIC3106_writeRegister(65, 0x09); // 65           HPROUT Output         <- [Mute=OFF][Power=ON]
   AIC3106_writeRegister(72, 0);
   AIC3106_writeRegister(82, 0x80);
   AIC3106_writeRegister(86, 0x09);
   AIC3106_writeRegister(92, 0x80);
   AIC3106_writeRegister(93, 0x09);
   AIC3106_writeRegister(101, 0x01);
   AIC3106_writeRegister(102, 0);
 //  AIC3106_writeRegister(43, 0x28);           //turn down the L DAC gain
 //  AIC3106_writeRegister(44, 0x28);           //turn down the R DAC gain

   return (ERR_NO_ERROR);
}

//-----------------------------------------------------------------------------
// /brief Read data from a register on the AIC3106.
//
// /param uint8_t in_reg_addr: The address of the register to be read from.
//
// /param uint8_t * dest_buffer: Pointer to buffer to store retrieved data.
//
// /return uint32_t ERR_NO_ERROR on sucess
//
//-----------------------------------------------------------------------------
uint32_t AIC3106_readRegister(uint8_t in_reg_addr, uint8_t *dest_buffer)
{
   uint32_t rtn;

   // write the register address that we want to read.
   rtn = I2C_write(I2C_PORT_AIC3106, I2C_ADDR_AIC3106, &in_reg_addr, 1, SKIP_STOP_BIT_AFTER_WRITE);
   if (rtn != ERR_NO_ERROR)
      return (rtn);

   // clock out the register data.
   rtn = I2C_read(I2C_PORT_AIC3106, I2C_ADDR_AIC3106, dest_buffer, 1, SKIP_BUSY_BIT_CHECK);

   return (rtn);
}

//-----------------------------------------------------------------------------
// /brief Write a register on the AIC3106.
//
// /param uint8_t in_reg_addr: The address of the register to be written to.
//
// /param uint8_t data: Data to be written to the register
//
// /return uint32_t ERR_NO_ERROR on sucess
//
//-----------------------------------------------------------------------------
uint32_t AIC3106_writeRegister(uint8_t in_reg_addr, uint8_t in_data)
{
   uint32_t rtn;
   uint8_t i2c_data[2];

   i2c_data[0] = in_reg_addr;
   i2c_data[1] = in_data;

   // write the register that we want to read.
   rtn = I2C_write(I2C_PORT_AIC3106, I2C_ADDR_AIC3106, i2c_data, 2, SET_STOP_BIT_AFTER_WRITE);

   return (rtn);
}

//-----------------------------------------------------------------------------
// Private Function Definitions
//-----------------------------------------------------------------------------


uint32_t I2CGPIO_init(uint16_t in_addr)
{
   uint32_t rtn = ERR_INVALID_PARAMETER;
   uint8_t i2c_data[3];

   if ((I2C_ADDR_GPIO_EX == in_addr) || (I2C_ADDR_GPIO_UI == in_addr))
   {
      // make sure polarity is not inverted.
      i2c_data[0] = CMD_BYTE_POLARITY0;
      i2c_data[1] = 0;
      i2c_data[2] = 0;
      rtn = I2C_write(I2C_PORT_GPIO, in_addr, i2c_data, 3, SET_STOP_BIT_AFTER_WRITE);
      if (rtn != ERR_NO_ERROR)
         return (rtn);

      // set config regs on I/O expander.
      if (I2C_ADDR_GPIO_EX == in_addr)
      {
         i2c_data[0] = CMD_BYTE_CONFIG0;
         i2c_data[1] = I2C_GPIO_CONFIG0_EX;
         i2c_data[2] = I2C_GPIO_CONFIG1_EX;
      }
      else
      {
         i2c_data[0] = CMD_BYTE_CONFIG0;
         i2c_data[1] = I2C_GPIO_CONFIG0_UI;
         i2c_data[2] = I2C_GPIO_CONFIG1_UI;
      }
      rtn = I2C_write(I2C_PORT_GPIO, in_addr, i2c_data, 3, SET_STOP_BIT_AFTER_WRITE);
   }

   return (rtn);
}

//-----------------------------------------------------------------------------
// \brief   get gpio input from one pin of the i2c I/O expander.
//
// \param   uint16_t in_addr - desired expander i2c address.
//
// \param   uint8_t in_pin_num - pin on expander to be read.
//
// \param   uint8_t *data - gpio data from expander
//                            0 -> pin is clear
//                            1 -> pin is set
//
// \return  uint32_t
//    ERR_NO_ERROR - input in bounds...gpio state returned in data.
//    ERR_INVALID_PARAMETER - input out of bounds.
//    else - something happened with i2c comm.
//-----------------------------------------------------------------------------
uint32_t I2CGPIO_getInput(uint16_t in_addr, uint8_t in_pin_num, uint8_t *data)
{
   uint32_t rtn = ERR_INVALID_PARAMETER;

   // check address and pin number.
   if (((I2C_ADDR_GPIO_EX == in_addr) || (I2C_ADDR_GPIO_UI == in_addr)) &&
      (in_pin_num < I2C_GPIO_PIN_MAX) &&
      (data != NULL))
   {
      uint8_t i2c_data;
      uint8_t gpio_bit = 0;

      // set command byte to read appropriate input.
      if (in_pin_num < 8)
      {
         i2c_data = CMD_BYTE_INPUT0;
         gpio_bit = 1 << in_pin_num;
      }
      else
      {
         i2c_data = CMD_BYTE_INPUT1;
         gpio_bit = 1 << (in_pin_num - 8);
      }

      // send i2c command.
      rtn = I2C_write(I2C_PORT_GPIO, in_addr, &i2c_data, 1, SKIP_STOP_BIT_AFTER_WRITE);
      if (rtn != ERR_NO_ERROR)
         return (rtn);

      // read the gpio data.
      rtn = I2C_read(I2C_PORT_GPIO, in_addr, &i2c_data, 1, SKIP_BUSY_BIT_CHECK);
      if (rtn != ERR_NO_ERROR)
         return (rtn);

      // check the input pin value and set var.
      if (i2c_data & gpio_bit)
         *data = 1;
      else
         *data = 0;
   }

   return (rtn);
}

//-----------------------------------------------------------------------------
// \brief   get gpio input from all pins of the i2c I/O expander.
//
// \param   uint16_t in_addr - desired expander i2c address.
//
// \param   uint16_t *data - gpio data from expander.
//
// \return  uint32_t
//    ERR_NO_ERROR - input in bounds...gpio state returned in data.
//    ERR_INVALID_PARAMETER - input out of bounds.
//    else - something happened with i2c comm.
//-----------------------------------------------------------------------------
uint32_t I2CGPIO_getInputAll(uint16_t in_addr, uint16_t *data)
{
   uint32_t rtn = ERR_INVALID_PARAMETER;

   if ((I2C_ADDR_GPIO_EX == in_addr) || (I2C_ADDR_GPIO_UI == in_addr) &&
      (data != NULL))
   {
      uint8_t i2c_data;

      // send i2c command to read input0.
      i2c_data = CMD_BYTE_INPUT0;
      rtn = I2C_write(I2C_PORT_GPIO, in_addr, &i2c_data, 1, SKIP_STOP_BIT_AFTER_WRITE);
      if (rtn != ERR_NO_ERROR)
         return (rtn);

      // read the gpio data for input0.
      rtn = I2C_read(I2C_PORT_GPIO, in_addr, &i2c_data, 1, SKIP_BUSY_BIT_CHECK);
      if (rtn != ERR_NO_ERROR)
         return (rtn);

      // copy gpio data into var.
      *data = i2c_data;

      // send i2c command to read input1.
      i2c_data = CMD_BYTE_INPUT1;
      rtn = I2C_write(I2C_PORT_GPIO, in_addr, &i2c_data, 1, SKIP_STOP_BIT_AFTER_WRITE);
      if (rtn != ERR_NO_ERROR)
         return (rtn);

      // read the gpio data for input1.
      rtn = I2C_read(I2C_PORT_GPIO, in_addr, &i2c_data, 1, SKIP_BUSY_BIT_CHECK);
      if (rtn != ERR_NO_ERROR)
         return (rtn);

      // copy gpio data into var.
      *data += (i2c_data << 8);
   }

   return (rtn);
}

//-----------------------------------------------------------------------------
// \brief   set gpio output for one pin of the i2c I/O expander.
//
// \param   uint16_t in_addr - desired expander i2c address.
//
// \param   uint8_t in_pin_num - pin on expander to be read.
//
// \param   uint16_t in_val - 0/1 to set or clear the pin.
//
// \return  uint32_t
//    ERR_NO_ERROR - pin set successfully.
//    ERR_INVALID_PARAMETER - invalid pin number.
//    else - something happened with i2c comm.
//-----------------------------------------------------------------------------
uint32_t I2CGPIO_setOutput(uint16_t in_addr, uint8_t in_pin_num, uint16_t in_val)
{
   uint32_t rtn = ERR_INVALID_PARAMETER;

   if (((I2C_ADDR_GPIO_EX == in_addr) || (I2C_ADDR_GPIO_UI == in_addr)) &&
      (in_pin_num < I2C_GPIO_PIN_MAX))
   {
      uint8_t i2c_data[2];
      uint8_t gpio_bit = 0;

      // set command byte to read appropriate output, so we do not change
      // any data that we do not want to.
      if (in_pin_num < 8)
      {
         i2c_data[0] = CMD_BYTE_OUTPUT0;
         gpio_bit = 1 << in_pin_num;
      }
      else
      {
         i2c_data[0] = CMD_BYTE_OUTPUT1;
         gpio_bit = 1 << (in_pin_num - 8);
      }

      // send i2c command.
      rtn = I2C_write(I2C_PORT_GPIO, in_addr, i2c_data, 1, SKIP_STOP_BIT_AFTER_WRITE);
      if (rtn != ERR_NO_ERROR)
         return (rtn);

      // read the gpio data.
      rtn = I2C_read(I2C_PORT_GPIO, in_addr, &i2c_data[1], 1, SKIP_BUSY_BIT_CHECK);
      if (rtn != ERR_NO_ERROR)
         return (rtn);

      // update the data to set/clr bit for pin num.
      if (in_val)
         SETBIT(i2c_data[1], gpio_bit);
      else
         CLRBIT(i2c_data[1], gpio_bit);

      // write the gpio data back to the I/O expander.
      rtn = I2C_write(I2C_PORT_GPIO, in_addr, i2c_data, 2, SET_STOP_BIT_AFTER_WRITE);
   }

   return (rtn);
}

//-----------------------------------------------------------------------------
// \brief   set gpio output for all pins of the i2c I/O expander.
//
// \param   uint16_t in_addr - desired expander i2c address.
//
// \param   uint16_t in_val - pattern data to set I/O expander pins.
//
// \return  uint32_t
//    ERR_NO_ERROR - pins set successfully.
//    else - something happened with i2c comm.
//-----------------------------------------------------------------------------
uint32_t I2CGPIO_setOutputAll(uint16_t in_addr, uint16_t in_val)
{
   uint32_t rtn = ERR_INVALID_PARAMETER;

   if ((I2C_ADDR_GPIO_EX == in_addr) || (I2C_ADDR_GPIO_UI == in_addr))
   {
      uint8_t i2c_data[3];

      // load up the array with the cmd and input data.
      i2c_data[0] = CMD_BYTE_OUTPUT0;
      i2c_data[1] = (uint8_t) (in_val & 0x00FF);
      i2c_data[2] = (uint8_t) (in_val >> 8);

      // write the gpio data to the I/O expander.
      rtn = I2C_write(I2C_PORT_GPIO, in_addr, i2c_data, 3, SET_STOP_BIT_AFTER_WRITE);
   }

   return (rtn);
}


uint32_t I2C_init(i2c_regs_t *i2c, i2c_clk_e in_clock_rate)
{
   // set the pinmux for the given i2c port.
   switch ((uint32_t)i2c)
   {
      case I2C0_REG_BASE:
         EVMC6748_pinmuxConfig(PINMUX_I2C0_REG, PINMUX_I2C0_MASK, PINMUX_I2C0_VAL);
         break;

      case I2C1_REG_BASE:
         EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_I2C1, PSC_ENABLE);
         EVMC6748_pinmuxConfig(PINMUX_I2C1_REG, PINMUX_I2C1_MASK, PINMUX_I2C1_VAL);
         break;

      default:
         return (ERR_INIT_FAIL);
   }

   // set global clock rate for future use.
   g_clock_rate = in_clock_rate;

   // put i2c in reset.
   i2c->ICMDR = 0;

   // configure clocks.
   // set prescaler for ~8MHz interal i2c clock.
   i2c->ICPSC = 2;

   switch (in_clock_rate)
   {
      // set prescaler and clock dividers to precomputed values for
      // input clock rate.
      case I2C_CLK_100K:
         i2c->ICCLKL = 35;
         i2c->ICCLKH = 35;
         break;

      case I2C_CLK_400K:
         i2c->ICCLKL = 5;
         i2c->ICCLKH = 5;
         break;
   }

   // release i2c from reset.
   SETBIT(i2c->ICMDR, IRS);

   return (ERR_NO_ERROR);
}

//-----------------------------------------------------------------------------
// \brief   read data from i2c bus.
//
// \param   i2c_regs_t *i2c - pointer to reg struct for the desired i2c port.
//
// \param   uint16_t in_addr - i2c address to read from.
//
// \param   uint8_t *dest_buffer - pointer to memory to copy the data being received.
//
// \param   uint16_t in_length - number of bytes to receive.
//
// \return  uint32_t
//    ERR_NO_ERROR - input in bounds, data received.
//    ERR_INVALID_PARAMETER - null pointers.
//-----------------------------------------------------------------------------
uint32_t I2C_read(i2c_regs_t *i2c, uint16_t in_addr, uint8_t *dest_buffer, uint16_t in_length, uint8_t chk_busy)
{
   uint32_t rtn = ERR_INVALID_PARAMETER;

   if ((i2c != NULL) && (dest_buffer != NULL))
   {
      uint32_t cnt = 0;
      uint16_t i;

      // wait for bus to be clear...we may want to skip this depending on which
      // device we are talking to. see device datasheets for more info.
      USTIMER_delay(1000);
      if (chk_busy)
         while (CHKBIT(i2c->ICSTR, BB)) {}

      // set byte count and slave address.
      i2c->ICCNT = in_length;
      i2c->ICSAR = in_addr;

      // configure i2c for master receive mode and release from reset.
      i2c->ICMDR = STT | MST | ICMDR_FREE | IRS;

      // receive data one byte at a time.
      for (i = 0; i < in_length; i++)
      {
         // do not want to send an ack on last byte.
         if (i == (in_length - 1))
         {
            SETBIT(i2c->ICMDR, NACKMOD);
         }

         // wait for data to be received.
         cnt = 0;
         do
         {
            if (cnt++ > I2C_TIMEOUT)
            {
               // timed out waiting for data...reinit and return error.
               I2C_init(i2c, g_clock_rate);
               return (ERR_TIMEOUT);
            }
         } while (!CHKBIT(i2c->ICSTR, ICRRDY));

         dest_buffer[i] = i2c->ICDRR;
      }

      // send stop condition.
      SETBIT(i2c->ICMDR, STP);

      rtn = ERR_NO_ERROR;
   }

   return (rtn);
}

//-----------------------------------------------------------------------------
uint32_t I2C_write(i2c_regs_t *i2c, uint16_t in_addr, uint8_t *src_buffer, uint16_t in_length, uint8_t set_stop)
{
   uint32_t rtn = ERR_INVALID_PARAMETER;

   if ((i2c != NULL) && (src_buffer != NULL))
   {
      uint32_t cnt = 0;
      uint16_t i;

      // wait for bus to be clear.
      USTIMER_delay(1000);
      while (CHKBIT(i2c->ICSTR, BB)) {}

      // set byte count and slave address.
      i2c->ICCNT = in_length;
      i2c->ICSAR = in_addr;

      // configure i2c for master transmit mode and release from reset.
      i2c->ICMDR = STT | MST | ICMDR_FREE | TRX | IRS;

      USTIMER_delay(10);

      // transmit data one byte at a time.
      for (i = 0; i < in_length; i++)
      {
         i2c->ICDXR = src_buffer[i];

         // wait for data to be copied to shift register.
         cnt = 0;
         do
         {
            if (cnt++ > I2C_TIMEOUT)
            {
               // timed out waiting for data...reinit and return error.
               I2C_init(i2c, g_clock_rate);
               return (ERR_TIMEOUT);
            }
         } while (!CHKBIT(i2c->ICSTR, ICXRDY));
      }


      if (set_stop)
         SETBIT(i2c->ICMDR, STP);

      rtn = ERR_NO_ERROR;
   }

   return (rtn);
}



uint32_t testAudioLineIn(void)
{
   uint32_t rtn = ERR_NO_ERROR;

   SETBIT(MCASP->XGBLCTL, XHCLKRST);
   while (!CHKBIT(MCASP->XGBLCTL, XHCLKRST)) {}
   SETBIT(MCASP->RGBLCTL, RHCLKRST);
   while (!CHKBIT(MCASP->RGBLCTL, RHCLKRST)) {}

   SETBIT(MCASP->XGBLCTL, XCLKRST);
   while (!CHKBIT(MCASP->XGBLCTL, XCLKRST)) {}
   SETBIT(MCASP->RGBLCTL, RCLKRST);
   while (!CHKBIT(MCASP->RGBLCTL, RCLKRST)) {}

   SETBIT(MCASP->XGBLCTL, XSRCLR);
   while (!CHKBIT(MCASP->XGBLCTL, XSRCLR)) {}
   SETBIT(MCASP->RGBLCTL, RSRCLR);
   while (!CHKBIT(MCASP->RGBLCTL, RSRCLR)) {}

   /* Write a 0, so that no underrun occurs after releasing the state machine */
   MCASP->XBUF3 = 0;

   SETBIT(MCASP->XGBLCTL, XSMRST);
   while (!CHKBIT(MCASP->XGBLCTL, XSMRST)) {}
   SETBIT(MCASP->RGBLCTL, RSMRST);
   while (!CHKBIT(MCASP->RGBLCTL, RSMRST)) {}

   SETBIT(MCASP->XGBLCTL, XFRST);
   while (!CHKBIT(MCASP->XGBLCTL, XFRST)) {}
   SETBIT(MCASP->RGBLCTL, RFRST);
   while (!CHKBIT(MCASP->RGBLCTL, RFRST)) {}
 while(!CHKBIT(MCASP->SRCTL3, XRDY)) {}
   MCASP->XBUF3 = 0;
   return (rtn);
}

// audio codec interface using MCASP

int main(void)
{

   int32_t dat;

  USTIMER_init();

  I2C_init(I2C0, I2C_CLK_400K);

   MCASP_init();

   AIC3106_init();

   testAudioLineIn();

  while(1) {

       while (!CHKBIT(MCASP->SRCTL3, XRDY)) {}

        dat = MCASP->XBUF4;

        MCASP->XBUF3 = dat * 3;

         }



}

