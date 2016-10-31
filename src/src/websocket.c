/* 
 * Based on: 
 * 
 * libwebsockets-test-server - libwebsockets test implementation
 *
 * Copyright (C) 2010-2016 Andy Green <andy@warmcat.com>
 *
 * This file is made available under the Creative Commons CC0 1.0
 * Universal Public Domain Dedication.
 */

#include "websocket.h"


int close_testing;
int max_poll_elements;
int debug_level = 0;

#ifdef EXTERNAL_POLL
struct lws_pollfd *pollfds;
int *fd_lookup;
int count_pollfds;
#endif
volatile int force_exit = 0;
struct lws_context *context;
struct lws_plat_file_ops fops_plat;

/* http server gets files from this path */
#define LOCAL_RESOURCE_PATH "lws/"
char *resource_path = LOCAL_RESOURCE_PATH;
#if defined(LWS_USE_POLARSSL)
#else
#if defined(LWS_USE_MBEDTLS)
#else
#if defined(LWS_OPENSSL_SUPPORT) && defined(LWS_HAVE_SSL_CTX_set1_param)
char crl_path[1024] = "";
#endif
#endif
#endif

pthread_mutex_t lock_established_conns;

/* singlethreaded version --> no locks */

void test_server_lock(int care)
{
  if (care)
    pthread_mutex_lock(&lock_established_conns);
}
void test_server_unlock(int care)
{
  if (care)
    pthread_mutex_unlock(&lock_established_conns);
}

/*
 * This demo server shows how to use libwebsockets for one or more
 * websocket protocols in the same server
 *
 * It defines the following websocket protocols:
 *
 *  dumb-increment-protocol:  once the socket is opened, an incrementing
 *				ascii string is sent down it every 50ms.
 *				If you send "reset\n" on the websocket, then
 *				the incrementing number is reset to 0.
 *
 *  lws-mirror-protocol: copies any received packet to every connection also
 *				using this protocol, including the sender
 */


#define MAX_MESSAGE_QUEUE 512

struct a_message {
	void *payload;
	size_t len;
};

static struct a_message ringbuffer[MAX_MESSAGE_QUEUE];
static int ringbuffer_head;

int
callback_lws_mirror(struct lws *wsi, enum lws_callback_reasons reason,
		    void *user, void *in, size_t len)
{
	struct per_session_data__lws_mirror *pss =
			(struct per_session_data__lws_mirror *)user;
	int n, m;

	switch (reason) {

	case LWS_CALLBACK_ESTABLISHED:
		lwsl_info("%s: LWS_CALLBACK_ESTABLISHED\n", __func__);
		pss->ringbuffer_tail = ringbuffer_head;
		pss->wsi = wsi;
		break;

	case LWS_CALLBACK_PROTOCOL_DESTROY:
		lwsl_notice("%s: mirror protocol cleaning up\n", __func__);
		for (n = 0; n < sizeof ringbuffer / sizeof ringbuffer[0]; n++)
			if (ringbuffer[n].payload)
				free(ringbuffer[n].payload);
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
		if (close_testing)
			break;
		while (pss->ringbuffer_tail != ringbuffer_head) {
			m = ringbuffer[pss->ringbuffer_tail].len;
			n = lws_write(wsi, (unsigned char *)
				   ringbuffer[pss->ringbuffer_tail].payload +
				   LWS_PRE, m, LWS_WRITE_TEXT);
			if (n < 0) {
				lwsl_err("ERROR %d writing to mirror socket\n", n);
				return -1;
			}
			if (n < m)
				lwsl_err("mirror partial write %d vs %d\n", n, m);

			if (pss->ringbuffer_tail == (MAX_MESSAGE_QUEUE - 1))
				pss->ringbuffer_tail = 0;
			else
				pss->ringbuffer_tail++;

			if (((ringbuffer_head - pss->ringbuffer_tail) &
			    (MAX_MESSAGE_QUEUE - 1)) == (MAX_MESSAGE_QUEUE - 15))
				lws_rx_flow_allow_all_protocol(lws_get_context(wsi),
					       lws_get_protocol(wsi));

			if (lws_send_pipe_choked(wsi)) {
				lws_callback_on_writable(wsi);
				break;
			}
		}
		break;

	case LWS_CALLBACK_RECEIVE:
		if (((ringbuffer_head - pss->ringbuffer_tail) &
		    (MAX_MESSAGE_QUEUE - 1)) == (MAX_MESSAGE_QUEUE - 1)) {
			lwsl_err("dropping!\n");
			goto choke;
		}

		if (ringbuffer[ringbuffer_head].payload)
			free(ringbuffer[ringbuffer_head].payload);

		ringbuffer[ringbuffer_head].payload = malloc(LWS_PRE + len);
		ringbuffer[ringbuffer_head].len = len;
		memcpy((char *)ringbuffer[ringbuffer_head].payload +
		       LWS_PRE, in, len);
		if (ringbuffer_head == (MAX_MESSAGE_QUEUE - 1))
			ringbuffer_head = 0;
		else
			ringbuffer_head++;

		if (((ringbuffer_head - pss->ringbuffer_tail) &
		    (MAX_MESSAGE_QUEUE - 1)) != (MAX_MESSAGE_QUEUE - 2))
			goto done;

choke:
		lwsl_debug("LWS_CALLBACK_RECEIVE: throttling %p\n", wsi);
		lws_rx_flow_control(wsi, 0);

done:
		lws_callback_on_writable_all_protocol(lws_get_context(wsi),
						      lws_get_protocol(wsi));
		break;

	/*
	 * this just demonstrates how to use the protocol filter. If you won't
	 * study and reject connections based on header content, you don't need
	 * to handle this callback
	 */

	case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
		dump_handshake_info(wsi);
		/* you could return non-zero here and kill the connection */
		break;

	default:
		break;
	}

	return 0;
}


