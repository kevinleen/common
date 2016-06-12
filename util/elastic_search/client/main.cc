#include "util/elastic_search/client/es_client.h"

DEF_uint32(
    test_mode, 0,
    "\t1: test for string\n\t2: test for json\n\t0:both json and string");
DEF_string(es_types, "", "types for elastic search");

static std::string mode2Str(uint8 mode) {
  if (mode == 1) return "for string";
  if (mode == 2) return "for json";
  return "both string and json";
}

int main(int argc, char** argv) {
  ccflag::init_ccflag(argc, argv);
  cclog::init_cclog(*argv);

  test::ESClient es(FLG_es_types);
  CHECK(es.init());

  WLOG<< "test mode: " << mode2Str(FLG_test_mode);
  switch (FLG_test_mode) {
    case 1:
      es.testString();
      break;
    case 2:
      es.testJson();
      break;
    default:
      es.testString();
      es.testJson();
      break;
  }

  while (1) {
    sys::sleep(1);
  }

  return 0;
}
