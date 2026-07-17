//-----------------------------------------------------------------------------
// \file    evmc6748_spi.c
// \brief   implementation of a spi driver for C6748.
//
//-----------------------------------------------------------------------------
#define PI 3.14
#include "VSK_6748.h"

//#include "evmc6748_i2c_gpio.h"



//-----------------------------------------------------------------------------
// Private Defines and Macros
//-----------------------------------------------------------------------------
#define TIMER_DIV       (12)
#define TICKS_PER_US    (2)
#ifdef DEBUG
#include "stdio.h"
#endif
#ifndef NULL
#define NULL 0
 #endif
// pinmux defines.
#define PINMUX_SPI0_REG_0     (3)
#define PINMUX_SPI0_MASK_0    (0x0000FFFF)
#define PINMUX_SPI0_VAL_0     (0x00001111)
#define PINMUX_SPI0_REG_1     (4)
#define PINMUX_SPI0_MASK_1    (0x000000F0)
#define PINMUX_SPI0_VAL_1     (0x00000010)
#define PINMUX_SPI1_REG       (5)
#define PINMUX_SPI1_MASK      (0x00FFFFF0)
#define PINMUX_SPI1_VAL       (0x00111110)
#define DEFAULT_CHAR_LEN      (16)
#define NUMONYX_CMD_WREN            (0x06)
#define NUMONYX_CMD_WRDI            (0x04)
#define NUMONYX_CMD_RDID            (0x9F)
#define NUMONYX_CMD_RDSR            (0x05)
#define NUMONYX_CMD_WRSR            (0x01)
#define NUMONYX_CMD_READ            (0x03)
#define NUMONYX_CMD_FAST_READ       (0x0B)
#define NUMONYX_CMD_PAGE_PROG       (0x02)
#define NUMONYX_CMD_SEC_ERASE       (0xD8)
#define NUMONYX_CMD_BULK_ERASE      (0xC7)
#define NUMONYX_CMD_RD_ELEC_SIG     (0xAB)

// numonyx command / response sizes.
#define CMD_SIZE_WREN               (1)
#define CMD_SIZE_RDSR               (1)
#define RESP_SIZE_RDSR              (1)
#define CMD_SIZE_FAST_READ          (5)
#define CMD_SIZE_PAGE_PROG          (4)
#define CMD_SIZE_SEC_ERASE          (4)

// numonyx status register defines.
#define SR_WRITE_PROTECT            (0x80)
#define SR_BLOCK_PROTECT_2          (0x10)
#define SR_BLOCK_PROTECT_1          (0x08)
#define SR_BLOCK_PROTECT_0          (0x04)
#define SR_WRITE_ENABLE_LATCH       (0x02)
#define SR_BUSY                     (0x01)

// numonyx mfr and device ids.
#define NUMONYX_MFR_ID              (0x20)
#define NUMONYX_DEV_ID_TYPE         (0x20)
#define NUMONYX_DEV_ID_CAP          (0x17)

#define MACADDR_ADDR          (SPIFLASH_CHIP_SIZE - SPIFLASH_SECTOR_SIZE)
#define MACADDR_NUM_BYTES     (6)

#define MAX_WRITE_ADDR        (MACADDR_ADDR - 1)
#define PINMUX_GPIO_UI_IO_EXP_REG   (6)
#define PINMUX_GPIO_UI_IO_EXP_MASK  (0x0000000F)
#define PINMUX_GPIO_UI_IO_EXP_VAL   (0x00000008)
#define GPIO_UI_IO_EXP_BANK         (2)
 #define GPIO_UI_IO_EXP_PIN          (7)


