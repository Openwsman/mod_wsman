/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "wsman-types.h"
#include "wsman-faults.h"
#include "wsman-soap-message.h"
#include "wsman-server-api.h"

#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_protocol.h"

/*#include "ap_compat.h"
#include "apr_strings.h"*/

extern char * apr_pstrdup (apr_pool_t *p, const char *s);
extern char * apr_pstrcat (apr_pool_t *p,...);

#define OPENWSMAN
#define BUFSIZE 512

/*
 * Declare ourselves so the configuration routines can find and know us.
 * We'll fill it in at the end of the module.
 */
module AP_MODULE_DECLARE_DATA wsman_module;

/* 
 * The return value instructs the caller concerning what happened and what to
 * do next:
 *  OK ("we did our thing")
 *  DECLINED ("this isn't something with which we want to get involved")
 *  HTTP_mumble ("an error status should be reported")
 */
static int wsman_handler(request_rec *r, int lookup)
{
	long len = 0, size = 0;
	char *request = NULL;
	char *buf = apr_pcalloc(r->pool, BUFSIZE);
	void* soap = NULL;

	WsmanMessage *wsman_msg = wsman_soap_message_new();

	/*
	if(NULL == ap_auth_name(r)) {
		ap_log_perror(APLOG_MARK, APLOG_ERR, DECLINED, r->pool, "mod_wsman error: %s", "server cfg error, all requests must be authenticated with basic/digest authentication.");
		return DECLINED;
	}
	*/
	if(OK != ap_setup_client_block(r, REQUEST_CHUNKED_DECHUNK)) {
		ap_log_perror(APLOG_MARK, APLOG_ERR, DECLINED, r->pool, "mod_wsman error: %s", "Failed to receive POST buffer");
		return DECLINED;
	}
	memset(buf, 0, BUFSIZE);
	while(0 < (len = ap_get_client_block(r, buf, BUFSIZE - 1))) {
		size += len;
		if(NULL == request) {
			request = (char *)apr_pstrdup((apr_pool_t *)r->pool, buf);
		} else {
			//char *tmp = request;
			request = (char *)apr_pstrcat((apr_pool_t *)r->pool, request, buf, NULL);
			//Need to free tmp since contents have been copied
		}
		memset(buf, 0, BUFSIZE);
	}
	u_buf_set(wsman_msg->request, request, size);

	soap = (void *)ap_get_module_config(r->server->module_config, &wsman_module);
	if(NULL != soap) {
		wsman_server_get_response(soap, wsman_msg);
		ap_rputs((char *)u_buf_ptr(wsman_msg->response), r);
	}
	return OK;
}

static void *wsman_create_server_config(apr_pool_t *p, server_rec *s)
{
	return ((void *)wsman_server_create_config("/etc/openwsman/openwsman.conf"));
}

static void register_hooks( apr_pool_t *p )
{
	ap_hook_quick_handler(wsman_handler, NULL, NULL, APR_HOOK_FIRST);
}

static const char *ConfigTag(cmd_parms *cmd, void *dummy, const char *arg)
{
	return ((void *)wsman_server_create_config((char *)arg));
}

static const command_rec wsman_commands[]=
{
	AP_INIT_TAKE1("Configuration", ConfigTag, NULL, RSRC_CONF, "path to openwsman config file"),
	{ NULL }
};

module AP_MODULE_DECLARE_DATA wsman_module =
{
	STANDARD20_MODULE_STUFF,
	NULL, 				/* per-directory config creator */
	NULL,				/* dir config merger */
	wsman_create_server_config,	/* server config creator */
	NULL,        			/* server config merger */
	wsman_commands,      		/* command table */
	register_hooks			/* register hooks */
};
