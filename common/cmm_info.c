/****************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ****************************************************************************

	Module Name:
	cmm_info.c
 */

#include "rt_config.h"

/*
	==========================================================================
	Description:
		Get Driver version.

	Return:
	==========================================================================
*/
BOOLEAN Set_DriverVersion_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 15, 0)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wdate-time"
#endif
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		DBGPRINT(RT_DEBUG_OFF, ("Driver version-%s %s %s\n", AP_DRIVER_VERSION, __DATE__, __TIME__));
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		DBGPRINT(RT_DEBUG_OFF, ("Driver version-%s %s %s\n", STA_DRIVER_VERSION, __DATE__, __TIME__));
#endif /* CONFIG_STA_SUPPORT */

#ifdef CONFIG_ANDES_SUPPORT
	if (pAd->chipCap.MCUType == ANDES) {
		UINT32 loop = 0;
		RTMP_CHIP_CAP *cap = &pAd->chipCap;

		if (pAd->chipCap.need_load_fw) {
			USHORT fw_ver, build_ver;
			fw_ver = (*(cap->FWImageName + 11) << 8) | (*(cap->FWImageName + 10));
			build_ver = (*(cap->FWImageName + 9) << 8) | (*(cap->FWImageName + 8));

			DBGPRINT(RT_DEBUG_OFF, ("fw version:%d.%d.%02d ", (fw_ver & 0xf000) >> 8,
							(fw_ver & 0x0f00) >> 8, fw_ver & 0x00ff));
			DBGPRINT(RT_DEBUG_OFF, ("build:%x\n", build_ver));
			DBGPRINT(RT_DEBUG_OFF, ("build time:"));

			for (loop = 0; loop < 16; loop++)
				DBGPRINT(RT_DEBUG_OFF, ("%c", *(cap->FWImageName + 16 + loop)));

			DBGPRINT(RT_DEBUG_OFF, ("\n"));
		}

		if (pAd->chipCap.need_load_rom_patch) {
			DBGPRINT(RT_DEBUG_OFF, ("rom patch version = \n"));

			for (loop = 0; loop < 4; loop++)
				DBGPRINT(RT_DEBUG_OFF, ("%c", *(cap->rom_patch + 24 + loop)));

			DBGPRINT(RT_DEBUG_OFF, ("\n"));

			DBGPRINT(RT_DEBUG_OFF, ("build time = \n"));

			for (loop = 0; loop < 16; loop++)
				DBGPRINT(RT_DEBUG_OFF, ("%c", *(cap->rom_patch + loop)));

			DBGPRINT(RT_DEBUG_OFF, ("\n"));
		}
	}
#endif

	return TRUE;
}

/*
	==========================================================================
	Description:
		Set Country Region.
		This command will not work, if the field of CountryRegion in eeprom is programmed.
	Return:
		TRUE if all parameters are OK, FALSE otherwise
	==========================================================================
*/
BOOLEAN Set_CountryRegion_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	BOOLEAN retval;

#ifdef EXT_BUILD_CHANNEL_LIST
	//JB WTF? return -EOPNOTSUPP;
	return FALSE;
#endif /* EXT_BUILD_CHANNEL_LIST */

	retval = RT_CfgSetCountryRegion(pAd, arg, BAND_24G);
	if (retval == FALSE)
		return FALSE;

	/* if set country region, driver needs to be reset*/
	BuildChannelList(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_CountryRegion_Proc::(CountryRegion=%d)\n",
			pAd->CommonCfg.CountryRegion));

	return TRUE;
}


/*
	==========================================================================
	Description:
		Set Country Region for A band.
		This command will not work, if the field of CountryRegion in eeprom is programmed.
	Return:
		TRUE if all parameters are OK, FALSE otherwise
	==========================================================================
*/
BOOLEAN Set_CountryRegionABand_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	int retval;

#ifdef EXT_BUILD_CHANNEL_LIST
	//JB WTF? return -EOPNOTSUPP;
	return FALSE;
#endif /* EXT_BUILD_CHANNEL_LIST */

	retval = RT_CfgSetCountryRegion(pAd, arg, BAND_5G);
	if (retval == FALSE)
		return FALSE;

	/* if set country region, driver needs to be reset*/
	BuildChannelList(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_CountryRegionABand_Proc::(CountryRegion=%d)\n",
			pAd->CommonCfg.CountryRegionForABand));
	return TRUE;
}


static BOOLEAN Set_Cmm_WirelessMode_Proc(RTMP_ADAPTER *pAd, PSTRING arg,
	BOOLEAN FlgIsDiffMbssModeUsed)
{
	BOOLEAN success = TRUE;
#ifdef CONFIG_AP_SUPPORT
	UINT32 i = 0;
#ifdef MBSS_SUPPORT
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if (FlgIsDiffMbssModeUsed) {
		LONG cfg_mode = simple_strtol(arg, 0, 10);

		/* assign wireless mode for the BSS */
		pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.PhyMode =
					cfgmode_2_wmode((UCHAR)cfg_mode);

		/*
			If the band is different with other BSS, we will correct it in
			RT_CfgSetMbssWirelessMode()
		*/
		success = RT_CfgSetMbssWirelessMode(pAd, arg);
	}
	else
#endif /* MBSS_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
	success = RT_CfgSetWirelessMode(pAd, arg);

	if (success)
	{
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		{
			/* recover Wmm Capable for "each" BSS */
			/* all phy mode of MBSS are the same */
			for (i = 0; i < pAd->ApCfg.BssidNum; i++)
			{
				pAd->ApCfg.MBSSID[i].wdev.bWmmCapable = \
										pAd->ApCfg.MBSSID[i].bWmmCapableOrg;

#ifdef MBSS_SUPPORT
				/* In Same-MBSS Mode, all phy modes are the same */
				if (FlgIsDiffMbssModeUsed == 0)
					pAd->ApCfg.MBSSID[i].wdev.PhyMode = pAd->CommonCfg.PhyMode;
#endif /* MBSS_SUPPORT */
			}

			RTMPSetPhyMode(pAd, pAd->CommonCfg.PhyMode);
		}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		{
			/* clean up previous SCAN result */
			BssTableInit(&pAd->ScanTab);
			pAd->StaCfg.LastScanTime = 0;

			RTMPSetPhyMode(pAd, pAd->CommonCfg.PhyMode);
#ifdef DOT11_N_SUPPORT
			if (WMODE_CAP_N(pAd->CommonCfg.PhyMode))
			{
				pAd->CommonCfg.BACapability.field.AutoBA = TRUE;
				pAd->CommonCfg.REGBACapability.field.AutoBA = TRUE;
			}
			else
			{
				pAd->CommonCfg.BACapability.field.AutoBA = FALSE;
				pAd->CommonCfg.REGBACapability.field.AutoBA = FALSE;
			}
#endif /* DOT11_N_SUPPORT */
			/* Set AdhocMode rates*/
			if (pAd->StaCfg.BssType == BSS_ADHOC)
			{
				MlmeUpdateTxRates(pAd, FALSE, 0);
				MakeIbssBeacon(pAd);           /* re-build BEACON frame*/
				AsicEnableIbssSync(pAd);       /* copy to on-chip memory*/
			}
		}
#endif /* CONFIG_STA_SUPPORT */


#ifdef CONFIG_AP_SUPPORT
#ifdef MBSS_SUPPORT
		DBGPRINT(RT_DEBUG_TRACE, ("Set_Cmm_WirelessMode_Proc::(=%d)\n", pAd->CommonCfg.PhyMode));
		DBGPRINT(RT_DEBUG_TRACE, ("Set_Cmm_WirelessMode_Proc::(BSS%d=%d)\n",
				pObj->ioctl_if, pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.PhyMode));

		for (i = 0; i < pAd->ApCfg.BssidNum; i++)
		{
			/*
				When last mode is not 11B-only, new mode is 11B, we need to re-make
				beacon frame content.

				Because we put support rate/extend support rate element in
				APMakeBssBeacon(), not APUpdateBeaconFrame().
			*/
			APMakeBssBeacon(pAd, i);
		}
#endif /* MBSS_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR, ("Set_WirelessMode_Proc::parameters out of range\n"));
	}

	return success;
}


#ifdef CONFIG_AP_SUPPORT
#ifdef MBSS_SUPPORT
/*
	==========================================================================
	Description:
		Set Wireless Mode for MBSS
	Return:
		TRUE if all parameters are OK, FALSE otherwise
	==========================================================================
*/
BOOLEAN Set_MBSS_WirelessMode_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	return Set_Cmm_WirelessMode_Proc(pAd, arg, 1);
}
#endif /* MBSS_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

/*
	==========================================================================
	Description:
		Set Wireless Mode
	Return:
		TRUE if all parameters are OK, FALSE otherwise
	==========================================================================
*/
BOOLEAN Set_WirelessMode_Proc(RTMP_ADAPTER *pAd, PSTRING arg)
{
	return Set_Cmm_WirelessMode_Proc(pAd, arg, 0);
}

#ifdef RT_CFG80211_SUPPORT
BOOLEAN Set_DisableCfg2040Scan_Proc(RTMP_ADAPTER *pAd, PSTRING arg)
{
	pAd->cfg80211_ctrl.FlgCfg8021Disable2040Scan = (UCHAR) simple_strtol(arg, 0, 10);
	DBGPRINT(RT_DEBUG_TRACE,
			("pAd->cfg80211_ctrl.FlgCfg8021Disable2040Scan  %d \n",
			pAd->cfg80211_ctrl.FlgCfg8021Disable2040Scan ));
	return TRUE;
}
#endif



/*
	==========================================================================
	Description:
		Set Channel
	Return:
		TRUE if all parameters are OK, FALSE otherwise
	==========================================================================
*/
BOOLEAN Set_Channel_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
#ifdef CONFIG_AP_SUPPORT
	int i;
#endif /* CONFIG_AP_SUPPORT */
 	BOOLEAN success = TRUE;
	UCHAR Channel;

	Channel = (UCHAR) simple_strtol(arg, 0, 10);

#ifdef APCLI_AUTO_CONNECT_SUPPORT
	if (pAd->ApCfg.ApCliAutoConnectChannelSwitching == FALSE)
		pAd->ApCfg.ApCliAutoConnectChannelSwitching = TRUE;
#endif /* APCLI_AUTO_CONNECT_SUPPORT */

	/* check if this channel is valid*/
	if (ChannelSanity(pAd, Channel) == TRUE) {
#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd) {
			pAd->CommonCfg.Channel = Channel;
			/* Save the channel on MlmeAux for CntlOidRTBssidProc used. */
			pAd->MlmeAux.Channel = Channel;

			if (MONITOR_ON(pAd)) {
				UCHAR rf_channel;
#ifdef DOT11_N_SUPPORT
				N_ChannelCheck(pAd);
				if (WMODE_CAP_N(pAd->CommonCfg.PhyMode) &&
					pAd->CommonCfg.RegTransmitSetting.field.BW == BW_40)
					rf_channel = N_SetCenCh(pAd, pAd->CommonCfg.Channel);
				else
#endif /* DOT11_N_SUPPORT */
					rf_channel = pAd->CommonCfg.Channel;

				AsicSwitchChannel(pAd, rf_channel, FALSE);
				AsicLockChannel(pAd, rf_channel);
				DBGPRINT(RT_DEBUG_TRACE, ("%s(): CtrlChannel(%d), CentralChannel(%d) \n",
							__FUNCTION__, pAd->CommonCfg.Channel,
							pAd->CommonCfg.CentralChannel));
			}
		}
#endif /* CONFIG_STA_SUPPORT */
		success = TRUE;
	} else {
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
			Channel = FirstChannel(pAd);
			DBGPRINT(RT_DEBUG_WARN,
					("This channel is out of channel list, set as the first channel(%d) \n ",
					 Channel));
		}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		success = FALSE;
		DBGPRINT(RT_DEBUG_WARN,("This channel is out of channel list, nothing to do!\n "));
#endif /* CONFIG_STA_SUPPORT */
	}

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
		if ((WMODE_CAP_5G(pAd->CommonCfg.PhyMode))
			&& (pAd->CommonCfg.bIEEE80211H == TRUE)) {
			for (i = 0; i < pAd->ChannelListNum; i++) {
				if (pAd->ChannelList[i].Channel == Channel) {
					if (pAd->ChannelList[i].RemainingTimeForUse > 0) {
						DBGPRINT(RT_DEBUG_ERROR,
								("ERROR: previous detection of a radar on this channel(Channel=%d)\n",
								Channel));
						success = FALSE;
						break;
					} else {
						DBGPRINT(RT_DEBUG_INFO,
								("RemainingTimeForUse %d ,Channel %d\n",
								pAd->ChannelList[i].RemainingTimeForUse,
								Channel));
					}
				}
			}
		}

		if (success == TRUE) {
			if ((pAd->CommonCfg.Channel > 14)
				&& (pAd->CommonCfg.bIEEE80211H == TRUE)) {
				pAd->Dot11_H.org_ch = pAd->CommonCfg.Channel;
			}

			pAd->CommonCfg.Channel = Channel;
#ifdef DOT11_N_SUPPORT
			N_ChannelCheck(pAd);
#endif /* DOT11_N_SUPPORT */

			if ((pAd->CommonCfg.Channel > 14)
				&& (pAd->CommonCfg.bIEEE80211H == TRUE)) {
				if (pAd->Dot11_H.RDMode == RD_SILENCE_MODE) {
					APStop(pAd);
					APStartUp(pAd);
				} else {
					NotifyChSwAnnToPeerAPs(pAd, ZERO_MAC_ADDR, pAd->CurrentAddress, 1, pAd->CommonCfg.Channel);
					pAd->Dot11_H.RDMode = RD_SWITCHING_MODE;
					pAd->Dot11_H.CSCount = 0;
					pAd->Dot11_H.new_channel = Channel;
				}
			} else {
				APStop(pAd);
				APStartUp(pAd);
			}
		}
	}
#endif /* CONFIG_AP_SUPPORT */

	if (success == TRUE) {
		DBGPRINT(RT_DEBUG_TRACE, ("Set_Channel_Proc::(Channel=%d)\n",
				pAd->CommonCfg.Channel));
#ifdef DFS_ATP_SUPPORT
		pAd->CommonCfg.RadarDetect.atp_set_channel_ready = TRUE;
		if ((pAd->CommonCfg.RadarDetect.atp_set_ht_bw
				|| pAd->CommonCfg.RadarDetect.atp_set_vht_bw)
				&& pAd->CommonCfg.RadarDetect.atp_set_channel_ready) {
			printk ("BW and Channel is ready\n");
		}
#endif /* DFS_ATP_SUPPORT */
	}

#ifdef APCLI_AUTO_CONNECT_SUPPORT
		pAd->ApCfg.ApCliAutoConnectChannelSwitching = FALSE;
#endif /* APCLI_AUTO_CONNECT_SUPPORT */
	return success;
}


/*
	==========================================================================
	Description:
		Set Short Slot Time Enable or Disable
	Return:
		TRUE if all parameters are OK, FALSE otherwise
	==========================================================================
*/
BOOLEAN Set_ShortSlot_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	BOOLEAN retval = RT_CfgSetShortSlot(pAd, arg);
	if (retval)
		DBGPRINT(RT_DEBUG_TRACE, ("Set_ShortSlot_Proc::(ShortSlot=%d)\n",
				pAd->CommonCfg.bUseShortSlotTime));

	return retval;
}


/*
	==========================================================================
	Description:
		Set Tx power
	Return:
		TRUE if all parameters are OK, FALSE otherwise
	==========================================================================
*/
BOOLEAN Set_TxPower_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	LONG TxPower;
	BOOLEAN success = FALSE;

	TxPower = simple_strtol(arg, 0, 10);
	if (TxPower <= 100) {
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
			pAd->CommonCfg.TxPowerPercentage = TxPower;
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd) {
			pAd->CommonCfg.TxPowerDefault = TxPower;
			pAd->CommonCfg.TxPowerPercentage = pAd->CommonCfg.TxPowerDefault;
		}
#endif /* CONFIG_STA_SUPPORT */
		success = TRUE;
	} else {
		success = FALSE;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("Set_TxPower_Proc::(TxPowerPercentage=%ld)\n",
			pAd->CommonCfg.TxPowerPercentage));

	return success;
}

/*
	==========================================================================
	Description:
		Set 11B/11G Protection
	Return:
		TRUE if all parameters are OK, FALSE otherwise
	==========================================================================
*/
BOOLEAN Set_BGProtection_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	switch (simple_strtol(arg, 0, 10)) {
	case 0: /*AUTO*/
		pAd->CommonCfg.UseBGProtection = 0;
		break;
	case 1: /*Always On*/
		pAd->CommonCfg.UseBGProtection = 1;
		break;
	case 2: /*Always OFF*/
		pAd->CommonCfg.UseBGProtection = 2;
		break;
	default:  /*Invalid argument */
		return FALSE;
	}

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		APUpdateCapabilityAndErpIe(pAd);
#endif /* CONFIG_AP_SUPPORT */

	DBGPRINT(RT_DEBUG_TRACE, ("Set_BGProtection_Proc::(BGProtection=%ld)\n",
			pAd->CommonCfg.UseBGProtection));
	return TRUE;
}

/*
	==========================================================================
	Description:
		Set TxPreamble
	Return:
		TRUE if all parameters are OK, FALSE otherwise
	==========================================================================
*/
BOOLEAN Set_TxPreamble_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	RT_802_11_PREAMBLE Preamble;

	Preamble = (RT_802_11_PREAMBLE)simple_strtol(arg, 0, 10);

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	if (Preamble == Rt802_11PreambleAuto)
		return FALSE;
#endif /* CONFIG_AP_SUPPORT */

	switch (Preamble) {
	case Rt802_11PreambleShort:
		pAd->CommonCfg.TxPreamble = Preamble;
#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
			MlmeSetTxPreamble(pAd, Rt802_11PreambleShort);
#endif /* CONFIG_STA_SUPPORT */
		break;
	case Rt802_11PreambleLong:
#ifdef CONFIG_STA_SUPPORT
	case Rt802_11PreambleAuto:
		/*
			If user wants AUTO, initialize to LONG here, then change according to AP's
			capability upon association
		*/
#endif /* CONFIG_STA_SUPPORT */
		pAd->CommonCfg.TxPreamble = Preamble;
#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
			MlmeSetTxPreamble(pAd, Rt802_11PreambleLong);
#endif /* CONFIG_STA_SUPPORT */
		break;
	default: /*Invalid argument */
		return FALSE;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("Set_TxPreamble_Proc::(TxPreamble=%ld)\n",
			pAd->CommonCfg.TxPreamble));

	return TRUE;
}

/*
	==========================================================================
	Description:
		Set RTS Threshold
	Return:
		TRUE if all parameters are OK, FALSE otherwise
	==========================================================================
*/
BOOLEAN Set_RTSThreshold_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	NDIS_802_11_RTS_THRESHOLD RtsThresh;

	RtsThresh = simple_strtol(arg, 0, 10);

	if ((RtsThresh > 0) && (RtsThresh <= MAX_RTS_THRESHOLD))
		pAd->CommonCfg.RtsThreshold  = (USHORT)RtsThresh;
#ifdef CONFIG_STA_SUPPORT
	else if (RtsThresh == 0)
		pAd->CommonCfg.RtsThreshold = MAX_RTS_THRESHOLD;
#endif /* CONFIG_STA_SUPPORT */
	else
		return FALSE; /*Invalid argument */

	DBGPRINT(RT_DEBUG_TRACE, ("Set_RTSThreshold_Proc::(RTSThreshold=%d)\n",
			pAd->CommonCfg.RtsThreshold));

	return TRUE;
}

/*
	==========================================================================
	Description:
		Set Fragment Threshold
	Return:
		TRUE if all parameters are OK, FALSE otherwise
	==========================================================================
*/
BOOLEAN Set_FragThreshold_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	 NDIS_802_11_FRAGMENTATION_THRESHOLD     FragThresh;

	FragThresh = simple_strtol(arg, 0, 10);

	if (FragThresh > MAX_FRAG_THRESHOLD || FragThresh < MIN_FRAG_THRESHOLD) {
		/*Illegal FragThresh so we set it to default*/
		pAd->CommonCfg.FragmentThreshold = MAX_FRAG_THRESHOLD;
	} else if (FragThresh % 2 == 1) {
		/*
			The length of each fragment shall always be an even number of octets,
			except for the last fragment of an MSDU or MMPDU, which may be either
			an even or an odd number of octets.
		*/
		pAd->CommonCfg.FragmentThreshold = (USHORT)(FragThresh - 1);
	} else {
		pAd->CommonCfg.FragmentThreshold = (USHORT)FragThresh;
	}

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd) {
		if (pAd->CommonCfg.FragmentThreshold == MAX_FRAG_THRESHOLD)
			pAd->CommonCfg.bUseZeroToDisableFragment = TRUE;
		else
			pAd->CommonCfg.bUseZeroToDisableFragment = FALSE;
	}
#endif /* CONFIG_STA_SUPPORT */

	DBGPRINT(RT_DEBUG_TRACE, ("Set_FragThreshold_Proc::(FragThreshold=%d)\n",
		pAd->CommonCfg.FragmentThreshold));

	return TRUE;
}

/*
	==========================================================================
	Description:
		Set TxBurst
	Return:
		TRUE if all parameters are OK, FALSE otherwise
	==========================================================================
*/
BOOLEAN Set_TxBurst_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	LONG TxBurst;

	TxBurst = simple_strtol(arg, 0, 10);
	if (TxBurst == 1)
		pAd->CommonCfg.bEnableTxBurst = TRUE;
	else if (TxBurst == 0)
		pAd->CommonCfg.bEnableTxBurst = FALSE;
	else
		return FALSE;  /*Invalid argument */

	DBGPRINT(RT_DEBUG_TRACE, ("Set_TxBurst_Proc::(TxBurst=%d)\n",
			pAd->CommonCfg.bEnableTxBurst));

	return TRUE;
}

#ifdef AGGREGATION_SUPPORT
/*
	==========================================================================
	Description:
		Set TxBurst
	Return:
		TRUE if all parameters are OK, FALSE otherwise
	==========================================================================
*/
BOOLEAN Set_PktAggregate_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	LONG aggre;

	aggre = simple_strtol(arg, 0, 10);

	if (aggre == 1)
		pAd->CommonCfg.bAggregationCapable = TRUE;
	else if (aggre == 0)
		pAd->CommonCfg.bAggregationCapable = FALSE;
	else
		return FALSE;  /*Invalid argument */

#ifdef CONFIG_AP_SUPPORT
#ifdef PIGGYBACK_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
		pAd->CommonCfg.bPiggyBackCapable = pAd->CommonCfg.bAggregationCapable;
		RTMPSetPiggyBack(pAd, pAd->CommonCfg.bPiggyBackCapable);
	}
#endif /* PIGGYBACK_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

	DBGPRINT(RT_DEBUG_TRACE, ("Set_PktAggregate_Proc::(AGGRE=%d)\n",
			pAd->CommonCfg.bAggregationCapable));

	return TRUE;
}
#endif


#ifdef INF_PPA_SUPPORT
BOOLEAN Set_INF_AMAZON_SE_PPA_Proc(PRTMP_ADAPTER pAd, PUCHAR arg)
{
	UINT status;
	UCHAR aggre;
	UINT32 g_if_id;

	aggre = simple_strtol(arg, 0, 10);

	if (ppa_hook_directpath_register_dev_fn == NULL) {
		DBGPRINT(RT_DEBUG_TRACE, ("%s::There is no PPA module\n", __FUNCTION__));
		return FALSE;
	}

	if (aggre == 1) {
		if (pAd->PPAEnable == TRUE) {
			DBGPRINT(RT_DEBUG_TRACE, ("PPA already enabled\n"));
		} else {
			if (pAd->pDirectpathCb == NULL) {
				DBGPRINT(RT_DEBUG_TRACE, ("Allocate memory for pDirectpathCb\n"));
				pAd->pDirectpathCb = os_alloc_mem(sizeof(PPA_DIRECTPATH_CB));
				if (!pAd->pDirectpathCb)
					return FALSE;
			}

			/* Register callback */
			pAd->pDirectpathCb->rx_fn = ifx_ra_start_xmit;
			pAd->pDirectpathCb->stop_tx_fn = NULL;
			pAd->pDirectpathCb->start_tx_fn = NULL;

			status = ppa_hook_directpath_register_dev_fn(
					&g_if_id, pAd->net_dev, pAd->pDirectpathCb,
					PPA_F_DIRECTPATH_REGISTER|PPA_F_DIRECTPATH_ETH_IF);

			if (status == IFX_SUCCESS) {
				pAd->g_if_id = g_if_id;
				pAd->PPAEnable = TRUE;
				DBGPRINT(RT_DEBUG_TRACE,
						("Register PPA success::ret=%d, id=%d\n",
						status, pAd->g_if_id));
			} else {
				DBGPRINT(RT_DEBUG_TRACE,
						("Register PPA fail::ret=%d\n",
						status));
			}
		}
	} else if (aggre == 0) {
		if (pAd->PPAEnable == FALSE) {
			DBGPRINT(RT_DEBUG_TRACE, ("PPA already disabled\n"));
		} else {
			g_if_id = pAd->g_if_id;
			status = ppa_hook_directpath_register_dev_fn(&g_if_id, pAd->net_dev, NULL, 0 /*PPA_F_DIRECTPATH_REGISTER*/);

			if (status == IFX_SUCCESS) {
				pAd->g_if_id = 0;
				pAd->PPAEnable = FALSE;
				DBGPRINT(RT_DEBUG_TRACE,
						("Unregister PPA success::ret=%d, if_id=%d\n",
						status, pAd->g_if_id));
			} else {
				DBGPRINT(RT_DEBUG_TRACE, ("Unregister PPA fail::ret=%d\n", status));
			}
		}
	} else {
		DBGPRINT(RT_DEBUG_TRACE, ("%s::Invalid argument=%d\n",
				__FUNCTION__, aggre));
		return FALSE;
	}

	return TRUE;
}
#endif /* INF_PPA_SUPPORT */


/*
	==========================================================================
	Description:
		Set IEEE80211H.
		This parameter is 1 when needs radar detection, otherwise 0
	Return:
		TRUE if all parameters are OK, FALSE otherwise
	==========================================================================
*/
BOOLEAN Set_IEEE80211H_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	LONG ieee80211h;

	ieee80211h = simple_strtol(arg, 0, 10);

	if (ieee80211h == 1)
		pAd->CommonCfg.bIEEE80211H = TRUE;
	else if (ieee80211h == 0)
		pAd->CommonCfg.bIEEE80211H = FALSE;
	else
		return FALSE;  /*Invalid argument */

	DBGPRINT(RT_DEBUG_TRACE, ("Set_IEEE80211H_Proc::(IEEE80211H=%d)\n",
			pAd->CommonCfg.bIEEE80211H));

	return TRUE;
}

#ifdef EXT_BUILD_CHANNEL_LIST
/*
	==========================================================================
	Description:
		Set Country Code.
	Return:
		TRUE if all parameters are OK, FALSE otherwise
	==========================================================================
*/
BOOLEAN Set_ExtCountryCode_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UCHAR CountryCode[3] = {0};

	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) == NDIS_STATUS_SUCCESS) {
		DBGPRINT(RT_DEBUG_TRACE,
				("%s can only be used when interface is down.\n",
				__FUNCTION__));
		return TRUE;
	}

	if (strlen(arg) == 2) {
		NdisMoveMemory(pAd->CommonCfg.CountryCode, arg, 2);
		pAd->CommonCfg.bCountryFlag = TRUE;
	} else {
		NdisZeroMemory(pAd->CommonCfg.CountryCode, 3);
		pAd->CommonCfg.bCountryFlag = FALSE;
	}

	NdisMoveMemory(CountryCode, pAd->CommonCfg.CountryCode, 2);
	DBGPRINT(RT_DEBUG_TRACE,
			("Set_CountryCode_Proc::(bCountryFlag=%d, CountryCode=%s)\n",
			pAd->CommonCfg.bCountryFlag,
			CountryCode));

	return TRUE;
}
/*
	==========================================================================
	Description:
		Set Ext DFS Type
	Return:
		TRUE if all parameters are OK, FALSE otherwise
	==========================================================================
*/
BOOLEAN Set_ExtDfsType_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UCHAR *pDfsType = &pAd->CommonCfg.DfsType;
	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) == NDIS_STATUS_SUCCESS) {
		DBGPRINT(RT_DEBUG_TRACE, ("%s can only be used when interface is down.\n",
				__FUNCTION__));
		return TRUE;
	}

	if (!strcmp(arg, "CE"))
		*pDfsType = CE;
	else if (!strcmp(arg, "FCC"))
		*pDfsType = FCC;
	else if (!strcmp(arg, "JAP"))
		*pDfsType = JAP;
	else
		DBGPRINT(RT_DEBUG_TRACE,
				("Unsupported DFS type:%s (Legal types are: CE, FCC, JAP)\n",
				arg));
	return TRUE;
}

/*
	==========================================================================
	Description:
		Add new channel list
	Return:
		TRUE if all parameters are OK, FALSE otherwise
	==========================================================================
*/
BOOLEAN Set_ChannelListAdd_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	CH_DESP inChDesp;
	PCH_REGION pChRegion = NULL;
	int loop = 0;

	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) == NDIS_STATUS_SUCCESS) {
		DBGPRINT(RT_DEBUG_TRACE,
				("%s can only be used when interface is down.\n",
				__FUNCTION__));
		return TRUE;
	}

	/* Get Channel Region (CountryCode)*/
	while (strcmp((PSTRING) ChRegion[loop].CountReg, "") != 0) {
		if (strncmp((PSTRING) ChRegion[loop].CountReg, pAd->CommonCfg.CountryCode, 2) == 0) {
			pChRegion = &ChRegion[loop];
			break;
		}
		loop++;
	}

	if (pChRegion == NULL) {
		DBGPRINT(RT_DEBUG_TRACE,
				("CountryCode is not configured or not valid\n"));
		return TRUE;
	}

	/* Parsing the arg, IN:arg; OUT:inChRegion */
	{
		UCHAR strBuff[64], count = 0;
		PUCHAR pStart, pEnd, tempIdx, tempBuff[5];

		if (strlen(arg) <64)
			NdisCopyMemory(strBuff, arg, strlen(arg));

		if ((pStart = rtstrchr(strBuff, '[')) != NULL) {
			if ((pEnd = rtstrchr(pStart++, ']')) != NULL) {
				tempBuff[count++] = pStart;
				for (tempIdx = pStart; tempIdx != pEnd; tempIdx++) {
					if (*tempIdx == ',') {
						*tempIdx = '\0';
						tempBuff[count++] = ++tempIdx;
					}
				}
				*(pEnd) = '\0';

				if (count != 5) {
					DBGPRINT(RT_DEBUG_TRACE,
							("Input Error. Too more or too less parameters.\n"));
					return TRUE;
				} else {
					inChDesp.FirstChannel = (UCHAR) simple_strtol(tempBuff[0], 0, 10);
					inChDesp.NumOfCh = (UCHAR) simple_strtol(tempBuff[1], 0, 10);
					inChDesp.MaxTxPwr = (UCHAR) simple_strtol(tempBuff[2], 0, 10);
					inChDesp.Geography = (!strcmp(tempBuff[3], "BOTH") ? BOTH: (!strcmp(tempBuff[3], "IDOR") ? IDOR : ODOR));
					inChDesp.DfsReq= (!strcmp(tempBuff[4], "TRUE") ? TRUE : FALSE);
				}
			} else {
				DBGPRINT(RT_DEBUG_TRACE, ("Missing End \"]\"\n"));
				return TRUE;
			}
		} else {
			DBGPRINT(RT_DEBUG_TRACE, ("%s: Invalid input format.\n", __FUNCTION__));
			return TRUE;
		}
	}

	/* Add entry to Channel List*/
	{
		UCHAR EntryIdx;
		PCH_DESP pChDesp = NULL;
		UCHAR CountryCode[3] = {0};
		if (pAd->CommonCfg.pChDesp == NULL) {
			pAd->CommonCfg.pChDesp = os_alloc_mem(MAX_PRECONFIG_DESP_ENTRY_SIZE*sizeof(CH_DESP));
			pChDesp = (PCH_DESP) pAd->CommonCfg.pChDesp;
			if (pChDesp) {
				for (EntryIdx = 0; pChRegion->pChDesp[EntryIdx].FirstChannel != 0; EntryIdx++) {
					if (EntryIdx == (MAX_PRECONFIG_DESP_ENTRY_SIZE-2)) {
						/* Keep an NULL entry in the end of table*/
						DBGPRINT(RT_DEBUG_TRACE, ("Table is full.\n"));
						return TRUE;
					}
					NdisCopyMemory(&pChDesp[EntryIdx], &pChRegion->pChDesp[EntryIdx], sizeof(CH_DESP));
				}
				/* Copy the NULL entry*/
				NdisCopyMemory(&pChDesp[EntryIdx], &pChRegion->pChDesp[EntryIdx], sizeof(CH_DESP));
			} else {
				return FALSE;
			}
		} else {
			pChDesp = (PCH_DESP) pAd->CommonCfg.pChDesp;
			for (EntryIdx= 0; pChDesp[EntryIdx].FirstChannel != 0; EntryIdx++) {
				if (EntryIdx ==  (MAX_PRECONFIG_DESP_ENTRY_SIZE-2)) {
					/* Keep an NULL entry in the end of table*/
					DBGPRINT(RT_DEBUG_TRACE, ("Table is full.\n"));
					return TRUE;
				}
			}
		}
		NdisMoveMemory(CountryCode, pAd->CommonCfg.CountryCode, 2);
		DBGPRINT(RT_DEBUG_TRACE, ("Add channel lists {%u, %u, %u, %s, %s} to %s.\n",
			inChDesp.FirstChannel,
			inChDesp.NumOfCh,
			inChDesp.MaxTxPwr,
			(inChDesp.Geography == BOTH) ? "BOTH" : (inChDesp.Geography == IDOR) ?  "IDOR" : "ODOR",
			(inChDesp.DfsReq == TRUE) ? "TRUE" : "FALSE",
			CountryCode));
		NdisCopyMemory(&pChDesp[EntryIdx], &inChDesp, sizeof(CH_DESP));
		pChDesp[++EntryIdx].FirstChannel = 0;
	}
	return TRUE;
}

BOOLEAN Set_ChannelListShow_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	PCH_REGION pChRegion = NULL;
	UCHAR EntryIdx, CountryCode[3]={0};

	/* Get Channel Region (CountryCode)*/

	int loop = 0;

	while (strcmp((PSTRING) ChRegion[loop].CountReg, "") != 0) {
		if (strncmp((PSTRING) ChRegion[loop].CountReg, pAd->CommonCfg.CountryCode, 2) == 0) {
			pChRegion = &ChRegion[loop];
			break;
		}
		loop++;
	}

	if (pChRegion == NULL) {
		DBGPRINT(RT_DEBUG_TRACE,
				("CountryCode is not configured or not valid\n"));
		return TRUE;
	}


	NdisMoveMemory(CountryCode, pAd->CommonCfg.CountryCode, 2);
	if (pAd->CommonCfg.DfsType == MAX_RD_REGION)
		pAd->CommonCfg.DfsType = pChRegion->DfsType;
	DBGPRINT(RT_DEBUG_ERROR, ("=========================================\n"));
	DBGPRINT(RT_DEBUG_ERROR, ("CountryCode:%s\n", CountryCode));
	DBGPRINT(RT_DEBUG_ERROR, ("DfsType:%s\n",
					(pAd->CommonCfg.DfsType == JAP) ? "JAP" :
					((pAd->CommonCfg.DfsType == FCC) ? "FCC" : "CE" )));

	if (pAd->CommonCfg.pChDesp != NULL) {
		PCH_DESP pChDesp = (PCH_DESP) pAd->CommonCfg.pChDesp;
		for (EntryIdx = 0; pChDesp[EntryIdx].FirstChannel != 0; EntryIdx++) {
			DBGPRINT(RT_DEBUG_ERROR, ("%u. {%3u, %2u, %2u, %s, %5s}.\n",
						EntryIdx,
						pChDesp[EntryIdx].FirstChannel,
						pChDesp[EntryIdx].NumOfCh,
						pChDesp[EntryIdx].MaxTxPwr,
						(pChDesp[EntryIdx].Geography == BOTH) ?
								"BOTH" : (pChDesp[EntryIdx].Geography == IDOR) ?
								"IDOR" : "ODOR",
						(pChDesp[EntryIdx].DfsReq == TRUE) ? "TRUE" : "FALSE"));
		}
	} else {
		DBGPRINT(RT_DEBUG_TRACE, ("Default channel list table:\n"));
		for (EntryIdx = 0; pChRegion->pChDesp[EntryIdx].FirstChannel != 0; EntryIdx++) {
			DBGPRINT(RT_DEBUG_ERROR, ("%u. {%3u, %2u, %2u, %s, %5s}.\n",
						EntryIdx,
						pChRegion->pChDesp[EntryIdx].FirstChannel,
						pChRegion->pChDesp[EntryIdx].NumOfCh,
						pChRegion->pChDesp[EntryIdx].MaxTxPwr,
						(pChRegion->pChDesp[EntryIdx].Geography == BOTH) ?
								"BOTH" : (pChRegion->pChDesp[EntryIdx].Geography == IDOR) ?
								"IDOR" : "ODOR",
						(pChRegion->pChDesp[EntryIdx].DfsReq == TRUE) ? "TRUE" : "FALSE"));
		}
	}
	DBGPRINT(RT_DEBUG_ERROR, ("=========================================\n"));
	return TRUE;
}

