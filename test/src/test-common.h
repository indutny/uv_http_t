#ifndef TEST_SRC_TEST_COMMON_H_
#define TEST_SRC_TEST_COMMON_H_

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "uv.h"
#include "uv_http_t.h"

#include "test-list.h"

static int fds[2];
static uv_loop_t* loop;
static int close_cb_called;

static struct {
  uv_pipe_t pipe;
  uv_link_source_t source;
  uv_link_observer_t observer;
  uv_http_t* http;
} server;


static struct {
  uv_thread_t thread;
} client;


#define CHECK(VALUE, MESSAGE)                                                \
    do {                                                                     \
      if ((VALUE)) break;                                                    \
      fprintf(stderr, "Assertion failure: " #MESSAGE "\n");                  \
      abort();                                                               \
    } while (0)

#define CHECK_EQ(A, B, MESSAGE) CHECK((A) == (B), MESSAGE)
#define CHECK_NE(A, B, MESSAGE) CHECK((A) != (B), MESSAGE)

static void client_thread_body(void* arg) {
  void (*fn)(int fd);

  fn = arg;

  fn(fds[0]);
}


static void client_send_str(const char* data) {
  int len;

  len = strlen(data);

  while (len > 0) {
    int err;

    do
      err = write(fds[0], data, len);
    while (err == -1 && errno == EINTR);

    CHECK_NE(err, -1, "client write failed");
    CHECK(err <= len, "client write overflow");

    data += err;
    len -= err;
  }
}


static void close_cb(uv_link_t* link) {
  close_cb_called++;
}


static void http_client_server_test(void (*client_fn)(int fd),
                                    uv_http_req_handler_cb handler) {
  int err;

  CHECK_NE(loop = uv_default_loop(), NULL, "uv_default_loop()");
  CHECK_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0, "socketpair()");

  /* Initialize source */

  CHECK_EQ(uv_pipe_init(loop, &server.pipe, 0), 0, "uv_pipe_init(server.pipe)");
  CHECK_EQ(uv_pipe_open(&server.pipe, fds[1]), 0, "uv_pipe_open(server.pipe)");

  CHECK_EQ(uv_link_source_init(&server.source, (uv_stream_t*) &server.pipe), 0,
           "uv_link_source_init(server.source)");

  /* Server part of the pair is uv_http_t */
  CHECK_NE(server.http = uv_http_create(handler, &err), NULL,
           "uv_http_create(server.http)");

  /* Create observer */
  CHECK_EQ(uv_link_observer_init(&server.observer), 0,
           "uv_link_observer_init(server.observer)");

  /* Chain server's links */
  CHECK_EQ(uv_link_chain((uv_link_t*) &server.source,
                         (uv_link_t*) server.http),
           0,
           "uv_link_chain(server.source)");
  CHECK_EQ(uv_link_chain((uv_link_t*) server.http,
                         (uv_link_t*) &server.observer),
           0,
           "uv_link_chain(server.http)");

  /* Start client thread */

  CHECK_EQ(uv_thread_create(&client.thread, client_thread_body, client_fn), 0,
           "uv_thread_create(client.thread)");

  /* Pre-start server */
  CHECK_EQ(uv_link_read_start((uv_link_t*) &server.observer), 0,
           "uv_link_read_start()");

  CHECK_EQ(uv_run(loop, UV_RUN_DEFAULT), 0, "uv_run() post");

  uv_thread_join(&client.thread);

  /* Free resources */

  uv_link_close((uv_link_t*) &server.observer, close_cb);

  CHECK_EQ(uv_run(loop, UV_RUN_DEFAULT), 0, "uv_run() post");
  CHECK_EQ(close_cb_called, 1, "close_cb must be called");

  memset(&server, 0, sizeof(server));
  memset(&client, 0, sizeof(client));

  CHECK_EQ(close(fds[0]), 0, "close(fds[0])");
  CHECK_NE(close(fds[1]), 0, "close(fds[1]) must fail");
}

#endif  /* TEST_SRC_TEST_COMMON_H_ */