#define SPIFLASH_SPI          (SPI1)
#define SPIFLASH_SPI0          (SPI0)
const uint32_t bitval_u32[32] =
{
   0x00000001, 0x00000002, 0x00000004, 0x00000008,
   0x00000010, 0x00000020, 0x00000040, 0x00000080,
   0x00000100, 0x00000200, 0x00000400, 0x00000800,
   0x00001000, 0x00002000, 0x00004000, 0x00008000,
   0x00010000, 0x00020000, 0x00040000, 0x00080000,
   0x00100000, 0x00200000, 0x00400000, 0x00800000,
   0x01000000, 0x02000000, 0x04000000, 0x08000000,
   0x10000000, 0x20000000, 0x40000000, 0x80000000
};
//static uint32_t localWrite(uint32_t in_addr, uint8_t *in_data, uint32_t in_length, uint8_t wait_for_completion);
//static uint32_t localErase(uint32_t in_addr, uint32_t in_length, uint8_t wait_for_completion);
//static void copyAddr(uint32_t addr, uint8_t *buffer);
static uint32_t init_psc(void);
 static uint32_t init_clocks(void);



//us timer starts...

 uint32_t EVMC6748_init(void)
{
   uint32_t rtn = 0;

   // configure power, sysconifg, and clocks.
   rtn = init_psc();
   rtn |= init_clocks();
   
   if (rtn)
      return (ERR_INIT_FAIL);
   else
      return (ERR_NO_ERROR);
}
uint32_t USTIMER_init(void)
{

   TMR0->GPINT_GPEN = GPENO12 | GPENI12;
   TMR0->GPDATA_GPDIR = GPDIRO12 | GPDIRI12;

   // stop and reset timer.
   TMR0->TGCR = 0x00000000;
   TMR0->TCR = 0x00000000;

   // disable interrupts and set emulation to free run.
   TMR0->INTCTLSTAT = 0;
   SETBIT(TMR0->EMUMGT, SOFT | FREE);

   // config timer0 in 32-bit unchained mode.
   // remove timer0 - 3:4 from reset.
   SETBIT(TMR0->TGCR, PRESCALER(TIMER_DIV - 1) | TIMMODE_32BIT_UNCHAINED | TIM34RS );

   // init timer0 - 1:2 period....use full range of counter.
   TMR0->TIM34 = 0x00000000;
   TMR0->PRD34 = 0xFFFFFFFF;

   // start timer0 - 3:4.
   SETBIT(TMR0->TCR, ENAMODE34_CONT);

   
   TMR1->GPINT_GPEN = GPENO12 | GPENI12;
   TMR1->GPDATA_GPDIR = GPDIRO12 | GPDIRI12;

   // stop and reset timer.
   TMR1->TGCR = 0x00000000;
   TMR1->TCR = 0x00000000;

   // disable interrupts and set emulation to free run.
   TMR1->INTCTLSTAT = 0;
   SETBIT(TMR1->EMUMGT, SOFT | FREE);

   // config timer1 in 32-bit unchained mode.
   SETBIT(TMR1->TGCR, PRESCALER(TIMER_DIV - 1) | TIMMODE_32BIT_UNCHAINED);

   // init timer1 - 3:4 period....0 until someone calls delay().
   TMR1->TIM34 = 0x00000000;
   TMR1->PRD34 = 0x00000000;
   
   return (ERR_NO_ERROR);
}

void USTIMER_delay(uint32_t in_delay)
{
   // stop the timer, clear int stat, and clear timer value.
   CLRBIT(TMR1->TGCR, TIM34RS);
   TMR1->TCR = 0x00000000;
   SETBIT(TMR1->INTCTLSTAT, PRDINTSTAT34);
   TMR1->TIM34 = 0x00000000;

   // setup compare time.
   // NOTE: not checking for possible rollover here...do not pass in a
   // value > 0x7FFFFFFF....would result in a much shorter delay than expected.
   TMR1->PRD34 = TICKS_PER_US * in_delay;
   
   // start timer1 - 3:4 to run once up to the period.
   SETBIT(TMR1->TCR, ENAMODE34_ONETIME);
   SETBIT(TMR1->TGCR, TIM34RS);
   
   // wait for the signal that we have hit our period.
   while (!CHKBIT(TMR1->INTCTLSTAT, PRDINTSTAT34))
   {
      asm("nop");
   }

}


void USTIMER_reset(void)
{
   CLRBIT(TMR0->TGCR, TIM34RS);
   TMR0->TIM34 = 0x00000000;
   SETBIT(TMR0->TGCR, TIM34RS);
}


