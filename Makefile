test: dist
	./out/Release/uv_http_t-test

example: dist
	./out/Release/uv_http_t-example

dist:
	gypkg build uv_http_t.gyp

.PHONY: test example dist
