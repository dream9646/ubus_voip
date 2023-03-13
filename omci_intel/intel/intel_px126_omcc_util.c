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
 * Module  : switch_hw_prx126
 * File    : switch_hw_prx126_acl.c
 *
 ******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>


#include "openssl/cmac.h"
#include "openssl/aes.h"
#include "openssl/evp.h"


#include "fwk_mutex.h"

#include "util.h"
#include "util_uci.h"
#include "intel_px126_util.h"

#include "omci/pon_adapter_msg.h"

/** Maximum baseline OMCI message length (excluding CRC). */
#define OMCI_MSG_MAX_LEN 40
/** Maximum extended OMCI message length (excluding MIC). */
#define OMCI_EXT_MSG_MAX_LEN 1976

/** CRC-32 table */
static const unsigned int crc32_i363_table[256] = {
	0x00000000L, 0x04c11db7L, 0x09823b6eL, 0x0d4326d9L,
	0x130476dcL, 0x17c56b6bL, 0x1a864db2L, 0x1e475005L,
	0x2608edb8L, 0x22c9f00fL, 0x2f8ad6d6L, 0x2b4bcb61L,
	0x350c9b64L, 0x31cd86d3L, 0x3c8ea00aL, 0x384fbdbdL,
	0x4c11db70L, 0x48d0c6c7L, 0x4593e01eL, 0x4152fda9L,
	0x5f15adacL, 0x5bd4b01bL, 0x569796c2L, 0x52568b75L,
	0x6a1936c8L, 0x6ed82b7fL, 0x639b0da6L, 0x675a1011L,
	0x791d4014L, 0x7ddc5da3L, 0x709f7b7aL, 0x745e66cdL,
	0x9823b6e0L, 0x9ce2ab57L, 0x91a18d8eL, 0x95609039L,
	0x8b27c03cL, 0x8fe6dd8bL, 0x82a5fb52L, 0x8664e6e5L,
	0xbe2b5b58L, 0xbaea46efL, 0xb7a96036L, 0xb3687d81L,
	0xad2f2d84L, 0xa9ee3033L, 0xa4ad16eaL, 0xa06c0b5dL,
	0xd4326d90L, 0xd0f37027L, 0xddb056feL, 0xd9714b49L,
	0xc7361b4cL, 0xc3f706fbL, 0xceb42022L, 0xca753d95L,
	0xf23a8028L, 0xf6fb9d9fL, 0xfbb8bb46L, 0xff79a6f1L,
	0xe13ef6f4L, 0xe5ffeb43L, 0xe8bccd9aL, 0xec7dd02dL,
	0x34867077L, 0x30476dc0L, 0x3d044b19L, 0x39c556aeL,
	0x278206abL, 0x23431b1cL, 0x2e003dc5L, 0x2ac12072L,
	0x128e9dcfL, 0x164f8078L, 0x1b0ca6a1L, 0x1fcdbb16L,
	0x018aeb13L, 0x054bf6a4L, 0x0808d07dL, 0x0cc9cdcaL,
	0x7897ab07L, 0x7c56b6b0L, 0x71159069L, 0x75d48ddeL,
	0x6b93dddbL, 0x6f52c06cL, 0x6211e6b5L, 0x66d0fb02L,
	0x5e9f46bfL, 0x5a5e5b08L, 0x571d7dd1L, 0x53dc6066L,
	0x4d9b3063L, 0x495a2dd4L, 0x44190b0dL, 0x40d816baL,
	0xaca5c697L, 0xa864db20L, 0xa527fdf9L, 0xa1e6e04eL,
	0xbfa1b04bL, 0xbb60adfcL, 0xb6238b25L, 0xb2e29692L,
	0x8aad2b2fL, 0x8e6c3698L, 0x832f1041L, 0x87ee0df6L,
	0x99a95df3L, 0x9d684044L, 0x902b669dL, 0x94ea7b2aL,
	0xe0b41de7L, 0xe4750050L, 0xe9362689L, 0xedf73b3eL,
	0xf3b06b3bL, 0xf771768cL, 0xfa325055L, 0xfef34de2L,
	0xc6bcf05fL, 0xc27dede8L, 0xcf3ecb31L, 0xcbffd686L,
	0xd5b88683L, 0xd1799b34L, 0xdc3abdedL, 0xd8fba05aL,
	0x690ce0eeL, 0x6dcdfd59L, 0x608edb80L, 0x644fc637L,
	0x7a089632L, 0x7ec98b85L, 0x738aad5cL, 0x774bb0ebL,
	0x4f040d56L, 0x4bc510e1L, 0x46863638L, 0x42472b8fL,
	0x5c007b8aL, 0x58c1663dL, 0x558240e4L, 0x51435d53L,
	0x251d3b9eL, 0x21dc2629L, 0x2c9f00f0L, 0x285e1d47L,
	0x36194d42L, 0x32d850f5L, 0x3f9b762cL, 0x3b5a6b9bL,
	0x0315d626L, 0x07d4cb91L, 0x0a97ed48L, 0x0e56f0ffL,
	0x1011a0faL, 0x14d0bd4dL, 0x19939b94L, 0x1d528623L,
	0xf12f560eL, 0xf5ee4bb9L, 0xf8ad6d60L, 0xfc6c70d7L,
	0xe22b20d2L, 0xe6ea3d65L, 0xeba91bbcL, 0xef68060bL,
	0xd727bbb6L, 0xd3e6a601L, 0xdea580d8L, 0xda649d6fL,
	0xc423cd6aL, 0xc0e2d0ddL, 0xcda1f604L, 0xc960ebb3L,
	0xbd3e8d7eL, 0xb9ff90c9L, 0xb4bcb610L, 0xb07daba7L,
	0xae3afba2L, 0xaafbe615L, 0xa7b8c0ccL, 0xa379dd7bL,
	0x9b3660c6L, 0x9ff77d71L, 0x92b45ba8L, 0x9675461fL,
	0x8832161aL, 0x8cf30badL, 0x81b02d74L, 0x857130c3L,
	0x5d8a9099L, 0x594b8d2eL, 0x5408abf7L, 0x50c9b640L,
	0x4e8ee645L, 0x4a4ffbf2L, 0x470cdd2bL, 0x43cdc09cL,
	0x7b827d21L, 0x7f436096L, 0x7200464fL, 0x76c15bf8L,
	0x68860bfdL, 0x6c47164aL, 0x61043093L, 0x65c52d24L,
	0x119b4be9L, 0x155a565eL, 0x18197087L, 0x1cd86d30L,
	0x029f3d35L, 0x065e2082L, 0x0b1d065bL, 0x0fdc1becL,
	0x3793a651L, 0x3352bbe6L, 0x3e119d3fL, 0x3ad08088L,
	0x2497d08dL, 0x2056cd3aL, 0x2d15ebe3L, 0x29d4f654L,
	0xc5a92679L, 0xc1683bceL, 0xcc2b1d17L, 0xc8ea00a0L,
	0xd6ad50a5L, 0xd26c4d12L, 0xdf2f6bcbL, 0xdbee767cL,
	0xe3a1cbc1L, 0xe760d676L, 0xea23f0afL, 0xeee2ed18L,
	0xf0a5bd1dL, 0xf464a0aaL, 0xf9278673L, 0xfde69bc4L,
	0x89b8fd09L, 0x8d79e0beL, 0x803ac667L, 0x84fbdbd0L,
	0x9abc8bd5L, 0x9e7d9662L, 0x933eb0bbL, 0x97ffad0cL,
	0xafb010b1L, 0xab710d06L, 0xa6322bdfL, 0xa2f33668L,
	0xbcb4666dL, 0xb8757bdaL, 0xb5365d03L, 0xb1f740b4L
};