uint32_t USTIMER_get(void)
{
   return (TMR0->TIM34 / TICKS_PER_US);
}

void USTIMER_set(uint32_t in_time)
{
   TMR0->TIM34 = TICKS_PER_US * in_time;
}

uint32_t EVMC6748_initRAM(void)
{
   uint32_t rtn = 0;
   
   // unlock the system config registers and set the ddr 2x clock source.
   SYSCONFIG->KICKR[0] = KICK0R_UNLOCK;
   SYSCONFIG->KICKR[1] = KICK1R_UNLOCK;
   CLRBIT(SYSCONFIG->CFGCHIP[3], CLK2XSRC);
   
   // enable emif3a clock.
   EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_EMIF3A, PSC_ENABLE);
   
   // check if vtp calibration is enabled.
   if (CHKBIT(VTPIO_CTL, 0x00000040))
   {
      // vtp cal disabled, begin cal.

      // enable input buffer and vtp.
      SETBIT(VTPIO_CTL, 0x00004000);
      CLRBIT(VTPIO_CTL, 0x00000040);
      
      // pulse clrz to init vtp cal.
      SETBIT(VTPIO_CTL, 0x00002000);
      CLRBIT(VTPIO_CTL, 0x00002000);
      SETBIT(VTPIO_CTL, 0x00002000);
      
      // poll ready bit to wait for cal to complete.
      while (!CHKBIT(VTPIO_CTL, 0x00008000)) {}
      
      // set lock and power save bits.
      SETBIT(VTPIO_CTL, 0x00000180);
   }

   // config ddr timing.
   DDR->DDRPHYCTL1 = 0x000000C4;
   DDR->SDCR = 0x0893C622;
   DDR->SDCR &= ((DDR->SDCR & 0xFF0FFFFF) | 0x00800000);
   DDR->SDCR = ((DDR->SDCR & 0xFF0FFFFF) | 0x02000000);
   DDR->SDCR &= (~0x00008000);
   
   DDR->SDTIMR1 = 0x20923A89;
   DDR->SDTIMR2 = 0x0015C720;
   DDR->SDCR2 = 0x0;
   DDR->SDRCR = 0x00000492;
   
   // set ddr2 to sync reset.
   SETBIT(DDR->SDRCR, 0xC0000000);
   
   // sync reset the ddr clock.
   EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_EMIF3A, PSC_SYNCRESET);
   
   // enable the clock.
   EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_EMIF3A, PSC_ENABLE);
   
   // disable self refresh.
   CLRBIT(DDR->SDRCR, 0xC0000000);

   if (rtn)
      return (ERR_INIT_FAIL);
   else
      return (ERR_NO_ERROR);
}

void EVMC6748_enableDsp(void)
{
   // power dsp core.
   EVMC6748_lpscTransition(PSC0, DOMAIN0, LPSC_DSP, PSC_ENABLE);

   // wake up dsp core and release from reset.
   SETBIT(PSC0->MDCTL[LPSC_DSP], LRST);
}

void EVMC6748_pinmuxConfig(uint32_t in_reg, uint32_t in_mask, uint32_t in_val)
{
   // unlock the system config registers.
   SYSCONFIG->KICKR[0] = KICK0R_UNLOCK;
   SYSCONFIG->KICKR[1] = KICK1R_UNLOCK;
   
   // make sure the pinmux register is cleared for the mask bits before
   // setting the value.
   CLRBIT(SYSCONFIG->PINMUX[in_reg], in_mask);
   SETBIT(SYSCONFIG->PINMUX[in_reg], in_val);
   
   // lock the system config registers.
   SYSCONFIG->KICKR[0] = KICK0R_LOCK;
   SYSCONFIG->KICKR[1] = KICK1R_LOCK;
}

