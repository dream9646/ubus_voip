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
 * Purpose : 5VT VOIP protocol stack
 * Module  : tdi_api_si3217x/src_0910
 * File    : fv_state_machine.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

#include "tdi_slic.h"
#include "fv_spi_wrapper.h"
#include "fv_tdi_daa.h"
#include "fv_state_machine.h"
#include "proslic.h"
#include "tdi_daa.h"
#ifdef SI3217X
#include "vdaa_registers.h"	//SI32178_FXO
#include "si3217x_registers.h"
#endif
#ifdef SI3226X
#include "si3226x_registers.h"
#endif
#ifdef SI3228X
#include "si3228x_registers.h"
#endif
#include "fv_tdi_log.h"

#define VOLTAGE_LSB 4000 // 931.323 * 2^32 /1000000000
#define LOOPCURRENT_LSB 7198365 // 1.676 * 2^32 / 1000

#define SLIC_RING_PATTERN_COMPLETE 1

#define Dbg_msg_stringify(s) [s] = {s, #s}

extern struct slic_fops_t slic_fops_g;
struct slic_state2str
{
	enum slic_state state;
	const char* str;
};

struct slic_state2str slic_state2str_array[] =
{
	Dbg_msg_stringify(SLIC_STATE_ON_HOOK),
	Dbg_msg_stringify(SLIC_STATE_OFF_HOOK),
	Dbg_msg_stringify(SLIC_STATE_RINGING),
	Dbg_msg_stringify(SLIC_STATE_TRANSITION),
	Dbg_msg_stringify(SLIC_STATE_OFFHOOK_JUDGEMENT)
};
/* _Static_assert needs gcc 4.6.0 */
#if __GNUC__ > 4 || \
	(__GNUC__ == 4 && (__GNUC_MINOR__ > 6 || \
			  (__GNUC_MINOR__ == 6 && \
			    __GNUC_PATCHLEVEL__ > 0)))

_Static_assert ((sizeof(slic_state2str_array) / sizeof(struct slic_state2str)) == SLIC_STATE_MAX,
		"slic_state2str_array member error");
#endif

const char * state_machine_get_slic_state2str(enum slic_state state)
{
	int32_t i = 0;
	for (i = 0; i < SLIC_STATE_MAX && slic_state2str_array[i].state != state; ++i)
		;

	return (i < SLIC_STATE_MAX) ? (slic_state2str_array[i].str) : ("Unknown SLIC state");
}

struct slic_event2str
{
	enum slic_event event;
	const char* str;
};

struct slic_event2str slic_event2str[] =
{
	Dbg_msg_stringify(SLIC_EVENT_ON_HOOK),
	Dbg_msg_stringify(SLIC_EVENT_OFF_HOOK),
	Dbg_msg_stringify(SLIC_EVENT_RINGING_ACTIVE),
	Dbg_msg_stringify(SLIC_EVENT_RINGING_INACTIVE),
	Dbg_msg_stringify(SLIC_EVENT_RTP_OFF_HOOK)
};
/* _Static_assert needs gcc 4.6.0 */
#if __GNUC__ > 4 || \
	(__GNUC__ == 4 && (__GNUC_MINOR__ > 6 || \
			  (__GNUC_MINOR__ == 6 && \
			    __GNUC_PATCHLEVEL__ > 0)))
_Static_assert ((sizeof(slic_event2str) / sizeof(struct slic_event2str)) == SLIC_EVENT_MAX, "slic_event2str member error");
#endif
static const char * get_slic_event2str(enum slic_event event)
{
	int32_t i = 0;
	for (i = 0; i < SLIC_EVENT_MAX && slic_event2str[i].event != event; ++i)
		;

	return (i < SLIC_EVENT_MAX) ? (slic_event2str[i].str) : ("Unknown SLIC event");
}

static enum slic_state slic_unexpected_event_transition(int32_t* ret_event_p, enum slic_event event, enum slic_state current_state)
{
	enum slic_state next_state = current_state;
	*ret_event_p = INVALID_EVENT_NUMBER;
	switch (event) {
	case SLIC_EVENT_ON_HOOK:
		next_state = SLIC_STATE_ON_HOOK;
		if (current_state != SLIC_STATE_ON_HOOK)
			*ret_event_p = ONHOOK_EVENT_NUMBER;
		break;
	case SLIC_EVENT_OFF_HOOK:
	case SLIC_EVENT_RTP_OFF_HOOK:
		next_state = SLIC_STATE_OFF_HOOK;
		if (current_state != SLIC_STATE_OFF_HOOK)
			*ret_event_p = OFFHOOK_EVENT_NUMBER;
		break;
	case SLIC_EVENT_RINGING_ACTIVE:
	case SLIC_EVENT_RINGING_INACTIVE:
		next_state = SLIC_STATE_RINGING;
		break;
	default:
		next_state = current_state;
	}

	LOGE("TDI: Unexpected event [%s] previous [%s] next [%s]",
			get_slic_event2str(event), state_machine_get_slic_state2str(current_state),
			state_machine_get_slic_state2str(next_state));

	return next_state;
}
/*********************************************** SLIC Function ****************************************************/

static inline int ts_diff_us(struct timespec t0, struct timespec t1) {
	return ((t1.tv_sec - t0.tv_sec) * 1000000) + ((t1.tv_nsec-t0.tv_nsec+500)/1000);
}
static inline int ts_diff_ms(struct timespec t0, struct timespec t1) {
	return ((t1.tv_sec - t0.tv_sec) * 1000 + (t1.tv_nsec-t0.tv_nsec+500000)/1000000);
}
static inline int ts_diff_sec(struct timespec t0, struct timespec t1) {
	int sec_diff = t1.tv_sec - t0.tv_sec;
	int ms_diff = (t1.tv_nsec-t0.tv_nsec)/1000000;
	if (ms_diff > 500)
		sec_diff++;
	else if (ms_diff < -500)
		sec_diff--;
	return sec_diff;
}

static inline int is_ts_zero(struct timespec t) {
	return (t.tv_sec == 0 && t.tv_nsec == 0);
}


static void dp_judgement(struct slic_dp *dp)
{
	uint64_t cycle_time;
	if (dp->parm.judgement_opt == FV_TDI_DP_JUDGEMENT_MAKE_BREAK) {
		if (dp->digit_counter > 1){
			if((dp->make_time < dp->parm.maketime_min) ||
					(dp->make_time > dp->parm.maketime_max)){
				LOGE("make time (%llu us) is out of range [%u ~ %u] us", dp->make_time
						, dp->parm.maketime_min, dp->parm.maketime_max);
				dp->detect_ratio_error++;
			} else {
				LOGV("make time (%llu us)", dp->make_time);
			}
		}
		/* We need to calibrate pulse detect time for 1st pulse break time of DP20 */
		if(dp->digit_counter == 1 && dp->break_time < dp->parm.breaktime_min)
			dp->break_time += dp->parm.firstpulse_detection_calibrate_time;

		/* If this pulse's break time is out of range, then "ratio error number" increased */
		if((dp->break_time < dp->parm.breaktime_min) ||
		   (dp->break_time > dp->parm.breaktime_max)) {
			LOGE("break time (%llu us) is out of range [%u ~ %u] us", dp->break_time,
				dp->parm.breaktime_min, dp->parm.breaktime_max);
			dp->detect_ratio_error++;
		} else {
			LOGV("break time (%llu us)", dp->break_time);
		}
	} else if (dp->parm.judgement_opt == FV_TDI_DP_JUDGEMENT_CYCLE) {
		if (dp->digit_counter > 1) {
			cycle_time = dp->make_time + dp->break_time;
			if((cycle_time < dp->parm.cycletime_min) ||
			   (cycle_time > dp->parm.cycletime_max)) {
				LOGE("cycle time %llu us [make %llu + break %llu] is out of range [%u to %u] us",
						cycle_time, dp->make_time, dp->break_time,
						dp->parm.cycletime_min,
						dp->parm.cycletime_max);
				dp->detect_ratio_error++;
			} else {
				LOGV("cycle time (%llu us), break time (%llu us)", cycle_time, dp->break_time);
			}
		}
	}
}

static void dp_speed_judgement(struct slic_dp *dp)
{
	if (dp->break_time > SLIC_DP_SPEED_TH) {
		if (dp->det_speed_factor >= 0) {
			dp->det_speed = FV_TDI_PULSEDIAL_10PPS;
			dp->det_speed_factor++;
			LOGD("DP detected speed is 10 pps");
		} else {
			dp->det_speed = FV_TDI_PULSEDIAL_20PPS;
			LOGI("DP break time = %lld us but judge to 20pps by factor", dp->break_time);
		}
	} else {
		if (dp->det_speed_factor <= 0) {
			dp->det_speed = FV_TDI_PULSEDIAL_20PPS;
			dp->det_speed_factor--;
			LOGD("DP detected speed is 20 pps");
		} else {
			dp->det_speed = FV_TDI_PULSEDIAL_10PPS;
			LOGI("DP break time = %lld us but judge to 10pps by factor", dp->break_time);
		}
	}
}

static int distinctive_ring_inactive_handler(Slic_state *s, fv_tdi *tdi)
{
	int ret = 0;
	if(s->cadence_repeat_count == 0 ) {
		s->cadence_num++;
		/* is last cadence */
		if (s->cadence_num >= FV_TDI_MAX_RINGING_CANDENCE ||
				s->ring_param.cadence[s->cadence_num].on_time_ms == 0) {
			if (s->pattern_repeat_count == 0) {
				/* last cadence and no need to repeat pattren */
				slic_fops_g.set_ring_osc_on_timers(tdi, 0);
				SLIC_STATUS_SET_LAST_RING_CADENCE(s);
				/* flush register of interrupt bit */
				slic_fops_g.get_ringing_state(tdi);
			} else {
				if (s->pattern_repeat_count > 0)
					s->pattern_repeat_count--;
				s->cadence_num = 0;
				/* Set to first cadence off timer be set at inactive expired */
				s->cadence_repeat_count = s->ring_param.cadence[0].repeat_times;
				slic_fops_g.set_ring_osc_on_timers(tdi, s->ring_param.cadence[0].on_time_ms);
			}

			ret = SLIC_RING_PATTERN_COMPLETE;
		} else {
			/* update next cadence */
			s->cadence_repeat_count = s->ring_param.cadence[s->cadence_num].repeat_times;
			slic_fops_g.set_ring_osc_on_timers(tdi, s->ring_param.cadence[s->cadence_num].on_time_ms);
		}
	} else if (s->cadence_repeat_count > 0) {
		s->cadence_repeat_count--;
	}
	SLIC_STATUS_SET_RINGING_INACTIVE(s);
	return ret;
}

/*********************************************** SLIC Function ****************************************************/ 
enum slic_state state_machine_get_slic_state(fv_tdi *tdi)
{
	if (tdi != NULL && tdi->priv != NULL)
		return ((Slic_state *)tdi->priv)->previous_state;
	else
		return -1;
}


/**
 * @fn unsigned int fv_state_machine_slic(int fd)
 * @brief this function uses to check the hook state change. Only 
 *        if hook state change, this function will return an event
 *        number, otherwise it will return invalid event number  
 * @param  fd is SPI file descriptor
 * @param  flash_time is the time to decide whether the hook state is
 *                    flash hook or not  
 * @return  void
 */
