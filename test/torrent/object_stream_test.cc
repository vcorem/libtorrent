#include "config.h"

#include <sstream>
#include <torrent/object.h>

#import "object_stream_test.h"
#import "object_test_utils.h"

CPPUNIT_TEST_SUITE_REGISTRATION(ObjectStreamTest);

static const char* ordered_bencode = "d1:ei0e4:ipv44:XXXX4:ipv616:XXXXXXXXXXXXXXXX1:md11:upload_onlyi3e12:ut_holepunchi4e11:ut_metadatai2e6:ut_pexi1ee13:metadata_sizei15408e1:pi16033e4:reqqi255e1:v15:uuTorrent 1.8.46:yourip4:XXXXe";
static const char* unordered_bencode = "d1:ei0e1:md11:upload_onlyi3e12:ut_holepunchi4e11:ut_metadatai2e6:ut_pexi1ee4:ipv44:XXXX4:ipv616:XXXXXXXXXXXXXXXX13:metadata_sizei15408e1:pi16033e4:reqqi255e1:v15:uuTorrent 1.8.46:yourip4:XXXXe";

static const char* string_bencode = "32:aaaaaaaabbbbbbbbccccccccdddddddd";

void
ObjectStreamTest::testInputOrdered() {
  torrent::Object orderedObj   = create_bencode(ordered_bencode);
  torrent::Object unorderedObj = create_bencode(unordered_bencode);

  CPPUNIT_ASSERT(!(orderedObj.flags() & torrent::Object::flag_unordered));
  CPPUNIT_ASSERT(unorderedObj.flags() & torrent::Object::flag_unordered);
}

void
ObjectStreamTest::testInputNullKey() {
  torrent::Object obj = create_bencode("d0:i1e5:filesi2ee");

  CPPUNIT_ASSERT(!(obj.flags() & torrent::Object::flag_unordered));
}

void
ObjectStreamTest::testOutputMask() {
  torrent::Object normalObj = create_bencode("d1:ai1e1:bi2e1:ci3ee");

  CPPUNIT_ASSERT(compare_bencode(normalObj, "d1:ai1e1:bi2e1:ci3ee"));

  normalObj.get_key("b").set_flags(torrent::Object::flag_session_data);
  normalObj.get_key("c").set_flags(torrent::Object::flag_static_data);

  CPPUNIT_ASSERT(compare_bencode(normalObj, "d1:ai1e1:ci3ee", torrent::Object::flag_session_data));
}

//
// Testing for bugs in bencode write.
//

// Dummy function that invalidates the buffer once called.

torrent::object_buffer_t
object_write_to_invalidate(void* data, torrent::object_buffer_t buffer) {
  return torrent::object_buffer_t(buffer.second, buffer.second);
}

void
ObjectStreamTest::testBuffer() {
  char raw_buffer[16];
  torrent::object_buffer_t buffer(raw_buffer, raw_buffer + 16);

  torrent::Object obj = create_bencode(string_bencode);

  object_write_bencode_c(&object_write_to_invalidate, NULL, buffer, &obj);
}

static const char* single_level_bencode = "d1:ai1e1:bi2e1:cl1:ai1e1:bi2eee";

void
ObjectStreamTest::testReadBencodeC() {
  torrent::Object orderedObj   = create_bencode_c(ordered_bencode);
  torrent::Object unorderedObj = create_bencode_c(unordered_bencode);

  CPPUNIT_ASSERT(!(orderedObj.flags() & torrent::Object::flag_unordered));
  CPPUNIT_ASSERT(unorderedObj.flags() & torrent::Object::flag_unordered);
  CPPUNIT_ASSERT(compare_bencode(orderedObj, ordered_bencode));

  //  torrent::Object single_level = create_bencode_c(single_level_bencode);
  torrent::Object single_level = create_bencode_c(single_level_bencode);

  CPPUNIT_ASSERT(compare_bencode(single_level, single_level_bencode));
}

bool object_write_bencode(const torrent::Object& obj, const char* original) {
  try {
    char buffer[1023];
    char* last = torrent::object_write_bencode(buffer, buffer + 1024, &obj).first;
    return std::strncmp(buffer, original, std::distance(buffer, last)) == 0;

  } catch (torrent::bencode_error& e) {
    return false;
  }
}

void
ObjectStreamTest::test_write() {
  torrent::Object obj;

  CPPUNIT_ASSERT(object_write_bencode(torrent::Object(), ""));
  CPPUNIT_ASSERT(object_write_bencode(torrent::Object((int64_t)1), "i1e"));
  CPPUNIT_ASSERT(object_write_bencode(torrent::Object("test"), "4:test"));
  CPPUNIT_ASSERT(object_write_bencode(torrent::Object::create_list(), "le"));
  CPPUNIT_ASSERT(object_write_bencode(torrent::Object::create_map(), "de"));

  obj = torrent::Object::create_map();
  obj.as_map()["a"] = (int64_t)1;
  CPPUNIT_ASSERT(object_write_bencode(obj, "d1:ai1ee"));

  obj.as_map()["b"] = "test";
  CPPUNIT_ASSERT(object_write_bencode(obj, "d1:ai1e1:b4:teste"));

  obj.as_map()["c"] = torrent::Object::create_list();
  obj.as_map()["c"].as_list().push_back("foo");
  CPPUNIT_ASSERT(object_write_bencode(obj, "d1:ai1e1:b4:test1:cl3:fooee"));

  obj.as_map()["c"].as_list().push_back(torrent::Object());
  obj.as_map()["d"] = torrent::Object();
  CPPUNIT_ASSERT(object_write_bencode(obj, "d1:ai1e1:b4:test1:cl3:fooee"));
}