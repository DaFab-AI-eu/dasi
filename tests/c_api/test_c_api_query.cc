#include "eckit/testing/Test.h"

#include "dasi/api/dasi_c.h"

#include <memory>
#include <cstring>

#define CHECK_RETURN(x) EXPECT((x) == DASI_SUCCESS);

namespace std {
template <> struct default_delete<dasi_query_t> {
    void operator() (const dasi_query_t* q) { CHECK_RETURN(dasi_free_query(q)); }
};
}

using namespace std::string_literals;

//----------------------------------------------------------------------------------------------------------------------

CASE("Construct from empty request") {

    dasi_query_t* r;
    CHECK_RETURN(dasi_new_query(&r));
    EXPECT(r);
    std::unique_ptr<dasi_query_t> rdeleter(r);

    dasi_bool_t has;
    CHECK_RETURN(dasi_query_has(r, "key1", &has));
    EXPECT(!has);
    CHECK_RETURN(dasi_query_has(r, "key2", &has));
    EXPECT(!has);

    const char* vals1[] = {"value1a", "value1b"};
    CHECK_RETURN(dasi_query_set(r, "key1", vals1, 2));
    CHECK_RETURN(dasi_query_has(r, "key1", &has));
    EXPECT(has);
    CHECK_RETURN(dasi_query_has(r, "key2", &has));
    EXPECT(!has);
    CHECK_RETURN(dasi_query_has(r, "key3", &has));
    EXPECT(!has);
    CHECK_RETURN(dasi_query_has(r, "key4", &has));
    EXPECT(!has);

    const char* vals2[] = {"value2"};
    CHECK_RETURN(dasi_query_set(r, "key2", vals2, 1));
    CHECK_RETURN(dasi_query_has(r, "key1", &has));
    EXPECT(has);
    CHECK_RETURN(dasi_query_has(r, "key2", &has));
    EXPECT(has);
    CHECK_RETURN(dasi_query_has(r, "key3", &has));
    EXPECT(!has);
    CHECK_RETURN(dasi_query_has(r, "key4", &has));
    EXPECT(!has);

    const char* vals3[] = {""};
    CHECK_RETURN(dasi_query_set(r, "key3", vals3, 0));
    CHECK_RETURN(dasi_query_has(r, "key1", &has));
    EXPECT(has);
    CHECK_RETURN(dasi_query_has(r, "key2", &has));
    EXPECT(has);
    CHECK_RETURN(dasi_query_has(r, "key3", &has));
    EXPECT(has);
    CHECK_RETURN(dasi_query_has(r, "key4", &has));
    EXPECT(!has);

    CHECK_RETURN(dasi_query_append(r, "key4", "value4"));
    CHECK_RETURN(dasi_query_has(r, "key1", &has));
    EXPECT(has);
    CHECK_RETURN(dasi_query_has(r, "key2", &has));
    EXPECT(has);
    CHECK_RETURN(dasi_query_has(r, "key3", &has));
    EXPECT(has);
    CHECK_RETURN(dasi_query_has(r, "key4", &has));
    EXPECT(has);

    CHECK_RETURN(dasi_query_append(r, "key4", "value4b"));
    CHECK_RETURN(dasi_query_has(r, "key1", &has));
    EXPECT(has);
    CHECK_RETURN(dasi_query_has(r, "key2", &has));
    EXPECT(has);
    CHECK_RETURN(dasi_query_has(r, "key3", &has));
    EXPECT(has);
    CHECK_RETURN(dasi_query_has(r, "key4", &has));
    EXPECT(has);

    long count;
    CHECK_RETURN(dasi_query_keyword_count(r, &count));
    EXPECT(count == 4);

    CHECK_RETURN(dasi_query_value_count(r, "key1", &count));
    EXPECT(count == 2);
    CHECK_RETURN(dasi_query_value_count(r, "key2", &count));
    EXPECT(count == 1);
    CHECK_RETURN(dasi_query_value_count(r, "key3", &count));
    EXPECT(count == 0);
    CHECK_RETURN(dasi_query_value_count(r, "key4", &count));
    EXPECT(count == 2);

    const char* val;
    CHECK_RETURN(dasi_query_get(r, "key1", 0, &val));
    EXPECT(::strcmp(val, "value1a") == 0);
    CHECK_RETURN(dasi_query_get(r, "key1", 1, &val));
    EXPECT(::strcmp(val, "value1b") == 0);
    CHECK_RETURN(dasi_query_get(r, "key2", 0, &val));
    EXPECT(::strcmp(val, "value2") == 0);
    CHECK_RETURN(dasi_query_get(r, "key4", 0, &val));
    EXPECT(::strcmp(val, "value4") == 0);
    CHECK_RETURN(dasi_query_get(r, "key4", 1, &val));
    EXPECT(::strcmp(val, "value4b") == 0);

    // Check that out-of-range accesses give appropriate errors

    int rc = dasi_query_get(r, "key1", -1, &val);
    EXPECT(rc == DASI_ERROR_ASSERT);
    rc = dasi_query_get(r, "key1", 2, &val);
    EXPECT(rc == DASI_ERROR_ASSERT);
    rc = dasi_query_get(r, "key3", 0, &val);
    EXPECT(rc == DASI_ERROR_ASSERT);
}

