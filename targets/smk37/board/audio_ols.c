/* SPDX-License-Identifier: GPL-3.0-only WITH GPL-3.0-linking-exception
 * Copyright (c) 2026 珠海笙科智能科技有限公司 (M-VAVE): the ALNK0 driver and the output mix, from smk37_open
 * app/board/audio.c and app/audio_output.c.
 * Copyright (c) 2026 Techmelec (OpenVALive): the adaptation.
 *
 * OpenVALive - SMK-37 audio: every 128 frames (2.9 ms) the interrupt asks the app's render (instead of
 * M-VAVE's instrument), then mixes the computer's USB audio in and feeds USB recording, as smk37_open does.
 * This file runs from internal RAM (FM1_AUDIO_CODE): the SDK pauses it (fm1_audio_suspend) around flash writes.
 * The app's render lives in the flash, more than a direct call away: it is called through a pointer. */
#include "fm1_audio.h"
#include "fm1_gpio.h"
#include "fm1_audio_io.h"
#include "../glue.h"
#include <string.h>

#define DMA_ALIGNED __attribute__((aligned(32)))
static int32_t receive[2][FM1_BLOCK_WORDS] DMA_ALIGNED;
static int32_t transmit[2][FM1_BLOCK_WORDS] DMA_ALIGNED;
static int32_t rendered[FM1_BLOCK_WORDS] DMA_ALIGNED;
static int32_t usb_pcm[FM1_BLOCK_WORDS] DMA_ALIGNED;
static struct fm1_audio_stats statistics;
static uint32_t block_ticks, previous_tick;
static uint8_t initialized, previous_valid;
static volatile unsigned mute_depth;
static volatile smk_render_fn app_render_fn;
static volatile uint32_t master_q12 = 2867;   /* 70 % until the app sets it */
static volatile uint32_t last_used;          /* timer ticks the last block took */

void smk_audio_set_render(smk_render_fn fn) { app_render_fn = fn; }
void smk_audio_set_master(uint32_t q12) { master_q12 = q12 > 4096U ? 4096U : q12; }
/* computed here, in the control task: no division in the interrupt */
uint32_t smk_audio_cpu_pct(void)
{
    static uint32_t smoothed_q12;
    uint32_t used = last_used, block = block_ticks;
    if (!block)
        return 0;
    if (used > 2U * block)
        used = 2U * block;
    smoothed_q12 = (smoothed_q12 * 7U + (uint32_t)(((uint64_t)used << 12) / block) + 4U) / 8U;
    return (smoothed_q12 * 100U + 2048U) >> 12;
}

static inline __attribute__((always_inline)) uint32_t elapsed(uint32_t start, uint32_t end)
{
    return end >= start ? end - start : end + FM1_AUDIO_TIMER_PERIOD + 1U - start;
}

static inline __attribute__((always_inline)) int32_t limit(int64_t value)
{
    if (((uint64_t)value + 8388608U) >> 24)
        return value < 0 ? -8388608 : 8388607;
    return (int32_t)value;
}

/* No libc calls in the DMA commit window, including during Flash writes. */
static FM1_AUDIO_CODE void copy_block(volatile int32_t *out, const int32_t *in)
{
    for (unsigned i = 0; i < FM1_BLOCK_WORDS; ++i)
        out[i] = in[i];
    __asm__ volatile("csync" ::: "memory");
}

static FM1_AUDIO_CODE void silence(int32_t *out)
{
    for (unsigned i = 0; i < FM1_BLOCK_WORDS; ++i)
        out[i] = 0;
}

/* The computer's audio (USB playback) on top, at the host's volume; then the block goes to USB recording. */
static FM1_AUDIO_CODE void output_mix(int32_t *output)
{
    uint16_t usb_gain[2] = {0, 0};
    int usb_frames = fm1_usb_audio_read(usb_pcm, FM1_BLOCK_FRAMES, usb_gain);
    if (usb_frames == FM1_BLOCK_FRAMES) {
        for (unsigned i = 0; i < FM1_BLOCK_WORDS; ++i)
            output[i] = limit((int64_t)output[i] + (((int64_t)usb_pcm[i] * usb_gain[i & 1U]) >> 7));
    } else if (usb_frames == -2) {
        silence(output);
    }
    (void)fm1_usb_audio_write(output, FM1_BLOCK_FRAMES);
}