void EVMC6748_lpscTransition(psc_regs_t *psc, uint32_t in_domain, uint8_t in_module, uint8_t in_next_state)
{
   // spin until existing transitions are done.
   while (CHKBIT(psc->PTSTAT, in_domain)) {}

   // if we are already in the requested state...just return.
   if (CHKBIT(psc->MDSTAT[in_module], MASK_STATE) == in_next_state)
   {
      return;
   }

   // setup the transition...clear the bits before setting the next state.
   CLRBIT(psc->MDCTL[in_module], NEXT);
   SETBIT(psc->MDCTL[in_module], in_next_state);

   // kick off the transition.
   SETBIT(psc->PTCMD, in_domain);

   // spin until transition is done.
   while (CHKBIT(psc->PTSTAT, in_domain)) {}

   while (CHKBIT(psc->MDSTAT[in_module], MASK_STATE) != in_next_state) {}
}


//-----------------------------------------------------------------------------
// helper function to initialize power and sleep config module.
//-----------------------------------------------------------------------------
uint32_t init_psc(void)
{
   //-----------------------------------------
   // PSC0, domain 0 - all modules, always on.
   //-----------------------------------------

   // configure the next state for psc0 modules.
   
   EVMC6748_lpscTransition(PSC0, DOMAIN0, LPSC_EMIFA, PSC_ENABLE);
   EVMC6748_lpscTransition(PSC0, DOMAIN0, LPSC_SPI0, PSC_ENABLE);
   EVMC6748_lpscTransition(PSC0, DOMAIN0, LPSC_MMCSD0, PSC_ENABLE);
   EVMC6748_lpscTransition(PSC0, DOMAIN0, LPSC_AINTC, PSC_ENABLE);
   EVMC6748_lpscTransition(PSC0, DOMAIN0, LPSC_UART0, PSC_ENABLE);
   EVMC6748_lpscTransition(PSC0, DOMAIN0, LPSC_SCR0, PSC_ENABLE);
   EVMC6748_lpscTransition(PSC0, DOMAIN0, LPSC_SCR1, PSC_ENABLE);
   EVMC6748_lpscTransition(PSC0, DOMAIN0, LPSC_SCR2, PSC_ENABLE);

   //-----------------------------------------
   // PSC1, domain 0 - all modules, always on.
   //-----------------------------------------

   // configure the next state for psc1 modules.
   
   EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_USB0, PSC_ENABLE);
   EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_USB1, PSC_ENABLE);
   EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_GPIO, PSC_ENABLE);
   EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_HPI, PSC_ENABLE);
   EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_EMAC, PSC_ENABLE);
   EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_MCASP0, PSC_ENABLE);
//    EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_SATA, PSC_ENABLE);
   EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_VPIF, PSC_ENABLE);
   EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_SPI1, PSC_ENABLE);
   EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_I2C1, PSC_ENABLE);
   EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_UART1, PSC_ENABLE);
   EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_UART2, PSC_ENABLE);
   EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_MCBSP0, PSC_ENABLE);
   EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_MCBSP1, PSC_ENABLE);
   EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_LCDC, PSC_ENABLE);
   EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_PWM, PSC_ENABLE);
   EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_MMCSD1, PSC_ENABLE);
   EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_RPI, PSC_ENABLE);
   
   return (ERR_NO_ERROR);
}

uint32_t init_clocks(void)
{
   uint32_t rtn;
   
   // unlock the system config registers.
   SYSCONFIG->KICKR[0] = KICK0R_UNLOCK;
   SYSCONFIG->KICKR[1] = KICK1R_UNLOCK;
   
   rtn = config_pll0(0,24,1,0,1,11,5);
   rtn |= config_pll1(24,1,0,1,2);

   // enable 4.5 divider PLL and set it as the EMIFA clock source.
//    SETBIT(SYSCONFIG->CFGCHIP[3], DIV4P5ENA | EMA_CLKSRC);
   
   return (rtn);
}

