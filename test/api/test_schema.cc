
#include "dasi/core/Test.h"

#include "dasi/api/Schema.h"
#include "dasi/api/Key.h"
#include "dasi/api/Request.h"

#include <sstream>
#include <string>

using dasi::Schema;
using dasi::Key;
using dasi::Request;
using namespace std::string_literals;

constexpr const char* TEST_SCHEMA = R"(
[key1, key2, key3,
  [key1a, key2a, key3,
     [key1b, key2b, key3b],
     [key1B, key2B, key3B]],

  [key1A, key2A, key3A,
     [key1b, key2b, key3b]],
  [Key1A, Key2A, Key3A,
     [key1b, key2b, key3b]]
]
[Key1, Key2, Key3,
  [key1a, key2a, key3,
     [key1b, key2b, key3b],
     [key1B, key2B, key3B]],

  [key1A, key2A, key3A,
     [key1b, key2b, key3b]],
  [Key1A, Key2A, Key3A,
     [key1b, key2b, key3b]]
])";


CASE("Schema outputting") {

    Schema s {
        {{"level1a", "level1b"},
         {{{"level2a", "level2b"},
           {{"level3a", "level3b"},
            {"Level3a", "Level3b"},
           }},
          {{"Level2a", "Level2b"},
           {{"level3a", "level3b"},
            {"Level3a", "Level3b"},
           }},
         }
        },
        {{"Level1a", "Level1b"},
         {{{"level2a", "level2b"},
           {{"level3a", "level3b"},
            {"Level3a", "Level3b"},
           }},
          {{"Level2a", "Level2b"},
           {{"level3a", "level3b"},
            {"Level3a", "Level3b"},
           }},
         }
        },
    };

    std::ostringstream ss;
    ss << s;

    EXPECT(ss.str() == R"(Schema[
 - [level1a, level1b
     [level2a, level2b
       [level3a, level3b]
       [Level3a, Level3b]
     ]
     [Level2a, Level2b
       [level3a, level3b]
       [Level3a, Level3b]
     ]
   ]
 - [Level1a, Level1b
     [level2a, level2b
       [level3a, level3b]
       [Level3a, Level3b]
     ]
     [Level2a, Level2b
       [level3a, level3b]
       [Level3a, Level3b]
     ]
   ]
])");
}

CASE("Schema parsing") {

    auto s = Schema::parse(TEST_SCHEMA);

    std::ostringstream ss;
    ss << s;

    EXPECT(ss.str() == R"(Schema[
 - [key1, key2, key3
     [key1a, key2a, key3
       [key1b, key2b, key3b]
       [key1B, key2B, key3B]
     ]
     [key1A, key2A, key3A
       [key1b, key2b, key3b]
     ]
     [Key1A, Key2A, Key3A
       [key1b, key2b, key3b]
     ]
   ]
 - [Key1, Key2, Key3
     [key1a, key2a, key3
       [key1b, key2b, key3b]
       [key1B, key2B, key3B]
     ]
     [key1A, key2A, key3A
       [key1b, key2b, key3b]
     ]
     [Key1A, Key2A, Key3A
       [key1b, key2b, key3b]
     ]
   ]
])");
}

CASE("Visit schema with a key") {

    auto s = Schema::parse(TEST_SCHEMA);
    Key k {
            {"key1", "k1"},
            {"key2", "k2"},
            {"key3", "k3"},
            {"key1a", "k11"},
            {"key2a", "k22"},
            {"key3", "k33"},
            {"key1B", "k111"},
            {"key2B", "k222"},
            {"key3B", "k333"},
    };

    struct MyVisitor {};

    std::cout << "Walking..." << std::endl;
    MyVisitor v;
    s.walk(k, v);
}

CASE("Visit schema with a request") {

    auto s = Schema::parse(TEST_SCHEMA);
    Request r {
            {"key1", {"k1"}},
            {"key2", {"k2"}},
            {"key3", {"k3"}},
            {"key1a", {"k11"}},
            {"key2a", {"k22"}},
            {"key3", {"k33"}},
            {"key1B", {"k111"}},
            {"key2B", {"k222"}},
            {"key3B", {"k333"}},
    };

    struct MyVisitor {};

    std::cout << "Walking..." << std::endl;
    MyVisitor v;
    s.walk(r, v);
}



int main(int argc, char** argv) {
    return ::dasi::run_tests();
}