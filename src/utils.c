#include <stdlib.h>
#include <string.h>

#include "include/uv_http_t.h"
#include "http_parser.h"

#include "src/utils.h"
#include "src/common.h"

static int uv_http_data_grow(uv_http_data_t* data, size_t new_size);

static const unsigned int kUVHTTPDataStorageChunk = 1024;

void uv_http_data_init(uv_http_data_t* data, char* storage, size_t limit) {
  data->value = storage;
  data->size = 0;
  data->limit = limit;

  data->storage = storage;
  data->storage_limit = limit;
}


void uv_http_data_free(uv_http_data_t* data) {
  if (data->value != data->storage)
    free(data->value);
  data->value = NULL;
}


int uv_http_data_queue(uv_http_data_t* data, const char* bytes, size_t size) {
  if (data->size + size >= data->limit) {
    int err;

    err = uv_http_data_grow(data, data->size + size);
    if (err != 0)
      return err;
  }

  memcpy(data->value + data->size, bytes, size);
  data->size += size;

  return 0;
}


void uv_http_data_dequeue(uv_http_data_t* data, size_t size) {
  /* The most usual case */
  if (data->size == size) {
    uv_http_data_free(data);
    uv_http_data_init(data, data->storage, data->storage_limit);
    return;
  }

  memmove(data->value, data->value + size, data->size - size);
  data->size -= size;
}


int uv_http_data_grow(uv_http_data_t* data, size_t new_size) {
  char* tmp;

  /* Round up size */
  if ((new_size % kUVHTTPDataStorageChunk) != 0)
    new_size += kUVHTTPDataStorageChunk - (new_size % kUVHTTPDataStorageChunk);

  if (data->value != data->storage) {
    tmp = realloc(data->value, new_size);
    if (tmp == NULL)
      return UV_ENOMEM;
    data->value = tmp;
    data->limit = new_size;
    return 0;
  }

  tmp = malloc(new_size);
  if (tmp == NULL)
    return UV_ENOMEM;
  memcpy(tmp, data->value, data->size);
  data->value = tmp;
  data->limit = new_size;
  return 0;
}


uv_http_method_t uv_http_convert_method(enum http_method method) {
  switch (method) {
    case HTTP_DELETE: return UV_HTTP_DELETE;
    case HTTP_GET: return UV_HTTP_GET;
    case HTTP_HEAD: return UV_HTTP_HEAD;
    case HTTP_POST: return UV_HTTP_POST;
    case HTTP_PUT: return UV_HTTP_PUT;
    case HTTP_CONNECT: return UV_HTTP_CONNECT;
    case HTTP_OPTIONS: return UV_HTTP_OPTIONS;
    case HTTP_TRACE: return UV_HTTP_TRACE;
    case HTTP_COPY: return UV_HTTP_COPY;
    case HTTP_LOCK: return UV_HTTP_LOCK;
    case HTTP_MKCOL: return UV_HTTP_MKCOL;
    case HTTP_MOVE: return UV_HTTP_MOVE;
    case HTTP_PROPFIND: return UV_HTTP_PROPFIND;
    case HTTP_PROPPATCH: return UV_HTTP_PROPPATCH;
    case HTTP_SEARCH: return UV_HTTP_SEARCH;
    case HTTP_UNLOCK: return UV_HTTP_UNLOCK;
    case HTTP_BIND: return UV_HTTP_BIND;
    case HTTP_REBIND: return UV_HTTP_REBIND;
    case HTTP_UNBIND: return UV_HTTP_UNBIND;
    case HTTP_ACL: return UV_HTTP_ACL;
    case HTTP_REPORT: return UV_HTTP_REPORT;
    case HTTP_MKACTIVITY: return UV_HTTP_MKACTIVITY;
    case HTTP_CHECKOUT: return UV_HTTP_CHECKOUT;
    case HTTP_MERGE: return UV_HTTP_MERGE;
    case HTTP_MSEARCH: return UV_HTTP_MSEARCH;
    case HTTP_NOTIFY: return UV_HTTP_NOTIFY;
    case HTTP_SUBSCRIBE: return UV_HTTP_SUBSCRIBE;
    case HTTP_UNSUBSCRIBE: return UV_HTTP_UNSUBSCRIBE;
    case HTTP_PATCH: return UV_HTTP_PATCH;
    case HTTP_PURGE: return UV_HTTP_PURGE;
    case HTTP_MKCALENDAR: return UV_HTTP_MKCALENDAR;
    case HTTP_LINK: return UV_HTTP_LINK;
    case HTTP_UNLINK: return UV_HTTP_UNLINK;
    default: CHECK(0, "Unexpected method");
  }
}
