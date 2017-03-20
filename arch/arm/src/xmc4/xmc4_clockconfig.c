/****************************************************************************
 * arch/arm/src/xmc4/xmc4_clockconfig.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Authors: Gregory Nutt <gnutt@nuttx.org>
 *
 * Reference: XMC4500 Reference Manual V1.5 2014-07 Microcontrollers.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * May include some logic from sample code provided by Infineon:
 *
 *   Copyright (C) 2011-2015 Infineon Technologies AG. All rights reserved.
 *
 * Infineon Technologies AG (Infineon) is supplying this software for use with
 * Infineon's microcontrollers.  This file can be freely distributed within
 * development tools that are supporting such microcontrollers.
 *
 * THIS SOFTWARE IS PROVIDED AS IS. NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 * INFINEON SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "up_arch.h"
#include "chip/xmc4_scu.h"
#include "xmc4_clockconfig.h"

#include <arch/board/board.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Oscilator reference frequency */

#define FOSCREF               (2500000U)

/* Loop delays at different CPU frequencies */

#define DELAY_CNT_50US_50MHZ  (2500)
#define DELAY_CNT_150US_50MHZ (7500)
#define DELAY_CNT_50US_48MHZ  (2400)
#define DELAY_CNT_50US_72MHZ  (3600)
#define DELAY_CNT_50US_96MHZ  (4800)
#define DELAY_CNT_50US_120MHZ (6000)
#define DELAY_CNT_50US_144MHZ (7200)

/* PLL settings */

#define SCU_PLLSTAT_OSC_USABLE \
  (SCU_PLLSTAT_PLLHV | SCU_PLLSTAT_PLLLV |  SCU_PLLSTAT_PLLSP)

#ifdef BOARD_PLL_CLOCKSRC_XTAL
#  define VCO ((BOARD_XTAL_FREQUENCY / BOARD_PLL_PDIV) * BOARD_PLL_NDIV)
#else /* BOARD_PLL_CLOCKSRC_XTAL */

#  define BOARD_PLL_PDIV  2
#  define BOARD_PLL_NDIV  24
#  define BOARD_PLL_K2DIV 1

#  define VCO ((OFI_FREQUENCY / BOARD_PLL_PDIV) * BOARD_PLL_NDIV)

#endif /* !BOARD_PLL_CLOCKSRC_XTAL */

#define PLL_K2DIV_24MHZ   (VCO / OFI_FREQUENCY)
#define PLL_K2DIV_48MHZ   (VCO / 48000000)
#define PLL_K2DIV_72MHZ   (VCO / 72000000)
#define PLL_K2DIV_96MHZ   (VCO / 96000000)
#define PLL_K2DIV_120MHZ  (VCO / 120000000)

#define CLKSET_VALUE      (0x00000000)
#define SYSCLKCR_VALUE    (0x00010001)
#define CPUCLKCR_VALUE    (0x00000000)
#define PBCLKCR_VALUE     (0x00000000)
#define CCUCLKCR_VALUE    (0x00000000)
#define WDTCLKCR_VALUE    (0x00000000)
#define EBUCLKCR_VALUE    (0x00000003)
#define USBCLKCR_VALUE    (0x00010000)
#define EXTCLKCR_VALUE    (0x01200003)

#if ((USBCLKCR_VALUE & SCU_USBCLKCR_USBSEL) == SCU_USBCLKCR_USBSEL_USBPLL)
#  define USB_DIV         3
#else
#  define USB_DIV         5
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: delay
 ****************************************************************************/

