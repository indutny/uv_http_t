{
  "targets": [{
    "target_name": "uv_http_t-example",
    "type": "executable",

    "include_dirs": [
      "src"
    ],

    "dependencies": [
      "../test/deps/libuv/uv.gyp:libuv",
      "../test/deps/uv_link_t/uv_link_t.gyp:uv_link_t",
      "../uv_http_t.gyp:uv_http_t",
    ],

    "sources": [
      "src/main.c",
    ],
  }],
}
