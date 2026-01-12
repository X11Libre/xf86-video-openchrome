/*
 * Copyright 2005-2016 The OpenChrome Project
 *                     [https://www.freedesktop.org/wiki/Openchrome]
 * Copyright 2004-2005 The Unichrome Project  [unichrome.sf.net]
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * via_tv.c
 *
 * Handles the initialization and management of TV output related
 * resources.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "via_driver.h"
#include <unistd.h>

static void
viaTVSetDisplaySource(ScrnInfoPtr pScrn, CARD8 displaySource)
{

    vgaHWPtr hwp = VGAHWPTR(pScrn);
    VIAPtr pVia = VIAPTR(pScrn);
    CARD8 sr12, sr13;
    CARD8 sr5a = 0x00;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Entered %s.\n", __func__));

    if ((pVia->Chipset == VIA_CX700)
        || (pVia->Chipset == VIA_VX800)
        || (pVia->Chipset == VIA_VX855)
        || (pVia->Chipset == VIA_VX900)) {

        sr5a = hwp->readSeq(hwp, 0x5A);
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "SR5A: 0x%02X\n", sr5a));
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "Setting 3C5.5A[0] to 0.\n"));
        ViaSeqMask(hwp, 0x5A, sr5a & 0xFE, 0x01);
    }

    sr12 = hwp->readSeq(hwp, 0x12);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "SR12: 0x%02X\n", sr12));
    sr13 = hwp->readSeq(hwp, 0x13);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "SR13: 0x%02X\n", sr13));
    switch (pVia->Chipset) {
    case VIA_CLE266:
        /* 3C5.12[5] - FPD18 pin strapping
         *             0: DIP0 (Digital Interface Port 0) is used by
         *                a TMDS transmitter (DVI)
         *             1: DIP0 (Digital Interface Port 0) is used by
         *                a TV encoder */
        if (sr12 & 0x20) {
            viaDIP0SetDisplaySource(pScrn, displaySource);
        } else {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                        "DIP0 was not set up for "
                        "an external TV encoder use.\n");
        }

        break;
    case VIA_KM400:
    case VIA_K8M800:
    case VIA_PM800:
    case VIA_P4M800PRO:
        /* 3C5.13[3] - DVP0D8 pin strapping
         *             0: AGP pins are used for AGP
         *             1: AGP pins are used by FPDP
         *                (Flat Panel Display Port)
         * 3C5.12[6] - DVP0D6 pin strapping
         *             0: Disable DVP0 (Digital Video Port 0)
         *             1: Enable DVP0 (Digital Video Port 0)
         * 3C5.12[5] - DVP0D5 pin strapping
         *             0: DVP0 is used by a TMDS transmitter (DVI)
         *             1: DVP0 is used by a TV encoder
         * 3C5.12[4] - DVP0D4 pin strapping
         *             0: Dual 12-bit FPDP (Flat Panel Display Port)
         *             1: 24-bit FPDP (Flat Panel Display Port) */
        if ((sr12 & 0x40) && (sr12 & 0x20)) {
            viaDVP0SetDisplaySource(pScrn, displaySource);
        } else if ((sr13 & 0x08) && (!(sr12 & 0x10))) {
            viaFPDPLowSetDisplaySource(pScrn, displaySource);
        } else if (sr13 & 0x08) {
            viaDVP1SetDisplaySource(pScrn, displaySource);
        } else {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                        "None of the external ports were set up for "
                        "external TV encoder use.\n");
        }

        break;
    case VIA_P4M890:
    case VIA_K8M890:
    case VIA_P4M900:
        /* 3C5.12[6] - FPD6 pin strapping
         *             0: Disable DVP0 (Digital Video Port 0)
         *             1: Enable DVP0 (Digital Video Port 0)
         * 3C5.12[5] - FPD5 pin strapping
         *             0: DVP0 is used by a TMDS transmitter (DVI)
         *             1: DVP0 is used by a TV encoder
         * 3C5.12[4] - FPD4 pin strapping
         *             0: Dual 12-bit FPDP (Flat Panel Display Port)
         *             1: 24-bit FPDP (Flat Panel Display Port) */
        if ((sr12 & 0x40) && (sr12 & 0x20) && (!(sr12 & 0x10))) {
            viaDVP0SetDisplaySource(pScrn, displaySource);
        } else {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                        "Unrecognized external TV encoder use.\n"
                        "Contact the developer for assistance.\n");
        }

        break;
    case VIA_CX700:
    case VIA_VX800:
    case VIA_VX855:
    case VIA_VX900:
        /* 3C5.13[6] - DVP1 DVP / capture port selection
         *             0: DVP1 is used as a DVP (Digital Video Port)
         *             1: DVP1 is used as a capture port
         */
        if (!(sr13 & 0x40)) {
            viaDVP1SetDisplaySource(pScrn, displaySource);
        } else {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                        "DVP1 is not set up for external TV "
                        "encoder use.\n");
        }

        break;
    default:
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                    "Unrecognized IGP for "
                    "an external TV encoder use.\n");
        break;
    }

    if ((pVia->Chipset == VIA_CX700)
        || (pVia->Chipset == VIA_VX800)
        || (pVia->Chipset == VIA_VX855)
        || (pVia->Chipset == VIA_VX900)) {

        hwp->writeSeq(hwp, 0x5A, sr5a);
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "Restoring 3C5.5A[0].\n"));
    }

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Exiting %s.\n", __func__));
}