int fv_state_machine_slic(Slic_state *s, fv_tdi *tdi)
{
	unsigned char linefeed_state, lcrrtp;
	long time_diff;
	struct timespec ts;
	enum slic_event event;
	int ret = INVALID_EVENT_NUMBER;
#define RING_TRIP_REACTION_TIME 230000L //us
	/* prevent to polling at TRANSITIONSTATE state of SLIC when ring-trip occurs for 230ms*/
	if(s->rtp_time.tv_sec != 0) {
		clock_gettime(CLOCK_MONOTONIC, &ts);
		if(ts_diff_us(s->rtp_time, ts) > RING_TRIP_REACTION_TIME) {
			s->rtp_time.tv_sec = 0;
		} else {
			return INVALID_EVENT_NUMBER;
		}
	}

	linefeed_state = ctrl_ReadRegisterWrapper((void *)(int)tdi->spi_fd, tdi->port, LINEFEED);
	lcrrtp = ctrl_ReadRegisterWrapper((void *)(int)tdi->spi_fd, tdi->port, LCRRTP);
	/* Steve, 20121107, when SLIC linefeed enter OPEN state due to voltage protection, state machine
	   need to revert to original voltage in next polling */
	if(linefeed_state == LF_OPEN){
		/* Please enable IRQEN3 */
		LOGAE("SLIC linefeed enter OPEN STATE HVIC Alarm 0x%x",
			ctrl_ReadRegisterWrapper((void *)(int)tdi->spi_fd, tdi->port, IRQ3));
		usleep(50000);  /* delay 50ms for doing next linefeed control */
		if(s->polarity_type == FV_TDI_VOLTAGE_POLARITY_POSITIVE)
			ctrl_WriteRegisterWrapper((void *)(int)tdi->spi_fd, tdi->port, LINEFEED, LF_FWD_ACTIVE);
		else if(s->polarity_type == FV_TDI_VOLTAGE_POLARITY_NEGATIVE)
			ctrl_WriteRegisterWrapper((void *)(int)tdi->spi_fd, tdi->port, LINEFEED, LF_REV_ACTIVE);
	}

	/* Determine current state */
	if(lcrrtp == 0xFF){
		/* Steve, 20121003, this case used for SLIC hardware broken (0xFF) , need to view it
		   as ONHOOK */
		event = SLIC_EVENT_ON_HOOK;
	} else if(((lcrrtp &0x01) == 0x01) &&
		   (SLIC_STATUS_IS_RINGING_START(s)) &&
		   (s->previous_state == SLIC_STATE_RINGING)) {  // Ringtrip only judge in ringing state
		LOGD("TDI: Ringtrip, LCRRTP(0x%x)", lcrrtp);
		event = SLIC_EVENT_RTP_OFF_HOOK;
		clock_gettime(CLOCK_MONOTONIC, &s->rtp_time);
	} else if(((lcrrtp &0x02)>>1)==PROSLIC_OFFHOOK){
		if (s->previous_state == SLIC_STATE_RINGING) {
			LOGD("TDI: Loop Closure, LCRRTP(0x%x)", lcrrtp);
		}
		event = SLIC_EVENT_OFF_HOOK;
	} else if(linefeed_state == 0x44) {
		event = SLIC_EVENT_RINGING_ACTIVE;
	} else if ((linefeed_state == 0x24)||(linefeed_state == 0x64) ||
		   (linefeed_state == 0x14)||(linefeed_state == 0x54)){
		event = SLIC_EVENT_RINGING_INACTIVE;
	} else {
		event = SLIC_EVENT_ON_HOOK;
	}
#if 0
  	if((linefeed_state == 0x44)||(linefeed_state == 0x24)||(linefeed_state == 0x64)){
    		if((lcrrtp&0x01)==0x01){
      			s->current_state = OFF_HOOK;
      			clock_gettime(CLOCK_MONOTONIC, &s->rtp_time);
      			ctrl_WriteRegisterWrapper((void *)(int)tdi->spi_fd, tdi->port, LCRRTP, lcrrtp &0xfe);
    		}
    		else if(((lcrrtp &0x02)>>1)==PROSLIC_OFFHOOK){
      			s->current_state = OFF_HOOK;
    		}
    		else{
      			s->current_state = RINGING;
    		}
  	}
  	else{  
	/* Steve, 20121003, this case used for SLIC hardware broken (0xFF) , need to view it
   		as ONHOOK */
    		if(lcrrtp == 0xFF){
      			s->current_state = ON_HOOK;
    		}
 	/* Steve, 20121031, when linefeed isn't on ringing state, no need to judge "ring trip
           offhook", this can avoid return "offhook" and "onhook" event when VOIP process initial */
    		else{
			if(((lcrrtp &0x02)>>1)==PROSLIC_OFFHOOK){
        			s->current_state = OFF_HOOK;
      			}
			else{
        			s->current_state = ON_HOOK;
      			}
    		}
  	}
#endif
	/* State machine: based on previous state */

	if(s->previous_state == SLIC_STATE_ON_HOOK) {
		if(event == SLIC_EVENT_OFF_HOOK) {
			if(s->offhook_judgement_threshold > 0){
				clock_gettime(CLOCK_MONOTONIC, &s->off_hook_time);
				s->previous_state = SLIC_STATE_OFFHOOK_JUDGEMENT;
				ret = PRE_OFFHOOK_EVENT_NUMBER;
			}else{
				s->previous_state = SLIC_STATE_OFF_HOOK;
				ret = OFFHOOK_EVENT_NUMBER;
			}

		} else if (event == SLIC_EVENT_RINGING_ACTIVE) {
			s->previous_state = SLIC_STATE_RINGING;
			SLIC_STATUS_SET_FIRST_RING_ACTIVE(s);
			SLIC_STATUS_SET_RINGING_ACTIVE(s);
		} else if (event == SLIC_EVENT_RINGING_INACTIVE) {
			s->previous_state = SLIC_STATE_RINGING;
			SLIC_STATUS_SET_FIRST_RING_ACTIVE(s);
			SLIC_STATUS_SET_RINGING_INACTIVE(s);
		} else if (event == SLIC_EVENT_ON_HOOK) {
			s->previous_state = SLIC_STATE_ON_HOOK;
		} else {
			s->previous_state = slic_unexpected_event_transition(&ret, event, s->previous_state);
		}

	} else if(s->previous_state == SLIC_STATE_OFF_HOOK) {
		if(event == SLIC_EVENT_ON_HOOK) {
			clock_gettime(CLOCK_MONOTONIC, &s->on_hook_time);
			s->previous_state = SLIC_STATE_TRANSITION;

			if(s->dp.digit_counter != 0){
				s->dp.make_time = ts_diff_us(s->dp.timer.init_make, s->on_hook_time);
			}
			s->dp.timer.init_break = s->on_hook_time;
		} else if(event == SLIC_EVENT_OFF_HOOK) {

			if(s->dp.enable == 1 && s->dp.digit_counter != 0) {
				clock_gettime(CLOCK_MONOTONIC, &ts);
				if(ts_diff_ms(s->dp.timer.pause, ts) >= s->dp.parm.interdigit_interval_ms) {
					/* For finish detecting a complete pulse, if "ratio error number > 1" then retuen pulse error event otherwise retuen real pulse number */

					LOGD("DP[%d] detected at OFFHOOK STATE", s->dp.digit_counter);
					if(s->dp.detect_ratio_error > 0){
						LOGD("OFFHOOK state, pulse error %d times", s->dp.detect_ratio_error);
						s->dp.detect_ratio_error = 0;
						ret = PULSE_TIME_DETECT_ERROR_EVENT_NUMBER;
					}else{
						ret = s->dp.digit_counter % 10;
						dp_speed_judgement(&(s->dp));
					}
					s->dp.digit_counter = 0;
				}
			}
			s->previous_state = SLIC_STATE_OFF_HOOK;
		} else {
			s->previous_state = slic_unexpected_event_transition(&ret, event, s->previous_state);
		}
	} else if(s->previous_state == SLIC_STATE_RINGING) {
		if(event == SLIC_EVENT_OFF_HOOK) {
			SLIC_STATUS_UNSET_FIRST_RING_ACTIVE(s);
			if(s->offhook_judgement_threshold > 0){
				clock_gettime(CLOCK_MONOTONIC, &s->off_hook_time);
				s->previous_state = SLIC_STATE_OFFHOOK_JUDGEMENT;
				ret = PRE_OFFHOOK_EVENT_NUMBER;
			}else{
				s->previous_state = SLIC_STATE_OFF_HOOK;
				ret = OFFHOOK_EVENT_NUMBER;
			}
		} else if (event == SLIC_EVENT_ON_HOOK) {
			SLIC_STATUS_UNSET_FIRST_RING_ACTIVE(s);
			s->previous_state = SLIC_STATE_ON_HOOK;
			if(SLIC_STATUS_IS_RINGING_START(s)){
				/* Polling interval to long or system crimp */
				LOGW("TDI: Reringing previous state [%s] and receive event [%s]",
					state_machine_get_slic_state2str(s->previous_state), get_slic_event2str(event));
				slic_fops_g.start_ring(tdi);
			}
		} else if (event == SLIC_EVENT_RINGING_ACTIVE) {
			if (SLIC_STATUS_IS_RINGING_INACTIVE(s)) {
				clock_gettime(CLOCK_MONOTONIC, &ts);
				if (!SLIC_STATUS_IS_FIRST_RING_ACTIVE(s)) {
					LOGD("CH[%d] ring off during %d ms", tdi->port, ts_diff_ms(s->ring_time, ts));
					s->ring_time = ts;
				}
				slic_fops_g.set_ring_osc_off_timers(tdi, s->ring_param.cadence[s->cadence_num].off_time_ms);
				SLIC_STATUS_SET_RINGING_ACTIVE(s);
			}
		} else if (event == SLIC_EVENT_RINGING_INACTIVE) {
			if (SLIC_STATUS_IS_LAST_RING_CADENCE(s) &&
			    slic_fops_g.get_ringing_state(tdi) == (SLIC_RINGING_INACTIVE_EXPIRED | SLIC_RINGING_ACTIVE_EXPIRED)) {
				LOGI("TDI: CH[%d] Stop Ringing", tdi->port);
				/* After tone stop should back to previous polarity type */
				tdi_slic_tonestop(tdi, 0, s->polarity_type);
				s->previous_state = SLIC_STATE_ON_HOOK;
			} else if (SLIC_STATUS_IS_RINGING_ACTIVE(s)) {
				int is_pattern_complete = 0;
				is_pattern_complete = distinctive_ring_inactive_handler(s, tdi);
				s->previous_state = SLIC_STATE_RINGING;
				clock_gettime(CLOCK_MONOTONIC, &ts);
				LOGD("CH[%d] ring on during %d ms", tdi->port, ts_diff_ms(s->ring_time, ts));
				s->ring_time = ts;

				if (is_pattern_complete == SLIC_RING_PATTERN_COMPLETE && SLIC_STATUS_IS_FIRST_RING_ACTIVE(s)) {
					SLIC_STATUS_UNSET_FIRST_RING_ACTIVE(s);
					ret = FIRST_RINGING_COMPLETE_EVENT_NUMBER;
				}
			}
		} else if (event == SLIC_EVENT_RTP_OFF_HOOK) {
			SLIC_STATUS_UNSET_FIRST_RING_ACTIVE(s);
			s->previous_state = SLIC_STATE_OFF_HOOK;
			ret = OFFHOOK_EVENT_NUMBER;
		} else {
			s->previous_state = slic_unexpected_event_transition(&ret, event, s->previous_state);
		}
	}else if(s->previous_state == SLIC_STATE_OFFHOOK_JUDGEMENT){
		if(event == SLIC_EVENT_OFF_HOOK) {
			clock_gettime(CLOCK_MONOTONIC, &ts);
			time_diff = ts_diff_us(s->off_hook_time, ts);

			if(time_diff >= s->offhook_judgement_threshold){
				SLIC_STATUS_SET_RINGING_STOP(s);
				s->previous_state = SLIC_STATE_OFF_HOOK;
				ret = OFFHOOK_EVENT_NUMBER;
			} else {
				s->previous_state = SLIC_STATE_OFFHOOK_JUDGEMENT;
			}
		} else if (event == SLIC_EVENT_ON_HOOK) {
			if(SLIC_STATUS_IS_RINGING_START(s)) {
				SLIC_STATUS_UNSET_FIRST_RING_ACTIVE(s);
				slic_fops_g.start_ring(tdi);
				/* Re-ringing and avoid to repeat first ringing complete */
				s->previous_state = SLIC_STATE_RINGING;
			} else {
				s->previous_state = SLIC_STATE_ON_HOOK;
				ret = PRE_ONHOOK_EVENT_NUMBER;
			}
		} else {
			s->previous_state = slic_unexpected_event_transition(&ret, event, s->previous_state);
		}
	} else { // SLIC_STATE_TRANSITION
		clock_gettime(CLOCK_MONOTONIC, &ts);

		time_diff = ts_diff_us(s->on_hook_time, ts);

		if (event == SLIC_EVENT_ON_HOOK) {
			if(time_diff >= s->upper_flashhooktime) {
				LOGD("Onhook detected, onhook time (%ld us), upper time (%ld us)", time_diff, s->upper_flashhooktime);
				s->previous_state = SLIC_STATE_ON_HOOK;
				s->dp.digit_counter = 0;
				s->dp.det_speed_factor = 0;
				ret = ONHOOK_EVENT_NUMBER;
			} else {
				s->previous_state = SLIC_STATE_TRANSITION;
			}
		} else if (event == SLIC_EVENT_OFF_HOOK){
			s->previous_state = SLIC_STATE_OFF_HOOK;
			s->on_hook_time = ts;
			if(s->dp.enable == 1 && time_diff <= s->lower_flashhooktime) {
				if(s->dp.digit_counter == 0 || ts_diff_ms(s->on_hook_time, ts) < s->dp.parm.interdigit_interval_ms) {

					s->dp.timer.pause = ts;
					s->dp.digit_counter++;

					s->dp.timer.init_make = ts;
					s->dp.break_time = ts_diff_us(s->dp.timer.init_break, ts);
					if(s->dp.parm.judgement_opt != FV_TDI_DP_JUDGEMENT_DISABLE){
						dp_judgement(&s->dp);
					}
					s->dp.det_speed = FV_TDI_PULSEDIAL_UNKNOW;

				} else {
					LOGD("DP[%d] detected at TRANSITION STATE", s->dp.digit_counter);
					if(s->dp.detect_ratio_error > 0){
						LOGD("pulse error %d times", s->dp.detect_ratio_error);
						s->dp.detect_ratio_error = 0;

						ret = PULSE_TIME_DETECT_ERROR_EVENT_NUMBER;
					}else{
						ret = s->dp.digit_counter % 10;
						dp_speed_judgement(&(s->dp));
					}
					s->dp.digit_counter = 0;
				}
			} else if(time_diff > s->lower_flashhooktime && time_diff < s->upper_flashhooktime) {
				LOGD("Flash-hook detected, flashhook time (%ld us), lower time (%ld us), upper time (%ld us)",
				      time_diff, s->lower_flashhooktime, s->upper_flashhooktime);
				ret = FLASHHOOK_EVENT_NUMBER;
			} else {
				LOGD("flashhook time (%ld us) >= flashhook upper time (%ld us)\n"
				     "Treat the condition that received upper flash timeout and offhook event"
				     " at the same polling period as flashhook.\n"
				     "There hypothesis the offhook event occured before upper flash timeout",
				     time_diff, s->upper_flashhooktime);
				ret = FLASHHOOK_EVENT_NUMBER;
			}
		} else {
			s->previous_state = slic_unexpected_event_transition(&ret, event, s->previous_state);
		}
	}
	return ret;
}

/*********************************************** DAA Function ****************************************************/

#define DAA_LOG_FILE	"/var/run/voip/log/daa.log"
#define DAA_CLI_FILE	"/tmp/daa.cli"
struct logm_t *lm_daa;

#define DBGMASK_EVENT		(1<<0)
#define DBGMASK_HOOKSTATE	(1<<1)
#define DBGMASK_RING		(1<<2)
#define DBGMASK_REVERSAL	(1<<3)
#define DBGMASK_VOLTAGE		(1<<4)
#define DBGMASK_CURRENT		(1<<5)
#define DBGMASK_REGISTER	(1<<6)
#define DBGMASK_CLI		(1<<7)
#define DBGMASK_ALL_ON_EVENT	(1<<8)

//#define DBGMASK_DBG_DEFAULT	(DBGMASK_EVENT)
#define DBGMASK_DBG_DEFAULT	0
//#define DBGMASK_LOG_DEFAULT	(DBGMASK_EVENT|DBGMASK_HOOKSTATE|DBGMASK_RING|DBGMASK_REVERSAL|DBGMASK_VOLTAGE|DBGMASK_CURRENT|DBGMASK_REGISTER|DBGMASK_CLI)
#define DBGMASK_LOG_DEFAULT	0x0

typedef struct
{
	// register value from hardware ///////////////////////////////
	unsigned char ctrl2;		// R2
	unsigned char intr_mask;	// R3
	unsigned char intr_source;	// R4

	unsigned char fxo_ctrl1;	// R5
	unsigned char fxo_ctrl2;	// R6
	unsigned char fxo_ctrl5;	// R31

	unsigned char i18n_ctrl1;	// R16
	unsigned char i18n_ctrl2;	// R17
	unsigned char i18n_ctrl3;	// R18
	unsigned char i18n_ctrl4;	// R19

	unsigned char ring_ctrl1;	// R22
	unsigned char ring_ctrl2;	// R23
	unsigned char ring_ctrl3;	// R24

	unsigned char linedev_status;	// R12
	unsigned char dc_termination;	// R26
	unsigned char loop_current;	// R28
	unsigned char line_voltage;	// R29

	unsigned char cvt_threshold;	// R43
	unsigned char cvt_intrctrl;	// R44
	unsigned char spark_quench;	// R59

	// fake the line voltage if (fake_voltage_min || fake_voltage_max)
	int fake_voltage_min;
	int fake_voltage_max;
} Daa_reg;

// KEVIN
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static int
str2argv(char *str, char *argv[], int argv_size)
{
	static char buff[1024];
	char *p = buff;
	int i = 0;

	strncpy(buff, str, 1024);
	while (p != NULL) {
		argv[i] = strsep(&p, " \t");
		if (strlen(argv[i]) > 0) {
			i++;
			if (i == argv_size - 1)
				break;
		}
	}
	argv[i] = 0;
	return i;
}

static int
str2val(char *str)
{
	int val;
	if (str[0] == 'x') {
		sscanf(str + 1, "%x", &val);
	} else if (str[0] == '0' && str[1] == 'x') {
		sscanf(str + 2, "%x", &val);
	} else {
		sscanf(str, "%d", &val);
	}
	return val;
}

static int
util_logprintf(char *logfile, char *fmt, ...)
{
	va_list ap;
	FILE *fp;
	int ret;

	if ((fp = fopen(logfile, "a")) == NULL)
		return (-1);

	va_start(ap, fmt);
	ret = vfprintf(fp, fmt, ap);
	va_end(ap);

	if (fclose(fp) == EOF)
		return (-2);	/* flush error? */

	return ret;
}
static char dbg_buff[4096];
static char log_buff[4096];
static char *dbg_ptr;
static char *log_ptr;
static void
dbg_log_buff_init(void)
{
	dbg_buff[0] = 0;
	dbg_ptr = dbg_buff;
	log_buff[0] = 0;
	log_ptr = log_buff;
}
static void
dbg_log_buff_add_line(unsigned int enable_mask, unsigned int dbgmask, unsigned int logmask, char *line)
{
	if (dbgmask & enable_mask)
		dbg_ptr += sprintf(dbg_ptr, "%s", line);
	if (logmask & enable_mask)
		log_ptr += sprintf(log_ptr, "%s", line);
}
static void
dbg_log_buff_flush_print(unsigned int dbg_enable, unsigned int log_enable)
{
	if (dbg_ptr != dbg_buff && dbg_enable)
		LOGF("%s\n", dbg_buff);
	if (log_ptr != log_buff && log_enable)
		logm_printf(lm_daa, LOG_CRIT, "%s\n", log_buff);
	dbg_log_buff_init();
}

static char *
timestamp2str(struct timespec *t)
{
	static char str_wheel[4][64];	// wheel for reentry
	static int index = 0;
	char *str = str_wheel[index%4];
	sprintf(str, "%ld.%03ld", t->tv_sec, t->tv_nsec/1000000);
	index++;
	return str;
}

char *hook_state_str[] = {
	"INIT",
	"DAA_ONHOOK_HANDSET_ONHOOK",
	"DAA_ONHOOK_HANDSET_OFFHOOK",
	"DAA_OFFHOOK_HANDSET_ONHOOK",
	"DAA_OFFHOOK_HANDSET_OFFHOOK",
	"DROPOUT"
};

char *daa_event_str[] = {
	"RING_START",
	"RING_STOP",
	"REVERSAL",
	"REVERSAL_TIMEOUT",
	"CONNECT",
	"DISCONNECT",
	"DAA_ONHOOK",
	"DAA_OFFHOOK",
	"HANDSET_ONHOOK",
	"HANDSET_OFFHOOK",
};

static struct timespec zero_timestamp = {0};

// return the loop current, unit 1.1ma
// when phone set is not connected, no capacitor effect:
// a. wait_ms must >=2 for this function to work properly
// when phone set is connected, because of the capacitor effect:
// a. wait_ms must >=20 to detect the line disconnect (which means the capacitor discharge needa 20ms when pstn is disconnected)
// b. wait_ms must >=128 to get the correct loop current (which means the capacitor discharge needa 128ms when pstn is connected)
#if 0
unsigned int
fv_state_machine_daa_get_loop_current(Daa_state *s, int wait_ms)
{
	unsigned char daa_reg_fxo_ctrl1 = tdi_daa_ctrl_get(s->fd, 5);	// b0: OH(offhook)
	unsigned char daa_reg_loop_current;

        if (daa_reg_fxo_ctrl1 & 0x1) {				// DAA offhook enabled
                // reg28(loop current): unit 1.1ma
                daa_reg_loop_current = tdi_daa_ctrl_get(s->fd, 28);
	} else {
                tdi_daa_ctrl_set(s->fd, 17, 0x20); 		// reg17(Internation ctrl2): disable offhook dc calibration
		tdi_daa_ctrl_set(s->fd, 31, 0xD1); 		// reg31(fxo ctrl5): set fast offhook selection from 128ms->8ms
                tdi_daa_ctrl_set(s->fd, 5, 0x01); 		// reg5(fxo ctrl1): set daa offhook, in order to measure loop current

                // because daa still need 8ms to check loop current
                usleep(wait_ms*1000);
                // reg28(loop current): unit 1.1ma
                daa_reg_loop_current = tdi_daa_ctrl_get(s->fd, 28);

                tdi_daa_ctrl_set(s->fd, 5, 0x08); 		// reg5: set daa back to onhook
                tdi_daa_ctrl_set(s->fd, 17, 0x00); 		// reg17: enable offhook adc calibration
		tdi_daa_ctrl_set(s->fd, 31, 0xA1); 		// reg31: set fast OFFHOOK selection from 8ms back to 128ms
	}
	return daa_reg_loop_current;
}
#else
unsigned int
fv_state_machine_daa_get_loop_current(Daa_state *s, int wait_ms)
{
	unsigned char daa_reg_fxo_ctrl1 = tdi_daa_ctrl_get(s->fd, 5);	// b0: OH(offhook)
	unsigned char daa_reg_loop_current = 0;
        if (daa_reg_fxo_ctrl1 & 0x1) {				// DAA offhook enabled
                // reg28(loop current): unit 1.1ma
                daa_reg_loop_current = tdi_daa_ctrl_get(s->fd, 28);
	} else {
		unsigned char daa_reg_i18n_ctrl2 = tdi_daa_ctrl_get(s->fd, 17);
		unsigned char daa_reg_resister_cal = tdi_daa_ctrl_get(s->fd, 25);
		unsigned char daa_reg_fxo_ctrl5 = tdi_daa_ctrl_get(s->fd, 31);
		// refer to Si3050+Si3011/18/19 section 5.6(p26) 5.25.2(p36):
		// a. si3050<->line-side-dev communication(250us)
		// b. fast offhook select (8ms..512ms -> 8ms)
		// c. adc auto calibration (256ms -> disabled)
		// d. resister calibration (17ms -> disabled)
		// e. FIR filter (1.5ms)
		// so we still 10ms (250us + 8ms + 1.5ms) for daa offhook to be stable
		tdi_daa_ctrl_set(s->fd, 17, daa_reg_i18n_ctrl2|(1<<5));	// bit 5 to disable adc auto calibration (256ms)
		tdi_daa_ctrl_set(s->fd, 25, daa_reg_resister_cal|(1<<5)); // bit 5 to disable resister calicration (17ms)
		tdi_daa_ctrl_set(s->fd, 31, daa_reg_fxo_ctrl5|(3<<5));	// bit 5,6 for fast offhook select, default 01:128ms, 11:8ms
		tdi_daa_ctrl_set(s->fd, 5, daa_reg_fxo_ctrl1|1);	// bit 0 to set daa offhook to measure loop current

                //usleep(10*1000);
                usleep(wait_ms*1000);

                // reg28(loop current): unit 1.1ma
                daa_reg_loop_current = tdi_daa_ctrl_get(s->fd, 28);

		// restore everything
		tdi_daa_ctrl_set(s->fd, 5, daa_reg_fxo_ctrl1);
		tdi_daa_ctrl_set(s->fd, 17, daa_reg_i18n_ctrl2);
		tdi_daa_ctrl_set(s->fd, 25, daa_reg_resister_cal);
		tdi_daa_ctrl_set(s->fd, 31, daa_reg_fxo_ctrl5);
	}
	return daa_reg_loop_current;
}
#endif