/** OMCI message formats (Device identifier field of OMCI message) */
enum omci_message_format {
	/** Baseline message format */
	OMCI_FORMAT_BASELINE = 0x0a,

	/** Extended message format */
	OMCI_FORMAT_EXTENDED = 0x0b
};

struct omci_msg_msg {
	/** Message contents */
	unsigned char contents[32];
} __attribute__ ((packed));



/** OMCI message header */
struct omci_header {
	/** Transaction identifier */
	unsigned short tci;
	/** Message type */
	unsigned char type;
	/** Device identifier type */
	unsigned char dev_id;
	/** Entity class */
	unsigned short class_id;
	/** Entity instance */
	unsigned short instance_id;
} __attribute__ ((packed));

/** Maximum length of OMCI message data. */
#define OMCI_EXT_MSG_MAX_DATA_LEN		(OMCI_EXT_MSG_MAX_LEN - \
						 sizeof(struct omci_header) - \
						 sizeof(unsigned short) /* len */)


/** OMCI message trailer */
struct omci_msg_trailer {
	/** Message contents */
	unsigned char contents[32];
	/** Message trailer CPCS UU */
	unsigned char cpcs_uu;
	/** Message trailer CPCS CPI */
	unsigned char cpcs_cpi;
	/** Message trailer SDU length */
	unsigned short cpcs_sdu;
	/** CRC32/MIC is left out */
} __attribute__ ((packed));


