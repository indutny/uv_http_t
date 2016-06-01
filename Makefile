test:
	./gyp_uv_http test -Duv_dir=`pwd`/test/deps/libuv \
		-Duv_link_t_dir=`pwd`/test/deps/uv_link_t
	make -C out/ -j8
	./out/Release/uv_http_t-test

dist:
	./gyp_uv_http -Duv_dir=`pwd`/test/deps/libuv \
		-Duv_link_t_dir=`pwd`/test/deps/uv_link_t
	make -C out/ -j8

example:
	./gyp_uv_http example -Duv_dir=`pwd`/test/deps/libuv \
		-Duv_link_t_dir=`pwd`/test/deps/uv_link_t
	make -C out/ -j8
	./out/Release/uv_http_t-example

.PHONY: dist test example
