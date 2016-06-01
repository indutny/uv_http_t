#include <stdlib.h>
#include <string.h>

#include "src/common.h"


static int uv_http_on_message_begin(http_parser* parser);
static int uv_http_on_headers_complete(http_parser* parser);
static int uv_http_on_message_complete(http_parser* parser);
static int uv_http_on_url(http_parser* parser, const char* value,
                          size_t length);
static int uv_http_on_header_field(http_parser* parser, const char* value,
                                   size_t length);
static int uv_http_on_header_value(http_parser* parser, const char* value,
                                   size_t length);
static int uv_http_on_body(http_parser* parser, const char* value,
                           size_t length);
static uv_http_method_t uv_http_convert_method(enum http_method method);
static int uv_http_process_pending(uv_http_t* http, uv_http_side_t side);

uv_http_t* uv_http_create(uv_http_req_handler_cb cb, int* err) {
  uv_http_t* res;
  http_parser_settings* settings;

  res = calloc(1, sizeof(*res));
  if (res == NULL) {
    *err = UV_ENOMEM;
    return NULL;
  }

  res->request_handler = cb;

  *err = uv_link_init((uv_link_t*) res, &uv_http_methods);
  if (*err != 0)
    goto fail_link_init;

  http_parser_init(&res->parser, HTTP_REQUEST);
  http_parser_pause(&res->parser, 1);
  http_parser_settings_init(&res->parser_settings);
  settings = &res->parser_settings;

  settings->on_message_begin = uv_http_on_message_begin;
  settings->on_url = uv_http_on_url;
  settings->on_header_field = uv_http_on_header_field;
  settings->on_header_value = uv_http_on_header_value;
  settings->on_headers_complete = uv_http_on_headers_complete;
  settings->on_body = uv_http_on_body;
  settings->on_message_complete = uv_http_on_message_complete;

  /* read_start() should not be blocked by this */
  res->reading |= (unsigned int) kUVHTTPSideRequest;

  return res;

fail_link_init:
  free(res);
  return NULL;
}


void uv_http_destroy(uv_http_t* http, uv_link_t* source, uv_link_close_cb cb) {
  /* TODO(indutny): implement me */
}


int uv_http_consume(uv_http_t* http, const char* data, size_t size) {
  size_t parsed;

  parsed = http_parser_execute(&http->parser, &http->parser_settings, data,
                               size);

  if (http->parser.http_errno != HPE_OK &&
      http->parser.http_errno != HPE_PAUSED) {
    /* Parser error */
    /* TODO(indutny): error message? */
    return UV_EPROTO;
  }

  CHECK(parsed <= size, "Parsed more than given!");

  /* Fully parsed */
  if (parsed == size)
    return 0;

  /* Store pending data for later use */
  return uv_http_queue_pending(&http->pending_data, data + parsed,
                               size - parsed);
}


int uv_http_queue_pending(uv_http_pending_t* buf, const char* data,
                          size_t size) {
  buf->size = size;
  buf->bytes = malloc(size);
  if (buf->bytes == NULL)
    return UV_ENOMEM;

  memcpy(buf->bytes, data, size);
  return 0;
}


int uv_http_process_pending(uv_http_t* http, uv_http_side_t side) {
  uv_http_pending_t* buf;
  char* bytes;
  size_t size;
  int err;

  buf = side == kUVHTTPSideConnection ? &http->pending_data :
                                        &http->pending_req_data;

  bytes = buf->bytes;
  if (bytes == NULL)
    return 0;

  size = buf->size;

  buf->bytes = NULL;
  buf->size = 0;

  if (side == kUVHTTPSideConnection)
    err = uv_http_consume(http, bytes, size);
  else
    err = uv_http_req_consume(http, http->last_req, bytes, size);

  if (err != 0)
    return err;

  free(bytes);

  return 0;
}


void uv_http_error(uv_http_t* http, int err) {
  /* TODO(indutny): implement me */
}


int uv_http_accept(uv_http_t* http, uv_http_req_t* req) {
  int err;

  err = uv_link_init((uv_link_t*) req, &uv_http_req_methods);
  if (err != 0)
    return err;

  *req = http->tmp_req;
  http->last_req =req;
  return 0;
}


int uv_http_read_start(uv_http_t* http, uv_http_side_t side) {
  int err;

  if (side == kUVHTTPSideRequest &&
      (http->reading & kUVHTTPSideRequest) == 0) {
    err = uv_http_process_pending(http, side);
    if (err != 0)
      return err;
  }

  http->reading |= (unsigned int) side;

  /* Wait until both reading sides will request reads */
  if ((http->reading & kReadingMask) != kReadingMask)
    return 0;

  http_parser_pause(&http->parser, 0);
  err = uv_http_process_pending(http, kUVHTTPSideConnection);
  if (err != 0)
    return err;

  return uv_link_read_start(http->parent);
}


