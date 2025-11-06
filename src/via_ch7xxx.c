/*
 * Copyright 2005 Terry Lewis. All Rights Reserved.
 * Copyright 2005 Philip Langdale. All Rights Reserved. (CH7011 additions)
 * Copyright 2004 The Unichrome Project  [unichrome.sf.net]
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
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "via_driver.h"
#include "via_ch7xxx.h"
#include <unistd.h>

#ifdef HAVE_DEBUG
/*
 *
 */
static void
CH7xxxPrintRegs(xf86OutputPtr output)
{
    ScrnInfoPtr pScrn = output->scrn;
    viaTVRecPtr pVIATV = (viaTVRecPtr) output->driver_private;
    CARD8 i, buf;

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Printing registers for %s\n",
               pVIATV->pVIATVI2CDev->DevName);

    for (i = 0; i < pVIATV->TVNumRegs; i++) {
        xf86I2CReadByte(pVIATV->pVIATVI2CDev, i, &buf);
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "TV%02X: 0x%02X\n", i, buf);
    }

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "End of TV registers.\n");
}
#endif /* HAVE_DEBUG */

/*
 *
 */
I2CDevPtr
ViaCH7xxxDetect(ScrnInfoPtr pScrn, I2CBusPtr pBus, CARD8 Address)
{
    VIADisplayPtr pVIADisplay = VIAPTR(pScrn)->pVIADisplay;
    I2CDevPtr pDev = xf86CreateI2CDevRec();
    CARD8 buf;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "%s\n", __func__));

    pDev->DevName = "CH7xxx";
    pDev->SlaveAddr = Address;
    pDev->pI2CBus = pBus;

    if (!xf86I2CDevInit(pDev)) {
        xf86DestroyI2CDevRec(pDev, TRUE);
        return NULL;
    }

    if (!xf86I2CReadByte(pDev, 0x4B, &buf)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Unable to read from %s Slave %d.\n",
                   pBus->BusName, Address);
        xf86DestroyI2CDevRec(pDev, TRUE);
        return NULL;
    }

    switch (buf) {
    case 0x17:
        xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Detected Chrontel CH7011 TV Encoder\n");
        pVIADisplay->TVEncoder = VIA_CH7011;
        pDev->DevName="CH7011";
        break;
    case 0x19:
        xf86I2CReadByte(pDev, 0x4A, &buf);
        if (buf == 0x81) {
            xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Detected Chrontel CH7019A LVDS Transmitter/TV Encoder\n");
            pVIADisplay->TVEncoder = VIA_CH7019A;
            pDev->DevName="CH7019A";
        } else {
            xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Detected Chrontel CH7019B LVDS Transmitter/TV Encoder\n");
            pVIADisplay->TVEncoder = VIA_CH7019B;
            pDev->DevName="CH7019B";
        }
        break;
    case 0x1B:
        xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Detected Chrontel CH7017 LVDS Transmitter\n");
        pVIADisplay->TVEncoder = VIA_CH7017;
        pDev->DevName="CH7017";
        break;
    case 0x3A:
        /* single init table --> single channel LVDS transmitter ? */
        xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Detected Chrontel CH7304 LVDS Transmitter\n");
        pVIADisplay->TVEncoder = VIA_CH7304;
        pDev->DevName="CH7304";
        break;
    case 0x3B:
        /* dual init table --> dual channel LVDS transmitter ? */
        xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Detected Chrontel CH7305 LVDS Transmitter\n");
        pVIADisplay->TVEncoder = VIA_CH7305;
        pDev->DevName="CH7305";
        break;
    default:
        pVIADisplay->TVEncoder = VIA_NONETV;
        xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "Unknown CH7xxx"
                   " device found. [%x:0x1B contains %x]\n",
                   Address, buf);
        xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Unknown CH7xxx encoder found\n");

        xf86DestroyI2CDevRec(pDev,TRUE);
        pDev = NULL;
        break;
    }

    return pDev;
}

/*
 *
 */

static void
CH7xxxSave(xf86OutputPtr output)
{
    int i;
#ifdef HAVE_DEBUG
    ScrnInfoPtr pScrn = output->scrn;
#endif /* HAVE_DEBUG */
    viaTVRecPtr pVIATV = (viaTVRecPtr) output->driver_private;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "%s\n", __func__));

    for (i = 0; i < pVIATV->TVNumRegs; i++)
        xf86I2CReadByte(pVIATV->pVIATVI2CDev, i, &(pVIATV->TVRegs[i]));
}