static void
viaTVEnableIOPads(ScrnInfoPtr pScrn, CARD8 ioPadState)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    VIAPtr pVia = VIAPTR(pScrn);
    CARD8 sr12, sr13;
    CARD8 sr5a = 0x00;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Entered %s.\n", __func__));

    if ((pVia->Chipset == VIA_CX700)
        || (pVia->Chipset == VIA_VX800)
        || (pVia->Chipset == VIA_VX855)
        || (pVia->Chipset == VIA_VX900)) {

        sr5a = hwp->readSeq(hwp, 0x5A);
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "SR5A: 0x%02X\n", sr5a));
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "Setting 3C5.5A[0] to 0.\n"));
        ViaSeqMask(hwp, 0x5A, sr5a & 0xFE, 0x01);
    }

    sr12 = hwp->readSeq(hwp, 0x12);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "SR12: 0x%02X\n", sr12));
    sr13 = hwp->readSeq(hwp, 0x13);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "SR13: 0x%02X\n", sr13));
    switch (pVia->Chipset) {
    case VIA_CLE266:
        /* 3C5.12[5] - FPD18 pin strapping
         *             0: DIP0 (Digital Interface Port 0) is used by
         *                a TMDS transmitter (DVI)
         *             1: DIP0 (Digital Interface Port 0) is used by
         *                a TV encoder */
        if (sr12 & 0x20) {
            viaDIP0SetIOPadState(pScrn, ioPadState);
        } else {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                        "DIP0 is not set up for "
                        "an external TV encoder use.\n");
        }

        break;
    case VIA_KM400:
    case VIA_K8M800:
    case VIA_PM800:
    case VIA_P4M800PRO:
        /* 3C5.13[3] - DVP0D8 pin strapping
         *             0: AGP pins are used for AGP
         *             1: AGP pins are used by FPDP
         *                (Flat Panel Display Port)
         * 3C5.12[6] - DVP0D6 pin strapping
         *             0: Disable DVP0 (Digital Video Port 0)
         *             1: Enable DVP0 (Digital Video Port 0)
         * 3C5.12[5] - DVP0D5 pin strapping
         *             0: DVP0 is used by a TMDS transmitter (DVI)
         *             1: DVP0 is used by a TV encoder
         * 3C5.12[4] - DVP0D4 pin strapping
         *             0: Dual 12-bit FPDP (Flat Panel Display Port)
         *             1: 24-bit FPDP (Flat Panel Display Port) */
        if ((sr12 & 0x40) && (sr12 & 0x20)) {
            viaDVP0SetIOPadState(pScrn, ioPadState);
        } else if ((sr13 & 0x08) && (!(sr12 & 0x10))) {
            viaFPDPLowSetIOPadState(pScrn, ioPadState);
        } else if (sr13 & 0x08) {
            viaDVP1SetIOPadState(pScrn, ioPadState);
        } else {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                        "None of the external ports were set up for "
                        "external TV encoder use.\n");
        }

        break;
    case VIA_P4M890:
    case VIA_K8M890:
    case VIA_P4M900:
        /* 3C5.12[6] - FPD6 pin strapping
         *             0: Disable DVP0 (Digital Video Port 0)
         *             1: Enable DVP0 (Digital Video Port 0)
         * 3C5.12[5] - FPD5 pin strapping
         *             0: DVP0 is used by a TMDS transmitter (DVI)
         *             1: DVP0 is used by a TV encoder
         * 3C5.12[4] - FPD4 pin strapping
         *             0: Dual 12-bit FPDP (Flat Panel Display Port)
         *             1: 24-bit FPDP (Flat Panel Display Port) */
        if ((sr12 & 0x40) && (sr12 & 0x20) && (!(sr12 & 0x10))) {
            viaDVP0SetIOPadState(pScrn, ioPadState);
        } else {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                        "Unrecognized external TV encoder use.\n"
                        "Contact the developer for assistance.\n");
        }

        break;
    case VIA_CX700:
    case VIA_VX800:
    case VIA_VX855:
    case VIA_VX900:
        /* 3C5.13[6] - DVP1 DVP / capture port selection
         *             0: DVP1 is used as a DVP (Digital Video Port)
         *             1: DVP1 is used as a capture port
         */
        if (!(sr13 & 0x40)) {
            viaDVP1SetIOPadState(pScrn, ioPadState);
        } else {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                        "DVP1 is not set up for external TV "
                        "encoder use.\n");
        }

        break;
    default:
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                    "Unrecognized IGP for "
                    "an external TV encoder use.\n");
        break;
    }

    if ((pVia->Chipset == VIA_CX700)
        || (pVia->Chipset == VIA_VX800)
        || (pVia->Chipset == VIA_VX855)
        || (pVia->Chipset == VIA_VX900)) {

        hwp->writeSeq(hwp, 0x5A, sr5a);
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "Restoring 3C5.5A[0].\n"));
    }

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Exiting %s.\n", __func__));
}