uint32_t config_pll0(uint32_t clkmode, uint32_t pllm, uint32_t postdiv, uint32_t plldiv1, uint32_t plldiv2, uint32_t plldiv3, uint32_t plldiv7)
{
   uint32_t i;
   
   // unlock the system config registers.
   SYSCONFIG->KICKR[0] = KICK0R_UNLOCK;
	SYSCONFIG->KICKR[1] = KICK1R_UNLOCK;

   // unlock pll regs.
   CLRBIT(SYSCONFIG->CFGCHIP[0], PLL0_MASTER_LOCK);
   
   // prepare to enable pll (PLLENSRC must be clear for PLLEN to have effect).
   CLRBIT(PLL0->PLLCTL, PLLENSRC);
   
   // disable external clock source.
   CLRBIT(PLL0->PLLCTL, EXTCLKSRC);
   
   // switch to bypass mode...wait 4 cycles to ensure it switches properly.
   CLRBIT(PLL0->PLLCTL, PLLEN);
   for (i = 0; i < 4; i++) {}
   
   // select clock mode (on-chip oscillator or external).
	CLRBIT(PLL0->PLLCTL, CLKMODE);
	SETBIT(PLL0->PLLCTL, (clkmode << CLKMODE_SHIFT));
   
   // reset the pll.
   CLRBIT(PLL0->PLLCTL, PLLRST);
   
   // disable the pll...set disable bit.
   SETBIT(PLL0->PLLCTL, PLLDIS);
   
   // PLL initialization sequence
   //----------------------------
   // power up the pll...clear power down bit.
   CLRBIT(PLL0->PLLCTL, PLLPWRDN);   
   
   // enable the pll...clear disable bit.
   CLRBIT(PLL0->PLLCTL, PLLDIS);
   
   /*PLL stabilisation time- take out this step , not required here when PLL in bypassmode*/
   for(i = 0; i < PLL_STABILIZATION_TIME; i++) {;}
   
   // program the required multiplier value.
   PLL0->PLLM = pllm;
   
   // program postdiv ratio.
   PLL0->POSTDIV = DIV_ENABLE | postdiv;
   
   // spin until all transitions are complete.
   while (CHKBIT(PLL0->PLLSTAT, GOSTAT)) {}
   
   // program the divisors.
   PLL0->PLLDIV1 = DIV_ENABLE | plldiv1;
   PLL0->PLLDIV2 = DIV_ENABLE | plldiv2;
   PLL0->PLLDIV3 = DIV_ENABLE | plldiv3;
   PLL0->PLLDIV4 = DIV_ENABLE | (((plldiv1 + 1) * 4) - 1);
   PLL0->PLLDIV6 = DIV_ENABLE | plldiv1;
   PLL0->PLLDIV7 = DIV_ENABLE | plldiv7;
   
   // kick off the transitions and spin until they are complete.
   SETBIT(PLL0->PLLCMD, GOSET);
   while (CHKBIT(PLL0->PLLSTAT, GOSTAT)) {}
   
   /*Wait for PLL to reset properly. See PLL spec for PLL reset time - This step is not required here -step11*/
   for(i = 0; i < PLL_RESET_TIME_CNT; i++) {;}   /*128 MXI Cycles*/
   
   // bring pll out of reset and wait for pll to lock.
   SETBIT(PLL0->PLLCTL, PLLRST);
   for (i = 0; i < PLL_LOCK_CYCLES; i++) {}
   
   // exit bypass mode.
   SETBIT(PLL0->PLLCTL, PLLEN);
   
   // lock pll regs.
   SETBIT(SYSCONFIG->CFGCHIP[0], PLL0_MASTER_LOCK);
   
   return (ERR_NO_ERROR);
}