static void delay(uint32_t cycles)
{
  volatile uint32_t i;

  for (i = 0; i < cycles ;++i)
    {
      __asm__ __volatile__ ("nop");
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: xmc4_clock_configure
 *
 * Description:
 *   Called to initialize the XMC4xxx chip.  This does whatever setup is
 *   needed to put the  MCU in a usable state.  This includes the
 *   initialization of clocking using the settings in board.h.
 *
 ****************************************************************************/

void xmc4_clock_configure(void)
{
  uint32_t regval;
  uint32_t bitset;

  /* Disable and clear OSC_HP Oscillator Watchdog, System VCO Lock, USB VCO
   * Lock, and OSC_ULP Oscillator Watchdog traps.
   */

  bitset  = SCU_TRAP_SOSCWDGT | SCU_TRAP_SVCOLCKT | SCU_TRAP_UVCOLCKT |
            SCU_TRAP_ULPWDGT;

  regval  = getreg32(XMC4_SCU_TRAPDIS);
  regval |= bitset;
  putreg32(regval, XMC4_SCU_TRAPDIS);
  putreg32(bitset, XMC4_SCU_TRAPCLR);

#ifdef BOARD_FOFI_CALIBRATION
  /* Enable factory calibration */

  regval  = getreg32(XMC4_SCU_PLLCON0);
  regval |= SCU_PLLCON0_FOTR;
  putreg32(regval, XMC4_SCU_PLLCON0);
#else
  /* Automatic calibration uses the fSTDBY */

  /* Enable HIB domain */
  /* Power up HIB domain if and only if it is currently powered down */

  regval = getreg32(XMC4_SCU_PWRSTAT);
  if ((regval & SCU_PWR_HIBEN) == 0)
    {
      regval  = getreg32(XMC4_SCU_PWRSET);
      regval |= SCU_PWR_HIBEN;
      putreg32(regval, XMC4_SCU_PWRSTAT);

      /* Wait until HIB domain is enabled */

      while((getreg32(XMC4_SCU_PWRSTAT) & SCU_PWR_HIBEN) == 0)
        {
        }
    }

  /* Remove the reset only if HIB domain were in a state of reset */

  regval = getreg32(XMC4_SCU_RSTSTAT);
  if ((regval & SCU_RSTSTAT_HIBRS) != 0)
    {
      regval = putreg32(SCU_RSTCLR_HIBRS, XMC4_SCU_RSTCLR);
      delay(DELAY_CNT_150US_50MHZ);
    }

#ifdef BOARD_STDBY_CLOCKSRC_OSCULP
  /* Enable OSC_ULP */

  regval = getreg32(XMC4_SCU_OSCULCTRL);
  if ((regval & SCU_OSCULCTRL_MODE_MASK) != 0)
    {
      /* Check SCU_MIRRSTS to ensure that no transfer over serial interface
       * is pending.
       */

      while ((getreg32(XMC4_SCU_MIRRSTS) & SCU_MIRRSTS_OSCULCTRL) != 0)
        {
        }

      /* Enable OSC_ULP */

      regval &= ~SCU_OSCULCTRL_MODE_MASK;
      putreg32(regval, XMC4_SCU_OSCULCTRL);

      /* Check if the clock is OK using OSCULP Oscillator Watchdog */

      while ((getreg32(XMC4_SCU_MIRRSTS) & SCU_MIRRSTS_HDCR) != 0)
        {
        }

      regval  = getreg32(XMC4_SCU_HDCR);
      regval |= SCU_HDCR_ULPWDGEN;
      putreg32(regval, XMC4_SCU_HDCR)

      /* Wait till clock is stable */

      do
        {
          /* Check SCU_MIRRSTS to ensure that no transfer over serial interface
           * is pending.
           */

          while ((getreg32(XMC4_SCU_MIRRSTS) & SCU_MIRRSTS_HDCLR) != 0)
            {
            }

          putreg32(SCU_HDCLR_ULPWDG, XMC4_SCU_HDCLR)
          delay(DELAY_CNT_50US_50MHZ);
        }
      while ((getreg32(XMC4_SCU_HDSTAT) & SCU_HDSTAT_ULPWDG) != 0);
    }

  /* Now OSC_ULP is running and can be used */

  while ((getreg32(XMC4_SCU_MIRRSTS) & SCU_MIRRSTS_HDCR) != 0)
    {
    }

  /* Select OSC_ULP as the clock source for RTC and STDBY */

  regval  = getreg32(XMC4_SCU_HDCR);
  regval |= (SCU_HDCR_RCS_ULP | SCU_HDCR_STDBYSEL_ULP);
  putreg32(regval, XMC4_SCU_HDCR)

  regval  = getreg32(XMC4_SCU_TRAPDIS);
  regval &= ~SCU_TRAP_ULPWDGT;
  putreg32(regval, XMC4_SCU_TRAPDIS);

#endif /* BOARD_STDBY_CLOCKSRC_OSCULP */

  /* Enable automatic calibration of internal fast oscillator */

  regval  = getreg32(XMC4_SCU_PLLCON0);
  regval |= SCU_PLLCON0_AOTREN;
  putreg32(regval, XMC4_SCU_PLLCON0);

#endif /* BOARD_FOFI_CALIBRATION  */

  delay(DELAY_CNT_50US_50MHZ);

#ifdef BOARD_ENABLE_PLL

  /* Enable PLL */

  regval  = getreg32(XMC4_SCU_PLLCON0);
  regval &= ~(SCU_PLLCON0_VCOPWD | SCU_PLLCON0_PLLPWD);
  putreg32(regval, XMC4_SCU_PLLCON0);

#ifdef BOARD_PLL_CLOCKSRC_XTAL
  /* Enable OSC_HP */

  if ((getreg32(XMC4_SCU_OSCHPCTRL) & SCU_OSCHPCTRL_MODE_MASK) != 0U)
    {
      regval  = getreg32(XMC4_SCU_OSCHPCTRL);
      regval &= ~(SCU_OSCHPCTRL_MODE_MASK | SCU_OSCHPCTRL_OSCVAL_MASK);
      regval |= ((BOARD_XTAL_FREQUENCY / FOSCREF) - 1) << SCU_OSCHPCTRL_OSCVAL_SHIFT;
      putreg32(regval, XMC4_SCU_OSCHPCTRL);

      /* Select OSC_HP clock as PLL input */

      regval  = getreg32(XMC4_SCU_PLLCON2);
      regval &= ~SCU_PLLCON2_PINSEL;
      putreg32(regval, XMC4_SCU_PLLCON2);

      /* Restart OSC Watchdog */

      regval  = getreg32(XMC4_SCU_PLLCON0);
      regval &= ~SCU_PLLCON0_OSCRES;
      putreg32(regval, XMC4_SCU_PLLCON0);

      /* Wait till OSC_HP output frequency is usable */

      while ((getreg32(XMC4_SCU_PLLSTAT) & SCU_PLLSTAT_OSC_USABLE) != SCU_PLLSTAT_OSC_USABLE)
        {
        }

      regval  = getreg32(SCU_TRAP_SOSCWDGT);
      regval &= ~bitset;
      putreg32(regval, SCU_TRAP_SOSCWDGT);
    }
#else /* BOARD_PLL_CLOCKSRC_XTAL */

  /* Select backup clock as PLL input */

  regval  = getreg32(XMC4_SCU_PLLCON2);
  regval |= SCU_PLLCON2_PINSEL;
  putreg32(regval, XMC4_SCU_PLLCON2);
#endif

  /* Go to bypass the Main PLL */

  regval  = getreg32(XMC4_SCU_PLLCON0);
  regval |= SCU_PLLCON0_VCOBYP;
  putreg32(regval, XMC4_SCU_PLLCON0);

  /* Disconnect Oscillator from PLL */

  regval |= SCU_PLLCON0_FINDIS;
  putreg32(regval, XMC4_SCU_PLLCON0);

  /* Setup divider settings for main PLL */

  regval = (SCU_PLLCON1_NDIV(BOARD_PLL_NDIV) |
            SCU_PLLCON1_K2DIV(PLL_K2DIV_24MHZ) |
            SCU_PLLCON1_PDIV(BOARD_PLL_PDIV));
  putreg32(regval, XMC4_SCU_PLLCON1);

  /* Set OSCDISCDIS */

  regval  = getreg32(XMC4_SCU_PLLCON0);
  regval |= SCU_PLLCON0_OSCDISCDIS;
  putreg32(regval, XMC4_SCU_PLLCON0);

  /* Connect Oscillator to PLL */

  regval  = getreg32(XMC4_SCU_PLLCON0);
  regval &= ~SCU_PLLCON0_FINDIS;
  putreg32(regval, XMC4_SCU_PLLCON0);

  /* Restart PLL Lock detection */

  regval |= SCU_PLLCON0_RESLD;
  putreg32(regval, XMC4_SCU_PLLCON0);

  /* wait for PLL Lock at 24MHz*/

  while ((getreg32(XMC4_SCU_PLLSTAT) & SCU_PLLSTAT_VCOLOCK) == 0)
    {
    }

  /* Disable bypass- put PLL clock back */

  regval  = getreg32(XMC4_SCU_PLLCON0);
  regval &= ~SCU_PLLCON0_VCOBYP;
  putreg32(regval, XMC4_SCU_PLLCON0);

  /* Wait for normal mode */

  while ((getreg32(XMC4_SCU_PLLSTAT) & SCU_PLLSTAT_VCOBYST) != 0)
    {
    }

  regval  = getreg32(XMC4_SCU_TRAPDIS);
  regval &= ~SCU_TRAP_UVCOLCKT;
  putreg32(regval, XMC4_SCU_TRAPDIS);
#endif /* BOARD_ENABLE_PLL */

  /* Before scaling to final frequency we need to setup the clock dividers */

  putreg32(SYSCLKCR_VALUE, XMC4_SCU_SYSCLKCR);
  putreg32(PBCLKCR_VALUE, XMC4_SCU_PBCLKCR);
  putreg32(CPUCLKCR_VALUE, XMC4_SCU_CPUCLKCR);
  putreg32(CCUCLKCR_VALUE, XMC4_SCU_CCUCLKCR);
  putreg32(WDTCLKCR_VALUE, XMC4_SCU_WDTCLKCR);
  putreg32(EBUCLKCR_VALUE, XMC4_SCU_EBUCLKCR);
  putreg32(USBCLKCR_VALUE | USB_DIV, XMC4_SCU_USBCLKCR);
  putreg32(EXTCLKCR_VALUE, XMC4_SCU_EXTCLKCR);

#if BOARD_ENABLE_PLL
  /* PLL frequency stepping...*/
  /* Reset OSCDISCDIS */

  regval  = getreg32(XMC4_SCU_PLLCON0);
  regval &= ~SCU_PLLCON0_OSCDISCDIS;
  putreg32(regval, XMC4_SCU_PLLCON0);

  regval = (SCU_PLLCON1_NDIV(BOARD_PLL_NDIV) |
            SCU_PLLCON1_K2DIV(PLL_K2DIV_48MHZ) |
            SCU_PLLCON1_PDIV(BOARD_PLL_PDIV));
  putreg32(regval, XMC4_SCU_PLLCON1);

  delay(DELAY_CNT_50US_48MHZ);

  regval = (SCU_PLLCON1_NDIV(BOARD_PLL_NDIV) |
            SCU_PLLCON1_K2DIV(PLL_K2DIV_72MHZ) |
            SCU_PLLCON1_PDIV(BOARD_PLL_PDIV));
  putreg32(regval, XMC4_SCU_PLLCON1);

  delay(DELAY_CNT_50US_72MHZ);

  regval = (SCU_PLLCON1_NDIV(BOARD_PLL_NDIV) |
            SCU_PLLCON1_K2DIV(PLL_K2DIV_96MHZ) |
            SCU_PLLCON1_PDIV(BOARD_PLL_PDIV));
  putreg32(regval, XMC4_SCU_PLLCON1);

  delay(DELAY_CNT_50US_96MHZ);

  regval = (SCU_PLLCON1_NDIV(BOARD_PLL_NDIV) |
            SCU_PLLCON1_K2DIV(PLL_K2DIV_120MHZ) |
            SCU_PLLCON1_PDIV(BOARD_PLL_PDIV));
  putreg32(regval, XMC4_SCU_PLLCON1);

  delay(DELAY_CNT_50US_120MHZ);

  regval = (SCU_PLLCON1_NDIV(BOARD_PLL_NDIV) |
            SCU_PLLCON1_K2DIV(BOARD_PLL_K2DIV) |
            SCU_PLLCON1_PDIV(BOARD_PLL_PDIV));
  putreg32(regval, XMC4_SCU_PLLCON1);

  delay(DELAY_CNT_50US_144MHZ);

#endif /* BOARD_ENABLE_PLL */

#ifdef BOARD_ENABLE_USBPLL
  /* Enable USB PLL first */

  regval = getreg32(XMC4_SCU_USBPLLCON);
  regval &= ~(SCU_USBPLLCON_VCOPWD | SCU_USBPLLCON_PLLPWD);
  getreg32(regval, XMC4_SCU_USBPLLCON);

  /* USB PLL uses as clock input the OSC_HP */
  /* check and if not already running enable OSC_HP */

  if ((getreg32(XMC4_SCU_OSCHPCTRL) & SCU_OSCHPCTRL_MODE_MASK) != 0U)
    {
      /* Check if Main PLL is switched on for OSC WDG */

      regval = getreg32(XMC4_SCU_PLLCON0);
      if ((regval & (SCU_PLLCON0_VCOPWD | SCU_PLLCON0_PLLPWD)) != 0)
        {
          /* Enable PLL first */

          regval  = getreg32(XMC4_SCU_PLLCON0);
          regval &= ~(SCU_PLLCON0_VCOPWD | SCU_PLLCON0_PLLPWD);
          putreg32(regval, XMC4_SCU_PLLCON0);
        }

      regval  = getreg32(XMC4_SCU_OSCHPCTRL);
      regval &= ~(SCU_OSCHPCTRL_MODE_MASK | SCU_OSCHPCTRL_OSCVAL_MASK);
      regval |= ((BOARD_XTAL_FREQUENCY / FOSCREF) - 1) << SCU_OSCHPCTRL_OSCVAL_SHIFT;
      putreg32(regval, XMC4_SCU_OSCHPCTRL);

      /* Restart OSC Watchdog */

      regval  = getreg32(XMC4_SCU_PLLCON0);
      regval &= ~SCU_PLLCON0_OSCRES;
      putreg32(regval, XMC4_SCU_PLLCON0);

      /* Wait till OSC_HP output frequency is usable */

      while ((getreg32(XMC4_SCU_PLLSTAT) & SCU_PLLSTAT_OSC_USABLE) != SCU_PLLSTAT_OSC_USABLE)
        {
        }
    }

  /* Setup USB PLL */
  /* Go to bypass the USB PLL */

  regval  = getreg32(XMC4_SCU_USBPLLCON);
  regval |= SCU_USBPLLCON_VCOBYP;
  putreg32(regval, XMC4_SCU_USBPLLCON);

  /* Disconnect Oscillator from USB PLL */

  regval |= SCU_USBPLLCON_FINDIS;
  putreg32(regval, XMC4_SCU_USBPLLCON);

  /* Setup Divider settings for USB PLL */

  regval = (SCU_USBPLLCON_NDIV(BOARD_USB_NDIV) | SCU_USBPLLCON_PDIV(BOARD_USB_PDIV));
  putreg32(regval, XMC4_SCU_USBPLLCON);

  /* Set OSCDISCDIS */

  regval |= SCU_USBPLLCON_OSCDISCDIS;
  putreg32(regval, XMC4_SCU_USBPLLCON);

  /* Connect Oscillator to USB PLL */

  regval &= ~SCU_USBPLLCON_FINDIS;
  putreg32(regval, XMC4_SCU_USBPLLCON);

  /* Restart PLL Lock detection */

  regval |= SCU_USBPLLCON_RESLD;
  putreg32(regval, XMC4_SCU_USBPLLCON);

  /* Wait for PLL Lock */

  while ((getreg32(XMC4_SCU_USBPLLSTAT) & SCU_USBPLLSTAT_VCOLOCK) == 0)
    {
    }

  regval  = getreg32(XMC4_SCU_TRAPDIS);
  regval &= ~SCU_TRAP_UVCOLCKT;
  putreg32(regval, XMC4_SCU_TRAPDIS);
#endif

  /* Enable selected clocks */

  putreg32(CLKSET_VALUE, XMC4_SCU_CLKSET);
}