/** Extended Create message */
struct omci_ext_msg_create {
	/** Attribute values of Set-by-create attributes (size depending on the
	   type of attribute) */
	unsigned char values[OMCI_EXT_MSG_MAX_DATA_LEN];
} __attribute__ ((packed));

/** Extended message body */
struct omci_ext_msg {
	/** Message length */
	unsigned short len;

	union {
		/** Create message */
		struct omci_ext_msg_create create;
		/** Create response */
	};
} __attribute__ ((packed));


/** OMCI Message header */
struct omci_msg {
	/** Message header */
	struct omci_header header;

	/** OMCI Message union */
	union {
		/** Plain message with trailer */
		struct omci_msg_trailer trailer;
		/** Extended message */
		struct omci_ext_msg ext;
	};
} __attribute__ ((packed));
/////////////////////////////////////////////////////////////

unsigned int pa_omci_crc32(uint32_t crc,
		       const uint8_t *data,
		       size_t data_size)
{
	int i;

	for (i = 0; i < data_size; i++)
		crc = (crc << 8) ^ crc32_i363_table[((crc >> 24) ^ data[i]) &
			0xff];

	return crc;
}

int omci_mic_cmac_calc(struct intel_omci_context *context,				  
				   const void *omci_msg, int len,
				   unsigned int *mic)
{
	int  ret = INTEL_PON_SUCCESS;

	unsigned char tag[AES_BLOCK_SIZE] = { 0 };
	unsigned char dir = OMCI_MSG_DIR_UPSTREAM;
	size_t taglen;

	fwk_mutex_lock(&context->omci_ik.lock);
	CMAC_CTX *ssl_ctx = CMAC_CTX_new();

	if(ssl_ctx ==NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_INFO, 0,"malloc CMAC error\n");
		fwk_mutex_unlock(&context->omci_ik.lock);
		return INTEL_PON_ERROR;

	}

	CMAC_Init(ssl_ctx, context->omci_ik.key, OMCI_MIC_IK_LEN, EVP_aes_128_cbc(), NULL);

	CMAC_Update(ssl_ctx, &dir, sizeof(dir));
	CMAC_Update(ssl_ctx, omci_msg, len);
	CMAC_Final(ssl_ctx, tag, &taglen);

	if (taglen != AES_BLOCK_SIZE)
		ret = INTEL_PON_ERROR;
	else
		*mic = tag[0] << 24 | tag[1] << 16 | tag[2] << 8 | tag[3];

	CMAC_CTX_free(ssl_ctx);
	fwk_mutex_unlock(&context->omci_ik.lock);	
	return ret;
}

/** Return true if message is baseline.
	\param[in] msg OMCI message pointer */
static inline bool omci_msg_is_baseline(const struct omci_msg *msg)
{
	return msg->header.dev_id == OMCI_FORMAT_BASELINE;
}

