{
  "targets": [{
    "target_name": "uv_http_t",
    "type": "<(library)",

    "direct_dependent_settings": {
      "include_dirs": [ "include" ],
    },
    "include_dirs": [
      # libuv
      "<(uv_dir)/include",

      # uv_link_t
      "<(uv_link_t_dir)/include",

      ".",
    ],

    "dependencies": [
      "deps/http-parser/http_parser.gyp:http_parser",
    ],

    "sources": [
      "src/uv_http_t.c",
      "src/uv_http_req_t.c",
      "src/methods.c",
      "src/req_methods.c",
    ],
  }],
}
