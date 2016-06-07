test: dist
	./out/Release/uv_http_t-test

example: dist
	./out/Release/uv_http_t-example

dist:
	gypkg gen uv_http_t.gyp
	make -C out/ -j8

.PHONY: test example dist