BOOLEAN Set_ChannelListDel_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UCHAR EntryIdx, TargetIdx, NumOfEntry;
	PCH_REGION pChRegion = NULL;
	PCH_DESP pChDesp = NULL;
	int loop;
	TargetIdx = simple_strtol(arg, 0, 10);

	if (RTMP_DRIVER_IOCTL_SANITY_CHECK(pAd, NULL) == NDIS_STATUS_SUCCESS) {
		DBGPRINT(RT_DEBUG_TRACE,
				("%s can only be used when interface is down.\n",
				__FUNCTION__));
		return TRUE;
	}

	/* Get Channel Region (CountryCode)*/
	loop = 0;
	while (strcmp((PSTRING) ChRegion[loop].CountReg, "") != 0) {
		if (strncmp((PSTRING) ChRegion[loop].CountReg, pAd->CommonCfg.CountryCode, 2) == 0) {
			pChRegion = &ChRegion[loop];
			break;
		}
		loop++;
	}

	if (pChRegion == NULL) {
		DBGPRINT(RT_DEBUG_TRACE, ("CountryCode is not configured or not valid\n"));
		return TRUE;
	}

	if (pAd->CommonCfg.pChDesp == NULL) {
		pAd->CommonCfg.pChDesp = os_alloc_mem(MAX_PRECONFIG_DESP_ENTRY_SIZE*sizeof(CH_DESP));
		if (pAd->CommonCfg.pChDesp) {
			pChDesp = (PCH_DESP) pAd->CommonCfg.pChDesp;
			for (EntryIdx= 0; pChRegion->pChDesp[EntryIdx].FirstChannel != 0; EntryIdx++) {
				if (EntryIdx == (MAX_PRECONFIG_DESP_ENTRY_SIZE-2)) {
					/* Keep an NULL entry in the end of table*/
					DBGPRINT(RT_DEBUG_TRACE, ("Table is full.\n"));
					return TRUE;
				}
				NdisCopyMemory(&pChDesp[EntryIdx],
						&pChRegion->pChDesp[EntryIdx],
						sizeof(CH_DESP));
			}
			/* Copy the NULL entry*/
			NdisCopyMemory(&pChDesp[EntryIdx],
					&pChRegion->pChDesp[EntryIdx], sizeof(CH_DESP));
		} else {
			return FALSE;
		}
	} else {
		pChDesp = (PCH_DESP) pAd->CommonCfg.pChDesp;
	}

	if (!strcmp(arg, "default")) {
		DBGPRINT(RT_DEBUG_TRACE, ("Default table used.\n" ));
		if (pAd->CommonCfg.pChDesp != NULL)
			os_free_mem(pAd->CommonCfg.pChDesp);
		pAd->CommonCfg.pChDesp = NULL;
		pAd->CommonCfg.DfsType = MAX_RD_REGION;
	} else if (!strcmp(arg, "all")) {
		DBGPRINT(RT_DEBUG_TRACE, ("Remove all entries.\n" ));
		for (EntryIdx = 0; EntryIdx < MAX_PRECONFIG_DESP_ENTRY_SIZE; EntryIdx++)
			NdisZeroMemory(&pChDesp[EntryIdx], sizeof(CH_DESP));
	} else if (TargetIdx < (MAX_PRECONFIG_DESP_ENTRY_SIZE-1)) {
		for (EntryIdx= 0; pChDesp[EntryIdx].FirstChannel != 0; EntryIdx++) {
			if (EntryIdx ==  (MAX_PRECONFIG_DESP_ENTRY_SIZE-2)) {
				/* Keep an NULL entry in the end of table */
				DBGPRINT(RT_DEBUG_TRACE, ("Last entry should be NULL.\n"));
				pChDesp[EntryIdx].FirstChannel = 0;
				return TRUE;
			}
		}
		NumOfEntry = EntryIdx;
		if (TargetIdx >= NumOfEntry) {
			DBGPRINT(RT_DEBUG_TRACE, ("Out of table range.\n"));
			return TRUE;
		}

		for (EntryIdx = TargetIdx; EntryIdx < NumOfEntry; EntryIdx++)
			NdisCopyMemory(&pChDesp[EntryIdx], &pChDesp[EntryIdx+1], sizeof(CH_DESP));

		NdisZeroMemory(&pChDesp[EntryIdx], sizeof(CH_DESP)); /*NULL entry*/
		DBGPRINT(RT_DEBUG_TRACE, ("Entry %u deleted.\n", TargetIdx));
	} else {
		DBGPRINT(RT_DEBUG_TRACE, ("Entry not found.\n"));
	}

	return TRUE;
}
#endif /* EXT_BUILD_CHANNEL_LIST  */


#ifdef DBG
/*
	==========================================================================
	Description:
		For Debug information
	Return:
		TRUE if all parameters are OK, FALSE otherwise
	==========================================================================
*/
BOOLEAN Set_Debug_Proc(RTMP_ADAPTER *pAd, PSTRING arg)
{
	u32 dbg;

	DBGPRINT(RT_DEBUG_TRACE, ("==>%s()\n", __FUNCTION__));

	dbg = simple_strtol(arg, 0, 10);
	if ( dbg <= RT_DEBUG_LOUD)
		RTDebugLevel = dbg;

	DBGPRINT(RT_DEBUG_TRACE, ("<==%s(RTDebugLevel = %d)\n",
			__FUNCTION__, RTDebugLevel));

	return TRUE;
}


/*
	==========================================================================
	Description:
		For DebugFunc information
	Return:
		TRUE if all parameters are OK, FALSE otherwise
	==========================================================================
*/
BOOLEAN Set_DebugFunc_Proc(RTMP_ADAPTER *pAd, PSTRING arg)
{
	DBGPRINT(RT_DEBUG_TRACE, ("==>%s()\n", __FUNCTION__));
	RTDebugFunc = simple_strtol(arg, 0, 10);
	DBGPRINT(RT_DEBUG_TRACE, ("Set RTDebugFunc = 0x%lx\n", RTDebugFunc));

	return TRUE;
}
#endif


BOOLEAN Show_DescInfo_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	return TRUE;
}

/*
	==========================================================================
	Description:
		Reset statistics counter

	Arguments:
		pAd            Pointer to our adapter
		arg

	Return:
		TRUE if all parameters are OK, FALSE otherwise
	==========================================================================
*/
BOOLEAN Set_ResetStatCounter_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	DBGPRINT(RT_DEBUG_TRACE, ("==>Set_ResetStatCounter_Proc\n"));

	/* add the most up-to-date h/w raw counters into software counters*/
	NICUpdateRawCounters(pAd);

	NdisZeroMemory(&pAd->WlanCounters, sizeof(COUNTER_802_11));
	NdisZeroMemory(&pAd->Counters8023, sizeof(COUNTER_802_3));
	NdisZeroMemory(&pAd->RalinkCounters, sizeof(COUNTER_RALINK));

#ifdef TXBF_SUPPORT
	if (pAd->chipCap.FlgHwTxBfCap) {
		int i;
		for (i = 0; i < MAX_LEN_OF_MAC_TABLE; i++)
			NdisZeroMemory(&pAd->MacTab.Content[i].TxBFCounters,
					sizeof(pAd->MacTab.Content[i].TxBFCounters));
	}
#endif /* TXBF_SUPPORT */

	return TRUE;
}


static BOOLEAN RTMPCheckStrPrintAble(CHAR *pInPutStr, UCHAR strLen)
{
	UCHAR i;

	for (i = 0; i < strLen; i++) {
		if ((pInPutStr[i] < 0x20) || (pInPutStr[i] > 0x7E))
			return FALSE;
	}

	return TRUE;
}


/*
	========================================================================

	Routine Description:
		Remove WPA Key process

	Arguments:
		pAd 		Pointer to our adapter
		pBuf		Pointer to the where the key stored

	Return Value:
		NDIS_SUCCESS	Add key successfully

	IRQL = DISPATCH_LEVEL

	Note:

	========================================================================
*/
#ifdef CONFIG_STA_SUPPORT
void RTMPSetDesiredRates(RTMP_ADAPTER *pAd, LONG Rates)
{
	NDIS_802_11_RATES aryRates;
	struct wifi_dev *wdev = &pAd->StaCfg.wdev;

	memset(&aryRates, 0x00, sizeof(NDIS_802_11_RATES));
	switch (pAd->CommonCfg.PhyMode) {
	case (WMODE_A): /* A only*/
		switch (Rates) {
		case 6000000: /*6M*/
			aryRates[0] = 0x0c; /* 6M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_0;
			break;
		case 9000000: /*9M*/
			aryRates[0] = 0x12; /* 9M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_1;
			break;
		case 12000000: /*12M*/
			aryRates[0] = 0x18; /* 12M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_2;
			break;
		case 18000000: /*18M*/
			aryRates[0] = 0x24; /* 18M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_3;
			break;
		case 24000000: /*24M*/
			aryRates[0] = 0x30; /* 24M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_4;
			break;
		case 36000000: /*36M*/
			aryRates[0] = 0x48; /* 36M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_5;
			break;
		case 48000000: /*48M*/
			aryRates[0] = 0x60; /* 48M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_6;
			break;
		case 54000000: /*54M*/
			aryRates[0] = 0x6c; /* 54M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_7;
			break;
		case -1: /*Auto*/
		default:
			aryRates[0] = 0x6c; /* 54Mbps*/
			aryRates[1] = 0x60; /* 48Mbps*/
			aryRates[2] = 0x48; /* 36Mbps*/
			aryRates[3] = 0x30; /* 24Mbps*/
			aryRates[4] = 0x24; /* 18M*/
			aryRates[5] = 0x18; /* 12M*/
			aryRates[6] = 0x12; /* 9M*/
			aryRates[7] = 0x0c; /* 6M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_AUTO;
			break;
		}
		break;
	case (WMODE_B | WMODE_G): /* B/G Mixed*/
	case (WMODE_B): /* B only*/
	case (WMODE_A | WMODE_B | WMODE_G): /* A/B/G Mixed*/
	default:
		switch (Rates) {
		case 1000000: /*1M*/
			aryRates[0] = 0x02;
			wdev->DesiredTransmitSetting.field.MCS = MCS_0;
			break;
		case 2000000: /*2M*/
			aryRates[0] = 0x04;
			wdev->DesiredTransmitSetting.field.MCS = MCS_1;
			break;
		case 5000000: /*5.5M*/
			aryRates[0] = 0x0b; /* 5.5M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_2;
			break;
		case 11000000: /*11M*/
			aryRates[0] = 0x16; /* 11M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_3;
			break;
		case 6000000: /*6M*/
			aryRates[0] = 0x0c; /* 6M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_0;
			break;
		case 9000000: /*9M*/
			aryRates[0] = 0x12; /* 9M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_1;
			break;
		case 12000000: /*12M*/
			aryRates[0] = 0x18; /* 12M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_2;
			break;
		case 18000000: /*18M*/
			aryRates[0] = 0x24; /* 18M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_3;
			break;
		case 24000000: /*24M*/
			aryRates[0] = 0x30; /* 24M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_4;
			break;
		case 36000000: /*36M*/
			aryRates[0] = 0x48; /* 36M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_5;
			break;
		case 48000000: /*48M*/
			aryRates[0] = 0x60; /* 48M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_6;
			break;
		case 54000000: /*54M*/
			aryRates[0] = 0x6c; /* 54M*/
			wdev->DesiredTransmitSetting.field.MCS = MCS_7;
			break;
		case -1: /*Auto*/
		default:
			if (pAd->CommonCfg.PhyMode == WMODE_B) {
				/*B Only*/
				aryRates[0] = 0x16; /* 11Mbps*/
				aryRates[1] = 0x0b; /* 5.5Mbps*/
				aryRates[2] = 0x04; /* 2Mbps*/
				aryRates[3] = 0x02; /* 1Mbps*/
			} else {
				/*(B/G) Mixed or (A/B/G) Mixed*/
				aryRates[0] = 0x6c; /* 54Mbps*/
				aryRates[1] = 0x60; /* 48Mbps*/
				aryRates[2] = 0x48; /* 36Mbps*/
				aryRates[3] = 0x30; /* 24Mbps*/
				aryRates[4] = 0x16; /* 11Mbps*/
				aryRates[5] = 0x0b; /* 5.5Mbps*/
				aryRates[6] = 0x04; /* 2Mbps*/
				aryRates[7] = 0x02; /* 1Mbps*/
			}
			wdev->DesiredTransmitSetting.field.MCS = MCS_AUTO;
			break;
		}
		break;
	}

	NdisZeroMemory(pAd->CommonCfg.DesireRate, MAX_LEN_OF_SUPPORTED_RATES);
	NdisMoveMemory(pAd->CommonCfg.DesireRate, &aryRates, sizeof(NDIS_802_11_RATES));
	DBGPRINT(RT_DEBUG_TRACE, (" RTMPSetDesiredRates (%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x)\n",
		pAd->CommonCfg.DesireRate[0], pAd->CommonCfg.DesireRate[1],
		pAd->CommonCfg.DesireRate[2], pAd->CommonCfg.DesireRate[3],
		pAd->CommonCfg.DesireRate[4], pAd->CommonCfg.DesireRate[5],
		pAd->CommonCfg.DesireRate[6], pAd->CommonCfg.DesireRate[7] ));
	/* Changing DesiredRate may affect the MAX TX rate we used to TX frames out*/
	MlmeUpdateTxRates(pAd, FALSE, 0);
}
#endif /* CONFIG_STA_SUPPORT */


#if defined(CONFIG_STA_SUPPORT) || defined(WPA_SUPPLICANT_SUPPORT)
NDIS_STATUS RTMPWPARemoveKeyProc(PRTMP_ADAPTER pAd, PVOID pBuf)
{
	PNDIS_802_11_REMOVE_KEY pKey;
	ULONG KeyIdx;
	NDIS_STATUS Status = NDIS_STATUS_FAILURE;
	BOOLEAN bTxKey; 		/* Set the key as transmit key*/
	BOOLEAN bPairwise;		/* Indicate the key is pairwise key*/
	BOOLEAN bKeyRSC;		/* indicate the receive  SC set by KeyRSC value.*/
					/* Otherwise, it will set by the NIC.*/
	BOOLEAN bAuthenticator; 	/* indicate key is set by authenticator.*/
	int i;
#ifdef APCLI_SUPPORT
#ifdef WPA_SUPPLICANT_SUPPORT
	UCHAR ifIndex;
	BOOLEAN apcliEn=FALSE;
	int idx, BssIdx;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
#endif/*WPA_SUPPLICANT_SUPPORT*/
#endif/*APCLI_SUPPORT*/
	DBGPRINT(RT_DEBUG_TRACE,("---> RTMPWPARemoveKeyProc\n"));

	pKey = (PNDIS_802_11_REMOVE_KEY) pBuf;
	KeyIdx = pKey->KeyIndex & 0xff;
	/* Bit 31 of Add-key, Tx Key*/
	bTxKey = (pKey->KeyIndex & 0x80000000) ? TRUE : FALSE;
	/* Bit 30 of Add-key PairwiseKey*/
	bPairwise = (pKey->KeyIndex & 0x40000000) ? TRUE : FALSE;
	/* Bit 29 of Add-key KeyRSC*/
	bKeyRSC = (pKey->KeyIndex & 0x20000000) ? TRUE : FALSE;
	/* Bit 28 of Add-key Authenticator*/
	bAuthenticator = (pKey->KeyIndex & 0x10000000) ? TRUE : FALSE;

	/* 1. If bTx is TRUE, return failure information*/
	if (bTxKey == TRUE)
		return(NDIS_STATUS_INVALID_DATA);

	/* 2. Check Pairwise Key*/
	if (bPairwise) {
		/* a. If BSSID is broadcast, remove all pairwise keys.*/
		/* b. If not broadcast, remove the pairwise specified by BSSID*/
		for (i = 0; i < SHARE_KEY_NUM; i++) {
#ifdef APCLI_SUPPORT
#ifdef WPA_SUPPLICANT_SUPPORT
			if (pObj->ioctl_if_type == INT_APCLI) {
				/*if (MAC_ADDR_EQUAL(pAd->ApCfg.ApCliTab[ifIndex].SharedKey[i].BssId, pKey->BSSID)) */
				{
					ifIndex = pObj->ioctl_if;
					BssIdx = pAd->ApCfg.BssidNum + MAX_MESH_NUM + ifIndex;
					DBGPRINT(RT_DEBUG_TRACE,("APCLI RTMPWPARemoveKeyProc(KeyIdx=%d)\n", i));
					pAd->ApCfg.ApCliTab[ifIndex].SharedKey[i].KeyLen = 0;
					pAd->ApCfg.ApCliTab[ifIndex].SharedKey[i].CipherAlg = CIPHER_NONE;
					AsicRemoveSharedKeyEntry(pAd, BssIdx, (UCHAR)i);
					Status = NDIS_STATUS_SUCCESS;
					break;
				}
			} else
#endif /*WPA_SUPPLICANT_SUPPORT*/
#endif /*APCLI_SUPPORT*/
			{
#ifdef CONFIG_STA_SUPPORT
				if (MAC_ADDR_EQUAL(pAd->SharedKey[BSS0][i].BssId, pKey->BSSID)) {
					DBGPRINT(RT_DEBUG_TRACE,("RTMPWPARemoveKeyProc(KeyIdx=%d)\n", i));
					pAd->SharedKey[BSS0][i].KeyLen = 0;
					pAd->SharedKey[BSS0][i].CipherAlg = CIPHER_NONE;
					AsicRemoveSharedKeyEntry(pAd, BSS0, (UCHAR)i);
					Status = NDIS_STATUS_SUCCESS;
					break;
				}
#endif /*CONFIG_STA_SUPPORT*/
			}
		}
	} else {
		/* 3. Group Key*/
		/* a. If BSSID is broadcast, remove all group keys indexed*/
		/* b. If BSSID matched, delete the group key indexed.*/
		DBGPRINT(RT_DEBUG_TRACE,("RTMPWPARemoveKeyProc(KeyIdx=%ld)\n", KeyIdx));
		pAd->SharedKey[BSS0][KeyIdx].KeyLen = 0;
		pAd->SharedKey[BSS0][KeyIdx].CipherAlg = CIPHER_NONE;
		AsicRemoveSharedKeyEntry(pAd, BSS0, (UCHAR)KeyIdx);
		Status = NDIS_STATUS_SUCCESS;
	}

	return Status;
}
#endif /* defined(CONFIG_STA_SUPPORT) || defined(WPA_SUPPLICANT_SUPPORT) */


#ifdef CONFIG_STA_SUPPORT
/*
	========================================================================

	Routine Description:
		Remove All WPA Keys

	Arguments:
		pAd 		Pointer to our adapter

	Return Value:
		None

	IRQL = DISPATCH_LEVEL

	Note:

	========================================================================
*/
void RTMPWPARemoveAllKeys(RTMP_ADAPTER *pAd)
{
	UCHAR i;
	struct wifi_dev *wdev = &pAd->StaCfg.wdev;

	DBGPRINT(RT_DEBUG_TRACE,("RTMPWPARemoveAllKeys(AuthMode=%d, WepStatus=%d)\n",
			wdev->AuthMode, wdev->WepStatus));
	/*
		For WEP/CKIP/WPA-None, there is no need to remove it, since WinXP won't set it
		again after Link up. And it will be replaced if user changed it.
	*/
	if (wdev->AuthMode < Ndis802_11AuthModeWPA ||
		wdev->AuthMode == Ndis802_11AuthModeWPANone)
		return;

#ifdef PCIE_PS_SUPPORT
		RTMP_CLEAR_PSFLAG(pAd, fRTMP_PS_CAN_GO_SLEEP);
#endif /* PCIE_PS_SUPPORT */

	/* set BSSID wcid entry of the Pair-wise Key table as no-security mode*/
	AsicRemovePairwiseKeyEntry(pAd, BSSID_WCID);

	/* set all shared key mode as no-security. */
	for (i = 0; i < SHARE_KEY_NUM; i++) {
		DBGPRINT(RT_DEBUG_TRACE,("remove %s key #%d\n",
				CipherName[pAd->SharedKey[BSS0][i].CipherAlg], i));
		NdisZeroMemory(&pAd->SharedKey[BSS0][i], sizeof(CIPHER_KEY));
		AsicRemoveSharedKeyEntry(pAd, BSS0, i);
	}
#ifdef PCIE_PS_SUPPORT
	RTMP_SET_PSFLAG(pAd, fRTMP_PS_CAN_GO_SLEEP);
#endif /* PCIE_PS_SUPPORT */

}
#endif /* CONFIG_STA_SUPPORT */


/*
	========================================================================
	Routine Description:
		Change NIC PHY mode. Re-association may be necessary

	Arguments:
		pAd - Pointer to our adapter
		phymode  -

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL

	========================================================================
*/
void RTMPSetPhyMode(RTMP_ADAPTER *pAd, ULONG phymode)
{
	int i;

	/* the selected phymode must be supported by the RF IC encoded in E2PROM*/
	pAd->CommonCfg.PhyMode = (UCHAR)phymode;

	DBGPRINT(RT_DEBUG_TRACE,("RTMPSetPhyMode : PhyMode=%d, channel=%d \n",
			pAd->CommonCfg.PhyMode, pAd->CommonCfg.Channel));
#ifdef EXT_BUILD_CHANNEL_LIST
	BuildChannelListEx(pAd);
#else
	BuildChannelList(pAd);
#endif /* EXT_BUILD_CHANNEL_LIST */

	/* sanity check user setting*/
	for (i = 0; i < pAd->ChannelListNum; i++) {
		if (pAd->CommonCfg.Channel == pAd->ChannelList[i].Channel)
			break;
	}

	if (i == pAd->ChannelListNum) {
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
			if (pAd->CommonCfg.Channel != 0)
				pAd->CommonCfg.Channel = FirstChannel(pAd);
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
			pAd->CommonCfg.Channel = FirstChannel(pAd);
#endif /* CONFIG_STA_SUPPORT */
		DBGPRINT(RT_DEBUG_ERROR,
				("RTMPSetPhyMode: channel is out of range, use first channel=%d \n",
				pAd->CommonCfg.Channel));
	}

	NdisZeroMemory(pAd->CommonCfg.SupRate, MAX_LEN_OF_SUPPORTED_RATES);
	NdisZeroMemory(pAd->CommonCfg.ExtRate, MAX_LEN_OF_SUPPORTED_RATES);
	NdisZeroMemory(pAd->CommonCfg.DesireRate, MAX_LEN_OF_SUPPORTED_RATES);
	switch (phymode) {
	case (WMODE_B):
		pAd->CommonCfg.SupRate[0] = 0x82;	/* 1 mbps, in units of 0.5 Mbps, basic rate*/
		pAd->CommonCfg.SupRate[1] = 0x84;	/* 2 mbps, in units of 0.5 Mbps, basic rate*/
		pAd->CommonCfg.SupRate[2] = 0x8B;	/* 5.5 mbps, in units of 0.5 Mbps, basic rate*/
		pAd->CommonCfg.SupRate[3] = 0x96;	/* 11 mbps, in units of 0.5 Mbps, basic rate*/
		pAd->CommonCfg.SupRateLen = 4;
		pAd->CommonCfg.ExtRateLen = 0;
		pAd->CommonCfg.DesireRate[0] = 2;	/* 1 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.DesireRate[1] = 4;	/* 2 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.DesireRate[2] = 11;	/* 5.5 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.DesireRate[3] = 22;	/* 11 mbps, in units of 0.5 Mbps*/
		/*pAd->CommonCfg.HTPhyMode.field.MODE = MODE_CCK;  This MODE is only FYI. not use*/
		break;
		/*
		In current design, we will put supported/extended rate element in
		beacon even we are 11n-only mode.
		Or some 11n stations will not connect to us if we do not put
		supported/extended rate element in beacon.
		*/
	case (WMODE_B | WMODE_G):
	case (WMODE_A | WMODE_B | WMODE_G):
#ifdef DOT11_N_SUPPORT
	case (WMODE_A | WMODE_B | WMODE_G | WMODE_GN | WMODE_AN):
	case (WMODE_B | WMODE_G | WMODE_GN):
#endif /* DOT11_N_SUPPORT */
#ifdef DOT11_VHT_AC
	case  (WMODE_B | WMODE_G | WMODE_GN |WMODE_A | WMODE_AN | WMODE_AC):
#endif /* DOT11_VHT_AC */
		pAd->CommonCfg.SupRate[0] = 0x82;	/* 1 mbps, in units of 0.5 Mbps, basic rate*/
		pAd->CommonCfg.SupRate[1] = 0x84;	/* 2 mbps, in units of 0.5 Mbps, basic rate*/
		pAd->CommonCfg.SupRate[2] = 0x8B;	/* 5.5 mbps, in units of 0.5 Mbps, basic rate*/
		pAd->CommonCfg.SupRate[3] = 0x96;	/* 11 mbps, in units of 0.5 Mbps, basic rate*/
		pAd->CommonCfg.SupRate[4] = 0x12;	/* 9 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.SupRate[5] = 0x24;	/* 18 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.SupRate[6] = 0x48;	/* 36 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.SupRate[7] = 0x6c;	/* 54 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.SupRateLen = 8;
		pAd->CommonCfg.ExtRate[0] = 0x0C;	/* 6 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.ExtRate[1] = 0x18;	/* 12 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.ExtRate[2] = 0x30;	/* 24 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.ExtRate[3] = 0x60;	/* 48 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.ExtRateLen = 4;
		pAd->CommonCfg.DesireRate[0] = 2;	/* 1 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.DesireRate[1] = 4;	/* 2 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.DesireRate[2] = 11;	/* 5.5 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.DesireRate[3] = 22;	/* 11 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.DesireRate[4] = 12;	/* 6 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.DesireRate[5] = 18;	/* 9 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.DesireRate[6] = 24;	/* 12 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.DesireRate[7] = 36;	/* 18 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.DesireRate[8] = 48;	/* 24 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.DesireRate[9] = 72;	/* 36 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.DesireRate[10] = 96;	/* 48 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.DesireRate[11] = 108;	/* 54 mbps, in units of 0.5 Mbps*/
		break;

	case (WMODE_A):
	case (WMODE_G):
#ifdef DOT11_N_SUPPORT
	case (WMODE_A | WMODE_AN):
	case (WMODE_A | WMODE_G | WMODE_GN | WMODE_AN):
	case (WMODE_G | WMODE_GN):
	case (WMODE_GN):
	case (WMODE_AN):
#endif /* DOT11_N_SUPPORT */
#ifdef DOT11_VHT_AC
	case (WMODE_A | WMODE_AN | WMODE_AC):
	case (WMODE_AN | WMODE_AC):
	case (WMODE_G | WMODE_GN |WMODE_A | WMODE_AN | WMODE_AC):
#endif /* DOT11_VHT_AC */
		pAd->CommonCfg.SupRate[0] = 0x8C;	/* 6 mbps, in units of 0.5 Mbps, basic rate*/
		pAd->CommonCfg.SupRate[1] = 0x12;	/* 9 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.SupRate[2] = 0x98;	/* 12 mbps, in units of 0.5 Mbps, basic rate*/
		pAd->CommonCfg.SupRate[3] = 0x24;	/* 18 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.SupRate[4] = 0xb0;	/* 24 mbps, in units of 0.5 Mbps, basic rate*/
		pAd->CommonCfg.SupRate[5] = 0x48;	/* 36 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.SupRate[6] = 0x60;	/* 48 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.SupRate[7] = 0x6c;	/* 54 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.SupRateLen = 8;
		pAd->CommonCfg.ExtRateLen = 0;
		pAd->CommonCfg.DesireRate[0] = 12;	/* 6 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.DesireRate[1] = 18;	/* 9 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.DesireRate[2] = 24;	/* 12 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.DesireRate[3] = 36;	/* 18 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.DesireRate[4] = 48;	/* 24 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.DesireRate[5] = 72;	/* 36 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.DesireRate[6] = 96;	/* 48 mbps, in units of 0.5 Mbps*/
		pAd->CommonCfg.DesireRate[7] = 108;	/* 54 mbps, in units of 0.5 Mbps*/
		break;
	default:
		break;
	}

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
		UINT apidx;

		for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++) {
			MlmeUpdateTxRates(pAd, FALSE, apidx);
		}
#ifdef APCLI_SUPPORT
		for (apidx = 0; apidx < MAX_APCLI_NUM; apidx++) {
			MlmeUpdateTxRates(pAd, FALSE, apidx + MIN_NET_DEVICE_FOR_APCLI);
		}
#endif /* APCLI_SUPPORT */
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd) {
		printk("%s: Update for STA\n", __FUNCTION__);
		MlmeUpdateTxRates(pAd, FALSE, BSS0);
	}
#endif /* CONFIG_STA_SUPPORT */

//CFG_TODO
#ifdef RT_CFG80211_P2P_SUPPORT
	NdisZeroMemory(pAd->cfg80211_ctrl.P2pSupRate, MAX_LEN_OF_SUPPORTED_RATES);
	NdisZeroMemory(pAd->cfg80211_ctrl.P2pExtRate, MAX_LEN_OF_SUPPORTED_RATES);

	pAd->cfg80211_ctrl.P2pSupRate[0] = 0x8C;	/* 6 mbps, in units of 0.5 Mbps, basic rate*/
	pAd->cfg80211_ctrl.P2pSupRate[1] = 0x12;	/* 9 mbps, in units of 0.5 Mbps*/
	pAd->cfg80211_ctrl.P2pSupRate[2] = 0x98;	/* 12 mbps, in units of 0.5 Mbps, basic rate*/
	pAd->cfg80211_ctrl.P2pSupRate[3] = 0x24;	/* 18 mbps, in units of 0.5 Mbps*/
	pAd->cfg80211_ctrl.P2pSupRate[4] = 0xb0;	/* 24 mbps, in units of 0.5 Mbps, basic rate*/
	pAd->cfg80211_ctrl.P2pSupRate[5] = 0x48;	/* 36 mbps, in units of 0.5 Mbps*/
	pAd->cfg80211_ctrl.P2pSupRate[6] = 0x60;	/* 48 mbps, in units of 0.5 Mbps*/
	pAd->cfg80211_ctrl.P2pSupRate[7] = 0x6c;	/* 54 mbps, in units of 0.5 Mbps*/
	pAd->cfg80211_ctrl.P2pSupRateLen = 8;
	pAd->cfg80211_ctrl.P2pExtRateLen = 0;

#ifdef RT_CFG80211_P2P_CONCURRENT_DEVICE
	MlmeUpdateTxRates(pAd, FALSE, MAIN_MBSSID + MIN_NET_DEVICE_FOR_CFG80211_VIF_P2P_GO);
	MlmeUpdateTxRates(pAd, FALSE, MAIN_MBSSID + MIN_NET_DEVICE_FOR_APCLI);
#endif
#endif /* RT_CFG80211_P2P_SUPPORT */

#ifdef DOT11_N_SUPPORT
	SetCommonHT(pAd);
#endif

#ifdef DOT11_VHT_AC
	SetCommonVHT(pAd);
#endif
}


/*
	========================================================================
	Description:
		Add Client security information into ASIC WCID table and IVEIV table.
	Return:
	========================================================================
*/
void RTMPAddWcidAttributeEntry(PRTMP_ADAPTER pAd, UCHAR BssIdx, UCHAR KeyIdx,
	UCHAR CipherAlg, MAC_TABLE_ENTRY *pEntry)
{
	UINT32 WCIDAttri = 0;
	USHORT offset;
	UCHAR IVEIV = 0;
	USHORT Wcid = 0;
#ifdef CONFIG_AP_SUPPORT
	BOOLEAN IEEE8021X = FALSE;
#endif

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
#ifdef APCLI_SUPPORT
		if (BssIdx >= MIN_NET_DEVICE_FOR_APCLI) {
			if (pEntry) {
				BssIdx -= MIN_NET_DEVICE_FOR_APCLI;
			} else {
				DBGPRINT(RT_DEBUG_WARN,
						("RTMPAddWcidAttributeEntry: AP-Client link doesn't need to set Group WCID Attribute. \n"));
				return;
			}
		} else
#endif /* APCLI_SUPPORT */
		{
			if (BssIdx >= pAd->ApCfg.BssidNum) {
				DBGPRINT(RT_DEBUG_ERROR,
						("RTMPAddWcidAttributeEntry: The BSS-index(%d) is out of range for MBSSID link. \n",
						BssIdx));
				return;
			}
		}

		/* choose wcid number*/
		if (pEntry)
			Wcid = pEntry->wcid;
		else
			GET_GroupKey_WCID(pAd, Wcid, BssIdx);
#ifdef DOT1X_SUPPORT
		if ((BssIdx < pAd->ApCfg.BssidNum) && (BssIdx < MAX_MBSSID_NUM(pAd)) && (BssIdx < HW_BEACON_MAX_NUM))
			IEEE8021X = pAd->ApCfg.MBSSID[BssIdx].wdev.IEEE8021X;
#endif
	}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd) {
		if (BssIdx > BSS0) {
			DBGPRINT(RT_DEBUG_ERROR,
					("RTMPAddWcidAttributeEntry: The BSS-index(%d) is out of range for Infra link. \n",
					BssIdx));
			return;
		}

		/*
			1.	In ADHOC mode, the AID is wcid number. And NO mesh link exists.
			2.	In Infra mode, the AID:1 MUST be wcid of infra STA.
						the AID:2~ assign to mesh link entry.
		*/
		if (pEntry)
			Wcid = pEntry->wcid;
		else
			Wcid = MCAST_WCID;
	}
#endif /* CONFIG_STA_SUPPORT */

	/* Update WCID attribute table*/
	{
		UINT32 wcid_attr_base = 0, wcid_attr_size = 0;
#ifdef RLT_MAC
		if (pAd->chipCap.hif_type == HIF_RLT) {
			wcid_attr_base = RLT_MAC_WCID_ATTRIBUTE_BASE;
			wcid_attr_size = RLT_HW_WCID_ATTRI_SIZE;
		}
#endif
#ifdef RTMP_MAC
		if (pAd->chipCap.hif_type == HIF_RTMP) {
			wcid_attr_base = MAC_WCID_ATTRIBUTE_BASE;
			wcid_attr_size = HW_WCID_ATTRI_SIZE;
		}
#endif
		offset = wcid_attr_base + (Wcid * wcid_attr_size);
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
			/*
				1.	Wds-links and Mesh-links always use Pair-wise key table.
				2. 	When the CipherAlg is TKIP, AES or the dynamic WEP is enabled,
					it needs to set key into Pair-wise Key Table.
				3.	The pair-wise key security mode is set NONE, it means as no security.
			*/
			if (pEntry && (IS_ENTRY_WDS(pEntry) || IS_ENTRY_MESH(pEntry)))
				WCIDAttri = (BssIdx<<4) | (CipherAlg<<1) | PAIRWISEKEYTABLE;
			else if ((pEntry) &&
					((CipherAlg == CIPHER_TKIP) ||
					 (CipherAlg == CIPHER_AES) ||
					 (CipherAlg == CIPHER_NONE) ||
					 (IEEE8021X == TRUE)))
				WCIDAttri = (BssIdx<<4) | (CipherAlg<<1) | PAIRWISEKEYTABLE;
			else
				WCIDAttri = (BssIdx<<4) | (CipherAlg<<1) | SHAREDKEYTABLE;
		}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd) {
			if (pEntry && IS_ENTRY_MESH(pEntry))
				WCIDAttri = (CipherAlg<<1) | PAIRWISEKEYTABLE;
			else
				WCIDAttri = (CipherAlg<<1) | SHAREDKEYTABLE;
		}
#endif /* CONFIG_STA_SUPPORT */

		RTMP_IO_WRITE32(pAd, offset, WCIDAttri);
	}

	/* Update IV/EIV table*/
	{
		UINT32 iveiv_tb_base = 0, iveiv_tb_size = 0;
#ifdef RLT_MAC
		if (pAd->chipCap.hif_type == HIF_RLT) {
			iveiv_tb_base = RLT_MAC_IVEIV_TABLE_BASE;
			iveiv_tb_size = RLT_HW_IVEIV_ENTRY_SIZE;
		}
#endif

#ifdef RTMP_MAC
		if (pAd->chipCap.hif_type == HIF_RTMP) {
			iveiv_tb_base = MAC_IVEIV_TABLE_BASE;
			iveiv_tb_size = HW_IVEIV_ENTRY_SIZE;
		}
#endif
		offset = iveiv_tb_base + (Wcid * iveiv_tb_size);

		/* WPA mode*/
		if ((CipherAlg == CIPHER_TKIP) || (CipherAlg == CIPHER_AES)) {
			/* Eiv bit on. keyid always is 0 for pairwise key */
			IVEIV = (KeyIdx <<6) | 0x20;
		} else {
			/* WEP KeyIdx is default tx key. */
			IVEIV = (KeyIdx << 6);
		}

		/* For key index and ext IV bit, so only need to update the position(offset+3).*/
#ifdef RTMP_MAC_USB
		RTUSBMultiWrite_OneByte(pAd, offset+3, &IVEIV);
#endif /* RTMP_MAC_USB */
	}
	DBGPRINT(RT_DEBUG_TRACE,
			("RTMPAddWcidAttributeEntry: WCID #%d, KeyIndex #%d, Alg=%s\n",
			Wcid, KeyIdx, CipherName[CipherAlg]));
	DBGPRINT(RT_DEBUG_TRACE,(" WCIDAttri = 0x%x \n",  WCIDAttri));
}

