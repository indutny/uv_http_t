#include "src/common.h"
#include "src/utils.h"

static void uv_http_req_write_cb(uv_link_t* link, int status, void* arg);

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
  err = uv_http_data_queue(&http->pending.req_data, data, size);
  if (err != 0) {
    uv_http_req_error(http, req, err);
    return 0;
  }

  return 0;
}


void uv_http_req_error(uv_http_t* http, uv_http_req_t* req, int err) {
  /* TODO(indutny): implement me */
}


void uv_http_req_on_active(uv_http_req_t* req, uv_http_req_active_cb cb) {
  if (req->http->active_req == req)
    return cb(req, 0);

  req->on_active = cb;
}


void uv_http_req_write_cb(uv_link_t* link, int status, void* arg) {
  /* TODO(indutny): handle error */
}


int uv_http_req_respond(uv_http_req_t* req,
                        unsigned short status,
                        const uv_buf_t* message,
                        const uv_buf_t header_fields[],
                        const uv_buf_t header_values[],
                        unsigned int header_count) {
  uv_http_t* http;
  uv_buf_t buf_storage[1024];
  char status_str[16];
  size_t status_len;
  uv_buf_t* bufs;
  uv_buf_t* pbufs;
  unsigned int nbufs;
  unsigned int i;
  int err;
  size_t total;

  http = req->http;

  if (req->http->active_req != req)
    return UV_EAGAIN;

  /* TODO(indutny): default message */
  nbufs = header_count * 4 + 3;
  if (req->chunked)
    nbufs++;

  if (nbufs > ARRAY_SIZE(buf_storage))
    bufs = malloc(nbufs * sizeof(*bufs));
  else
    bufs = buf_storage;

  if (bufs == NULL)
    return UV_ENOMEM;

  status_len = snprintf(status_str, sizeof(status_str), "HTTP/%hu.%hu %hu ",
                        req->http_major, req->http_minor, status);

  bufs[0] = uv_buf_init(status_str, status_len);
  bufs[1] = *message;
  bufs[2] = uv_buf_init("\r\n", 2);
  for (i = 0; i < header_count; i++) {
    bufs[i * 4 + 3] = header_fields[i];
    bufs[i * 4 + 4] = uv_buf_init(": ", 2);
    bufs[i * 4 + 5] = header_values[i];
    bufs[i * 4 + 6] = uv_buf_init("\r\n", 2);
  }
  if (req->chunked)
    bufs[nbufs - 1] = uv_buf_init("Transfer-Encoding: chunked\r\n\r\n", 30);
  else
    bufs[nbufs - 1] = uv_buf_init("\r\n\r\n", 4);

  total = 0;
  for (i = 0; i < nbufs; i++)
    total += bufs[i].len;

  err = uv_link_try_write(http->parent, bufs, nbufs);
  if (err < 0)
    goto done;

  /* Fully written */
  if ((size_t) err == total) {
    err = 0;
    goto done;
  }

  /* Partial write, queue rest */
  pbufs = bufs;
  for (; nbufs > 0; pbufs++, nbufs--) {
    /* Slice */
    if (pbufs[0].len > (size_t) err) {
      pbufs[0].base += err;
      pbufs[0].len -= err;
      err = 0;
      break;

    /* Discard */
    } else {
      err -= pbufs[0].len;
    }
  }

  err = uv_link_propagate_write(http->parent, (uv_link_t*) req, pbufs, nbufs,
                                NULL, uv_http_req_write_cb, NULL);

done:
  if (bufs != buf_storage)
    free(bufs);

  return err;
}