static int
sprintf_daa_registers(char *buff, Daa_reg *daa_reg)
{
	char *str = buff;
	int v = daa_reg->line_voltage;
	if (v > 127)
		v -= 256;

	// register summary
	str += sprintf(str, "ctrl2=0x%02x, intrmask=0x%02x, intrsrc=0x%02x\n",
		daa_reg->ctrl2, daa_reg->intr_mask, daa_reg->intr_source);
	str += sprintf(str, "fxoctrl1=0x%02x, fxoctrl2=0x%02x, fxoctrl5=0x%02x, spark_quench=0x%02x\n",
		daa_reg->fxo_ctrl1, daa_reg->fxo_ctrl2, daa_reg->fxo_ctrl5, daa_reg->spark_quench);
	str += sprintf(str, "i18n1=0x%02x, i18n2=0x%02x, i18n3=0x%02x, i18n4=0x%02x\n",
		daa_reg->i18n_ctrl1, daa_reg->i18n_ctrl2, daa_reg->i18n_ctrl3, daa_reg->i18n_ctrl4);
	str += sprintf(str, "ring1=0x%02x, ring2=0x%02x, ring3=0x%02x\n",
		daa_reg->ring_ctrl1, daa_reg->ring_ctrl2, daa_reg->ring_ctrl3);
	str += sprintf(str, "linedevstat=0x%02x, dcterm=0x%02x, current=0x%02x, voltage=0x%02x(%d%s)\n",
		daa_reg->linedev_status, daa_reg->dc_termination, daa_reg->loop_current, daa_reg->line_voltage, 
		v, (daa_reg->fake_voltage_min||daa_reg->fake_voltage_max)?", faked":"");
	str += sprintf(str, "cvt_threshold=0x%02x, cvt_intrctrl=0x%02x\n",
		daa_reg->cvt_threshold, daa_reg->cvt_intrctrl);

	return (str-buff);
}

static int
sprintf_daa_registers_config_fields(char *buff, Daa_reg *daa_reg)
{
	char *str = buff;

	str += sprintf(str, "RDI=%d\n", daa_reg->ctrl2&(1<<2));	// b2: ring detect intr mode

	str += sprintf(str, "IntrMask=");
	if (daa_reg->intr_mask & (1<<7))	str += sprintf(str, " RDT");
	if (daa_reg->intr_mask & (1<<6))	str += sprintf(str, " ROV");
	if (daa_reg->intr_mask & (1<<5))	str += sprintf(str, " FDT");
	if (daa_reg->intr_mask & (1<<4))	str += sprintf(str, " BDT");
	if (daa_reg->intr_mask & (1<<3))	str += sprintf(str, " DOD");
	if (daa_reg->intr_mask & (1<<2))	str += sprintf(str, " LCSO");
	if (daa_reg->intr_mask & (1<<0))	str += sprintf(str, " POL");
	str += sprintf(str, "\n");

	str += sprintf(str, "FXOCtrl> ONHM=%d, OH(offhook)=%d, PDL=%d, FULL=%d, FOH=%d, OHS.OHS2.SQ1.SQ2=%d%d%d%db, LVFD=%d\n",
		(daa_reg->fxo_ctrl1>>3)&1, (daa_reg->fxo_ctrl1>>0)&1,
		(daa_reg->fxo_ctrl2>>4)&1,
		(daa_reg->fxo_ctrl5>>7)&1, (daa_reg->fxo_ctrl5>>5)&3,
		(daa_reg->i18n_ctrl1>>6)&1, (daa_reg->fxo_ctrl5>>3)&1, (daa_reg->spark_quench>>6)&1, (daa_reg->spark_quench>>4)&1,
		(daa_reg->fxo_ctrl5>>0)&1);
	str += sprintf(str, "I18N> RT=%d, MCAL=%d, CALD=%d, RT2=%d, RFWE=%d\n",
		(daa_reg->i18n_ctrl1>>0)&1,
		(daa_reg->i18n_ctrl2>>6)&1, (daa_reg->i18n_ctrl2>>5)&1, (daa_reg->i18n_ctrl2>>4)&1,
		(daa_reg->i18n_ctrl3>>1)&1);
	str += sprintf(str, "RING> RDLY=%d, RMX=%d, RTO=%d, RCC=%d, RNGV=%d, RAS=%d\n",
		((daa_reg->ring_ctrl1>>6)&3) + (((daa_reg->ring_ctrl2>>7)&1)<<2), (daa_reg->ring_ctrl1>>0)&63,
		(daa_reg->ring_ctrl2>>3)&15, (daa_reg->ring_ctrl2>>0)&7,
		(daa_reg->ring_ctrl3>>7)&1, (daa_reg->ring_ctrl3>>0)&63);
	str += sprintf(str, "CV> DCV=%d, MIN=%d, CVT=%d, CVI=%d, CVS=%d, CVM=%d, CVP=%d\n",
		(daa_reg->dc_termination>>6)&3,  (daa_reg->dc_termination>>4)&3,
		daa_reg->cvt_threshold,
		(daa_reg->cvt_intrctrl>>3)&1, (daa_reg->cvt_intrctrl>>2)&1, (daa_reg->cvt_intrctrl>>1)&1, (daa_reg->cvt_intrctrl>>0)&1);

	return (str-buff);
}

static int
sprintf_daa_registers_status_fields(char *buff, Daa_reg *daa_reg)
{
	char *str = buff;
	int v = daa_reg->line_voltage;
	if (v > 127)
		v -= 256;

	str += sprintf(str, "IntrSrc>");
	if (daa_reg->intr_source & (1<<7))	str += sprintf(str, " RDT");
	if (daa_reg->intr_source & (1<<6))	str += sprintf(str, " ROV");
	if (daa_reg->intr_source & (1<<5))	str += sprintf(str, " FDT");
	if (daa_reg->intr_source & (1<<4))	str += sprintf(str, " BDT");
	if (daa_reg->intr_source & (1<<3))	str += sprintf(str, " DOD");
	if (daa_reg->intr_source & (1<<2))	str += sprintf(str, " LCSO");
	if (daa_reg->intr_source & (1<<0))	str += sprintf(str, " POL");
	str += sprintf(str, "\n");

	str += sprintf(str, "FXOCtrl> RDTN:%d, RDTP:%d, RDT:%d\n",
		(daa_reg->fxo_ctrl1>>6)&1, (daa_reg->fxo_ctrl1>>5)&1, (daa_reg->fxo_ctrl1>>2)&1);
	str += sprintf(str, "I18N> DOD:%d\n",
		(daa_reg->i18n_ctrl4>>1)&1);
	str += sprintf(str, "CV> FDT:%d, LCS:%d, LCS2:%d, LVS:%d\n",
		(daa_reg->linedev_status>>6)&1,  (daa_reg->linedev_status>>0)&31,
		daa_reg->loop_current,
		v);

	return (str-buff);
}

static int
sprintf_daa_state(char *buff, Daa_state *s)
{
	char *str = buff;
	unsigned int msdiff;
	int i;

	str += sprintf(str, "dbgmask=0x%x, logmask=0x%x\n", s->cfg_dbgmask, s->cfg_logmask);

	str += sprintf(str, "\nconfig:\n");
	str += sprintf(str, "voltage_amplitude_reversal_diff=%d(v)\n", s->cfg_voltage_amplitude_reversal_diff);
	str += sprintf(str, "voltage_amplitude_onhook_max=%d(v)\n", s->cfg_voltage_amplitude_onhook_max);
	str += sprintf(str, "voltage_amplitude_onhook_min=%d(v)\n", s->cfg_voltage_amplitude_onhook_min);
	str += sprintf(str, "voltage_amplitude_offhook_max=%d(v)\n", s->cfg_voltage_amplitude_offhook_max);
	str += sprintf(str, "voltage_amplitude_offhook_min=%d(v)\n", s->cfg_voltage_amplitude_offhook_min);
	str += sprintf(str, "vhistory_stable_amplitude_diff=%d(v)\n", s->cfg_vhistory_stable_amplitude_diff);
	str += sprintf(str, "vhistory_stable_onhook_time=%d(ms)\n", s->cfg_vhistory_onhook_stable_time);
	str += sprintf(str, "vhistory_stable_onhook2offhook_transient_time=%d(ms)\n", s->cfg_vhistory_onhook2offhook_transient_time);
	str += sprintf(str, "vhistory_onhook2offhook_stable_time=%d(ms)\n", s->cfg_vhistory_onhook2offhook_stable_time);
	str += sprintf(str, "vhistory_dropout2offhook_stable_time=%d(ms)\n", s->cfg_vhistory_dropout2offhook_stable_time);
	str += sprintf(str, "vhistory_offhook2dropout_stable_time=%d(ms)\n", s->cfg_vhistory_offhook2dropout_stable_time);
	str += sprintf(str, "vhistory_stable_drop_time_max=%d(ms)\n", s->cfg_vhistory_stable_drop_time_max);
	str += sprintf(str, "vdrop_dropout_count=%d\n", s->cfg_vdrop_dropout_count);
	str += sprintf(str, "vdrop_dropout_amplitude_drop=%d(v)\n", s->cfg_vdrop_dropout_amplitude_drop);
	str += sprintf(str, "vdrop_dropout_confirm_time=%d(ms)\n", s->cfg_vdrop_dropout_confirm_time);
	str += sprintf(str, "vdrop_dropout_offhook_amplitude_drop=%d(v)\n", s->cfg_vdrop_dropout_offhook_amplitude_drop);
	str += sprintf(str, "vdrop_dropout_offhook_confirm_time=%d(ms)\n", s->cfg_vdrop_dropout_offhook_confirm_time);
	str += sprintf(str, "vdrop_ripple_amplitude_diff=%d(v)\n", s->cfg_vdrop_ripple_amplitude_diff);
	str += sprintf(str, "vdrop_timeout=%d(ms)\n", s->cfg_vdrop_timeout);
	str += sprintf(str, "loop_current_valid_min=%d(1.1ma)\n", s->cfg_loop_current_valid_min);
	str += sprintf(str, "loop_current_valid_max=%d(1.1ma)\n", s->cfg_loop_current_valid_max);
	str += sprintf(str, "loop_current_stable_diff=%d(1.1ma)\n", s->cfg_loop_current_stable_diff);
	str += sprintf(str, "alert_ringon_time_max=%d(ms)\n", s->cfg_alert_ringon_time_max);
	str += sprintf(str, "alert_ring_timeout=%d(ms)\n", s->cfg_alert_ring_timeout);
	str += sprintf(str, "normal_ring_timeout=%d(ms)\n", s->cfg_normal_ring_timeout);
	str += sprintf(str, "reversal_timeout=%d(ms)\n", s->cfg_reversal_timeout);
	str += sprintf(str, "offhook2onhook_confirm_time=%d(ms)\n", s->cfg_offhook2onhook_confirm_time);
	str += sprintf(str, "dropout2onhook_confirm_time=%d(ms)\n", s->cfg_dropout2onhook_confirm_time);
	str += sprintf(str, "active_offhook_confirm_time=%d(ms)\n", s->cfg_active_offhook_confirm_time);
	str += sprintf(str, "alert_ring_offhook_confirm_time=%d(ms)\n", s->cfg_alert_ring_offhook_confirm_time);
	str += sprintf(str, "normal_ring_offhook_confirm_time=%d(ms)\n", s->cfg_normal_ring_offhook_confirm_time);
	str += sprintf(str, "ignore_ring_stop_reversed_check_time=%d(ms)\n", s->cfg_ignore_ring_stop_reversed_check_time);

	str += sprintf(str, "\nstatus:\n");
	str += sprintf(str, "seq=%d, now=%s\n", s->seq, timestamp2str(&s->now_timestamp));
	if (! is_ts_zero(s->ringon_timestamp)) {
		msdiff = ts_diff_ms(s->ringon_timestamp, s->now_timestamp);
		str += sprintf(str, "ringon_timestamp=%s (%d.%03ds ago)\n",
			timestamp2str(&s->ringon_timestamp), msdiff/1000, msdiff%1000);
	} else {
		str += sprintf(str, "ringon_timestamp=0\n");
	}
	if (! is_ts_zero(s->ringoff_timestamp)) {
		msdiff = ts_diff_ms(s->ringoff_timestamp, s->now_timestamp);
		str += sprintf(str, "ringoff_timestamp=%s (%d.%03ds ago)\n",
			timestamp2str(&s->ringoff_timestamp), msdiff/1000, msdiff%1000);
	} else {
		str += sprintf(str, "ringoff_timestamp=0\n");
	}
	if (! is_ts_zero(s->reversal_timestamp)) {
		msdiff = ts_diff_ms(s->reversal_timestamp, s->now_timestamp);
		str += sprintf(str, "reversal_timestamp=%s (%d.%03ds ago)\n",
			timestamp2str(&s->reversal_timestamp), msdiff/1000, msdiff%1000);
	} else {
		str += sprintf(str, "reversal_timestamp=0\n");
	}
	if (! is_ts_zero(s->voltage_onhook_timestamp)) {
		msdiff = ts_diff_ms(s->voltage_onhook_timestamp, s->now_timestamp);
		str += sprintf(str, "voltage_onhook_timestamp=%s (%d.%03ds ago)\n",
			timestamp2str(&s->voltage_onhook_timestamp), msdiff/1000, msdiff%1000);
	} else {
		str += sprintf(str, "voltage_onhook_timestamp=0\n");
	}
	if (! is_ts_zero(s->voltage_offhook_timestamp)) {
		msdiff = ts_diff_ms(s->voltage_offhook_timestamp, s->now_timestamp);
		str += sprintf(str, "voltage_offhook_timestamp=%s (%d.%03ds ago)\n",
			timestamp2str(&s->voltage_offhook_timestamp), msdiff/1000, msdiff%1000);
	} else {
		str += sprintf(str, "voltage_offhook_timestamp=0\n");
	}

	str += sprintf(str, "vhistory[..%d]\n", s->seq%VHISTORY_TOTAL);
	for (i=0; i< VHISTORY_TOTAL; i++) {
		str += sprintf(str, " %d", s->vhistory[(s->seq+1+i)%VHISTORY_TOTAL]);
		if (i%32 == 31)
			str += sprintf(str, "\n");
 	}
	if (i%32 != 0)
		str += sprintf(str, "\n");
	str += sprintf(str, "vhistory stable: learned min/max=%d/%d(abs=%u/%u)v, stamp=%s, time=%dms, drop_time=%dms, drop=%dv\n",
		s->vhistory_stable_learned_min, s->vhistory_stable_learned_max,
		s->vhistory_stable_learned_abs_min, s->vhistory_stable_learned_abs_max,
		timestamp2str(&s->vhistory_stable_timestamp), s->vhistory_stable_time, s->vhistory_stable_drop_time, s->vhistory_stable_drop_amplitude);
	str += sprintf(str, "vhistory_stable: onhook_voltage=%dv, onhook2offhook_timestamp=%s(time=%d), offhook_reversal_timestamp=%s(time=%d)\n",
		s->vhistory_stable_onhook_voltage,
		timestamp2str(&s->vhistory_stable_onhook2offhook_timestamp), s->vhistory_stable_onhook2offhook_time,
		timestamp2str(&s->vhistory_stable_offhook_reversal_timestamp), s->vhistory_stable_offhook_reversal_time);

	str += sprintf(str, "vdrop[0..%d]", s->vdrop_total-1);
	for (i=0; i< s->vdrop_total; i++) {
		if (i+1 < s->vdrop_total) {
			int msdiff = ts_diff_ms(s->vdrop_timestamp[i], s->vdrop_timestamp[i+1]);
			str += sprintf(str, " %d(%dms)", s->vdrop[i], msdiff);
		} else {
			str += sprintf(str, " %d", s->vdrop[i]);
		}
	}
	str += sprintf(str, ", (onhook=%d between=%d offhook=%d)\n",
		s->vdrop_onhook_count, s->vdrop_between_count, s->vdrop_offhook_count);
	str += sprintf(str, "vdrop total=%d, time=%dms, amplitude=%dv, offhook_time=%dms, offhook_amplitude=%dv, is_dropout=%d (%ds ago)\n",
		s->vdrop_total,
		s->vdrop_time, s->vdrop_amplitude, s->vdrop_offhook_time, s->vdrop_offhook_amplitude,
		s->vdrop_is_dropout, s->vdrop_age_sec);

	str += sprintf(str, "is_ringing=%d, %s, ringon/ringoff cadence=%d/%dms time=%d/%dms, ringoff_voltage=%dv\n",
		s->is_ringing, (s->is_alert_ring)?"alert ring":"normal ring",
		s->ringon_cadence, s->ringoff_cadence, s->ringon_time, s->ringoff_time,
		s->ringoff_voltage);

	str += sprintf(str, "%s, voltage=%dv(onhook=%dv,offhook=%dv), current=%d(min:%d-max:%d)\n",
		hook_state_str[s->hook_state_prev],
		s->line_voltage, s->learned_onhook_voltage, s->learned_offhook_voltage,
		s->loop_current, s->learned_loop_current_min, s->learned_loop_current_max);

	return (str-buff);
}