/*
	==========================================================================
	Description:
		Parse encryption type
Arguments:
	pAdapter	Pointer to our adapter
	wrq		Pointer to the ioctl argument

	==========================================================================
*/
PSTRING GetEncryptType(CHAR enc)
{
	if (enc == Ndis802_11WEPDisabled)
		return "NONE";
	if (enc == Ndis802_11WEPEnabled)
		return "WEP";
	if (enc == Ndis802_11TKIPEnable)
		return "TKIP";
	if (enc == Ndis802_11AESEnable)
		return "AES";
	if (enc == Ndis802_11TKIPAESMix)
		return "TKIPAES";
	else
		return "UNKNOW";
}

PSTRING GetAuthMode(CHAR auth)
{
	if (auth == Ndis802_11AuthModeOpen)
		return "OPEN";
	if (auth == Ndis802_11AuthModeShared)
		return "SHARED";
	if (auth == Ndis802_11AuthModeAutoSwitch)
		return "AUTOWEP";
	if (auth == Ndis802_11AuthModeWPA)
		return "WPA";
	if (auth == Ndis802_11AuthModeWPAPSK)
		return "WPAPSK";
	if (auth == Ndis802_11AuthModeWPANone)
		return "WPANONE";
	if (auth == Ndis802_11AuthModeWPA2)
		return "WPA2";
	if (auth == Ndis802_11AuthModeWPA2PSK)
		return "WPA2PSK";
	if (auth == Ndis802_11AuthModeWPA1WPA2)
		return "WPA1WPA2";
	if (auth == Ndis802_11AuthModeWPA1PSKWPA2PSK)
		return "WPA1PSKWPA2PSK";

	return "UNKNOW";
}


/*
	==========================================================================
	Description:
		Get site survey results
	Arguments:
		pAdapter	Pointer to our adapter
		wrq		Pointer to the ioctl argument

	Return Value:
		None

	Note:
		Usage:
				1.) UI needs to wait 4 seconds after issue a site survey command
				2.) iwpriv ra0 get_site_survey
				3.) UI needs to prepare at least 4096bytes to get the results
	==========================================================================
*/
#define	LINE_LEN	(4+33+20+23+9+7+7+3)	/* Channel+SSID+Bssid+Security+Signal+WiressMode+ExtCh+NetworkType*/

void RTMPCommSiteSurveyData(PSTRING msg, BSS_ENTRY *pBss, UINT32 MsgLen)
{
	int Rssi = 0;
	UINT Rssi_Quality = 0;
	NDIS_802_11_NETWORK_TYPE wireless_mode;
	CHAR Ssid[MAX_LEN_OF_SSID +1];
	STRING SecurityStr[32] = {0};
	NDIS_802_11_ENCRYPTION_STATUS ap_cipher = Ndis802_11EncryptionDisabled;
	NDIS_802_11_AUTHENTICATION_MODE ap_auth_mode = Ndis802_11AuthModeOpen;

	/*Channel*/
	sprintf(msg+strlen(msg),"%-4d", pBss->Channel);

	/*SSID*/
	NdisZeroMemory(Ssid, (MAX_LEN_OF_SSID +1));
	if (RTMPCheckStrPrintAble((PCHAR)pBss->Ssid, pBss->SsidLen)) {
		NdisMoveMemory(Ssid, pBss->Ssid, pBss->SsidLen);
	} else {
		int idx = 0;
		sprintf(Ssid, "0x");
		for (idx = 0; (idx < 14) && (idx < pBss->SsidLen); idx++)
			sprintf(Ssid + 2 + (idx*2), "%02X", (UCHAR)pBss->Ssid[idx]);
	}
	sprintf(msg+strlen(msg),"%-33s", Ssid);

	/*BSSID*/
	sprintf(msg+strlen(msg),"%02x:%02x:%02x:%02x:%02x:%02x   ", pBss->Bssid[0],
			pBss->Bssid[1], pBss->Bssid[2], pBss->Bssid[3],
			pBss->Bssid[4], pBss->Bssid[5]);

	/*Security*/
	RTMPZeroMemory(SecurityStr, 32);
	if ((Ndis802_11AuthModeWPA <= pBss->AuthMode) &&
		(pBss->AuthMode <= Ndis802_11AuthModeWPA1PSKWPA2PSK)) {
		if (pBss->AuthMode == Ndis802_11AuthModeWPANone) {
			ap_auth_mode = pBss->AuthMode;
				ap_cipher = pBss->WPA.PairCipher;
		} else if (pBss->AuthModeAux == Ndis802_11AuthModeOpen) {
			ap_auth_mode = pBss->AuthMode;
			if ((ap_auth_mode == Ndis802_11AuthModeWPA) ||
				(ap_auth_mode == Ndis802_11AuthModeWPAPSK)) {
				if (pBss->WPA.PairCipherAux == Ndis802_11WEPDisabled)
					ap_cipher = pBss->WPA.PairCipher;
				else
					ap_cipher = Ndis802_11TKIPAESMix;
			} else if ((ap_auth_mode == Ndis802_11AuthModeWPA2) ||
				(ap_auth_mode == Ndis802_11AuthModeWPA2PSK)) {
				if (pBss->WPA2.PairCipherAux == Ndis802_11WEPDisabled)
					ap_cipher = pBss->WPA2.PairCipher;
				else
					ap_cipher = Ndis802_11TKIPAESMix;
			}
		} else if ((pBss->AuthMode == Ndis802_11AuthModeWPAPSK) ||
			 (pBss->AuthMode == Ndis802_11AuthModeWPA2PSK)) {
			if ((pBss->AuthModeAux == Ndis802_11AuthModeWPAPSK) ||
				(pBss->AuthModeAux == Ndis802_11AuthModeWPA2PSK))
				ap_auth_mode = Ndis802_11AuthModeWPA1PSKWPA2PSK;
			else
				ap_auth_mode = pBss->AuthMode;

			if (pBss->WPA.PairCipher != pBss->WPA2.PairCipher)
				ap_cipher = Ndis802_11TKIPAESMix;
			else if ((pBss->WPA.PairCipher == pBss->WPA2.PairCipher) &&
					 (pBss->WPA.PairCipherAux != pBss->WPA2.PairCipherAux))
				ap_cipher = Ndis802_11TKIPAESMix;
			else if ((pBss->WPA.PairCipher == pBss->WPA2.PairCipher) &&
				 (pBss->WPA.PairCipherAux == pBss->WPA2.PairCipherAux) &&
				 (pBss->WPA.PairCipherAux != Ndis802_11WEPDisabled))
				ap_cipher = Ndis802_11TKIPAESMix;
			else if ((pBss->WPA.PairCipher == pBss->WPA2.PairCipher) &&
				 (pBss->WPA.PairCipherAux == pBss->WPA2.PairCipherAux) &&
				 (pBss->WPA.PairCipherAux == Ndis802_11WEPDisabled))
				ap_cipher = pBss->WPA.PairCipher;
		} else if ((pBss->AuthMode == Ndis802_11AuthModeWPA) ||
				 (pBss->AuthMode == Ndis802_11AuthModeWPA2)) {
			if ((pBss->AuthModeAux == Ndis802_11AuthModeWPA) ||
				(pBss->AuthModeAux == Ndis802_11AuthModeWPA2))
				ap_auth_mode = Ndis802_11AuthModeWPA1WPA2;
			else
				ap_auth_mode = pBss->AuthMode;

			if (pBss->WPA.PairCipher != pBss->WPA2.PairCipher)
				ap_cipher = Ndis802_11TKIPAESMix;
			else if ((pBss->WPA.PairCipher == pBss->WPA2.PairCipher) &&
				 (pBss->WPA.PairCipherAux != pBss->WPA2.PairCipherAux))
				ap_cipher = Ndis802_11TKIPAESMix;
			else if ((pBss->WPA.PairCipher == pBss->WPA2.PairCipher) &&
				 (pBss->WPA.PairCipherAux == pBss->WPA2.PairCipherAux) &&
				 (pBss->WPA.PairCipherAux != Ndis802_11WEPDisabled))
				ap_cipher = Ndis802_11TKIPAESMix;
			else if ((pBss->WPA.PairCipher == pBss->WPA2.PairCipher) &&
				 (pBss->WPA.PairCipherAux == pBss->WPA2.PairCipherAux) &&
				 (pBss->WPA.PairCipherAux == Ndis802_11WEPDisabled))
				ap_cipher = pBss->WPA.PairCipher;
		}

		sprintf(SecurityStr, "%s/%s", GetAuthMode((CHAR)ap_auth_mode),
				GetEncryptType((CHAR)ap_cipher));
	} else {
		ap_auth_mode = pBss->AuthMode;
		ap_cipher = pBss->WepStatus;
		if (ap_cipher == Ndis802_11WEPDisabled)
			sprintf(SecurityStr, "NONE");
		else if (ap_cipher == Ndis802_11WEPEnabled)
			sprintf(SecurityStr, "WEP");
		else
			sprintf(SecurityStr, "%s/%s", GetAuthMode((CHAR)ap_auth_mode),
					GetEncryptType((CHAR)ap_cipher));
	}

	sprintf(msg+strlen(msg), "%-23s", SecurityStr);

	/* Rssi*/
	Rssi = (int)pBss->Rssi;
	if (Rssi >= -50)
		Rssi_Quality = 100;
	else if (Rssi >= -80)    /* between -50 ~ -80dbm*/
		Rssi_Quality = (UINT)(24 + ((Rssi + 80) * 26)/10);
	else if (Rssi >= -90)   /* between -80 ~ -90dbm*/
		Rssi_Quality = (UINT)(((Rssi + 90) * 26)/10);
	else    /* < -84 dbm*/
		Rssi_Quality = 0;

	sprintf(msg+strlen(msg),"%-9d", Rssi_Quality);

	/* Wireless Mode*/
	wireless_mode = NetworkTypeInUseSanity(pBss);
	if (wireless_mode == Ndis802_11FH ||
		wireless_mode == Ndis802_11DS)
		sprintf(msg+strlen(msg),"%-7s", "11b");
	else if (wireless_mode == Ndis802_11OFDM5)
		sprintf(msg+strlen(msg),"%-7s", "11a");
	else if (wireless_mode == Ndis802_11OFDM5_N)
		sprintf(msg+strlen(msg),"%-7s", "11a/n");
	else if (wireless_mode == Ndis802_11OFDM5_AC)
		sprintf(msg+strlen(msg),"%-7s", "11a/n/ac");
	else if (wireless_mode == Ndis802_11OFDM24)
		sprintf(msg+strlen(msg),"%-7s", "11b/g");
	else if (wireless_mode == Ndis802_11OFDM24_N)
		sprintf(msg+strlen(msg),"%-7s", "11b/g/n");
	else
		sprintf(msg+strlen(msg),"%-7s", "unknow");

	/* Ext Channel*/
	if (pBss->AddHtInfoLen > 0) {
		if (pBss->AddHtInfo.AddHtInfo.ExtChanOffset == EXTCHA_ABOVE)
			sprintf(msg+strlen(msg),"%-7s", " ABOVE");
		else if (pBss->AddHtInfo.AddHtInfo.ExtChanOffset == EXTCHA_BELOW)
			sprintf(msg+strlen(msg),"%-7s", " BELOW");
		else
			sprintf(msg+strlen(msg),"%-7s", " NONE");
	} else {
		sprintf(msg+strlen(msg),"%-7s", " NONE");
	}

	/*Network Type */
	if (pBss->BssType == BSS_ADHOC)
		sprintf(msg+strlen(msg),"%-3s", " Ad");
	else
		sprintf(msg+strlen(msg),"%-3s", " In");

	sprintf(msg+strlen(msg),"\n");
}

#if defined (AP_SCAN_SUPPORT) || defined (CONFIG_STA_SUPPORT)
void RTMPIoctlGetSiteSurvey(PRTMP_ADAPTER pAdapter, RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	PSTRING msg;
	int i;
	int WaitCnt;
	int status;
	int max_len = LINE_LEN;
	BSS_ENTRY *pBss;
	UINT32 TotalLen, BufLen = IW_SCAN_MAX_DATA;

#ifdef CONFIG_STA_SUPPORT
#endif /* CONFIG_STA_SUPPORT */

	TotalLen = sizeof(CHAR)*((MAX_LEN_OF_BSS_TABLE)*max_len) + 100;

	if (wrq->u.data.length == 0)
		BufLen = IW_SCAN_MAX_DATA;
	else
		BufLen = wrq->u.data.length;

	msg = os_alloc_mem(TotalLen);
	if (msg == NULL) {
		return;
	}

	memset(msg, 0 , TotalLen);
	sprintf(msg,"%s","\n");
	sprintf(msg+strlen(msg),"%-4s%-33s%-20s%-23s%-9s%-7s%-7s%-3s\n",
		"Ch", "SSID", "BSSID", "Security", "Siganl(%)", "W-Mode", " ExtCH"," NT");

	WaitCnt = 0;
#ifdef CONFIG_STA_SUPPORT
	pAdapter->StaCfg.bSkipAutoScanConn = TRUE;
#endif

	while ((ScanRunning(pAdapter) == TRUE) && (WaitCnt++ < 200))
		OS_WAIT(500);

	for (i = 0; i < pAdapter->ScanTab.BssNr; i++) {
		pBss = &pAdapter->ScanTab.BssEntry[i];

		if (pBss->Channel == 0)
			break;

		if ((strlen(msg)+100 ) >= BufLen)
			break;

		RTMPCommSiteSurveyData(msg, pBss, TotalLen);
	}

#ifdef CONFIG_STA_SUPPORT
	pAdapter->StaCfg.bSkipAutoScanConn = FALSE;
#endif
	wrq->u.data.length = strlen(msg);
	status = copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length);

	DBGPRINT(RT_DEBUG_TRACE,
			("RTMPIoctlGetSiteSurvey - wrq->u.data.length = %d\n",
			wrq->u.data.length));
	os_free_mem(msg);
}
#endif


void RTMPIoctlGetMacTableStaInfo(PRTMP_ADAPTER pAd, RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	int i;
	RT_802_11_MAC_TABLE *pMacTab = NULL;
	PRT_802_11_MAC_ENTRY pDst;
	MAC_TABLE_ENTRY *pEntry;

	/* allocate memory */
	pMacTab = os_alloc_mem(sizeof(RT_802_11_MAC_TABLE));
	if (pMacTab == NULL) {
		return;
	}

	NdisZeroMemory(pMacTab, sizeof(RT_802_11_MAC_TABLE));
	for (i = 0; i < MAX_LEN_OF_MAC_TABLE; i++) {
		pEntry = &(pAd->MacTab.Content[i]);
		if (IS_ENTRY_CLIENT(pEntry) && (pEntry->Sst == SST_ASSOC)) {
			pDst = &pMacTab->Entry[pMacTab->Num];
			pDst->ApIdx = pEntry->apidx;
			COPY_MAC_ADDR(pDst->Addr, &pEntry->Addr);
			pDst->Aid = (UCHAR)pEntry->Aid;
			pDst->Psm = pEntry->PsMode;
#ifdef DOT11_N_SUPPORT
			pDst->MimoPs = pEntry->MmpsMode;
#endif
			/* Fill in RSSI per entry*/
			pDst->AvgRssi0 = pEntry->RssiSample.AvgRssi0;
			pDst->AvgRssi1 = pEntry->RssiSample.AvgRssi1;
			pDst->AvgRssi2 = pEntry->RssiSample.AvgRssi2;

			/* the connected time per entry*/
			pDst->ConnectedTime = pEntry->StaConnectTime;
			pDst->TxRate.word = pEntry->HTPhyMode.word;
			pDst->LastRxRate = pEntry->LastRxRate;
			pMacTab->Num += 1;
		}
	}

	wrq->u.data.length = sizeof(RT_802_11_MAC_TABLE);
	if (copy_to_user(wrq->u.data.pointer, pMacTab, wrq->u.data.length)) {
		DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n",
				__FUNCTION__));
	}

	if (pMacTab != NULL)
		os_free_mem(pMacTab);
}


#define	MAC_LINE_LEN	(1+14+4+4+4+4+10+10+10+6+6)	/* "\n"+Addr+aid+psm+datatime+rxbyte+txbyte+current tx rate+last tx rate+"\n" */
void RTMPIoctlGetMacTable(PRTMP_ADAPTER pAd, RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	int i;
	RT_802_11_MAC_TABLE *pMacTab = NULL;
	RT_802_11_MAC_ENTRY *pDst;
	MAC_TABLE_ENTRY *pEntry;
	char *msg;

	/* allocate memory */
	pMacTab = os_alloc_mem(sizeof(RT_802_11_MAC_TABLE));
	if (pMacTab == NULL) {
		return;
	}

	NdisZeroMemory(pMacTab, sizeof(RT_802_11_MAC_TABLE));
	pMacTab->Num = 0;
	for (i = 0; i < MAX_LEN_OF_MAC_TABLE; i++) {
		pEntry = &(pAd->MacTab.Content[i]);

		if (IS_ENTRY_CLIENT(pEntry) && (pEntry->Sst == SST_ASSOC)) {
			pDst = &pMacTab->Entry[pMacTab->Num];
			pDst->ApIdx = (UCHAR)pEntry->apidx;
			COPY_MAC_ADDR(pDst->Addr, &pEntry->Addr);
			pDst->Aid = (UCHAR)pEntry->Aid;
			pDst->Psm = pEntry->PsMode;
#ifdef DOT11_N_SUPPORT
			pDst->MimoPs = pEntry->MmpsMode;
#endif
			/* Fill in RSSI per entry*/
			pDst->AvgRssi0 = pEntry->RssiSample.AvgRssi0;
			pDst->AvgRssi1 = pEntry->RssiSample.AvgRssi1;
			pDst->AvgRssi2 = pEntry->RssiSample.AvgRssi2;

			/* The connected time per entry*/
			pDst->ConnectedTime = pEntry->StaConnectTime;
			pDst->TxRate.word = pEntry->HTPhyMode.word;
			pMacTab->Num += 1;
		}
	}

	wrq->u.data.length = sizeof(RT_802_11_MAC_TABLE);
	if (copy_to_user(wrq->u.data.pointer, pMacTab, wrq->u.data.length)) {
		DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));
	}

	msg = os_alloc_mem(sizeof(CHAR)*(MAX_LEN_OF_MAC_TABLE*MAC_LINE_LEN));
	if (msg == NULL) {
		goto LabelOK;
	}
	memset(msg, 0 ,MAX_LEN_OF_MAC_TABLE*MAC_LINE_LEN );
	sprintf(msg,"%s","\n");
	sprintf(msg+strlen(msg),"%-14s%-4s%-4s%-4s%-4s%-6s%-6s%-10s%-10s%-10s\n",
		"MAC", "AP",  "AID", "PSM", "AUTH", "CTxR", "LTxR","LDT", "RxB", "TxB");

	for (i = 0; i < MAX_LEN_OF_MAC_TABLE; i++) {
		MAC_TABLE_ENTRY *pEntry = &pAd->MacTab.Content[i];

		if (IS_ENTRY_CLIENT(pEntry) && (pEntry->Sst == SST_ASSOC)) {
			if ((strlen(msg)+MAC_LINE_LEN ) >= (MAX_LEN_OF_MAC_TABLE*MAC_LINE_LEN) )
				break;
			sprintf(msg+strlen(msg),"%02x%02x%02x%02x%02x%02x  ", PRINT_MAC(pEntry->Addr));
			sprintf(msg+strlen(msg),"%-4d", (int)pEntry->apidx);
			sprintf(msg+strlen(msg),"%-4d", (int)pEntry->Aid);
			sprintf(msg+strlen(msg),"%-4d", (int)pEntry->PsMode);
			sprintf(msg+strlen(msg),"%-4d", (int)pEntry->AuthState);
			sprintf(msg+strlen(msg),"%-6d",RateIdToMbps[pAd->MacTab.Content[i].CurrTxRate]);
			sprintf(msg+strlen(msg),"%-6d",0/*RateIdToMbps[pAd->MacTab.Content[i].HTPhyMode.word]*/); /* ToDo*/
			sprintf(msg+strlen(msg),"%-10d",0/*pAd->MacTab.Content[i].HSCounter.LastDataPacketTime*/); /* ToDo*/
			sprintf(msg+strlen(msg),"%-10d",0/*pAd->MacTab.Content[i].HSCounter.TotalRxByteCount*/); /* ToDo*/
			sprintf(msg+strlen(msg),"%-10d\n",0/*pAd->MacTab.Content[i].HSCounter.TotalTxByteCount*/); /* ToDo*/
		}
	}

	//JB WTF? Just a debug message?
	DBGPRINT(RT_DEBUG_TRACE, ("%s", msg));
	os_free_mem(msg);

LabelOK:
	if (pMacTab != NULL)
		os_free_mem(pMacTab);
}

#if defined(INF_AR9) || defined(BB_SOC)
#if defined(AR9_MAPI_SUPPORT) || defined(BB_SOC)
#ifdef CONFIG_AP_SUPPORT
void RTMPAR9IoctlGetMacTable(PRTMP_ADAPTER pAd, RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	int i;
	char *msg = os_alloc_mem(sizeof(CHAR)*(MAX_LEN_OF_MAC_TABLE*MAC_LINE_LEN));
	if (msg == NULL) {
		return;
	}
	memset(msg, 0 ,MAX_LEN_OF_MAC_TABLE*MAC_LINE_LEN );
	sprintf(msg,"%s","\n");
	sprintf(msg+strlen(msg),"%-14s%-4s%-4s%-4s%-4s%-6s%-6s%-10s%-10s%-10s\n",
		"MAC", "AP",  "AID", "PSM", "AUTH", "CTxR", "LTxR","LDT", "RxB", "TxB");

	for (i = 0; i < MAX_LEN_OF_MAC_TABLE; i++) {
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];
		if (IS_ENTRY_CLIENT(pEntry) && (pEntry->Sst == SST_ASSOC)) {
			if ((strlen(msg)+MAC_LINE_LEN ) >= (MAX_LEN_OF_MAC_TABLE*MAC_LINE_LEN) )
				break;
			sprintf(msg+strlen(msg),"%02x%02x%02x%02x%02x%02x  ",
					pEntry->Addr[0], pEntry->Addr[1],
					pEntry->Addr[2], pEntry->Addr[3],
					pEntry->Addr[4], pEntry->Addr[5]);
			sprintf(msg+strlen(msg),"%-4d", (int)pEntry->apidx);
			sprintf(msg+strlen(msg),"%-4d", (int)pEntry->Aid);
			sprintf(msg+strlen(msg),"%-4d", (int)pEntry->PsMode);
			sprintf(msg+strlen(msg),"%-4d", (int)pEntry->AuthState);
			sprintf(msg+strlen(msg),"%-6d",RateIdToMbps[pAd->MacTab.Content[i].CurrTxRate]);
			sprintf(msg+strlen(msg),"%-6d",0/*RateIdToMbps[pAd->MacTab.Content[i].HTPhyMode.word]*/); /* ToDo*/
			sprintf(msg+strlen(msg),"%-10d",0/*pAd->MacTab.Content[i].HSCounter.LastDataPacketTime*/); /* ToDo*/
			sprintf(msg+strlen(msg),"%-10d",0/*pAd->MacTab.Content[i].HSCounter.TotalRxByteCount*/); /* ToDo*/
			sprintf(msg+strlen(msg),"%-10d\n",0/*pAd->MacTab.Content[i].HSCounter.TotalTxByteCount*/); /* ToDo*/
		}
	}
	/* for compatible with old API just do the printk to console*/
	wrq->u.data.length = strlen(msg);
	if (copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length)) {
		DBGPRINT(RT_DEBUG_OFF, ("%s", msg));
	}

	os_free_mem(msg);
}

void RTMPIoctlGetSTAT2(PRTMP_ADAPTER pAd, RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	char *msg;
	PMULTISSID_STRUCT pMbss;
	int apidx;

	msg = os_alloc_mem(sizeof(CHAR)*(pAd->ApCfg.BssidNum*(14*128)));
	if (msg == NULL) {
		return;
	}
	memset(msg, 0 ,pAd->ApCfg.BssidNum*(14*128));
	sprintf(msg,"%s","\n");

	for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++) {
		pMbss = &pAd->ApCfg.MBSSID[apidx];

		sprintf(msg+strlen(msg),"ra%d\n",apidx);
		sprintf(msg+strlen(msg),"bytesTx = %ld\n",(pMbss->TransmittedByteCount));
		sprintf(msg+strlen(msg),"bytesRx = %ld\n",(pMbss->ReceivedByteCount));
		sprintf(msg+strlen(msg),"pktsTx = %ld\n",pMbss->TxCount);
		sprintf(msg+strlen(msg),"pktsRx = %ld\n",pMbss->RxCount);
		sprintf(msg+strlen(msg),"errorsTx = %ld\n",pMbss->TxErrorCount);
		sprintf(msg+strlen(msg),"errorsRx = %ld\n",pMbss->RxErrorCount);
		sprintf(msg+strlen(msg),"discardPktsTx = %ld\n",pMbss->TxDropCount);
		sprintf(msg+strlen(msg),"discardPktsRx = %ld\n",pMbss->RxDropCount);
		sprintf(msg+strlen(msg),"ucPktsTx = %ld\n",pMbss->ucPktsTx);
		sprintf(msg+strlen(msg),"ucPktsRx = %ld\n",pMbss->ucPktsRx);
		sprintf(msg+strlen(msg),"mcPktsTx = %ld\n",pMbss->mcPktsTx);
		sprintf(msg+strlen(msg),"mcPktsRx = %ld\n",pMbss->mcPktsRx);
		sprintf(msg+strlen(msg),"bcPktsTx = %ld\n",pMbss->bcPktsTx);
		sprintf(msg+strlen(msg),"bcPktsRx = %ld\n",pMbss->bcPktsRx);
	}

	wrq->u.data.length = strlen(msg);
	if (copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length)) {
		DBGPRINT(RT_DEBUG_OFF, ("%s", msg));
	}

	os_free_mem(msg);
}


void RTMPIoctlGetRadioDynInfo(PRTMP_ADAPTER pAd, RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	char *msg;
	PMULTISSID_STRUCT pMbss;
	int status,bandwidth,ShortGI;
	struct wifi_dev *wdev;

	msg = os_alloc_mem(sizeof(CHAR)*(4096));
	if (msg == NULL) {
		return;
	}
	memset(msg, 0 ,4096);
	sprintf(msg,"%s","\n");

	pMbss = &pAd->ApCfg.MBSSID[0];
	wdev = &pMbss->wdev;
	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF))
		status = 0;
	else
		status = 1;

	if (pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth  == BW_40)
		bandwidth = 1;
	else
		bandwidth = 0;

	if (pAd->CommonCfg.RegTransmitSetting.field.ShortGI == GI_800)
		ShortGI = 1;
	else
		ShortGI = 0;

	sprintf(msg+strlen(msg),"status = %d\n",status);
	sprintf(msg+strlen(msg),"channelsInUse = %d\n",pAd->ChannelListNum);
	sprintf(msg+strlen(msg),"channel = %d\n",pAd->CommonCfg.Channel);
	sprintf(msg+strlen(msg),"chanWidth = %d\n",bandwidth);
	sprintf(msg+strlen(msg),"guardIntvl = %d\n",ShortGI);
	sprintf(msg+strlen(msg),"MCS = %d\n",wdev->DesiredTransmitSetting.field.MCS);

	wrq->u.data.length = strlen(msg);

	if (copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length)) {
		DBGPRINT(RT_DEBUG_OFF, ("%s", msg));
	}

	os_free_mem(msg);
}
#endif /*CONFIG_AP_SUPPORT*/
#endif /*AR9_MAPI_SUPPORT*/
#endif /* INF_AR9 */

#ifdef DOT11_N_SUPPORT
BOOLEAN Set_BASetup_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UCHAR mac[6], tid;
	PSTRING token;
	STRING sepValue[] = ":", DASH = '-';
	int i;
	MAC_TABLE_ENTRY *pEntry;

/*
	The BASetup inupt string format should be xx:xx:xx:xx:xx:xx-d,
		=>The six 2 digit hex-decimal number previous are the Mac address,
		=>The seventh decimal number is the tid value.
*/
	/*DBGPRINT(RT_DEBUG_TRACE,("\n%s\n", arg));*/

	/*Mac address acceptable format 01:02:03:04:05:06 length 17 plus
	 * the "-" and tid value in decimal format.*/
	if (strlen(arg) < 19)
		return FALSE;

	token = strchr(arg, DASH);
	if ((token != NULL) && (strlen(token) > 1)) {
		tid = (UCHAR) simple_strtol((token+1), 0, 10);
		/* tid is 0 ~ 7; Or kernel will crash in BAOriSessionSetUp() */
		if (tid > (NUM_OF_TID-1))
			return FALSE;

		*token = '\0';
		for (i = 0, token = rstrtok(arg, &sepValue[0]); token; token = rstrtok(NULL, &sepValue[0]), i++) {
			if ((strlen(token) != 2) || (!isxdigit(*token)) || (!isxdigit(*(token+1))))
				return FALSE;
			AtoH(token, (&mac[i]), 1);
		}
		if (i != 6)
			return FALSE;

		DBGPRINT(RT_DEBUG_OFF, ("\n%02x:%02x:%02x:%02x:%02x:%02x-%02x\n",
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], tid));

		pEntry = MacTableLookup(pAd, (PUCHAR) mac);
		if (pEntry) {
			DBGPRINT(RT_DEBUG_OFF, ("\nSetup BA Session: Tid = %d\n", tid));
			BAOriSessionSetUp(pAd, pEntry, tid, 0, 100, TRUE);
		}

		return TRUE;
	}

	return FALSE;
}

BOOLEAN Set_BADecline_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG bBADecline = simple_strtol(arg, 0, 10);
	if (bBADecline == 0) {
		pAd->CommonCfg.bBADecline = FALSE;
	} else if (bBADecline == 1) {
		pAd->CommonCfg.bBADecline = TRUE;
	} else {
		return FALSE; /*Invalid argument*/
	}

	DBGPRINT(RT_DEBUG_TRACE, ("Set_BADecline_Proc::(BADecline=%d)\n",
			pAd->CommonCfg.bBADecline));

	return TRUE;
}

BOOLEAN Set_BAOriTearDown_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UCHAR mac[6], tid;
	PSTRING token;
	STRING sepValue[] = ":", DASH = '-';
	int i;
	MAC_TABLE_ENTRY *pEntry;

	/*DBGPRINT(RT_DEBUG_TRACE,("\n%s\n", arg));*/
/*
	The BAOriTearDown inupt string format should be xx:xx:xx:xx:xx:xx-d,
		=>The six 2 digit hex-decimal number previous are the Mac address,
		=>The seventh decimal number is the tid value.
*/
	/*Mac address acceptable format 01:02:03:04:05:06 length 17 plus
	 * the "-" and tid value in decimal format.*/
	if (strlen(arg) < 19)
		return FALSE;

	token = strchr(arg, DASH);
	if ((token != NULL) && (strlen(token) > 1)) {
		tid = simple_strtol((token+1), 0, 10);
		/* tid will be 0 ~ 7; Or kernel will crash in BAOriSessionTearDown() */
		if (tid > (NUM_OF_TID-1))
			return FALSE;

		*token = '\0';
		for (i = 0, token = rstrtok(arg, &sepValue[0]); token; token = rstrtok(NULL, &sepValue[0]), i++) {
			if ((strlen(token) != 2) || (!isxdigit(*token)) || (!isxdigit(*(token+1))))
				return FALSE;
			AtoH(token, (&mac[i]), 1);
		}
		if (i != 6)
			return FALSE;

		DBGPRINT(RT_DEBUG_OFF, ("\n%02x:%02x:%02x:%02x:%02x:%02x-%02x",
				PRINT_MAC(mac), tid));

		pEntry = MacTableLookup(pAd, (PUCHAR) mac);

		if (pEntry) {
			DBGPRINT(RT_DEBUG_OFF, ("\nTear down Ori BA Session: Tid = %d\n", tid));
			BAOriSessionTearDown(pAd, pEntry->wcid, tid, FALSE, TRUE);
		}

		return TRUE;
	}

	return FALSE;
}

BOOLEAN Set_BARecTearDown_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UCHAR mac[6], tid;
	PSTRING token;
	STRING sepValue[] = ":", DASH = '-';
	int i;
	MAC_TABLE_ENTRY *pEntry;

	/*DBGPRINT(RT_DEBUG_TRACE,("\n%s\n", arg));*/
/*
	The BARecTearDown inupt string format should be xx:xx:xx:xx:xx:xx-d,
		=>The six 2 digit hex-decimal number previous are the Mac address,
		=>The seventh decimal number is the tid value.
*/
	/*Mac address acceptable format 01:02:03:04:05:06 length 17 plus
	 * the "-" and tid value in decimal format.*/
	if (strlen(arg) < 19)
		return FALSE;

	token = strchr(arg, DASH);
	if ((token != NULL) && (strlen(token) > 1)) {
		tid = simple_strtol((token+1), 0, 10);
		/* tid will be 0 ~ 7; Or kernel will crash in BARecSessionTearDown() */
		if (tid > (NUM_OF_TID-1))
			return FALSE;

		*token = '\0';
		for (i = 0, token = rstrtok(arg, &sepValue[0]); token; token = rstrtok(NULL, &sepValue[0]), i++) {
			if ((strlen(token) != 2) || (!isxdigit(*token)) || (!isxdigit(*(token+1))))
				return FALSE;
			AtoH(token, (&mac[i]), 1);
		}
		if (i != 6)
			return FALSE;

		DBGPRINT(RT_DEBUG_OFF, ("\n%02x:%02x:%02x:%02x:%02x:%02x-%02x",
				PRINT_MAC(mac), tid));

		pEntry = MacTableLookup(pAd, (PUCHAR) mac);

		if (pEntry) {
			DBGPRINT(RT_DEBUG_OFF,
					("\nTear down Rec BA Session: Tid = %d\n",
					tid));
			BARecSessionTearDown(pAd, pEntry->wcid, tid, FALSE);
		}

		return TRUE;
	}

	return FALSE;
}

BOOLEAN Set_HtBw_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG HtBw = simple_strtol(arg, 0, 10);

	if (HtBw == BW_40)
		pAd->CommonCfg.RegTransmitSetting.field.BW = BW_40;
	else if (HtBw == BW_20)
		pAd->CommonCfg.RegTransmitSetting.field.BW = BW_20;
	else
		return FALSE;  /*Invalid argument */

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtBw_Proc::(HtBw=%d)\n",
			pAd->CommonCfg.RegTransmitSetting.field.BW));

#ifdef DFS_ATP_SUPPORT
	pAd->CommonCfg.RadarDetect.atp_set_ht_bw = TRUE;
	if (pAd->CommonCfg.RadarDetect.atp_set_channel_ready) {
		printk ("BW and Channel is ready\n");
	}
#endif
	return TRUE;
}


BOOLEAN Set_HtMcs_Proc(RTMP_ADAPTER *pAd, PSTRING arg)
{
#ifdef CONFIG_AP_SUPPORT
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
#endif
#ifdef CONFIG_STA_SUPPORT
	BOOLEAN bAutoRate = FALSE;
#endif
	UCHAR HtMcs = MCS_AUTO, Mcs_tmp, ValidMcs = 15;
#ifdef DOT11_VHT_AC
	PSTRING mcs_str, ss_str;
	UCHAR ss = 0, mcs = 0;
#endif
	struct wifi_dev *wdev;

#ifdef DOT11_VHT_AC
	ss_str = arg;
	if ((mcs_str = rtstrchr(arg, ':')) != NULL) {
		*mcs_str = 0;
		mcs_str++;

		DBGPRINT(RT_DEBUG_TRACE, ("%s(): ss_str=%s, mcs_str=%s\n",
				__FUNCTION__, ss_str, mcs_str));

		if (strlen(ss_str) && strlen(mcs_str)) {
			mcs = simple_strtol(mcs_str, 0, 10);
			ss = simple_strtol(ss_str, 0, 10);

			if ((ss <= pAd->CommonCfg.TxStream) && (mcs <= 9)) {
				HtMcs = ((ss - 1) <<4) | mcs;
			} else {
				HtMcs = MCS_AUTO;
				ss = 0;
			}
			DBGPRINT(RT_DEBUG_TRACE, ("%s(): %dSS-MCS%d, Auto=%s\n",
					__FUNCTION__, ss, mcs,
					(HtMcs == MCS_AUTO && ss == 0) ? "TRUE" : "FALSE"));
			Set_FixedTxMode_Proc(pAd, "VHT");
		}
	} else
#endif /* DOT11_VHT_AC */
	{
#ifdef DOT11N_SS3_SUPPORT
		if (pAd->CommonCfg.TxStream >= 3)
			ValidMcs = 23;
#endif
		Mcs_tmp = simple_strtol(arg, 0, 10);
		if (Mcs_tmp <= ValidMcs || Mcs_tmp == 32)
			HtMcs = Mcs_tmp;
		else
			HtMcs = MCS_AUTO;
	}

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
		wdev = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;
		wdev->DesiredTransmitSetting.field.MCS = HtMcs;
		DBGPRINT(RT_DEBUG_TRACE, ("Set_HtMcs_Proc::(HtMcs=%d) for ra%d\n",
				wdev->DesiredTransmitSetting.field.MCS, pObj->ioctl_if));
	}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd) {
		wdev = &pAd->StaCfg.wdev;
		wdev->DesiredTransmitSetting.field.MCS = HtMcs;
		wdev->bAutoTxRateSwitch = (HtMcs == MCS_AUTO) ? TRUE:FALSE;
		DBGPRINT(RT_DEBUG_TRACE,
				("Set_HtMcs_Proc::(HtMcs=%d, bAutoTxRateSwitch = %d)\n",
				wdev->DesiredTransmitSetting.field.MCS,
				wdev->bAutoTxRateSwitch));

		if ((!WMODE_CAP_N(pAd->CommonCfg.PhyMode)) ||
			(pAd->MacTab.Content[BSSID_WCID].HTPhyMode.field.MODE < MODE_HTMIX)) {
			if ((wdev->DesiredTransmitSetting.field.MCS != MCS_AUTO) &&
				(HtMcs <= 3) &&
				(wdev->DesiredTransmitSetting.field.FixedTxMode == FIXED_TXMODE_CCK)) {
				RTMPSetDesiredRates(pAd, (LONG) (RateIdToMbps[HtMcs] * 1000000));
			} else if ((wdev->DesiredTransmitSetting.field.MCS != MCS_AUTO) &&
				(HtMcs <= 7) &&
				(wdev->DesiredTransmitSetting.field.FixedTxMode == FIXED_TXMODE_OFDM)) {
				RTMPSetDesiredRates(pAd, (LONG) (RateIdToMbps[HtMcs+4] * 1000000));
			} else {
				bAutoRate = TRUE;
			}

			if (bAutoRate) {
				wdev->DesiredTransmitSetting.field.MCS = MCS_AUTO;
				RTMPSetDesiredRates(pAd, -1);
			}
			DBGPRINT(RT_DEBUG_TRACE, ("Set_HtMcs_Proc::(FixedTxMode=%d)\n",
					wdev->DesiredTransmitSetting.field.FixedTxMode));
		}
		if (ADHOC_ON(pAd))
			return TRUE;
	}
