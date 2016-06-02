#ifndef INCLUDE_UV_HTTP_H_
#define INCLUDE_UV_HTTP_H_

#include "uv.h"
#include "uv_link_t.h"

/* NOTE: can be cast to `uv_link_t` */
typedef struct uv_http_s uv_http_t;
typedef struct uv_http_req_s uv_http_req_t;

typedef void (*uv_http_req_handler_cb)(uv_http_t* http, const char* url,
                                       size_t url_size);
typedef int (*uv_http_req_cb)(uv_http_req_t* req);
typedef int (*uv_http_req_value_cb)(uv_http_req_t* req, const char* value,
                                    size_t length);
typedef void (*uv_http_req_active_cb)(uv_http_req_t* req, int status);

enum uv_http_method_e {
  UV_HTTP_DELETE,
  UV_HTTP_GET,
  UV_HTTP_HEAD,
  UV_HTTP_POST,
  UV_HTTP_PUT,
  UV_HTTP_CONNECT,
  UV_HTTP_OPTIONS,
  UV_HTTP_TRACE,
  UV_HTTP_COPY,
  UV_HTTP_LOCK,
  UV_HTTP_MKCOL,
  UV_HTTP_MOVE,
  UV_HTTP_PROPFIND,
  UV_HTTP_PROPPATCH,
  UV_HTTP_SEARCH,
  UV_HTTP_UNLOCK,
  UV_HTTP_BIND,
  UV_HTTP_REBIND,
  UV_HTTP_UNBIND,
  UV_HTTP_ACL,
  UV_HTTP_REPORT,
  UV_HTTP_MKACTIVITY,
  UV_HTTP_CHECKOUT,
  UV_HTTP_MERGE,
  UV_HTTP_MSEARCH,
  UV_HTTP_NOTIFY,
  UV_HTTP_SUBSCRIBE,
  UV_HTTP_UNSUBSCRIBE,
  UV_HTTP_PATCH,
  UV_HTTP_PURGE,
  UV_HTTP_MKCALENDAR,
  UV_HTTP_LINK,
  UV_HTTP_UNLINK
};
typedef enum uv_http_method_e uv_http_method_t;

/* Flags, actually */
enum uv_http_req_state_e {
  kUVHTTPReqStateEnd = 0x1,
  kUVHTTPReqStateFinished = 0x2,
};
typedef enum uv_http_req_state_e uv_http_req_state_t;

struct uv_http_req_s {
  UV_LINK_FIELDS

  /* Public fields */
  uv_http_t* http;

  unsigned short http_major;
  unsigned short http_minor;
  uv_http_method_t method;

  /* Use chunked encoding, `1` by default */
  unsigned int chunked:1;

  uv_http_req_value_cb on_header_field;
  uv_http_req_value_cb on_header_value;
  uv_http_req_cb on_headers_complete;

  /* Invoked when the request becomes available for sending response and
   * doing writes
   * (Semi-private, use `uv_http_req_on_active()`)
   */
  uv_http_req_active_cb on_active;

  /* Private fields */
  uv_http_req_state_t state;
  unsigned int reading: 1;
  unsigned int pending_eof: 1;
  uv_http_req_t* next;
};

UV_EXTERN uv_http_t* uv_http_create(uv_http_req_handler_cb cb, int* err);
UV_EXTERN int uv_http_accept(uv_http_t* http, uv_http_req_t* req);

/* Request */

/* NOTE: `cb` may be executed synchronously */
UV_EXTERN void uv_http_req_on_active(uv_http_req_t* req,
                                     uv_http_req_active_cb cb);

UV_EXTERN int uv_http_req_respond(uv_http_req_t* req,
                                  unsigned short status,
                                  const uv_buf_t* message,
                                  const uv_buf_t header_fields[],
                                  const uv_buf_t header_values[],
                                  unsigned int header_count);

#endif  /* INCLUDE_UV_LINK_H_ */
