#include "src/common.h"

static int uv_http_req_link_read_start(uv_link_t* link) {
  uv_http_req_t* req;

  /* TODO(indutny): check pending eof */
  req = (uv_http_req_t*) link;
  req->reading = 1;
  return 0;
}


static int uv_http_req_link_read_stop(uv_link_t* link) {
  uv_http_req_t* req;

  req = (uv_http_req_t*) link;
  req->reading = 0;
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
  uv_buf_t* pbufs;
  int err;

  req = (uv_http_req_t*) link;

  /* No IPC on HTTP requests */
  if (send_handle != NULL)
    return UV_ENOSYS;

  /* Write works only for an active request */
  if (req->http->active_req != req)
    return UV_EAGAIN;

  err = uv_http_req_prepare_write(req, buf_storage, ARRAY_SIZE(buf_storage),
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
  /* Try write is not supported because of chunked encoding */
  return UV_ENOSYS;
}


static int uv_http_req_link_shutdown(uv_link_t* link,
                                     uv_link_t* source,
                                     uv_link_shutdown_cb cb,
                                     void* arg) {
  /* TODO(indutny): implement me */
  return UV_EPROTO;
}


static void uv_http_req_link_close(uv_link_t* link, uv_link_t* source,
                                   uv_link_close_cb cb) {
  uv_http_req_t* req;

  req = (uv_http_req_t*) link;
  req->reading = 0;

  uv_http_on_req_close(req->http, req);

  cb(source);
}


uv_link_methods_t uv_http_req_methods = {
  .read_start = uv_http_req_link_read_start,
  .read_stop = uv_http_req_link_read_stop,

  .write = uv_http_req_link_write,
  .try_write = uv_http_req_link_try_write,
  .shutdown = uv_http_req_link_shutdown,
  .close = uv_http_req_link_close
};