static void
CH7xxxRestore(xf86OutputPtr output)
{
    int i;
#ifdef HAVE_DEBUG
    ScrnInfoPtr pScrn = output->scrn;
#endif /* HAVE_DEBUG */
    viaTVRecPtr pVIATV = (viaTVRecPtr) output->driver_private;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "%s\n", __func__));

    for (i = 0; i < pVIATV->TVNumRegs; i++)
        xf86I2CWriteByte(pVIATV->pVIATVI2CDev, i, pVIATV->TVRegs[i]);
}

static CARD8
CH7xxxDACSenseI2C(I2CDevPtr pDev)
{
    CARD8 save, sense;

    /* Turn all DACP on*/
    xf86I2CWriteByte(pDev, 0x49, 0x20);

    /* Make sure Bypass mode is disabled (DACBP) bit0 is set to '0' */
    xf86I2CReadByte(pDev, 0x21, &save);
    xf86I2CWriteByte(pDev, 0x21, save & ~0x01);

    /* Set Sense bit0 to '1' */
    xf86I2CReadByte(pDev, 0x20, &save);
    xf86I2CWriteByte(pDev, 0x20, save | 0x01);

    /* Set Sense bit0 back to '0' */
    xf86I2CReadByte(pDev, 0x20, &save);
    xf86I2CWriteByte(pDev, 0x20, save & ~0x01);

    /* Read DACT status bits */
    xf86I2CReadByte(pDev, 0x20, &sense);

    return (sense & 0x1F);
}

/*
 *  A CH7xxx hack. (T. Lewis. S-Video fixed by P. Langdale)
 *
 *  CH7xxx Cable types (C+S and YcBcR untested and almost certainly wrong) 
 *      0x10 = Composite
 *      0x0C = S-Video
 *      0x02 = Composite+S-Video
 *      0x04 = YcBcR
 *      0x00 = Nothing Connected
 */

static Bool
CH7xxxDACSense(xf86OutputPtr output)
{
    ScrnInfoPtr pScrn = output->scrn;
    viaTVRecPtr pVIATV = (viaTVRecPtr) output->driver_private;
    CARD8 sense;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "%s\n", __func__));

    /* is this needed? IH */
    if (!pVIATV->pVIATVI2CDev ||
        !pVIATV->TVEncoder)
        return FALSE;

    sense = CH7xxxDACSenseI2C(pVIATV->pVIATVI2CDev);

    /* I'm sure these case values are correct,
     * but we should get something in any case.
     * 0x10 (Composite), 0x0C (S-Video) and 0x00 (Nothing connected)
     * seem to be correct however.
     */
    switch (sense) {
    case 0x10:
        pVIATV->TVOutput = TVOUTPUT_COMPOSITE;
        xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "CH7xxx: Composite connected.\n");
        return TRUE;
    case 0x0C:
        pVIATV->TVOutput = TVOUTPUT_SVIDEO;
        xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "CH7xxx: S-Video connected.\n");
        return TRUE;
    case 0x02:
        pVIATV->TVOutput = TVOUTPUT_SC;
        xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "CHxxx: Composite+S-Video connected.\n");
        return TRUE;
    case 0x04:
        pVIATV->TVOutput = TVOUTPUT_YCBCR;
        xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "CHxxx: YcBcR Connected.\n");
        return TRUE;
    case 0x00:
        pVIATV->TVOutput = TVOUTPUT_NONE;
        xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "CH7xxx: Nothing connected.\n");
        return FALSE;
    default:
        pVIATV->TVOutput = TVOUTPUT_NONE;
        xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "CH7xxx: Unknown cable combination: 0x0%2X.\n",sense);
        return FALSE;
    }
}

