/******************************************************************
 *
 * Copyright (C) 2011 5V Technologies Ltd.
 * All Rights Reserved.
 *
 * This program is the proprietary software of 5V Technologies Ltd
 * and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from 5VT.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : 5VT OMCI protocol stack
 * Module  : gpon_hw_prx126
 * File    : intel_px126_pm.h
 *
 ******************************************************************/
#ifndef __INTEL_PX126_PM_H__
#define __INTEL_PX126_PM_H__

//////////////////////////////////////////////////////////////////////

/** GEM/XGEM port counters.
 *  Used by \ref fapi_pon_gem_port_counters_get and
 *  \ref fapi_pon_gem_all_counters_get.
 */
struct intel_px126_pon_gem_port_counters {
	/** GEM port ID for which the counters shall be reported. */
	unsigned short gem_port_id;
	/** Transmitted GEM frames. */
	unsigned long long tx_frames;
	/** Transmitted GEM frame fragments. */
	unsigned long long tx_fragments;
	/** Transmitted bytes in GEM frames.
	 *  This value reports user payload bytes only, not including
	 *  header bytes.
	 */
	unsigned long long tx_bytes;
	/** Received GEM frames.
	 *  This value reports the number of valid GEM frames that have
	 *  been received without uncorrectable errors and a valid HEC.
	 */
	unsigned long long rx_frames;
	/** Received GEM frame fragments. */
	unsigned long long rx_fragments;
	/** Received bytes in valid GEM frames.
	 *  This value reports user payload bytes only, not including
	 *  header bytes.
	 */
	unsigned long long rx_bytes;
	/** XGEM key errors.
	 *  The number of received key errors is provided for XG-PON,
	 *  XGS-PON, and NG-PON2 only. The value is set to 0 for GPON
	 *  implementations.
	 */
	unsigned long long key_errors;
};

/** Structure to collect counters related to GEM TC reception and transmission.
*/
struct intel_px126_pon_gtc_counters {
	/** Number of BIP errors. */
	unsigned long long bip_errors;
	/** Number of discarded GEM frames due to an invalid HEC.
	 *  Functionally the same as gem_hec_errors_uncorr,
	 *  which is not provided as a separate counter.
	 */
	unsigned long long disc_gem_frames;
	/** Number of corrected GEM HEC errors. */
	unsigned long long gem_hec_errors_corr;
	/** Number of uncorrected GEM HEC errors. */
	unsigned long long gem_hec_errors_uncorr;
	/** Number of corrected bandwidth map HEC errors. */
	unsigned long long bwmap_hec_errors_corr;
	/** Number of bytes received in corrected FEC codewords. */
	unsigned long long bytes_corr;
	/** Number of FEC codewords corrected */
	unsigned long long fec_codewords_corr;
	/** Number of uncorrectable FEC codewords */
	unsigned long long fec_codewords_uncorr;
	/** Number of total received frames */
	unsigned long long total_frames;
	/** Number FEC errored seconds */
	unsigned long long fec_sec;
	/** Number Idle GEM errors */
	unsigned long long gem_idle;
	/** Number of downstream synchronization losses */
	unsigned long long lods_events;
	/** Dying Gasp activation time, given in multiples of 125 us */
	unsigned long long dg_time;
};

/** Structure to collect counters related to XGEM TC reception and transmission.
 *  Used by \ref fapi_pon_xgtc_counters_get.
*/
struct intel_px126_pon_xgtc_counters {
	/** Uncorrected PSBd HEC errors */
	unsigned long long psbd_hec_err_uncorr;
	/** PSBd HEC errors.
	 *  This is the number of HEC errors detected in any of the fields
	 *  of the downstream physical sync block.
	 */
	unsigned long long psbd_hec_err_corr;
	/** Uncorrected FS HEC errors */
	unsigned long long fs_hec_err_uncorr;
	/** Corrected FS HEC errors */
	unsigned long long fs_hec_err_corr;
	/** Lost words due to uncorrectable HEC errors.
	 *  This is the number of four-byte words lost because of an
	 *  XGEM frame HEC error. In general, all XGTC payload following
	 *  the error it lost, until the next PSBd event.
	 */
	unsigned long long lost_words;
	/** PLOAM MIC errors.
	 *  This is the number of received PLOAM messages with an invalid
	 *  Message Integrity Check (MIC) field.
	 */
	unsigned long long ploam_mic_err;

