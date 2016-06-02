#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

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

#endif  /* SRC_UTILS_H_ */