static void
viaTVSetClockDriveStrength(ScrnInfoPtr pScrn, CARD8 clockDriveStrength)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    VIAPtr pVia = VIAPTR(pScrn);
    CARD8 sr12, sr13;
    CARD8 sr5a = 0x00;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Entered %s.\n", __func__));

    if ((pVia->Chipset == VIA_CX700)
        || (pVia->Chipset == VIA_VX800)
        || (pVia->Chipset == VIA_VX855)
        || (pVia->Chipset == VIA_VX900)) {

        sr5a = hwp->readSeq(hwp, 0x5A);
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "SR5A: 0x%02X\n", sr5a));
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "Setting 3C5.5A[0] to 0.\n"));
        ViaSeqMask(hwp, 0x5A, sr5a & 0xFE, 0x01);
    }

    sr12 = hwp->readSeq(hwp, 0x12);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "SR12: 0x%02X\n", sr12));
    sr13 = hwp->readSeq(hwp, 0x13);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "SR13: 0x%02X\n", sr13));
    switch (pVia->Chipset) {
    case VIA_CLE266:
        break;
    case VIA_KM400:
    case VIA_K8M800:
    case VIA_PM800:
    case VIA_P4M800PRO:
        /* 3C5.13[3] - DVP0D8 pin strapping
         *             0: AGP pins are used for AGP
         *             1: AGP pins are used by FPDP
         *                (Flat Panel Display Port)
         * 3C5.12[6] - DVP0D6 pin strapping
         *             0: Disable DVP0 (Digital Video Port 0)
         *             1: Enable DVP0 (Digital Video Port 0)
         * 3C5.12[5] - DVP0D5 pin strapping
         *             0: DVP0 is used by a TMDS transmitter (DVI)
         *             1: DVP0 is used by a TV encoder
         * 3C5.12[4] - DVP0D4 pin strapping
         *             0: Dual 12-bit FPDP (Flat Panel Display Port)
         *             1: 24-bit FPDP (Flat Panel Display Port) */
        if ((sr12 & 0x40) && (sr12 & 0x20)) {
            viaDVP0SetClockDriveStrength(pScrn, clockDriveStrength);
        } else if ((sr13 & 0x08) && (!(sr12 & 0x10))) {
        } else if (sr13 & 0x08) {
            viaDVP1SetClockDriveStrength(pScrn, clockDriveStrength);
        }

        break;
    case VIA_P4M890:
    case VIA_K8M890:
    case VIA_P4M900:
        /* 3C5.12[6] - FPD6 pin strapping
         *             0: Disable DVP0 (Digital Video Port 0)
         *             1: Enable DVP0 (Digital Video Port 0)
         * 3C5.12[5] - FPD5 pin strapping
         *             0: DVP0 is used by a TMDS transmitter (DVI)
         *             1: DVP0 is used by a TV encoder
         * 3C5.12[4] - FPD4 pin strapping
         *             0: Dual 12-bit FPDP (Flat Panel Display Port)
         *             1: 24-bit FPDP (Flat Panel Display Port) */
        if ((sr12 & 0x40) && (sr12 & 0x20) && (!(sr12 & 0x10))) {
            viaDVP0SetClockDriveStrength(pScrn, clockDriveStrength);
        }

        break;
    case VIA_CX700:
    case VIA_VX800:
    case VIA_VX855:
    case VIA_VX900:
        /* 3C5.13[6] - DVP1 DVP / capture port selection
         *             0: DVP1 is used as a DVP (Digital Video Port)
         *             1: DVP1 is used as a capture port */
        if (!(sr13 & 0x40)) {
            viaDVP1SetClockDriveStrength(pScrn, clockDriveStrength);
        }

        break;
    default:
        break;
    }

    if ((pVia->Chipset == VIA_CX700)
        || (pVia->Chipset == VIA_VX800)
        || (pVia->Chipset == VIA_VX855)
        || (pVia->Chipset == VIA_VX900)) {

        hwp->writeSeq(hwp, 0x5A, sr5a);
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "Restoring 3C5.5A[0].\n"));
    }

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Exiting %s.\n", __func__));
}