enum demo_protocols {
	/* always first */
	PROTOCOL_HTTP = 0,

	PROTOCOL_LWS_MIRROR,
	/* always last */
	DEMO_PROTOCOL_COUNT
};

/* list of supported protocols and callbacks */

static struct lws_protocols protocols[] = {
	/* first protocol must always be HTTP handler */

	{
		"http-only",		/* name */
		callback_http,		/* callback */
		sizeof (struct per_session_data__http),	/* per_session_data_size */
		0,			/* max frame size / rx buffer */
	},
	{
	  "lws-mirror-protocol",
	  callback_lws_mirror,
	  sizeof(struct per_session_data__lws_mirror),
	  128, /* rx buf size must be >= permessage-deflate rx size */
        },

	{ NULL, NULL, 0, 0 } /* terminator */
};


void sighandler(int sig)
{
	force_exit = 1;
	lws_cancel_service(context);
}

static const struct lws_extension exts[] = {
	{
		"permessage-deflate",
		lws_extension_callback_pm_deflate,
		"permessage-deflate"
	},
	{
		"deflate-frame",
		lws_extension_callback_pm_deflate,
		"deflate_frame"
	},
	{ NULL, NULL, NULL /* terminator */ }
};


void *ThreadService(void *threadid)
{
  while (lws_service_tsi(context, 50, (int)(long)threadid) >= 0 && !force_exit)
    ;
  return NULL;
}

/* Set up our port for listening */
struct lws_context* init_websocket() {
  
  struct lws_context_creation_info info;
  int opts = 0;
  
  /* Insure we are configured */
  if(mudsettingGetInt("websocket_port") == 0)
    mudsettingSetInt("websocket_port", DFLT_WEBSOCKET_PORT);

  memset(&info, 0, sizeof info);
  
  info.port = mudsettingGetInt("websocket_port");
  info.protocols = protocols;
  info.ssl_cert_filepath = NULL;
  info.ssl_private_key_filepath = NULL;
  info.iface = NULL;
  info.uid = -1;
  info.gid = -1;
  info.max_http_header_pool = 16;

  info.options = opts | LWS_SERVER_OPTION_VALIDATE_UTF8;
  info.extensions = exts;
  info.timeout_secs = 5;
  info.ssl_cipher_list = "ECDHE-ECDSA-AES256-GCM-SHA384:"
    "ECDHE-RSA-AES256-GCM-SHA384:"
    "DHE-RSA-AES256-GCM-SHA384:"
    "ECDHE-RSA-AES256-SHA384:"
    "HIGH:!aNULL:!eNULL:!EXPORT:"
    "!DES:!MD5:!PSK:!RC4:!HMAC_SHA1:"
    "!SHA1:!DHE-RSA-AES128-GCM-SHA256:"
    "!DHE-RSA-AES128-SHA256:"
    "!AES128-GCM-SHA256:"
    "!AES128-SHA256:"
    "!DHE-RSA-AES256-SHA256:"
    "!AES256-GCM-SHA384:"
    "!AES256-SHA256";

  info.count_threads = 4;
  /* Create our LWS context and bind to port */
  context = lws_create_context(&info);

  /* Error checking */
  if (context == NULL) {
    log_string("ERROR: Websockets failed to init");
    exit(1);
  }

  threadpool thpool = thpool_init(4);
  int i;
  for (i=0; i<4; i++){
    thpool_add_work(thpool, (void*)ThreadService, (void*)(long)i);
  };
  
  return context;
}

