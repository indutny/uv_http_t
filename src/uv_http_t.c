#include <stdlib.h>
#include <string.h>

#include "src/common.h"
#include "src/utils.h"


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

static int uv_http_process_pending(uv_http_t* http, uv_http_side_t side);
static int uv_http_emit_req(uv_http_t* http);
static void uv_http_maybe_close(uv_http_t* http);
static void uv_http_free(uv_http_t* http);
static int uv_http_on_field(uv_http_t* http, uv_http_header_state_t next,
                            const char* value, size_t size);

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

  uv_http_data_init(&res->pending.data, NULL, 0);
  uv_http_data_init(&res->pending.req_data, NULL, 0);
  uv_http_data_init(&res->pending.url_or_header, res->pending.storage,
                    sizeof(res->pending.storage));

  return res;

fail_link_init:
  free(res);
  return NULL;
}


void uv_http_free(uv_http_t* http) {
  uv_http_data_free(&http->pending.data);
  uv_http_data_free(&http->pending.req_data);
  uv_http_data_free(&http->pending.url_or_header);
  free(http);
}


void uv_http_destroy(uv_http_t* http, uv_link_t* source, uv_link_close_cb cb) {
  /* Wait for all active reqs to go down */
  if (http->active_reqs > 0) {
    CHECK_EQ(http->close_cb, NULL, "double close attempt");

    http->close_cb = cb;
    http->close_source = source;
    return;
  }

  cb(source);
  uv_http_free(http);
}


void uv_http_maybe_close(uv_http_t* http) {
  uv_link_close_cb cb;
  uv_link_t* source;

  if (http->close_cb == NULL)
    return;

  if (http->active_reqs > 0)
    return;

  cb = http->close_cb;
  source = http->close_source;
  http->close_cb = NULL;
  http->close_source = NULL;

  cb(source);
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
  return uv_http_data_queue(&http->pending.data, data + parsed, size - parsed);
}


int uv_http_process_pending(uv_http_t* http, uv_http_side_t side) {
  uv_http_data_t* data;
  char* bytes;
  size_t size;
  int err;

  data = side == kUVHTTPSideConnection ? &http->pending.data :
                                         &http->pending.req_data;
  if (data->size == 0)
    return 0;

  bytes = data->value;
  size = data->size;

  if (side == kUVHTTPSideConnection)
    err = uv_http_consume(http, bytes, size);
  else
    err = uv_http_req_consume(http, http->current_req, bytes, size);

  uv_http_data_dequeue(data, size);

  if (err != 0)
    return err;

  free(bytes);

  return 0;
}


void uv_http_on_req_close(uv_http_t* http, uv_http_req_t* req) {
  http->active_reqs--;
  if (http->current_req == req)
    http->current_req = NULL;
  uv_http_maybe_close(http);
}


void uv_http_error(uv_http_t* http, int err) {
  /* TODO(indutny): implement me */
}


int uv_http_accept(uv_http_t* http, uv_http_req_t* req) {
  int err;

  err = uv_link_init((uv_link_t*) req, &uv_http_req_methods);
  if (err != 0)
    return err;

  req->http_major = http->tmp_req.http_major;
  req->http_minor = http->tmp_req.http_minor;
  req->method = http->tmp_req.method;

  req->http = http;
  http->current_req = req;
  http->active_reqs++;
  return 0;
}


int uv_http_read_start(uv_http_t* http, uv_http_side_t side) {
  unsigned int old;
  int err;

  if (side == kUVHTTPSideRequest &&
      (http->reading & kUVHTTPSideRequest) == 0) {
    err = uv_http_process_pending(http, side);
    if (err != 0)
      return err;
  }

  old = http->reading;
  http->reading |= (unsigned int) side;
  if (old == http->reading)
    return 0;

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
  unsigned int old;

  old = http->reading;
  http->reading &= ~(unsigned int) side;
  if (old == http->reading)
    return 0;

  http_parser_pause(&http->parser, 1);
  return uv_link_read_stop(http->parent);
}


int uv_http_emit_req(uv_http_t* http) {
  uv_http_data_t* url;

  url = &http->pending.url_or_header;
  http->request_handler(http, url->value, url->size);

  /* TODO(indutny): is there any reason to loosen this? */
  CHECK_NE(http->current_req, NULL, "request_handler must accept request");

  /* Requests starts in not-reading mode */
  if (!http->current_req->reading)
    return uv_http_read_stop(http, kUVHTTPSideRequest);

  return 0;
}


