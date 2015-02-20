#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#include <nuttx/irq.h>
#include <nuttx/arch.h>
#include <nuttx/list.h>

#include "up_arch.h"
#include "tsb_tmr.h"
#include "tsb_scm.h"

struct tsb_tmr
{
    uint32_t base;
    uint32_t irqn;
    void (*tmr_isr)(void *data);
    void *data;
    uint32_t cnt;
};

struct tsb_tmr *g_tsb_tmr[TMR_MAX];

static void tmr_clock_enable(int enable)
{
    static int refcnt = 0;

    //TODO manage concurent access
    if (enable) {
        if (!refcnt) {
            tsb_clk_enable(TSB_CLK_TMR);
            tsb_clk_enable(TSB_CLK_TIMER);
            tsb_clk_enable(TSB_CLK_WDT);
            tsb_reset(TSB_RST_TMR);
            tsb_reset(TSB_RST_TIMER);
            tsb_reset(TSB_RST_WDT);
        }
        refcnt++;
    } else {
        refcnt--;
        if (!refcnt) {
            tsb_clk_disable(TSB_CLK_TMR);
            tsb_clk_disable(TSB_CLK_TIMER);
            tsb_clk_disable(TSB_CLK_WDT);
        } else if (refcnt < 0) {
            lldbg("clock disabled too many time!\n");
            refcnt = 0;
        }
    }
}

static uint32_t tmr_read(struct tsb_tmr *tmr, uint32_t reg)
{
    return getreg32(tmr->base + reg);
}

static void tmr_write(struct tsb_tmr *tmr, uint32_t reg, uint32_t val)
{

    while(tmr_read(tmr, TMR_RSR));
    putreg32(val, tmr->base + reg);
}

static void tmr_enable(struct tsb_tmr *tmr)
{
    uint32_t ctrl = tmr_read(tmr, TMR_CTRL);

    ctrl |= TMR_CTRL_TIMER_ENABLE;
    ctrl |= TMR_CTRL_IRQ_ENABLE;
    tmr_write(tmr, TMR_CTRL, ctrl);
}

static void tmr_disable(struct tsb_tmr *tmr)
{
    tmr_write(tmr, TMR_CTRL, 0);
    tmr_write(tmr, TMR_ISR, TMR_ISR_CLEAR);    
}

static int tmr_irq_handler(int irq, void *context)
{
    struct tsb_tmr *tmr;

    tmr = g_tsb_tmr[irq - TSB_IRQ_TMR0];
    if (tmr->tmr_isr)
        tmr->tmr_isr(tmr->data);
    else
        tmr_disable(tmr);
    tmr_write(tmr, TMR_ISR, TMR_ISR_CLEAR);

    return 0;
}

struct tsb_tmr *tmr_alloc(int tmr)
{
    struct tsb_tmr *tsb_tmr;

    tsb_tmr = malloc(sizeof(*tsb_tmr));
    if (!tsb_tmr)
        return NULL;

    tsb_tmr->base = TMR0_BASE + TMRX_OFFSET * tmr;
    tsb_tmr->irqn = TSB_IRQ_TMR0 + tmr;
    g_tsb_tmr[tmr] = tsb_tmr;
    return tsb_tmr;
}

void tmr_free(struct tsb_tmr *tsb_tmr)
{
    free(tsb_tmr);
}

void tmr_setup(struct tsb_tmr *tmr, void (*tmr_isr)(void *data), void *data)
{
    tmr->tmr_isr = tmr_isr;
    tmr->data = data;

    //TODO move it to tmr_enable / tmr_disable
    tmr_clock_enable(1);

    tmr_write(tmr, TMR_ISR, TMR_ISR_CLEAR);
    irq_attach(tmr->irqn, tmr_irq_handler);
    up_enable_irq(tmr->irqn);
}

static inline uint32_t ns_to_cnt(uint32_t ns)
{
    return ns / TMR_CYCLE;
}

static inline uint32_t cnt_to_ns(uint32_t cnt)
{
    return TMR_CYCLE * cnt;
}

void tmr_schedule(struct tsb_tmr *tmr, uint32_t ns, int periodic)
{
    uint32_t ctrl = tmr_read(tmr, TMR_CTRL);
    uint32_t cnt = ns_to_cnt(ns);

    tmr->cnt = cnt;
    tmr_write(tmr, TMR_LOAD, cnt);
    if (periodic)
        ctrl |= TMR_CTRL_AUTO_RELOAD;
    else
        ctrl &= ~TMR_CTRL_AUTO_RELOAD;
    tmr_write(tmr, TMR_CTRL, ctrl);

    tmr_enable(tmr);
}

void tmr_cancel(struct tsb_tmr *tmr)
{
    tmr_disable(tmr);
}

uint32_t tmr_get(struct tsb_tmr *tmr)
{
    return cnt_to_ns(tmr->cnt - tmr_read(tmr, TMR_CNT));
}

static struct tsb_tmr *g_tmr;
static struct timespec g_tsb_timespec;
void timer_1000ms(void *data)
{
    g_tsb_timespec.tv_sec++;
}

void tsb_timer_init(void)
{


    g_tsb_timespec.tv_nsec = 0;
    g_tsb_timespec.tv_sec = 0;
    g_tmr = tmr_alloc(0);
    tmr_setup(g_tmr, timer_1000ms, g_tmr);
    tmr_schedule(g_tmr, 1000000000, 1);
}

void tsb_timer_get(struct timespec *ts)
{
    ts->tv_sec = g_tsb_timespec.tv_sec;
    ts->tv_nsec = tmr_get(g_tmr);
}