static CARD8
CH7011ModeIndex(xf86OutputPtr output, DisplayModePtr mode)
{
    ScrnInfoPtr pScrn = output->scrn;
    viaTVRecPtr pVIATV = (viaTVRecPtr) output->driver_private;
    int i;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "%s\n", __func__));

    for (i = 0; CH7011Table[i].Width; i++) {
        if ((CH7011Table[i].Width == mode->CrtcHDisplay) &&
            (CH7011Table[i].Height == mode->CrtcVDisplay) &&
            (CH7011Table[i].Standard == pVIATV->TVType) &&
            !(strcmp(CH7011Table[i].name, mode->name)))
            return i;
    }

    xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "%s:"
               " Mode \"%s\" not found in Table\n", __func__, mode->name);
    return 0xFF;
}

static CARD8
CH7019ModeIndex(xf86OutputPtr output, DisplayModePtr mode)
{
    ScrnInfoPtr pScrn = output->scrn;
    viaTVRecPtr pVIATV = (viaTVRecPtr) output->driver_private;
    int i;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "%s\n", __func__));
    for (i = 0; CH7019Table[i].Width; i++) {
        if ((CH7019Table[i].Width == mode->CrtcHDisplay) &&
            (CH7019Table[i].Height == mode->CrtcVDisplay) &&
            (CH7019Table[i].Standard == pVIATV->TVType) &&
            !(strcmp(CH7019Table[i].name, mode->name)))
            return i;
    }

    xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "%s:"
               " Mode \"%s\" not found in Table\n", __func__, mode->name);
    return 0xFF;
}

/*
 *
 */
static ModeStatus
CH7xxxModeValid(xf86OutputPtr output, DisplayModePtr mode)
{
    ScrnInfoPtr pScrn = output->scrn;
    viaTVRecPtr pVIATV = (viaTVRecPtr) output->driver_private;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "%s\n", __func__));

    if ((mode->PrivSize != sizeof(struct CH7xxxModePrivate)) ||
        ((mode->Private != (void *) &CH7xxxModePrivateNTSC) &&
         (mode->Private != (void *) &CH7xxxModePrivatePAL))) {
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Not a mode defined by the TV Encoder.\n");
        return MODE_BAD;
    }

    if ((pVIATV->TVType == TVTYPE_NTSC) &&
        (mode->Private != (void *) &CH7xxxModePrivateNTSC)) {
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "TV standard is NTSC. This is a PAL mode.\n");
        return MODE_BAD;
    } else if ((pVIATV->TVType == TVTYPE_PAL) &&
               (mode->Private != (void *) &CH7xxxModePrivatePAL)) {
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "TV standard is PAL. This is a NTSC mode.\n");
        return MODE_BAD;
    }

    if (pVIATV->TVEncoder == VIA_CH7011) {
        if (CH7011ModeIndex(output, mode) != 0xFF)
            return MODE_OK;
    } else {
        if (CH7019ModeIndex(output, mode) != 0xFF)
            return MODE_OK;
    }

    return MODE_BAD;
}

