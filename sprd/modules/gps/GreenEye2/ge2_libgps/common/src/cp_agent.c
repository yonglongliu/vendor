#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <cutils/log.h>
#include <stdlib.h>
#include <unistd.h>    
#include <sys/stat.h>
#include <string.h>

#include "gps_cmd.h"
#include "sprd_agps_agent.h"
#include "agps_api.h"
#include "gps_common.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define  LOG_TAG  "LIBGPS_AGENT"
#define  E(...)   ALOGE(__VA_ARGS__)
#define  D(...)   ALOGD(__VA_ARGS__)

#ifndef GNSS_LCS
agps_c_plane_interface_t *c_interface;

int lcppayload_cb(agps_dl_pdu_params_t *dl_pdu_params)
{
	//open engine
	GpsState *s = _gps_state;
	D("lcppayload_cb enter\n");
	
	if(agpsdata->cur_state != MSB_SI)
	{
	    E("In cp mode,but maybe up is running:%d",agpsdata->cur_state);
	}
	agpsdata->cur_state = CP_NI;
	
	if(s->gps_flag != CMD_START)
	{
	    D("cp payload,begin gps_start");
	    agpsdata->cp_begin = 1;
	    gps_start();            //note:start and stop must be pair,otherwise maybe wrong!
	    usleep(600000);    //in cp mode,just sleep 500ms
	}
	
	ctrlplane_handleDlData((agps_cp_dl_pdu_params_t *)dl_pdu_params);
	
	return 1;
} 

int lmeas_term_cb(void)
{
	//will be close ,idle on
	GpsState *s = _gps_state;
	D("lmeas_term_cb enter");
	ctrlplane_handleStopSession();
	return 1;
}
int lrrc_state_cb(unsigned int rrc_state)
{   
	ctrlplane_handleRrcStateChange(rrc_state);
	D("lrrc_state_cb is over");
	return 1;
}
int le911notify_cb(void)
{
	return 1;
}
int lcp_reset_cb(short type)
{
	clear_agps_data();
	D("lcp_reset_cb over, type= %d\n",type);
	return 1;
}
int lmtlr_notify_cb(agps_user_notify_params_t *agps_user_notify_params)
{
	agps_cp_notify_params_t *cp_param = (agps_cp_notify_params_t *)agps_user_notify_params;
	E("==>>libgps,notify cb,handle_id=%d,ni_type=%d",cp_param->handle_id,cp_param->ni_type);
	agpsdata->cur_state = CP_NI;
	ctrlplane_handleNotifyAndVerification(*cp_param);
	D("lmtlr_notify_cb over\n");
	return 1;
}

int lmolr_resp_cb(int locationEstimate_len, unsigned char *locationEstimate)
{
    ctrlplane_handleMolrResponse(locationEstimate_len, locationEstimate);
    D("lmolr_resp_cb is over\n");
    return 1;
}

int lagps_pos_resp(PosElementPR_e evt_type, int len, char *rsp_param)
{
    D("lagps_pos_resp enter,call ctrlplane_handleAgpsCposResponse\n");
    ctrlplane_handleAgpsCposResponse(evt_type,len,rsp_param);
    return 1;
}

static agps_c_plane_hook_t  c_hook =
{
	sizeof(agps_c_plane_hook_t),
	lcppayload_cb,
	lmeas_term_cb,
	lrrc_state_cb,
	le911notify_cb,
	lcp_reset_cb,
	lmtlr_notify_cb,
	lmolr_resp_cb, 
	lagps_pos_resp,
};

unsigned long int libgps_c_plane_stack_network_ulreq(agps_cp_ul_pdu_params_t *pParams)
{
	D("libgps_c_plane_stack_network_ulreq is enter\n");
	c_interface->network_ulreq((agps_ul_pdu_params_t *)pParams);
	return 0;
}
unsigned long int libgps_lcsverificationresp(unsigned long notifId,unsigned long present, unsigned long verificationResponse)
{
     D("libgps_lcsverificationresp is enter");
     c_interface->lcsverificationresp(notifId,present,verificationResponse);
     return 0;
}

unsigned long int lcs_molr(agps_molr_params_t *agps_molr_params)
{
    D("lcs molr is enter");
    c_interface->lcs_molr(agps_molr_params);
    return 0;
}

unsigned long int lcs_pos(agps_pos_params_t *agps_pos_params)
{
	D("lcs pos is enter");

	c_interface->lcs_pos(agps_pos_params);
	return 0;
}

void agent_thread(void)
{
    D("agent_thread start\n");

    c_interface = (agps_c_plane_interface_t *)get_agps_cp_interface();
    c_interface->init(&c_hook);

}
#endif