static void
viaTVSetDataDriveStrength(ScrnInfoPtr pScrn, CARD8 dataDriveStrength)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    VIAPtr pVia = VIAPTR(pScrn);
    CARD8 sr12, sr13;
    CARD8 sr5a = 0x00;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Entered %s.\n", __func__));

    if ((pVia->Chipset == VIA_CX700)
        || (pVia->Chipset == VIA_VX800)
        || (pVia->Chipset == VIA_VX855)
        || (pVia->Chipset == VIA_VX900)) {

        sr5a = hwp->readSeq(hwp, 0x5A);
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "SR5A: 0x%02X\n", sr5a));
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "Setting 3C5.5A[0] to 0.\n"));
        ViaSeqMask(hwp, 0x5A, sr5a & 0xFE, 0x01);
    }

    sr12 = hwp->readSeq(hwp, 0x12);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "SR12: 0x%02X\n", sr12));
    sr13 = hwp->readSeq(hwp, 0x13);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "SR13: 0x%02X\n", sr13));
    switch (pVia->Chipset) {
    case VIA_CLE266:
        break;
    case VIA_KM400:
    case VIA_K8M800:
    case VIA_PM800:
    case VIA_P4M800PRO:
        /* 3C5.13[3] - DVP0D8 pin strapping
         *             0: AGP pins are used for AGP
         *             1: AGP pins are used by FPDP
         *                (Flat Panel Display Port)
         * 3C5.12[6] - DVP0D6 pin strapping
         *             0: Disable DVP0 (Digital Video Port 0)
         *             1: Enable DVP0 (Digital Video Port 0)
         * 3C5.12[5] - DVP0D5 pin strapping
         *             0: DVP0 is used by a TMDS transmitter (DVI)
         *             1: DVP0 is used by a TV encoder
         * 3C5.12[4] - DVP0D4 pin strapping
         *             0: Dual 12-bit FPDP (Flat Panel Display Port)
         *             1: 24-bit FPDP (Flat Panel Display Port) */
        if ((sr12 & 0x40) && (sr12 & 0x20)) {
            viaDVP0SetDataDriveStrength(pScrn, dataDriveStrength);
        } else if ((sr13 & 0x08) && (!(sr12 & 0x10))) {
        } else if (sr13 & 0x08) {
            viaDVP1SetDataDriveStrength(pScrn, dataDriveStrength);
        }

        break;
    case VIA_P4M890:
    case VIA_K8M890:
    case VIA_P4M900:
        /* 3C5.12[6] - FPD6 pin strapping
         *             0: Disable DVP0 (Digital Video Port 0)
         *             1: Enable DVP0 (Digital Video Port 0)
         * 3C5.12[5] - FPD5 pin strapping
         *             0: DVP0 is used by a TMDS transmitter (DVI)
         *             1: DVP0 is used by a TV encoder
         * 3C5.12[4] - FPD4 pin strapping
         *             0: Dual 12-bit FPDP (Flat Panel Display Port)
         *             1: 24-bit FPDP (Flat Panel Display Port) */
        if ((sr12 & 0x40) && (sr12 & 0x20) && (!(sr12 & 0x10))) {
            viaDVP0SetDataDriveStrength(pScrn, dataDriveStrength);
        }

        break;
    case VIA_CX700:
    case VIA_VX800:
    case VIA_VX855:
    case VIA_VX900:
        /* 3C5.13[6] - DVP1 DVP / capture port selection
         *             0: DVP1 is used as a DVP (Digital Video Port)
         *             1: DVP1 is used as a capture port */
        if (!(sr13 & 0x40)) {
            viaDVP1SetDataDriveStrength(pScrn, dataDriveStrength);
        }

        break;
    default:
        break;
    }

    if ((pVia->Chipset == VIA_CX700)
        || (pVia->Chipset == VIA_VX800)
        || (pVia->Chipset == VIA_VX855)
        || (pVia->Chipset == VIA_VX900)) {

        hwp->writeSeq(hwp, 0x5A, sr5a);
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "Restoring 3C5.5A[0].\n"));
    }

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Exiting %s.\n", __func__));
}

