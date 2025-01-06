#include "eckit/testing/Test.h"

#include "dasi/api/Query.h"

#include <sstream>
#include <string>

using namespace std::string_literals;

//----------------------------------------------------------------------------------------------------------------------

CASE("Construct from empty request") {

    dasi::Query r;
    EXPECT(!r.has("key1"));
    EXPECT(!r.has("key2"));

    {
        std::ostringstream ss;
        ss << r;
        EXPECT(ss.str() == "{}");
    }

    r.set("key1", {"value1a", "value1b"});
    EXPECT(r.has("key1"));
    EXPECT(!r.has("key2"));
    EXPECT(!r.has("key3"));
    EXPECT(!r.has("key4"));

    r.set("key2", {"value2"});
    EXPECT(r.has("key1"));
    EXPECT(r.has("key2"));
    EXPECT(!r.has("key3"));
    EXPECT(!r.has("key4"));

    r.set("key3", {});
    EXPECT(r.has("key1"));
    EXPECT(r.has("key2"));
    EXPECT(r.has("key3"));
    EXPECT(!r.has("key4"));

    r.append("key4", "value4");
    EXPECT(r.has("key1"));
    EXPECT(r.has("key2"));
    EXPECT(r.has("key3"));
    EXPECT(r.has("key4"));

    r.append("key4", "value4b");
    EXPECT(r.has("key1"));
    EXPECT(r.has("key2"));
    EXPECT(r.has("key3"));
    EXPECT(r.has("key4"));

    {
        std::ostringstream ss;
        ss << r;
        EXPECT(ss.str() == "{key1=[value1a,value1b],key2=[value2],key3=[],key4=[value4,value4b]}");
    }
}

CASE("Construct from initialiser list") {

    dasi::Query r {
        {"key1", {}},
        {"key2", {"value2"}},
        {"key3", {"value3a", "value3b"}},
    };

    EXPECT(r.has("key1"));
    EXPECT(r.has("key2"));
    EXPECT(r.has("key3"));
    EXPECT(!r.has("other"));

    bool k1 = false;
    bool k2 = false;
    bool k3 = false;
    int count = 0;
    for (const auto& kv : r) {
        ++count;
        if (kv.first == "key1") {
            EXPECT(kv.second.size() == 0);
            k1 = true;
        } else if (kv.first == "key2") {
            EXPECT(kv.second.size() == 1);
            EXPECT(kv.second[0] == "value2");
            k2 = true;
        } else if (kv.first == "key3") {
            EXPECT(kv.second.size() == 2);
            EXPECT(kv.second[0] == "value3a");
            EXPECT(kv.second[1] == "value3b");
            k3 = true;
        }
    }
    EXPECT(count == 3);
    EXPECT(k1 && k2 && k3);

    std::ostringstream ss;
    ss << r;
    EXPECT(ss.str() == "{key1=[],key2=[value2],key3=[value3a,value3b]}");
}

CASE("Construct from string") {

    dasi::Query r("key1=value1,key3=value3a/value3b,key2=value2");

    EXPECT(r.has("key1"));
    EXPECT(r.has("key2"));
    EXPECT(r.has("key3"));
    EXPECT(!r.has("other"));

    bool k1 = false;
    bool k2 = false;
    bool k3 = false;
    int count = 0;
    for (const auto& kv : r) {
        ++count;
        if (kv.first == "key1") {
            EXPECT(kv.second.size() == 1);
            EXPECT(kv.second[0] == "value1");
            k1 = true;
        } else if (kv.first == "key2") {
            EXPECT(kv.second.size() == 1);
            EXPECT(kv.second[0] == "value2");
            k2 = true;
        } else if (kv.first == "key3") {
            EXPECT(kv.second.size() == 2);
            EXPECT(kv.second[0] == "value3a");
            EXPECT(kv.second[1] == "value3b");
            k3 = true;
        }
    }
    EXPECT(count == 3);
    EXPECT(k1 && k2 && k3);

    std::ostringstream ss;
    ss << r;
    EXPECT(ss.str() == "{key1=[value1],key2=[value2],key3=[value3a,value3b]}");
}

CASE("Modify existing query") {

    dasi::Query r {
        {"key1", {"value1"}},
        {"key2", {"value2"}},
        {"key3", {"value3"}},
    };

    {
        std::ostringstream ss;
        ss << r;
        EXPECT(ss.str() == "{key1=[value1],key2=[value2],key3=[value3]}");
    }

    r.set("key1", {});
    r.set("key3", {"VALUE3a", "VALUE3b"});

    {
        std::ostringstream ss;
        ss << r;
        EXPECT(ss.str() == "{key1=[],key2=[value2],key3=[VALUE3a,VALUE3b]}");
    }
}

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char** argv) {
    return eckit::testing::run_tests(argc, argv);
}