static void
CH7xxxModeI2C(xf86OutputPtr output, DisplayModePtr mode)
{
    ScrnInfoPtr pScrn = output->scrn;
    VIAPtr pVia = VIAPTR(pScrn);
    viaTVRecPtr pVIATV = (viaTVRecPtr) output->driver_private;
    CARD8   i, j;
    VIABIOSTVMASKTableRec mask;
    struct CH7xxxTableRec Table;

    if (pVIATV->TVEncoder == VIA_CH7011) {
        Table = CH7011Table[CH7011ModeIndex(output, mode)];
        mask = ch7011MaskTable;
    } else {
        Table = CH7019Table[CH7019ModeIndex(output, mode)];
        mask = ch7019MaskTable;
    }

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "%s\n", __func__));

    xf86I2CWriteByte(pVIATV->pVIATVI2CDev, 0x49, 0x3E);
    xf86I2CWriteByte(pVIATV->pVIATVI2CDev, 0x1E, 0xD0);

    for (i = 0,j = 0; (j < mask.numTV) && (i < VIA_BIOS_TABLE_NUM_TV_REG); i++) {
        if (mask.TV[i] == 0xFF) {
            xf86I2CWriteByte(pVIATV->pVIATVI2CDev, i, Table.TV[i]);
            j++;
        } else {
            xf86I2CWriteByte(pVIATV->pVIATVI2CDev, i, pVIATV->TVRegs[i]);
        }
    }

    if ((pVIATV->TVType == TVTYPE_NTSC) && pVIATV->TVDotCrawl) {
        CARD16 *DotCrawl = Table.DotCrawlNTSC;
        CARD8 address, save;

        for (i = 1; i < (DotCrawl[0] + 1); i++) {
            address = (CARD8)(DotCrawl[i] & 0xFF);

            save = (CARD8)(DotCrawl[i] >> 8);
            xf86I2CWriteByte(pVIATV->pVIATVI2CDev, address, save);
        }
    }

    /*
     * Only Composite and SVideo have been tested.
     */
    switch (pVIATV->TVOutput) {
    case TVOUTPUT_COMPOSITE:
        xf86I2CWriteByte(pVIATV->pVIATVI2CDev, 0x49, 0x2E);
        break;
    case TVOUTPUT_SVIDEO:
        xf86I2CWriteByte(pVIATV->pVIATVI2CDev, 0x49, 0x32);
        break;
    case TVOUTPUT_SC:
        xf86I2CWriteByte(pVIATV->pVIATVI2CDev, 0x49, 0x3C);
        break;
    case TVOUTPUT_YCBCR:
        xf86I2CWriteByte(pVIATV->pVIATVI2CDev, 0x49, 0x3A);
        break;
    default:
        break;
    }

    if (pVia->IsSecondary) { /* Patch as setting 2nd path */
        j = (CARD8)(mask.misc2 >> 5);
        for (i = 0; i < j; i++)
            xf86I2CWriteByte(pVIATV->pVIATVI2CDev, Table.Patch2[i] & 0xFF, Table.Patch2[i] >> 8);
    }
}

static void
CH7xxxModeCrtc(xf86OutputPtr output, DisplayModePtr mode)
{
    xf86CrtcPtr crtc = output->crtc;
    ScrnInfoPtr pScrn = crtc->scrn;
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    VIAPtr pVia =  VIAPTR(pScrn);
    VIADisplayPtr pVIADisplay = pVia->pVIADisplay;
    viaTVRecPtr pVIATV = (viaTVRecPtr) output->driver_private;
    CARD8  *CRTC, *Misc;
    int  i, j;
    VIABIOSTVMASKTableRec mask;
    struct CH7xxxTableRec Table;

    if (pVIATV->TVEncoder == VIA_CH7011) {
        Table = CH7011Table[CH7011ModeIndex(output, mode)];
        mask = ch7011MaskTable;
    } else {
        Table = CH7019Table[CH7019ModeIndex(output, mode)];
        mask = ch7019MaskTable;
    }

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "%s\n", __func__));

    if (pVia->IsSecondary) {
        switch (pScrn->bitsPerPixel) {
        case 16:
            CRTC = Table.CRTC2_16BPP;
            break;
        case 32:
            CRTC = Table.CRTC2_32BPP;
            break;
        case 8:
        default:
            CRTC = Table.CRTC2_8BPP;
            break;
        }
        Misc = Table.Misc2;

        for (i = 0, j = 0; i < mask.numCRTC2; j++) {
            if (mask.CRTC2[j] == 0xFF) {
                hwp->writeCrtc(hwp, j + 0x50, CRTC[j]);
                i++;
            }
        }

        if (mask.misc2 & 0x18) {
            pVIADisplay->Clock = (Misc[3] << 8) & Misc[4];
            /* VIASetUseExternalClock(hwp); */
        }

        ViaCrtcMask(hwp, 0x6A, 0xC0, 0xC0);
        ViaCrtcMask(hwp, 0x6B, 0x01, 0x01);
        ViaCrtcMask(hwp, 0x6C, 0x01, 0x01);

        /* Disable LCD Scaling */
        if (!pVia->SAMM || pVia->FirstInit)
            hwp->writeCrtc(hwp, 0x79, 0x00);
    } else {
        CRTC = Table.CRTC1;
        Misc = Table.Misc1;

        for (i = 0, j = 0; i < mask.numCRTC1; j++) {
            if (mask.CRTC1[j] == 0xFF) {
                hwp->writeCrtc(hwp, j, CRTC[j]);
                i++;
            }
        }

        ViaCrtcMask(hwp, 0x33, Misc[0], 0x20);
        hwp->writeCrtc(hwp, 0x6A, Misc[1]);

        if ((pVia->Chipset == VIA_CLE266) &&
            CLE266_REV_IS_AX(pVia->ChipRev)) {
            hwp->writeCrtc(hwp, 0x6B, Misc[2] | 0x81);
            /* Fix TV clock Polarity for CLE266A2 */
            if (pVia->ChipRev == 0x02)
                hwp->writeCrtc(hwp, 0x6C, Misc[3] | 0x01);
        } else
            hwp->writeCrtc(hwp, 0x6B, Misc[2] | 0x01);

        if (mask.misc1 & 0x30) {
            /* CLE266Ax use 2x XCLK */
            if ((pVia->Chipset == VIA_CLE266) &&
                CLE266_REV_IS_AX(pVia->ChipRev))
                pVIADisplay->Clock = 0x471C;
            else
                pVIADisplay->Clock = (Misc[4] << 8) | Misc[5];
        }

        ViaCrtcMask(hwp, 0x6A, 0x40, 0x40);
        ViaCrtcMask(hwp, 0x6B, 0x01, 0x01);
        ViaCrtcMask(hwp, 0x6C, 0x01, 0x01);
    }

    ViaSeqMask(hwp, 0x1E, 0xC0, 0xC0); /* Enable DI0/DVP0 */
}