#endif /* CONFIG_STA_SUPPORT */

	SetCommonHT(pAd);

#ifdef WFA_VHT_PF
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
		int idx;

		NdisAcquireSpinLock(&pAd->MacTabLock);
		for (idx = 1; idx < MAX_LEN_OF_MAC_TABLE; idx++) {
			MAC_TABLE_ENTRY *pEntry = &pAd->MacTab.Content[idx];

			if (IS_ENTRY_CLIENT(pEntry) && (pEntry->apidx == pObj->ioctl_if)) {
				if ((HtMcs == MCS_AUTO) &&  ss == 0) {
					UCHAR TableSize = 0;

					MlmeSelectTxRateTable(pAd, pEntry, &pEntry->pTable, &TableSize, &pEntry->CurrTxRateIndex);
					MlmeNewTxRate(pAd, pEntry);
					pEntry->bAutoTxRateSwitch = TRUE;
#ifdef NEW_RATE_ADAPT_SUPPORT
					if (! ADAPT_RATE_TABLE(pEntry->pTable))
#endif
						pEntry->HTPhyMode.field.ShortGI = GI_800;
				} else {
					pEntry->HTPhyMode.field.MCS = pMbss->HTPhyMode.field.MCS;
					pEntry->bAutoTxRateSwitch = FALSE;

					/* If the legacy mode is set, overwrite the transmit setting of this entry. */
					RTMPUpdateLegacyTxSetting((UCHAR)pMbss->DesiredTransmitSetting.field.FixedTxMode, pEntry);
				}
			}
		}
		NdisReleaseSpinLock(&pAd->MacTabLock);
	}
#endif /* CONFIG_AP_SUPPORT */
#endif /* WFA_VHT_PF */

	return TRUE;
}

BOOLEAN Set_HtGi_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG HtGi = simple_strtol(arg, 0, 10);

	if (HtGi == GI_400)
		pAd->CommonCfg.RegTransmitSetting.field.ShortGI = GI_400;
	else if ( HtGi == GI_800 )
		pAd->CommonCfg.RegTransmitSetting.field.ShortGI = GI_800;
	else
		return FALSE; /* Invalid argument */

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtGi_Proc::(ShortGI=%d)\n",
			pAd->CommonCfg.RegTransmitSetting.field.ShortGI));

	return TRUE;
}


BOOLEAN Set_HtTxBASize_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UCHAR Size = simple_strtol(arg, 0, 10);

	if (Size <= 0 || Size >= 64) {
		Size = 8;
	}

	pAd->CommonCfg.TxBASize = Size-1;
	DBGPRINT(RT_DEBUG_ERROR, ("Set_HtTxBASize ::(TxBASize= %d)\n", Size));

	return TRUE;
}

BOOLEAN Set_HtDisallowTKIP_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG Value = simple_strtol(arg, 0, 10);

	if (Value == 1) {
		pAd->CommonCfg.HT_DisallowTKIP = TRUE;
	} else {
		pAd->CommonCfg.HT_DisallowTKIP = FALSE;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtDisallowTKIP_Proc ::%s\n",
			(pAd->CommonCfg.HT_DisallowTKIP == TRUE) ?
			"enabled" : "disabled"));

	return TRUE;
}

BOOLEAN Set_HtOpMode_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG Value = simple_strtol(arg, 0, 10);

	if (Value == HTMODE_GF)
		pAd->CommonCfg.RegTransmitSetting.field.HTMODE  = HTMODE_GF;
	else if ( Value == HTMODE_MM )
		pAd->CommonCfg.RegTransmitSetting.field.HTMODE  = HTMODE_MM;
	else
		return FALSE; /*Invalid argument */

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtOpMode_Proc::(HtOpMode=%d)\n",
			pAd->CommonCfg.RegTransmitSetting.field.HTMODE));

	return TRUE;
}

BOOLEAN Set_HtStbc_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG Value = simple_strtol(arg, 0, 10);

	if (Value == STBC_USE)
		pAd->CommonCfg.RegTransmitSetting.field.STBC = STBC_USE;
	else if ( Value == STBC_NONE )
		pAd->CommonCfg.RegTransmitSetting.field.STBC = STBC_NONE;
	else
		return FALSE; /*Invalid argument */

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_Stbc_Proc::(HtStbc=%d)\n",
			pAd->CommonCfg.RegTransmitSetting.field.STBC));

	return TRUE;
}

BOOLEAN Set_HtHtc_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG Value;

	Value = simple_strtol(arg, 0, 10);
	if (Value == 0)
		pAd->HTCEnable = FALSE;
	else if ( Value ==1 )
		pAd->HTCEnable = TRUE;
	else
		return FALSE; /*Invalid argument */

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtHtc_Proc::(HtHtc=%d)\n",pAd->HTCEnable));

	return TRUE;
}

BOOLEAN Set_HtExtcha_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG Value;

	Value = simple_strtol(arg, 0, 10);

	if (Value == 0)
		pAd->CommonCfg.RegTransmitSetting.field.EXTCHA  = EXTCHA_BELOW;
	else if ( Value ==1 )
		pAd->CommonCfg.RegTransmitSetting.field.EXTCHA = EXTCHA_ABOVE;
	else
		return FALSE; /*Invalid argument */

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtExtcha_Proc::(HtExtcha=%d)\n",
			pAd->CommonCfg.RegTransmitSetting.field.EXTCHA));

	return TRUE;
}

BOOLEAN Set_HtMpduDensity_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG Value = simple_strtol(arg, 0, 10);

	if (Value <=7)
		pAd->CommonCfg.BACapability.field.MpduDensity = Value;
	else
		pAd->CommonCfg.BACapability.field.MpduDensity = 4;

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtMpduDensity_Proc::(HtMpduDensity=%d)\n",
			pAd->CommonCfg.BACapability.field.MpduDensity));

	return TRUE;
}

BOOLEAN Set_HtBaWinSize_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG Value = simple_strtol(arg, 0, 10);

#ifdef CONFIG_AP_SUPPORT
	/* Intel IOT*/
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		Value = 64;
#endif /* CONFIG_AP_SUPPORT */

	if (Value >=1 && Value <= 64) {
		pAd->CommonCfg.REGBACapability.field.RxBAWinLimit = Value;
		pAd->CommonCfg.BACapability.field.RxBAWinLimit = Value;
	} else {
		pAd->CommonCfg.REGBACapability.field.RxBAWinLimit = 64;
		pAd->CommonCfg.BACapability.field.RxBAWinLimit = 64;
	}

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtBaWinSize_Proc::(HtBaWinSize=%d)\n",
			pAd->CommonCfg.BACapability.field.RxBAWinLimit));

	return TRUE;
}

BOOLEAN Set_HtRdg_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG Value = simple_strtol(arg, 0, 10);

	if (Value == 0) {
		pAd->CommonCfg.bRdg = FALSE;
	} else if (Value == 1) {
		pAd->HTCEnable = TRUE;
		pAd->CommonCfg.bRdg = TRUE;
	} else {
		return FALSE; /*Invalid argument*/
	}

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtRdg_Proc::(HtRdg=%d)\n",
			pAd->CommonCfg.bRdg));

	return TRUE;
}

BOOLEAN Set_HtLinkAdapt_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG Value = simple_strtol(arg, 0, 10);

	if (Value == 0) {
		pAd->bLinkAdapt = FALSE;
	} else if (Value == 1) {
		pAd->HTCEnable = TRUE;
		pAd->bLinkAdapt = TRUE;
	} else {
		return FALSE; /*Invalid argument*/
	}

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtLinkAdapt_Proc::(HtLinkAdapt=%d)\n",
			pAd->bLinkAdapt));

	return TRUE;
}

BOOLEAN Set_HtAmsdu_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG Value = simple_strtol(arg, 0, 10);

	pAd->CommonCfg.BACapability.field.AmsduEnable = (Value == 0) ? FALSE : TRUE;
	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtAmsdu_Proc::(HtAmsdu=%d)\n",
			pAd->CommonCfg.BACapability.field.AmsduEnable));

	return TRUE;
}

BOOLEAN Set_HtAutoBa_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG Value = simple_strtol(arg, 0, 10);

	if (Value == 0) {
		pAd->CommonCfg.BACapability.field.AutoBA = FALSE;
		pAd->CommonCfg.BACapability.field.Policy = BA_NOTUSE;
	} else if (Value == 1) {
		pAd->CommonCfg.BACapability.field.AutoBA = TRUE;
		pAd->CommonCfg.BACapability.field.Policy = IMMED_BA;
	} else {
		return FALSE; /*Invalid argument*/
	}

	pAd->CommonCfg.REGBACapability.field.AutoBA = pAd->CommonCfg.BACapability.field.AutoBA;
	pAd->CommonCfg.REGBACapability.field.Policy = pAd->CommonCfg.BACapability.field.Policy;
	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtAutoBa_Proc::(HtAutoBa=%d)\n",
			pAd->CommonCfg.BACapability.field.AutoBA));

	return TRUE;
}

BOOLEAN Set_HtProtect_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG Value = simple_strtol(arg, 0, 10);
	if (Value == 0)
		pAd->CommonCfg.bHTProtect = FALSE;
	else if (Value == 1)
		pAd->CommonCfg.bHTProtect = TRUE;
	else
		return FALSE; /*Invalid argument*/

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtProtect_Proc::(HtProtect=%d)\n",
			pAd->CommonCfg.bHTProtect));

	return TRUE;
}

BOOLEAN Set_SendSMPSAction_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UCHAR mac[6], mode;
	PSTRING token;
	STRING sepValue[] = ":", DASH = '-';
	int i;
	MAC_TABLE_ENTRY *pEntry;

	/*DBGPRINT(RT_DEBUG_TRACE,("\n%s\n", arg));*/
/*
	The BARecTearDown inupt string format should be xx:xx:xx:xx:xx:xx-d,
		=>The six 2 digit hex-decimal number previous are the Mac address,
		=>The seventh decimal number is the mode value.
*/
	if (strlen(arg) < 19)  /*Mac address acceptable format 01:02:03:04:05:06 length 17 plus the "-" and mode value in decimal format.*/
		return FALSE;

	token = strchr(arg, DASH);
	if ((token != NULL) && (strlen(token) > 1)) {
		mode = simple_strtol((token+1), 0, 10);
		if (mode > MMPS_DISABLE)
			return FALSE;

		*token = '\0';
		for (i = 0, token = rstrtok(arg, &sepValue[0]); token; token = rstrtok(NULL, &sepValue[0]), i++) {
			if ((strlen(token) != 2) || (!isxdigit(*token)) || (!isxdigit(*(token+1))))
				return FALSE;
			AtoH(token, (&mac[i]), 1);
		}

		if (i != 6)
			return FALSE;

		DBGPRINT(RT_DEBUG_OFF, ("\n%02x:%02x:%02x:%02x:%02x:%02x-%02x",
				PRINT_MAC(mac), mode));

		pEntry = MacTableLookup(pAd, mac);
		if (pEntry) {
			DBGPRINT(RT_DEBUG_OFF, ("\nSendSMPSAction SMPS mode = %d\n", mode));
			SendSMPSAction(pAd, pEntry->wcid, mode);
		}

		return TRUE;
	}

	return FALSE;
}

BOOLEAN Set_HtMIMOPSmode_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG Value = simple_strtol(arg, 0, 10);

	if (Value <=3)
		pAd->CommonCfg.BACapability.field.MMPSmode = Value;
	else
		pAd->CommonCfg.BACapability.field.MMPSmode = 3;

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtMIMOPSmode_Proc::(MIMOPS mode=%d)\n",
			pAd->CommonCfg.BACapability.field.MMPSmode));

	return TRUE;
}

#ifdef CONFIG_AP_SUPPORT
/*
	==========================================================================
	Description:
		Set Tx Stream number
	Return:
		TRUE if all parameters are OK, FALSE otherwise
	==========================================================================
*/
BOOLEAN Set_HtTxStream_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG Value = simple_strtol(arg, 0, 10);

	if ((Value <= 3) && (Value >= 1) && (Value <= pAd->Antenna.field.TxPath)) /* 3*3*/
		pAd->CommonCfg.TxStream = Value;
	else
		pAd->CommonCfg.TxStream = pAd->Antenna.field.TxPath;

	if ((pAd->MACVersion < RALINK_2883_VERSION) &&
		(pAd->CommonCfg.TxStream > 2)) {
		pAd->CommonCfg.TxStream = 2; /* only 2 TX streams for RT2860 series*/
	}

	SetCommonHT(pAd);
	APStop(pAd);
	APStartUp(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtTxStream_Proc::(Tx Stream=%d)\n",
			pAd->CommonCfg.TxStream));

	return TRUE;
}

/*
	==========================================================================
	Description:
		Set Rx Stream number
	Return:
		TRUE if all parameters are OK, FALSE otherwise
	==========================================================================
*/
BOOLEAN Set_HtRxStream_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG Value = simple_strtol(arg, 0, 10);

	if ((Value <= 3) && (Value >= 1) && (Value <= pAd->Antenna.field.RxPath))
		pAd->CommonCfg.RxStream = Value;
	else
		pAd->CommonCfg.RxStream = pAd->Antenna.field.RxPath;

	if ((pAd->MACVersion < RALINK_2883_VERSION) &&
		(pAd->CommonCfg.RxStream > 2)) {
		/* 3*3*/
		pAd->CommonCfg.RxStream = 2; /* only 2 RX streams for RT2860 series*/
	}

	SetCommonHT(pAd);
	APStop(pAd);
	APStartUp(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtRxStream_Proc::(Rx Stream=%d)\n",
			pAd->CommonCfg.RxStream));

	return TRUE;
}

#ifdef DOT11_N_SUPPORT
#ifdef GREENAP_SUPPORT
BOOLEAN Set_GreenAP_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG Value = simple_strtol(arg, 0, 10);
	if (Value == 0) {
		pAd->ApCfg.bGreenAPActive=FALSE;
		pAd->ApCfg.bGreenAPEnable = FALSE;
	} else if (Value == 1) {
		pAd->ApCfg.bGreenAPEnable = TRUE;
	} else {
		return FALSE; /*Invalid argument*/
	}

	DBGPRINT(RT_DEBUG_TRACE, ("Set_GreenAP_Proc::(bGreenAPEnable=%d)\n",
			pAd->ApCfg.bGreenAPEnable));

	return TRUE;
}
#endif /* GREENAP_SUPPORT */
#endif /* DOT11_N_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

BOOLEAN Set_ForceShortGI_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG Value = simple_strtol(arg, 0, 10);
	if (Value == 0)
		pAd->WIFItestbed.bShortGI = FALSE;
	else if (Value == 1)
		pAd->WIFItestbed.bShortGI = TRUE;
	else
		return FALSE; /*Invalid argument*/

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_ForceShortGI_Proc::(ForceShortGI=%d)\n",
			pAd->WIFItestbed.bShortGI));

	return TRUE;
}

BOOLEAN Set_ForceGF_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG Value = simple_strtol(arg, 0, 10);
	if (Value == 0)
		pAd->WIFItestbed.bGreenField = FALSE;
	else if (Value == 1)
		pAd->WIFItestbed.bGreenField = TRUE;
	else
		return FALSE; /*Invalid argument*/

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_ForceGF_Proc::(ForceGF=%d)\n",
			pAd->WIFItestbed.bGreenField));

	return TRUE;
}

BOOLEAN Set_HtMimoPs_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG Value = simple_strtol(arg, 0, 10);
	if (Value == 0)
		pAd->CommonCfg.bMIMOPSEnable = FALSE;
	else if (Value == 1)
		pAd->CommonCfg.bMIMOPSEnable = TRUE;
	else
		return FALSE; /*Invalid argument*/

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtMimoPs_Proc::(HtMimoPs=%d)\n",
			pAd->CommonCfg.bMIMOPSEnable));

	return TRUE;
}


#ifdef DOT11N_DRAFT3
BOOLEAN Set_HT_BssCoex_Proc(PRTMP_ADAPTER pAd, PSTRING pParam)
{
	UCHAR bBssCoexEnable = simple_strtol(pParam, 0, 10);

	pAd->CommonCfg.bBssCoexEnable = ((bBssCoexEnable == 1) ? TRUE: FALSE);

	DBGPRINT(RT_DEBUG_TRACE, ("Set bBssCoexEnable=%d!\n",
			pAd->CommonCfg.bBssCoexEnable));

	if ((pAd->CommonCfg.bBssCoexEnable == FALSE) &&
		pAd->CommonCfg.bRcvBSSWidthTriggerEvents) {
		/* switch back 20/40 */
		DBGPRINT(RT_DEBUG_TRACE, ("Set bBssCoexEnable:  Switch back 20/40. \n"));
		pAd->CommonCfg.bRcvBSSWidthTriggerEvents = FALSE;
		if ((pAd->CommonCfg.Channel <=14) &&
			(pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth == BW_40)) {
			pAd->CommonCfg.AddHTInfo.AddHtInfo.RecomWidth = 1;
			pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset =
					pAd->CommonCfg.RegTransmitSetting.field.EXTCHA;
		}
	}

	return TRUE;
}


BOOLEAN Set_HT_BssCoexApCntThr_Proc(PRTMP_ADAPTER pAd, PSTRING pParam)
{
	pAd->CommonCfg.BssCoexApCntThr = simple_strtol(pParam, 0, 10);

	DBGPRINT(RT_DEBUG_TRACE, ("Set BssCoexApCntThr=%d!\n", pAd->CommonCfg.BssCoexApCntThr));

	return TRUE;
}
#endif /* DOT11N_DRAFT3 */
#endif /* DOT11_N_SUPPORT */


#ifdef DOT11_VHT_AC
BOOLEAN Set_VhtBw_Proc(RTMP_ADAPTER *pAd,  PSTRING arg)
{
	ULONG vht_cw;
	UCHAR cent_ch;
	vht_cw = simple_strtol(arg, 0, 10);

	if (vht_cw == VHT_BW_80)
		pAd->CommonCfg.vht_bw = VHT_BW_80;
	else
		pAd->CommonCfg.vht_bw = VHT_BW_2040;

	if (!WMODE_CAP_AC(pAd->CommonCfg.PhyMode))
		goto direct_done;

	SetCommonHT(pAd);
	if (pAd->CommonCfg.BBPCurrentBW == BW_80)
		cent_ch = pAd->CommonCfg.vht_cent_ch;
	else
		cent_ch = pAd->CommonCfg.CentralChannel;

	AsicSwitchChannel(pAd, cent_ch, FALSE);
	AsicLockChannel(pAd, cent_ch);

	DBGPRINT(RT_DEBUG_TRACE,
			("BW_%s, PrimaryChannel(%d), %s CentralChannel = %d, apply it immediately\n",
			(pAd->CommonCfg.BBPCurrentBW == BW_80 ? "80":
			(pAd->CommonCfg.BBPCurrentBW == BW_40 ? "40" : "20")),
			pAd->CommonCfg.Channel,
			(pAd->CommonCfg.BBPCurrentBW == BW_80 ? "VHT" : "HT"),
			cent_ch));

direct_done:
	DBGPRINT(RT_DEBUG_TRACE, ("Set_VhtBw_Proc::(VHT_BW=%d)\n",
			pAd->CommonCfg.vht_bw));

#ifdef DFS_ATP_SUPPORT
	pAd->CommonCfg.RadarDetect.atp_set_vht_bw = TRUE;
	if (pAd->CommonCfg.RadarDetect.atp_set_channel_ready) {
		printk ("BW and Channel is ready\n");
	}
#endif /* DFS_ATP_SUPPORT */

	return TRUE;
}


BOOLEAN set_VhtBwSignal_Proc(RTMP_ADAPTER *pAd, PSTRING arg)
{
	ULONG bw_signal = simple_strtol(arg, 0, 10);

	if (bw_signal <= 2)
		pAd->CommonCfg.vht_bw_signal = bw_signal;
	else
		pAd->CommonCfg.vht_bw_signal = BW_SIGNALING_DISABLE;
	DBGPRINT(RT_DEBUG_TRACE, ("%s(): vht_bw_signal=%d(%s)\n",
			__FUNCTION__, pAd->CommonCfg.vht_bw_signal,
			(pAd->CommonCfg.vht_bw_signal == BW_SIGNALING_DYNAMIC ? "Dynamic" :
			(pAd->CommonCfg.vht_bw_signal == BW_SIGNALING_STATIC ? "Static" : "Disable"))));

	return TRUE;
}


BOOLEAN Set_VhtStbc_Proc(RTMP_ADAPTER *pAd, PSTRING arg)
{
	ULONG Value = simple_strtol(arg, 0, 10);

	if (Value == STBC_USE)
		pAd->CommonCfg.vht_stbc = STBC_USE;
	else if ( Value == STBC_NONE )
		pAd->CommonCfg.vht_stbc = STBC_NONE;
	else
		return FALSE; /*Invalid argument */

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_VhtStbc_Proc::(VhtStbc=%d)\n",
			pAd->CommonCfg.vht_stbc));

	return TRUE;
}

BOOLEAN Set_VhtDisallowNonVHT_Proc( RTMP_ADAPTER *pAd,  PSTRING arg)
{
	ULONG Value = simple_strtol(arg, 0, 10);

	if (Value == 0)
		pAd->CommonCfg.bNonVhtDisallow = FALSE;
	else
		pAd->CommonCfg.bNonVhtDisallow = TRUE;

	DBGPRINT(RT_DEBUG_TRACE,
			("Set_VhtDisallowNonVHT_Proc::(bNonVhtDisallow=%d)\n",
			pAd->CommonCfg.bNonVhtDisallow));

	return TRUE;
}
#endif /* DOT11_VHT_AC */

BOOLEAN Set_FixedTxMode_Proc(RTMP_ADAPTER *pAd, PSTRING arg)
{
	struct wifi_dev *wdev = NULL;
#ifdef CONFIG_AP_SUPPORT
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
#endif
	int fix_tx_mode = RT_CfgSetFixedTxPhyMode(arg);

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		wdev = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;
#endif
#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		wdev = &pAd->StaCfg.wdev;
#endif
	if (wdev)
		wdev->DesiredTransmitSetting.field.FixedTxMode = fix_tx_mode;

	DBGPRINT(RT_DEBUG_TRACE, ("%s():(FixedTxMode=%d)\n", __FUNCTION__,
			fix_tx_mode));

	return TRUE;
}

#ifdef CONFIG_APSTA_MIXED_SUPPORT
BOOLEAN Set_OpMode_Proc(RTMP_ADAPTER *pAd, PSTRING arg)
{
	ULONG Value = simple_strtol(arg, 0, 10);

#ifdef RTMP_MAC_USB
	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_START_UP))
#endif /* RTMP_MAC_USB */
	{
		DBGPRINT(RT_DEBUG_ERROR, ("Cannot switch operate mode on interface up !! \n"));
		return FALSE;
	}

	if (Value == 0)
		pAd->OpMode = OPMODE_STA;
	else if (Value == 1)
		pAd->OpMode = OPMODE_AP;
	else
		return FALSE; /*Invalid argument*/

	DBGPRINT(RT_DEBUG_TRACE, ("Set_OpMode_Proc::(OpMode=%s)\n",
			pAd->OpMode == 1 ? "AP Mode" : "STA Mode"));

	return TRUE;
}
#endif /* CONFIG_APSTA_MIXED_SUPPORT */


#ifdef DBG_CTRL_SUPPORT
#ifdef INCLUDE_DEBUG_QUEUE
/* ---------------------- DEBUG QUEUE ------------------------*/

#define DBQ_LENGTH	512
#define DBQ_DATA_LENGTH	8

/* Define to include TX and RX HT Control field in log */
/* #define DBQ_INCLUDE_HTC */

typedef
struct {
	UCHAR type;			/* type of data*/
	ULONG timestamp;		/* sec/usec timestamp from gettimeofday*/
	UCHAR data[DBQ_DATA_LENGTH];	/* data*/
} DBQUEUE_ENTRY;

/* Type field definitions */
#define DBQ_TYPE_EMPTY	0
#define DBQ_TYPE_TXWI	0x70		/* TXWI*/
#define DBQ_TYPE_TXHDR	0x72		/* TX Header*/
#define DBQ_TYPE_TXFIFO	0x73		/* TX Stat FIFO*/
#define DBQ_TYPE_RXWI	0x78		/* RXWI uses 0x78 to 0x7A for 5 longs*/
#define DBQ_TYPE_RXHDR	0x7B		/* RX Header*/
#define DBQ_TYPE_TXQHTC	0x7c		/* RX Qos+HT Control field*/
#define DBQ_TYPE_RXQHTC	0x7d		/* RX Qos+HT Control field */
#define DBQ_TYPE_RALOG	0x7e		/* RA Log */

#define DBQ_INIT_SIG	0x4442484E	/* 'DBIN' - dbqInit initialized flag*/
#define DBQ_ENA_SIG	0x4442454E	/* 'DBEN' - dbqEnable enabled flag*/

static DBQUEUE_ENTRY dbQueue[DBQ_LENGTH];
static ULONG dbqTail = 0;
static ULONG dbqEnable = 0;
static ULONG dbqInit = 0;

/* dbQueueInit - initialize Debug Queue variables and clear the queue*/
void dbQueueInit(void)
{
	int i;

	for (i = 0; i < DBQ_LENGTH; i++)
		dbQueue[i].type = DBQ_TYPE_EMPTY;
	dbqTail = 0;
	dbqInit = DBQ_INIT_SIG;
}

/* dbQueueEnqueue - enqueue data*/
void dbQueueEnqueue(UCHAR type, UCHAR *data)
{
	DBQUEUE_ENTRY *oldTail;
	struct timeval tval;

	if (dbqEnable != DBQ_ENA_SIG || data == NULL)
		return;

	if (dbqInit != DBQ_INIT_SIG || dbqTail >= DBQ_LENGTH)
		dbQueueInit();

	oldTail = &dbQueue[dbqTail];

	/* Advance tail and mark as empty*/
	if (dbqTail >= DBQ_LENGTH-1)
		dbqTail = 0;
	else
		dbqTail++;
	dbQueue[dbqTail].type = DBQ_TYPE_EMPTY;

	/* Enqueue data*/
	oldTail->type = type;
	do_gettimeofday(&tval);
	oldTail->timestamp = tval.tv_sec*1000000L + tval.tv_usec;
	memcpy(oldTail->data, data, DBQ_DATA_LENGTH);
}

void dbQueueEnqueueTxFrame(UCHAR *pTxWI, UCHAR *pHeader_802_11)
{
	dbQueueEnqueue(DBQ_TYPE_TXWI, pTxWI);

	/* 802.11 Header */
	if (pHeader_802_11 != NULL) {
		dbQueueEnqueue(DBQ_TYPE_TXHDR, pHeader_802_11);
#ifdef DBQ_INCLUDE_HTC
		/* Qos+HT Control field */
		if ((pHeader_802_11[0] & 0x08) && (pHeader_802_11[1] & 0x80))
			dbQueueEnqueue(DBQ_TYPE_TXQHTC, pHeader_802_11+24);
#endif
	}
}

void dbQueueEnqueueRxFrame(UCHAR *pRxWI, UCHAR *pHeader_802_11, ULONG flags)
{
	/* Ignore Beacons if disabled */
	if ((flags & DBF_DBQ_NO_BCN) && (pHeader_802_11[0] & 0xfc)==0x80)
		return;

	/* RXWI */
	dbQueueEnqueue(DBQ_TYPE_RXWI, pRxWI);
	if (flags & DBF_DBQ_RXWI_FULL) {
		dbQueueEnqueue(DBQ_TYPE_RXWI+1, pRxWI+8);
		dbQueueEnqueue(DBQ_TYPE_RXWI+2, pRxWI+16);
	}

	/* 802.11 Header */
	dbQueueEnqueue(DBQ_TYPE_RXHDR, (UCHAR *)pHeader_802_11);

#ifdef DBQ_INCLUDE_HTC
	/* Qos+HT Control field */
	if ((pHeader_802_11[0] & 0x08) &&
		(pHeader_802_11[1] & 0x80))
		dbQueueEnqueue(DBQ_TYPE_RXQHTC, pHeader_802_11+24);
#endif
}


/* dbQueueDisplayPhy - Display PHY rate */
static void dbQueueDisplayPHY(USHORT phyRate)
{
	static CHAR *mode[4] = {" C", "oM","mM", "gM"};

	DBGPRINT(RT_DEBUG_OFF, ("%2s%02d %c%c%c%c",
		//(phyRate>>8) & 0xFF, phyRate & 0xFF,
		mode[(phyRate>>14) & 0x3],			// Mode: c, o, m, g
		phyRate & 0x7F,					// MCS
		(phyRate & 0x0100)? 'S': 'L',			// Guard Int: S or L
		(phyRate & 0x0080)? '4': '2',			// BW: 4 or 2
		(phyRate & 0x0200)? 'S': 's',			// STBC:  S or s
		(phyRate & 0x2000)? 'I': ((phyRate & 0x800)? 'E': '_')	// Beamforming:  E or I or _
		) );
}

/* dbQueueDump - dump contents of debug queue*/
static void dbQueueDump(PRTMP_ADAPTER pAd, BOOLEAN decode)
{
	DBQUEUE_ENTRY *oldTail;
	int i, origMCS, succMCS;
	ULONG lastTimestamp = 0;
	BOOLEAN showTimestamp;
	USHORT phyRate;

	if (dbqInit != DBQ_INIT_SIG || dbqTail >= DBQ_LENGTH)
		return;

	oldTail = &dbQueue[dbqTail];

	for (i = 0; i < DBQ_LENGTH; i++) {
		if (++oldTail >= &dbQueue[DBQ_LENGTH])
			oldTail = dbQueue;

		/* Skip empty entries*/
		if (oldTail->type == DBQ_TYPE_EMPTY)
			continue;

		showTimestamp = FALSE;

		switch (oldTail->type) {
		case 0x70:	/* TXWI - 2 longs, MSB to LSB */
		case 0x78:	/* RXWI - 2 longs, MSB to LSB */
			showTimestamp = TRUE;

			if (decode && oldTail->type == 0x70) {
				DBGPRINT(RT_DEBUG_OFF, ("\nTxWI ") );
				dbQueueDisplayPHY(oldTail->data[3]*256 + oldTail->data[2]);
				DBGPRINT(RT_DEBUG_OFF, ("%c s=%03X %02X %s-",
						(oldTail->data[0] & 0x10)? 'A': '_',			// AMPDU
						(oldTail->data[7]*256 + oldTail->data[6]) & 0xFFF,	// Size
						oldTail->data[5],					// WCID
						(oldTail->data[4] & 0x01)? "AK": "NA" ));		// ACK/NACK
			}
			else if (decode && oldTail->type == 0x78) {
				DBGPRINT(RT_DEBUG_OFF, ("\nRxWI ") );
				dbQueueDisplayPHY(oldTail->data[7]*256 + oldTail->data[6]);
				DBGPRINT(RT_DEBUG_OFF, (" s=%03X %02X %02X%01X-",
						(oldTail->data[3]*256 + oldTail->data[2]) & 0xFFF,	// Size
						oldTail->data[0],					// WCID
						oldTail->data[5], oldTail->data[4]>>4 ));		// Seq Number
			} else {
				DBGPRINT(RT_DEBUG_OFF, ("\n%cxWI %02X%02X %02X%02X-%02X%02X %02X%02X----",
					oldTail->type == 0x70? 'T': 'R',
					oldTail->data[3], oldTail->data[2], oldTail->data[1], oldTail->data[0],
					oldTail->data[7], oldTail->data[6], oldTail->data[5], oldTail->data[4]));
			}
			break;

		case 0x79:	/* RXWI - next 2 longs, MSB to LSB */
			if (decode) {
				DBGPRINT(RT_DEBUG_OFF, ("Rx2  %2d %2d %2d S:%d %d %d ",
						ConvertToRssi(pAd, (CHAR)oldTail->data[0], RSSI_0),
						ConvertToRssi(pAd, (CHAR)oldTail->data[1], RSSI_1),
						ConvertToRssi(pAd, (CHAR)oldTail->data[2], RSSI_2),
						(oldTail->data[4]*3 + 8)/16,
						(oldTail->data[5]*3 + 8)/16,
						(oldTail->data[6]*3 + 8)/16));
			} else {
				DBGPRINT(RT_DEBUG_OFF, ("Rx2  %02X%02X %02X%02X-%02X%02X %02X%02X    ",
						oldTail->data[3], oldTail->data[2], oldTail->data[1], oldTail->data[0],
						oldTail->data[7], oldTail->data[6], oldTail->data[5], oldTail->data[4]));
			}
			break;


		case 0x7c:	/* TX HTC+QoS, 6 bytes, MSB to LSB */
		case 0x7d:	/* RX HTC+QoS, 6 bytes, MSB to LSB */
			DBGPRINT(RT_DEBUG_OFF, ("%cxHTC  H:%02X%02X%02X%02X Q:%02X%02X   ",
					oldTail->type == 0x7c? 'T': 'R',
					oldTail->data[5], oldTail->data[4], oldTail->data[3], oldTail->data[2],
					oldTail->data[1], oldTail->data[0]) );
			break;

		case 0x72:	/* Tx 802.11 header, MSB to LSB, translate type/subtype*/
		case 0x7b:	/* Rx*/
			{
			UCHAR tCode;
			struct _typeTableEntry {
				UCHAR code;	/* Type/subtype*/
				CHAR str[4];
			} *pTab;
			static struct _typeTableEntry typeTable[] = {
				{0x00, "mARq"}, {0x01, "mARp"}, {0x02, "mRRq"}, {0x03, "mRRp"},
				{0x04, "mPRq"}, {0x05, "mPRp"}, {0x08, "mBcn"}, {0x09, "mATI"},
				{0x0a, "mDis"}, {0x0b, "mAut"}, {0x0c, "mDAu"}, {0x0d, "mAct"},
				{0x0e, "mANA"},
				{0x17, "cCWr"}, {0x18, "cBAR"}, {0x19, "cBAc"}, {0x1a, "cPSP"},
				{0x1b, "cRTS"}, {0x1c, "cCTS"}, {0x1d, "cACK"}, {0x1e, "cCFE"},
				{0x1f, "cCEA"},
				{0x20, "dDat"}, {0x21, "dDCA"}, {0x22, "dDCP"}, {0x23, "dDAP"},
				{0x24, "dNul"}, {0x25, "dCFA"}, {0x26, "dCFP"}, {0x27, "dCAP"},
				{0x28, "dQDa"}, {0x29, "dQCA"}, {0x2a, "dQCP"}, {0x2b, "dQAP"},
				{0x2c, "dQNu"}, {0x2e, "dQNP"}, {0x2f, "dQNA"},
				{0xFF, "RESV"}};

			tCode = ((oldTail->data[0]<<2) & 0x30) | ((oldTail->data[0]>>4) & 0xF);
			for (pTab=typeTable; pTab->code!=0xFF; pTab++) {
				if (pTab->code == tCode)
					break;
			}

			DBGPRINT(RT_DEBUG_OFF, ("%cxH  %c%c%c%c [%02X%02X %02X%02X]       \n",
					oldTail->type == 0x72? 'T': 'R',
					pTab->str[0], pTab->str[1], pTab->str[2], pTab->str[3],
					oldTail->data[3], oldTail->data[2], oldTail->data[1], oldTail->data[0]));
			}
			break;

		case 0x73:	/* TX STAT FIFO*/
			showTimestamp = TRUE;

			/* origMCS is limited to 4 bits. Check for case of MCS16 to 23*/
			origMCS = (oldTail->data[0]>>1) & 0xF;
			succMCS = (oldTail->data[2] & 0x7F);
			if (succMCS>origMCS && origMCS<8)
				origMCS += 16;
			phyRate = (oldTail->data[3]<<8) + oldTail->data[2];

			DBGPRINT(RT_DEBUG_OFF, ("TxFI %02X%02X%02X%02X=%c%c%c%c%c M%02d/%02d%c%c",
					oldTail->data[3], oldTail->data[2],
					oldTail->data[1], oldTail->data[0],
					(phyRate & 0x0100)? 'S': 'L',			/* Guard Int:    S or L */
					(phyRate & 0x0080)? '4': '2',			/* BW:  4 or 2 */
					(phyRate & 0x0200)? 'S': 's',			/* STBC:         S or s */
					(phyRate & 0x2000)? 'I': ((phyRate & 0x0800)? 'E': '_'), /* Beamforming:  E or I or _ */
					(oldTail->data[0] & 0x40)? 'A': '_',		/* Aggregated:   A or _ */
					succMCS, origMCS,				/* MCS:          <Final>/<orig> */
					succMCS==origMCS? ' ': '*',			/* Retry:        '*' if MCS doesn't match */
					(oldTail->data[0] & 0x20)? ' ': 'F') );		/* Success/Fail  _ or F */
			break;
		case 0x7E:	/* RA Log info */
			{
				struct {USHORT phy; USHORT per; USHORT tp; USHORT bfPer;} *p = (void*)(oldTail->data);
				DBGPRINT(RT_DEBUG_OFF, ("RALog %02X%02X %d %d %d    ",
						(p->phy>>8) & 0xFF, p->phy & 0xFF,
						p->per, p->tp, p->bfPer));
			}
			break;

		default:
			DBGPRINT(RT_DEBUG_OFF,
					("%02X   %02X%02X %02X%02X %02X%02X %02X%02X   ",
					oldTail->type, oldTail->data[0],
					oldTail->data[1], oldTail->data[2],
					oldTail->data[3], oldTail->data[4],
					oldTail->data[5], oldTail->data[6],
					 oldTail->data[7]));
			break;
		}

		if (showTimestamp) {
			ULONG t = oldTail->timestamp;
			ULONG dt = oldTail->timestamp-lastTimestamp;

			DBGPRINT(RT_DEBUG_OFF, ("%lu.%06lu ", t/1000000L, t % 1000000L) );

			if (dt > 999999L)
				DBGPRINT(RT_DEBUG_OFF, ("+%lu.%06lu s\n", dt/1000000L, dt % 1000000L) );
			else
				DBGPRINT(RT_DEBUG_OFF, ("+%lu us\n", dt) );

			lastTimestamp = oldTail->timestamp;
		}
	}
}

