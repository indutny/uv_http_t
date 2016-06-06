{
  "targets": [{
    "target_name": "uv_http_t",
    "type": "<!(gypkg type)",

    "direct_dependent_settings": {
      "include_dirs": [ "include" ],
    },

    "variables": {
      "gypkg_deps": [
        "git://github.com/libuv/libuv.git#v1.9.1:uv.gyp:libuv",
        "git://github.com/indutny/uv_link_t:uv_link_t.gyp:uv_link_t",
      ],
    },

    "dependencies": [
      "<!@(gypkg deps <(gypkg_deps))",
      "deps/http-parser/http_parser.gyp:http_parser",
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
  }],
}