	/** XGEM HEC Error count */
	unsigned long long xgem_hec_err_corr;

	/** Discarded XGEM frames */
	unsigned long long xgem_hec_err_uncorr;
};

/** Downstream FEC counters for ITU PON operation modes.
 *  Used by \ref fapi_pon_fec_counters_get.
 */
struct intel_px126_pon_fec_counters {
	/** Corrected bytes.
	 *  This is the number of bytes that could be corrected by the
	 *  downstream FEC process.
	 */
	unsigned long long bytes_corr;
	/** Corrected code words.
	 *  This is the number of FEC code words which could be corrected.
	 */
	unsigned long long words_corr;
	/** Uncorrectable code words.
	 *  This is the number of received FEC code words which could not
	 *  be corrected.
	 */
	unsigned long long words_uncorr;
	/** Total number of code words.
	 *  This is the total number of received FEC code words.
	 */
	unsigned long long words;
	/** FEC errored seconds.
	 *  Number of one-second intervals in which at least one
	 *  uncorrectable FEC error has been observed.
	 */
	unsigned long long seconds;
};

/** Allocation-specific counters.
 *  Used by \ref fapi_pon_alloc_counters_get.
 */
struct intel_px126_pon_alloc_counters {
	/** Allocations received.
	 *  This is the number of individual allocations that have been
	 *  received for a given allocation (T-CONT).
	 */
	unsigned long long allocations;
	/** GEM idle frames.
	 *  This is the number of GEM idle frames that have been sent
	 *  within the selected allocation (T-CONT).
	 *  It represents the available but unused upstream bandwidth.
	 */
	unsigned long long idle;
	/** Upstream average bandwidth.
	 *  This is the assigned upstream bandwidth, averaged over 1 second.
	 *  The value is given in units of bit/s.
	 */
	unsigned long long us_bw;
};

/** PLOAM downstream message counters.
 *  The available PLOAM downstream message types depend on the operation
 *  mode (GPON, XG-PON, XGS-PON, or NG-PON2).
 *  The counter increment rate is 2/125 us or slower.
 *  Counters for unused messages are always reported as 0.
 *  Used by \ref fapi_pon_ploam_ds_counters_get.
 */
struct intel_px126_pon_ploam_ds_counters {
	/** Upstream overhead message (GPON). */
	unsigned long long us_overhead;
	/** Assign ONU ID message (GPON, XG-PON, NG-PON2, XGS-PON). */
	unsigned long long assign_onu_id;
	/** Ranging time message (GPON, XG-PON, NG-PON2, XGS-PON). */
	unsigned long long ranging_time;
	/** Deactivate ONU ID message (GPON, XG-PON, NG-PON2, XGS-PON). */
	unsigned long long deact_onu;
	/** Disable serial number message (GPON, XG-PON, NG-PON2, XGS-PON). */
	unsigned long long disable_ser_no;
	/** Encrypted port ID message (GPON). */
	unsigned long long enc_port_id;
	/** Request password message (GPON). */
	unsigned long long req_passwd;
	/** Assign allocation ID message (GPON, XG-PON, NG-PON2, XGS-PON). */
	unsigned long long assign_alloc_id;
	/** No message (GPON). */
	unsigned long long no_message;
	/** Popup message (GPON). */
	unsigned long long popup;
	/** Request key message (GPON). */
	unsigned long long req_key;
	/** Configure port ID message (GPON). */
	unsigned long long config_port_id;
	/** Physical Equipment Error (PEE) message (GPON). */
	unsigned long long pee;
	/** Change Power Level (CPL) message (GPON, NG-PON2). */
	unsigned long long cpl;
	/** PON Section Trace (PST) message (GPON). */
	unsigned long long pst;
	/** BER interval message (GPON). */
	unsigned long long ber_interval;
	/** Key switching time message (GPON). */
	unsigned long long key_switching;
	/** Extended burst length message (GPON). */
	unsigned long long ext_burst;
	/** PON ID message (GPON). */
	unsigned long long pon_id;
	/** Swift popup message (GPON). */
	unsigned long long swift_popup;
	/** Ranging adjustment message (GPON). */
	unsigned long long ranging_adj;
	/** Sleep allow message (GPON, XG-PON, NG-PON2, XGS-PON). */
	unsigned long long sleep_allow;
	/** Request registration message (XG-PON, NG-PON2, XGS-PON). */
	unsigned long long req_reg;
	/** Key control message (XG-PON, NG-PON2, XGS-PON). */
	unsigned long long key_control;
	/** Burst profile message (NG-PON2, XGS-PON, XG-PON). */
	unsigned long long burst_profile;
	/** Calibration request message (NG-PON2). */
	unsigned long long cal_req;
	/** Adjust transmitter wavelength message (NG-PON2). */
	unsigned long long tx_wavelength;
	/** Tuning control message with operation code "request" (NG-PON2). */
	unsigned long long tuning_request;
	/** Tuning control message with operation code "complete" (NG-PON2). */
	unsigned long long tuning_complete;
	/** System profile message (NG-PON2). */
	unsigned long long system_profile;
	/** Channel profile message (NG-PON2). */
	unsigned long long channel_profile;
	/** Protection control message (NG-PON2). */
	unsigned long long protection;
	/** Power consumption inquire message (NG-PON2). */
	unsigned long long power;
	/** Rate control message (NG-PON2). */
	unsigned long long rate;
	/** Reset message. */
	unsigned long long reset;
	/** Unknown message. */
	unsigned long long unknown;
	/** Sum of all messages. */
	unsigned long long all;
};