/** Function to calculate crc */
static int omci_msg_mic_calc_crc(const void *omci_msg, int len,
					     unsigned int *crc_mic)
{
	*crc_mic = ~pa_omci_crc32(0xffffffff, omci_msg, len);

	return INTEL_PON_SUCCESS;
}

unsigned short omci_msg_data_len(const struct omci_msg *msg, uint16_t offset)
{
	unsigned short len;

	if (omci_msg_is_baseline(msg))
		return sizeof(struct omci_msg_msg);

	//len = ntoh16(msg->ext.len);
	len = ntohs(msg->ext.len);

	if (offset > len)
		len = 0;
	else
		len -= offset;

	if (len > OMCI_EXT_MSG_MAX_DATA_LEN)
		return OMCI_EXT_MSG_MAX_DATA_LEN;

	return len;
}

unsigned short omci_msg_len(const struct omci_msg *msg)
{
	if (omci_msg_is_baseline(msg)) {
		return OMCI_MSG_MAX_LEN;
	} else {
		return sizeof(struct omci_header) +
			sizeof(unsigned short) + /* len */
			omci_msg_data_len(msg, 0);
	}
}

static bool omci_msg_check_size(const struct omci_msg *msg, const unsigned short len)
{
	/* check for minimal message size:
	 *  - omci_header for baseline messages
	 *  - omci_header+uint_16 for extended messages
	 */
	if (len < sizeof(struct omci_header) + sizeof(unsigned short))
		return false;

	/* FIXME: when using omci_simulate messages has extra bytes, would be
	 * better to check for exact size
	 */
	if (len < omci_msg_len(msg))
		return false;

	return true;
}


/* Dummy check of crc/mic, always return success */
static int __intel_omcc_msg_check_dummy(void *context,
					    const void *omci_msg, int len,
					    unsigned int crc_mic)
{
	/* avoid "unused argument" warnings */
	INTEL_UNUSED(context);
	INTEL_UNUSED(omci_msg);
	INTEL_UNUSED(len);
	INTEL_UNUSED(crc_mic);

	return INTEL_PON_SUCCESS;
}

/** Dummy function to calculate crc or mic */
static int  __intel_omcc_msg_mic_calc_dummy(void *context,
					       const void *omci_msg, int len,
					       unsigned int *crc_mic)
{
	INTEL_UNUSED(context);
	INTEL_UNUSED(omci_msg);
	INTEL_UNUSED(len);

	*crc_mic = 0;

	return INTEL_PON_SUCCESS;
}
					       
static int intel_omcc_msg_recv_omcc(void *context,
				 const unsigned char *omci_msg,
				 const unsigned short len,
				 const unsigned int *crc)
{
	struct intel_omci_context *ctx = (struct intel_omci_context *)context;

	unsigned int crc_mic = *crc;
	util_dbprintf(omci_env_g.debug_level, LOG_INFO, 0,"%p, %p, %u, %p", context, omci_msg, len, crc);

	if(ctx ==NULL || omci_msg == NULL){
		
		util_dbprintf(omci_env_g.debug_level, LOG_INFO, 0,"null point %p, %p", ctx, omci_msg);
		return INTEL_PON_ERROR;
	}

	if ((len) > (OMCI_EXT_MSG_MAX_LEN)) { 
		util_dbprintf(omci_env_g.debug_level, LOG_INFO, 0,"omci msg error(%d)", len);
		return INTEL_PON_ERROR;
	}

	if(ctx->omcc_raw_rcv)
		ctx->omcc_raw_rcv((void*)omci_msg, len,crc_mic);

	return INTEL_PON_SUCCESS;
}