static int
parse_daa_cli_line(Daa_state *s, Daa_reg *daa_reg, char *input_buff)
{
	static char output_buff[4096];
	char *str = output_buff;
	int argc;
	char *argv[16];
	int ret = 0;

	output_buff[0] = 0;
	argc = str2argv(input_buff, argv, 16);

	if (argc == 1 && strcmp(argv[0], "onhook") == 0) {
                tdi_daa_ctrl_set(s->fd, 5, 0x08); 		// reg5: set daa back to onhook

	} else if (argc == 1 && strcmp(argv[0], "offhook") == 0) {
                tdi_daa_ctrl_set(s->fd, 5, 0x01); 		// reg5(fxo ctrl1): set daa offhook, in order to measure loop current

	} else if (argc >=1 && strcmp(argv[0], "current") == 0) {
		int loop_current, wait_ms = 128;
		int v1, v2, v3;

		if (argc == 2)
			wait_ms = str2val(argv[1]);

		v1 = tdi_daa_ctrl_get(s->fd, 29);
		if ( v1> 127) v1 -= 256;

                loop_current = fv_state_machine_daa_get_loop_current(s, wait_ms);
		v2 = tdi_daa_ctrl_get(s->fd, 29);
		if ( v2> 127) v2 -= 256;

		usleep(wait_ms*1000);	// wait for voltage to be stable in case the pstn line is connected

		v3 = tdi_daa_ctrl_get(s->fd, 29);
		if ( v3> 127) v3 -= 256;

		str += sprintf(str, "daa loop current = %d, line voltage %d->%d->%d (wait %dms)\n", loop_current, v1, v2, v3, wait_ms);

	} else if (argc >=2 && strcmp(argv[0], "fake_voltage") == 0) {
		daa_reg->fake_voltage_min = daa_reg->fake_voltage_max = str2val(argv[1]);
		if (argc == 3)
			daa_reg->fake_voltage_max = str2val(argv[2]);
		if (daa_reg->fake_voltage_min || daa_reg->fake_voltage_max) {
			if (daa_reg->fake_voltage_min > 126)
				daa_reg->fake_voltage_min = 126;
			else if (daa_reg->fake_voltage_min < -126)
				daa_reg->fake_voltage_min = -126;
			if (daa_reg->fake_voltage_max > 126)
				daa_reg->fake_voltage_max = 126;
			else if (daa_reg->fake_voltage_max < -126)
				daa_reg->fake_voltage_max = -126;
			str += sprintf(str, "fake_voltage enabled %d..%d\n", daa_reg->fake_voltage_min, daa_reg->fake_voltage_max);
		} else {
			str += sprintf(str, "fake_voltage disabled\n");
		}

	} else if (argc == 2 && strcmp(argv[0], "reg") == 0) {
		int regnum = str2val(argv[1]);
		unsigned char value = tdi_daa_ctrl_get(s->fd, regnum);
		str += sprintf(str, "reg%d = 0x%x\n", regnum, value);

	} else if (argc == 3 && strcmp(argv[0], "reg") == 0) {
		int regnum = str2val(argv[1]);
		unsigned char value = str2val(argv[2]);
		unsigned value2;

		tdi_daa_ctrl_set(s->fd, regnum, value);
		value2 = tdi_daa_ctrl_get(s->fd, regnum);
		if (value2 != value) {
			str += sprintf(str, "reg%d = 0x%x? (should be 0x%x!)\n", regnum, value2, value);
		} else {
			str += sprintf(str, "reg%d = 0x%x\n", regnum, value2);
		}

	} else if (argc == 2 && strcmp(argv[0], "dbg") == 0) {
		s->cfg_dbgmask = str2val(argv[1]);
		str += sprintf(str, "dbgmask=0x%x\n", s->cfg_dbgmask);

	} else if (argc == 2 && strcmp(argv[0], "log") == 0) {
		if (strcmp(argv[1], "clear") == 0) {
			unlink(DAA_LOG_FILE);
			str += sprintf(str, "clear file %s\n", DAA_LOG_FILE);
		} else {
			s->cfg_logmask = str2val(argv[1]);
			str += sprintf(str, "logmask=0x%x\n", s->cfg_logmask);
		}

	} else if (argc == 3 && strcmp(argv[0], "set") == 0) {
		int value = str2val(argv[2]);
		if (strcmp(argv[1], "dbgmask") == 0) {
			s->cfg_dbgmask = value;
			str += sprintf(str, "dbgmask=0x%x\n", s->cfg_dbgmask);
		} else if (strcmp(argv[1], "logmask") == 0) {
			s->cfg_logmask = value;
			str += sprintf(str, "logmask=0x%x\n", s->cfg_logmask);

		} else if (strcmp(argv[1], "voltage_amplitude_reversal_diff") == 0) {
			s->cfg_voltage_amplitude_reversal_diff = value;
			str += sprintf(str, "voltage_amplitude_reversal_diff=%d(v)\n", s->cfg_voltage_amplitude_reversal_diff);
		} else if (strcmp(argv[1], "voltage_amplitude_onhook_max") == 0) {
			s->cfg_voltage_amplitude_onhook_max = value;
			str += sprintf(str, "voltage_amplitude_onhook_max=%d(v)\n", s->cfg_voltage_amplitude_onhook_max);
		} else if (strcmp(argv[1], "voltage_amplitude_onhook_min") == 0) {
			s->cfg_voltage_amplitude_onhook_min = value;
			str += sprintf(str, "voltage_amplitude_onhook_min=%d(v)\n", s->cfg_voltage_amplitude_onhook_min);
		} else if (strcmp(argv[1], "voltage_amplitude_offhook_max") == 0) {
			s->cfg_voltage_amplitude_offhook_max = value;
			str += sprintf(str, "voltage_amplitude_offhook_max=%d(v)\n", s->cfg_voltage_amplitude_offhook_max);
		} else if (strcmp(argv[1], "voltage_amplitude_offhook_min") == 0) {
			s->cfg_voltage_amplitude_offhook_min = value;
			str += sprintf(str, "voltage_amplitude_offhook_min=%d(v)\n", s->cfg_voltage_amplitude_offhook_min);

		} else if (strcmp(argv[1], "vhistory_stable_amplitude_diff") == 0) {
			s->cfg_vhistory_stable_amplitude_diff = value;
			str += sprintf(str, "vhistory_stable_amplitude_diff=%d(v)\n", s->cfg_vhistory_stable_amplitude_diff);
		} else if (strcmp(argv[1], "vhistory_stable_onhook_time") == 0) {
			s->cfg_vhistory_onhook_stable_time = value;
			str += sprintf(str, "vhistory_stable_onhook_time=%d(ms)\n", s->cfg_vhistory_onhook_stable_time);
		} else if (strcmp(argv[1], "vhistory_stable_onhook2offhook_transient_time") == 0) {
			s->cfg_vhistory_onhook2offhook_transient_time = value;
			str += sprintf(str, "vhistory_stable_onhook2offhook_transient_time=%d(ms)\n", s->cfg_vhistory_onhook2offhook_transient_time);
		} else if (strcmp(argv[1], "vhistory_onhook2offhook_stable_time") == 0) {
			s->cfg_vhistory_onhook2offhook_stable_time = value;
			str += sprintf(str, "vhistory_onhook2offhook_stable_time=%d(ms)\n", s->cfg_vhistory_onhook2offhook_stable_time);
		} else if (strcmp(argv[1], "vhistory_dropout2offhook_stable_time") == 0) {
			s->cfg_vhistory_dropout2offhook_stable_time = value;
			str += sprintf(str, "vhistory_dropout2offhook_stable_time=%d(ms)\n", s->cfg_vhistory_dropout2offhook_stable_time);
		} else if (strcmp(argv[1], "vhistory_offhook2dropout_stable_time") == 0) {
			s->cfg_vhistory_offhook2dropout_stable_time = value;
			str += sprintf(str, "vhistory_offhook2dropout_stable_time=%d(ms)\n", s->cfg_vhistory_offhook2dropout_stable_time);
		} else if (strcmp(argv[1], "vhistory_stable_drop_time_max") == 0) {
			s->cfg_vhistory_stable_drop_time_max = value;
			str += sprintf(str, "vhistory_stable_drop_time_max=%d(ms)\n", s->cfg_vhistory_stable_drop_time_max);

		} else if (strcmp(argv[1], "vdrop_dropout_count") == 0) {
			s->cfg_vdrop_dropout_count = value;
			str += sprintf(str, "vdrop_dropout_count=%d\n", s->cfg_vdrop_dropout_count);
		} else if (strcmp(argv[1], "vdrop_dropout_amplitude_drop") == 0) {
			s->cfg_vdrop_dropout_amplitude_drop = value;
			str += sprintf(str, "vdrop_dropout_amplitude_drop=%d(v)\n", s->cfg_vdrop_dropout_amplitude_drop);
		} else if (strcmp(argv[1], "vdrop_dropout_confirm_time") == 0) {
			s->cfg_vdrop_dropout_confirm_time = value;
			str += sprintf(str, "vdrop_dropout_confirm_time=%d(ms)\n", s->cfg_vdrop_dropout_confirm_time);
		} else if (strcmp(argv[1], "vdrop_dropout_offhook_amplitude_drop") == 0) {
			s->cfg_vdrop_dropout_offhook_amplitude_drop = value;
			str += sprintf(str, "vdrop_dropout_offhook_amplitude_drop=%d(v)\n", s->cfg_vdrop_dropout_offhook_amplitude_drop);
		} else if (strcmp(argv[1], "vdrop_dropout_offhook_confirm_time") == 0) {
			s->cfg_vdrop_dropout_offhook_confirm_time = value;
			str += sprintf(str, "vdrop_dropout_offhook_confirm_time=%d(ms)\n", s->cfg_vdrop_dropout_offhook_confirm_time);
		} else if (strcmp(argv[1], "vdrop_ripple_amplitude_diff") == 0) {
			s->cfg_vdrop_ripple_amplitude_diff = value;
			str += sprintf(str, "vdrop_ripple_amplitude_diff=%d(v)\n", s->cfg_vdrop_ripple_amplitude_diff);
		} else if (strcmp(argv[1], "vdrop_timeout") == 0) {
			s->cfg_vdrop_timeout = value;
			str += sprintf(str, "vdrop_timeout=%d\n", s->cfg_vdrop_timeout);

		} else if (strcmp(argv[1], "loop_current_valid_min") == 0) {
			s->cfg_loop_current_valid_min = value;
			str += sprintf(str, "loop_current_valid_min=%d\n", s->cfg_loop_current_valid_min);
		} else if (strcmp(argv[1], "loop_current_valid_max") == 0) {
			s->cfg_loop_current_valid_max = value;
			str += sprintf(str, "loop_current_valid_max=%d\n", s->cfg_loop_current_valid_max);
		} else if (strcmp(argv[1], "loop_current_stable_diff") == 0) {
			s->cfg_loop_current_stable_diff = value;
			str += sprintf(str, "loop_current_stable_diff=%d\n", s->cfg_loop_current_stable_diff);

		} else if (strcmp(argv[1], "alert_ringon_time_max") == 0) {
			s->cfg_alert_ringon_time_max = value;
			str += sprintf(str, "alert_ringon_time_max=%d(ms)\n", s->cfg_alert_ringon_time_max);
		} else if (strcmp(argv[1], "alert_ring_timeout") == 0) {
			s->cfg_alert_ring_timeout = value;
			str += sprintf(str, "alert_ring_timeout=%d(ms)\n", s->cfg_alert_ring_timeout);
		} else if (strcmp(argv[1], "normal_ring_timeout") == 0) {
			s->cfg_normal_ring_timeout = value;
			str += sprintf(str, "normal_ring_timeout=%d(ms)\n", s->cfg_normal_ring_timeout);
		} else if (strcmp(argv[1], "reversal_timeout") == 0) {
			s->cfg_reversal_timeout = value;
			str += sprintf(str, "reversal_timeout=%d(ms)\n", s->cfg_reversal_timeout);
		} else if (strcmp(argv[1], "offhook2onhook_confirm_time") == 0) {
			s->cfg_offhook2onhook_confirm_time = value;
			str += sprintf(str, "offhook2onhook_confirm_time=%d(ms)\n", s->cfg_offhook2onhook_confirm_time);
		} else if (strcmp(argv[1], "dropout2onhook_confirm_time") == 0) {
			s->cfg_dropout2onhook_confirm_time = value;
			str += sprintf(str, "dropout2onhook_confirm_time=%d(ms)\n", s->cfg_dropout2onhook_confirm_time);
		} else if (strcmp(argv[1], "active_offhook_confirm_time") == 0) {
			s->cfg_active_offhook_confirm_time = value;
			str += sprintf(str, "active_offhook_confirm_time=%d(ms)\n", s->cfg_active_offhook_confirm_time);
		} else if (strcmp(argv[1], "alert_ring_offhook_confirm_time") == 0) {
			s->cfg_alert_ring_offhook_confirm_time = value;
			str += sprintf(str, "alert_ring_offhook_confirm_time=%d(ms)\n", s->cfg_alert_ring_offhook_confirm_time);
		} else if (strcmp(argv[1], "normal_ring_offhook_confirm_time") == 0) {
			s->cfg_normal_ring_offhook_confirm_time = value;
			str += sprintf(str, "normal_ring_offhook_confirm_time=%d(ms)\n", s->cfg_normal_ring_offhook_confirm_time);
		} else if (strcmp(argv[1], "ignore_ring_stop_reversed_check_time") == 0) {
			s->cfg_ignore_ring_stop_reversed_check_time = value;
			str += sprintf(str, "ignore_ring_stop_reversed_check_time=%d(ms)\n", s->cfg_ignore_ring_stop_reversed_check_time);
		} else {
			str += sprintf(str, "%s not recognized\n", argv[1]);
		}

	} else if (argc == 1 && strcmp(argv[0], "dump") == 0) {
		str += sprintf(str, "seq=%d\n", s->seq);
		str += sprintf(str, "registers:\n");
		str += sprintf_daa_registers(str, daa_reg);
		str += sprintf(str, "\nregisters config fields:\n");
		str += sprintf_daa_registers_config_fields(str, daa_reg);
		str += sprintf(str, "\nregisters status fields:\n");
		str += sprintf_daa_registers_status_fields(str, daa_reg);
		str += sprintf(str, "\n");
		str += sprintf_daa_state(str, s);

	} else if (argc == 1 && strcmp(argv[0], "logtest") == 0) {
		LOGV("Verbose - %d\n", LOG_LEVEL_VERBOSE);
		LOGD("Debug   - %d\n", LOG_LEVEL_DEBUG);
		LOGI("Info    - %d\n", LOG_LEVEL_INFO);
		LOGN("Notice  - %d\n", LOG_LEVEL_NOTICE);
		LOGW("Warning - %d\n", LOG_LEVEL_WARNING);
		LOGE("Error   - %d\n", LOG_LEVEL_ERROR);
		LOGF("Fatal   - %d\n", LOG_LEVEL_FATAL);

	} else if (argc >=1) {
		str += sprintf(str,
			"\ndbgmask=0x%x, logmask=0x%x, Daa_state size=%d",
			s->cfg_dbgmask, s->cfg_logmask, sizeof(Daa_state));
		if (daa_reg->fake_voltage_min || daa_reg->fake_voltage_max) {
			str += sprintf(str,", fake_voltage %d..%d\n",
				daa_reg->fake_voltage_min, daa_reg->fake_voltage_max);
		}
		str += sprintf(str,
			"\navailable cmd:\n"
			"	dbg [mask]\n"
			"	log [mask]|clear\n"
			"	offhook\n"
			"	onhook\n"
			"	fake_voltage [min] [max] (0 0 to disable)\n"
			"	current [wait_ms]\n"
			"	reg [reg_num] [value]\n"
			"	set [config_key] [value]\n"
			"	dump\n"
			"	help\n\n"
			"mask: b8:all_on_event b7:cli b6:register b5:current b4:voltage b3:reversal b2:ring b1:hook_state b0:event\n");
		ret = -1;
	}
	if (s->cfg_logmask & DBGMASK_CLI)
		util_logprintf(DAA_LOG_FILE, "%s\n", output_buff);
	LOGF("%s", output_buff);

	return ret;
}
static int
parse_daa_cli(Daa_state *s, Daa_reg *daa_reg)
{
	char buff[1024];
	FILE *fp;

	if ((fp = fopen(DAA_CLI_FILE, "rb")) == NULL)
		return -1;

	buff[0] = 0;
	while (fgets(buff, 1024, fp) > 0) {
		int len = strlen(buff);
		if (buff[len - 1] == '\n' || buff[len - 1] == '\r') {
			buff[len - 1] = 0;
			len--;
		}
		if (buff[len - 1] == '\n' || buff[len - 1] == '\r') {
			buff[len - 1] = 0;
			len--;
		}
		if (len >0)
			parse_daa_cli_line(s, daa_reg, buff);
	}
	fclose(fp);
	unlink(DAA_CLI_FILE);

	return 0;
}

