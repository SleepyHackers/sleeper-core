#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#include <libwebsockets.h>

#include "lws-http.h"

#include "thpool.h"
#include "mud.h"

#define DFLT_WEBSOCKET_PORT 2067

extern struct lws_context* init_websocket();

extern int close_testing;
extern int max_poll_elements;

#ifdef EXTERNAL_POLL
extern struct lws_pollfd *pollfds;
extern int *fd_lookup;
extern int count_pollfds;
#endif
extern volatile int force_exit;
extern struct lws_context *context;
extern char *resource_path;
#if defined(LWS_OPENSSL_SUPPORT) && defined(LWS_HAVE_SSL_CTX_set1_param)
extern char crl_path[1024];
#endif

#ifndef __func__
#define __func__ __FUNCTION__
#endif

struct per_session_data__http {
	lws_filefd_type fd;
#ifdef LWS_WITH_CGI
	struct lws_cgi_args args;
#endif
#if defined(LWS_WITH_CGI) || !defined(LWS_NO_CLIENT)
	int reason_bf;
#endif
	unsigned int client_finished:1;


	struct lws_spa *spa;
	char result[500 + LWS_PRE];
	int result_len;

	char filename[256];
	long file_length;
	lws_filefd_type post_fd;
};

/*
 * one of these is auto-created for each connection and a pointer to the
 * appropriate instance is passed to the callback in the user parameter
 *
 * for this example protocol we use it to individualize the count for each
 * connection.
 */

struct per_session_data__dumb_increment {
	int number;
};

struct per_session_data__lws_mirror {
	struct lws *wsi;
	int ringbuffer_tail;
};

struct per_session_data__echogen {
	size_t total;
	size_t total_rx;
	int fd;
	int fragsize;
	int wr;
};

struct per_session_data__lws_status {
	struct per_session_data__lws_status *list;
	struct timeval tv_established;
	int last;
	char ip[270];
	char user_agent[512];
	const char *pos;
	int len;
};

extern int
callback_lws_mirror(struct lws *wsi, enum lws_callback_reasons reason,
		    void *user, void *in, size_t len);
extern int
callback_dumb_increment(struct lws *wsi, enum lws_callback_reasons reason,
			void *user, void *in, size_t len);
extern int
callback_lws_echogen(struct lws *wsi, enum lws_callback_reasons reason,
			void *user, void *in, size_t len);
extern int
callback_lws_status(struct lws *wsi, enum lws_callback_reasons reason,
		    void *user, void *in, size_t len);


extern void
dump_handshake_info(struct lws *wsi);

#endif