static void
ViaTVSave(xf86OutputPtr output)
{
    viaTVRecPtr pVIATV = (viaTVRecPtr) output->driver_private;

    if (pVIATV->TVSave)
        pVIATV->TVSave(output);
}

static void
ViaTVRestore(xf86OutputPtr output)
{
    viaTVRecPtr pVIATV = (viaTVRecPtr) output->driver_private;

    if (pVIATV->TVRestore)
        pVIATV->TVRestore(output);
}

static Bool
ViaTVDACSense(xf86OutputPtr output)
{
    viaTVRecPtr pVIATV = (viaTVRecPtr) output->driver_private;

    if (pVIATV->TVDACSense)
        return pVIATV->TVDACSense(output);

    return FALSE;
}

static void
ViaTVSetMode(xf86OutputPtr output, DisplayModePtr mode)
{
    viaTVRecPtr pVIATV = (viaTVRecPtr) output->driver_private;

    if (pVIATV->TVModeI2C)
        pVIATV->TVModeI2C(output, mode);

    if (pVIATV->TVModeCrtc)
        pVIATV->TVModeCrtc(output, mode);

    /* TV reset. */
    xf86I2CWriteByte(pVIATV->pVIATVI2CDev, 0x1D, 0x00);
    xf86I2CWriteByte(pVIATV->pVIATVI2CDev, 0x1D, 0x80);
}