int uv_http_on_field(uv_http_t* http, uv_http_header_state_t next,
                     const char* value, size_t size) {
  uv_http_data_t* data;
  uv_http_req_t* req;

  data = &http->pending.url_or_header;
  req = http->current_req;

  /* Transition */
  if (http->pending.header_state != next) {
    int err;

    err = 0;
    switch (http->pending.header_state) {
      case kUVHTTPHeaderStateURL:
        err = uv_http_emit_req(http);
        if (err != 0)
          return err;
        req = http->current_req;
        break;
      case kUVHTTPHeaderStateField:
        if (req->on_header_field != NULL)
          err = req->on_header_field(req, data->value, data->size);
        break;
      case kUVHTTPHeaderStateValue:
        if (req->on_header_value != NULL)
          err = req->on_header_value(req, data->value, data->size);
        break;
      default:
        CHECK(0, "Unexpected transition");
        break;
    }

    /* Here `err` is a parser-like error */
    if (err != 0)
      return UV_EPROTO;

    uv_http_data_dequeue(data, data->size);
    http->pending.header_state = next;
  }

  /* Faciliate light requests */
  switch (http->pending.header_state) {
    case kUVHTTPHeaderStateField:
      if (req->on_header_field == NULL)
        return 0;
      break;
    case kUVHTTPHeaderStateValue:
      if (req->on_header_value == NULL)
        return 0;
      break;
    case kUVHTTPHeaderStateComplete:
      if (req->on_headers_complete == NULL)
        return 0;
      req->on_headers_complete(req);
      break;
    default:
      break;
  }

  if (size != 0)
    return uv_http_data_queue(data, value, size);

  return 0;
}


/* Parser callbacks */


int uv_http_on_message_begin(http_parser* parser) {
  uv_http_t* http;

  http = container_of(parser, uv_http_t, parser);

  http->tmp_req.http_major = parser->http_major;
  http->tmp_req.http_minor = parser->http_minor;
  http->tmp_req.method = uv_http_convert_method(parser->method);

  http->current_req = NULL;
  http->pending.header_state = kUVHTTPHeaderStateURL;

  return 0;
}


int uv_http_on_url(http_parser* parser, const char* value,
                   size_t length) {
  uv_http_t* http;
  int err;

  http = container_of(parser, uv_http_t, parser);

  err = uv_http_on_field(http, kUVHTTPHeaderStateURL, value, length);
  if (err != 0) {
    uv_http_error(http, err);
    return -1;
  }
  return 0;
}


int uv_http_on_headers_complete(http_parser* parser) {
  uv_http_t* http;
  int err;

  http = container_of(parser, uv_http_t, parser);

  err = uv_http_on_field(http, kUVHTTPHeaderStateComplete, NULL, 0);
  if (err != 0) {
    uv_http_error(http, err);
    return -1;
  }
  return 0;
}


int uv_http_on_header_field(http_parser* parser, const char* value,
                            size_t length) {
  uv_http_t* http;
  int err;
  http = container_of(parser, uv_http_t, parser);
  err = uv_http_on_field(http, kUVHTTPHeaderStateField, value, length);
  if (err != 0) {
    uv_http_error(http, err);
    return -1;
  }
  return 0;
}


int uv_http_on_header_value(http_parser* parser, const char* value,
                            size_t length) {
  uv_http_t* http;
  int err;
  http = container_of(parser, uv_http_t, parser);
  err = uv_http_on_field(http, kUVHTTPHeaderStateValue, value, length);
  if (err != 0) {
    uv_http_error(http, err);
    return -1;
  }
  return 0;
}


int uv_http_on_body(http_parser* parser, const char* value, size_t length) {
  uv_http_t* http;
  uv_http_req_t* req;

  http = container_of(parser, uv_http_t, parser);
  req = http->current_req;

  uv_http_req_consume(http, req, value, length);
  if (!req->reading && uv_http_read_stop(http, kUVHTTPSideRequest) != 0)
    return -1;

  return 0;
}


int uv_http_on_message_complete(http_parser* parser) {
  uv_http_t* http;
  uv_http_req_t* req;

  http = container_of(parser, uv_http_t, parser);
  req = http->current_req;

  uv_http_req_eof(http, req);

  /* Change of requests */
  http->current_req = NULL;
  http->reading |= (unsigned int) kUVHTTPSideRequest;

  return 0;
}
