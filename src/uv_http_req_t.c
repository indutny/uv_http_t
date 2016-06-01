#include "src/common.h"

void uv_http_req_eof(uv_http_t* http, uv_http_req_t* req) {
  if (req->reading)
    return uv_link_propagate_read_cb((uv_link_t*) req, UV_EOF, NULL);

  req->pending_eof = 1;
}


int uv_http_req_consume(uv_http_t* http, uv_http_req_t* req,
                        const char* data, size_t size) {
  int err;

  while (req->reading && size != 0) {
    uv_buf_t buf;
    size_t avail;

    uv_link_propagate_alloc_cb((uv_link_t*) req, size, &buf);
    if (buf.len == 0) {
      uv_link_propagate_read_cb((uv_link_t*) req, UV_ENOBUFS, &buf);
      return 0;
    }

    avail = buf.len;
    if (avail > size)
      avail = size;

    memcpy(buf.base, data, avail);
    uv_link_propagate_read_cb((uv_link_t*) req, avail, &buf);

    size -= avail;
    data += avail;
  }

  /* Fully consumed */
  if (size == 0)
    return 0;

  /* Pending data */
  err = uv_http_queue_pending(&http->pending_req_data, data, size);
  if (err != 0) {
    uv_http_req_error(http, req, err);
    return 0;
  }

  return 0;
}


void uv_http_req_error(uv_http_t* http, uv_http_req_t* req, int err) {
}