static void
ViaTVPower(xf86OutputPtr output, Bool On)
{
#ifdef HAVE_DEBUG
    ScrnInfoPtr pScrn = output->scrn;
#endif /* HAVE_DEBUG */
    viaTVRecPtr pVIATV = (viaTVRecPtr) output->driver_private;

#ifdef HAVE_DEBUG
    if (On)
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "%s: On.\n", __func__);
    else
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "%s: Off.\n", __func__);
#endif

    if (pVIATV->TVPower)
        pVIATV->TVPower(output, On);
}

#ifdef HAVE_DEBUG
void
ViaTVPrintRegs(xf86OutputPtr output)
{
    viaTVRecPtr pVIATV = (viaTVRecPtr) output->driver_private;

    if (pVIATV->TVPrintRegs)
        pVIATV->TVPrintRegs(output);
}
#endif /* HAVE_DEBUG */

static void
via_tv_create_resources(xf86OutputPtr output)
{
}

static Bool
via_tv_set_property(xf86OutputPtr output, Atom property,
                    RRPropertyValuePtr value)
{
    return TRUE;
}

static Bool
via_tv_get_property(xf86OutputPtr output, Atom property)
{
    return FALSE;
}

static void
via_tv_dpms(xf86OutputPtr output, int mode)
{
    switch (mode) {
    case DPMSModeOn:
        ViaTVPower(output, TRUE);
        break;

    case DPMSModeStandby:
    case DPMSModeSuspend:
    case DPMSModeOff:
        ViaTVPower(output, FALSE);
        break;
    }
}

static void
via_tv_save(xf86OutputPtr output)
{
    ViaTVSave(output);
}

static void
via_tv_restore(xf86OutputPtr output)
{
    ViaTVRestore(output);
}

static int
via_tv_mode_valid(xf86OutputPtr output, DisplayModePtr pMode)
{
    ScrnInfoPtr pScrn = output->scrn;
    int ret = MODE_OK;

    if (!ViaModeDotClockTranslate(pScrn, pMode))
        return MODE_NOCLOCK;

    return ret;
}

static Bool
via_tv_mode_fixup(xf86OutputPtr output, DisplayModePtr mode,
                  DisplayModePtr adjusted_mode)
{
    return TRUE;
}

static void
via_tv_prepare(xf86OutputPtr output)
{
    via_tv_dpms(output, DPMSModeOff);
}

static void
via_tv_commit(xf86OutputPtr output)
{
    via_tv_dpms(output, DPMSModeOn);
}

static void
via_tv_mode_set(xf86OutputPtr output, DisplayModePtr mode,
                DisplayModePtr adjusted_mode)
{
    ScrnInfoPtr pScrn = output->scrn;
    drmmode_crtc_private_ptr iga = output->crtc->driver_private;
    VIAPtr pVia = VIAPTR(pScrn);

    /* TV on FirstCrtc */
    if (output->crtc) {
        viaTVSetDisplaySource(pScrn, iga->index ? 0x01 : 0x00);

        /* Set I/O pads to automatic on / off mode. */
        viaTVEnableIOPads(pScrn, 0x03);

        viaTVSetClockDriveStrength(pScrn, 0x03);
        viaTVSetDataDriveStrength(pScrn, 0x03);

        ViaTVSetMode(output, adjusted_mode);
    }

    pVia->FirstInit = FALSE;
}

