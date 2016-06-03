#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uv.h"
#include "uv_link_t.h"
#include "uv_http_t.h"

#define CHECK(V) if ((V) != 0) abort()
#define CHECK_ALLOC(V) if ((V) == NULL) abort()

typedef struct client_s client_t;
typedef struct req_s req_t;

static uv_tcp_t server;

struct client_s {
  uv_tcp_t tcp;
  uv_link_source_t source;
  uv_http_t* http;
};

struct req_s {
  uv_http_req_t req;
  uv_link_observer_t observer;
};

static void close_cb(uv_link_t* link) {
  client_t* client;

  client = link->data;
  free(client);
}

static void read_cb(uv_link_observer_t* observer,
                    ssize_t nread,
                    const uv_buf_t* buf) {
  client_t* client;

  client = observer->data;

  if (nread < 0) {
    fprintf(stderr, "error or close\n");
    uv_link_close((uv_link_t*) observer, close_cb);
    return;
  }

  fprintf(stderr, "read \"%.*s\"\n", (int) nread, buf->base);
}


static void req_close_cb(uv_link_t* link) {
  free(link->data);
  fprintf(stderr, "closed\n");
}


static void req_shutdown_cb(uv_link_t* link, int status, void* arg) {
  fprintf(stderr, "shutdown\n");
  uv_link_close(link, close_cb);
}


static void req_write_cb(uv_link_t* link, int status, void* arg) {
  /* no-op */
  fprintf(stderr, "sent response\n");
  CHECK(uv_link_shutdown(link, req_shutdown_cb, NULL));
}


static void req_active_cb(uv_http_req_t* req, int status) {
  req_t* r;
  uv_buf_t buf;

  r = req->data;

  buf = uv_buf_init("OK", 2);
  CHECK(uv_http_req_respond(&r->req, 200, &buf, NULL, NULL, 0));

  buf = uv_buf_init("hello world", 11);
  CHECK(uv_link_write((uv_link_t*) &r->observer, &buf, 1, NULL, req_write_cb,
                      NULL));
}


static void request_cb(uv_http_t* http, const char* url, size_t url_size) {
  req_t* r;

  fprintf(stderr, "Got request: %.*s\n", (int) url_size, url);
  CHECK_ALLOC(r = malloc(sizeof(*r)));
  CHECK(uv_http_accept(http, &r->req));
  CHECK(uv_link_observer_init(&r->observer));
  CHECK(uv_link_chain((uv_link_t*) &r->req, (uv_link_t*) &r->observer));
  CHECK(uv_link_read_start((uv_link_t*) &r->observer));

  r->req.data = r;
  uv_http_req_on_active(&r->req, req_active_cb);
}


static void connection_cb(uv_stream_t* s, int status) {
  int err;
  client_t* client;

  CHECK_ALLOC(client = malloc(sizeof(*client)));

  CHECK(uv_tcp_init(uv_default_loop(), &client->tcp));
  CHECK(uv_accept(s, (uv_stream_t*) &client->tcp));
  CHECK(uv_link_source_init(&client->source, (uv_stream_t*) &client->tcp));

  CHECK_ALLOC(client->http = uv_http_create(request_cb, &err));
  CHECK(err);
  CHECK(uv_link_chain((uv_link_t*) &client->source, (uv_link_t*) client->http));

  CHECK(uv_link_read_start((uv_link_t*) client->http));
}


int main() {
  static const int kBacklog = 128;

  uv_loop_t* loop;
  struct sockaddr_in addr;

  loop = uv_default_loop();

  CHECK(uv_tcp_init(loop, &server));
  CHECK(uv_ip4_addr("0.0.0.0", 9000, &addr));
  CHECK(uv_tcp_bind(&server, (struct sockaddr*) &addr, 0));

  fprintf(stderr, "Listening on 0.0.0.0:9000\n");

  CHECK(uv_listen((uv_stream_t*) &server, kBacklog, connection_cb));
  CHECK(uv_run(loop, UV_RUN_DEFAULT));

  return 0;
}