int uv_http_read_stop(uv_http_t* http, uv_http_side_t side) {
  http->reading &= ~(unsigned int) side;
  http_parser_pause(&http->parser, 1);
  return uv_link_read_stop(http->parent);
}

/* Parser callbacks */


int uv_http_on_message_begin(http_parser* parser) {
  uv_http_t* http;
  uv_http_req_t* req;

  http = container_of(parser, uv_http_t, parser);

  req = &http->tmp_req;
  req->http_major = parser->http_major;
  req->http_minor = parser->http_minor;
  req->method = uv_http_convert_method(parser->method);

  http->last_req = NULL;

  return 0;
}


int uv_http_on_url(http_parser* parser, const char* value,
                   size_t length) {
  uv_http_t* http;
  http = container_of(parser, uv_http_t, parser);

  http->request_handler(http, value, length);
  /* TODO(indutny): is there any reason to loosen this? */
  CHECK_NE(http->last_req, NULL, "request_handler must accept request");

  return 0;
}


int uv_http_on_headers_complete(http_parser* parser) {
  uv_http_t* http;
  http = container_of(parser, uv_http_t, parser);
  return http->last_req->on_headers_complete(http->last_req);
}


int uv_http_on_message_complete(http_parser* parser) {
  uv_http_t* http;
  uv_http_req_t* req;

  http = container_of(parser, uv_http_t, parser);
  req = http->last_req;

  uv_http_req_eof(http, req);

  /* Change of requests */
  http->last_req = NULL;
  http->reading |= (unsigned int) kUVHTTPSideRequest;

  return 0;
}


int uv_http_on_header_field(http_parser* parser, const char* value,
                            size_t length) {
  uv_http_t* http;
  http = container_of(parser, uv_http_t, parser);
  return http->last_req->on_header_field(http->last_req, value, length);
}


int uv_http_on_header_value(http_parser* parser, const char* value,
                            size_t length) {
  uv_http_t* http;
  http = container_of(parser, uv_http_t, parser);
  return http->last_req->on_header_value(http->last_req, value, length);
}


int uv_http_on_body(http_parser* parser, const char* value, size_t length) {
  uv_http_t* http;
  uv_http_req_t* req;

  http = container_of(parser, uv_http_t, parser);
  req = http->last_req;

  uv_http_req_consume(http, req, value, length);
  if (!req->reading && uv_http_read_stop(http, kUVHTTPSideRequest) != 0)
    return -1;

  return 0;
}


/* Routines */


uv_http_method_t uv_http_convert_method(enum http_method method) {
  switch (method) {
    case HTTP_DELETE: return UV_HTTP_DELETE;
    case HTTP_GET: return UV_HTTP_GET;
    case HTTP_HEAD: return UV_HTTP_HEAD;
    case HTTP_POST: return UV_HTTP_POST;
    case HTTP_PUT: return UV_HTTP_PUT;
    case HTTP_CONNECT: return UV_HTTP_CONNECT;
    case HTTP_OPTIONS: return UV_HTTP_OPTIONS;
    case HTTP_TRACE: return UV_HTTP_TRACE;
    case HTTP_COPY: return UV_HTTP_COPY;
    case HTTP_LOCK: return UV_HTTP_LOCK;
    case HTTP_MKCOL: return UV_HTTP_MKCOL;
    case HTTP_MOVE: return UV_HTTP_MOVE;
    case HTTP_PROPFIND: return UV_HTTP_PROPFIND;
    case HTTP_PROPPATCH: return UV_HTTP_PROPPATCH;
    case HTTP_SEARCH: return UV_HTTP_SEARCH;
    case HTTP_UNLOCK: return UV_HTTP_UNLOCK;
    case HTTP_BIND: return UV_HTTP_BIND;
    case HTTP_REBIND: return UV_HTTP_REBIND;
    case HTTP_UNBIND: return UV_HTTP_UNBIND;
    case HTTP_ACL: return UV_HTTP_ACL;
    case HTTP_REPORT: return UV_HTTP_REPORT;
    case HTTP_MKACTIVITY: return UV_HTTP_MKACTIVITY;
    case HTTP_CHECKOUT: return UV_HTTP_CHECKOUT;
    case HTTP_MERGE: return UV_HTTP_MERGE;
    case HTTP_MSEARCH: return UV_HTTP_MSEARCH;
    case HTTP_NOTIFY: return UV_HTTP_NOTIFY;
    case HTTP_SUBSCRIBE: return UV_HTTP_SUBSCRIBE;
    case HTTP_UNSUBSCRIBE: return UV_HTTP_UNSUBSCRIBE;
    case HTTP_PATCH: return UV_HTTP_PATCH;
    case HTTP_PURGE: return UV_HTTP_PURGE;
    case HTTP_MKCALENDAR: return UV_HTTP_MKCALENDAR;
    case HTTP_LINK: return UV_HTTP_LINK;
    case HTTP_UNLINK: return UV_HTTP_UNLINK;
    default: CHECK(0, "Unexpected method");
  }
}
