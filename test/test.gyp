{
  "targets": [{
    "target_name": "uv_http_t-test",
    "type": "executable",

    "include_dirs": [
      "src"
    ],

    "variables": {
      "gypkg_deps": [
        "git://github.com/libuv/libuv.git@^1.9.0:uv.gyp:libuv",
        "git://github.com/indutny/uv_link_t@^1.0.0:uv_link_t.gyp:uv_link_t",
      ],
    },

    "dependencies": [
      "<!@(gypkg deps <(gypkg_deps))",
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