int
fv_state_machine_daa_init(Daa_state *s)
{
	// keep attrs inited by caller
	int fd_orig = s->fd;
	int idx_orig = s->idx;

	memset(s, 0, sizeof(Daa_state));

	clock_gettime(CLOCK_MONOTONIC, &s->now_timestamp);

	s->cfg_voltage_amplitude_reversal_diff = VOLTAGE_AMPLITUDE_REVERSAL_DIFF;
	s->cfg_voltage_amplitude_onhook_max = VOLTAGE_AMPLITUDE_ONHOOK_MAX;
	s->cfg_voltage_amplitude_onhook_min = VOLTAGE_AMPLITUDE_ONHOOK_MIN;
	s->cfg_voltage_amplitude_offhook_max = VOLTAGE_AMPLITUDE_OFFHOOK_MAX;
	s->cfg_voltage_amplitude_offhook_min = VOLTAGE_AMPLITUDE_OFFHOOK_MIN;

	s->cfg_vhistory_stable_amplitude_diff = VHISTORY_STABLE_AMPLITUDE_DIFF;
	s->cfg_vhistory_onhook_stable_time = VHISTORY_ONHOOK_STABLE_TIME;
	s->cfg_vhistory_onhook2offhook_transient_time = VHISTORY_ONHOOK2OFFHOOK_TRANSIENT_TIME;
	s->cfg_vhistory_onhook2offhook_stable_time = VHISTORY_ONHOOK2OFFHOOK_STABLE_TIME;
	s->cfg_vhistory_dropout2offhook_stable_time = VHISTORY_DROPOUT2OFFHOOK_STABLE_TIME;
	s->cfg_vhistory_offhook2dropout_stable_time = VHISTORY_OFFHOOK2DROPOUT_STABLE_TIME;
	s->cfg_vhistory_stable_drop_time_max = VHISTORY_STABLE_DROP_TIME_MAX;

	s->cfg_vdrop_dropout_count = VDROP_DROPOUT_COUNT;
	s->cfg_vdrop_dropout_amplitude_drop = VDROP_DROPOUT_AMPLITUDE_DROP;
	s->cfg_vdrop_dropout_confirm_time = VDROP_DROPOUT_CONFIRM_TIME;
	s->cfg_vdrop_dropout_offhook_amplitude_drop = VDROP_DROPOUT_OFFHOOK_AMPLITUDE_DROP;
	s->cfg_vdrop_dropout_offhook_confirm_time = VDROP_DROPOUT_OFFHOOK_CONFIRM_TIME;
	s->cfg_vdrop_ripple_amplitude_diff = VDROP_RIPPLE_AMPLITUDE_DIFF;
	s->cfg_vdrop_timeout = VDROP_TIMEOUT;

	s->cfg_loop_current_valid_min = LOOP_CURRENT_VALID_MIN;
	s->cfg_loop_current_valid_max = LOOP_CURRENT_VALID_MAX;
	s->cfg_loop_current_stable_diff = LOOP_CURRENT_STABLE_DIFF;

	s->cfg_alert_ringon_time_max = ALERT_RINGON_TIME_MAX;
	s->cfg_alert_ring_timeout = ALERT_RING_TIMEOUT;
	s->cfg_normal_ring_timeout = NORMAL_RING_TIMEOUT;
	s->cfg_reversal_timeout = REVERSAL_TIMEOUT;
	s->cfg_offhook2onhook_confirm_time = OFFHOOK2ONHOOK_CONFIRM_TIME;
	s->cfg_dropout2onhook_confirm_time = DROPOUT2ONHOOK_CONFIRM_TIME;
	s->cfg_active_offhook_confirm_time = ACTIVE_OFFHOOK_CONFIRM_TIME;
	s->cfg_alert_ring_offhook_confirm_time = ALERT_RING_OFFHOOK_CONFIRM_TIME;
	s->cfg_normal_ring_offhook_confirm_time = NORMAL_RING_OFFHOOK_CONFIRM_TIME;
	s->cfg_ignore_ring_stop_reversed_check_time =  IGNORE_RING_STOP_REVERSED_CHECK_TIME;

	s->cfg_dbgmask = DBGMASK_DBG_DEFAULT;
	s->cfg_logmask = DBGMASK_LOG_DEFAULT;

	s->learned_loop_current_max = LOOP_CURRENT_MAX_DEFAULT;
	s->learned_loop_current_min = LOOP_CURRENT_MIN_DEFAULT;

	s->vhistory_stable_onhook_voltage = (VOLTAGE_AMPLITUDE_ONHOOK_MIN + VOLTAGE_AMPLITUDE_ONHOOK_MAX)/2;
	s->vhistory_stable_onhook2offhook_timestamp = s->now_timestamp;
	s->vhistory_stable_offhook_reversal_timestamp = zero_timestamp;

	s->hook_state_prev = STATE_INIT;

	// restore attrs inited by caller
	s->fd = fd_orig;
	s->idx = idx_orig;

	lm_daa = logm_register_and_find("DAA", LOG_WARNING, LOG_WARNING, LOG_USER, LOG_DESTMASK_FILE, DAA_LOG_FILE);
	return 0;
}

// by default, the state machine is inited from hook_state == STATE_INIT
// it might takes N*100ms to judge which state should go after INIT
// the caller can quick init the hook_state to state other than STATE_INIT
// which could be more meaningful for caller application
unsigned int
fv_state_machine_daa_init_hook_state(Daa_state *s)
{
	unsigned char daa_reg_fxo_ctrl1 = tdi_daa_ctrl_get(s->fd, 5);
	unsigned char daa_reg_loop_current = tdi_daa_ctrl_get(s->fd, 28);
	unsigned char daa_reg_line_voltage = tdi_daa_ctrl_get(s->fd, 29);

	int hook_state = s->hook_state_prev;	// should be STATE_INIT
	unsigned int daa_eventmask = 0;

	// clear intr register
	tdi_daa_ctrl_set(s->fd, 4, 0);

	s->loop_current = daa_reg_loop_current;
	s->line_voltage = daa_reg_line_voltage;
	if (s->line_voltage > 127)
		s->line_voltage -= 256;

	if (daa_reg_fxo_ctrl1 & (RDT|RDTP|RDTN)) {			// check RDT, RDTP, RDTN for sine wave, onhook confirmed
		hook_state = STATE_DAA_ONHOOK_HANDSET_ONHOOK;
	} else if (abs(s->line_voltage) >= s->cfg_voltage_amplitude_onhook_min ||
		   abs(s->line_voltage) > s->cfg_voltage_amplitude_offhook_max) { // curr voltage is onhook or between onhook/offhook
		// voltage is onhook, pstn line is not in use, it should be safe to turn daa offhook to get loop current
		if (fv_state_machine_daa_get_loop_current(s, 20) == 0) {	// this function need 20ms to detect PSTN disconnect
			hook_state = STATE_DROPOUT;
		} else {
			hook_state = STATE_DAA_ONHOOK_HANDSET_ONHOOK;
		}
	} else {								// curr voltage is offhook or under offhook
		// count loop current only if it is large enough
		if (s->loop_current >= s->cfg_loop_current_valid_min) {
			// DAA offhook
			if (s->learned_loop_current_max - s->loop_current < s->loop_current - s->learned_loop_current_min) {
				// current near max, only daa offhook
				hook_state = STATE_DAA_OFFHOOK_HANDSET_ONHOOK;
			} else {
				// current near min, both daa & handset offhook
				hook_state = STATE_DAA_OFFHOOK_HANDSET_OFFHOOK;
			}
		} else {							// curr voltage is offhook or under offhook, current==0
			// Some case DAA have voltage by induction, need to check current
			// it should be okay to turn daa offhook to get loop current,
			if (fv_state_machine_daa_get_loop_current(s, 20) == 0) {	// this function need 20ms to detect PSTN disconnect
				hook_state = STATE_DROPOUT;
			} else {
				hook_state = STATE_DAA_ONHOOK_HANDSET_OFFHOOK;
			}
		}
	}

	// // judge connect/disconnect
	if (hook_state == STATE_DROPOUT)
		daa_eventmask |= DAA_EVENTMASK_DISCONNECT;
	else
		daa_eventmask |= DAA_EVENTMASK_CONNECT;
	// judge handset offhook/onhook
	if (hook_state == STATE_DAA_ONHOOK_HANDSET_OFFHOOK ||
	    hook_state == STATE_DAA_OFFHOOK_HANDSET_OFFHOOK)
		daa_eventmask |= DAA_EVENTMASK_HANDSET_OFFHOOK;
	else if (hook_state == STATE_DAA_ONHOOK_HANDSET_ONHOOK ||
		 hook_state == STATE_DAA_OFFHOOK_HANDSET_ONHOOK)
		daa_eventmask |= DAA_EVENTMASK_HANDSET_ONHOOK;
	// judge daa offhook/onhook
	if (hook_state == STATE_DAA_OFFHOOK_HANDSET_ONHOOK ||
	    hook_state == STATE_DAA_OFFHOOK_HANDSET_OFFHOOK)
		daa_eventmask |= DAA_EVENTMASK_DAA_OFFHOOK;
	else if (hook_state == STATE_DAA_ONHOOK_HANDSET_ONHOOK ||
		 hook_state == STATE_DAA_ONHOOK_HANDSET_OFFHOOK)
		daa_eventmask |= DAA_EVENTMASK_DAA_ONHOOK;

	s->hook_state_prev = hook_state;
	s->daa_eventmask_prev = daa_eventmask;

	return daa_eventmask;
}

static int
load_daa_registers(Daa_state *s, Daa_reg *daa_reg)
{
	static char line_buff[1000];

	daa_reg->ctrl2 = tdi_daa_ctrl_get(s->fd, 2);
	daa_reg->intr_mask = tdi_daa_ctrl_get(s->fd, 3);
	daa_reg->intr_source = tdi_daa_ctrl_get(s->fd, 4);

	// clear intr ASAP, so we can see next intr, NOTE: write 0 to clear!
	if (daa_reg->intr_source !=0)
		tdi_daa_ctrl_set(s->fd, 4, 0);

	daa_reg->fxo_ctrl1 = tdi_daa_ctrl_get(s->fd, 5);
	daa_reg->fxo_ctrl2 = tdi_daa_ctrl_get(s->fd, 6);
	daa_reg->fxo_ctrl5 = tdi_daa_ctrl_get(s->fd, 31);

	daa_reg->i18n_ctrl1 = tdi_daa_ctrl_get(s->fd, 16);
	daa_reg->i18n_ctrl2 = tdi_daa_ctrl_get(s->fd, 17);
	daa_reg->i18n_ctrl3 = tdi_daa_ctrl_get(s->fd, 18);
	daa_reg->i18n_ctrl4 = tdi_daa_ctrl_get(s->fd, 19);

	daa_reg->ring_ctrl1 = tdi_daa_ctrl_get(s->fd, 22);
	daa_reg->ring_ctrl2 = tdi_daa_ctrl_get(s->fd, 23);
	daa_reg->ring_ctrl3 = tdi_daa_ctrl_get(s->fd, 24);

	daa_reg->linedev_status = tdi_daa_ctrl_get(s->fd, 12);
	daa_reg->dc_termination = tdi_daa_ctrl_get(s->fd, 26);
	daa_reg->loop_current = tdi_daa_ctrl_get(s->fd, 28);
	daa_reg->line_voltage = tdi_daa_ctrl_get(s->fd, 29);

	daa_reg->cvt_threshold = tdi_daa_ctrl_get(s->fd, 43);
	daa_reg->cvt_intrctrl = tdi_daa_ctrl_get(s->fd, 44);
	daa_reg->spark_quench = tdi_daa_ctrl_get(s->fd, 59);

	// clear intr ASAP, so we can see next intr, NOTE: write 0 to clear!
	if ((daa_reg->cvt_intrctrl>>3)&1)
		tdi_daa_ctrl_set(s->fd, 44, (daa_reg->cvt_intrctrl& ~(1<<3)));

	if (daa_reg->fake_voltage_min || daa_reg->fake_voltage_max) {
		int min, v;
		if (daa_reg->fake_voltage_min == daa_reg->fake_voltage_max) {
			v = daa_reg->fake_voltage_min;
		} else {
			if (daa_reg->fake_voltage_min < daa_reg->fake_voltage_max)
				min = daa_reg->fake_voltage_min;
			else
				min = daa_reg->fake_voltage_max;
			v = min + random()%(abs(daa_reg->fake_voltage_max-daa_reg->fake_voltage_min)+1);
		}
		if (v <0)
			daa_reg->line_voltage = v+256;
		else
			daa_reg->line_voltage = v;
	}

	if ((s->cfg_dbgmask|s->cfg_logmask) & (DBGMASK_REGISTER|DBGMASK_ALL_ON_EVENT)) {
		char *str = line_buff;
		if (s->seq%60 == 0) {	// show config per N*0.1 sec
			str += sprintf(str, "seq=%d, t=%s\n",
				s->seq, timestamp2str(&s->now_timestamp));
			str += sprintf(str, "registers:\n");
			str += sprintf_daa_registers(str, daa_reg);
			str += sprintf(str, "\nregisters config fields:\n");
			str += sprintf_daa_registers_config_fields(str, daa_reg);
			str += sprintf(str, "\nregisters status fields:\n");
			str += sprintf_daa_registers_status_fields(str, daa_reg);
		} else {
			str += sprintf(str, "seq=%d, t=%s\n",
				s->seq, timestamp2str(&s->now_timestamp));
			str += sprintf_daa_registers_status_fields(str, daa_reg);
		}
		dbg_log_buff_add_line(DBGMASK_REGISTER|DBGMASK_ALL_ON_EVENT, s->cfg_dbgmask, s->cfg_logmask, line_buff);
	}
	return 0;
}

