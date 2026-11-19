/* SPDX-License-Identifier: GPL-3.0-only WITH GPL-3.0-linking-exception
 * Copyright (c) 2026 Techmelec (OpenVALive).
 *
 * OpenVALive SMK-37 - the app's settings for this target. The app itself is OpenVALive CORE's (va/app/);
 * the platform it expects (fm1-base's plat.h) is smk37_app.c's, on M-VAVE's SDK. Flash objects: smk37_app.c's
 * store, in Open's project slots (no APP_STORE_MAP).
 */
#pragma once
/* the UI -> audio queue: 64 events (the FM-1 has 128); the SDK's heap needs the kilobyte. The UI pushes a few per
 * frame and the audio side empties it every 2.9 ms */
#define OLS_EVQ_SIZE 64u
#include "../../va/app/openvalive_app.h"

/* the header's MASTER %: the SMK-37's MASTER encoder (smk37_app.c) */
static uint32_t smk_master_pct(void);
#define APP_MASTER_PCT() smk_master_pct()

#define APP_NAME "OPENVALIVE"
#define APP_SPLASH_RGB RGB(90, 200, 160)
#define APP_KEYS_SCREEN 1u      /* no lights in the keys: a layer button held shows them on the screen */
#ifdef OLS_NO_BLUETOOTH
#define APP_SPLASH              /* the boot logo (va/app/src/splash.c): room for it without the Bluetooth stack */
#endif
#define APP_TOUCH               /* key velocity and pressure: the SMK-37's calibrated keys, the pads */
#define APP_OVERLOAD            /* rendering over 90 % of its time: app_overload() sheds a voice */
#ifdef OLS_VERSION
#define APP_VERSION OLS_VERSION /* build.py --version */
#endif

/* host checks (tests/host_test.c) */
#define APP_HOST_EXPECT
int app_expect(const char *what, int *got);