/*
	struct lws_context_creation_info info;
	char interface_name[128] = "";
	unsigned int ms, oldms = 0;
	const char *iface = NULL;
	char cert_path[1024] = "";
	char key_path[1024] = "";
	char ca_path[1024] = "";
	int uid = -1, gid = -1;
	int use_ssl = 0;
	int opts = 0;
	int n = 0;
*/

	/*
	 * take care to zero down the info struct, he contains random garbaage
	 * from the stack otherwise
	 */



/*
	memset(&info, 0, sizeof info);
	info.port = 7681;

	while (n >= 0) {
		n = getopt_long(argc, argv, "eci:hsap:d:Dr:C:K:A:R:vu:g:", options, NULL);
		if (n < 0)
			continue;
		switch (n) {
		case 'e':
			opts |= LWS_SERVER_OPTION_LIBEV;
			break;

			opts |= LWS_SERVER_OPTION_ALLOW_NON_SSL_ON_SSL_PORT;
			break;
		case 'p':
			info.port = atoi(optarg);
			break;
		case 'i':
			strncpy(interface_name, optarg, sizeof interface_name);
			interface_name[(sizeof interface_name) - 1] = '\0';
			iface = interface_name;
			break;
		case 'c':
			close_testing = 1;
			fprintf(stderr, " Close testing mode -- closes on "
					   "client after 50 dumb increments"
					   "and suppresses lws_mirror spam\n");
			break;
		case 'r':
			resource_path = optarg;
			printf("Setting resource path to \"%s\"\n", resource_path);
			break;
		case 'C':
			strncpy(cert_path, optarg, sizeof(cert_path) - 1);
			cert_path[sizeof(cert_path) - 1] = '\0';
			break;
		case 'K':
			strncpy(key_path, optarg, sizeof(key_path) - 1);
			key_path[sizeof(key_path) - 1] = '\0';
			break;
		case 'A':
			strncpy(ca_path, optarg, sizeof(ca_path) - 1);
			ca_path[sizeof(ca_path) - 1] = '\0';
			break;
#if defined(LWS_OPENSSL_SUPPORT)
		case 'v':
			use_ssl = 1;
			opts |= LWS_SERVER_OPTION_REQUIRE_VALID_OPENSSL_CLIENT_CERT;
			break;
#if defined(LWS_USE_POLARSSL)
#else
#if defined(LWS_USE_MBEDTLS)
#else
#if defined(LWS_HAVE_SSL_CTX_set1_param)
		case 'R':
			strncpy(crl_path, optarg, sizeof(crl_path) - 1);
			crl_path[sizeof(crl_path) - 1] = '\0';
			break;
#endif
#endif
#endif
#endif
		case 'h':
			fprintf(stderr, "Usage: test-server "
					"[--port=<p>] [--ssl] "
					"[-d <log bitfield>] "
					"[--resource_path <path>]\n");
			exit(1);
		}
	}

#if !defined(LWS_NO_DAEMONIZE) && !defined(WIN32)
*/
	/*
	 * normally lock path would be /var/lock/lwsts or similar, to
	 * simplify getting started without having to take care about
	 * permissions or running as root, set to /tmp/.lwsts-lock
	 */
/*	if (daemonize && lws_daemonize("/tmp/.lwsts-lock")) {
		fprintf(stderr, "Failed to daemonize\n");
		return 10;
	}
#endif

	signal(SIGINT, sighandler);

#ifndef _WIN32
*/
	/* we will only try to log things according to our debug_level */
/*	setlogmask(LOG_UPTO (LOG_DEBUG));
	openlog("lwsts", syslog_options, LOG_DAEMON);
#endif
*/
	/* tell the library what debug level to emit and to send it to syslog */