/*
	Set_DebugQueue_Proc - Control DBQueue
		iwpriv ra0 set DBQueue=dd.
			dd: 0=>disable, 1=>enable, 2=>dump, 3=>clear, 4=>dump & decode
*/
BOOLEAN Set_DebugQueue_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG argValue = simple_strtol(arg, 0, 10);

	switch (argValue) {
	case 0:
		dbqEnable = 0;
		break;
	case 1:
		dbqEnable = DBQ_ENA_SIG;
		break;
	case 2:
		dbQueueDump(pAd, FALSE);
		break;
	case 3:
		dbQueueInit();
		break;
	case 4:
		dbQueueDump(pAd, TRUE);
		break;
	default:
		return FALSE;
	}

	return TRUE;
}
#endif /* INCLUDE_DEBUG_QUEUE */
#endif /* DBG_CTRL_SUPPORT */

#ifdef STREAM_MODE_SUPPORT
/*
	========================================================================
	Routine Description:
		Set the enable/disable the stream mode

	Arguments:
		1:	enable for 1SS
		2:	enable for 2SS
		3:	enable for 1SS and 2SS
		0:	disable

	Notes:
		Currently only support 1SS
	========================================================================
*/
BOOLEAN Set_StreamMode_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UINT32 streamWord, reg, regAddr;

	if (pAd->chipCap.FlgHwStreamMode == FALSE) {
		DBGPRINT(RT_DEBUG_ERROR, ("chip not supported feature\n"));
		return FALSE;
	}

	pAd->CommonCfg.StreamMode = (simple_strtol(arg, 0, 10) & 0x3);
	DBGPRINT(RT_DEBUG_TRACE, ("%s():StreamMode=%d\n", __FUNCTION__,
			pAd->CommonCfg.StreamMode));

	streamWord = StreamModeRegVal(pAd);
	for (regAddr = TX_CHAIN_ADDR0_H; regAddr <= TX_CHAIN_ADDR3_H; regAddr += 8) {
		RTMP_IO_READ32(pAd, regAddr, &reg);
		reg &= (~0x000F0000);
		RTMP_IO_WRITE32(pAd, regAddr, streamWord | reg);
	}

	return TRUE;
}


static BOOLEAN Set_StreamModeMac_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	return FALSE;
}


BOOLEAN Set_StreamModeMCS_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	pAd->CommonCfg.StreamModeMCS = simple_strtol(arg, 0, 16);
	DBGPRINT(RT_DEBUG_TRACE, ("%s():StreamModeMCS=%02X\n",
			__FUNCTION__, pAd->CommonCfg.StreamModeMCS));

	return TRUE;
}
#endif /* STREAM_MODE_SUPPORT */


#ifdef PRE_ANT_SWITCH
/*
	Set_PreAntSwitch_Proc - enable/disable Preamble Antenna Switch
	usage: iwpriv ra0 set PreAntSwitch=[0 | 1]
*/
BOOLEAN Set_PreAntSwitch_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	pAd->CommonCfg.PreAntSwitch = simple_strtol(arg, 0, 10)!=0;
	DBGPRINT(RT_DEBUG_TRACE, ("%s():(PreAntSwitch=%d)\n",
			__FUNCTION__, pAd->CommonCfg.PreAntSwitch));
	return TRUE;
}


/*
	Set_PreAntSwitchRSSI_Proc - set Preamble Antenna Switch RSSI threshold
	usage: iwpriv ra0 set PreAntSwitchRSSI=<RSSI threshold in dBm>
*/
BOOLEAN Set_PreAntSwitchRSSI_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	pAd->CommonCfg.PreAntSwitchRSSI = simple_strtol(arg, 0, 10);
	DBGPRINT(RT_DEBUG_TRACE, ("%s():(PreAntSwitchRSSI=%d)\n",
			__FUNCTION__, pAd->CommonCfg.PreAntSwitchRSSI));
	return TRUE;
}

/*
	Set_PreAntSwitchTimeout_Proc - set Preamble Antenna Switch Timeout threshold
		usage: iwpriv ra0 set PreAntSwitchTimeout=<timeout in seconds, 0=>disabled>
*/
BOOLEAN Set_PreAntSwitchTimeout_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	pAd->CommonCfg.PreAntSwitchTimeout = simple_strtol(arg, 0, 10);
	DBGPRINT(RT_DEBUG_TRACE, ("%s():(PreAntSwitchTimeout=%d)\n",
			__FUNCTION__, pAd->CommonCfg.PreAntSwitchTimeout));
	return TRUE;
}
#endif /* PRE_ANT_SWITCH */


#ifdef CFO_TRACK
/*
	Set_CFOTrack_Proc - enable/disable CFOTrack
	usage: iwpriv ra0 set CFOTrack=[0..8]
*/
BOOLEAN Set_CFOTrack_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	pAd->CommonCfg.CFOTrack = simple_strtol(arg, 0, 10);
	DBGPRINT(RT_DEBUG_TRACE, ("%s():(CFOTrack=%d)\n",
			__FUNCTION__, pAd->CommonCfg.CFOTrack));
	return TRUE;
}
#endif


#ifdef DBG_CTRL_SUPPORT
BOOLEAN Set_DebugFlags_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	pAd->CommonCfg.DebugFlags = simple_strtol(arg, 0, 16);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_DebugFlags_Proc::(DebugFlags=%02lX)\n",
			pAd->CommonCfg.DebugFlags));
	return TRUE;
}
#endif

BOOLEAN Set_LongRetryLimit_Proc(RTMP_ADAPTER *pAd, PSTRING arg)
{
	TX_RTY_CFG_STRUC tx_rty_cfg;
	UCHAR LongRetryLimit = (UCHAR)simple_strtol(arg, 0, 10);

	RTMP_IO_READ32(pAd, TX_RTY_CFG, &tx_rty_cfg.word);
	tx_rty_cfg.field.LongRtyLimit = LongRetryLimit;
	RTMP_IO_WRITE32(pAd, TX_RTY_CFG, tx_rty_cfg.word);
	DBGPRINT(RT_DEBUG_TRACE,
			("IF Set_LongRetryLimit_Proc::(tx_rty_cfg=0x%x)\n",
			tx_rty_cfg.word));
	return TRUE;
}

BOOLEAN Set_ShortRetryLimit_Proc(RTMP_ADAPTER *pAd, PSTRING arg)
{
	TX_RTY_CFG_STRUC tx_rty_cfg;
	UCHAR ShortRetryLimit = (UCHAR)simple_strtol(arg, 0, 10);

	RTMP_IO_READ32(pAd, TX_RTY_CFG, &tx_rty_cfg.word);
	tx_rty_cfg.field.ShortRtyLimit = ShortRetryLimit;
	RTMP_IO_WRITE32(pAd, TX_RTY_CFG, tx_rty_cfg.word);
	DBGPRINT(RT_DEBUG_TRACE,
			("IF Set_ShortRetryLimit_Proc::(tx_rty_cfg=0x%x)\n",
			tx_rty_cfg.word));
	return TRUE;
}

BOOLEAN Set_AutoFallBack_Proc(PRTMP_ADAPTER pAdapter, PSTRING arg)
{
	return RT_CfgSetAutoFallBack(pAdapter, arg);
}


PSTRING RTMPGetRalinkAuthModeStr(NDIS_802_11_AUTHENTICATION_MODE authMode)
{
	switch (authMode) {
	case Ndis802_11AuthModeOpen:
		return "OPEN";
	case Ndis802_11AuthModeWPAPSK:
		return "WPAPSK";
	case Ndis802_11AuthModeShared:
		return "SHARED";
	case Ndis802_11AuthModeAutoSwitch:
		return "WEPAUTO";
	case Ndis802_11AuthModeWPA:
		return "WPA";
	case Ndis802_11AuthModeWPA2:
		return "WPA2";
	case Ndis802_11AuthModeWPA2PSK:
		return "WPA2PSK";
	case Ndis802_11AuthModeWPA1PSKWPA2PSK:
		return "WPAPSKWPA2PSK";
	case Ndis802_11AuthModeWPA1WPA2:
		return "WPA1WPA2";
	case Ndis802_11AuthModeWPANone:
		return "WPANONE";
	default:
		return "UNKNOW";
	}
}

PSTRING RTMPGetRalinkEncryModeStr(USHORT encryMode)
{
	switch (encryMode) {
	case Ndis802_11WEPDisabled:
		return "NONE";
	case Ndis802_11WEPEnabled:
		return "WEP";
	case Ndis802_11TKIPEnable:
		return "TKIP";
	case Ndis802_11AESEnable:
		return "AES";
	case Ndis802_11TKIPAESMix:
		return "TKIPAES";
	default:
		return "UNKNOW";
	}
}

#ifdef DBG
static int Show_SSID_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	UCHAR ssid_str[33];

	NdisZeroMemory(&ssid_str[0], 33);
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
		POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
		NdisMoveMemory(&ssid_str[0], pAd->ApCfg.MBSSID[pObj->ioctl_if].Ssid,
				pAd->ApCfg.MBSSID[pObj->ioctl_if].SsidLen);
	}
#endif

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd) {
		NdisMoveMemory(&ssid_str[0], pAd->CommonCfg.Ssid,
				pAd->CommonCfg.SsidLen);
	}
#endif
	snprintf(pBuf, BufLen, "\t%s", ssid_str);
	return 0;
}


static int Show_WirelessMode_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	switch (pAd->CommonCfg.PhyMode) {
	case (WMODE_B | WMODE_G):
		snprintf(pBuf, BufLen, "\t11B/G");
		break;
	case (WMODE_B):
		snprintf(pBuf, BufLen, "\t11B");
		break;
	case (WMODE_A):
		snprintf(pBuf, BufLen, "\t11A");
		break;
	case (WMODE_A | WMODE_B | WMODE_G):
		snprintf(pBuf, BufLen, "\t11A/B/G");
		break;
	case (WMODE_G):
		snprintf(pBuf, BufLen, "\t11G");
		break;
#ifdef DOT11_N_SUPPORT
	case (WMODE_A | WMODE_B | WMODE_G | WMODE_GN | WMODE_AN):
		snprintf(pBuf, BufLen, "\t11A/B/G/N");
		break;
	case (WMODE_GN):
		snprintf(pBuf, BufLen, "\t11N only with 2.4G");
		break;
	case (WMODE_G | WMODE_GN):
		snprintf(pBuf, BufLen, "\t11G/N");
		break;
	case (WMODE_A | WMODE_AN):
		snprintf(pBuf, BufLen, "\t11A/N");
		break;
	case (WMODE_B | WMODE_G | WMODE_GN):
		snprintf(pBuf, BufLen, "\t11B/G/N");
		break;
	case (WMODE_A | WMODE_G | WMODE_GN | WMODE_AN):
		snprintf(pBuf, BufLen, "\t11A/G/N");
		break;
	case (WMODE_AN):
		snprintf(pBuf, BufLen, "\t11N only with 5G");
		break;
#endif /* DOT11_N_SUPPORT */
	default:
		snprintf(pBuf, BufLen, "\tUnknow Value(%d)", pAd->CommonCfg.PhyMode);
		break;
	}

	return 0;
}

static int Show_TxBurst_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	snprintf(pBuf, BufLen, "\t%s", pAd->CommonCfg.bEnableTxBurst ?
			"TRUE":"FALSE");
	return 0;
}

static int Show_TxPreamble_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	switch(pAd->CommonCfg.TxPreamble) {
	case Rt802_11PreambleShort:
		snprintf(pBuf, BufLen, "\tShort");
		break;
	case Rt802_11PreambleLong:
		snprintf(pBuf, BufLen, "\tLong");
		break;
	case Rt802_11PreambleAuto:
		snprintf(pBuf, BufLen, "\tAuto");
		break;
	default:
		snprintf(pBuf, BufLen, "\tUnknown Value(%lu)",
				pAd->CommonCfg.TxPreamble);
		break;
	}

	return 0;
}


static int Show_TxPower_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	snprintf(pBuf, BufLen, "\t%lu", pAd->CommonCfg.TxPowerPercentage);
	return 0;
}


static int Show_Channel_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	snprintf(pBuf, BufLen, "\t%d", pAd->CommonCfg.Channel);
	return 0;
}

static int Show_BGProtection_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	switch (pAd->CommonCfg.UseBGProtection) {
	case 1: /*Always On*/
		snprintf(pBuf, BufLen, "\tON");
		break;
	case 2: /*Always OFF*/
		snprintf(pBuf, BufLen, "\tOFF");
		break;
	case 0: /*AUTO*/
		snprintf(pBuf, BufLen, "\tAuto");
		break;
	default:
		snprintf(pBuf, BufLen, "\tUnknow Value(%lu)",
				pAd->CommonCfg.UseBGProtection);
		break;
	}
	return 0;
}

static int Show_RTSThreshold_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	snprintf(pBuf, BufLen, "\t%u", pAd->CommonCfg.RtsThreshold);
	return 0;
}

static int Show_FragThreshold_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	snprintf(pBuf, BufLen, "\t%u", pAd->CommonCfg.FragmentThreshold);
	return 0;
}


#ifdef DOT11_N_SUPPORT
static int Show_HtBw_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	if (pAd->CommonCfg.RegTransmitSetting.field.BW == BW_40) {
		snprintf(pBuf, BufLen, "\t40 MHz");
	} else {
		snprintf(pBuf, BufLen, "\t20 MHz");
	}
	return 0;
}

static int Show_HtMcs_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	struct wifi_dev *wdev = NULL;
#ifdef CONFIG_AP_SUPPORT
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		wdev = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		wdev = &pAd->StaCfg.wdev;
#endif /* CONFIG_STA_SUPPORT */
	if (wdev)
		snprintf(pBuf, BufLen, "\t%u", wdev->DesiredTransmitSetting.field.MCS);
	return 0;
}

static int Show_HtGi_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	switch(pAd->CommonCfg.RegTransmitSetting.field.ShortGI) {
	case GI_400:
		snprintf(pBuf, BufLen, "\tGI_400");
		break;
	case GI_800:
		snprintf(pBuf, BufLen, "\tGI_800");
		break;
	default:
		snprintf(pBuf, BufLen, "\tUnknow Value(%u)",
				pAd->CommonCfg.RegTransmitSetting.field.ShortGI);
		break;
	}
	return 0;
}

static int Show_HtOpMode_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	switch(pAd->CommonCfg.RegTransmitSetting.field.HTMODE) {
	case HTMODE_GF:
		snprintf(pBuf, BufLen, "\tGF");
		break;
	case HTMODE_MM:
		snprintf(pBuf, BufLen, "\tMM");
		break;
	default:
		snprintf(pBuf, BufLen, "\tUnknow Value(%u)",
				pAd->CommonCfg.RegTransmitSetting.field.HTMODE);
		break;
	}
	return 0;
}

static int Show_HtExtcha_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	switch(pAd->CommonCfg.RegTransmitSetting.field.EXTCHA) {
	case EXTCHA_BELOW:
		snprintf(pBuf, BufLen, "\tBelow");
		break;
	case EXTCHA_ABOVE:
		snprintf(pBuf, BufLen, "\tAbove");
		break;
	default:
		snprintf(pBuf, BufLen, "\tUnknow Value(%u)",
				pAd->CommonCfg.RegTransmitSetting.field.EXTCHA);
		break;
	}
	return 0;
}
#endif // DBG

#ifdef DBG
static int Show_HtMpduDensity_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	snprintf(pBuf, BufLen, "\t%u", pAd->CommonCfg.BACapability.field.MpduDensity);
	return 0;
}

static int Show_HtBaWinSize_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	snprintf(pBuf, BufLen, "\t%u", pAd->CommonCfg.BACapability.field.RxBAWinLimit);
	return 0;
}

static int Show_HtRdg_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	snprintf(pBuf, BufLen, "\t%s", pAd->CommonCfg.bRdg ? "TRUE":"FALSE");
	return 0;
}

static int Show_HtAmsdu_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	snprintf(pBuf, BufLen, "\t%s", pAd->CommonCfg.BACapability.field.AmsduEnable ? "TRUE":"FALSE");
	return 0;
}

static int Show_HtAutoBa_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	snprintf(pBuf, BufLen, "\t%s", pAd->CommonCfg.BACapability.field.AutoBA ? "TRUE":"FALSE");
	return 0;
}
#endif //DBG
#endif /* DOT11_N_SUPPORT */

#ifdef DBG
static int Show_CountryRegion_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	snprintf(pBuf, BufLen, "\t%d", pAd->CommonCfg.CountryRegion);
	return 0;
}

static int Show_CountryRegionABand_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	snprintf(pBuf, BufLen, "\t%d", pAd->CommonCfg.CountryRegionForABand);
	return 0;
}

static int Show_CountryCode_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	snprintf(pBuf, BufLen, "\t%s", pAd->CommonCfg.CountryCode);
	return 0;
}


#ifdef AGGREGATION_SUPPORT
static int Show_PktAggregate_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	snprintf(pBuf, BufLen, "\t%s", pAd->CommonCfg.bAggregationCapable ? "TRUE":"FALSE");
	return 0;
}
#endif

static int Show_WmmCapable_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
#ifdef CONFIG_AP_SUPPORT
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		snprintf(pBuf, BufLen, "\t%s", pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.bWmmCapable ? "TRUE":"FALSE");
#endif

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		snprintf(pBuf, BufLen, "\t%s", pAd->CommonCfg.bWmmCapable ? "TRUE":"FALSE");
#endif

	return 0;
}

static int Show_IEEE80211H_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	snprintf(pBuf, BufLen, "\t%s", pAd->CommonCfg.bIEEE80211H ? "TRUE":"FALSE");
	return 0;
}

#ifdef CONFIG_STA_SUPPORT
static int Show_NetworkType_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	switch (pAd->StaCfg.BssType) {
	case BSS_ADHOC:
		snprintf(pBuf, BufLen, "\tAdhoc");
		break;
	case BSS_INFRA:
		snprintf(pBuf, BufLen, "\tInfra");
		break;
	case BSS_ANY:
		snprintf(pBuf, BufLen, "\tAny");
		break;
	case BSS_MONITOR:
		snprintf(pBuf, BufLen, "\tMonitor");
		break;
	default:
		sprintf(pBuf, "\tUnknow Value(%d)", pAd->StaCfg.BssType);
		break;
	}
	return 0;
}

static int Show_WPAPSK_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	if ((pAd->StaCfg.WpaPassPhraseLen >= 8) &&
		(pAd->StaCfg.WpaPassPhraseLen < 64)) {
		snprintf(pBuf, BufLen, "\tWPAPSK = %s", pAd->StaCfg.WpaPassPhrase);
	} else {
		int idx;
		snprintf(pBuf, BufLen, "\tWPAPSK = ");
		for (idx = 0; idx < 32; idx++)
			snprintf(pBuf+strlen(pBuf), BufLen-strlen(pBuf),
					"%02X", pAd->StaCfg.WpaPassPhrase[idx]);
	}

	return 0;
}

static int Show_AutoReconnect_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	snprintf(pBuf, BufLen, "\tAutoReconnect = %d", pAd->StaCfg.bAutoReconnect);
	return 0;
}

#endif /* CONFIG_STA_SUPPORT */

static int Show_AuthMode_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	NDIS_802_11_AUTHENTICATION_MODE AuthMode = Ndis802_11AuthModeOpen;
	struct wifi_dev *wdev = NULL;
#ifdef CONFIG_AP_SUPPORT
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		wdev  = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;
#endif

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		wdev = &pAd->StaCfg.wdev;
#endif
	if (!wdev)
		return -1;

	AuthMode = wdev->AuthMode;
	if ((AuthMode >= Ndis802_11AuthModeOpen) &&
		(AuthMode <= Ndis802_11AuthModeWPA1PSKWPA2PSK))
		snprintf(pBuf, BufLen, "\t%s", RTMPGetRalinkAuthModeStr(AuthMode));
	else
		snprintf(pBuf, BufLen, "\tUnknow Value(%d)", AuthMode);

	return 0;
}

static int Show_EncrypType_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	struct wifi_dev *wdev = NULL;
	NDIS_802_11_WEP_STATUS WepStatus = Ndis802_11WEPDisabled;
#ifdef CONFIG_AP_SUPPORT
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		wdev = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;
#endif

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		wdev = &pAd->StaCfg.wdev;
#endif
	if (!wdev)
		return -1;

	WepStatus = wdev->WepStatus;
	if ((WepStatus >= Ndis802_11WEPEnabled) &&
		(WepStatus <= Ndis802_11Encryption4KeyAbsent))
		snprintf(pBuf, BufLen, "\t%s", RTMPGetRalinkEncryModeStr(WepStatus));
	else
		snprintf(pBuf, BufLen, "\tUnknow Value(%d)", WepStatus);

	return 0;
}

static int Show_DefaultKeyID_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	struct wifi_dev *wdev = NULL;
#ifdef CONFIG_AP_SUPPORT
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		wdev = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;
#endif

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		wdev = &pAd->StaCfg.wdev;
#endif
	if (!wdev)
		return -1;

	snprintf(pBuf, BufLen, "\t%d", wdev->DefaultKeyId);
	return 0;
}

static int Show_WepKey_Proc(PRTMP_ADAPTER pAd, int KeyIdx, PSTRING pBuf, ULONG BufLen)
{
	UCHAR   Key[16] = {0}, KeyLength = 0;
	int index = BSS0;
#ifdef CONFIG_AP_SUPPORT
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		index = pObj->ioctl_if;
#endif
	KeyLength = pAd->SharedKey[index][KeyIdx].KeyLen;
	NdisMoveMemory(Key, pAd->SharedKey[index][KeyIdx].Key, KeyLength);

	/*check key string is ASCII or not*/
	if (RTMPCheckStrPrintAble((PCHAR)Key, KeyLength)) {
		sprintf(pBuf, "\t%s", Key);
	} else {
		int idx;
		sprintf(pBuf, "\t");
		for (idx = 0; idx < KeyLength; idx++)
			sprintf(pBuf+strlen(pBuf), "%02X", Key[idx]);
	}
	return 0;
}

static int Show_Key1_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	Show_WepKey_Proc(pAd, 0, pBuf, BufLen);
	return 0;
}

static int Show_Key2_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	Show_WepKey_Proc(pAd, 1, pBuf, BufLen);
	return 0;
}

static int Show_Key3_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	Show_WepKey_Proc(pAd, 2, pBuf, BufLen);
	return 0;
}

static int Show_Key4_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	Show_WepKey_Proc(pAd, 3, pBuf, BufLen);
	return 0;
}

static int Show_PMK_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	int idx;
	UCHAR PMK[32] = {0};

#ifdef CONFIG_AP_SUPPORT
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		NdisMoveMemory(PMK, pAd->ApCfg.MBSSID[pObj->ioctl_if].PMK, 32);
#endif

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		NdisMoveMemory(PMK, pAd->StaCfg.PMK, 32);
#endif
	sprintf(pBuf, "\tPMK = ");
	for (idx = 0; idx < 32; idx++)
		sprintf(pBuf+strlen(pBuf), "%02X", PMK[idx]);

	return 0;
}
#endif //DBG


static int Show_STA_RAInfo_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	sprintf(pBuf, "\n");
#ifdef PRE_ANT_SWITCH
	sprintf(pBuf+strlen(pBuf), "PreAntSwitch: %d\n", pAd->CommonCfg.PreAntSwitch);
	sprintf(pBuf+strlen(pBuf), "PreAntSwitchRSSI: %d\n", pAd->CommonCfg.PreAntSwitchRSSI);
#endif

#ifdef NEW_RATE_ADAPT_SUPPORT
	sprintf(pBuf+strlen(pBuf), "LowTrafficThrd: %d\n", pAd->CommonCfg.lowTrafficThrd);
	sprintf(pBuf+strlen(pBuf), "TrainUpRule: %d\n", pAd->CommonCfg.TrainUpRule);
	sprintf(pBuf+strlen(pBuf), "TrainUpRuleRSSI: %d\n", pAd->CommonCfg.TrainUpRuleRSSI);
	sprintf(pBuf+strlen(pBuf), "TrainUpLowThrd: %d\n", pAd->CommonCfg.TrainUpLowThrd);
	sprintf(pBuf+strlen(pBuf), "TrainUpHighThrd: %d\n", pAd->CommonCfg.TrainUpHighThrd);
#endif

#ifdef STREAM_MODE_SUPPORT
	sprintf(pBuf+strlen(pBuf), "StreamMode: %d\n", pAd->CommonCfg.StreamMode);
	sprintf(pBuf+strlen(pBuf), "StreamModeMCS: 0x%04x\n", pAd->CommonCfg.StreamModeMCS);
#endif

#ifdef TXBF_SUPPORT
	sprintf(pBuf+strlen(pBuf), "ITxBfEn: %d\n", pAd->CommonCfg.RegTransmitSetting.field.ITxBfEn);
	sprintf(pBuf+strlen(pBuf), "ITxBfTimeout: %ld\n", pAd->CommonCfg.ITxBfTimeout);
	sprintf(pBuf+strlen(pBuf), "ETxBfTimeout: %ld\n", pAd->CommonCfg.ETxBfTimeout);
	sprintf(pBuf+strlen(pBuf), "ETxBfEnCond: %ld\n", pAd->CommonCfg.ETxBfEnCond);
	sprintf(pBuf+strlen(pBuf), "ETxBfNoncompress: %d\n", pAd->CommonCfg.ETxBfNoncompress);
	sprintf(pBuf+strlen(pBuf), "ETxBfIncapable: %d\n", pAd->CommonCfg.ETxBfIncapable);
#endif

#ifdef DBG_CTRL_SUPPORT
	sprintf(pBuf+strlen(pBuf), "DebugFlags: 0x%lx\n", pAd->CommonCfg.DebugFlags);
#endif
	return 0;
}

#ifdef DBG
#ifdef CONFIG_AP_SUPPORT
static BOOLEAN Show_TxInfo_Proc(RTMP_ADAPTER *pAd, PSTRING arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR apidx = pObj->ioctl_if;
	int i;

	printk("BSS%d\n", apidx);
	printk("TX Retry Count: %7d\n", pAd->ApCfg.MBSSID[apidx].StatTxRetryOkCount);
	printk("TX Fail  Count: %7d\n", pAd->ApCfg.MBSSID[apidx].StatTxFailCount);
	printk("--------------------------------------\n");

	for (i = 0; i < MAX_LEN_OF_MAC_TABLE; i++) {
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];
		if ((IS_ENTRY_CLIENT(pEntry)) &&
			(pEntry->apidx == apidx) && (pEntry->Sst == SST_ASSOC)) {
			printk("%02X:%02X:%02X:%02X:%02X:%02X ==>", PRINT_MAC(pEntry->Addr));
			printk("TX Retry(%7d), Tx Fail(%7d)\n",
					pEntry->StatTxRetryOkCount,
					pEntry->StatTxFailCount);
		}
	}
}
#endif /* CONFIG_AP_SUPPORT */
#endif //DBG

BOOLEAN Show_MacTable_Proc(RTMP_ADAPTER *pAd, PSTRING arg)
{
	int i;
	UINT32 RegValue;
	ULONG DataRate = 0;

	printk("\n");
	RTMP_IO_READ32(pAd, BKOFF_SLOT_CFG, &RegValue);
	printk("BackOff Slot      : %s slot time, BKOFF_SLOT_CFG(0x1104) = 0x%08x\n",
			OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_SHORT_SLOT_INUSED) ? "short" : "long",
 			RegValue);

#ifdef DOT11_N_SUPPORT
	printk("HT Operating Mode : %d\n", pAd->CommonCfg.AddHTInfo.AddHtInfo2.OperaionMode);
	printk("\n");
#endif
	printk("\n%-19s%-4s%-4s%-4s%-4s%-8s",
		   "MAC", "AID", "BSS", "PSM", "WMM", "MIMOPS");

	if (pAd->CommonCfg.RxStream == 3)
		printk("%-7s%-7s%-7s","RSSI0", "RSSI1","RSSI2");
	else if (pAd->CommonCfg.RxStream == 2)
		printk("%-7s%-7s", "RSSI0", "RSSI1");
	else
		printk("%-7s", "RSSI0");

	printk("%-10s%-6s%-6s%-6s%-6s%-7s%-7s%-7s\n", "PhMd", "BW", "MCS", "SGI", "STBC", "Idle", "Rate", "TIME");

	for (i = 0; i < MAX_LEN_OF_MAC_TABLE; i++) {
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];
		if ((IS_ENTRY_CLIENT(pEntry) || (IS_ENTRY_APCLI(pEntry)))
			&& (pEntry->Sst == SST_ASSOC)) {
			DataRate=0;
			//getRate(pEntry->HTPhyMode, &DataRate);
			RtmpDrvRateGet(pAd, pEntry->HTPhyMode.field.MODE, pEntry->HTPhyMode.field.ShortGI,
					pEntry->HTPhyMode.field.BW, pEntry->HTPhyMode.field.MCS,
					newRateGetAntenna(pEntry->MaxHTPhyMode.field.MCS), &DataRate);
			DataRate /= 500000;
			DataRate /= 2;

			printk("%02X:%02X:%02X:%02X:%02X:%02X  ", PRINT_MAC(pEntry->Addr));
			printk("%-4d", (int)pEntry->Aid);
			printk("%-4d", (int)pEntry->apidx);
			printk("%-4d", (int)pEntry->PsMode);
			printk("%-4d", (int)CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE));
#ifdef DOT11_N_SUPPORT
			printk("%-8d", (int)pEntry->MmpsMode);
#endif
			if (pAd->CommonCfg.RxStream == 3)
				printk("%-7d%-7d%-7d", pEntry->RssiSample.AvgRssi0, pEntry->RssiSample.AvgRssi1, pEntry->RssiSample.AvgRssi2);
			else if (pAd->CommonCfg.RxStream == 2)
				printk("%-7d%-7d", pEntry->RssiSample.AvgRssi0, pEntry->RssiSample.AvgRssi1);
			else
				printk("%-7d", pEntry->RssiSample.AvgRssi0);

			printk("%-10s", get_phymode_str(pEntry->HTPhyMode.field.MODE));
			printk("%-6s", get_bw_str(pEntry->HTPhyMode.field.BW));
#ifdef DOT11_VHT_AC
			if (pEntry->HTPhyMode.field.MODE == MODE_VHT)
				printk("%dS-M%-2d", ((pEntry->HTPhyMode.field.MCS>>4) + 1), (pEntry->HTPhyMode.field.MCS & 0xf));
			else
#endif /* DOT11_VHT_AC */
			printk("%-6d", pEntry->HTPhyMode.field.MCS);
			printk("%-6d", pEntry->HTPhyMode.field.ShortGI);
			printk("%-6d", pEntry->HTPhyMode.field.STBC);
			printk("%-7d", (int)(pEntry->StaIdleTimeout - pEntry->NoDataIdleCount));
			printk("%-7d", (int)DataRate);
#ifdef CONFIG_AP_SUPPORT
			printk("%-7d", (int)pEntry->StaConnectTime);
#endif
			printk("\t\t\t\t\t\t\t%-10d, %d, %d%%\n", pEntry->DebugFIFOCount, pEntry->DebugTxCount,
					(pEntry->DebugTxCount) ? ((pEntry->DebugTxCount-pEntry->DebugFIFOCount)*100/pEntry->DebugTxCount) : 0);
			printk(" MaxCap:%-10s", get_phymode_str(pEntry->MaxHTPhyMode.field.MODE));
			printk("%-6s", get_bw_str(pEntry->MaxHTPhyMode.field.BW));
#ifdef DOT11_VHT_AC
			if (pEntry->MaxHTPhyMode.field.MODE == MODE_VHT)
				printk("%dS-M%d", ((pEntry->MaxHTPhyMode.field.MCS>>4) + 1), (pEntry->MaxHTPhyMode.field.MCS & 0xf));
			else
#endif
			printk("%-6d", pEntry->MaxHTPhyMode.field.MCS);
			printk("%-6d", pEntry->MaxHTPhyMode.field.ShortGI);
			printk("%-6d\n", pEntry->MaxHTPhyMode.field.STBC);
			printk("\n");
		}
	}

	return TRUE;
}

//global non-debug?
BOOLEAN show_devinfo_proc(RTMP_ADAPTER *pAd, PSTRING arg)
{
	UCHAR *pstr;
	int i, wcid;

	DBGPRINT(RT_DEBUG_OFF, ("Device MAC\n"));
	if (pAd->OpMode == OPMODE_AP)
		pstr = "AP";
	else if (pAd->OpMode == OPMODE_STA)
		pstr = "STA";
	else
		pstr = "Unknown";

	DBGPRINT(RT_DEBUG_OFF, ("Operation Mode: %s\n", pstr));

	pstr = wmode_2_str(pAd->CommonCfg.PhyMode);
	if (pstr) {
		DBGPRINT(RT_DEBUG_OFF, ("WirelessMode: %s(%d)\n", pstr,
				pAd->CommonCfg.PhyMode));
		os_free_mem(pstr);
	}

	DBGPRINT(RT_DEBUG_OFF, ("Channel: %d\n", pAd->CommonCfg.Channel));
	DBGPRINT(RT_DEBUG_OFF, ("\tCentralChannel: %d\n", pAd->CommonCfg.CentralChannel));

#ifdef DOT11_VHT_AC
	if (WMODE_CAP_AC(pAd->CommonCfg.PhyMode))
		DBGPRINT(RT_DEBUG_OFF, ("\tVHT CentralChannel: %d\n", pAd->CommonCfg.vht_cent_ch));
#endif /* DOT11_VHT_AC */
	DBGPRINT(RT_DEBUG_OFF, ("\tRF Channel: %d\n", pAd->LatchRfRegs.Channel));

	DBGPRINT(RT_DEBUG_OFF, ("Bandwidth\n"));
	pstr = (pAd->CommonCfg.RegTransmitSetting.field.BW) ? "20/40" : "20";
	DBGPRINT(RT_DEBUG_OFF, ("\tHT-BW: %s\n", pstr));
#ifdef DOT11_VHT_AC
	if (WMODE_CAP_AC(pAd->CommonCfg.PhyMode)) {
		if (pAd->CommonCfg.vht_bw)
			pstr = "80";
		DBGPRINT(RT_DEBUG_OFF, ("\tVHT-BW: %s\n", pstr));
	}
#endif

#ifdef RT65xx
	if (IS_RT65XX(pAd)) {
		dump_bw_info(pAd);
#ifdef RT8592
		if (IS_RT8592(pAd))
			dump_pwr_info(pAd);
#endif /* RT8592 */
	}
#endif /* RT65xx */

	DBGPRINT(RT_DEBUG_OFF, ("Security\n"));
	DBGPRINT(RT_DEBUG_OFF, ("MAX_LEN_OF_MAC_TABLE: %d\n", MAX_LEN_OF_MAC_TABLE));
	DBGPRINT(RT_DEBUG_OFF, ("MAX_NUMBER_OF_MAC: %d\n", MAX_NUMBER_OF_MAC));
	DBGPRINT(RT_DEBUG_OFF, ("HW_RESERVED_WCID: %d\n", HW_RESERVED_WCID(pAd)));
	DBGPRINT(RT_DEBUG_OFF, ("LAST_SPECIFIC_WCID: %d\n", LAST_SPECIFIC_WCID(pAd)));
	DBGPRINT(RT_DEBUG_OFF, ("MAX_AVAILABLE_CLIENT_WCID: %d\n", MAX_AVAILABLE_CLIENT_WCID(pAd)));

	for (i = 0; i < MAX_MBSSID_NUM(pAd); i++) {
		GET_GroupKey_WCID(pAd, wcid, i);
		DBGPRINT(RT_DEBUG_OFF, ("%d GROUPKEY_WCID: %d\n", i, wcid));
	}

	return TRUE;
}


#ifdef DBG
static CHAR *wdev_type_str[]={"AP", "STA", "ADHOC", "WDS", "MESH", "Unknown"};

