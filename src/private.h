#ifndef SRC_PRIVATE_H_
#define SRC_PRIVATE_H_

#include "http_parser.h"
#include "uv_link_t.h"

#include "src/utils.h"
#include "src/queue.h"

struct uv_http_s {
  UV_LINK_FIELDS

  uv_http_req_handler_cb request_handler;

  struct {
    unsigned short http_major;
    unsigned short http_minor;
    uv_http_method_t method;
  } tmp_req;
  uv_http_req_t* current_req;
  unsigned int active_reqs;

  uv_link_close_cb close_cb;
  uv_link_t* close_source;

  unsigned int reading:2;

  http_parser parser;
  http_parser_settings parser_settings;

  uv_http_data_t pending_data;
  uv_http_data_t pending_req_data;
  uv_http_data_t pending_url;

  /* TODO(indutny): consider increasing it, the idea is to handle the
   * most common length */
  char url_storage[512];
};

/* NOTE: used as flags too */
enum uv_http_side_e {
  kUVHTTPSideRequest = 1,
  kUVHTTPSideConnection = 2
};
typedef enum uv_http_side_e uv_http_side_t;

static const unsigned int kReadingMask = kUVHTTPSideRequest |
                                         kUVHTTPSideConnection;

uv_link_methods_t uv_http_methods;
uv_link_methods_t uv_http_req_methods;

void uv_http_destroy(uv_http_t* http, uv_link_t* source, uv_link_close_cb cb);

int uv_http_consume(uv_http_t* http, const char* data, size_t size);
void uv_http_error(uv_http_t* http, int err);
void uv_http_on_req_close(uv_http_t* http, uv_http_req_t* req);

void uv_http_req_error(uv_http_t* http, uv_http_req_t* req, int err);
void uv_http_req_eof(uv_http_t* http, uv_http_req_t* req);
int uv_http_req_consume(uv_http_t* http, uv_http_req_t* req,
                        const char* data, size_t size);

int uv_http_read_start(uv_http_t* http, uv_http_side_t side);
int uv_http_read_stop(uv_http_t* http, uv_http_side_t side);

#endif  /* SRC_PRIVATE_H_ */
