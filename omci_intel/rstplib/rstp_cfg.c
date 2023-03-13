#include <stddef.h>
#include "bitmap.h"
#include "uid_stp.h"
#include "stp_in.h"
#include "stp_to.h"

#include "util.h"



int 
rstp_cfg_stp_enable (void)
{
	UID_STP_CFG_T uid_cfg;
	int rc;

	uid_cfg.field_mask = BR_CFG_STATE;
	uid_cfg.stp_enabled = STP_ENABLED;
	rc = STP_IN_stpm_set_cfg (0, NULL, &uid_cfg);

	if (rc) 
	{
		dbprintf (LOG_ERR,"can't enable: %s\n", STP_IN_get_error_explanation (rc));
		return -1;
	}

	return 0;
}

int rstp_cfg_stp_disable (void)
{
	UID_STP_CFG_T uid_cfg;
	int rc;

	uid_cfg.field_mask = BR_CFG_STATE;
	uid_cfg.stp_enabled = STP_DISABLED;
 	rc = STP_IN_stpm_set_cfg (0, NULL, &uid_cfg);
 	if (rc) 
	{
		dbprintf (LOG_ERR,"can't disable: %s\n", STP_IN_get_error_explanation (rc));
		return -1;
	}

	return 0;
}

static void
rstp_cfg_set_bridge_cfg_value (unsigned long value, unsigned long val_mask)
{
  UID_STP_CFG_T uid_cfg;
  char*         val_name;
  int           rc;

  uid_cfg.field_mask = val_mask;
  switch (val_mask) {
    case BR_CFG_STATE:
      uid_cfg.stp_enabled = value;
      val_name = "state";
      break;
    case BR_CFG_PRIO:
      uid_cfg.bridge_priority = value;
      val_name = "priority";
      break;
    case BR_CFG_AGE:
      uid_cfg.max_age = value;
      val_name = "max_age";
      break;
    case BR_CFG_HELLO:
      uid_cfg.hello_time = value;
      val_name = "hello_time";
      break;
    case BR_CFG_DELAY:
      uid_cfg.forward_delay = value;
      val_name = "forward_delay";
      break;
    case BR_CFG_FORCE_VER:
      uid_cfg.force_version = value;
      val_name = "force_version";
      break;
    case BR_CFG_AGE_MODE:
    case BR_CFG_AGE_TIME:
    default: printf ("Invalid value mask 0X%lx\n", val_mask);  return;
      break;
  }

  rc = STP_IN_stpm_set_cfg (0, NULL, &uid_cfg);

  if (0 != rc) {
    dbprintf (LOG_ERR, "Can't change rstp bridge %s:%s", val_name, STP_IN_get_error_explanation (rc));
  } else {
    dbprintf (LOG_WARNING, "Changed rstp bridge %s\n", val_name);
  }
}

int
rstp_cfg_br_prio_set (unsigned short br_prio)
{
  rstp_cfg_set_bridge_cfg_value ( br_prio, BR_CFG_PRIO);
  return 0;
}

int
rstp_cfg_br_maxage_set (unsigned short br_maxage)
{
  rstp_cfg_set_bridge_cfg_value ( br_maxage, BR_CFG_AGE);
  return 0;
}

int
rstp_cfg_br_hello_time_set( unsigned short br_hello_time)
{
  rstp_cfg_set_bridge_cfg_value ( br_hello_time, BR_CFG_HELLO);
  return 0;
}

int
rstp_cfg_br_fdelay_set ( unsigned short br_fdelay)
{
  rstp_cfg_set_bridge_cfg_value ( br_fdelay, BR_CFG_DELAY);
  return 0;
}

static void
rstp_cfg_set_rstp_port_cfg_value (int port_index,
                         unsigned long value,
                         unsigned long val_mask)
{
  UID_STP_PORT_CFG_T uid_cfg;
  int           rc;
  char          *val_name;

  if (port_index > 0) {
    BitmapClear(&uid_cfg.port_bmp);
    BitmapSetBit(&uid_cfg.port_bmp, port_index - 1);
  } else {
    BitmapSetAllBits(&uid_cfg.port_bmp);
  }

  uid_cfg.field_mask = val_mask;
  switch (val_mask) {
    case PT_CFG_MCHECK:
      val_name = "mcheck";
      break;
    case PT_CFG_COST:
      uid_cfg.admin_port_path_cost = value;
      val_name = "path cost";
      break;
    case PT_CFG_PRIO:
      uid_cfg.port_priority = value;
      val_name = "priority";
      break;
    case PT_CFG_P2P:
      uid_cfg.admin_point2point = (ADMIN_P2P_T) value;
      val_name = "p2p flag";
      break;
    case PT_CFG_EDGE:
      uid_cfg.admin_edge = value;
      val_name = "adminEdge";
      break;
    case PT_CFG_NON_STP:
      uid_cfg.admin_non_stp = value;
      val_name = "adminNonStp";
      break;
#ifdef STP_DBG
    case PT_CFG_DBG_SKIP_TX:
      uid_cfg.skip_tx = value;
      val_name = "skip tx";
      break;
    case PT_CFG_DBG_SKIP_RX:
      uid_cfg.skip_rx = value;
      val_name = "skip rx";
      break;
#endif
    case PT_CFG_STATE:
    default:
      printf ("Invalid value mask 0X%lx\n", val_mask);
      return;
  }

  rc = STP_IN_set_port_cfg (0, &uid_cfg);
  if (0 != rc) {
    printf ("can't change rstp port[s] %s: %s\n",
           val_name, STP_IN_get_error_explanation (rc));
  } else {
    printf ("changed rstp port[s] %s\n", val_name);
  }

  /* show_rstp_port (&uid_cfg.port_bmp, 0); */
}

int
rstp_cfg_prt_prio_set (int port_index ,unsigned long value )
{
  rstp_cfg_set_rstp_port_cfg_value (port_index, value, PT_CFG_PRIO);
  return 0;
}

int
rstp_cfg_prt_pcost_set (int port_index, unsigned long value )
{
  rstp_cfg_set_rstp_port_cfg_value (port_index, value, PT_CFG_COST);
  return 0;
}

int
rstp_cfg_prt_mcheck_set (int port_index )
{
  rstp_cfg_set_rstp_port_cfg_value (port_index, 0, PT_CFG_MCHECK);
  return 0;
}