static PSTRING wdev_type2str(int type)
{
	switch (type) {
	case WDEV_TYPE_AP:
		return wdev_type_str[0];
	case WDEV_TYPE_STA:
		return wdev_type_str[1];
	case WDEV_TYPE_ADHOC:
		return wdev_type_str[2];
	case WDEV_TYPE_WDS:
		return wdev_type_str[3];
	case WDEV_TYPE_MESH:
		return wdev_type_str[4];
	default:
		return wdev_type_str[5];
	}
}
#endif /* DBG */

BOOLEAN show_sysinfo_proc(RTMP_ADAPTER *pAd, PSTRING arg)
{
	int idx;
	UINT32 total_size = 0, cntr_size;
	struct wifi_dev *wdev;

	DBGPRINT(RT_DEBUG_OFF, ("Device Instance\n"));
	for (idx = 0; idx < WDEV_NUM_MAX; idx++) {
		DBGPRINT(RT_DEBUG_OFF, ("\tWDEV %02d:", idx));
		if (pAd->wdev_list[idx]) {
			UCHAR *str = NULL;

			wdev = pAd->wdev_list[idx];
			DBGPRINT(RT_DEBUG_OFF, ("\n\t\tName/Type:%s/%s\n",
						RTMP_OS_NETDEV_GET_DEVNAME(wdev->if_dev),
						wdev_type2str(wdev->wdev_type)));
			DBGPRINT(RT_DEBUG_OFF, ("\t\tListIdx:%d\n", wdev->wdev_idx));
			DBGPRINT(RT_DEBUG_OFF, ("\t\tMacAddr:%02x:%02x:%02x:%02x:%02x:%02x\n",
						PRINT_MAC(wdev->if_addr)));
			DBGPRINT(RT_DEBUG_OFF, ("\t\tBSSID:%02x:%02x:%02x:%02x:%02x:%02x\n",
						PRINT_MAC(wdev->bssid)));
			str = wmode_2_str(wdev->PhyMode);
			if (str) {
				DBGPRINT(RT_DEBUG_OFF, ("\t\tPhyMode:%s\n", str));
				os_free_mem(str);
			}
			DBGPRINT(RT_DEBUG_OFF, ("\t\tChannel:%d\n", wdev->channel));
			DBGPRINT(RT_DEBUG_OFF, ("\t\tPortSecured/AllowTx: %d:%d\n",
						wdev->PortSecured, wdev->allow_data_tx));
			DBGPRINT(RT_DEBUG_OFF, ("\t\ttx_pkt_checker:%p\n", wdev->tx_pkt_allowed));

			DBGPRINT(RT_DEBUG_OFF, ("\t\tif_dev:0x%p \tfunc_dev:0x%p \tsys_handle:0x%p\n",
						wdev->if_dev, wdev->func_dev, wdev->sys_handle));
		} else {
			DBGPRINT(RT_DEBUG_OFF, ("\n"));
		}
	}

	DBGPRINT(RT_DEBUG_OFF, ("Memory Statistics:\n"));
	DBGPRINT(RT_DEBUG_OFF, ("\tsize>\n"));
	DBGPRINT(RT_DEBUG_OFF, ("\t\tpAd = \t\t%lu bytes\n\n", sizeof(*pAd)));
	DBGPRINT(RT_DEBUG_OFF, ("\t\t\tCommonCfg = \t%lu bytes\n", sizeof(pAd->CommonCfg)));
	total_size += sizeof(pAd->CommonCfg);
#ifdef CONFIG_AP_SUPPORT
	DBGPRINT(RT_DEBUG_OFF, ("\t\t\tApCfg = \t%lu bytes\n", sizeof(pAd->ApCfg)));
	total_size += sizeof(pAd->ApCfg);
	DBGPRINT(RT_DEBUG_OFF, ("\t\t\t\tMBSSID = \t%d B (PerMBSS =%lu B, Total MBSS Num= %d)\n",
			sizeof(pAd->ApCfg.MBSSID), sizeof(struct _MULTISSID_STRUCT), HW_BEACON_MAX_NUM));
#ifdef APCLI_SUPPORT
	DBGPRINT(RT_DEBUG_OFF, ("\t\t\t\t\tAPCLI = \t%lu bytes (PerAPCLI =%lu bytes, Total APCLI Num= %d)\n",
			sizeof(pAd->ApCfg.ApCliTab), sizeof(struct _APCLI_STRUCT), MAX_APCLI_NUM));
#endif
#endif /* CONFIG_AP_SUPPORT */

	DBGPRINT(RT_DEBUG_OFF, ("\t\t\tMgmtRing = \t%lu bytes\n", sizeof(pAd->MgmtRing)));
	total_size += sizeof(pAd->MgmtRing);

	DBGPRINT(RT_DEBUG_OFF, ("\t\t\tMlme = \t%lu bytes\n", sizeof(pAd->Mlme)));
	total_size += sizeof(pAd->Mlme);
#ifdef CONFIG_STA_SUPPORT
	DBGPRINT(RT_DEBUG_OFF, ("\t\t\tMlmeAux = \t%lu bytes\n", sizeof(pAd->MlmeAux)));
	total_size += sizeof(pAd->MlmeAux);
#endif
	DBGPRINT(RT_DEBUG_OFF, ("\t\t\tMacTab = \t%lu bytes\n", sizeof(pAd->MacTab)));
	total_size += sizeof(pAd->MacTab);

#ifdef DOT11_N_SUPPORT
	DBGPRINT(RT_DEBUG_OFF, ("\t\t\tBATable = \t%lu bytes\n", sizeof(pAd->BATable)));
	total_size += sizeof(pAd->BATable);
#endif
	cntr_size = sizeof(pAd->Counters8023) + sizeof(pAd->WlanCounters) +
			sizeof(pAd->RalinkCounters) + /* sizeof(pAd->DrsCounters) */+
			sizeof(pAd->PrivateInfo);
	DBGPRINT(RT_DEBUG_OFF, ("\t\t\tCounter** = \t%u bytes\n", cntr_size));
	total_size += cntr_size;

#if defined (AP_SCAN_SUPPORT) || defined (CONFIG_STA_SUPPORT)
	DBGPRINT(RT_DEBUG_OFF, ("\t\t\tScanTab = \t%lu bytes\n", sizeof(pAd->ScanTab)));
	total_size += sizeof(pAd->ScanTab);
#endif
	DBGPRINT(RT_DEBUG_OFF, ("\tsize> Total = \t\t%d bytes, Others = %lu bytes\n\n",
			total_size, sizeof(*pAd)-total_size));

	return TRUE;
}


BOOLEAN show_trinfo_proc(RTMP_ADAPTER *pAd, PSTRING arg)
{
#ifdef RTMP_MAC_USB
	if (IS_USB_INF(pAd)) {
		DBGPRINT(RT_DEBUG_OFF, ("TxRing Configuration\n"));
		DBGPRINT(RT_DEBUG_OFF, ("\nRxRing Configuration\n"));
		DBGPRINT(RT_DEBUG_OFF, ("\nPBF Configuration\n"));
	}
#endif
	return TRUE;
}


#ifdef SINGLE_SKU
//JB: ???? three arguments?
BOOLEAN Show_ModuleTxpower_Proc(PRTMP_ADAPTER pAd, PSTRING pBuf, ULONG BufLen)
{
	snprintf(pBuf, BufLen, "\tModuleTxpower = %d", pAd->CommonCfg.ModuleTxpower);
//JB:Moron	return 0;
	retrun TRUE;
}
#endif /* SINGLE_SKU */

#ifdef APCLI_SUPPORT
BOOLEAN RTMPIoctlConnStatus(PRTMP_ADAPTER pAd, PSTRING arg)
{
	int i;
	POS_COOKIE pObj;
	UCHAR ifIndex;
	BOOLEAN bConnect = FALSE;

	pObj = (POS_COOKIE) pAd->OS_Cookie;

	DBGPRINT(RT_DEBUG_TRACE, ("==>RTMPIoctlConnStatus\n"));

	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;

	DBGPRINT(RT_DEBUG_OFF, ("=============================================================\n"));
	if ((pAd->ApCfg.ApCliTab[ifIndex].CtrlCurrState == APCLI_CTRL_CONNECTED)
		&& (pAd->ApCfg.ApCliTab[ifIndex].SsidLen != 0)) {
		for (i = 0; i < MAX_LEN_OF_MAC_TABLE; i++) {
			PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];

			if (IS_ENTRY_APCLI(pEntry)
				&& (pEntry->Sst == SST_ASSOC)
				&& (pEntry->PortSecured == WPA_802_1X_PORT_SECURED)) {
				DBGPRINT(RT_DEBUG_OFF, 
						("ApCli%d Connected AP : %02X:%02X:%02X:%02X:%02X:%02X   SSID:%s\n",
						ifIndex, PRINT_MAC(pEntry->Addr),
						pAd->ApCfg.ApCliTab[ifIndex].Ssid));
				bConnect = TRUE;
			}
		}

		if (!bConnect)
			DBGPRINT(RT_DEBUG_OFF, ("ApCli%d Connected AP : Disconnect\n",ifIndex));
	} else {
		DBGPRINT(RT_DEBUG_OFF, ("ApCli%d Connected AP : Disconnect\n",ifIndex));
	}

	DBGPRINT(RT_DEBUG_OFF, ("=============================================================\n"));
	 	DBGPRINT(RT_DEBUG_TRACE, ("<==RTMPIoctlConnStatus\n"));
	return TRUE;
}
#endif /*APCLI_SUPPORT*/

#ifdef TXBF_SUPPORT
/*
	Set_InvTxBfTag_Proc - Invalidate BF Profile Tags
	usage: "iwpriv ra0 set InvTxBfTag=n"
	Reset Valid bit and zero out MAC address of each profile. The next profile will be stored in profile 0
*/
BOOLEAN Set_InvTxBfTag_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	int profileNum;
	UINT rValue;

	/* Disable Profile Updates during access */
#ifndef MT76x2
	UCHAR row[EXP_MAX_BYTES];
	UCHAR r163Value = 0;

	RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R163, &r163Value);
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R163, r163Value & ~0x88);

	/* Invalidate Implicit tags */
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R179, 0);
	for (profileNum=0; profileNum<4; profileNum++) {
		Read_TagField(pAd, row, profileNum);
		row[0] &= 0x7F;
		row[1] = row[2] = row[3] = row[4] = row[5] = row[6] = 0xAA;
		Write_TagField(pAd, row, profileNum);
	}

	/* Invalidate Explicit tags */
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R179, 4);
	for (profileNum=0; profileNum<4; profileNum++) {
		Read_TagField(pAd, row, profileNum);
		row[0] &= 0x7F;
		row[1] = row[2] = row[3] = row[4] = row[5] = row[6] = 0x55;
		Write_TagField(pAd, row, profileNum);
	}

	/* Restore Profile Updates */
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R163, r163Value);
#else
	PFMU_PROFILE *prof;

	prof = os_alloc_mem(sizeof(PROFILE_DATA));
	if (!prof) {
//JB: Moron		return -3;
		return FALSE;
	}

	RTMP_IO_READ32(pAd, PFMU_R1, &rValue);
	RTMP_IO_WRITE32(pAd, PFMU_R1, rValue & ~0x330);

	for (profileNum = 0; profileNum < 8; profileNum++) {
		// Clear the profile valid bit
		prof->validFlg = 0;
		TxBfProfileTagValid(pAd, prof, profileNum);

		// Clear the mac address
		TxBfProfileTagRead(pAd, prof, profileNum);
		prof->DMAC[0] = prof->DMAC[1] = prof->DMAC[2] = 0xFF;
		prof->DMAC[3] = prof->DMAC[4] = prof->DMAC[5] = 0xFF;
		prof->CMDInIdx = 2;
		TxBfProfileTagWrite(pAd, prof, profileNum);
	}

	/* Restore Profile Updates */
	RTMP_IO_WRITE32(pAd, PFMU_R1, rValue);

	os_free_mem(prof);
#endif

	return TRUE;
}


/*
	Set_ETxBfCodebook_Proc - Set ETxBf Codebook
	usage: iwpriv ra0 set ETxBfCodebook=0 to 3
*/
BOOLEAN Set_ETxBfCodebook_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	TX_TXBF_CFG_0_STRUC regValue;
	ULONG t = simple_strtol(arg, 0, 10);
	if (t > 3) {
		DBGPRINT(RT_DEBUG_ERROR, ("Set_ETxBfCodebook_Proc: value > 3!\n"));
		return FALSE;
	}

	RTMP_IO_READ32(pAd, TX_TXBF_CFG_0, &regValue.word);
	regValue.field.EtxbfFbkCode = t;
	RTMP_IO_WRITE32(pAd, TX_TXBF_CFG_0, regValue.word);
	return TRUE;
}


/*
	Set_ETxBfCoefficient_Proc - Set ETxBf Coefficient
	usage: iwpriv ra0 set ETxBfCoefficient=0 to 3
*/
BOOLEAN Set_ETxBfCoefficient_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	TX_TXBF_CFG_0_STRUC regValue;
	ULONG t = simple_strtol(arg, 0, 10);
	if (t > 3) {
		DBGPRINT(RT_DEBUG_ERROR, ("Set_ETxBfCoefficient_Proc: value > 3!\n"));
		return FALSE;
	}

	RTMP_IO_READ32(pAd, TX_TXBF_CFG_0, &regValue.word);
	regValue.field.EtxbfFbkCoef = t;
	RTMP_IO_WRITE32(pAd, TX_TXBF_CFG_0, regValue.word);
	return TRUE;
}


/*
	Set_ETxBfGrouping_Proc - Set ETxBf Grouping
		usage: iwpriv ra0 set ETxBfGrouping=0 to 2
*/
BOOLEAN Set_ETxBfGrouping_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	TX_TXBF_CFG_0_STRUC regValue;
	ULONG t = simple_strtol(arg, 0, 10);
	if (t > 2) {
		DBGPRINT(RT_DEBUG_ERROR, ("Set_ETxBfGrouping_Proc: value > 2!\n"));
		return FALSE;
	}

	RTMP_IO_READ32(pAd, TX_TXBF_CFG_0, &regValue.word);
	regValue.field.EtxbfFbkNg = t;
	RTMP_IO_WRITE32(pAd, TX_TXBF_CFG_0, regValue.word);
	return TRUE;
}


/*
	Set_ETxBfNoncompress_Proc - Set ETxBf Noncompress option
		usage: iwpriv ra0 set ETxBfNoncompress=0 or 1
*/
BOOLEAN Set_ETxBfNoncompress_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG t = simple_strtol(arg, 0, 10);
	if (t > 1) {
		DBGPRINT(RT_DEBUG_ERROR, ("Set_ETxBfNoncompress_Proc: value > 1!\n"));
		return FALSE;
	}

	pAd->CommonCfg.ETxBfNoncompress = t;
	return TRUE;
}


/*
	Set_ETxBfIncapable_Proc - Set ETxBf Incapable option
		usage: iwpriv ra0 set ETxBfIncapable=0 or 1
*/
BOOLEAN Set_ETxBfIncapable_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG t = simple_strtol(arg, 0, 10);
	if (t > 1)
		return FALSE;

	pAd->CommonCfg.ETxBfIncapable = t;
	setETxBFCap(pAd, &pAd->CommonCfg.HtCapability.TxBFCap);

	return TRUE;
}


/*
	Set_ITxBfDivCal_Proc - Calculate ITxBf Divider Calibration parameters
	usage: iwpriv ra0 set ITxBfDivCal=dd
			0=>display calibration parameters
			1=>update EEPROM values
			2=>update BBP R176
			10=>display calibration parameters and dump capture data
			11=>Skip divider calibration, just capture and dump capture data
*/
BOOLEAN Set_ITxBfDivCal_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	int calFunction = simple_strtol(arg, 0, 10);

	return pAd->chipOps.fITxBfDividerCalibration(pAd, calFunction, 0, NULL);
}


/*
	Set_ITxBfLNACal_Proc - Calculate ITxBf LNA Calibration parameters
	usage: iwpriv ra0 set ITxBfLnaCal=dd
			0=>display calibration parameters
			1=>update EEPROM values
			2=>update BBP R174
			10=>display calibration parameters and dump capture data
*/
BOOLEAN Set_ITxBfLnaCal_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UCHAR channel = pAd->CommonCfg.Channel;
	int calFunction;

#ifdef RALINK_ATE
	if (ATE_ON(pAd))
		channel = pAd->ate.Channel;
#endif
	calFunction = simple_strtol(arg, 0, 10);
	return pAd->chipOps.fITxBfLNACalibration(pAd, calFunction, 0, channel<=14);
}

#ifdef MT76x2
BOOLEAN mt76x2_Set_ITxBfCal_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	int calFunction = simple_strtol(arg, 0, 10);
	int calParams[2];
	int ret;
	UINT value32;
	ITXBF_PHASE_PARAMS phaseParams;
	UCHAR divPhase[2] = {0}, phaseValues[2] = {0};
	UCHAR channel = pAd->CommonCfg.Channel;

#ifdef RALINK_ATE
	if (ATE_ON(pAd))
		channel = pAd->ate.Channel;
#endif /* RALINK_ATE */

	ret = iCalcCalibration(pAd, calParams, 0);
	if (ret < 0) {
		if (ret == -3)
			DBGPRINT(RT_DEBUG_OFF, ("Set_ITxBfCal_Proc: kmalloc failed\n"));
		else if (ret == -2)
			DBGPRINT(RT_DEBUG_OFF, ("Set_ITxBfCal_Proc: MAC Address mismatch\n"));
		else
			DBGPRINT(RT_DEBUG_OFF, ("Set_ITxBfCal_Proc: Invalid profiles\n"));

		return FALSE;
	}

	/* Update BBP R176 and EEPROM for Ant 0 and 2 */
	switch (calFunction) {
	case 0:
		/* Remove Divider phase */
		pAd->chipOps.fITxBfDividerCalibration(pAd, 10, 0, divPhase);
		phaseValues[0] = calParams[0] - divPhase[0];
		DBGPRINT(RT_DEBUG_OFF, ("ITxBfCal Result = [0x%02x]\n",
				phaseValues[0]));
#ifdef RALINK_ATE
		pAd->ate.calParams[0] = (UCHAR)phaseValues[0];

		/* Double check */
		DBGPRINT((calFunction==0? RT_DEBUG_OFF: RT_DEBUG_WARN),
				("ITxBfCal Result in ATE = [0x%02x]\n",
				pAd->ate.calParams[0]));
#endif /* RALINK_ATE */
		break;

	case 1:
		/* Display result */
		DBGPRINT(RT_DEBUG_OFF, ("ITxBfCal Result = [0x%02x]\n", calParams[0]));
#ifdef RALINK_ATE
		pAd->ate.calParams[0] = (UCHAR)calParams[0];

		/* Double check */
		DBGPRINT((calFunction==0? RT_DEBUG_OFF: RT_DEBUG_WARN),
				("ITxBfCal Result in ATE = [0x%02x]\n",
				pAd->ate.calParams[0]));
#endif /* RALINK_ATE */

		//mt_rf_write(pAd, RF_Path0,	   RFDIGI_TRX4, 0x28585); // Auto LNA gain
		//mt_rf_write(pAd, RF_Path1,	   RFDIGI_TRX4, 0x28585); // Auto LNA gain
		//RTMP_IO_WRITE32(pAd,CORE_R4,   0x00000001); // core soft reset enable
		//RTMP_IO_WRITE32(pAd,CORE_R4,   0x00000000); // core soft reset disable

		/* Select Ant 0 */
		RTMP_IO_WRITE32(pAd, CAL_R0, 0);

		/* Update TXBE_R13 */
		//phaseValues[0] = 0x64;
		RTMP_IO_WRITE32(pAd, TXBE_R13, calParams[0]);

		/* Remove Divider phase */
		pAd->chipOps.fITxBfDividerCalibration(pAd, 10, 0, divPhase);
		phaseValues[0] = calParams[0] - divPhase[0];
		DBGPRINT(RT_DEBUG_OFF, (
				"%s : \n"
				"Phase compensated value = 0x%x\n"
				"Residual phase          = 0x%x\n",
				__FUNCTION__, calParams[0], phaseValues[0]));

		/* Enable TX Phase Compensation */
		RTMP_IO_READ32(pAd, TXBE_R12, &value32);
		RTMP_IO_WRITE32(pAd, TXBE_R12, value32 | 0x08);

		/* Enable Rx Phase compensation */
		//pAd->chipOps.fITxBfLNAPhaseCompensate(pAd);

		/* Update EEPROM */
		ITxBFGetEEPROM(pAd, &phaseParams, 0, 0);

		/* Only allow calibration on specific channels */
		if (channel == 1) {
			phaseParams.E1gBeg = phaseValues[0];
		} else if (channel == 14) {
			phaseParams.E1gEnd = phaseValues[0];
		} else if (channel == 36) {
			phaseParams.E1aLowBeg = phaseValues[0];
		} else if (channel == 64) {
			phaseParams.E1aLowEnd = phaseValues[0];
		} else if (channel == 100) {
			phaseParams.E1aMidBeg = phaseValues[0];
		} else if (channel == 120) {
			phaseParams.E1aMidMid = phaseValues[0];
		} else if (channel == 140) {
			phaseParams.E1aMidEnd = phaseValues[0];
		} else if (channel == 149) {
			phaseParams.E1aHighBeg = phaseValues[0];
		} else if (channel == 173) {
			phaseParams.E1aHighEnd = phaseValues[0];
		} else {
			DBGPRINT(RT_DEBUG_OFF,
					("Invalid channel: %d\nMust calibrate channel 1, 14, 36, 64, 100, 128, 132 or 165",
					channel));
			return FALSE;
		}

		ITxBFSetEEPROM(pAd, &phaseParams, 0, 0);

		DBGPRINT(RT_DEBUG_WARN, 
				("Set_ITxBfCal_Proc: Calibration Parameters updated\n"));
		break;
	default:
		break;
	}

	return TRUE;
}
#endif // MT76x2


/*
	Set_ETxBfEnCond_Proc - enable/disable ETxBF
	usage: iwpriv ra0 set ETxBfEnCond=dd
		0=>disable, 1=>enable
	Note: After use this command, need to re-run apStartup()/LinkUp() operations to sync all status.
		  If ETxBfIncapable!=0 then we don't need to reassociate.
*/
BOOLEAN Set_ETxBfEnCond_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UCHAR i, enableETxBf;
	MAC_TABLE_ENTRY *pEntry;
#ifndef MT76x2
	UINT8 byteValue;
#endif
	enableETxBf = simple_strtol(arg, 0, 10);
	if (enableETxBf > 1)
		return FALSE;

	pAd->CommonCfg.ETxBfEnCond = enableETxBf && (pAd->Antenna.field.TxPath > 1);
	pAd->CommonCfg.RegTransmitSetting.field.TxBF = enableETxBf==0? 0: 1;

	setETxBFCap(pAd, &pAd->CommonCfg.HtCapability.TxBFCap);
#ifdef VHT_TXBF_SUPPORT
	setVHTETxBFCap(pAd, &pAd->CommonCfg.vht_cap_ie.vht_cap);
#endif
	rtmp_asic_set_bf(pAd);

	if (enableETxBf) {
		RTMP_IO_WRITE32(pAd,PFMU_R54, 0x150); // Solve the MCS8 and MCS9 TP degradation when PN on
	} else {
		RTMP_IO_WRITE32(pAd,PFMU_R54, 0x100);
	}

	for (i = 0; i < MAX_LEN_OF_MAC_TABLE; i++) {
		pEntry = &pAd->MacTab.Content[i];
		if (!IS_ENTRY_NONE(pEntry)) {
#ifdef VHT_TXBF_SUPPORT
			if (WMODE_CAP_AC(pAd->CommonCfg.PhyMode) &&
				(pAd->CommonCfg.Channel > 14))
				pEntry->eTxBfEnCond = clientSupportsVHTETxBF(pAd, &pEntry->vht_cap_ie.vht_cap) ? enableETxBf: 0;
			else
#endif
				pEntry->eTxBfEnCond = clientSupportsETxBF(pAd, &pEntry->HTCapability.TxBFCap)? enableETxBf: 0;
			pEntry->bfState = READY_FOR_SNDG0;
			pEntry->HTPhyMode.field.eTxBF = pEntry->eTxBfEnCond;
			pEntry->phyETxBf = pEntry->eTxBfEnCond;
#ifdef MCS_LUT_SUPPORT
			asic_mcs_lut_update(pAd, pEntry);
#endif /* MCS_LUT_SUPPORT */
		}

		DBGPRINT(RT_DEBUG_TRACE, ("Set ETxBfEn=%d, Final ETxBF status =%d!\n",
				enableETxBf , pEntry->eTxBfEnCond));
	}

#ifndef MT76x2
	if (pAd->CommonCfg.RegTransmitSetting.field.ITxBfEn || enableETxBf) {
		RT30xxReadRFRegister(pAd, RF_R39, &byteValue);
		byteValue |= 0x40;
		RT30xxWriteRFRegister(pAd, RF_R39, byteValue);

		RT30xxReadRFRegister(pAd, RF_R49, &byteValue);
		byteValue |= 0x20;
		RT30xxWriteRFRegister(pAd, RF_R49, byteValue);
	} else {
		/* depends on Gary Tsao's comments. we shall disable it */
		if (pAd->CommonCfg.RegTransmitSetting.field.ITxBfEn == 0) {
			RT30xxReadRFRegister(pAd, RF_R39, &byteValue);
			byteValue &= (~0x40);
			RT30xxWriteRFRegister(pAd, RF_R39, byteValue);

			RT30xxReadRFRegister(pAd, RF_R49, &byteValue);
			byteValue &= (~0x20);
			RT30xxWriteRFRegister(pAd, RF_R49, byteValue);
		}
	}
#endif

	return TRUE;
}

/*
	Set_StaETxBfEnCond_Proc - enable/disable ETxBF
	usage: iwpriv ra0 set StaETxBfEnCond=dd
		0=>disable, 1=>enable
	Note: After use this command, need to re-run StaStartup()/LinkUp() operations to sync all status.
		  If ETxBfIncapable!=0 then we don't need to reassociate.
*/
BOOLEAN Set_StaETxBfEnCond_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UCHAR enableETxBf;
	MAC_TABLE_ENTRY *pEntry;

	enableETxBf = simple_strtol(arg, 0, 10);
	if (enableETxBf > 1)
		return FALSE;

	pAd->CommonCfg.ETxBfEnCond = enableETxBf && (pAd->Antenna.field.TxPath > 1);
	pAd->CommonCfg.RegTransmitSetting.field.TxBF = enableETxBf==0? 0: 1;

	setETxBFCap(pAd, &pAd->CommonCfg.HtCapability.TxBFCap);
#ifdef VHT_TXBF_SUPPORT
	setVHTETxBFCap(pAd, &pAd->CommonCfg.vht_cap_ie.vht_cap);
#endif
	rtmp_asic_set_bf(pAd);

	if (enableETxBf) {
		RTMP_IO_WRITE32(pAd,PFMU_R54, 0x150); // Solve the MCS8 and MCS9 TP degradation when PN on
	} else {
		RTMP_IO_WRITE32(pAd,PFMU_R54, 0x100);
	}

#ifdef VHT_TXBF_SUPPORT
	DBGPRINT(RT_DEBUG_TRACE, ("802.11 mode =%d, Channel =%d!\n",
			pAd->CommonCfg.PhyMode , pAd->CommonCfg.Channel));

	pEntry = &pAd->MacTab.Content[BSSID_WCID];
	if (!IS_ENTRY_NONE(pEntry)) {
		if (WMODE_CAP_AC(pAd->CommonCfg.PhyMode) &&
			(pAd->CommonCfg.Channel > 14)) {
			pEntry->eTxBfEnCond = clientSupportsVHTETxBF(pAd, &pEntry->vht_cap_ie.vht_cap) ? enableETxBf: 0;
			DBGPRINT(RT_DEBUG_TRACE, ("VHT mode!\n"));
			DBGPRINT(RT_DEBUG_TRACE, ("AP Bfee Cap =%d, AP Bfer Cap =%d!\n",
					pEntry->vht_cap_ie.vht_cap.bfee_cap_su,
					pEntry->vht_cap_ie.vht_cap.bfer_cap_su));
		} else {
			pEntry->eTxBfEnCond = clientSupportsETxBF(pAd, &pEntry->HTCapability.TxBFCap)? enableETxBf: 0;
			DBGPRINT(RT_DEBUG_TRACE, ("HT mode!\n"));
		}
	}
#else
	pEntry->eTxBfEnCond = clientSupportsETxBF(pAd, &pEntry->HTCapability.TxBFCap)? enableETxBf: 0;
	DBGPRINT(RT_DEBUG_TRACE, ("HT mode!\n"));
#endif
	pEntry->bfState = READY_FOR_SNDG0;
	pEntry->HTPhyMode.field.eTxBF = pEntry->eTxBfEnCond;
	pEntry->phyETxBf = pEntry->eTxBfEnCond;
#ifdef MCS_LUT_SUPPORT
	asic_mcs_lut_update(pAd, pEntry);
#endif
	DBGPRINT(RT_DEBUG_TRACE, ("Set ETxBfEn=%d, Final ETxBF status =%d!\n",
			enableETxBf , pEntry->eTxBfEnCond));
	return TRUE;
}


BOOLEAN Set_NoSndgCntThrd_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UCHAR i;

	for (i = 0; i < MAX_LEN_OF_MAC_TABLE; i++) {
		pAd->MacTab.Content[i].noSndgCntThrd = simple_strtol(arg, 0, 10);
	}
	return TRUE;
}

BOOLEAN Set_NdpSndgStreams_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UCHAR i;
	for (i = 0; i < MAX_LEN_OF_MAC_TABLE; i++) {
		pAd->MacTab.Content[i].ndpSndgStreams = simple_strtol(arg, 0, 10);
	}
	return TRUE;
}


BOOLEAN Set_Trigger_Sounding_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UCHAR macAddr[MAC_ADDR_LEN];
	CHAR *value;
	int i;
	MAC_TABLE_ENTRY *pEntry = NULL;

	/* Mac address acceptable format 01:02:03:04:05:06 length 17 */
	if (strlen(arg) != 17)
		return FALSE;

	for (i = 0, value = rstrtok(arg,":"); value; value = rstrtok(NULL,":")) {
		if ((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) )
			return FALSE;  /*Invalid*/

		AtoH(value, &macAddr[i++], 1);
	}

	/*DBGPRINT(RT_DEBUG_TRACE, ("TriggerSounding=%02x:%02x:%02x:%02x:%02x:%02x\n",*/
	/*		macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5], macAddr[6]) );*/
	pEntry = MacTableLookup(pAd, macAddr);
	if (pEntry == NULL)
		return FALSE;

	Trigger_Sounding_Packet(pAd, SNDG_TYPE_SOUNDING, 0, pEntry->sndgMcs, pEntry);

	return TRUE;
}

/*
	Set_ITxBfEn_Proc - enable/disable ITxBF
	usage: iwpriv ra0 set ITxBfEn=dd
	0=>disable, 1=>enable
*/
#ifndef MT76x2
BOOLEAN Set_ITxBfEn_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UCHAR i;
	UCHAR enableITxBF;
	BOOLEAN bCalibrated;
	UINT8 byteValue;

	enableITxBF = simple_strtol(arg, 0, 10);
	if (enableITxBF > 1)
		return FALSE;

	bCalibrated = rtmp_chk_itxbf_calibration(pAd);
	DBGPRINT(RT_DEBUG_TRACE,
			("Set ITxBfEn=%d, calibration of ITxBF=%d, so enableITxBF=%d!\n",
			enableITxBF , bCalibrated, (enableITxBF & bCalibrated)));

	enableITxBF &= bCalibrated;
	pAd->CommonCfg.RegTransmitSetting.field.ITxBfEn = enableITxBF && (pAd->Antenna.field.TxPath > 1);
	rtmp_asic_set_bf(pAd);

	for (i = 0; i < MAX_LEN_OF_MAC_TABLE; i++) {
		MAC_TABLE_ENTRY *pMacEntry = &pAd->MacTab.Content[i];
		if ((!IS_ENTRY_NONE(pMacEntry)) && (pAd->Antenna.field.TxPath> 1))
			pMacEntry->iTxBfEn = enableITxBF;
	}

	if (enableITxBF || pAd->CommonCfg.ETxBfEnCond) {
		RT30xxReadRFRegister(pAd, RF_R39, &byteValue);
		byteValue |= 0x40;
		RT30xxWriteRFRegister(pAd, RF_R39, byteValue);

		RT30xxReadRFRegister(pAd, RF_R49, &byteValue);
		byteValue |= 0x20;
		RT30xxWriteRFRegister(pAd, RF_R49, byteValue);
	}

	/* If enabling ITxBF then set LNA compensation, do a Divider Calibration and update BBP registers */
	if (enableITxBF) {
		pAd->chipOps.fITxBfLNAPhaseCompensate(pAd);
		pAd->chipOps.fITxBfDividerCalibration(pAd, 2, 0, NULL);
	} else {
		/* depends on Gary Tsao's comments. */
		if (pAd->CommonCfg.ETxBfEnCond == 0) {
			RT30xxReadRFRegister(pAd, RF_R39, &byteValue);
			byteValue &= (~0x40);
			RT30xxWriteRFRegister(pAd, RF_R39, byteValue);

			RT30xxReadRFRegister(pAd, RF_R49, &byteValue);
			byteValue &= (~0x20);
			RT30xxWriteRFRegister(pAd, RF_R49, byteValue);
		}

		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R173, 0);
		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R27, &byteValue);
		byteValue &= ~0x60;

		for ( i = 0; i < 3; i++) {
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R27, (byteValue & (i << 5)));
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R174, 0);
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R176, 0);
		}
	}
	return TRUE;
}

#else
BOOLEAN Set_ITxBfEn_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UINT32  i;
	UCHAR enableITxBF;
	BOOLEAN bCalibrated;
	UINT value32;

	enableITxBF = simple_strtol(arg, 0, 10);
	if (enableITxBF > 1)
		return FALSE;

	bCalibrated = rtmp_chk_itxbf_calibration(pAd);
	DBGPRINT(RT_DEBUG_TRACE,
			("Set ITxBfEn=%d, calibration of ITxBF=%d, so enableITxBF=%d!\n",
			enableITxBF , bCalibrated, (enableITxBF & bCalibrated)));

	enableITxBF &= bCalibrated;
	pAd->CommonCfg.RegTransmitSetting.field.ITxBfEn = enableITxBF && (pAd->Antenna.field.TxPath > 1);
	rtmp_asic_set_bf(pAd);

	if (enableITxBF) {
		RTMP_IO_WRITE32(pAd,PFMU_R54, 0x150); // Solve the MCS8 and MCS9 TP degradation when PN on
	} else {
		RTMP_IO_WRITE32(pAd,PFMU_R54, 0x100);
	}

	/* Check how many clients could be applied ITxBf */
	for (i = 0; i < MAX_LEN_OF_MAC_TABLE; i++) {
		MAC_TABLE_ENTRY *pMacEntry = &pAd->MacTab.Content[i];
		if ((!IS_ENTRY_NONE(pMacEntry)) && (pAd->Antenna.field.TxPath> 1)) {
			pMacEntry->iTxBfEn = enableITxBF;
			pMacEntry->HTPhyMode.field.iTxBF = enableITxBF;

#ifdef MCS_LUT_SUPPORT
			asic_mcs_lut_update(pAd, pMacEntry);
#endif /* MCS_LUT_SUPPORT */
		}
	}

	/* If enabling ITxBF then set LNA and TX phase compensation */
	if (enableITxBF) {
		/* Enable TX/Rx Phase Compensation */
		//mt_rf_write(pAd, RF_Path0,	   RFDIGI_TRX4, 0x28585);
		//mt_rf_write(pAd, RF_Path1,	   RFDIGI_TRX4, 0x28585);
		pAd->chipOps.fITxBfLNAPhaseCompensate(pAd);

		/* Enable TX Phase Compensation */
		RTMP_IO_READ32(pAd, TXBE_R12, &value32);
		RTMP_IO_WRITE32(pAd, TXBE_R12, value32 | 0x08);

		RtmpOsMsDelay(10); // waiting 10ms
	} else {
		/* Disable Tx/Rx Phase Compensation */
		RTMP_IO_READ32(pAd, TXBE_R12, &value32);
		RTMP_IO_WRITE32(pAd, TXBE_R12, value32 & (~0x0FF));
	}

	return TRUE;
}
#endif
#endif /* TXBF_SUPPORT */