static xf86OutputStatus
via_tv_detect(xf86OutputPtr output)
{
    xf86OutputStatus status = XF86OutputStatusDisconnected;

    if (ViaTVDACSense(output))
        status = XF86OutputStatusConnected;

    return status;
}

static DisplayModePtr
via_tv_get_modes(xf86OutputPtr output)
{
    DisplayModePtr modes = NULL, mode = NULL;
    viaTVRecPtr pVIATV = (viaTVRecPtr) output->driver_private;
    int i;

    for (i = 0; i < pVIATV->TVNumModes; i++) {
        mode = xf86DuplicateMode(&pVIATV->TVModes[i]);
        modes = xf86ModesAdd(modes, mode);
    }

    return modes;
}

static void
via_tv_destroy(xf86OutputPtr output)
{
}

static const xf86OutputFuncsRec via_tv_funcs = {
    .create_resources   = via_tv_create_resources,
    .set_property       = via_tv_set_property,
#ifdef RANDR_13_INTERFACE
    .get_property       = via_tv_get_property,
#endif
    .dpms               = via_tv_dpms,
    .save               = via_tv_save,
    .restore            = via_tv_restore,
    .mode_valid         = via_tv_mode_valid,
    .mode_fixup         = via_tv_mode_fixup,
    .prepare            = via_tv_prepare,
    .commit             = via_tv_commit,
    .mode_set           = via_tv_mode_set,
    .detect             = via_tv_detect,
    .get_modes          = via_tv_get_modes,
    .destroy            = via_tv_destroy,
};

/*
 * TV initialization
 */
void
via_tv_init(ScrnInfoPtr pScrn)
{
    VIAPtr pVia = VIAPTR(pScrn);
    VIADisplayPtr pVIADisplay = pVia->pVIADisplay;
    viaTVRecPtr pVIATV;
    I2CDevPtr pI2CDevice = NULL;
    xf86OutputPtr output;
    char outputNameBuffer[32];

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Entered %s.\n", __func__));

    /* preset some pVIADisplay TV related values -- move up */
    pVIADisplay->TVEncoder = VIA_NONETV;

    /*
     * On an SK43G (KM400/Ch7011), false positive detections at a VT162x
     * chip were observed, so try to detect the Ch7011 first.
     */
    if (pVIADisplay->pI2CBus2 && xf86I2CProbeAddress(pVIADisplay->pI2CBus2, 0xEC))
        pI2CDevice = ViaCH7xxxDetect(pScrn, pVIADisplay->pI2CBus2, 0xEC);
    else if (pVIADisplay->pI2CBus2 && xf86I2CProbeAddress(pVIADisplay->pI2CBus2, 0x40))
        pI2CDevice = ViaVT162xDetect(pScrn, pVIADisplay->pI2CBus2, 0x40);
    else if (pVIADisplay->pI2CBus3 && xf86I2CProbeAddress(pVIADisplay->pI2CBus3, 0x40))
        pI2CDevice = ViaVT162xDetect(pScrn, pVIADisplay->pI2CBus3, 0x40);
    else if (pVIADisplay->pI2CBus2 && xf86I2CProbeAddress(pVIADisplay->pI2CBus2, 0xEA))
        pI2CDevice = ViaCH7xxxDetect(pScrn, pVIADisplay->pI2CBus2, 0xEA);
    else if (pVIADisplay->pI2CBus3 && xf86I2CProbeAddress(pVIADisplay->pI2CBus3, 0xEA))
        pI2CDevice = ViaCH7xxxDetect(pScrn, pVIADisplay->pI2CBus3, 0xEA);

    if (!pI2CDevice) {
        xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
                    "Did not detect a TV encoder.\n");
        goto exit;
    }

    pVIATV = (viaTVRecPtr) xnfcalloc(1, sizeof(viaTVRec));
    if (pVIATV) {
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                            "Failed to allocate storage for TV.\n"));
        goto free_i2c;
    }

    sprintf(outputNameBuffer, "TV-%d", (pVIADisplay->numberTV));
    output = xf86OutputCreate(pScrn, &via_tv_funcs, outputNameBuffer);
    if (!output) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "Failed to register TV as an output.\n");
        goto free_mem;
    }

    /*
     * Increment the number of TV encoders.
     */
    pVIADisplay->numberTV++;

    pVia->FirstInit = TRUE;

    output->driver_private = pVIATV;

    /*
     * To allow TV output on both CRTCs, set bit 0 and 1.
     */
    output->possible_crtcs = BIT(1) | BIT(0);

    pVIATV->TVEncoder = pVIADisplay->TVEncoder;
    pVIATV->TVOutput = pVIADisplay->TVOutput;
    pVIATV->TVType = pVIADisplay->TVType;
    pVIATV->TVDotCrawl = pVIADisplay->TVDotCrawl;
    pVIATV->TVDeflicker = pVIADisplay->TVDeflicker;

    pVIATV->TVSave = NULL;
    pVIATV->TVRestore = NULL;
    pVIATV->TVDACSense = NULL;
    pVIATV->TVModeValid = NULL;
    pVIATV->TVModeI2C = NULL;
    pVIATV->TVModeCrtc = NULL;
    pVIATV->TVPower = NULL;
    pVIATV->TVModes = NULL;