FM1_AUDIO_CODE __attribute__((interrupt(""))) void fm1_audio_irq(void)
{
    uint8_t pending = fm1_audio_pending();
    unsigned rx = pending & 1U;
    uint32_t start = FM1_AUDIO_TIMER_COUNT, interval, used;
    unsigned half, committed = 0;
    fm1_audio_ack(pending);
    if (!rx || mute_depth)
        return;
    if (!previous_valid) {
        previous_tick = start;
        previous_valid = 1;
        return;
    }
    interval = elapsed(previous_tick, start);
    if (interval < 80000U) {
        ++statistics.short_intervals;
        return;
    }
    previous_tick = start;
    if (interval < block_ticks - (block_ticks >> 3)) {
        ++statistics.short_intervals;
        return;
    }
    if (interval > block_ticks + (block_ticks >> 3)) {
        ++statistics.overruns;
        return;
    }
    half = fm1_audio_free_half();
    smk_render_fn render = app_render_fn;
    if (render)
        render(rendered, FM1_BLOCK_FRAMES, master_q12);
    else
        silence(rendered);
    output_mix(rendered);
    uint32_t flags = fm1_audio_irq_save();
    if (fm1_audio_half_owned(half) &&
        elapsed(start, FM1_AUDIO_TIMER_COUNT) < block_ticks - (block_ticks >> 3)) {
        copy_block(transmit[half], rendered);
        committed = fm1_audio_half_owned(half);
    }
    fm1_audio_irq_restore(flags);
    if (!committed)
        ++statistics.overruns;
    pending = fm1_audio_pending();
    if (pending) {
        fm1_audio_ack(pending);
        if (pending & 1U)
            ++statistics.overruns;
    }
    used = elapsed(start, FM1_AUDIO_TIMER_COUNT);
    if (used > statistics.peak_ticks)
        statistics.peak_ticks = used;
    last_used = used;
    ++statistics.blocks;
}

void fm1_output_reset(void) {}

void fm1_audio_suspend(void)
{
    uint32_t flags = fm1_device_irq_save();
    if (mute_depth++ == 0) {
        for (unsigned half = 0; half < 2; ++half)
            for (unsigned i = 0; i < FM1_BLOCK_WORDS; ++i)
                transmit[half][i] = 0;
        fm1_device_fence();
    }
    fm1_device_irq_restore(flags);
}

void fm1_audio_resume(void)
{
    uint32_t flags = fm1_device_irq_save();
    if (mute_depth && --mute_depth == 0)
        previous_valid = 0;
    fm1_device_irq_restore(flags);
}

__attribute__((cold, minsize)) int fm1_audio_start(void)
{
    if (initialized)
        return 0;
    uint32_t clock = fm1_device_clock();
    if (!clock)
        return -1;
    block_ticks = (uint32_t)((uint64_t)clock * FM1_BLOCK_FRAMES / FM1_AUDIO_HARDWARE_RATE);
    if (!block_ticks)
        return -1;
    memset(receive, 0, sizeof receive);
    memset(transmit, 0, sizeof transmit);
    FM1_ALNK_CON0 = 0; FM1_ALNK_CON1 = 0; FM1_ALNK_CON2 = 0; FM1_ALNK_CON3 = 0;
    *(volatile uint32_t *)0x51030U &= ~(3U << 6); /* fixed port C route */
    const uint32_t outputs = (1U << 0) | (1U << 1) | (1U << 2) | (1U << 6);
    FM1_PR(FM1_PC, FM1_OUT) |= outputs;
    FM1_PR(FM1_PC, FM1_DIE) |= 1U << 3;
    FM1_PR(FM1_PC, FM1_DIR) = (FM1_PR(FM1_PC, FM1_DIR) & ~outputs) | (1U << 3);
    FM1_PR(FM1_PC, FM1_PD) &= ~(1U << 3);
    FM1_PR(FM1_PC, FM1_PU) |= 1U << 3;
    *(volatile uint32_t *)0x10014U &= ~(15U << 8); /* 11.2896 MHz, 256 fs */
    FM1_ALNK_CON3 = 3U | (4U << 5);
    FM1_ALNK_CON0 = (1U << 8) | (1U << 7);
    FM1_ALNK_LEN = FM1_BLOCK_WORDS;
    FM1_ALNK_ADR0 = (uint32_t)(uintptr_t)receive;
    FM1_ALNK_ADR3 = (uint32_t)(uintptr_t)transmit;
    FM1_ALNK_CON1 = 1U | (1U << 2) | (1U << 3) | (1U << 12) | (1U << 14);
    fm1_device_fence();
    fm1_device_irq_attach(FM1_AUDIO_IRQ, fm1_audio_irq, FM1_AUDIO_PRIORITY);
    FM1_ALNK_CON0 |= 1U << 11;
    initialized = 1;
    return 0;
}

void fm1_audio_stats_read(struct fm1_audio_stats *out)
{
    if (!out)
        return;
    uint32_t flags = fm1_device_irq_save();
    *out = statistics;
    fm1_device_irq_restore(flags);
}