#ifdef VHT_TXBF_SUPPORT
/*
	The VhtNDPA sounding inupt string format should be xx:xx:xx:xx:xx:xx-d,
		=>The six 2 digit hex-decimal number previous are the Mac address,
		=>The seventh decimal number is the MCS value.
*/
BOOLEAN Set_VhtNDPA_Sounding_Proc(RTMP_ADAPTER *pAd, PSTRING arg)
{
	UCHAR mac[6];
	UINT mcs;
	PSTRING token;
	STRING sepValue[] = ":", DASH = '-';
	int i;
	MAC_TABLE_ENTRY *pEntry = NULL;

	DBGPRINT(RT_DEBUG_TRACE,("\n%s\n", arg));

	/*Mac address acceptable format 01:02:03:04:05:06 length 17 plus the "-" and MCS value in decimal format.*/
	if (strlen(arg) < 19)
		return FALSE;

	token = strchr(arg, DASH);
	if ((token != NULL) && (strlen(token) > 1)) {
		mcs = (UINT)simple_strtol((token+1), 0, 10);

		*token = '\0';
		for (i = 0, token = rstrtok(arg, &sepValue[0]); token; token = rstrtok(NULL, &sepValue[0]), i++) {
			if ((strlen(token) != 2) || (!isxdigit(*token)) || (!isxdigit(*(token+1))))
				return FALSE;
			AtoH(token, (&mac[i]), 1);
		}
		if (i != 6)
			return FALSE;

		DBGPRINT(RT_DEBUG_OFF, ("\n%02x:%02x:%02x:%02x:%02x:%02x-%02x\n",
				PRINT_MAC(mac), mcs));

		pEntry = MacTableLookup(pAd, (PUCHAR) mac);
		if (pEntry) {
#ifdef SOFT_SOUNDING
			pEntry->snd_rate.field.MODE = MODE_VHT;
			pEntry->snd_rate.field.BW = (mcs / 100) > BW_80 ? BW_80 : (mcs / 100);
			mcs %= 100;
			pEntry->snd_rate.field.MCS = ((mcs / 10) << 4 | (mcs % 10));

			DBGPRINT(RT_DEBUG_OFF,
					("%s():Trigger VHT NDPA Sounding=%02x:%02x:%02x:%02x:%02x:%02x, snding rate=VHT-%sHz, %dSS-MCS%d\n",
					__FUNCTION__, PRINT_MAC(mac),
					get_bw_str(pEntry->snd_rate.field.BW),
					(pEntry->snd_rate.field.MCS >> 4) + 1,
					pEntry->snd_rate.field.MCS & 0xf));
#endif
			trigger_vht_ndpa(pAd, pEntry);
		}

		return TRUE;
	}

	return FALSE;
}
#endif /* VHT_TXBF_SUPPORT */

#ifdef MT76x2
#ifdef TXBF_SUPPORT
static USHORT PFMU_TimeOut;
static UCHAR MatrixForm[5];
static UCHAR StsSnr[2];
static UCHAR TxScale[4];
static UCHAR macAddr[MAC_ADDR_LEN];
static UCHAR FlgStatus[4];
static USHORT CMDInIdx = 0, dCMDInIdx = 0;
static UCHAR psi21;
static UCHAR phill;

BOOLEAN Set_TxBfProfileTag_Help(PRTMP_ADAPTER pAd, PSTRING arg)
{
	printk("========================================================================================================================\n"
		   "TxBfProfile Tag setting example :\n"
		   "iwpriv ra0 set TxBfProfileTagMatrix=ng:cw:nrow:nol:LM\n"
		   "iwpriv ra0 set TxBfProfileTagMac=xx:xx:xx:xx:xx:xx\n"
		   "iwpriv ra0 set TxBfProfileTagTxScale=Tx1_scale_2ss[7:0]:Tx0_scale_2ss[7:0]:Tx1_scale_1ss[7:0]:Tx0_scale_1ss[7:0]\n"
		   "iwpriv ra0 set TxBfProfileTagSNR=STS1_SNR[7:0]:STS0_SNR[7:0]\n"
		   "iwpriv ra0 set TxBfProfileTagTimeOut=timeout[15:0]\n"
		   "iwpriv ra0 set TxBfProfileTagFlg=LD:EO:IO:I/E\n"
		   "\n\n"
		   "iwpriv ra0 set ATETXBF=xx (0:no BF; 1:ETxBF, 2:ITxBF, 3:Enable TXBF support, 4:Disable TXBF support)\n"
		   "iwpriv ra0 set TxBfProfileTagValid=0/1:profile index\n"
		   "\n"
		   "Read TxBf profile Tag :\n"
		   "iwpriv ra0 set TxBfProfileTagRead=xx (select a profile only)\n"
		   "\n"
		   "Write TxBf profile Tag :\n"
		   "iwpriv ra0 set TxBfProfileTagWrite=xx (select a profile only)\n"
		   "\n\n"
		   "Read TxBf profile Data	:\n"
		   "iwpriv ra0 set TxBfProfileDataRead=xx:xx (select a profile and subcarrier to load seting)\n"
		   "iwpriv ra0 set TxBfProfileDataReadAll=TxBfType:subcarrier:BW  (TxBfType : 0x00 (IxBf), 0x01 (ETxBf)\n"
		   "                                                              (BW       : 0x00 (20M) , 0x01 (40M), 0x02 (80M)\n"
		   "\n"
		   "Write TxBf profile Data :\n"
		   "iwpriv ra0 set TxBfProfileDataWrite=profile:subcarrier:psi21:pill\n"
		   "iwpriv ra0 set TxBfProfileDataWriteAll=TxBfType:subcarrier:BW  (TxBfType : 0x00 (IxBf), 0x01 (ETxBf)\n"
		   "                                                               (BW       : 0x00 (20M) , 0x01 (40M), 0x02 (80M)\n"
		   "========================================================================================================================\n");

	CMDInIdx = 0; // Clear index
	return TRUE;
}


BOOLEAN Set_TxBfProfileTagValid(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UCHAR profileIdx, validFlg;
	UCHAR Input[2];
	CHAR *value;
	UINT value32, readValue32[5];
	int i;

	/* Profile Select : Subcarrier Select */
	if (strlen(arg) != 5)
		return FALSE;

	for (i = 0, value = rstrtok(arg,":"); value; value = rstrtok(NULL,":")) {
		if ((!isxdigit(*value)) || (!isxdigit(*(value+1))) )
			return FALSE;  /*Invalid*/

		AtoH(value, &Input[i++], 1);
	}

	validFlg    = Input[0];
	profileIdx = Input[1];

	// Read PFMU_R10 (0x2f28) first
	RTMP_IO_READ32(pAd, PFMU_R10, &value32);
	value32 &= (~0x3C00);
	// Wite PFMU_R10 to trigger read command
	RTMP_IO_WRITE32(pAd, PFMU_R10, ((profileIdx << 10)|value32));
	// Read PFMU_R11 ~ R15
	RTMP_IO_READ32(pAd, PFMU_R11, &readValue32[0]);
	RTMP_IO_READ32(pAd, PFMU_R12, &readValue32[1]);
	RTMP_IO_READ32(pAd, PFMU_R13, &readValue32[2]);
	RTMP_IO_READ32(pAd, PFMU_R14, &readValue32[3]);
	RTMP_IO_READ32(pAd, PFMU_R15, &readValue32[4]);

	/*
		Update the valid bit
	*/
	readValue32[0] &= ~(1 << 7);
	readValue32[0] |= validFlg << 7;

	// Read PFMU_R10 (0x2f28) first
	RTMP_IO_READ32(pAd, PFMU_R10, &value32);
	value32 &= (~0x3C00);
	// Wite PFMU_R10 to trigger read command
	RTMP_IO_WRITE32(pAd, PFMU_R10, ((profileIdx << 10)|value32));
	// Write PFMU_R11 ~ R15
	RTMP_IO_WRITE32(pAd, PFMU_R15, readValue32[4]);
	RTMP_IO_WRITE32(pAd, PFMU_R14, readValue32[3]);
	RTMP_IO_WRITE32(pAd, PFMU_R13, readValue32[2]);
	RTMP_IO_WRITE32(pAd, PFMU_R12, readValue32[1]);
	RTMP_IO_WRITE32(pAd, PFMU_R11, readValue32[0]);

	return TRUE;
}


BOOLEAN Set_TxBfProfileTagRead(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UCHAR profileIdx;
	UINT value32, readValue32[5];
	UCHAR validFlg;

	profileIdx = simple_strtol(arg, 0, 10);

	// Read PFMU_R10 (0x2f28) first
	RTMP_IO_READ32(pAd, PFMU_R10, &value32);
	value32 &= (~0x3C00);
	// Wite PFMU_R10 to trigger read command
	RTMP_IO_WRITE32(pAd, PFMU_R10, ((profileIdx << 10)|value32));
	// Read PFMU_R19 ~ R23
	RTMP_IO_READ32(pAd, PFMU_R11, &readValue32[0]);
	RTMP_IO_READ32(pAd, PFMU_R12, &readValue32[1]);
	RTMP_IO_READ32(pAd, PFMU_R13, &readValue32[2]);
	RTMP_IO_READ32(pAd, PFMU_R14, &readValue32[3]);
	RTMP_IO_READ32(pAd, PFMU_R15, &readValue32[4]);

	/*
			Bit 63:62       ng[1:0]
			    61:60       cw[1:0]
			    59:58       nrow[1:0]
			    57:56       ncol[1:0]
			     1:0        LM[1:0]
	*/
	MatrixForm[0] = (UCHAR)((readValue32[1] >> 30) & 0x3);
	MatrixForm[1] = (UCHAR)((readValue32[1] >> 28) & 0x3);
	MatrixForm[2] = (UCHAR)((readValue32[1] >> 26) & 0x3);
	MatrixForm[3] = (UCHAR)((readValue32[1] >> 24) & 0x3);
	MatrixForm[4] = (UCHAR)(readValue32[0] & 0x3);

	/*
		Bit 55 : 8       Mac Adress [47:0]
	*/
	macAddr[0] = (UCHAR)((readValue32[1] >> 16) & 0x0FF);
	macAddr[1] = (UCHAR)((readValue32[1] >> 8) & 0x0FF);
	macAddr[2] = (UCHAR)((readValue32[1]) & 0x0FF);
	macAddr[3] = (UCHAR)((readValue32[0] >> 24) & 0x0FF);
	macAddr[4] = (UCHAR)((readValue32[0] >> 16) & 0x0FF);
	macAddr[5] = (UCHAR)((readValue32[0] >> 8)  & 0x0FF);

	/*
		Bit 63 : 56       Tx1_scale_2ss[7:0]
			 55 : 48       Tx0_scale_2ss[7:0]
			 47 : 40       Tx1_scale_1ss[7:0]
			 36 : 32       Tx0_scale_2ss[7:0]
	*/
	TxScale[0] = (UCHAR)((readValue32[3] >> 24) & 0x0FF);
	TxScale[1] = (UCHAR)((readValue32[3] >> 16) & 0x0FF);
	TxScale[2] = (UCHAR)((readValue32[3] >> 8) & 0x0FF);
	TxScale[3] = (UCHAR)((readValue32[3] >> 0) & 0x0FF);

	/*
		Bit 31 : 24       STS1_SNR[7:0]
			 23 : 16       STS0_SNR[7:0]
	*/
	StsSnr[0] = (UCHAR)((readValue32[2] >> 24) & 0x0FF);
	StsSnr[1] = (UCHAR)((readValue32[2] >> 16) & 0x0FF);

	/*
		Bit 15 : 0       PFMU_TimeOut[15:0]
	*/
	PFMU_TimeOut = (USHORT)(readValue32[2] & 0x0FFFF);

	/*
		LD, EO, IO, I/E
	*/
	FlgStatus[0] = (UCHAR)((readValue32[4] >> 8) & 0x1);
	FlgStatus[1] = (UCHAR)((readValue32[4] >> 7) & 0x1);
	FlgStatus[2] = (UCHAR)((readValue32[4] >> 6) & 0x1);
	FlgStatus[3] = (UCHAR)((readValue32[4] >> 5) & 0x1);

	/*
		Check profile valid staus
	*/
	validFlg = (readValue32[0] >> 7) & 0x1;

	printk("============================= TxBf profile Tage Info ==============================\n"
		   "Pofile valid status = %d\n"
		   "Profile index = %d\n\n"
		   "PFMU_11 = 0x%x, PFMU_R12=0x%x, PFMU_R13=0x%x, PFMU_R14=0x%x\n\n"
		   "ng[1:0]=%d, cw[1:0]=%d, nrow[1:0]=%d, ncol[1:0]=%d, LM[1:0]=%d\n\n"
		   "Destimation MAC Address=%x:%x:%x:%x:%x:%x\n\n"
		   "Tx1_scal_2ss=0x%x, Tx0_scale_2ss=0x%x, Tx1_scal_1ss=0x%x, Tx0_scale_1ss=0x%x\n\n"
		   "STS1_SNR=0x%x, STS0_SNR=0x%x\n"
		   "timeout[15:0]=0x%x\n"
		   "LD=%d, EO=%d, IO=%d, I/E=%d\n"
		   "===================================================================================\n",
		   validFlg,
		   profileIdx,
		   readValue32[0], readValue32[1], readValue32[2], readValue32[3],
		   MatrixForm[0], MatrixForm[1], MatrixForm[2], MatrixForm[3], MatrixForm[4],
		   macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5],
		   TxScale[0], TxScale[1], TxScale[2], TxScale[3],
		   StsSnr[0], StsSnr[1],
		   PFMU_TimeOut,
		   FlgStatus[0], FlgStatus[1], FlgStatus[2], FlgStatus[3]);

	return TRUE;
}


BOOLEAN Set_TxBfProfileTagWrite(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UCHAR profileIdx;
	UINT value32, readValue32[5];

	profileIdx = simple_strtol(arg, 0, 10);

	// Read PFMU_R10 (0x2f28) first
	RTMP_IO_READ32(pAd, PFMU_R10, &value32);
	value32 &= (~0x3C00);
	// Wite PFMU_R10 to trigger read command
	RTMP_IO_WRITE32(pAd, PFMU_R10, ((profileIdx << 10)|value32));
	// Read PFMU_R19 ~ R23
	RTMP_IO_READ32(pAd, PFMU_R11, &readValue32[0]);
	RTMP_IO_READ32(pAd, PFMU_R12, &readValue32[1]);
	RTMP_IO_READ32(pAd, PFMU_R13, &readValue32[2]);
	RTMP_IO_READ32(pAd, PFMU_R14, &readValue32[3]);
	RTMP_IO_READ32(pAd, PFMU_R15, &readValue32[4]);

	DBGPRINT(RT_DEBUG_TRACE,
			("============================= TxBf profile Tage Write ==============================\n"
			"Profile index = %d\n\n",
			profileIdx));

	/*
		Bit  63:62       ng[1:0]
		 61:60       cw[1:0]
		 59:58       nrow[1:0]
		 57:56       ncol[1:0]
		 1 :  0       LM[1:0]
	*/
	if (CMDInIdx & 1) {
		value32  = (LONG)MatrixForm[0] << 30;
		value32 |= (LONG)MatrixForm[1] << 28;
		value32 |= (LONG)MatrixForm[2] << 26;
		value32 |= (LONG)MatrixForm[3] << 24;
		readValue32[1] &= 0x00FFFFFF;
		readValue32[1] |= value32;

		readValue32[0] &= ~(0x3);
		readValue32[0] |= MatrixForm[4];

		DBGPRINT(RT_DEBUG_TRACE,
				("ng[1:0]=%d, cw[1:0]=%d, nrow[1:0]=%d, ncol[1:0]=%d, LM[1:0]=%d\n\n",
				MatrixForm[0], MatrixForm[1], MatrixForm[2],
				MatrixForm[3], MatrixForm[4]));
	}

	/*
		Bit 55 : 8       Mac Adress [47:0]
	*/
	if (CMDInIdx & 2) {
		value32  = (LONG)macAddr[0] << 16;
		value32 |= (LONG)macAddr[1] << 8;
		value32 |= (LONG)macAddr[2] << 0;
		readValue32[1] &= 0xFF000000;
		readValue32[1] |= value32;

		value32 = (LONG)macAddr[3] << 24;
		value32 |= (LONG)macAddr[4] << 16;
		value32 |= (LONG)macAddr[5] << 8;
		readValue32[0] &= 0x000000FF;
		readValue32[0] |= value32;

		DBGPRINT(RT_DEBUG_TRACE,
				("Destimation MAC Address=%x:%x:%x:%x:%x:%x\n\n",
				macAddr[0], macAddr[1], macAddr[2], macAddr[3],
				macAddr[4], macAddr[5]));
	}

	/*
	  Bit	63 : 56       Tx1_scale_2ss[7:0]
		55 : 48       Tx0_scale_2ss[7:0]
		47 : 40       Tx1_scale_1ss[7:0]
		36 : 32       Tx0_scale_2ss[7:0]
	*/
	if (CMDInIdx & 4) {
		value32  = (LONG)TxScale[0] << 24;
		value32 |= (LONG)TxScale[1] << 16;
		value32 |= (LONG)TxScale[2] << 8;
		value32 |= (LONG)TxScale[3] << 0;
		readValue32[3] = value32;

		DBGPRINT(RT_DEBUG_TRACE,
				("Tx1_scal_2ss=0x%x, Tx0_scale_2ss=0x%x, Tx1_scal_1ss=0x%x, Tx0_scale_1ss=0x%x\n\n",
				TxScale[0], TxScale[1], TxScale[2], TxScale[3]));
	}

	/*
	  Bit	31 : 24       STS1_SNR[7:0]
		23 : 16       STS0_SNR[7:0]
	*/
	if (CMDInIdx & 8) {
		value32  = (LONG)StsSnr[0] << 24;
		value32 |= (LONG)StsSnr[1] << 16;
		readValue32[2] &= 0x0000FFFF;
		readValue32[2] |= value32;

		DBGPRINT(RT_DEBUG_TRACE, ("STS1_SNR=0x%x, STS0_SNR=0x%x\n",
				StsSnr[0], StsSnr[1]));
	}
	/*
		Bit 15 : 0       timeout[15:0]
	*/
	if (CMDInIdx & 16) {
		readValue32[2] &= 0xFFFF0000;
		readValue32[2] |= (ULONG)PFMU_TimeOut;

		DBGPRINT(RT_DEBUG_TRACE, ("timeout[15:0]=0x%x\n", PFMU_TimeOut));
	}

	DBGPRINT(RT_DEBUG_TRACE, ("PFMU_11 = 0x%x, PFMU_R12=0x%x, PFMU_R13=0x%x, PFMU_R14=0x%x\n\n",
		readValue32[0], readValue32[1], readValue32[2], readValue32[3]));
	/*
		Bit 15 : 0	 timeout[15:0]
	*/
	if (CMDInIdx & 32) {
		readValue32[4] &= 0xFFFFFE00;
		readValue32[4] |= FlgStatus[0] << 8;
		readValue32[4] |= FlgStatus[1] << 7;
		readValue32[4] |= FlgStatus[2] << 6;
		readValue32[4] |= FlgStatus[3] << 5;

		DBGPRINT(RT_DEBUG_TRACE, ("LD=%d, EO=%d, IO=%d, I/E=%d\n"
			"===================================================================================\n",
			FlgStatus[0], FlgStatus[1], FlgStatus[2], FlgStatus[3]));
	}

	// Read PFMU_R10 (0x2f28) first
	RTMP_IO_READ32(pAd, PFMU_R10, &value32);
	value32 &= (~0x3C00);
	// Wite PFMU_R10 to trigger read command
	RTMP_IO_WRITE32(pAd, PFMU_R10, ((profileIdx << 10)|value32));
	// Write PFMU_R11 ~ R15
	RTMP_IO_WRITE32(pAd, PFMU_R15, readValue32[4]);
	RTMP_IO_WRITE32(pAd, PFMU_R14, readValue32[3]);
	RTMP_IO_WRITE32(pAd, PFMU_R13, readValue32[2]);
	RTMP_IO_WRITE32(pAd, PFMU_R12, readValue32[1]);
	RTMP_IO_WRITE32(pAd, PFMU_R11, readValue32[0]);

	CMDInIdx = 0; // clear tag indicator

	return TRUE;
}


BOOLEAN Set_TxBfProfileDataRead(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UCHAR profileIdx, subcarrierIdx;
	UCHAR Input[2];
	CHAR *value;
	UINT value32, readValue32[5];
	int i;

	printk("arg length=%zu\n", strlen(arg));

	/* Profile Select : Subcarrier Select */
	if (strlen(arg) != 5)
		return FALSE;

	for (i = 0, value = rstrtok(arg,":"); value; value = rstrtok(NULL,":")) {
		if ((!isxdigit(*value)) || (!isxdigit(*(value+1))) )
			return FALSE;  /*Invalid*/

		AtoH(value, &Input[i++], 1);
	}

	profileIdx    = Input[0];
	subcarrierIdx = Input[1];

	// Read PFMU_R10 (0x2f28) first
	RTMP_IO_READ32(pAd, PFMU_R10, &value32);
	// Wite PFMU_R10 to trigger read command
	RTMP_IO_WRITE32(pAd, PFMU_R10, ((profileIdx << 10)|subcarrierIdx));
	// Read PFMU_R19 ~ R23
	RTMP_IO_READ32(pAd, PFMU_R19, &readValue32[0]);
	RTMP_IO_READ32(pAd, PFMU_R20, &readValue32[1]);
	RTMP_IO_READ32(pAd, PFMU_R21, &readValue32[2]);
	RTMP_IO_READ32(pAd, PFMU_R22, &readValue32[3]);
	RTMP_IO_READ32(pAd, PFMU_R23, &readValue32[4]);

	psi21 = (readValue32[0] >> 16) & 0x00FF;
	phill = (readValue32[0] >> 0)  & 0x00FF;

	printk("============================= TxBf profile Data Info ==============================\n"
		   "Profile index = %d,    subcarrierIdx = %d\n\n"
		   "PFMU_19 = 0x%x, PFMU_R20=0x%x, PFMU_R21=0x%x, PFMU_R22=0x%x\n\n"
		   "psi21 = 0x%x\n\n"
		   "phill = 0x%x\n\n"
		   "===================================================================================\n",
		   profileIdx, subcarrierIdx,
		   readValue32[0], readValue32[1], readValue32[2], readValue32[3],
		   psi21, phill);

	return TRUE;
}


BOOLEAN Set_TxBfProfileDataWrite(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UCHAR profileIdx, subcarrierIdx;
	UCHAR Input[4];
	CHAR *value;
	UINT value32, readValue32[5];
	int i;

	/* Profile Select : Subcarrier Select */
	if (strlen(arg) != 11)
		return FALSE;

	for (i = 0, value = rstrtok(arg,":"); value; value = rstrtok(NULL,":")) {
		if ((!isxdigit(*value)) || (!isxdigit(*(value+1))) )
			return FALSE;  /*Invalid*/

		AtoH(value, &Input[i++], 1);
	}

	profileIdx  = Input[0];
	subcarrierIdx = Input[1];

	psi21 = Input[2];
	phill = Input[3];

	// Read PFMU_R10 (0x2f28) first
	RTMP_IO_READ32(pAd, PFMU_R10, &value32);
	// Wite PFMU_R10 to trigger read command
	RTMP_IO_WRITE32(pAd, PFMU_R10, ((profileIdx << 10)|subcarrierIdx));
	// Read PFMU_R19 ~ R23
	RTMP_IO_READ32(pAd, PFMU_R19, &readValue32[0]);
	RTMP_IO_READ32(pAd, PFMU_R20, &readValue32[1]);
	RTMP_IO_READ32(pAd, PFMU_R21, &readValue32[2]);
	RTMP_IO_READ32(pAd, PFMU_R22, &readValue32[3]);
	RTMP_IO_READ32(pAd, PFMU_R23, &readValue32[4]);

	readValue32[0] = (psi21 << 16) | phill;

	// Read PFMU_R10 (0x2f28) first
	RTMP_IO_READ32(pAd, PFMU_R10, &value32);
	// Wite PFMU_R10 to trigger read command
	RTMP_IO_WRITE32(pAd, PFMU_R10, ((profileIdx << 10)|subcarrierIdx));
	// Wite PFMU_R19 ~ R23
	RTMP_IO_WRITE32(pAd, PFMU_R23, readValue32[4]);
	RTMP_IO_WRITE32(pAd, PFMU_R22, readValue32[3]);
	RTMP_IO_WRITE32(pAd, PFMU_R21, readValue32[2]);
	RTMP_IO_WRITE32(pAd, PFMU_R20, readValue32[1]);
	RTMP_IO_WRITE32(pAd, PFMU_R19, readValue32[0]);

	dCMDInIdx = 0; // clear profile data write indicator

	return TRUE;
}


static SC_TABLE_ENTRY impSCTable[3] = { {224, 255, 1, 31}, {198, 254, 2, 58}, {134, 254, 2, 122} };
static SC_TABLE_ENTRY expSCTable[3] = { {224, 255, 1, 31}, {198, 254, 2, 58}, {134, 254, 2, 122} };

BOOLEAN Set_TxBfProfileDataWriteAll(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UCHAR profileIdx;
	USHORT scIndex;
	UCHAR TxBfType, BW, psi21_I, phill_I;
	CHAR *value;
	UINT value32, readValue32[5];
	SC_TABLE_ENTRY *pTab;
	UCHAR Input[3];
	int i;

	/* Profile Select : Subcarrier Select */
	if (strlen(arg) != 8)
		return FALSE;

	for (i = 0, value = rstrtok(arg,":"); value; value = rstrtok(NULL,":")) {
		if ((!isxdigit(*value)) || (!isxdigit(*(value+1))) )
			return FALSE;  /*Invalid*/

		AtoH(value, &Input[i++], 1);
	}

	TxBfType = Input[0];
	profileIdx = Input[1];
	BW = Input[2];

	psi21_I = 0x00;
	phill_I = 0xFF;

	if (TxBfType == 0) // ITxBf
		pTab = &impSCTable[BW];
	else
		pTab = &expSCTable[BW];

	/* Negative subcarriers */
	for (scIndex = pTab->lwb1; scIndex <= pTab->upb1; scIndex++) {
		// Read PFMU_R10 (0x2f28) first
		RTMP_IO_READ32(pAd, PFMU_R10, &value32);
		// Wite PFMU_R10 to trigger read command
		RTMP_IO_WRITE32(pAd, PFMU_R10, ((profileIdx << 10)|scIndex));
		// Read PFMU_R19 ~ R23
		RTMP_IO_READ32(pAd, PFMU_R19, &readValue32[0]);
		RTMP_IO_READ32(pAd, PFMU_R20, &readValue32[1]);
		RTMP_IO_READ32(pAd, PFMU_R21, &readValue32[2]);
		RTMP_IO_READ32(pAd, PFMU_R22, &readValue32[3]);
		RTMP_IO_READ32(pAd, PFMU_R23, &readValue32[4]);

		readValue32[0] = (psi21_I << 16) | phill_I;

		// Read PFMU_R10 (0x2f28) first
		RTMP_IO_READ32(pAd, PFMU_R10, &value32);
		// Wite PFMU_R10 to trigger read command
		RTMP_IO_WRITE32(pAd, PFMU_R10, ((profileIdx << 10)|scIndex));
		// Wite PFMU_R19 ~ R23
		RTMP_IO_WRITE32(pAd, PFMU_R23, readValue32[4]);
		RTMP_IO_WRITE32(pAd, PFMU_R22, readValue32[3]);
		RTMP_IO_WRITE32(pAd, PFMU_R21, readValue32[2]);
		RTMP_IO_WRITE32(pAd, PFMU_R20, readValue32[1]);
		RTMP_IO_WRITE32(pAd, PFMU_R19, readValue32[0]);
	}

	/* Positive subcarriers */
	for (scIndex = pTab->lwb2; scIndex <= pTab->upb2; scIndex++) {
		// Read PFMU_R10 (0x2f28) first
		RTMP_IO_READ32(pAd, PFMU_R10, &value32);
		// Wite PFMU_R10 to trigger read command
		RTMP_IO_WRITE32(pAd, PFMU_R10, ((profileIdx << 10)|scIndex));
		// Read PFMU_R19 ~ R23
		RTMP_IO_READ32(pAd, PFMU_R19, &readValue32[0]);
		RTMP_IO_READ32(pAd, PFMU_R20, &readValue32[1]);
		RTMP_IO_READ32(pAd, PFMU_R21, &readValue32[2]);
		RTMP_IO_READ32(pAd, PFMU_R22, &readValue32[3]);
		RTMP_IO_READ32(pAd, PFMU_R23, &readValue32[4]);

		readValue32[0] = (psi21_I << 16) | phill_I;

		// Read PFMU_R10 (0x2f28) first
		RTMP_IO_READ32(pAd, PFMU_R10, &value32);
		// Wite PFMU_R10 to trigger read command
		RTMP_IO_WRITE32(pAd, PFMU_R10, ((profileIdx << 10)|scIndex));
		// Wite PFMU_R19 ~ R23
		RTMP_IO_WRITE32(pAd, PFMU_R23, readValue32[4]);
		RTMP_IO_WRITE32(pAd, PFMU_R22, readValue32[3]);
		RTMP_IO_WRITE32(pAd, PFMU_R21, readValue32[2]);
		RTMP_IO_WRITE32(pAd, PFMU_R20, readValue32[1]);
		RTMP_IO_WRITE32(pAd, PFMU_R19, readValue32[0]);
	}

	return TRUE;
}


BOOLEAN Set_TxBfProfileDataReadAll(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UCHAR profileIdx;
	UCHAR TxBfType, BW, psi21_I, phill_I;
	CHAR *value;
	UINT value32, readValue32[5];
	UCHAR Input[3];
	SC_TABLE_ENTRY *pTab;
	int i, scIndex, idx;

	/* Profile Select : Subcarrier Select */
	if (strlen(arg) != 8)
		return FALSE;

	for (i = 0, value = rstrtok(arg,":"); value; value = rstrtok(NULL,":")) {
		if ((!isxdigit(*value)) || (!isxdigit(*(value+1))) )
			return FALSE;  /*Invalid*/

		AtoH(value, &Input[i++], 1);
	}

	TxBfType = Input[0];
	profileIdx = Input[1];
	BW = Input[2];

	if (TxBfType == 0) // ITxBf
		pTab = &impSCTable[BW];
	else
		pTab = &expSCTable[BW];

	/* Negative subcarriers */
	idx = 0;
	for (scIndex = pTab->lwb1; scIndex <= pTab->upb1; scIndex++) {
		// Read PFMU_R10 (0x2f28) first
		RTMP_IO_READ32(pAd, PFMU_R10, &value32);
		// Wite PFMU_R10 to trigger read command
		RTMP_IO_WRITE32(pAd, PFMU_R10, ((profileIdx << 10)|scIndex));
		// Read PFMU_R19 ~ R23
		RTMP_IO_READ32(pAd, PFMU_R19, &readValue32[0]);
		RTMP_IO_READ32(pAd, PFMU_R20, &readValue32[1]);
		RTMP_IO_READ32(pAd, PFMU_R21, &readValue32[2]);
		RTMP_IO_READ32(pAd, PFMU_R22, &readValue32[3]);
		RTMP_IO_READ32(pAd, PFMU_R23, &readValue32[4]);

		phill_I = readValue32[0] & 0x0FF;
		psi21_I = (readValue32[0] >> 16) & 0x0FF;

		if ((idx & 0x7) == 0) {
			printk("\n");
			printk("(%x, %x) ", psi21_I, phill_I);
		} else {
			printk("(%x, %x) ", psi21_I, phill_I);
		}

		idx++;
	}

	printk("\n");

	/* Positive subcarriers */
	idx = 0;
	for (scIndex = pTab->lwb2; scIndex <= pTab->upb2; scIndex++) {
		// Read PFMU_R10 (0x2f28) first
		RTMP_IO_READ32(pAd, PFMU_R10, &value32);
		// Wite PFMU_R10 to trigger read command
		RTMP_IO_WRITE32(pAd, PFMU_R10, ((profileIdx << 10)|scIndex));
		// Read PFMU_R19 ~ R23
		RTMP_IO_READ32(pAd, PFMU_R19, &readValue32[0]);
		RTMP_IO_READ32(pAd, PFMU_R20, &readValue32[1]);
		RTMP_IO_READ32(pAd, PFMU_R21, &readValue32[2]);
		RTMP_IO_READ32(pAd, PFMU_R22, &readValue32[3]);
		RTMP_IO_READ32(pAd, PFMU_R23, &readValue32[4]);

		phill_I = readValue32[0] & 0x0FF;
		psi21_I = (readValue32[0] >> 16) & 0x0FF;

		if ((idx & 0x7) == 0) {
			printk("\n");
			printk("(%x, %x) ", psi21_I, phill_I);
		} else {
			printk("(%x, %x) ", psi21_I, phill_I);
		}

		idx++;
	}

	return TRUE;
}

BOOLEAN Set_TxBfProfileTag_TimeOut(PRTMP_ADAPTER pAd, PSTRING arg)
{
	PFMU_TimeOut = simple_strtol(arg, 0, 10);
	CMDInIdx |= 16;

	return TRUE;
}


BOOLEAN Set_TxBfProfileTag_Matrix(PRTMP_ADAPTER pAd, PSTRING arg)
{
	CHAR *value;
	int i;

	/* ng[1:0], cw[1:0], nrow[1:0], ncol[1:0] */
	if (strlen(arg) != 14)
		return FALSE;

	for (i = 0, value = rstrtok(arg,":"); value; value = rstrtok(NULL,":")) {
		if ((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) )
			return FALSE;  /*Invalid*/

		AtoH(value, &MatrixForm[i++], 1);
	}

	CMDInIdx |= 1;

	DBGPRINT(RT_DEBUG_TRACE,
			("ng[1:0]=%x\n" "cw[1:0]=%x\n" "nrow[1:0]=%x\n" "nol[1:0]=%x\n" "LM[1:0]=%x\n",
			 MatrixForm[0], MatrixForm[1], MatrixForm[2],
			 MatrixForm[3], MatrixForm[4]));
	return TRUE;
}


BOOLEAN Set_TxBfProfileTag_SNR(PRTMP_ADAPTER pAd, PSTRING arg)
{
	CHAR *value;
	int i;

	/* STS1_SNR: STS0_SNR */
	if (strlen(arg) != 5)
		return FALSE;

	for (i = 0, value = rstrtok(arg,":"); value; value = rstrtok(NULL,":")) {
		if ((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) )
			return FALSE;  /*Invalid*/

		AtoH(value, &StsSnr[i++], 1);
	}

	CMDInIdx |= 8;

	DBGPRINT(RT_DEBUG_TRACE, ("STS1_SNR=0x%x\n" "STS1_SNR=0x%x\n",
			StsSnr[0], StsSnr[1]));

	return TRUE;
}


BOOLEAN Set_TxBfProfileTag_TxScale(PRTMP_ADAPTER pAd, PSTRING arg)
{
	CHAR *value;
	int i;

	/* Tx1_scale_2ss: Tx0_scale_2ss: Tx1_scale_1ss: Tx0_scale_1ss*/
	if (strlen(arg) != 11)
		return FALSE;

	for (i = 0, value = rstrtok(arg,":"); value; value = rstrtok(NULL,":")) {
		if ((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) )
			return FALSE;  /*Invalid*/

		AtoH(value, &TxScale[i++], 1);
	}

	CMDInIdx |= 4;

	DBGPRINT(RT_DEBUG_TRACE,
			("Tx1_scale_2ss=0x%x\n" "Tx0_scale_2ss=0x%x\n" "Tx1_scale_1ss=0x%x\n" "Tx0_scale_1ss=0x%x\n",
			TxScale[0], TxScale[1], TxScale[2], TxScale[3]));

	return TRUE;
}


BOOLEAN Set_TxBfProfileTag_MAC(PRTMP_ADAPTER pAd, PSTRING arg)
{
	CHAR *value;
	int i;

	/* Mac address acceptable format 01:02:03:04:05:06 length 17 */
	if (strlen(arg) != 17)
		return FALSE;

	for (i = 0, value = rstrtok(arg,":"); value; value = rstrtok(NULL,":")) {
		if ((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) )
			return FALSE;  /*Invalid*/

		AtoH(value, &macAddr[i++], 1);
	}

	CMDInIdx |= 2;

	DBGPRINT(RT_DEBUG_TRACE,
			("Set_TxBfProfileTage_MAC\n" "DS MAC=0x%x:0x%x:0x%x:0x%x:0x%x:0x%x \n",
			macAddr[0],macAddr[1],macAddr[2],macAddr[3],
			macAddr[4],macAddr[5]));

	return TRUE;
}


BOOLEAN Set_TxBfProfileTag_Flg(PRTMP_ADAPTER pAd, PSTRING arg)
{
	CHAR *value;
	int i;

	/* LD, EO, IO, I/E */
	if (strlen(arg) != 11)
		return FALSE;

	for (i = 0, value = rstrtok(arg,":"); value; value = rstrtok(NULL,":")) {
		if ((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) )
			return FALSE;  /*Invalid*/

		AtoH(value, &FlgStatus[i++], 1);
	}

	CMDInIdx |= 32;

	DBGPRINT(RT_DEBUG_TRACE, ("LD=%d\n" "EO=%d\n" "IO=%d\n" "I/E=%d\n",
			FlgStatus[0], FlgStatus[1], FlgStatus[2], FlgStatus[3]));

	return TRUE;
}
#endif  //TXBF_SUPPORT
#endif  //MT76x2