#ifdef HAVE_DEBUG
    pVIATV->TVPrintRegs = NULL;
#endif /* HAVE_DEBUG */
    pVIATV->LCDPower = NULL;
    pVIATV->TVNumRegs = 0;

    pVIATV->pVIATVI2CDev = pI2CDevice;

    switch (pVIATV->TVEncoder) {
        case VIA_VT1621:
        case VIA_VT1622:
        case VIA_VT1623:
        case VIA_VT1625:
            ViaVT162xInit(output);
            break;
        case VIA_CH7011:
        case VIA_CH7019A:
        case VIA_CH7019B:
            ViaCH7xxxInit(output);
            break;
        default:
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                        "Was not able to initialize a known TV encoder.\n");
            goto free_mem;
            break;
    }

    if (!pVIATV->TVSave || !pVIATV->TVRestore
        || !pVIATV->TVDACSense || !pVIATV->TVModeValid
        || !pVIATV->TVModeI2C || !pVIATV->TVModeCrtc
        || !pVIATV->TVPower || !pVIATV->TVModes
#ifdef HAVE_DEBUG
        || !pVIATV->TVPrintRegs
#endif /* HAVE_DEBUG */
        ) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "TV encoder was not properly initialized.\n");
        goto free_mem;
    }

    /*
     * Save TV registers.
     */
    pVIATV->TVSave(output);

    goto exit;
free_mem:
    pVIATV->TVOutput = TVOUTPUT_NONE;
    pVIATV->TVEncoder = VIA_NONETV;

    pVIATV->TVSave = NULL;
    pVIATV->TVRestore = NULL;
    pVIATV->TVDACSense = NULL;
    pVIATV->TVModeValid = NULL;
    pVIATV->TVModeI2C = NULL;
    pVIATV->TVModeCrtc = NULL;
    pVIATV->TVPower = NULL;
    pVIATV->TVModes = NULL;
#ifdef HAVE_DEBUG
    pVIATV->TVPrintRegs = NULL;
#endif /* HAVE_DEBUG */
    pVIATV->TVNumRegs = 0;

    xf86DestroyI2CDevRec(pVIATV->pVIATVI2CDev, TRUE);
    pVIATV->pVIATVI2CDev = NULL;
    free(pVIATV);
free_i2c:
    pVIADisplay->TVOutput = TVOUTPUT_NONE;
    pVIADisplay->TVEncoder = VIA_NONETV;
exit:
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Exiting %s.\n", __func__));
    return;
}
