{
  "variables": {
    "gypkg_deps": [
      "git://github.com/libuv/libuv.git@^1.9.0 => uv.gyp:libuv",
      "git://github.com/nodejs/http-parser.git@^2.7.0 [gpg] => http_parser.gyp:http_parser",
      "git://github.com/indutny/uv_link_t@^1.0.0 [gpg] => uv_link_t.gyp:uv_link_t",
    ],
  },

  "targets": [{
    "target_name": "uv_http_t",
    "type": "<!(gypkg type)",

    "direct_dependent_settings": {
      "include_dirs": [ "include" ],
    },

    "dependencies": [
      "<!@(gypkg deps <(gypkg_deps))",
    ],

    "include_dirs": [
      ".",
    ],

    "sources": [
      "src/uv_http_t.c",
      "src/uv_http_req_t.c",
      "src/methods.c",
      "src/req_methods.c",
      "src/utils.c",
    ],
  }, {
    "target_name": "uv_http_t-test",
    "type": "executable",

    "include_dirs": [
      "src"
    ],

    "dependencies": [
      "<!@(gypkg deps <(gypkg_deps))",
      "uv_http_t",
    ],

    "sources": [
      "test/src/main.c",
      "test/src/test-req-no-headers.c",
      "test/src/test-req-with-headers.c",
      "test/src/test-req-respond.c",
      "test/src/test-req-write.c",
    ],
  }, {
    "target_name": "uv_http_t-example",
    "type": "executable",

    "include_dirs": [
      "src"
    ],

    "dependencies": [
      "<!@(gypkg deps <(gypkg_deps))",
      "uv_http_t",
    ],

    "sources": [
      "example/src/main.c",
    ],
  }],
}
