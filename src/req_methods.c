#include "src/common.h"

static int uv_http_req_link_read_start(uv_link_t* link) {
  uv_http_req_t* req;

  req = (uv_http_req_t*) link;
  if (req->pending_err != 0)
    return req->pending_err;

  if (req->reading)
    return 0;

  if (req->pending_eof) {
    uv_link_propagate_read_cb(link, UV_EOF, NULL);
    req->pending_eof = 0;
  }

  req->reading = 1;
  if (req->http->last_req == req)
    return uv_http_read_start(req->http, kUVHTTPSideRequest);
  else
    return 0;
}


static int uv_http_req_link_read_stop(uv_link_t* link) {
  uv_http_req_t* req;

  req = (uv_http_req_t*) link;
  if (req->pending_err != 0)
    return req->pending_err;

  if (!req->reading)
    return 0;

  req->reading = 0;
  if (req->http->last_req == req)
    return uv_http_read_stop(req->http, kUVHTTPSideRequest);
  else
    return 0;
}


static int uv_http_req_link_write(uv_link_t* link,
                                  uv_link_t* source,
                                  const uv_buf_t bufs[],
                                  unsigned int nbufs,
                                  uv_stream_t* send_handle,
                                  uv_link_write_cb cb,
                                  void* arg) {
  uv_http_req_t* req;
  uv_buf_t buf_storage[1024];
  char prefix[1024];
  uv_buf_t* pbufs;
  int err;

  req = (uv_http_req_t*) link;
  if (req->pending_err != 0)
    return req->pending_err;

  /* No IPC on HTTP requests */
  if (send_handle != NULL)
    return UV_ENOSYS;

  /* Write works only for an active request */
  if (req->http->active_req != req)
    return UV_EAGAIN;

  err = uv_http_req_prepare_write(req, prefix, sizeof(prefix),
                                  buf_storage, ARRAY_SIZE(buf_storage),
                                  bufs, nbufs, &pbufs, &nbufs);
  if (err != 0)
    return err;

  err = uv_link_propagate_write(req->http->parent, source, pbufs, nbufs, NULL,
                                cb, arg);
  if (pbufs != buf_storage && pbufs != bufs)
    free(pbufs);

  return err;
}


static int uv_http_req_link_try_write(uv_link_t* link,
                                      const uv_buf_t bufs[],
                                      unsigned int nbufs) {
  uv_http_req_t* req;

  req = (uv_http_req_t*) link;
  if (req->pending_err != 0)
    return req->pending_err;
  /* Try write is not supported because of chunked encoding */
  return UV_ENOSYS;
}


static int uv_http_req_link_shutdown(uv_link_t* link,
                                     uv_link_t* source,
                                     uv_link_shutdown_cb cb,
                                     void* arg) {
  uv_http_req_t* req;
  uv_buf_t buf;
  int err;

  req = (uv_http_req_t*) link;
  if (req->pending_err != 0)
    return req->pending_err;

  /* Shutdown works only for an active request */
  if (req->http->active_req != req)
    return UV_EAGAIN;

  /* TODO(indutny): invoke callback for non-chunked */
  if (!req->chunked || !req->has_response || req->shutdown)
    return kUVHTTPErrShutdownNotChunked;

  buf = uv_buf_init("0\r\n\r\n", 5);
  err = uv_link_propagate_write(req->http->parent, source, &buf, 1, NULL, cb,
                                arg);
  if (err != 0)
    return err;

  req->shutdown = 1;
  uv_http_on_req_finish(req->http, req);
  return 0;
}


static void uv_http_req_link_close(uv_link_t* link, uv_link_t* source,
                                   uv_link_close_cb cb) {
  uv_http_req_t* req;

  req = (uv_http_req_t*) link;
  req->reading = 0;

  /* Request wasn't shutdown */
  if (!req->shutdown) {
    uv_http_error(req->http, kUVHTTPErrCloseWithoutShutdown);
    uv_http_on_req_finish(req->http, req);
  }

  uv_http_close_req(req->http, req);
  uv_http_maybe_close(req->http);

  cb(source);
}


uv_link_methods_t uv_http_req_methods = {
  .read_start = uv_http_req_link_read_start,
  .read_stop = uv_http_req_link_read_stop,

  .write = uv_http_req_link_write,
  .try_write = uv_http_req_link_try_write,
  .shutdown = uv_http_req_link_shutdown,
  .close = uv_http_req_link_close,
  .strerror = uv_http_link_strerror
};