#ifdef DOT11_N_SUPPORT
void assoc_ht_info_debugshow(PRTMP_ADAPTER pAd, MAC_TABLE_ENTRY *pEntry,
	UCHAR ht_cap_len, HT_CAPABILITY_IE *pHTCapability)
{
	HT_CAP_INFO *pHTCap;
	HT_CAP_PARM *pHTCapParm;
	EXT_HT_CAP_INFO *pExtHT;
#ifdef TXBF_SUPPORT
	HT_BF_CAP *pBFCap;
#endif /* TXBF_SUPPORT */

	if (pHTCapability && (ht_cap_len > 0)) {
		pHTCap = &pHTCapability->HtCapInfo;
		pHTCapParm = &pHTCapability->HtCapParm;
		pExtHT = &pHTCapability->ExtHtCapInfo;
#ifdef TXBF_SUPPORT
		pBFCap = &pHTCapability->TxBFCap;
#endif /* TXBF_SUPPORT */

		DBGPRINT(RT_DEBUG_TRACE, ("Peer - 11n HT Info\n"));
		DBGPRINT(RT_DEBUG_TRACE, ("\tHT Cap Info: \n"));
		DBGPRINT(RT_DEBUG_TRACE,
				("\t\t HT_RX_LDPC(%d), BW(%d), MIMOPS(%d), GF(%d), ShortGI_20(%d), ShortGI_40(%d)\n",
				pHTCap->ht_rx_ldpc, pHTCap->ChannelWidth,
				pHTCap->MimoPs, pHTCap->GF, pHTCap->ShortGIfor20,
				pHTCap->ShortGIfor40));
		DBGPRINT(RT_DEBUG_TRACE,
				("\t\t TxSTBC(%d), RxSTBC(%d), DelayedBA(%d), A-MSDU(%d), CCK_40(%d)\n",
				pHTCap->TxSTBC, pHTCap->RxSTBC, pHTCap->DelayedBA,
				pHTCap->AMsduSize, pHTCap->CCKmodein40));
		DBGPRINT(RT_DEBUG_TRACE,
				("\t\t PSMP(%d), Forty_Mhz_Intolerant(%d), L-SIG(%d)\n",
				pHTCap->PSMP, pHTCap->Forty_Mhz_Intolerant,
				pHTCap->LSIGTxopProSup));

		DBGPRINT(RT_DEBUG_TRACE, ("\tHT Parm Info: \n"));
		DBGPRINT(RT_DEBUG_TRACE,
				("\t\t MaxRx A-MPDU Factor(%d), MPDU Density(%d)\n",
				pHTCapParm->MaxRAmpduFactor, pHTCapParm->MpduDensity));

		DBGPRINT(RT_DEBUG_TRACE, ("\tHT MCS set: \n"));
		DBGPRINT(RT_DEBUG_TRACE,
				("\t\t RxMCS(%02x %02x %02x %02x %02x) MaxRxMbps(%d) TxMCSSetDef(%02x)\n",
				pHTCapability->MCSSet[0], pHTCapability->MCSSet[1],
				pHTCapability->MCSSet[2], pHTCapability->MCSSet[3],
				 pHTCapability->MCSSet[4],
				(pHTCapability->MCSSet[11]<<8) + pHTCapability->MCSSet[10],
				pHTCapability->MCSSet[12]));

		DBGPRINT(RT_DEBUG_TRACE, ("\tExt HT Cap Info: \n"));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\t PCO(%d), TransTime(%d), MCSFeedback(%d), +HTC(%d), RDG(%d)\n",
			pExtHT->Pco, pExtHT->TranTime, pExtHT->MCSFeedback, pExtHT->PlusHTC, pExtHT->RDGSupport));

#ifdef TXBF_SUPPORT
		DBGPRINT(RT_DEBUG_TRACE, ("\tTX BF Cap: \n"));
		DBGPRINT(RT_DEBUG_TRACE,
				("\t\t ImpRxCap(%d), RXStagSnd(%d), TXStagSnd(%d), RxNDP(%d), TxNDP(%d) ImpTxCap(%d)\n",
				pBFCap->TxBFRecCapable, pBFCap->RxSoundCapable, pBFCap->TxSoundCapable,
				pBFCap->RxNDPCapable, pBFCap->TxNDPCapable, pBFCap->ImpTxBFCapable));
		DBGPRINT(RT_DEBUG_TRACE,
				("\t\t Calibration(%d), ExpCSICapable(%d), ExpComSteerCapable(%d), ExpCSIFbk(%d), ExpNoComBF(%d) ExpComBF(%d)\n",
				pBFCap->Calibration, pBFCap->ExpCSICapable, pBFCap->ExpComSteerCapable,
				pBFCap->ExpCSIFbk, pBFCap->ExpNoComBF, pBFCap->ExpComBF));
		DBGPRINT(RT_DEBUG_TRACE,
				("\t\t MinGrouping(%d), CSIBFAntSup(%d), NoComSteerBFAntSup(%d), ComSteerBFAntSup(%d), CSIRowBFSup(%d) ChanEstimation(%d)\n",
				pBFCap->MinGrouping, pBFCap->CSIBFAntSup, pBFCap->NoComSteerBFAntSup,
				pBFCap->ComSteerBFAntSup, pBFCap->CSIRowBFSup, pBFCap->ChanEstimation));
#endif /* TXBF_SUPPORT */

		DBGPRINT(RT_DEBUG_TRACE,
				("\nPeer - MODE=%d, BW=%d, MCS=%d, ShortGI=%d, MaxRxFactor=%d, MpduDensity=%d, MIMOPS=%d, AMSDU=%d\n",
				pEntry->HTPhyMode.field.MODE, pEntry->HTPhyMode.field.BW,
				pEntry->HTPhyMode.field.MCS, pEntry->HTPhyMode.field.ShortGI,
				pEntry->MaxRAmpduFactor, pEntry->MpduDensity,
				pEntry->MmpsMode, pEntry->AMsduSize));

#ifdef DOT11N_DRAFT3
		DBGPRINT(RT_DEBUG_TRACE, ("\tExt Cap Info: \n"));
		DBGPRINT(RT_DEBUG_TRACE,
			("\t\tBss2040CoexistMgmt=%d\n",
			pEntry->BSS2040CoexistenceMgmtSupport));
#endif /* DOT11N_DRAFT3 */
	}
}


BOOLEAN Set_BurstMode_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	ULONG Value = simple_strtol(arg, 0, 10);
	pAd->CommonCfg.bRalinkBurstMode = ((Value == 1) ? TRUE : FALSE);

	AsicSetRalinkBurstMode(pAd, pAd->CommonCfg.bRalinkBurstMode);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_BurstMode_Proc ::%s\n",
			(pAd->CommonCfg.bRalinkBurstMode == TRUE) ?
			"enabled" : "disabled"));

	return TRUE;
}
#endif /* DOT11_N_SUPPORT */


#ifdef DOT11_VHT_AC
void assoc_vht_info_debugshow(RTMP_ADAPTER *pAd, MAC_TABLE_ENTRY *pEntry,
	VHT_CAP_IE *vht_cap, VHT_OP_IE *vht_op)
{
	VHT_CAP_INFO *cap_info;
	VHT_MCS_SET *mcs_set;
	VHT_OP_INFO *op_info;
	VHT_MCS_MAP *mcs_map;

	if (!WMODE_CAP_AC(pAd->CommonCfg.PhyMode))
		return;

	DBGPRINT(RT_DEBUG_TRACE, ("Peer - 11AC VHT Info\n"));
	if (vht_cap) {
		cap_info = &vht_cap->vht_cap;
		mcs_set = &vht_cap->mcs_set;

		hex_dump("peer vht_cap raw data", (UCHAR *)cap_info, sizeof(VHT_CAP_INFO));
		hex_dump("peer vht_mcs raw data", (UCHAR *)mcs_set, sizeof(VHT_MCS_SET));

		DBGPRINT(RT_DEBUG_TRACE, ("\tVHT Cap Info: \n"));
		DBGPRINT(RT_DEBUG_TRACE,
				("\t\tMaxMpduLen(%d), BW(%d), SGI_80M(%d), RxLDPC(%d), TxSTBC(%d), RxSTBC(%d), +HTC-VHT(%d)\n",
				cap_info->max_mpdu_len, cap_info->ch_width,
				cap_info->sgi_80M, cap_info->rx_ldpc, cap_info->tx_stbc,
				cap_info->rx_stbc, cap_info->htc_vht_cap));
		DBGPRINT(RT_DEBUG_TRACE,
				("\t\tMaxAmpduExp(%d), VhtLinkAdapt(%d), RxAntConsist(%d), TxAntConsist(%d)\n",
				cap_info->max_ampdu_exp, cap_info->vht_link_adapt,
				cap_info->rx_ant_consistency, cap_info->tx_ant_consistency));
		mcs_map = &mcs_set->rx_mcs_map;
		DBGPRINT(RT_DEBUG_TRACE,
				("\t\tRxMcsSet: HighRate(%d), RxMCSMap(%d,%d,%d,%d,%d,%d,%d)\n",
				mcs_set->rx_high_rate, mcs_map->mcs_ss1,
				mcs_map->mcs_ss2, mcs_map->mcs_ss3,
				mcs_map->mcs_ss4, mcs_map->mcs_ss5,
				mcs_map->mcs_ss6, mcs_map->mcs_ss7));
		mcs_map = &mcs_set->tx_mcs_map;
		DBGPRINT(RT_DEBUG_TRACE,
				("\t\tTxMcsSet: HighRate(%d), TxMcsMap(%d,%d,%d,%d,%d,%d,%d)\n",
				mcs_set->tx_high_rate, mcs_map->mcs_ss1, mcs_map->mcs_ss2, mcs_map->mcs_ss3,
				mcs_map->mcs_ss4, mcs_map->mcs_ss5, mcs_map->mcs_ss6, mcs_map->mcs_ss7));
#ifdef VHT_TXBF_SUPPORT
		DBGPRINT(RT_DEBUG_TRACE,
				("\t\tETxBfCap: Bfer(%d), Bfee(%d), SndDim(%d)\n",
				cap_info->bfer_cap_su, cap_info->bfee_cap_su, cap_info->num_snd_dimension));
#endif
	}

	if (vht_op) {
		op_info = &vht_op->vht_op_info;
		mcs_map = &vht_op->basic_mcs_set;
		DBGPRINT(RT_DEBUG_TRACE, ("\tVHT OP Info: \n"));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\tChannel Width(%d), CenteralFreq1(%d), CenteralFreq2(%d)\n",
				op_info->ch_width, op_info->center_freq_1, op_info->center_freq_2));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\tBasicMCSSet(SS1:%d, SS2:%d, SS3:%d, SS4:%d, SS5:%d, SS6:%d, SS7:%d)\n",
				mcs_map->mcs_ss1, mcs_map->mcs_ss2, mcs_map->mcs_ss3,
				mcs_map->mcs_ss4, mcs_map->mcs_ss5, mcs_map->mcs_ss6,
				mcs_map->mcs_ss7));
	}

	DBGPRINT(RT_DEBUG_TRACE, ("\n"));

}
#endif /* DOT11_VHT_AC */


BOOLEAN Set_RateAdaptInterval(RTMP_ADAPTER *pAd, PSTRING arg)
{
	UINT32 ra_time, ra_qtime;
	PSTRING token;
	char sep = ':';
	ULONG irqFlags;
/*
	The ra_interval inupt string format should be d:d, in units of ms.
		=>The first decimal number indicates the rate adaptation checking period,
		=>The second decimal number indicates the rate adaptation quick response checking period.
*/
	DBGPRINT(RT_DEBUG_TRACE,("%s():%s\n", __FUNCTION__, arg));

	token = strchr(arg, sep);
	if (token != NULL) {
		*token = '\0';

		if (strlen(arg) && strlen(token+1)) {
			ra_time = simple_strtol(arg, 0, 10);
			ra_qtime = simple_strtol(token+1, 0, 10);
			DBGPRINT(RT_DEBUG_OFF,
					("%s():Set RateAdaptation TimeInterval as(%d:%d) ms\n",
					__FUNCTION__, ra_time, ra_qtime));

			RTMP_IRQ_LOCK(&pAd->irq_lock, irqFlags);
			pAd->ra_interval = ra_time;
			pAd->ra_fast_interval = ra_qtime;
#ifdef CONFIG_AP_SUPPORT
			if (pAd->ApCfg.ApQuickResponeForRateUpTimerRunning == TRUE) {
				BOOLEAN Cancelled;

				RTMPCancelTimer(&pAd->ApCfg.ApQuickResponeForRateUpTimer, &Cancelled);
				pAd->ApCfg.ApQuickResponeForRateUpTimerRunning = FALSE;
			}
#endif /* CONFIG_AP_SUPPORT  */
			RTMP_IRQ_UNLOCK(&pAd->irq_lock, irqFlags);
			return TRUE;
		}
	}

	return FALSE;
}


BOOLEAN Set_VcoPeriod_Proc(RTMP_ADAPTER *pAd, PSTRING arg)
{
	pAd->chipCap.VcoPeriod = simple_strtol(arg, 0, 10);

	DBGPRINT(RT_DEBUG_TRACE,
			("VCO Period = %d seconds\n", pAd->chipCap.VcoPeriod));
	return TRUE;
}

#ifdef SINGLE_SKU
BOOLEAN Set_ModuleTxpower_Proc(PRTMP_ADAPTER pAd, PSTRING arg)
{
	UINT16 Value;

	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE)) {
		DBGPRINT(RT_DEBUG_ERROR,
				("Do NOT accept this command after interface is up.\n"));
		return FALSE;
	}

	Value = (UINT16)simple_strtol(arg, 0, 10);
	pAd->CommonCfg.ModuleTxpower = Value;
	DBGPRINT(RT_DEBUG_TRACE, ("IF Set_ModuleTxpower_Proc::(ModuleTxpower=%d)\n",
			pAd->CommonCfg.ModuleTxpower));
	return TRUE;
}
#endif /* SINGLE_SKU */


#ifdef WFA_VHT_PF
BOOLEAN set_vht_nss_mcs_cap(RTMP_ADAPTER *pAd, PSTRING arg)
{
	CHAR *token, sep[2] = {':', '-'};
	UCHAR val[3] = {0}, ss, mcs_l, mcs_h, mcs_cap;
	BOOLEAN status = FALSE;
	int idx = 0;

	DBGPRINT(RT_DEBUG_TRACE, ("%s():intput string=%s\n", __FUNCTION__, arg));
	ss = mcs_l = mcs_h = 0;

	while (arg) {
		DBGPRINT(RT_DEBUG_TRACE, ("%s():intput string[len=%d]=%s\n",
				__FUNCTION__, strlen(arg), arg));
		if (idx < 2) {
			token = rtstrchr(arg, sep[idx]);
			if (!token) {
				DBGPRINT(RT_DEBUG_TRACE,
						("cannot found token '%c' in string \"%s\"!\n",
						sep[idx], arg));
				return FALSE;
			}
			*token++ = 0;
		} else
			token = NULL;

		if (strlen(arg)) {
			val[idx] = (UCHAR)simple_strtoul(arg, NULL, 10);
			DBGPRINT(RT_DEBUG_TRACE, ("%s():token string[len=%d]=%s, val[%d]=%d\n",
					__FUNCTION__, strlen(arg), arg, idx, val[idx]));
			idx++;
		}
		arg = token;
		if (idx == 3)
			break;
	}

	if (idx < 3)
		return FALSE;

	ss = val[0];
	mcs_l = val[1];
	mcs_h = val[2];
	DBGPRINT(RT_DEBUG_TRACE, ("ss=%d, mcs_l=%d, mcs_h=%d\n", ss, mcs_l, mcs_h));
	if (ss && mcs_h) {
		if (ss <= pAd->chipCap.max_nss)
			pAd->CommonCfg.vht_nss_cap = ss;
		else
			pAd->CommonCfg.vht_nss_cap = pAd->chipCap.max_nss;

		switch (mcs_h) {
		case 7:
			mcs_cap = VHT_MCS_CAP_7;
			break;
		case 8:
			mcs_cap = VHT_MCS_CAP_8;
			break;
		case 9:
			mcs_cap = VHT_MCS_CAP_9;
			break;
		default:
			mcs_cap = VHT_MCS_CAP_9;
			break;
		}

		if (mcs_h <= pAd->chipCap.max_vht_mcs)
			pAd->CommonCfg.vht_mcs_cap = mcs_cap;
		else
			pAd->CommonCfg.vht_mcs_cap = pAd->chipCap.max_vht_mcs;

		DBGPRINT(RT_DEBUG_TRACE, ("%s():ss=%d, mcs_cap=%d, vht_nss_cap=%d, vht_mcs_cap=%d\n",
					__FUNCTION__, ss, mcs_cap,
					pAd->CommonCfg.vht_nss_cap,
					pAd->CommonCfg.vht_mcs_cap));
		status = TRUE;
	}

	return status;
}


BOOLEAN set_vht_nss_mcs_opt(RTMP_ADAPTER *pAd, PSTRING arg)
{
	DBGPRINT(RT_DEBUG_TRACE, ("%s():intput string=%s\n", __FUNCTION__, arg));

	return Set_HtMcs_Proc(pAd, arg);
}


BOOLEAN set_vht_opmode_notify_ie(RTMP_ADAPTER *pAd, PSTRING arg)
{
	CHAR *token;
	UINT ss, bw;
	BOOLEAN status = FALSE;

	DBGPRINT(RT_DEBUG_TRACE, ("%s():intput string=%s\n", __FUNCTION__, arg));
	token = rtstrchr(arg, ':');
	if (!token)
		return FALSE;

	*token = 0;
	token++;
	if (strlen(arg) && strlen(token)) {
		ss = simple_strtoul(arg, NULL, 10);
		bw = simple_strtoul(token, NULL, 10);

		DBGPRINT(RT_DEBUG_TRACE, ("%s():ss=%d, bw=%d\n", __FUNCTION__, ss, bw));
		if (ss > 0 && ss <= 2)
			pAd->vht_pf_op_ss = ss;
		else
			pAd->vht_pf_op_ss = pAd->Antenna.field.RxPath;

		switch (bw) {
		case 20:
			pAd->vht_pf_op_bw = BAND_WIDTH_20;
			break;
		case 40:
			pAd->vht_pf_op_bw = BAND_WIDTH_40;
			break;
		case 80:
		default:
			pAd->vht_pf_op_bw = BAND_WIDTH_80;
			break;
		}
		status = TRUE;
	}

	pAd->force_vht_op_mode = status;

	DBGPRINT(RT_DEBUG_TRACE,
			("%s():force_vht_op_mode=%d, vht_pf_op_ss=%d, vht_pf_op_bw=%d\n",
			__FUNCTION__, pAd->force_vht_op_mode, pAd->vht_pf_op_ss,
			pAd->vht_pf_op_bw));

	return status;
}


BOOLEAN set_force_operating_mode(RTMP_ADAPTER *pAd, PSTRING arg)
{
	pAd->force_vht_op_mode = (simple_strtol(arg, 0, 10) > 0 ? TRUE : FALSE);

	if (pAd->force_vht_op_mode == TRUE) {
		pAd->vht_pf_op_ss = 1; // 1SS
		pAd->vht_pf_op_bw = BAND_WIDTH_20; // 20M
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s(): force_operating_mode=%d\n",
				__FUNCTION__, pAd->force_vht_op_mode));
	if (pAd->force_vht_op_mode == TRUE) {
		DBGPRINT(RT_DEBUG_TRACE,
				("\tforce_operating_mode as %dSS in 20MHz BW\n",
				pAd->vht_pf_op_ss));
	}

	return TRUE;
}


BOOLEAN set_force_noack(RTMP_ADAPTER *pAd, PSTRING arg)
{
	pAd->force_noack = (simple_strtol(arg, 0, 10) > 0 ? TRUE : FALSE);
	DBGPRINT(RT_DEBUG_TRACE, ("%s(): force_noack=%d\n",
			__FUNCTION__, pAd->force_noack));

	return TRUE;
}


BOOLEAN set_force_amsdu(RTMP_ADAPTER *pAd, PSTRING arg)
{
	pAd->force_amsdu = (simple_strtol(arg, 0, 10) > 0 ? TRUE : FALSE);
	DBGPRINT(RT_DEBUG_TRACE, ("%s(): force_amsdu=%d\n",
			__FUNCTION__, pAd->force_amsdu));
	return TRUE;
}


BOOLEAN set_force_vht_sgi(RTMP_ADAPTER *pAd, PSTRING arg)
{
	pAd->vht_force_sgi = (simple_strtol(arg, 0, 10) > 0 ? TRUE : FALSE);
	DBGPRINT(RT_DEBUG_TRACE, ("%s(): vht_force_sgi=%d\n",
			__FUNCTION__, pAd->vht_force_sgi));

	return TRUE;
}


BOOLEAN set_force_vht_tx_stbc(RTMP_ADAPTER *pAd, PSTRING arg)
{
	pAd->vht_force_tx_stbc = (simple_strtol(arg, 0, 10) > 0 ? TRUE : FALSE);
	if (pAd->CommonCfg.TxStream < 2) {
		DBGPRINT(RT_DEBUG_TRACE,
				("%s(): TxStream=%d is not enough for TxSTBC!\n",
				__FUNCTION__, pAd->CommonCfg.TxStream));
		pAd->vht_force_tx_stbc = 0;
	}
	DBGPRINT(RT_DEBUG_TRACE, ("%s(): vht_force_tx_stbc=%d\n",
				__FUNCTION__, pAd->vht_force_tx_stbc));

	return TRUE;
}


BOOLEAN set_force_ext_cca(RTMP_ADAPTER *pAd, PSTRING arg)
{
	ULONG cca_cfg;
	UINT32 mac_val;

	cca_cfg = (simple_strtol(arg, 0, 10) > 0 ? TRUE : FALSE);
	if (cca_cfg)
		mac_val = 0x04101b3f;
	else
		mac_val = 0x583f;
	RTMP_IO_WRITE32(pAd, TXOP_CTRL_CFG, mac_val);

	return TRUE;
}


BOOLEAN set_rx_rts_cts(RTMP_ADAPTER *pAd, PSTRING arg)
{
	UINT32 do_rx, i;
	UINT32 mac_val, mask;
	PSTRING cmds[4] = {"no", "rts", "cts", "all"};

	for (i =0 ; i < 4; i++) {
		if (strcmp(arg, cmds[i]) ==0)
			break;
	}

	if (i >= 4)
		i = 0;

	RTMP_IO_READ32(pAd, RX_FILTR_CFG, &mac_val);
	switch (i) {
	case 1: // receive rts
		mac_val &= (~0x1000);
		break;
	case 2: // receive cts
		mac_val &= (~0x0800);
		break;
	case 3: // receive both
		mac_val &= (~0x1800);
		break;
	case 0: // filter out both rts/cts
	default:
		mac_val |= 0x1800;
		break;
	}
	RTMP_IO_WRITE32(pAd, RX_FILTR_CFG, mac_val);
	RTMP_IO_READ32(pAd, RX_FILTR_CFG, &mac_val);

	DBGPRINT(RT_DEBUG_TRACE,
			("%s():Configure the RTS/CTS filter as receive %s(0x%x)\n",
			__FUNCTION__, cmds[i], mac_val));

	return TRUE;
}
#endif /* WFA_VHT_PF */

BOOLEAN set_cal_test(RTMP_ADAPTER *pAd, PSTRING arg)
{
	UCHAR cal_test_type;

	cal_test_type = simple_strtol(arg, 0, 10);

	if (pAd->chipOps.cal_test) {
		pAd->chipOps.cal_test(pAd, cal_test_type);
	}

	return TRUE;
}


#ifdef DOT11_N_SUPPORT

#ifdef MT76x2
#define MAX_AGG_CNT	48
#elif defined(RT65xx) || defined(MT7601)
#define MAX_AGG_CNT	32
#elif defined(RT2883) || defined(RT3883)
#define MAX_AGG_CNT	16
#else
#define MAX_AGG_CNT	8
#endif

/* DisplayTxAgg - display Aggregation statistics from MAC */
void DisplayTxAgg (RTMP_ADAPTER *pAd)
{
	ULONG totalCount;
	ULONG aggCnt[MAX_AGG_CNT + 2];
	int i;

	AsicReadAggCnt(pAd, aggCnt, sizeof(aggCnt) / sizeof(ULONG));
	totalCount = aggCnt[0] + aggCnt[1];
	if (totalCount > 0)
		for (i = 0; i < MAX_AGG_CNT; i++) {
			DBGPRINT(RT_DEBUG_OFF,
					("\t%d MPDU=%ld (%ld%%)\n", i+1,
					aggCnt[i+2],
					aggCnt[i+2]*100/totalCount));
		}
	printk("====================\n");

}
#endif /* DOT11_N_SUPPORT */

#ifdef RLT_RF
static BOOLEAN set_rlt_rf(RTMP_ADAPTER *pAd, PSTRING arg)
{
	int bank_id = 0, rf_id = 0, rv = 0, rf_v;
	UCHAR rf_val = 0;

	if (arg) {
		rv = sscanf(arg, "%d-%d-%x", &(bank_id), &(rf_id), &(rf_v));
		DBGPRINT(RT_DEBUG_TRACE,
				("%s():rv = %d, bank_id = %d, rf_id = %d, rf_val = 0x%02x\n",
				__FUNCTION__, rv, bank_id, rf_id, rf_v));
		rf_val = (UCHAR )rf_v;
		if (rv == 3) {
			rlt_rf_write(pAd, (UCHAR)bank_id, (UCHAR)rf_id, (UCHAR)rf_val);
			rlt_rf_read(pAd, bank_id, rf_id, &rf_val);
			DBGPRINT(RT_DEBUG_TRACE, ("%s():%d %03d 0x%02X\n",
					__FUNCTION__, bank_id, rf_id, rf_val));
		} else if (rv == 2) {
			rlt_rf_read(pAd, bank_id, rf_id, &rf_val);
			DBGPRINT(RT_DEBUG_TRACE, ("%s():%d %03d 0x%02X\n",
					__FUNCTION__, bank_id, rf_id, rf_val));
		}
	}

	return TRUE;
}
#endif /* RLT_RF */

#ifdef MT_RF
static BOOLEAN set_mt_rf(RTMP_ADAPTER *ad, PSTRING arg)
{
	int rf_idx = 0, offset = 0, rv = 0;
	UINT rf_val = 0;

	if (arg) {
		rv = sscanf(arg, "%d-%x-%x", &(rf_idx), &(offset), &(rf_val));
		DBGPRINT(RT_DEBUG_TRACE,
				("%s():rv = %d, rf_idx = %d, offset = 0x%04x, rf_val = 0x%08x\n",
				__FUNCTION__, rv, rf_idx, offset, rf_val));
		if (rv == 3) {
			mt_rf_write(ad, (u8)rf_idx, (u16)offset, (u32)rf_val);
			rf_val = 0;
			mt_rf_read(ad, (u8)rf_idx, (u16)offset, (u32*)&rf_val);

			DBGPRINT(RT_DEBUG_TRACE,
					("%s():%d 0x%04x 0x%04x\n",
					__FUNCTION__, rf_idx, offset, rf_val));
		} else if (rv == 2) {
			mt_rf_read(ad, (u8)rf_idx, (u16)offset, (u32*)&rf_val);
			DBGPRINT(RT_DEBUG_TRACE,
					("%s():%d 0x%04x 0x%08x\n", __FUNCTION__,
					rf_idx, offset, rf_val));
		} else if (rv == 1) {
			//read all offset in the same rf index
			for (offset = 0; offset <= 0x33c; offset = offset+4) {
				mt_rf_read(ad, (u8)rf_idx, (u16)offset, (u32*)&rf_val);
				DBGPRINT(RT_DEBUG_TRACE,
						("%s():%d 0x%04x 0x%08x\n",
						__FUNCTION__, rf_idx, offset, rf_val));
				rf_val = 0;
			}
			offset = 0xfff;
			mt_rf_read(ad, (u8)rf_idx, (u16)offset, (u32*)&rf_val);
			DBGPRINT(RT_DEBUG_TRACE, ("%s():%d 0x%04x 0x%08x\n",
					__FUNCTION__, rf_idx, offset, rf_val));
		}
	}

	return TRUE;
}
#endif /* MT_RF */

BOOLEAN set_rf(RTMP_ADAPTER *pAd, PSTRING arg)
{
	BOOLEAN ret;

#ifdef RLT_RF
	if (pAd->chipCap.rf_type == RF_RLT)
		ret = set_rlt_rf(pAd, arg);
#endif

#ifdef MT_RF
	if (pAd->chipCap.rf_type == RF_MT)
		ret = set_mt_rf(pAd, arg);
#endif
	return ret;
}

static struct {
	PSTRING name;
	int (*show_proc)(RTMP_ADAPTER *pAd, PSTRING arg, ULONG BufLen);
} *PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC, RTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC[] = {
#ifdef DBG
	{"SSID",		Show_SSID_Proc},
	{"WirelessMode",	Show_WirelessMode_Proc},
	{"TxBurst",		Show_TxBurst_Proc},
	{"TxPreamble",		Show_TxPreamble_Proc},
	{"TxPower",		Show_TxPower_Proc},
	{"Channel",		Show_Channel_Proc},
	{"BGProtection",	Show_BGProtection_Proc},
	{"RTSThreshold",	Show_RTSThreshold_Proc},
	{"FragThreshold",	Show_FragThreshold_Proc},
#ifdef DOT11_N_SUPPORT
	{"HtBw",		Show_HtBw_Proc},
	{"HtMcs",		Show_HtMcs_Proc},
	{"HtGi",		Show_HtGi_Proc},
	{"HtOpMode",		Show_HtOpMode_Proc},
	{"HtExtcha",		Show_HtExtcha_Proc},
	{"HtMpduDensity",	Show_HtMpduDensity_Proc},
	{"HtBaWinSize",		Show_HtBaWinSize_Proc},
	{"HtRdg",		Show_HtRdg_Proc},
	{"HtAmsdu",		Show_HtAmsdu_Proc},
	{"HtAutoBa",		Show_HtAutoBa_Proc},
#endif /* DOT11_N_SUPPORT */
	{"CountryRegion",	Show_CountryRegion_Proc},
	{"CountryRegionABand",	Show_CountryRegionABand_Proc},
	{"CountryCode",		Show_CountryCode_Proc},
#ifdef AGGREGATION_SUPPORT
	{"PktAggregate",	Show_PktAggregate_Proc},
#endif
	{"WmmCapable",		Show_WmmCapable_Proc},
	{"IEEE80211H",		Show_IEEE80211H_Proc},
#ifdef CONFIG_STA_SUPPORT
	{"NetworkType",		Show_NetworkType_Proc},
	{"WPAPSK",		Show_WPAPSK_Proc},
	{"AutoReconnect", 	Show_AutoReconnect_Proc},
#endif
	{"AuthMode",		Show_AuthMode_Proc},
	{"EncrypType",		Show_EncrypType_Proc},
	{"DefaultKeyID",	Show_DefaultKeyID_Proc},
	{"Key1",		Show_Key1_Proc},
	{"Key2",		Show_Key2_Proc},
	{"Key3",		Show_Key3_Proc},
	{"Key4",		Show_Key4_Proc},
	{"PMK",			Show_PMK_Proc},
#ifdef SINGLE_SKU
	{"ModuleTxpower",	Show_ModuleTxpower_Proc},
#endif
#endif /* DBG */
	{"rainfo",		Show_STA_RAInfo_Proc},
	{NULL, NULL}
};


int RTMPShowCfgValue(PRTMP_ADAPTER pAd, PSTRING pName, PSTRING pBuf,
	UINT32 MaxLen)
{
	int Status = 0;

	for (PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC = RTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC;
		PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC->name;
		PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC++) {
		if (!strcmp(pName, PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC->name)) {
			if (PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC->show_proc(pAd, pBuf, MaxLen))
				Status = -EINVAL;
			break;  /*Exit for loop.*/
		}
	}

	if (PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC->name == NULL) {
		snprintf(pBuf, MaxLen, "\n");
		for (PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC = RTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC;
			PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC->name;
			PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC++) {
			if ((strlen(pBuf) + strlen(PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC->name)) >= MaxLen)
				break;
			sprintf(pBuf, "%s%s\n", pBuf, PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC->name);
		}
	}

	return Status;
}


BOOLEAN show_pwr_info(RTMP_ADAPTER *ad, PSTRING arg)
{
	if (ad->chipOps.show_pwr_info) {
		ad->chipOps.show_pwr_info(ad);
	}

	return TRUE;
}

#ifdef DBG_DIAGNOSE
BOOLEAN Show_Diag_Proc(RTMP_ADAPTER *pAd, PSTRING arg)
{
	RtmpDiagStruct *pDiag = NULL;
	UCHAR i, start, stop, McsIdx;
	UCHAR McsMaxIdx = MAX_MCS_SET;
#ifdef DBG_TXQ_DEPTH
	UCHAR SwQNumLevel;
#endif
#ifdef DBG_TX_RING_DEPTH
	UCHAR TxDescNumLevel;
#endif
#ifdef DOT11_VHT_AC
	UCHAR vht_mcs_max_idx = MAX_VHT_MCS_SET;
#endif

	pDiag = os_alloc_mem(sizeof(RtmpDiagStruct));
	if (!pDiag) {
		return FALSE;
	}

	NdisMoveMemory(pDiag, &pAd->DiagStruct, sizeof(RtmpDiagStruct));

#ifdef DOT11N_SS3_SUPPORT
	if (IS_RT2883(pAd) || IS_RT3883(pAd))
		McsMaxIdx = 24;
#endif
	if (pDiag->inited == FALSE)
		goto done;

	start = pDiag->ArrayStartIdx;
	stop = pDiag->ArrayCurIdx;
	printk("Start=%d, stop=%d!\n\n", start, stop);
	printk("    %-12s", "Time(Sec)");

	for (i=1; i< DIAGNOSE_TIME; i++) {
		printk("%-7d", i);
	}

	printk("\n    -------------------------------------------------------------------------------\n");
	printk("Tx Info:\n");
	printk("    %-12s", "TxDataCnt");
	for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME) {
		printk("%-7d", pDiag->diag_info[i].TxDataCnt);
	}

	printk("\n    %-12s", "TxFailCnt");
	for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME) {
		printk("%-7d", pDiag->diag_info[i].TxFailCnt);
	}

#ifdef DBG_TX_AGG_CNT
	printk("\n    %-12s", "TxAggCnt");
	for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME){
		printk("%-7d", pDiag->diag_info[i].TxAggCnt);
	}
	printk("\n");
#endif

#ifdef DBG_TXQ_DEPTH
	printk("\n    %-12s\n", "Sw-Queued TxSwQCnt");
	for (SwQNumLevel = 0 ; SwQNumLevel < 9; SwQNumLevel++) {
		if (SwQNumLevel == 8)
			printk("\t>%-5d",  SwQNumLevel);
		else
			printk("\t%-6d", SwQNumLevel);
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
			printk("%-7d", pDiag->diag_info[i].TxSWQueCnt[SwQNumLevel]);

		printk("\n");
	}
#endif

#ifdef DBG_TX_RING_DEPTH
	printk("\n    %-12s\n", "DMA-Queued TxDescCnt");
	for (TxDescNumLevel = 0; TxDescNumLevel < 16; TxDescNumLevel++) {
		if (TxDescNumLevel == 15)
			printk("\t>%-5d",  TxDescNumLevel);
		else
			printk("\t%-6d",  TxDescNumLevel);
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
			printk("%-7d", pDiag->diag_info[i].TxDescCnt[TxDescNumLevel]);

		printk("\n");
	}
#endif

#ifdef DOT11_N_SUPPORT
#ifdef DBG_TX_AGG_CNT
	printk("\n    %-12s\n", "Tx-Agged AMPDUCnt");
	for (McsIdx =0 ; McsIdx < 16; McsIdx++) {
		printk("\t%-6d", (McsIdx+1));
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME) {
			printk("%d(%d%%)  ", pDiag->diag_info[i].TxAMPDUCnt[McsIdx],
				pDiag->diag_info[i].TxAMPDUCnt[McsIdx] ?
				(pDiag->diag_info[i].TxAMPDUCnt[McsIdx] * 100 / pDiag->diag_info[i].TxAggCnt) : 0);
		}
		printk("\n");
	}
#endif /* DBG_TX_AGG_CNT */
#endif /* DOT11_N_SUPPORT */

#ifdef DBG_TX_MCS
	printk("\n    %-12s\n", "TxMcsCnt_HT");
	for (McsIdx =0 ; McsIdx < McsMaxIdx; McsIdx++) {
		printk("\t%-6d", McsIdx);
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
			printk("%-7d", pDiag->diag_info[i].TxMcsCnt_HT[McsIdx]);
		printk("\n");
	}

#ifdef DOT11_VHT_AC
	printk("\n    %-12s\n", "TxMcsCnt_VHT");
	for (McsIdx =0 ; McsIdx < vht_mcs_max_idx; McsIdx++) {
		printk("\t%-6d", McsIdx);
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
			printk("%-7d", pDiag->diag_info[i].TxMcsCnt_VHT[McsIdx]);
		printk("\n");
	}
#endif /* DOT11_VHT_AC */
#endif /* DBG_TX_MCS */

	printk("Rx Info\n");
	printk("    %-12s", "RxDataCnt");
	for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME) {
		printk("%-7d", pDiag->diag_info[i].RxDataCnt);
	}
	printk("\n    %-12s", "RxCrcErrCnt");
	for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME) {
		printk("%-7d", pDiag->diag_info[i].RxCrcErrCnt);
	}

#ifdef DBG_RX_MCS
	printk("\n    %-12s\n", "RxMcsCnt_HT");
	for (McsIdx =0 ; McsIdx < McsMaxIdx; McsIdx++) {
		printk("\t%-6d", McsIdx);
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME) {
			printk("(%d,%d)\t", pDiag->diag_info[i].RxCrcErrCnt_HT[McsIdx],
				pDiag->diag_info[i].RxMcsCnt_HT[McsIdx] + pDiag->diag_info[i].RxCrcErrCnt_HT[McsIdx]);
		}
		printk("\n");
	}

#ifdef DOT11_VHT_AC
	printk("\n    %-12s\n", "RxMcsCnt_VHT");
	for (McsIdx =0 ; McsIdx < vht_mcs_max_idx; McsIdx++) {
		printk("\t%-6d", McsIdx);
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME) {
			printk("(%d,%d)\t", pDiag->diag_info[i].RxCrcErrCnt_VHT[McsIdx],
				pDiag->diag_info[i].RxMcsCnt_VHT[McsIdx] + pDiag->diag_info[i].RxCrcErrCnt_VHT[McsIdx]);
		}
		printk("\n");
	}
#endif /* DOT11_VHT_AC */

#endif /* DBG_RX_MCS */
	printk("\n-------------\n");
done:
	os_free_mem(pDiag);
	return TRUE;
}
#endif /* DBG_DIAGNOSE */