/*
	lws_set_log_level(debug_level, lwsl_emit_syslog);

	lwsl_notice("libwebsockets test server - license LGPL2.1+SLE\n");
	lwsl_notice("(C) Copyright 2010-2016 Andy Green <andy@warmcat.com>\n");

	printf("Using resource path \"%s\"\n", resource_path);
#ifdef EXTERNAL_POLL
	max_poll_elements = getdtablesize();
	pollfds = malloc(max_poll_elements * sizeof (struct lws_pollfd));
	fd_lookup = malloc(max_poll_elements * sizeof (int));
	if (pollfds == NULL || fd_lookup == NULL) {
		lwsl_err("Out of memory pollfds=%d\n", max_poll_elements);
		return -1;
	}
#endif

	info.iface = iface;
	info.protocols = protocols;
	info.ssl_cert_filepath = NULL;
	info.ssl_private_key_filepath = NULL;

	if (use_ssl) {
		if (strlen(resource_path) > sizeof(cert_path) - 32) {
			lwsl_err("resource path too long\n");
			return -1;
		}
		if (!cert_path[0])
			sprintf(cert_path, "%s/libwebsockets-test-server.pem",
								resource_path);
		if (strlen(resource_path) > sizeof(key_path) - 32) {
			lwsl_err("resource path too long\n");
			return -1;
		}
		if (!key_path[0])
			sprintf(key_path, "%s/libwebsockets-test-server.key.pem",
								resource_path);

		info.ssl_cert_filepath = cert_path;
		info.ssl_private_key_filepath = key_path;
		if (ca_path[0])
			info.ssl_ca_filepath = ca_path;
	}
//	info.gid = gid;
//	info.uid = uid;
	info.max_http_header_pool = 16;
	info.options = opts | LWS_SERVER_OPTION_VALIDATE_UTF8;
	info.extensions = exts;
	info.timeout_secs = 5;
	info.ssl_cipher_list = "ECDHE-ECDSA-AES256-GCM-SHA384:"
			       "ECDHE-RSA-AES256-GCM-SHA384:"
			       "DHE-RSA-AES256-GCM-SHA384:"
			       "ECDHE-RSA-AES256-SHA384:"
			       "HIGH:!aNULL:!eNULL:!EXPORT:"
			       "!DES:!MD5:!PSK:!RC4:!HMAC_SHA1:"
			       "!SHA1:!DHE-RSA-AES128-GCM-SHA256:"
			       "!DHE-RSA-AES128-SHA256:"
			       "!AES128-GCM-SHA256:"
			       "!AES128-SHA256:"
			       "!DHE-RSA-AES256-SHA256:"
			       "!AES256-GCM-SHA384:"
			       "!AES256-SHA256";

	if (use_ssl)
*/
		/* redirect guys coming on http */
/*
		info.options |= LWS_SERVER_OPTION_REDIRECT_HTTP_TO_HTTPS;

	context = lws_create_context(&info);
	if (context == NULL) {
		lwsl_err("libwebsocket init failed\n");
		return -1;
	}
*/
	/* this shows how to override the lws file operations.	You don't need
	 * to do any of this unless you have a reason (eg, want to serve
	 * compressed files without decompressing the whole archive)
	 */
	/* stash original platform fops */
/*	fops_plat = *(lws_get_fops(context)); */
	/* override the active fops */
/*	lws_get_fops(context)->open = test_server_fops_open;

	n = 0; 
#ifdef EXTERNAL_POLL
	int ms_1sec = 0;
#endif
	while (n >= 0 && !force_exit) {
		struct timeval tv;

		gettimeofday(&tv, NULL);
*/
		/*
		 * This provokes the LWS_CALLBACK_SERVER_WRITEABLE for every
		 * live websocket connection using the DUMB_INCREMENT protocol,
		 * as soon as it can take more packets (usually immediately)
		 */
/*
		ms = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
		if ((ms - oldms) > 50) {
			lws_callback_on_writable_all_protocol(context,
				&protocols[PROTOCOL_DUMB_INCREMENT]);
			oldms = ms;
		}

#ifdef EXTERNAL_POLL
*/
		/*
		 * this represents an existing server's single poll action
		 * which also includes libwebsocket sockets
		 */
/*
		n = poll(pollfds, count_pollfds, 50);
		if (n < 0)
			continue;

		if (n) {
			for (n = 0; n < count_pollfds; n++)
				if (pollfds[n].revents)
*/
					/*
					* returns immediately if the fd does not
					* match anything under libwebsockets
					* control
					*/
/*
					if (lws_service_fd(context,
								  &pollfds[n]) < 0)
						goto done;
*/
			/* if needed, force-service wsis that may not have read all input */
/*			while (!lws_service_adjust_timeout(context, 1, 0)) {
				lwsl_notice("extpoll doing forced service!\n");
				lws_service_tsi(context, -1, 0);
			}
		} else {
*/
			/* no revents, but before polling again, make lws check for any timeouts */
/*			if (ms - ms_1sec > 1000) {
				lwsl_notice("1 per sec\n");
				lws_service_fd(context, NULL);
				ms_1sec = ms;
		}	
		}
#else
*/
		/*
		 * If libwebsockets sockets are all we care about,
		 * you can use this api which takes care of the poll()
		 * and looping through finding who needed service.
		 *
		 * If no socket needs service, it'll return anyway after
		 * the number of ms in the second argument.
		 */
/*
		n = lws_service(context, 50);
#endifo
	}

#ifdef EXTERNAL_POLL
done:
#endif

	lws_context_destroy(context);

	lwsl_notice("libwebsockets-test-server exited cleanly\n");

#ifndef _WIN32
	closelog();
#endif


*/