static void
state_update_vhistory(Daa_state *s, Daa_reg *daa_reg)
{
	static char line_buff[1000];
	int i;

	// update vhistory[]
	s->vhistory[s->seq%VHISTORY_TOTAL] = s->line_voltage;
	s->vhistory_timestamp[s->seq%VHISTORY_TOTAL] = s->now_timestamp;
	// ps: even for (RDT|RDTP|RDTN) cases,
	//     we record the ac line voltage ASIS since they would be ignored in hookstate and reversal judgement because of RDTN/RDTP/RDT,
	//     and the randomness of sine wave voltage would prevent the voltages become a false-stable state in future vhistory[] judgement

	// in the begining, use last voltage as learned stable voltage with stable time set to 0
	s->vhistory_stable_learned_min = s->vhistory_stable_learned_max = s->line_voltage;
	s->vhistory_stable_learned_abs_min = s->vhistory_stable_learned_abs_max = abs(s->line_voltage);
	s->vhistory_stable_timestamp = s->now_timestamp;
	s->vhistory_stable_time = s->vhistory_stable_drop_time = s->vhistory_stable_drop_amplitude = 0;

	if (daa_reg->fxo_ctrl1 & (RDT|RDTP|RDTN)) {	// check RDT, RDTP, RDTN for sine wave
		// update onhook2offhook_timestamp for ring, as ring is also a onhook state indication
		if (daa_reg->fxo_ctrl1 & RDT) {
			s->vhistory_stable_onhook2offhook_timestamp = s->now_timestamp;
			s->vhistory_stable_onhook2offhook_time = 0;
			s->vhistory_stable_offhook_reversal_timestamp = zero_timestamp;
		}
	} else {
		int amplitude = abs(s->line_voltage);
		int v, vnext, vmin, vmax, abs_vmin, abs_vmax;
		int drop_end_voltage;
		struct timespec drop_end_timestamp;

		// for vhisttory_stable_time
		v = vmin = vmax = s->line_voltage;
		abs_vmin = abs_vmax = abs(v);
		// for vhistory_stable drop_time & drop_amplitude
		drop_end_voltage = s->line_voltage;
		drop_end_timestamp = s->now_timestamp;

		for (i=1; i<VHISTORY_TOTAL; i++) {
			if (s->seq -i < 0)
				break;
			vnext = v;
			v = s->vhistory[(s->seq-i)%VHISTORY_TOTAL];

			if (abs(v) > abs_vmax)
				abs_vmax = abs(v);
			if (abs(v) < abs_vmin)
				abs_vmin = abs(v);
			if (v > vmax)
				vmax = v;
			if (v < vmin)
				vmin = v;
			if (abs_vmax - abs_vmin <= s->cfg_vhistory_stable_amplitude_diff) {
				if (abs(v) >= abs(vnext)) {
					int drop_time = ts_diff_ms(s->vhistory_timestamp[(s->seq-i)%VHISTORY_TOTAL], drop_end_timestamp);
					int drop_amplitude = abs(v)-abs(drop_end_voltage);
					if (drop_time > s->vhistory_stable_drop_time && drop_amplitude >=2) {	// use diff over 2 to ensure the drop tendency
						s->vhistory_stable_drop_time = drop_time;
						s->vhistory_stable_drop_amplitude = drop_amplitude;
					}
				} else {
					drop_end_timestamp = s->vhistory_timestamp[(s->seq-i)%VHISTORY_TOTAL];
					drop_end_voltage = v;
				}

				s->vhistory_stable_learned_min = vmin;
				s->vhistory_stable_learned_max = vmax;
				s->vhistory_stable_learned_abs_min = abs_vmin;
				s->vhistory_stable_learned_abs_max = abs_vmax;
				s->vhistory_stable_timestamp = s->vhistory_timestamp[(s->seq-i)%VHISTORY_TOTAL];
				s->vhistory_stable_time = ts_diff_ms(s->vhistory_stable_timestamp, s->now_timestamp);
				// if stable time is long enough to cover the most critial requirement(cfg_vhistory_offhook2dropout_stable_time),
				// no need to further calc the stable time, as which might introduce bigger variation in vmin/vmax
				if (s->vhistory_stable_time > s->cfg_vhistory_offhook2dropout_stable_time)
					break;
			} else {
				break;
			}
		}
		// reversal amplitude may lower then offhook min
		if  (amplitude <= s->cfg_voltage_amplitude_offhook_max) {
			if (is_ts_zero(s->vhistory_stable_offhook_reversal_timestamp)) {
				if (s->seq >= 1) {
					int vprev = s->vhistory[(s->seq-1)%VHISTORY_TOTAL];
					if ((s->line_voltage >=0 && vprev <0) || (s->line_voltage <0 && vprev >=0))
						s->vhistory_stable_offhook_reversal_timestamp = s->now_timestamp;
				}
			}
		}

		// find the vhistory stable onhook voltage and timestamp
		if (amplitude < s->cfg_voltage_amplitude_onhook_max && amplitude >= s->cfg_voltage_amplitude_onhook_min) {
			if  (s->vhistory_stable_time >= s->cfg_vhistory_onhook_stable_time)
				s->vhistory_stable_onhook_voltage = s->line_voltage;
			if (abs(amplitude - abs(s->vhistory_stable_onhook_voltage)) <= s->cfg_vhistory_stable_amplitude_diff)
				s->vhistory_stable_onhook2offhook_timestamp = s->now_timestamp;
			s->vhistory_stable_onhook2offhook_time = 0;
			s->vhistory_stable_offhook_reversal_timestamp = zero_timestamp;

		// find the transient time from stable onhook voltage to stable offhook voltage
		} else if  (amplitude <= s->cfg_voltage_amplitude_offhook_max && amplitude >= s->cfg_voltage_amplitude_offhook_min) {
			if (s->vhistory_stable_time >= s->cfg_vhistory_onhook2offhook_stable_time) {
				if (s->hook_state_prev == STATE_DAA_ONHOOK_HANDSET_ONHOOK) {
					// the time diff from latest stable onhook voltage to stable offhook voltage
					// since 0 is used as special meaning "vhistory_stable_onhook2offhook_time has not been set yet"
					// we set diff result to 1 when vhistory_stable_onhook2offhook_timestamp >= vhistory_stable_timestamp
					// case a: (==)
					//	the start of offhook stable time is the same as the end of ring timestamp
					//	so the msdiff from stable onhook(which is ring) to stable offhook time will be 0
					// case b: (>)
					//	the end of ring could be happen to be around 7v, 
					//	the start of offhook stable time becomes even eariler than the end of ringing
					//	so the msdiff become negative
					int msdiff;
					if ((msdiff = ts_diff_ms(s->vhistory_stable_onhook2offhook_timestamp, s->vhistory_stable_timestamp)) <=0)
						msdiff = 1;
					if (s->vhistory_stable_onhook2offhook_time == 0)
						s->internal_event_detected++;	// display debug for the 1st measured onhook2offhook_time
					s->vhistory_stable_onhook2offhook_time = msdiff;
					// offhook_reversal_time is used to find out whether voltage reversal happens before offhook voltage become stable
					if (!is_ts_zero(s->vhistory_stable_offhook_reversal_timestamp)) {
						if ((msdiff = ts_diff_ms(s->vhistory_stable_offhook_reversal_timestamp, s->vhistory_stable_timestamp)) <=0)
							msdiff = 0;
						s->vhistory_stable_offhook_reversal_time = msdiff;
					} else {
						s->vhistory_stable_offhook_reversal_time = 0;
					}
				}
			}
		}
	}
	// print vhistory[]
	if ((s->cfg_dbgmask|s->cfg_logmask) & (DBGMASK_VOLTAGE|DBGMASK_ALL_ON_EVENT)) {
		char *str = line_buff;
		str += sprintf(str, "seq%d: vhistory[..%d]\n", s->seq, s->seq%VHISTORY_TOTAL);
		for (i=0; i< VHISTORY_TOTAL; i++) {
			str += sprintf(str, " %d", s->vhistory[(s->seq+1+i)%VHISTORY_TOTAL]);
			if (i%32 == 31)
				str += sprintf(str, "\n");
		}
		if (i%32 != 0)
			str += sprintf(str, "\n");

		str += sprintf(str, "vhistory stable: learned min/max=%d/%d(abs=%u/%u)v, stamp=%s, time=%dms, drop_time=%dms, drop=%dv\n",
			s->vhistory_stable_learned_min, s->vhistory_stable_learned_max,
			s->vhistory_stable_learned_abs_min, s->vhistory_stable_learned_abs_max,
			timestamp2str(&s->vhistory_stable_timestamp), s->vhistory_stable_time, s->vhistory_stable_drop_time, s->vhistory_stable_drop_amplitude);
		str += sprintf(str, "vhistory_stable: onhook_voltage=%dv, onhook2offhook_timestamp=%s(time=%d), offhook_reversal_timestamp=%s(time=%d)\n",
			s->vhistory_stable_onhook_voltage,
			timestamp2str(&s->vhistory_stable_onhook2offhook_timestamp), s->vhistory_stable_onhook2offhook_time,
			timestamp2str(&s->vhistory_stable_offhook_reversal_timestamp), s->vhistory_stable_offhook_reversal_time);
		dbg_log_buff_add_line(DBGMASK_VOLTAGE|DBGMASK_ALL_ON_EVENT, s->cfg_dbgmask, s->cfg_logmask, line_buff);
	}
}

