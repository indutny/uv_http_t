#include "src/common.h"

static int uv_http_link_read_start(uv_link_t* link) {
  uv_http_t* http;
  http = (uv_http_t*) link;

  return uv_http_read_start(http, kUVHTTPSideConnection);
}


static int uv_http_link_read_stop(uv_link_t* link) {
  uv_http_t* http;
  http = (uv_http_t*) link;

  return uv_http_read_stop(http, kUVHTTPSideConnection);
}


static int uv_http_link_write(uv_link_t* link,
                              uv_link_t* source,
                              const uv_buf_t bufs[],
                              unsigned int nbufs,
                              uv_stream_t* send_handle,
                              uv_link_write_cb cb,
                              void* arg) {
  /* No support for writes */
  return UV_ENOSYS;
}


static int uv_http_link_try_write(uv_link_t* link,
                                  const uv_buf_t bufs[],
                                  unsigned int nbufs) {
  /* No support for writes */
  return UV_ENOSYS;
}


static int uv_http_link_shutdown(uv_link_t* link,
                                 uv_link_t* source,
                                 uv_link_shutdown_cb cb,
                                 void* arg) {
  return uv_link_propagate_shutdown(link->parent, source, cb, arg);
}


static void uv_http_link_close(uv_link_t* link, uv_link_t* source,
                               uv_link_close_cb cb) {
  uv_http_t* http;
  http = (uv_http_t*) link;

  uv_http_destroy(http, source, cb);
}


/* TODO(indutny): multi-threading? */
static char shared_storage[16 * 1024];
static int shared_storage_busy;


static void uv_http_link_alloc_cb_override(uv_link_t* link,
                                           size_t suggested_size,
                                           uv_buf_t* buf) {
  if (shared_storage_busy) {
    *buf = uv_buf_init(malloc(suggested_size), suggested_size);
  } else {
    /* Most likely case */
    *buf = uv_buf_init(shared_storage, sizeof(shared_storage));
    shared_storage_busy = 1;
  }

  if (buf->base == NULL)
    buf->len = 0;
}


static void uv_http_link_read_cb_override(uv_link_t* link,
                                          ssize_t nread,
                                          const uv_buf_t* buf) {
  uv_http_t* http;
  int err;

  http = (uv_http_t*) link;

  if (nread >= 0)
    err = uv_http_consume(http, buf->base, nread);
  else
    err = nread;

  if (err != 0)
    uv_http_error(http, err);

  shared_storage_busy = 0;
  if (buf->base != shared_storage)
    free(buf->base);
}


uv_link_methods_t uv_http_methods = {
  .read_start = uv_http_link_read_start,
  .read_stop = uv_http_link_read_stop,

  .write = uv_http_link_write,
  .try_write = uv_http_link_try_write,
  .shutdown = uv_http_link_shutdown,
  .close = uv_http_link_close,

  .alloc_cb_override = uv_http_link_alloc_cb_override,
  .read_cb_override = uv_http_link_read_cb_override
};
