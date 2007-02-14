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

/* 
 * Apache example module.  Provide demonstrations of how modules do things.
 *
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <syslog.h>

#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_main.h"
#include "http_protocol.h"
#include "util_script.h"


// #include "u/libu.h"
#include "wsman-types.h"
#include "wsman-faults.h"
#include "wsman-soap-message.h"
#include "wsman-server-api.h"

#define OPENWSMAN


/** helper to write out the headers */
static int
list_headers_callback(void *rec, const char *key, const char *value)
{
    request_rec *r = (request_rec *) rec;
    ap_rprintf(r, "%s: %s<br>", key, value);
    return 1;
}

/** write out the headers of the request. */
static void
list_headers(request_rec * r)
{
    ap_table_do(list_headers_callback, r, r->headers_in, NULL);
}

/** send the error message to the client browser */
static void
send_error_message(request_rec * r, const char *error)
{
    // ap_log_error(APLOG_MARK, APLOG_ERR, r->server, "mod_wsman: %s", error);
    r->content_type = "text/html";
    ap_send_http_header(r);
    ap_rputs(DOCTYPE_HTML_3_2, r);
    ap_rputs("<HTML>\n", r);
    ap_rputs(" <HEAD>\n", r);
    ap_rputs("  <TITLE>Apache WS-Management Handler\n", r);
    ap_rputs("  </TITLE>\n", r);
    ap_rputs(" </HEAD>\n", r);
    ap_rputs(" <BODY>\n", r);
    ap_rputs("  <H1>mod_wsman Apache WS-Management Server Error</H1>\n", r);
    ap_rprintf(r,
               "<p><strong>%s</strong><br>Please see the documentation for details.</p>",
               error);
    ap_rputs("  <H2>Content headers of the request</H2>\n", r);
    list_headers(r);
    ap_rputs("</BODY></HTML>\n", r);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/* Data declarations.                                                       */
/*                                                                          */
/* Here are the static cells and structure declarations private to our      */
/* module.                                                                  */
/*                                                                          */
/*--------------------------------------------------------------------------*/


/*
 * To avoid leaking memory from pools other than the per-request one, we
 * allocate a module-private pool, and then use a sub-pool of that which gets
 * freed each time we modify the trace.  That way previous layers of trace
 * data don't get lost.
 */
static pool *wsman_pool = NULL;

/*
 * Declare ourselves so the configuration routines can find and know us.
 * We'll fill it in at the end of the module.
 */
module MODULE_VAR_EXPORT wsman_module;

/*--------------------------------------------------------------------------*/
/*                                                                          */
/* Now we declare our content handlers, which are invoked when the server   */
/* encounters a document which our module is supposed to have a chance to   */
/* see.  (See mod_mime's SetHandler and AddHandler directives, and the      */
/* mod_info and mod_status examples, for more details.)                     */
/*                                                                          */
/* Since content handlers are dumping data directly into the connexion      */
/* (using the r*() routines, such as rputs() and rprintf()) without         */
/* intervention by other parts of the server, they need to make             */
/* sure any accumulated HTTP headers are sent first.  This is done by       */
/* calling send_http_header().  Otherwise, no header will be sent at all,   */
/* and the output sent to the client will actually be HTTP-uncompliant.     */
/*--------------------------------------------------------------------------*/
/* 
 * Sample content handler.
 *
 * The return value instructs the caller concerning what happened and what to
 * do next:
 *  OK ("we did our thing")
 *  DECLINED ("this isn't something with which we want to get involved")
 *  HTTP_mumble ("an error status should be reported")
 */
static int wsman_handler(request_rec *r)
{
    char *bodyBlock = ap_pcalloc( wsman_pool, 1024 );
    char *fullMsg = NULL;
    int ret = 0;
    const char *error = NULL;
    
    WsmanMessage *wsman_msg = wsman_soap_message_new();

    ret = ap_setup_client_block( r, REQUEST_CHUNKED_DECHUNK );
    if (ap_auth_name(r) == NULL) {
        send_error_message(r, "Server configuration error, the server has to authenticate all request either with Basic or with Digest authentication.");
        ap_kill_timeout(r);
        return OK;
    } 
    if(OK != ret) {
        send_error_message(r, "Failed to start receiving POST buffer");
        ap_kill_timeout(r);
        return OK;
    }
    ret = ap_should_client_block(r);
    if (ret != 0) {
    	long len = ap_get_client_block( r, bodyBlock, 1023 );
    	while (len > 0) {
    		bodyBlock[len]=0;
    		fullMsg = (fullMsg==NULL) ? ap_pstrdup( wsman_pool, bodyBlock ):
    									ap_pstrcat( wsman_pool, fullMsg, bodyBlock );
	    	len = ap_get_client_block( r, bodyBlock, 1023 );
    	}
	u_buf_set(wsman_msg->request, fullMsg, strlen( fullMsg ));
    } else {
        send_error_message(r, "No body received");
        ap_kill_timeout(r);
        return OK;
    }   

    /*
     * We're about to start sending content, so we need to force the HTTP
     * headers to be sent at this point.  Otherwise, no headers will be sent
     * at all.  We can set any we like first, of course.  **NOTE** Here's
     * where you set the "Content-type" header, and you do so by putting it in
     * r->content_type, *not* r->headers_out("Content-type").  If you don't
     * set it, it will be filled in with the server's default type (typically
     * "text/plain").  You *must* also ensure that r->content_type is lower
     * case.
     *
     * We also need to start a timer so the server can know if the connexion
     * is broken.
     */
    r->content_type = "application/soap+xml;charset=UTF-8";

    ap_soft_timeout("wsman test", r);
    ap_send_http_header(r);

    /*
     * If we're only supposed to send header information (HEAD request), we're
     * already there.
     */
    if (r->header_only) {
        ap_kill_timeout(r);
        return OK;
    }

    /*
     * Now send our actual output.  Since we tagged this as being
     * "text/html", we need to embed any HTML.
     */

    void* soap = NULL;
    soap = (void *)ap_get_module_config(r->server->module_config, &wsman_module);
    if (soap != NULL) {
            wsman_server_get_response( soap, wsman_msg );
	    ap_rputs( (char *)u_buf_ptr(wsman_msg->response), r );

	   /*
	     * We're all done, so cancel the timeout we set.  Since this is probably
	     * the end of the request we *could* assume this would be done during
	     * post-processing - but it's possible that another handler might be
	     * called and inherit our outstanding timer.  Not good; to each its own.
	     */
	    ap_kill_timeout(r);
    } else {
    }

    /*
     * We did what we wanted to do, so tell the rest of the server we
     * succeeded.
     */
    return OK;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/* Now let's declare routines for each of the callback phase in order.      */
/* (That's the order in which they're listed in the callback list, *not     */
/* the order in which the server calls them!  See the command_rec           */
/* declaration near the bottom of this file.)  Note that these may be       */
/* called for situations that don't relate primarily to our function - in   */
/* other words, the fixup handler shouldn't assume that the request has     */
/* to do with "example" stuff.                                              */
/*                                                                          */
/* With the exception of the content handler, all of our routines will be   */
/* called for each request, unless an earlier handler from another module   */
/* aborted the sequence.                                                    */
/*                                                                          */
/* Handlers that are declared as "int" can return the following:            */
/*                                                                          */
/*  OK          Handler accepted the request and did its thing with it.     */
/*  DECLINED    Handler took no action.                                     */
/*  HTTP_mumble Handler looked at request and found it wanting.             */
/*                                                                          */
/* What the server does after calling a module handler depends upon the     */
/* handler's return value.  In all cases, if the handler returns            */
/* DECLINED, the server will continue to the next module with an handler    */
/* for the current phase.  However, if the handler return a non-OK,         */
/* non-DECLINED status, the server aborts the request right there.  If      */
/* the handler returns OK, the server's next action is phase-specific;      */
/* see the individual handler comments below for details.                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/* 
 * This function is called during server initialisation.  Any information
 * that needs to be recorded must be in static cells, since there's no
 * configuration record.
 *
 * There is no return value.
 */

/*
 * All our module-initialiser does is add its trace to the log.
 */
static void wsman_init(server_rec *s, pool *p)
{
    /*
     * If we haven't already allocated our module-private pool, do so now.
     */
    if (wsman_pool == NULL) {
        wsman_pool = ap_make_sub_pool(NULL);
    };
}

/*
 * This function gets called to create a per-server configuration
 * record.  It will always be called for the "default" server.
 *
 * The return value is a pointer to the created module-specific
 * structure.
 */
static void *wsman_create_server_config(pool *p, server_rec *s)
{
	void *vPtr = (void *)wsman_server_create_config(NULL);
	
    return vPtr;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/* All of the routines have been declared now.  Here's the list of          */
/* directives specific to our module, and information about where they      */
/* may appear and how the command parser should pass them to us for         */
/* processing.  Note that care must be taken to ensure that there are NO    */
/* collisions of directive names between modules.                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                                                                          */
/* Now the list of content handlers available from this module.             */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/* 
 * List of content handlers our module supplies.  Each handler is defined by
 * two parts: a name by which it can be referenced (such as by
 * {Add,Set}Handler), and the actual routine name.  The list is terminated by
 * a NULL block, since it can be of variable length.
 *
 * Note that content-handlers are invoked on a most-specific to least-specific
 * basis; that is, a handler that is declared for "text/plain" will be
 * invoked before one that was declared for "text / *".  Note also that
 * if a content-handler returns anything except DECLINED, no other
 * content-handlers will be called.
 */
static const handler_rec wsman_handlers[] =
{
    {"wsman-handler", wsman_handler},
    {NULL}
};

/*--------------------------------------------------------------------------*/
/*                                                                          */
/* Finally, the list of callback routines and data structures that          */
/* provide the hooks into our module from the other parts of the server.    */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/* 
 * Module definition for configuration.  If a particular callback is not
 * needed, replace its routine name below with the word NULL.
 *
 * The number in brackets indicates the order in which the routine is called
 * during request processing.  Note that not all routines are necessarily
 * called (such as if a resource doesn't have access restrictions).
 */
module MODULE_VAR_EXPORT wsman_module =
{
    STANDARD_MODULE_STUFF,
    wsman_init,               /* module initializer */
    NULL,  /* per-directory config creator */
    NULL,   /* dir config merger */
    wsman_create_server_config,       /* server config creator */
    NULL,        /* server config merger */
    NULL,               /* command table */
    wsman_handlers,           /* [9] list of handlers */
    NULL,  					/* [2] filename-to-URI translation */
    NULL,      				/* [5] check/validate user_id */
    NULL,       			/* [6] check user_id is valid *here* */
    NULL,     				/* [4] check access by host address */
    NULL,       			/* [7] MIME type checker/setter */
    NULL,       			/* [8] fixups */
    NULL,       			/* [10] logger */
    NULL,      				/* [3] header parser */
    NULL,         /* process initializer */
    NULL,         /* process exit/cleanup */
    NULL   					/* [1] post read_request handling */
};