static void
state_update_vdrop(Daa_state *s, Daa_reg *daa_reg)
{
	static char line_buff[1000];
	int is_reinit_vdrop = 0, is_add_new_vdrop = 0;
	int v, v_last;
	int i;

	if (daa_reg->fxo_ctrl1 & (RDT|RDTP|RDTN)) {	// check RDT, RDTP, RDTN for sine wave
		if (daa_reg->fxo_ctrl1 & RDT) {
			s->vdrop[0] = (s->cfg_voltage_amplitude_onhook_max + s->cfg_voltage_amplitude_onhook_min)/2;
			s->vdrop_timestamp[0] = s->now_timestamp;
			s->vdrop_total = 1;

			s->vdrop_onhook_count = 1;
			s->vdrop_between_count = s->vdrop_offhook_count = 0;
			s->vdrop_time = s->vdrop_offhook_time = 0;
			s->vdrop_amplitude = s->vdrop_offhook_amplitude = 0;
			s->vdrop_is_dropout = 0;
			s->vdrop_age_sec= 0;
		}

	} else {
		v = abs(s->line_voltage);
		/* This is to avoid the drop up case that small voltage case            */
#if 0
		if (v >= s->cfg_voltage_amplitude_offhook_min &&
			abs(s->line_voltage_prev) >= s->cfg_voltage_amplitude_offhook_min &&
			s->line_voltage * s->line_voltage_prev <0) {
#else
		/* But there are a case of reversed is like 10 9 -2 -9 -10              */
		/* This case will not reinit vdrop then error detected it to drop       */
		if (s->line_voltage * s->line_voltage_prev <0) {
#endif
			is_reinit_vdrop = 1;		// clear vdrop[] because voltage reversed and large enough
			s->internal_event_detected++;	// display debug for vdrop[] cleared by the reversed voltage 

		} else if (s->vdrop_total) {
			v_last = s->vdrop[s->vdrop_total-1];
			if ( v < v_last)
				is_add_new_vdrop = 1;
			else if (v > v_last + s->cfg_vdrop_ripple_amplitude_diff)
				is_reinit_vdrop = 1;	// clear vdrop[] because ripple too large
		} else {
			is_reinit_vdrop = 1;
		}

		// update vdrop[]
		if (is_reinit_vdrop) {
			// vdrop array reinit
			s->vdrop[0] = v;
			s->vdrop_timestamp[0] = s->now_timestamp;
			s->vdrop_total = 1;
		} else if (is_add_new_vdrop) {
			// add one to vdrop array
			if (s->vdrop_total < VDROP_TOTAL) {
				s->vdrop[s->vdrop_total] = v;
				s->vdrop_timestamp[s->vdrop_total] = s->now_timestamp;
				s->vdrop_total++;
			}
		}

		// check if vdrop[] result is dropout
		s->vdrop_time = ts_diff_ms(s->vdrop_timestamp[0], s->vdrop_timestamp[s->vdrop_total-1]);
		s->vdrop_amplitude = abs(s->vdrop[0] - s->vdrop[s->vdrop_total-1]);
		s->vdrop_offhook_time = 0;
		s->vdrop_offhook_amplitude = 0;
		for (i=0; i< s->vdrop_total; i++) {
			if (s->vdrop[i] <= s->cfg_voltage_amplitude_offhook_max) {
				s->vdrop_offhook_time = ts_diff_ms(s->vdrop_timestamp[i], s->vdrop_timestamp[s->vdrop_total-1]);
				s->vdrop_offhook_amplitude = abs(s->vdrop[i] - s->vdrop[s->vdrop_total-1]);
				break;
			}
		}
		s->vdrop_age_sec = ts_diff_sec(s->vdrop_timestamp[s->vdrop_total-1], s->now_timestamp);
		if (s->vdrop_time >= s->cfg_vdrop_dropout_confirm_time &&		// capacitor effect detection since onhook range
		    s->vdrop_amplitude >= s->cfg_vdrop_dropout_amplitude_drop &&
		    s->vdrop_total >= s->cfg_vdrop_dropout_count) {
			s->vdrop_is_dropout = 1;
		} else if (s->vdrop_offhook_time >= s->cfg_vdrop_dropout_offhook_confirm_time &&	// capacitor effect detection since offhook range
			   s->vdrop_offhook_amplitude >= s->cfg_vdrop_dropout_offhook_amplitude_drop) {
			s->vdrop_is_dropout = 2;
		} else {
			s->vdrop_is_dropout = 0;
		}

		// update vdrop_onhook_count, vdrop_between_count, vdrop_offhook_count
		s->vdrop_onhook_count = s->vdrop_between_count = s->vdrop_offhook_count = 0;
		for (i=0; i< s->vdrop_total; i++) {
			if (s->vdrop[i] >= s->cfg_voltage_amplitude_onhook_min)
				s->vdrop_onhook_count++;
			else if (s->vdrop[i] > s->cfg_voltage_amplitude_offhook_max)
				s->vdrop_between_count++;
			else	// we count offhook and under offhook together
				s->vdrop_offhook_count++;
		}
	}
	// print vdrop[]
	if ((s->cfg_dbgmask|s->cfg_logmask) & (DBGMASK_VOLTAGE|DBGMASK_ALL_ON_EVENT)) {
		char *str = line_buff;
		str += sprintf(str, "seq%d: vdrop[0..%d]", s->seq, s->vdrop_total-1);
		for (i=0; i< s->vdrop_total; i++)
			str += sprintf(str, " %d", s->vdrop[i]);
		str += sprintf(str, ", (onhook=%d between=%d offhook=%d)\n",
			s->vdrop_onhook_count, s->vdrop_between_count, s->vdrop_offhook_count);
		str += sprintf(str, "vdrop total=%d, time=%dms, amplitude=%dv, offhook_time=%dms, offhook_amplitude=%dv, is_dropout=%d (%ds ago)\n",
			s->vdrop_total,
			s->vdrop_time, s->vdrop_amplitude, s->vdrop_offhook_time, s->vdrop_offhook_amplitude,
			s->vdrop_is_dropout, s->vdrop_age_sec);
		dbg_log_buff_add_line(DBGMASK_VOLTAGE|DBGMASK_ALL_ON_EVENT, s->cfg_dbgmask, s->cfg_logmask, line_buff);
	}
}

static void
state_update_line_current(Daa_state *s)
{
	static char line_buff[1000];

	// update loop current max/min only if it is large enough //////////////////////////////
	if (s->loop_current >= s->cfg_loop_current_valid_min &&					// only admit loop current >= 3*1.1ma
	    s->loop_current <= s->cfg_loop_current_valid_max &&					// only admit loop current <= 30*1.1ma
	    abs(s->loop_current-s->loop_current_prev) <= s->cfg_loop_current_stable_diff) {	// only admit stable loop current (diff with prev <=3*1.1ma)
		int learned_loop_current_max_orig = s->learned_loop_current_max;
		int learned_loop_current_min_orig = s->learned_loop_current_min;

		if (s->learned_loop_current_max > s->loop_current && s->loop_current > s->learned_loop_current_min) {
			if (s->learned_loop_current_max-s->loop_current < s->loop_current-s->learned_loop_current_min)
				s->learned_loop_current_max = (s->learned_loop_current_max + s->loop_current)/2;
			else if (s->learned_loop_current_max-s->loop_current > s->loop_current-s->learned_loop_current_min)
				s->learned_loop_current_min = (s->loop_current + s->learned_loop_current_min)/2;
		} else {
			if (s->learned_loop_current_max == 0 || s->loop_current > s->learned_loop_current_max)
				s->learned_loop_current_max = s->loop_current;	// the current that ony DAA is offhook
			if (s->learned_loop_current_min == 0 || s->loop_current < s->learned_loop_current_min)
				s->learned_loop_current_min = s->loop_current;	// the current that both DAA and Hanset are offhook
		}

		if (s->learned_loop_current_max != learned_loop_current_max_orig ||
		    s->learned_loop_current_min != learned_loop_current_min_orig) {
		    	if ((s->cfg_dbgmask|s->cfg_logmask) & (DBGMASK_CURRENT|DBGMASK_ALL_ON_EVENT)) {
				sprintf(line_buff, "seq%d: current=%d(min:%d-max:%d -> min:%d-max:%d)\n",
					s->seq, s->loop_current, learned_loop_current_min_orig, learned_loop_current_max_orig, s->learned_loop_current_min, s->learned_loop_current_max);
				dbg_log_buff_add_line(DBGMASK_CURRENT|DBGMASK_ALL_ON_EVENT, s->cfg_dbgmask, s->cfg_logmask, line_buff);
			}
		}
	}
}

/**
 * @fn unsigned int fv_state_machine_daa(int dev_fd)
 * @brief this function uses to check the daa system state change. Only
 *        if system state change, this function will return an event
 *        number(like ringing, polarity reverse, otherwise it will return
 *        invalid event number
 * @param  dev_fd is SPI file descriptor
 * @return  unsigned int
 */
unsigned int
fv_state_machine_daa(Daa_state *s)
{
	static char line_buff[1000];

	int hook_state;
	char *hook_reason;
	static char hook_reason_buff[1000];
	unsigned int daa_eventmask = 0;

	//static Daa_state *s = NULL;
	static Daa_reg *daa_reg = NULL;
	if (daa_reg == NULL) {
		daa_reg = malloc(sizeof(Daa_reg));
	}
	dbg_log_buff_init();

	s->seq++;
	clock_gettime(CLOCK_MONOTONIC, &s->now_timestamp);
	s->internal_event_detected = 0;

	// load register values  /////////////////////////////////////////////////////
	load_daa_registers(s, daa_reg);

	// keep previous voltage/current
	s->loop_current_prev = s->loop_current;
	s->line_voltage_prev = s->line_voltage;

	// load new voltage/current
	s->loop_current = daa_reg->loop_current;
	s->line_voltage = daa_reg->line_voltage;
	if (s->line_voltage > 127)
		s->line_voltage -= 256;

	// the following 2 state_update will handle the RDT/RDTP/RDTN cases internally
	state_update_vhistory(s, daa_reg);
	state_update_vdrop(s, daa_reg);
	state_update_line_current(s);

	// RING judgement ////////////////////////////////////////////////////////////

	// IMPORTANT!
	// ring judgement must be done before other parts
	// because the line voltage can be interpreted as DC offset only if ringing is not happening

	if (daa_reg->intr_source & (1<<7)) {
		unsigned int msdiff = ts_diff_ms(s->ringon_timestamp, s->now_timestamp);
		if (daa_reg->fxo_ctrl1 & RDT) {	// HW ring detected
			s->ringon_timestamp = s->now_timestamp;
			if (s->is_ringing >0) {
				s->is_ringing++;
				if ((s->cfg_dbgmask|s->cfg_logmask) & (DBGMASK_RING|DBGMASK_ALL_ON_EVENT)) {
					s->ringoff_cadence = msdiff - s->ringon_cadence;
					sprintf(line_buff, "seq%d: RingOn %d, ringoff_cadence=%ums\n", s->seq, s->is_ringing, s->ringoff_cadence);
					dbg_log_buff_add_line(DBGMASK_RING|DBGMASK_ALL_ON_EVENT, s->cfg_dbgmask, s->cfg_logmask, line_buff);
					s->internal_event_detected++;
				}
			} else {
				s->is_ringing = 1;
				s->ringoff_voltage = 0;
				s->ringon_time = s->ringoff_time = s->ringon_cadence = s->ringoff_cadence = 0;
				if ((s->cfg_dbgmask|s->cfg_logmask) & (DBGMASK_RING||DBGMASK_ALL_ON_EVENT)) {
					sprintf(line_buff, "seq%d: RingOn %d, %ums since last RingOn\n", s->seq, s->is_ringing, msdiff);
					dbg_log_buff_add_line(DBGMASK_RING|DBGMASK_ALL_ON_EVENT, s->cfg_dbgmask, s->cfg_logmask, line_buff);
				}
				daa_eventmask |= DAA_EVENTMASK_RING_START;

				// when a ring event happens, the reversal timeout should be canceled,
				// as voip would relay handset back to S port if RingStop happens
				s->reversal_timestamp = zero_timestamp;
				daa_eventmask &= (~DAA_EVENTMASK_REVERSAL_TIMEOUT);
			}
		} else {
			s->ringoff_timestamp = s->now_timestamp;
			if (msdiff > s->ringon_cadence) {
				// update ringon_cadence if it is longer than the old value
				s->ringon_cadence = msdiff;
			} else if (abs(s->line_voltage) >= s->cfg_voltage_amplitude_onhook_min &&
			           s->ringoff_voltage * s->line_voltage >0) {
				// update ringon_cadence if a complete ringon is detected
				// a. the ringoff voltage should be in onhook range (so the ring is not stopped by handset offhook)
				// b. voltage polarity is not reversed (so the ring is not stopped by remote caller)
				s->ringon_cadence = msdiff;
			}
			if ((s->cfg_dbgmask|s->cfg_logmask) & (DBGMASK_RING|DBGMASK_ALL_ON_EVENT)) {
				sprintf(line_buff, "seq%d: RingOff %d, ringon_cadence=%ums\n", s->seq, s->is_ringing, s->ringon_cadence);
				dbg_log_buff_add_line(DBGMASK_RING|DBGMASK_ALL_ON_EVENT, s->cfg_dbgmask, s->cfg_logmask, line_buff);
				s->internal_event_detected++;
			}
		}
	}

	if (s->is_ringing) {
		if (daa_reg->fxo_ctrl1 & RDT) {	// HW ring detected
			s->ringon_time = ts_diff_ms(s->ringon_timestamp, s->now_timestamp);
			s->ringing_timestamp = s->now_timestamp;

		} else if ((daa_reg->fxo_ctrl1 & (RDT|RDTP|RDTN)) == 0) {// no sine wave in ringing => ringoff
			// the ringoff_time might be incorrect if the callerid is transferred during ringoff,
			// the voltage would drop to offhook range, transfer callerid from CO to handset, then voltage back to onhook
			// so the time used in transfering callerid will be counted as ringoff_time
			// ps: we use only ringon_time for alert ring recognition, so ringoff_time is not very important
			s->ringoff_time = ts_diff_ms(s->ringoff_timestamp, s->now_timestamp);

			if (s->ringoff_voltage == 0) {
				s->ringoff_voltage = s->line_voltage;
				if ((s->cfg_dbgmask|s->cfg_logmask) & (DBGMASK_RING|DBGMASK_ALL_ON_EVENT)) {
					sprintf(line_buff, "seq%d: Ring %d, ringoff_voltage=%dv\n", s->seq, s->is_ringing, s->ringoff_voltage);
					dbg_log_buff_add_line(DBGMASK_RING|DBGMASK_ALL_ON_EVENT, s->cfg_dbgmask, s->cfg_logmask, line_buff);
				}
			} else if (s->ringoff_voltage * s->line_voltage <0 &&
				   abs(abs(s->line_voltage)-abs(s->ringoff_voltage)) <= s->cfg_voltage_amplitude_reversal_diff &&
				   s->ringoff_time > s->cfg_ignore_ring_stop_reversed_check_time) {
				// ringoff_voltage reversed to onhook voltage, ring is stopped by remote caller, report RING_STOP immediately
				if ((s->cfg_dbgmask|s->cfg_logmask) & (DBGMASK_RING|DBGMASK_ALL_ON_EVENT)) {
					sprintf(line_buff, "seq%d: Ring %d Stop by caller, ringoff_voltage reversed to onhook %dv->%dv\n", s->seq, s->is_ringing, s->ringoff_voltage, s->line_voltage);
					dbg_log_buff_add_line(DBGMASK_RING|DBGMASK_ALL_ON_EVENT, s->cfg_dbgmask, s->cfg_logmask, line_buff);
				}
				s->ringing_timestamp = zero_timestamp;
				s->is_ringing = 0;
				daa_eventmask |= DAA_EVENTMASK_RING_STOP;
			}
		}
		// if ringon_time is longer than cfg_alert_ringon_time_max
		// we dont need to wait till the ringoff intr to calc ringon_cadence to know the ring is not alert ring
		if (s->ringon_time <= s->cfg_alert_ringon_time_max && s->ringon_cadence <= s->cfg_alert_ringon_time_max)
		    	s->is_alert_ring = 1;
		else
			s->is_alert_ring = 0;

		// check ring timeout (RingOn not happened after last RingOff), which means RING STOP
		if (! is_ts_zero(s->ringing_timestamp)) {
			unsigned int msdiff = ts_diff_ms(s->ringing_timestamp, s->now_timestamp);
			unsigned int cfg_ring_timeout = (s->is_alert_ring)?s->cfg_alert_ring_timeout:s->cfg_normal_ring_timeout;
			if (msdiff > cfg_ring_timeout) {
				if ((s->cfg_dbgmask|s->cfg_logmask) & (DBGMASK_RING|DBGMASK_ALL_ON_EVENT)) {
					sprintf(line_buff, "seq%d: Ring %d Timeout, msdiff=%u(last Ringing till now)\n", s->seq, s->is_ringing, msdiff);
					dbg_log_buff_add_line(DBGMASK_RING|DBGMASK_ALL_ON_EVENT, s->cfg_dbgmask, s->cfg_logmask, line_buff);
				}
				s->ringing_timestamp = zero_timestamp;
				s->is_ringing = 0;
				daa_eventmask |= DAA_EVENTMASK_RING_STOP;
			}
		}

		if ((s->cfg_dbgmask|s->cfg_logmask) & (DBGMASK_RING|DBGMASK_ALL_ON_EVENT)) {
			sprintf(line_buff, "is_ringing=%d, is_alert_ring=%d, ringon/ringoff cadence=%d/%dms time=%d/%dms, ringoff_voltage=%dv\n",
				s->is_ringing, s->is_alert_ring,
				s->ringon_cadence, s->ringoff_cadence, s->ringon_time, s->ringoff_time,
				s->ringoff_voltage);
			dbg_log_buff_add_line(DBGMASK_RING|DBGMASK_ALL_ON_EVENT, s->cfg_dbgmask, s->cfg_logmask, line_buff);
		}
	}

	// LINE_REVERSAL judgement ////////////////////////////////////////////////////////////

	// line reversal is to notify voip a call would come in PSTN line, so voip can relay handset on O port
	// line reversal timeout is to notify voip that call didnt happen, so voip can relay handset back to S port
	// when a ring event happens, the reversal timeout should be canceled, as voip would relay handset back to S port if RingStop happens
	// when a offhook happens, the reversal timeout should be canceled, as voip would relay handset back to S port if handset Onhook happens

	// verify if reversal timeout happens
	if (! is_ts_zero(s->reversal_timestamp)) {
		unsigned int msdiff = ts_diff_ms(s->reversal_timestamp, s->now_timestamp);
		if (msdiff > s->cfg_reversal_timeout) {
			s->reversal_timestamp = zero_timestamp;
			if ((s->cfg_dbgmask|s->cfg_logmask) & (DBGMASK_REVERSAL|DBGMASK_ALL_ON_EVENT)) {
				sprintf(line_buff, "seq%d: line reversal timeout (%dms)\n", s->seq, msdiff);
				dbg_log_buff_add_line(DBGMASK_REVERSAL|DBGMASK_ALL_ON_EVENT, s->cfg_dbgmask, s->cfg_logmask, line_buff);
			}
			daa_eventmask |= DAA_EVENTMASK_REVERSAL_TIMEOUT;
		}
	}

	// RDTP||RDTN on but RDT is not set, could be
	//      a. line reversal
	//	b. start of ring-on cycle
	//	c. pstn line plug-in or plug-out
	// since we can not exclude case c, so wo dont use (RDTP||RDTN ) && !RDT for reversal judgement
	// we use volatge value instead with case RDT/RDTP/RDTN excluded
	if ((daa_reg->fxo_ctrl1 & (RDT|RDTP|RDTN)) == 0) {	// exclude RDT, RDTP, RDTN for sine wave
		if (s->line_voltage*s->line_voltage_prev <0) {	// one is positive & another is negative
			int v1 = abs(s->line_voltage_prev);
			int v2 = abs(s->line_voltage);
			if (v1 >= s->cfg_voltage_amplitude_onhook_min && v1 < s->cfg_voltage_amplitude_onhook_max &&
			    v2 >= s->cfg_voltage_amplitude_onhook_min && v2 < s->cfg_voltage_amplitude_onhook_max &&
			    abs(v1-v2) <= s->cfg_voltage_amplitude_reversal_diff) {
				s->reversal_timestamp = s->now_timestamp;
				if ((s->cfg_dbgmask|s->cfg_logmask) & (DBGMASK_REVERSAL|DBGMASK_ALL_ON_EVENT)) {
					sprintf(line_buff, "seq%d: line reversal detected (%dv->%dv)\n", s->seq, s->line_voltage_prev, s->line_voltage);
					dbg_log_buff_add_line(DBGMASK_REVERSAL|DBGMASK_ALL_ON_EVENT, s->cfg_dbgmask, s->cfg_logmask, line_buff);
				}
				daa_eventmask |= DAA_EVENTMASK_REVERSAL;
			}
		}
	}

	// ONHOOK/OFFHOOK/DROPOUT judgement ////////////////////////////////////////////////////////////

	hook_state = s->hook_state_prev;
	hook_reason = "-";

	if (daa_reg->fxo_ctrl1 & RDT) {
		s->voltage_onhook_timestamp = zero_timestamp;
		s->voltage_offhook_timestamp = zero_timestamp;

		// RDT: a ring sine wave both positive & negative detected => onhook
		hook_state = STATE_DAA_ONHOOK_HANDSET_ONHOOK;
		hook_reason = "RDT, Ring Detected by hw";

	} else if (daa_reg->fxo_ctrl1 & (RDTP|RDTN)) {
		s->voltage_onhook_timestamp = zero_timestamp;
		s->voltage_offhook_timestamp = zero_timestamp;

		// RDTN, RDTP on: could be either line reversal or ring-on or ripple caused by plugin/plugout
		if ((s->cfg_dbgmask|s->cfg_logmask) & (DBGMASK_HOOKSTATE|DBGMASK_ALL_ON_EVENT)) {
			sprintf(line_buff, "seq%d:%s%s, transient sine wave, voltage ignored\n",
				s->seq, (daa_reg->fxo_ctrl1&RDTN)?" RDTN":"", (daa_reg->fxo_ctrl1&RDTP)?" RDTP":"");
			dbg_log_buff_add_line(DBGMASK_HOOKSTATE|DBGMASK_ALL_ON_EVENT, s->cfg_dbgmask, s->cfg_logmask, line_buff);
		}

	} else if (s->vdrop_is_dropout && s->vdrop_age_sec <= s->cfg_vdrop_timeout/1000) { // voltage drops slowly because of capacitor effect => dropout
		s->voltage_onhook_timestamp = zero_timestamp;
		s->voltage_offhook_timestamp = zero_timestamp;

		hook_state = STATE_DROPOUT;
		if (s->vdrop_is_dropout == 2)
			hook_reason = "V drops widely in offhook range";
		else
			hook_reason = "V drops slowly";

	// we exclude following 3 cases in above
	// a. sine wave caused by ringing
	// b. transient sine wave caused by line reversal or pstn line plugin/plugout
	// c. slowly dropping voltage caused by pstn line dropout
	//
	// then try to figure out the hookstate based on the voltage value and tendency since below

	} else if (abs(s->line_voltage) >= s->cfg_voltage_amplitude_onhook_min) { // curr voltage is onhook
		int is_onhook_confirmed = 0;
		unsigned int msdiff;

		s->voltage_offhook_timestamp = zero_timestamp;
		if (s->hook_state_prev != STATE_DAA_ONHOOK_HANDSET_ONHOOK &&
		    is_ts_zero(s->voltage_onhook_timestamp)) {
			s->voltage_onhook_timestamp = s->now_timestamp;
			if ((s->cfg_dbgmask|s->cfg_logmask) & (DBGMASK_HOOKSTATE|DBGMASK_ALL_ON_EVENT)) {
				sprintf(line_buff, "seq%d: %dv->%dv(onhook) set onhook start timer\n", s->seq, s->line_voltage_prev, s->line_voltage);
				dbg_log_buff_add_line(DBGMASK_HOOKSTATE|DBGMASK_ALL_ON_EVENT, s->cfg_dbgmask, s->cfg_logmask, line_buff);
				s->internal_event_detected++;
			}
		}

		msdiff = ts_diff_ms(s->voltage_onhook_timestamp, s->now_timestamp);
		if (s->hook_state_prev == STATE_DROPOUT) {
			// trigger state change if onhook voltage remains over dropout2onhook_confirm_time (>150ms)
			// ps: this short time is to let hardware have time to detect the RDT bit if the PSTN is plugged in while ringing
			//     a sine wave ring of 20hz will takes 50ms, and the hw ring validation waits 100ms to generate thr RDTI interrupt
			if (msdiff >= s->cfg_dropout2onhook_confirm_time)
				is_onhook_confirmed = 1;
		} else {
			// trigger state change if onhook voltage remains over offhook2onhook confirm time (2sec)
			if (msdiff >= s->cfg_offhook2onhook_confirm_time)
				is_onhook_confirmed = 1;
		}
		if (is_onhook_confirmed) {
			s->learned_onhook_voltage = s->line_voltage;
			// either DAA or HANDSET offhook would set voltage offhook, so we know both DAA/HANDSET are onhook
			hook_state = STATE_DAA_ONHOOK_HANDSET_ONHOOK;
			sprintf(hook_reason_buff, "%dv->%dv(onhook) remains %d.%03dsec",
		    		s->line_voltage_prev, s->line_voltage, msdiff/1000, msdiff%1000);
			hook_reason = hook_reason_buff;
		}

	} else if (abs(s->line_voltage) > s->cfg_voltage_amplitude_offhook_max) { // curr voltage is between onhook/offhook
		s->voltage_onhook_timestamp = zero_timestamp;
		s->voltage_offhook_timestamp = zero_timestamp;

		if ((s->cfg_dbgmask|s->cfg_logmask) & (DBGMASK_HOOKSTATE|DBGMASK_ALL_ON_EVENT)) {
			sprintf(line_buff, "seq%d: %dv(between onhook/offhook)\n", s->seq, s->line_voltage);
			dbg_log_buff_add_line(DBGMASK_HOOKSTATE|DBGMASK_ALL_ON_EVENT, s->cfg_dbgmask, s->cfg_logmask, line_buff);
		}

	} else if (abs(s->line_voltage) >= s->cfg_voltage_amplitude_offhook_min) { // curr voltage is offhook
		int is_offhook_confirmed = 0;

		s->voltage_onhook_timestamp = zero_timestamp;
		if (s->hook_state_prev != STATE_DAA_ONHOOK_HANDSET_OFFHOOK &&
		    s->hook_state_prev != STATE_DAA_OFFHOOK_HANDSET_ONHOOK &&
		    s->hook_state_prev != STATE_DAA_OFFHOOK_HANDSET_OFFHOOK &&
		    is_ts_zero(s->voltage_offhook_timestamp)) {
			s->voltage_offhook_timestamp = s->now_timestamp;
			if ((s->cfg_dbgmask|s->cfg_logmask) & (DBGMASK_HOOKSTATE|DBGMASK_ALL_ON_EVENT)) {
				sprintf(line_buff, "seq%d: %dv->%dv(offhook) set offhook start timer\n", s->seq, s->line_voltage_prev, s->line_voltage);
				dbg_log_buff_add_line(DBGMASK_HOOKSTATE|DBGMASK_ALL_ON_EVENT, s->cfg_dbgmask, s->cfg_logmask, line_buff);
				s->internal_event_detected++;
			}
		}

		// voltage comes to offhook from onhook
		if (s->hook_state_prev == STATE_DAA_ONHOOK_HANDSET_ONHOOK) {
			// the voltage remain stable in offhook for a while
			// and the tranient time from stable ohook voltage to stable offhook voltage is short
			// ps: if reversal happens within the transient_time, the offhook_reversal_time could be minus from transient_time
			if (s->vhistory_stable_time >= s->cfg_vhistory_onhook2offhook_stable_time &&
			    s->vhistory_stable_onhook2offhook_time > 0 &&	// 0 means stable offhook voltage not found in vhistory
			    s->vhistory_stable_onhook2offhook_time-s->vhistory_stable_offhook_reversal_time <= s->cfg_vhistory_onhook2offhook_transient_time) {

				unsigned int msdiff = ts_diff_ms(s->voltage_offhook_timestamp, s->now_timestamp);
				char *offhook_str;
				if (daa_eventmask & DAA_EVENTMASK_RING_STOP) {
					is_offhook_confirmed = 1;
					if (s->is_alert_ring) {
						offhook_str = "alert ring stop";
					} else {
						offhook_str = "normal ring stop";
					}
				} else if (s->is_ringing) {
					if (s->vhistory_stable_drop_time > s->cfg_vhistory_stable_drop_time_max) {
						// ignore a offhook voltage if it continuously drops with total diff over 2v for a while
						if ((s->cfg_dbgmask|s->cfg_logmask) & (DBGMASK_HOOKSTATE|DBGMASK_ALL_ON_EVENT)) {
							sprintf(line_buff, "seq%d: stable_drop_amplitude %d, stable_drop_time %d, %s ring offhook ignored!\n",
								s->seq, s->vhistory_stable_drop_amplitude, s->vhistory_stable_drop_time, s->is_alert_ring?"alert":"normal");
							dbg_log_buff_add_line(DBGMASK_HOOKSTATE|DBGMASK_ALL_ON_EVENT, s->cfg_dbgmask, s->cfg_logmask, line_buff);
							s->internal_event_detected++;
						}
					} else {
						if (s->is_alert_ring) {
							if (msdiff >= s->cfg_alert_ring_offhook_confirm_time) {
								is_offhook_confirmed = 1;
								offhook_str = "alert ring";
							}
						} else {
							if (msdiff >= s->cfg_normal_ring_offhook_confirm_time) {
								is_offhook_confirmed = 1;
								offhook_str = "normal ring";
							}
						}
					}
				} else {
					if (msdiff >= s->cfg_active_offhook_confirm_time) {
						is_offhook_confirmed = 1;
						offhook_str = "active";
					}
				}
				if (is_offhook_confirmed) {
					sprintf(hook_reason_buff, "%dv(%s offhook) remains stable(%dv-%dv) %d.%03dsec",
						s->line_voltage, offhook_str, s->vhistory_stable_learned_min, s->vhistory_stable_learned_max,
						s->vhistory_stable_time/1000, s->vhistory_stable_time%1000);
				}
			}

		} else if (s->hook_state_prev == STATE_DROPOUT || s->hook_state_prev == STATE_INIT) {
			if (s->vhistory_stable_time >= s->cfg_vhistory_dropout2offhook_stable_time) {
				is_offhook_confirmed = 1;
				sprintf(hook_reason_buff, "%dv(offhook) remains stable(%dv-%dv) %d.%03dsec",
					s->line_voltage, s->vhistory_stable_learned_min, s->vhistory_stable_learned_max,
					s->vhistory_stable_time/1000, s->vhistory_stable_time%1000);
			}
		} else {
			// DAA_ONHOOK_HANDSET_OFFHOOK || DAA_OFFHOOK_HANDSET_ONHOOK || DAA_OFFHOOK_HANDSET_OFFHOOK
			if (s->loop_current > 0 && s->loop_current_prev == 0) {
				is_offhook_confirmed = 0;	// although current changed, we skip the first read non-zero loop current, it might be not stable
				s->internal_event_detected++;
			} else {
				is_offhook_confirmed = 1;
				sprintf(hook_reason_buff, "%dv(offhook) loop current(%d->%d) changes",
					s->line_voltage, s->loop_current_prev, s->loop_current);
			}
		}

		if (is_offhook_confirmed) {
			hook_reason = hook_reason_buff;

			// when a offhook happens, the reversal timeout should be canceled,
			// as voip would relay handset back to S port if handset Onhook happens
			s->reversal_timestamp = zero_timestamp;
			daa_eventmask &= (~DAA_EVENTMASK_REVERSAL_TIMEOUT);
			// when a offhook happens, the ring timeout should be canceled,
			// as voip would relay handset back to S port if handset Onhook happens
			s->ringing_timestamp = zero_timestamp;
			s->is_ringing = 0;
			daa_eventmask &= (~DAA_EVENTMASK_RING_STOP);

			s->learned_offhook_voltage = s->line_voltage;

			// if loop current is not zero, then hw daa offhook op should be 1
			// if loop current is zero, then hw daa offhook op should be 0
			// when loop current is not syncronized with offhook op, 
			// we assume it is a transient state, ignore it and do not chnage hook_state
			if (s->loop_current) {
				if (s->loop_current >= s->cfg_loop_current_valid_min &&					// only admit loop current >= 3*1.1ma
				    s->loop_current <= s->cfg_loop_current_valid_max &&					// only admit loop current <= 30*1.1ma
				    abs(s->loop_current-s->loop_current_prev) <= s->cfg_loop_current_stable_diff) {	// only admit stable loop current (diff with prev <=3*1.1ma)
					// DAA offhook is confirmed only if detected loop_current > 0 && hw daa offhook op is set
					if (daa_reg->fxo_ctrl1 & DAA_OFFHOOK_OP) {
						if (s->learned_loop_current_max - s->loop_current
						    < s->loop_current - s->learned_loop_current_min) {
							// current near max, only daa offhook
							hook_state = STATE_DAA_OFFHOOK_HANDSET_ONHOOK;
						} else {
							// current near min, both daa & handset offhook
							hook_state = STATE_DAA_OFFHOOK_HANDSET_OFFHOOK;
						}
					}
				}
			} else {
				// DAA onhook is confirmed only if detected loop_current == 0 && hw daa offhook op is not set
				if ((daa_reg->fxo_ctrl1 & DAA_OFFHOOK_OP) == 0) {
					// DAA onhook, handset offhook
					hook_state = STATE_DAA_ONHOOK_HANDSET_OFFHOOK;
				}
			}
		}

	} else {	// curr voltage is under offhook
		s->voltage_onhook_timestamp = zero_timestamp;
		s->voltage_offhook_timestamp = zero_timestamp;

		if (s->hook_state_prev != STATE_DROPOUT) {
			if (s->vhistory_stable_time >= s->cfg_vhistory_offhook2dropout_stable_time &&
			    s->vhistory_stable_learned_max < s->cfg_voltage_amplitude_offhook_min &&
			    s->vhistory_stable_learned_min >= 0) {
				hook_state = STATE_DROPOUT;
				sprintf(hook_reason_buff, "%dv(under offhook) remains stable(%dv-%dv) %d.%03dsec",
					s->line_voltage, s->vhistory_stable_learned_min, s->vhistory_stable_learned_max,
					s->vhistory_stable_time/1000, s->vhistory_stable_time%1000);
				hook_reason = hook_reason_buff;
			}
		}
	}

	if (hook_state != s->hook_state_prev) {
		if ((s->cfg_dbgmask|s->cfg_logmask) & (DBGMASK_HOOKSTATE|DBGMASK_ALL_ON_EVENT)) {
			sprintf(line_buff, "seq%d: State %s -> %s, %s\n", s->seq, hook_state_str[s->hook_state_prev], hook_state_str[hook_state], hook_reason);
			dbg_log_buff_add_line(DBGMASK_HOOKSTATE|DBGMASK_ALL_ON_EVENT, s->cfg_dbgmask, s->cfg_logmask, line_buff);
		}

		// judge connect/disconnect
		if (hook_state == STATE_DROPOUT) {
			if (s->hook_state_prev != STATE_DROPOUT)
				daa_eventmask |= DAA_EVENTMASK_DISCONNECT;
		} else {

			if (s->hook_state_prev == STATE_DROPOUT || s->hook_state_prev == STATE_INIT) {
				if (fv_state_machine_daa_get_loop_current(s, 20) == 0) {	// this function need 20ms to detect PSTN disconnect
					sprintf(line_buff, "judge to %s but the current is 0 change to DROPOUT\n", hook_state_str[hook_state]);
					dbg_log_buff_add_line(DBGMASK_ALL_ON_EVENT, s->cfg_dbgmask, s->cfg_logmask, line_buff);
					hook_state = STATE_DROPOUT;
				} else {
					daa_eventmask |= DAA_EVENTMASK_CONNECT;
				}
			}
		}
		// judge handset offhook/onhook
		if (!(s->hook_state_prev == STATE_DAA_ONHOOK_HANDSET_OFFHOOK ||
		      s->hook_state_prev == STATE_DAA_OFFHOOK_HANDSET_OFFHOOK) &&
		     (hook_state == STATE_DAA_ONHOOK_HANDSET_OFFHOOK ||
		      hook_state == STATE_DAA_OFFHOOK_HANDSET_OFFHOOK))
		     	daa_eventmask |= DAA_EVENTMASK_HANDSET_OFFHOOK;
		else if (!(s->hook_state_prev == STATE_DAA_ONHOOK_HANDSET_ONHOOK ||
		           s->hook_state_prev == STATE_DAA_OFFHOOK_HANDSET_ONHOOK) &&
		          (hook_state == STATE_DAA_ONHOOK_HANDSET_ONHOOK ||
		           hook_state == STATE_DAA_OFFHOOK_HANDSET_ONHOOK))
		     	daa_eventmask |= DAA_EVENTMASK_HANDSET_ONHOOK;
		// judge daa offhook/onhook
		if (!(s->hook_state_prev == STATE_DAA_OFFHOOK_HANDSET_ONHOOK ||
		      s->hook_state_prev == STATE_DAA_OFFHOOK_HANDSET_OFFHOOK) &&
		     (hook_state == STATE_DAA_OFFHOOK_HANDSET_ONHOOK ||
		      hook_state == STATE_DAA_OFFHOOK_HANDSET_OFFHOOK))
		     	daa_eventmask |= DAA_EVENTMASK_DAA_OFFHOOK;
		else if (!(s->hook_state_prev == STATE_DAA_ONHOOK_HANDSET_ONHOOK ||
		           s->hook_state_prev == STATE_DAA_ONHOOK_HANDSET_OFFHOOK) &&
		          (hook_state == STATE_DAA_ONHOOK_HANDSET_ONHOOK ||
		           hook_state == STATE_DAA_ONHOOK_HANDSET_OFFHOOK))
		     	daa_eventmask |= DAA_EVENTMASK_DAA_ONHOOK;
	}

	if ((s->cfg_dbgmask|s->cfg_logmask) & (DBGMASK_HOOKSTATE|DBGMASK_ALL_ON_EVENT)) {
		sprintf(line_buff, "%s, is_ringing=%d%s%s%s, is_alert_ring=%d, voltage=%dv(onhook=%dv,offhook=%dv), current=%d(min:%d-max:%d)\n",
			hook_state_str[hook_state], s->is_ringing,
			(daa_reg->fxo_ctrl1&RDTN)?" RDTN":"", (daa_reg->fxo_ctrl1&RDTP)?" RDTP":"", (daa_reg->fxo_ctrl1&RDT)?" RDT":"",
			s->is_alert_ring,
			s->line_voltage, s->learned_onhook_voltage, s->learned_offhook_voltage,
			s->loop_current, s->learned_loop_current_min, s->learned_loop_current_max
			);
		dbg_log_buff_add_line(DBGMASK_HOOKSTATE|DBGMASK_ALL_ON_EVENT, s->cfg_dbgmask, s->cfg_logmask, line_buff);
	}

	if (daa_eventmask && ((s->cfg_dbgmask|s->cfg_logmask) & (DBGMASK_EVENT|DBGMASK_ALL_ON_EVENT))) {
		sprintf(line_buff, "seq%d: daa_eventmask(0x%x) %s\n", s->seq, daa_eventmask, fv_state_machine_daa_mask2str(daa_eventmask));
		dbg_log_buff_add_line(DBGMASK_EVENT|DBGMASK_ALL_ON_EVENT, s->cfg_dbgmask, s->cfg_logmask, line_buff);
	}

	{
		unsigned int dbg_enable = 1;
		unsigned int log_enable = 1;

		// when DBGMASK_ALL_ON_EVENT bit is set, disable print if event and internal event are zero
		if (daa_eventmask == 0 && s->internal_event_detected == 0) {
			if (s->cfg_dbgmask & DBGMASK_ALL_ON_EVENT)
				dbg_enable = 0;
			if (s->cfg_logmask & DBGMASK_ALL_ON_EVENT)
				log_enable = 0;
		}

		dbg_log_buff_flush_print(dbg_enable, log_enable);
	}

	s->hook_state_prev = hook_state;
	s->daa_eventmask_prev = daa_eventmask;

	// check /tmp/daa.cli input
	parse_daa_cli(s, daa_reg);

	// daa_eventmask might contain multiple events for voip
	return daa_eventmask;
}

unsigned int
fv_state_machine_daa_get_eventmask(Daa_state *s)
{
	return s->daa_eventmask_prev;
}

// while daa_eventmask represents the current change of related mask,
// this statemask returns the current state related mask
unsigned int
fv_state_machine_daa_get_statemask(Daa_state *s)
{
	unsigned int daa_statemask = 0;

	if (s->is_ringing)
		daa_statemask |= DAA_EVENTMASK_RING_START;

	if (!is_ts_zero(s->reversal_timestamp))
		daa_statemask |= DAA_EVENTMASK_REVERSAL;

	if (s->hook_state_prev == STATE_DROPOUT)
		 daa_statemask |= DAA_EVENTMASK_DISCONNECT;
	else
		 daa_statemask |= DAA_EVENTMASK_CONNECT;

	// judge handset offhook/onhook
	if (s->hook_state_prev == STATE_DAA_ONHOOK_HANDSET_OFFHOOK ||
	    s->hook_state_prev == STATE_DAA_OFFHOOK_HANDSET_OFFHOOK)
		daa_statemask |= DAA_EVENTMASK_HANDSET_OFFHOOK;
	else if (s->hook_state_prev == STATE_DAA_ONHOOK_HANDSET_ONHOOK ||
		 s->hook_state_prev == STATE_DAA_OFFHOOK_HANDSET_ONHOOK)
		daa_statemask |= DAA_EVENTMASK_HANDSET_ONHOOK;
	// judge daa offhook/onhook
	if (s->hook_state_prev == STATE_DAA_OFFHOOK_HANDSET_ONHOOK ||
	    s->hook_state_prev == STATE_DAA_OFFHOOK_HANDSET_OFFHOOK)
	    	daa_statemask |= DAA_EVENTMASK_DAA_OFFHOOK;
	else if (s->hook_state_prev == STATE_DAA_ONHOOK_HANDSET_ONHOOK ||
		 s->hook_state_prev == STATE_DAA_ONHOOK_HANDSET_OFFHOOK)
		daa_statemask |= DAA_EVENTMASK_DAA_ONHOOK;

	return daa_statemask;
}

char *
fv_state_machine_daa_mask2str(unsigned int mask)
{
	static char maskstr[256];
	char *str = maskstr;
	int i;
	for (i=0; i<10; i++) {
		if (mask & (1<<i))
			str += sprintf(str, "%s%s", (str == maskstr)?"":" ", daa_event_str[i]);
	}
	return maskstr;
}

// below function are for backward compatibility
static int
daa_mask2number(unsigned int eventmask)
{
	// turn eventmask to EVENT_NUMBER for backward compatibility /////////////////////////////////////////////////

	// report connect/disconnect first
	// eg: when voip is in DISCONNECTED state, it wont accept any event other than CONNECTED
	if (eventmask & DAA_EVENTMASK_CONNECT) 		return DAA_EVENT_CONNECT;
	if (eventmask & DAA_EVENTMASK_DISCONNECT) 	return DAA_EVENT_DISCONNECT;

	// report handset status over daa status
	if (eventmask & DAA_EVENTMASK_HANDSET_ONHOOK) 	return DAA_EVENT_HANDSET_ONHOOK;
	if (eventmask & DAA_EVENTMASK_HANDSET_OFFHOOK) 	return DAA_EVENT_HANDSET_OFFHOOK;
	if (eventmask & DAA_EVENTMASK_DAA_ONHOOK) 	return DAA_EVENT_DAA_ONHOOK;
	if (eventmask & DAA_EVENTMASK_DAA_OFFHOOK) 	return DAA_EVENT_DAA_OFFHOOK;

	// report ring/reversal last, as they could be missed without disturbing the voip internal state

	// report ring over reversal since ring is generally the event after reversal,
	// so if ring event happens, there is no need to report reversal event
	if (eventmask & DAA_EVENTMASK_RING_START) 	return DAA_EVENT_RING_START;
	if (eventmask & DAA_EVENTMASK_RING_STOP) 	return DAA_EVENT_RING_STOP;
	if (eventmask & DAA_EVENTMASK_REVERSAL) 	return DAA_EVENT_REVERSAL;
	if (eventmask & DAA_EVENTMASK_REVERSAL_TIMEOUT) return DAA_EVENT_REVERSAL_TIMEOUT;

	return DAA_EVENT_NONE;
}

int
fv_state_machine_daa_get_eventnumber(Daa_state *s)
{
	return daa_mask2number(fv_state_machine_daa_get_eventmask(s));
}

int
fv_state_machine_daa_get_statenumber(Daa_state *s)
{
	unsigned int mask = fv_state_machine_daa_get_statemask(s);
	// remove connect/disconnect from statemask as it would always override others in mask2number()
	mask &= (~(DAA_EVENTMASK_CONNECT|DAA_EVENTMASK_CONNECT));
	return daa_mask2number(mask);
}
