#include "test-common.h"

static uv_http_req_t req;

static void req_no_headers_client(int fd) {
  client_send_str("GET /some/path HTTP/1.1\r\n\r\n");
}


static void req_no_headers_server(uv_http_t* http,
                                  const char* url,
                                  size_t url_len) {
  CHECK_EQ(strncmp("/some/path", url, url_len), 0, "URL should match");
  CHECK_EQ(uv_http_accept(http, &req), 0, "uv_http_accept()");
}


TEST_IMPL(req_no_headers) {
  http_client_server_test(req_no_headers_client, req_no_headers_server);
}
