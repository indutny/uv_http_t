#include <stdlib.h>
#include <string.h>

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
