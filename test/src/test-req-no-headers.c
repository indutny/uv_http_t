#include "test-common.h"

static int read_called;
static int req_closed;

static uv_http_req_t req;
static uv_link_observer_t req_observer;

static void req_no_headers_client(int fd) {
  client_send_str("POST /so");
  /* Let it through */
  usleep(100000);
  client_send_str("me/path HTTP/1.1\r\nContent-Length: 1\r\n\r\na");
}


static void req_close_cb(uv_link_t* req) {
  req_closed++;
}


static void req_observer_read_cb(uv_link_observer_t* observer,
                                 ssize_t nread,
                                 const uv_buf_t* buf) {
  read_called++;

  if (nread > 0) {
    CHECK_EQ(nread, 1, "nread == 1");
    CHECK_EQ(buf->base[0], 'a', "one char body");
  }

  if (nread == UV_EOF) {
    CHECK_EQ(uv_link_read_stop((uv_link_t*) server.http), 0,
             "uv_read_stop(server)");

    uv_link_close((uv_link_t*) &req_observer, req_close_cb);
  }
}


static void req_no_headers_server(uv_http_t* http,
                                  const char* url,
                                  size_t url_len) {
  CHECK_EQ(strncmp("/some/path", url, url_len), 0, "URL should match");
  CHECK_EQ(uv_http_accept(http, &req), 0, "uv_http_accept()");

  CHECK_EQ(uv_link_read_start((uv_link_t*) &req), 0, "uv_read_start(req)");
  CHECK_EQ(uv_link_observer_init(&req_observer), 0,
           "uv_link_observer_init(req_observer)");
  CHECK_EQ(uv_link_chain((uv_link_t*) &req, (uv_link_t*) &req_observer), 0,
           "uv_link_chain(req, req_observer)");

  req_observer.observer_read_cb = req_observer_read_cb;
}


TEST_IMPL(req_no_headers) {
  http_client_server_test(req_no_headers_client, req_no_headers_server);
  CHECK_EQ(read_called, 2, "read_called == 2");
  CHECK_EQ(req_closed, 1, "req_closed == 1");
}