int intel_omcc_msg_send(struct intel_omci_context *context,
			      const char *msg,
			      const unsigned short len,
			      unsigned int crc_mic)
{

	enum pon_adapter_errno ret;
	struct pa_node *np = NULL,*n = NULL;
	unsigned short length = len;
	struct omci_msg omci_msg;
	int error;
	//enum pa_pon_op_mode op_mode;
	util_dbprintf(omci_env_g.debug_level, LOG_INFO, 0, "len=%d ,crc=0x%x,op=%d\n", len,crc_mic,context->op_mode);

	if (!context || !msg)
		return INTEL_PON_ERROR;

	if (!omci_msg_check_size((struct omci_msg *)msg, len)) {
		dbprintf(LOG_ERR, " - message too short (%d bytes)\n", len);
		return -1;
	}

	memcpy(&omci_msg, msg, (length < sizeof(omci_msg) ? length : sizeof(omci_msg)));

	/* baseline OMCI message */
	if (omci_msg_is_baseline(&omci_msg)) {
		/* add baseline trailer */
		omci_msg.trailer.cpcs_uu = 0x00;
		omci_msg.trailer.cpcs_cpi = 0x00;
		omci_msg.trailer.cpcs_sdu = htons(0x0028);
		length = 44;
	}
	
	if(context->op_mode == PA_PON_MODE_UNKNOWN){
		context->op_mode = intel_omci_pa_get_pon_op_mode(context);
	}
	
#if 0
	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "op_mode=%d\n",context->op_mode);

	
	util_dbprintf(omci_env_g.debug_level, LOG_ERR,0,"Got new integrity key from firmware: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		context->omci_ik.key[0], context->omci_ik.key[1],
		context->omci_ik.key[2], context->omci_ik.key[3],
		context->omci_ik.key[4], context->omci_ik.key[5],
		context->omci_ik.key[6], context->omci_ik.key[7],
		context->omci_ik.key[8], context->omci_ik.key[9],
		context->omci_ik.key[10], context->omci_ik.key[11],
		context->omci_ik.key[12], context->omci_ik.key[13],
		context->omci_ik.key[14], context->omci_ik.key[15]);
#endif
	if(context->op_mode == PA_PON_MODE_G984){		
		error = omci_msg_mic_calc_crc(&omci_msg,length, &crc_mic);
		
	}else{
		error = omci_mic_cmac_calc(context,&omci_msg,length, &crc_mic);
	}
	
	if (error != INTEL_PON_SUCCESS){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "OMCI SEND ERROR(%d) crc=0x%x\n",crc_mic);
		return INTEL_PON_ERROR;
	}
		
	crc_mic = htonl(crc_mic);
	
	
	util_dbprintf(omci_env_g.debug_level, LOG_INFO, 0, "crc = 0x%x",crc_mic);
	/* Send OMCI msg via OMCC socket */
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, msg_ops, msg_send)) {
//			fwk_mutex_lock(&np->pa_lock);
			ret = np->pa_ops->msg_ops->msg_send(
				         np->ll_ctx, (void *)&omci_msg, length, &crc_mic);
//			fwk_mutex_unlock(&np->pa_lock);
			if (ret != PON_ADAPTER_SUCCESS) {

				util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "OMCI SEND ERROR(%d)\n",ret);
				return INTEL_PON_ERROR;
			}		
		}
	}	
	
	return INTEL_PON_SUCCESS;
}


int intel_omcc_init(struct intel_omci_context *context)
{
	enum pon_adapter_errno ret;
	struct pa_node *np = NULL,*n = NULL;
	
	context->omcc_ctx.msg_mic_check = __intel_omcc_msg_check_dummy;
	context->omcc_ctx.msg_mic_calc = __intel_omcc_msg_mic_calc_dummy;

	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
		
		if (PA_EXISTS(np->pa_ops, msg_ops, msg_rx_cb_register)) {
			//fwk_mutex_lock(&np->pa_lock);
			ret = np->pa_ops->msg_ops->msg_rx_cb_register(
				np->ll_ctx,intel_omcc_msg_recv_omcc,context);
			//fwk_mutex_unlock(&np->pa_lock);
			if (ret != PON_ADAPTER_SUCCESS) {

				util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "OMCI SEND ERROR(%d)",ret);
				return INTEL_PON_ERROR;
			}		
		}

	}
	
	return INTEL_PON_SUCCESS;
}

