#ifndef _TSB_TMR_H_
#define _TSB_TMR_H_

#define TMR0_BASE                   0x40002000
#define TMRX_OFFSET                 0x00000100
#define TMR_MAX                     5
#define TMR_CYCLE                   19        //ns

#define TMR_LOAD                    0x00000000
#define TMR_CNT                     0x00000004
#define   TMR_CTRL_IRQ_ENABLE       (1 << 2)
#define   TMR_CTRL_AUTO_RELOAD      (1 << 1)
#define   TMR_CTRL_TIMER_ENABLE     (1 << 0)
#define TMR_CTRL                    0x00000008
#define TMR_ISR                     0x0000000C
#define   TMR_ISR_CLEAR             (1)
#define TMR_WDT                     0x00000040
#define TMR_RSR                     0x00000050
#define   TMR_RSR_CTRLPEND          (1 << 9)
#define   TMR_RSR_LOADPEND          (1 << 8)
#define   TMR_RSR_CTRLBUSY          (1 << 1)
#define   TMR_RSR_LOADBUSY          (1 << 0)

#endif