/** PLOAM upstream message counters.
 *  The available PLOAM upstream message types depend on the operation mode
 *  (GPON, XG-PON, XGS-PON, or NG-PON2).
 *  The counter increment rate is 1/125 us or slower.
 *  Counters for unused messages are always reported as 0.
 *  Used by \ref fapi_pon_ploam_us_counters_get.
 */
struct intel_px126_pon_ploam_us_counters {
	/** Serial number ONU message (GPON, XG-PON, NG-PON2, XGS-PON). */
	unsigned long long ser_no;
	/** Password message (GPON). */
	unsigned long long passwd;
	/** Dying Gasp (DG) message (GPON). */
	unsigned long long dying_gasp;
	/** No message (GPON). */
	unsigned long long no_message;
	/** Encryption key message (GPON). */
	unsigned long long enc_key;
	/** Physical Equipment Error (PEE) message (GPON). */
	unsigned long long pee;
	/** PON Section Trace (PST) message (GPON). */
	unsigned long long pst;
	/** Remote Error Indication (REI) message (GPON). */
	unsigned long long rei;
	/** Acknowledge message (GPON, XG-PON, NG-PON2, XGS-PON). */
	unsigned long long ack;
	/** Sleep request message (GPON, XG-PON, NG-PON2, XGS-PON). */
	unsigned long long sleep_req;
	/** Registration message (XG-PON, NG-PON2, XGS-PON). */
	unsigned long long reg;
	/** Key report message (XG-PON, NG-PON2, XGS-PON). */
	unsigned long long key_rep;
	/** Tuning response message (NG-PON2). */
	unsigned long long tuning_resp;
	/** Power consumption report message (NG-PON2). */
	unsigned long long power_rep;
	/** Rate response message (NG-PON2). */
	unsigned long long rate_resp;
	/** Sum of all messages. */
	unsigned long long all;
};

/** Ethernet frame receive or transmit packet and byte counters per GEM port.
 *  Used by \ref fapi_pon_eth_rx_counters_get and
 *  \ref fapi_pon_eth_tx_counters_get.
 */
struct intel_px126_pon_eth_counters {
	/** Ethernet payload bytes. */
	unsigned long long bytes;
	/** Ethernet packets below 64 byte size. */
	unsigned long long frames_lt_64;
	/** Ethernet packets of 64 byte size. */
	unsigned long long frames_64;
	/** Ethernet packets of 65 to 127 byte size. */
	unsigned long long frames_65_127;
	/** Ethernet packets of 128 to 255 byte size. */
	unsigned long long frames_128_255;
	/** Ethernet packets of 256 to 511 byte size. */
	unsigned long long frames_256_511;
	/** Ethernet packets of 512 to 1023 byte size. */
	unsigned long long frames_512_1023;
	/** Ethernet packets of 1024 to 1518 byte size. */
	unsigned long long frames_1024_1518;
	/** Ethernet packets of more than 1518 byte size. */
	unsigned long long frames_gt_1518;
	/** Ethernet packets with incorrect FCS value. */
	unsigned long long frames_fcs_err;
	/** Ethernet payload bytes in packets with incorrect FCS value. */
	unsigned long long bytes_fcs_err;
	/** Ethernet packets which exceed the maximum length. */
	unsigned long long frames_too_long;
};

