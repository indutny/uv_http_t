#include "test-common.h"

static int req_closed;

static uv_http_req_t req;

static void req_respond_client(int fd) {
  char buf[1024];
  int len;

  client_send_str("GET /path HTTP/1.1\r\n\r\n");
  len = client_receive(buf, sizeof(buf));

  /* TODO(indutny): Could be partial read, do something about it */
  CHECK_EQ(strncmp(buf, "HTTP/1.1 200 OK\r\nABC: DEF\r\nX-Something: some-value\r\n\r\n", len), 0, "expected data");
}


static void req_close_cb(uv_link_t* req) {
  req_closed++;
}


static int req_on_complete(uv_http_req_t* req) {
  uv_link_close((uv_link_t*) req, req_close_cb);
  return 0;
}


static void req_respond_server(uv_http_t* http,
                               const char* url,
                               size_t url_len) {
  uv_buf_t fields[2];
  uv_buf_t values[2];
  uv_buf_t status;

  CHECK_EQ(uv_http_accept(http, &req), 0, "uv_http_accept()");

  fields[0] = uv_buf_init("ABC", 3);
  values[0] = uv_buf_init("DEF", 3);
  fields[1] = uv_buf_init("X-Something", 11);
  values[1] = uv_buf_init("some-value", 10);

  status = uv_buf_init("OK", 2);
  CHECK_EQ(uv_http_req_respond(&req, 200, &status, fields, values, 2),
           0,
           "uv_http_req_respond");

  CHECK_EQ(uv_link_read_stop((uv_link_t*) server.http), 0,
           "uv_read_stop(server)");

  req.on_headers_complete = req_on_complete;
}


TEST_IMPL(req_respond) {
  http_client_server_test(req_respond_client, req_respond_server);
  CHECK_EQ(req_closed, 1, "req_closed == 1");
}