/*
 *
 */
static void
CH7xxxTVPower(xf86OutputPtr output, Bool On)
{
#ifdef HAVE_DEBUG
    ScrnInfoPtr pScrn = output->scrn;
#endif /* HAVE_DEBUG */
    viaTVRecPtr pVIATV = (viaTVRecPtr) output->driver_private;

    if (On) {
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "%s: On\n", __func__));
        xf86I2CWriteByte(pVIATV->pVIATVI2CDev, 0x49, 0x20);
    } else {
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "%s: Off\n", __func__));
        xf86I2CWriteByte(pVIATV->pVIATVI2CDev, 0x49, 0x3E);
        xf86I2CWriteByte(pVIATV->pVIATVI2CDev, 0x1E, 0xD0);
    }
}

static void
CH7019LCDPower(xf86OutputPtr output, Bool On)
{
#ifdef HAVE_DEBUG
    ScrnInfoPtr pScrn = output->scrn;
#endif /* HAVE_DEBUG */
    viaTVRecPtr pVIATV = (viaTVRecPtr) output->driver_private;
    CARD8 W_Buffer[2], R_Buffer[1];
    int i;

    if (On) {
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "%s: On\n", __func__));
        W_Buffer[0] = 0x63;
        W_Buffer[1] = 0x4B;
        xf86I2CWriteRead(pVIATV->pVIATVI2CDev, W_Buffer,2, NULL,0);
        W_Buffer[0] = 0x66;
        W_Buffer[1] = 0x20;
        xf86I2CWriteRead(pVIATV->pVIATVI2CDev, W_Buffer,2, NULL,0);

        for (i = 0; i < 10; i++) {
            W_Buffer[0] = 0x63;
            xf86I2CWriteRead(pVIATV->pVIATVI2CDev, W_Buffer,1, R_Buffer,1);
            usleep(100);
            W_Buffer[0] = 0x63;
            W_Buffer[1] = (R_Buffer[0] | 0x40);
            xf86I2CWriteRead(pVIATV->pVIATVI2CDev, W_Buffer,2, NULL,0);
            DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
                             "%s: [%d]write 0x63 = %X!\n",
                             __func__, i+1, W_Buffer[1]));
            usleep(1);
            W_Buffer[0] = 0x63;
            W_Buffer[1] &= ~0x40;
            xf86I2CWriteRead(pVIATV->pVIATVI2CDev, W_Buffer,2, NULL,0);
            DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
                             "%s: [%d]write 0x63 = %X!\n",
                             __func__, i+1, W_Buffer[1]));
            usleep(100);
            W_Buffer[0] = 0x66;
            xf86I2CWriteRead(pVIATV->pVIATVI2CDev, W_Buffer,1, R_Buffer,1);

            if (((R_Buffer[0] & 0x44) == 0x44) || (i >= 9)) {
                /* PLL lock OK, Turn on VDD */
                usleep(500);
                W_Buffer[1] = R_Buffer[0] | 0x01;
                xf86I2CWriteRead(pVIATV->pVIATVI2CDev, W_Buffer,2, NULL,0);
                DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
                         "%s: CH7019 PLL lock ok!\n", __func__));
                /* reset data path */
                W_Buffer[0] = 0x48;
                xf86I2CWriteRead(pVIATV->pVIATVI2CDev, W_Buffer,1, R_Buffer,1);
                W_Buffer[1] = R_Buffer[0] & ~0x08;
                xf86I2CWriteRead(pVIATV->pVIATVI2CDev, W_Buffer,2, NULL,0);
                usleep(1);
                W_Buffer[1] = R_Buffer[0];
                xf86I2CWriteRead(pVIATV->pVIATVI2CDev, W_Buffer,2, NULL,0);
                break;
            }

            DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
                             "%s: [%d]CH7019 PLL lock fail!\n",
                             __func__, i+1));
            DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
                             "%s: [%d]0x66 = %X!\n",
                             __func__, i+1, R_Buffer[0]));
        }
    } else {
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "%s: Off\n", __func__));
        /* Turn off VDD (Turn off backlignt only) */
        W_Buffer[0] = 0x66;
        xf86I2CWriteRead(pVIATV->pVIATVI2CDev, W_Buffer,1, R_Buffer,1);
        W_Buffer[1] &= ~0x01;
        xf86I2CWriteRead(pVIATV->pVIATVI2CDev, W_Buffer,2, NULL,0);
        usleep(100);
        /* Turn off LVDS path */
        W_Buffer[0] = 0x63;
        xf86I2CWriteRead(pVIATV->pVIATVI2CDev, W_Buffer,1, R_Buffer,1);
        W_Buffer[1] = (R_Buffer[0] | 0x40);
        xf86I2CWriteRead(pVIATV->pVIATVI2CDev, W_Buffer,2, NULL,0);
    }
}