CASE("Construct from string") {

    dasi_query_t* r;
    CHECK_RETURN(dasi_new_query_from_string(&r, "key1=value1,key3=value3a/value3b,key2=value2"));
    EXPECT(r);
    std::unique_ptr<dasi_query_t> rdeleter(r);

    long count;
    CHECK_RETURN(dasi_query_keyword_count(r, &count));
    EXPECT(count == 3);

    CHECK_RETURN(dasi_query_value_count(r, "key1", &count));
    EXPECT(count == 1);
    CHECK_RETURN(dasi_query_value_count(r, "key2", &count));
    EXPECT(count == 1);
    CHECK_RETURN(dasi_query_value_count(r, "key3", &count));
    EXPECT(count == 2);

    const char* val;
    CHECK_RETURN(dasi_query_get(r, "key1", 0, &val));
    EXPECT(::strcmp(val, "value1") == 0);
    CHECK_RETURN(dasi_query_get(r, "key2", 0, &val));
    EXPECT(::strcmp(val, "value2") == 0);
    CHECK_RETURN(dasi_query_get(r, "key3", 0, &val));
    EXPECT(::strcmp(val, "value3a") == 0);
    CHECK_RETURN(dasi_query_get(r, "key3", 1, &val));
    EXPECT(::strcmp(val, "value3b") == 0);
}

CASE("Modify existing query") {

    dasi_query_t* r;
    CHECK_RETURN(dasi_new_query_from_string(&r, "key1=value1,key3=value3,key2=value2"));
    EXPECT(r);
    std::unique_ptr<dasi_query_t> rdeleter(r);

    long count;
    CHECK_RETURN(dasi_query_keyword_count(r, &count));
    EXPECT(count == 3);

    CHECK_RETURN(dasi_query_value_count(r, "key1", &count));
    EXPECT(count == 1);
    CHECK_RETURN(dasi_query_value_count(r, "key2", &count));
    EXPECT(count == 1);
    CHECK_RETURN(dasi_query_value_count(r, "key3", &count));
    EXPECT(count == 1);

    const char* vals1[] = {""};
    const char* vals3[] = {"VALUE3a", "VALUE3b"};
    CHECK_RETURN(dasi_query_set(r, "key1", vals1, 0));
    CHECK_RETURN(dasi_query_set(r, "key3", vals3, 2));

    CHECK_RETURN(dasi_query_keyword_count(r, &count));
    EXPECT(count == 3);

    CHECK_RETURN(dasi_query_value_count(r, "key1", &count));
    EXPECT(count == 0);
    CHECK_RETURN(dasi_query_value_count(r, "key2", &count));
    EXPECT(count == 1);
    CHECK_RETURN(dasi_query_value_count(r, "key3", &count));
    EXPECT(count == 2);

    const char* val;
    CHECK_RETURN(dasi_query_get(r, "key2", 0, &val));
    EXPECT(::strcmp(val, "value2") == 0);
    CHECK_RETURN(dasi_query_get(r, "key3", 0, &val));
    EXPECT(::strcmp(val, "VALUE3a") == 0);
    CHECK_RETURN(dasi_query_get(r, "key3", 1, &val));
    EXPECT(::strcmp(val, "VALUE3b") == 0);
}

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char** argv) {
    return eckit::testing::run_tests(argc, argv);
}
