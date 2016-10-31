#ifndef LWS_HTTP_H
#define LWS_HTTP_H

extern int callback_http(struct lws *wsi, enum lws_callback_reasons reason, void *user,
			 void *in, size_t len);


#endif
