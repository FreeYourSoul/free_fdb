// MIT License
//
// Copyright (c) 2021 Quentin Balland
// Repository : https://github.com/FreeYourSoul/free_fdb
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//         of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
//         to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//         copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
//         copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//         AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <catch2/catch.hpp>

#include <mutex>
#include <thread>

#include <fmt/format.h>

#include <free_fdb/ffdb.hh>

namespace {

[[nodiscard]] static std::string local_path_cluster_file() {
  std::string file_path = __FILE__;
  std::string dir_path = file_path.substr(0, file_path.rfind('\\'));
  if (dir_path.size() == file_path.size())
	dir_path = file_path.substr(0, file_path.rfind('/'));
  return dir_path + "/fdb.cluster";
}

}// namespace

std::once_flag once;

TEST_CASE("ffdb_testcase_put_get_delete") {

  // static, only one instance need to be done, re-starting network doesn't work well with foundationdb
  // https://github.com/apple/foundationdb/issues/2981
  static auto ffdb = ffdb::free_fdb(local_path_cluster_file());
  // full clear db for test
  std::call_once(once, [trans = ffdb.make_transaction()]() {
	trans->del_range("", "\xFF");
	trans->commit();
  });

  SECTION("put_get test") {
	auto trans = ffdb.make_transaction();
	trans->put("key_1", "value_1");
	trans->put("key_2", "value_2");
	trans->put("key_3", "value_3");
	trans->put("key_4", "value_4");

	auto kv1 = trans->get("key_1");
	auto kv2 = trans->get("key_2");
	auto kv3 = trans->get("key_3");
	auto kv4 = trans->get("key_4");

	CHECK(kv1.has_value());
	CHECK(kv2.has_value());
	CHECK(kv3.has_value());
	CHECK(kv4.has_value());

	CHECK(kv1->key == "key_1");
	CHECK(kv1->value == "value_1");

	CHECK(kv2->key == "key_2");
	CHECK(kv2->value == "value_2");

	CHECK(kv3->key == "key_3");
	CHECK(kv3->value == "value_3");

	CHECK(kv4->key == "key_4");
	CHECK(kv4->value == "value_4");

	auto kv_not_found = trans->get("NOT_FOUND");
	CHECK_FALSE(kv_not_found.has_value());

  }// End section : put_get test

  SECTION("None found (not committed before)") {
	//	std::this_thread::sleep_for(std::chrono::seconds(1));
	auto t = ffdb.make_transaction();
	auto k = t->get("key_1");

	CHECK_FALSE(k.has_value());
  }// End section : None found (not committed before)

  SECTION("commit test") {
	auto trans = ffdb.make_transaction();

	trans->put("key_1", "value_1");
	trans->put("key_2", "value_2");
	trans->put("key_3", "value_3");
	trans->put("key_4", "value_4");

	{
	  auto trans2 = ffdb.make_transaction();
	  auto key_1 = trans2->get("key_1");
	  auto key_2 = trans2->get("key_2");
	  auto key_3 = trans2->get("key_3");
	  auto key_4 = trans2->get("key_4");
	  CHECK_FALSE(key_1.has_value());
	  CHECK_FALSE(key_2.has_value());
	  CHECK_FALSE(key_3.has_value());
	  CHECK_FALSE(key_4.has_value());
	}
	trans->commit();

	{
	  auto trans2 = ffdb.make_transaction();
	  auto key_1 = trans2->get("key_1");
	  auto key_2 = trans2->get("key_2");
	  auto key_3 = trans2->get("key_3");
	  auto key_4 = trans2->get("key_4");
	  CHECK(key_1.has_value());
	  CHECK(key_2.has_value());
	  CHECK(key_3.has_value());
	  CHECK(key_4.has_value());
	}

	SECTION("delete test") {

	  auto trans_del = ffdb.make_transaction();

	  trans_del->del("key_1");
	  trans_del->del("key_2");
	  trans_del->del("key_3");

	  auto key_1 = trans_del->get("key_1");
	  auto key_2 = trans_del->get("key_2");
	  auto key_3 = trans_del->get("key_3");
	  auto key_4 = trans_del->get("key_4");

	  CHECK_FALSE(key_1.has_value());
	  CHECK_FALSE(key_2.has_value());
	  CHECK_FALSE(key_3.has_value());
	  CHECK(key_4.has_value());

	  trans_del->del("key_4");
	  key_4 = trans_del->get("key_4");
	  CHECK_FALSE(key_4.has_value());

	}// End section : delete test

  }// End section : test commit

  SECTION("list test") {

	auto trans_clear = ffdb.make_transaction();

	// full clear db to ensure everything is clean
	trans_clear->del_range("", "\xFF");
	trans_clear->commit();

	auto trans = ffdb.make_transaction();

	// 4 key 'A' starting
	trans->put("A_key_1", "A_value_1");
	trans->put("A_key_2", "A_value_2");
	trans->put("A_key_3", "A_value_3");
	trans->put("A_key_4", "A_value_4");
	// 2 key 'B' starting
	trans->put("B_key_1", "B_value_1");
	trans->put("B_key_2", "B_value_2");
	// 1 key 'C' starting
	trans->put("C_key_1", "C_value_1");
	// 3 key 'D' starting
	trans->put("D_key_1", "D_value_1");
	trans->put("D_key_2", "D_value_2");
	trans->put("D_key_3", "D_value_3");

	// from [A, B[ (B excluded) = 4
	auto result_A_to_B = trans->get_range("A", "B");

	CHECK(4 == result_A_to_B.values.size());
	CHECK_FALSE(result_A_to_B.truncated);

	CHECK(result_A_to_B.values[0].key == "A_key_1");
	CHECK(result_A_to_B.values[0].value == "A_value_1");
	CHECK(result_A_to_B.values[1].key == "A_key_2");
	CHECK(result_A_to_B.values[1].value == "A_value_2");
	CHECK(result_A_to_B.values[2].key == "A_key_3");
	CHECK(result_A_to_B.values[2].value == "A_value_3");
	CHECK(result_A_to_B.values[3].key == "A_key_4");
	CHECK(result_A_to_B.values[3].value == "A_value_4");

	// from [B, E[ (E excluded / non existant) = 6
	auto result_B_to_E = trans->get_range("B", "E");
	CHECK(6 == result_B_to_E.values.size());
	CHECK_FALSE(result_B_to_E.truncated);

	CHECK(result_B_to_E.values[0].key == "B_key_1");
	CHECK(result_B_to_E.values[0].value == "B_value_1");
	CHECK(result_B_to_E.values[1].key == "B_key_2");
	CHECK(result_B_to_E.values[1].value == "B_value_2");
	CHECK(result_B_to_E.values[2].key == "C_key_1");
	CHECK(result_B_to_E.values[2].value == "C_value_1");
	CHECK(result_B_to_E.values[3].key == "D_key_1");
	CHECK(result_B_to_E.values[3].value == "D_value_1");
	CHECK(result_B_to_E.values[4].key == "D_key_2");
	CHECK(result_B_to_E.values[4].value == "D_value_2");
	CHECK(result_B_to_E.values[5].key == "D_key_3");
	CHECK(result_B_to_E.values[5].value == "D_value_3");

	// '@' is before 'A' ==> man ascii
	// from [@, A_key_3[ (A_key_2 excluded / non existant) = 2
	auto result_a_to_A3 = trans->get_range("@", "A_key_3");
	CHECK(2 == result_a_to_A3.values.size());
	CHECK_FALSE(result_a_to_A3.truncated);

	CHECK(result_a_to_A3.values[0].key == "A_key_1");
	CHECK(result_a_to_A3.values[0].value == "A_value_1");
	CHECK(result_a_to_A3.values[1].key == "A_key_2");
	CHECK(result_a_to_A3.values[1].value == "A_value_2");

  }// End section : list test

}// End TestCase : ffdb_testcase_put_get_delete