/*
 *
 */
void
ViaCH7xxxInit(xf86OutputPtr output)
{
#ifdef HAVE_DEBUG
    ScrnInfoPtr pScrn = output->scrn;
#endif /* HAVE_DEBUG */
    viaTVRecPtr pVIATV = (viaTVRecPtr) output->driver_private;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "%s\n", __func__));

    switch (pVIATV->TVEncoder) {
    case VIA_CH7011:
        pVIATV->TVSave = CH7xxxSave;
        pVIATV->TVRestore = CH7xxxRestore;
        pVIATV->TVDACSense = CH7xxxDACSense;
        pVIATV->TVModeValid = CH7xxxModeValid;
        pVIATV->TVModeI2C = CH7xxxModeI2C;
        pVIATV->TVModeCrtc = CH7xxxModeCrtc;
        pVIATV->TVPower = CH7xxxTVPower;
        pVIATV->TVModes = CH7011Modes;
        pVIATV->TVNumModes = sizeof(CH7011Modes) / sizeof(DisplayModeRec);
        pVIATV->LCDPower = NULL;
        pVIATV->TVNumRegs = CH_7011_MAX_NUM_REG;
#ifdef HAVE_DEBUG
        pVIATV->TVPrintRegs = CH7xxxPrintRegs;
#endif
        break;
    case VIA_CH7019A:
    case VIA_CH7019B:
        pVIATV->TVDACSense = CH7xxxDACSense;
        pVIATV->TVSave = CH7xxxSave;
        pVIATV->TVRestore = CH7xxxRestore;
        pVIATV->TVModeValid = CH7xxxModeValid;
        pVIATV->TVModeI2C = CH7xxxModeI2C;
        pVIATV->TVModeCrtc = CH7xxxModeCrtc;
        pVIATV->TVPower = CH7xxxTVPower;
        pVIATV->TVModes = CH7019Modes;
        pVIATV->TVNumModes = sizeof(CH7019Modes) / sizeof(DisplayModeRec);
        pVIATV->LCDPower = CH7019LCDPower;
        pVIATV->TVNumRegs = CH_7019_MAX_NUM_REG;
#ifdef HAVE_DEBUG
        pVIATV->TVPrintRegs = CH7xxxPrintRegs;
#endif
        break;
    default:
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "%s missing\n",
                __func__));
        break;
    }

    /* Save before continuing */
    if (pVIATV->TVSave)
        pVIATV->TVSave(output);
}
