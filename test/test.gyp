{
  "targets": [{
    "target_name": "uv_http_t-test",
    "type": "executable",

    "include_dirs": [
      "src"
    ],

    "dependencies": [
      "deps/libuv/uv.gyp:libuv",
      "deps/uv_link_t/uv_link_t.gyp:uv_link_t",
      "../uv_http_t.gyp:uv_http_t",
    ],

    "sources": [
      "src/main.c",
      "src/test-req-no-headers.c",
      "src/test-req-with-headers.c",
      "src/test-req-respond.c",
      "src/test-req-write.c",
    ],
  }],
}