struct intel_px126_switch_eth_port_counters {
	unsigned long long RxExtendedVlanDiscardPkts;
	unsigned long long MtuExceedDiscardPkts;
	unsigned long long TxUnderSizeGoodPkts;
	unsigned long long TxOversizeGoodPkts;
	unsigned long long RxGoodPkts;
	unsigned long long RxUnicastPkts;
	unsigned long long RxBroadcastPkts;
	unsigned long long RxMulticastPkts;
	unsigned long long RxFCSErrorPkts;
	unsigned long long RxUnderSizeGoodPkts;
	unsigned long long RxOversizeGoodPkts;
	unsigned long long RxUnderSizeErrorPkts;
	unsigned long long RxGoodPausePkts;
	unsigned long long RxOversizeErrorPkts;
	unsigned long long RxAlignErrorPkts;
	unsigned long long RxFilteredPkts;
	unsigned long long Rx64BytePkts;
	unsigned long long Rx127BytePkts;
	unsigned long long Rx255BytePkts;
	unsigned long long Rx511BytePkts;
	unsigned long long Rx1023BytePkts;
	unsigned long long RxMaxBytePkts;
	unsigned long long TxGoodPkts;
	unsigned long long TxUnicastPkts;
	unsigned long long TxBroadcastPkts;
	unsigned long long TxMulticastPkts;
	unsigned long long TxSingleCollCount;
	unsigned long long TxMultCollCount;
	unsigned long long TxLateCollCount;
	unsigned long long TxExcessCollCount;
	unsigned long long TxCollCount;
	unsigned long long TxPauseCount;
	unsigned long long Tx64BytePkts;
	unsigned long long Tx127BytePkts;
	unsigned long long Tx255BytePkts;
	unsigned long long Tx511BytePkts;
	unsigned long long Tx1023BytePkts;
	unsigned long long TxMaxBytePkts;
	unsigned long long TxDroppedPkts;
	unsigned long long TxAcmDroppedPkts;
	unsigned long long RxDroppedPkts;
	unsigned long long RxGoodBytes;
	unsigned long long RxBadBytes;
	unsigned long long TxGoodBytes;
	unsigned long long RxOverflowError;
};


// Prototype
int intel_px126_pm_gem_port_counters_get( unsigned int gem_port_id, struct intel_px126_pon_gem_port_counters *counter);
int intel_px126_pm_gem_all_counters_get(struct intel_px126_pon_gem_port_counters *counter);
int intel_px126_pm_gtc_counters_get( struct intel_px126_pon_gtc_counters *counter);
int intel_px126_pm_xgtc_counters_get( struct intel_px126_pon_xgtc_counters *counter);
int intel_px126_pm_fec_counters_get( struct intel_px126_pon_fec_counters *counter);
int intel_px126_pm_alloc_counters_get( unsigned int alloc_index, struct intel_px126_pon_alloc_counters *counter);
int intel_px126_pm_ploam_ds_counters_get(struct intel_px126_pon_ploam_ds_counters *counter);
int intel_px126_pm_ploam_us_counters_get( struct intel_px126_pon_ploam_us_counters *counter);
int intel_px126_pm_eth_rx_counters_get( unsigned int gem_port_id, struct intel_px126_pon_eth_counters *counter);
int intel_px126_pm_eth_tx_counters_get( unsigned int gem_port_id, struct intel_px126_pon_eth_counters *counter);
int intel_px126_pm_eth_port_counter_get( unsigned int me_id, struct intel_px126_switch_eth_port_counters *counter);
int intel_px126_pm_eth_port_refresh( unsigned int me_id);
// 220809 LEO START added
int intel_px126_pm_ifname_get( unsigned short class_id, unsigned short me_id, char *ifname, int size);
// 220809 LEO END

#endif // End __INTEL_PX126_PM_H__
