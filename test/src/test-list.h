#ifndef TEST_SRC_TEST_LIST_H_
#define TEST_SRC_TEST_LIST_H_

#define TEST_ENUM(V)                                                          \
    V(req_no_headers)                                                         \
    V(req_with_headers)                                                       \
    V(req_respond)                                                            \
    V(req_write)                                                              \

#define TEST_DECL(N) void test__##N();

TEST_ENUM(TEST_DECL)

#undef TEST_DECL

#define TEST_FN(N) test__##N
#define TEST_IMPL(N) void test__##N()

#endif  /* TEST_SRC_TEST_LIST_H_ */
