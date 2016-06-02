#include "test-common.h"

static int req_closed;
static int field_called;
static int value_called;
static int headers_complete;

static uv_http_req_t req;

static void req_with_headers_client(int fd) {
  client_send_str("POST /so");
  /* Let it through */
  usleep(100000);
  client_send_str("me/path HTTP/1.1\r\nContent");
  usleep(100000);
  client_send_str("-Length: 1\r\nX-Val: val");
  usleep(100000);
  client_send_str("ue\r\n\r\na");
}


static void req_close_cb(uv_link_t* req) {
  req_closed++;
}


static int req_on_field(uv_http_req_t* req, const char* value, size_t size) {
  CHECK_EQ(headers_complete, 0, "field after headers_complete");
  switch (field_called++) {
    case 0:
      CHECK_EQ(strncmp(value, "Content-Length", size), 0, "field 0");
      break;
    case 1:
      CHECK_EQ(strncmp(value, "X-Val", size), 0, "field 1");
      break;
    default:
      CHECK(0, "Unexpected field");
      break;
  }
  return 0;
}


static int req_on_value(uv_http_req_t* req, const char* value, size_t size) {
  switch (value_called++) {
    case 0:
      CHECK_EQ(strncmp(value, "1", size), 0, "field 0");
      break;
    case 1:
      CHECK_EQ(strncmp(value, "value", size), 0, "field 1");
      break;
    default:
      CHECK(0, "Unexpected field");
      break;
  }
  return 0;
}


static int req_on_complete(uv_http_req_t* req) {
  headers_complete++;

  CHECK_EQ(uv_link_read_stop((uv_link_t*) server.http), 0,
           "uv_read_stop(server)");

  uv_link_close((uv_link_t*) req, req_close_cb);

  return 0;
}


static void req_with_headers_server(uv_http_t* http,
                                    const char* url,
                                    size_t url_len) {
  CHECK_EQ(strncmp("/some/path", url, url_len), 0, "URL should match");
  CHECK_EQ(uv_http_accept(http, &req), 0, "uv_http_accept()");

  CHECK_EQ(uv_link_read_start((uv_link_t*) &req), 0, "uv_read_start(req)");

  req.on_header_field = req_on_field;
  req.on_header_value = req_on_value;
  req.on_headers_complete = req_on_complete;
}


TEST_IMPL(req_with_headers) {
  http_client_server_test(req_with_headers_client, req_with_headers_server);
  CHECK_EQ(headers_complete, 1, "headers_complete == 1");
  CHECK_EQ(field_called, 2, "field_called == 2");
  CHECK_EQ(value_called, 2, "value_called == 2");
  CHECK_EQ(req_closed, 1, "req_closed == 1");
}