uint32_t config_pll1(uint32_t pllm, uint32_t postdiv, uint32_t plldiv1, uint32_t plldiv2, uint32_t plldiv3)
{
   uint32_t i;
   
   // unlock the system config registers.
   SYSCONFIG->KICKR[0] = KICK0R_UNLOCK;
	SYSCONFIG->KICKR[1] = KICK1R_UNLOCK;
   
   // unlock pll regs.
   CLRBIT(SYSCONFIG->CFGCHIP[3], PLL1_MASTER_LOCK);
   
   // prepare to enable pll (PLLENSRC must be clear for PLLEN to have effect).
   CLRBIT(PLL1->PLLCTL, PLLENSRC);
   CLRBIT(PLL1->PLLCTL, EXTCLKSRC);
   
   // switch to bypass mode...wait 4 cycles to ensure it switches properly.
   CLRBIT(PLL1->PLLCTL, PLLEN);
   for (i = 0; i < 4; i++) {}
   
   // reset the pll.
   CLRBIT(PLL1->PLLCTL, PLLRST);
   
   // disable the pll...set disable bit.
   SETBIT(PLL1->PLLCTL, PLLDIS);
   
   // PLL initialization sequence
   //----------------------------
   // power up the pll...clear power down bit.
   CLRBIT(PLL1->PLLCTL, PLLPWRDN);
   
   // enable the pll...clear disable bit.
   CLRBIT(PLL1->PLLCTL, PLLDIS);
   
   /*PLL stabilisation time- take out this step , not required here when PLL in bypassmode*/
   for(i = 0; i < PLL_STABILIZATION_TIME; i++) {;}
   
   // program the required multiplier value.
   PLL1->PLLM = pllm;
   
   // program postdiv ratio.
   PLL1->POSTDIV = DIV_ENABLE | postdiv;
   
   // spin until all transitions are complete.
   while (CHKBIT(PLL1->PLLSTAT, GOSTAT)) {}
   
   // program the divisors.
   PLL1->PLLDIV1 = DIV_ENABLE | plldiv1;
   PLL1->PLLDIV2 = DIV_ENABLE | plldiv2;
   PLL1->PLLDIV3 = DIV_ENABLE | plldiv3;
   
   // kick off the transitions and spin until they are complete.
   SETBIT(PLL1->PLLCMD, GOSET);
   while (CHKBIT(PLL1->PLLSTAT, GOSTAT)) {}
   
   /*Wait for PLL to reset properly. See PLL spec for PLL reset time - This step is not required here -step11*/
   for(i = 0; i < PLL_RESET_TIME_CNT; i++) {;}
   
   // bring pll out of reset and wait for pll to lock.
   SETBIT(PLL1->PLLCTL, PLLRST);
   for (i = 0; i < PLL_LOCK_CYCLES; i++) {}
   
   // exit bypass mode.
   SETBIT(PLL1->PLLCTL, PLLEN);
   
   // lock pll regs.
   SETBIT(SYSCONFIG->CFGCHIP[3], PLL1_MASTER_LOCK);
   
   return (ERR_NO_ERROR);
}

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
/// timer ends
uint32_t SPI_init(spi_regs_t *spi, spi_config_t *in_config)
{
   uint32_t rtn = ERR_INIT_FAIL;
   
  // enable the psc and config pinmux for the given spi port.
   switch ((uint32_t)spi)
   {
      case SPI0_REG_BASE:
         EVMC6748_lpscTransition(PSC0, DOMAIN0, LPSC_SPI0, PSC_ENABLE);
         EVMC6748_pinmuxConfig(PINMUX_SPI0_REG_0, PINMUX_SPI0_MASK_0, PINMUX_SPI0_VAL_0);
         EVMC6748_pinmuxConfig(PINMUX_SPI0_REG_1, PINMUX_SPI0_MASK_1, PINMUX_SPI0_VAL_1);
         break;
         
      case SPI1_REG_BASE:
         EVMC6748_lpscTransition(PSC1, DOMAIN0, LPSC_SPI1, PSC_ENABLE);
         EVMC6748_pinmuxConfig(PINMUX_SPI1_REG, PINMUX_SPI1_MASK, PINMUX_SPI1_VAL);
         break;
      
      default:
         return (ERR_INIT_FAIL);
   }

   if (in_config != NULL)
   {
      uint32_t prescaler;
      
      // reset spi port.
      spi->SPIGCR0 = 0;
      USTIMER_delay(5);
      SETBIT(spi->SPIGCR0, RESET);
      
      // config master/slave mode.
      if (SPI_MODE_MASTER == in_config->mode)
      {
         // set clkmod and master for master mode.
         spi->SPIGCR1 = CLKMOD | MASTER;
      }
      else if (SPI_MODE_SLAVE == in_config->mode)
      {
         // clear spigcr1 for slave mode.
         spi->SPIGCR1 = 0;
      }
      else
      {
         return (ERR_INIT_FAIL);
      }
      
      // config pin options.
      switch (in_config->pin_option)
      {
         case SPI_3PIN:
            // enable spi SOMI, SIMO, and CLK.
            spi->SPIPC0 = SOMI | SIMO | CLK;
            // config SCS0 as gpio output.
            SETBIT(spi->SPIPC1, SCS0);
            break;

         case SPI_4PIN_CS:
            // enable spi SOMI, SIMO, CLK, and SCS0.
            spi->SPIPC0 = SOMI | SIMO | CLK | SCS0;
            break;

         case SPI_4PIN_EN:
            // enable spi SOMI, SIMO, CLK, and ENA.
            spi->SPIPC0 = SOMI | SIMO | CLK | ENA;
            break;

         case SPI_5PIN:
            // enable spi SOMI, SIMO, CLK, SCS0, and ENA.
            spi->SPIPC0 = SOMI | SIMO | CLK | ENA | SCS0;
            break;

            default:
            return (ERR_INIT_FAIL);
      }
      
      // config the cs active...high or low.
      spi->SPIDEF = 0;
      if (SPI_CS_ACTIVE_LOW == in_config->cs_active)
      {
         // clear csnr for active low and set cs default to 1.
         spi->SPIDAT1 = 0;
         SETBIT(spi->SPIDEF, CSDEF0);
      }
      else if (SPI_CS_ACTIVE_HIGH == in_config-> cs_active)
      {
         // set csnr for active high and set cs default to 0.
         spi->SPIDAT1 = 0;
         SETBIT(spi->SPIDAT1, CSNR);
      }
      else
      {
         return (ERR_INIT_FAIL);
      }
      
      // config spi direction, polarity, and phase.
      spi->SPIFMT0 = 0;
      
      if (SPI_SHIFT_LSB == in_config->shift_dir)
      {
         SETBIT(spi->SPIFMT0, SHIFTDIR);
      }
      
      if (in_config->polarity)
      {
         SETBIT(spi->SPIFMT0, POLARITY);
      }
      
      if (in_config->phase)
      {
         SETBIT(spi->SPIFMT0, PHASE);
      }
      
      // set the prescaler and character length.
      prescaler = (((SYSCLOCK2_HZ / in_config->freq) - 1) & 0xFF);
      #ifdef DEBUG
      printf("desired spi freq: %u\r\n", in_config->freq);
      printf("spi sysclock:     %u\r\n", SYSCLOCK2_HZ);
      printf("prescaler set to: %u\r\n", prescaler);
      #endif
      SETBIT(spi->SPIFMT0, (prescaler << SHIFT_PRESCALE));
      SETBIT(spi->SPIFMT0, (DEFAULT_CHAR_LEN << SHIFT_CHARLEN));
      
      spi->SPIDELAY = (8 << 24) | (8 << 16);

      // disable interrupts.
      spi->SPIINT = 0x00;
      spi->SPILVL = 0x00;
      
      // enable spi.
      SETBIT(spi->SPIGCR1, ENABLE);

      rtn = ERR_NO_ERROR;
   }

   return (rtn);
}
uint32_t Spi_inti(void)
{
   uint32_t rtn = ERR_NO_ERROR;
   spi_config_t spi_config;

   spi_config.mode = SPI_MODE_MASTER;
   spi_config.pin_option = SPI_4PIN_CS;
   spi_config.cs_active = SPI_CS_ACTIVE_LOW;
   spi_config.shift_dir = SPI_SHIFT_MSB;
   spi_config.polarity = 0;
   spi_config.phase = 0;
   spi_config.freq = 15000000;

   rtn = SPI_init(SPIFLASH_SPI0, &spi_config);
   rtn = SPI_init(SPIFLASH_SPI,  &spi_config);

   if (rtn != ERR_NO_ERROR)
   {
      #ifdef DEBUG
      printf("spi flash, spi init error:\t%d\r\n", rtn);
      #endif
      return (ERR_INIT_FAIL);
   }

   return (rtn);
}



   
   

  


