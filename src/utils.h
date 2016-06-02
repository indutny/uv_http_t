#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include "include/uv_http_t.h"
#include "http_parser.h"

typedef struct uv_http_data_s uv_http_data_t;

struct uv_http_data_s {
  char* value;
  size_t size;
  size_t limit;

  /* Private */
  char* storage;
  size_t storage_limit;
};

void uv_http_data_init(uv_http_data_t* data, char* storage, size_t limit);
void uv_http_data_free(uv_http_data_t* data);

int uv_http_data_queue(uv_http_data_t* data, const char* bytes, size_t size);
void uv_http_data_dequeue(uv_http_data_t* data, size_t size);

uv_http_method_t uv_http_convert_method(enum http_method method);

#endif  /* SRC_UTILS_H_ */
