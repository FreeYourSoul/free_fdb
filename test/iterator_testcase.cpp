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

#include "db_setup_test.hh"

static std::once_flag once;

TEST_CASE("iterator_testcase", "[db_test]") {

  // full clear db for test
  std::call_once(once, [trans = testing::ffdb.make_transaction()]() {
	trans->del_range("", "\xFF");
	trans->commit();
  });

  auto init_trans = testing::ffdb.make_transaction();

  // 4 key 'A' starting
  init_trans->put("A_key_1", "A_value_1");
  init_trans->put("A_key_2", "A_value_2");
  init_trans->put("A_key_3", "A_value_3");
  init_trans->put("A_key_4", "A_value_4");
  // 2 key 'B' starting
  init_trans->put("B_key_1", "B_value_1");
  init_trans->put("B_key_2", "B_value_2");
  // 1 key 'C' starting
  init_trans->put("C_key_1", "C_value_1");
  // 3 key 'D' starting
  init_trans->put("D_key_1", "D_value_1");
  init_trans->put("D_key_2", "D_value_2");
  init_trans->put("D_key_3", "D_value_3");

  init_trans->commit();

  SECTION("setup test") {
	auto trans = testing::ffdb.make_transaction();
	CHECK(trans->get("A_key_1"));
	CHECK(trans->get("A_key_2"));
	CHECK(trans->get("A_key_3"));
	CHECK(trans->get("A_key_4"));
	CHECK(trans->get("B_key_1"));
	CHECK(trans->get("B_key_2"));
	CHECK(trans->get("C_key_1"));
	CHECK(trans->get("D_key_1"));
	CHECK(trans->get("D_key_2"));
	CHECK(trans->get("D_key_3"));
  }// End section : Check setup

  SECTION("iterate through all with next") {
	auto opt = ffdb::it_options{"A", "D"};
	auto it = testing::ffdb.make_iterator(std::move(opt));

	it.seek_first();

	// first has been found
	CHECK(it.is_valid());
	std::uint32_t counter = 1;

	SECTION("with next") {

	  while (it.is_valid()) {
		++counter;
		it.next();
		if (counter > 7) {
		  FAIL("counter should be equal to 7, failure on iteration");
		}
	  }
	  CHECK(7 == counter);

	}// End section : with next

	SECTION("with ++") {
	  for (; it.is_valid(); ++it) {
		++counter;
		if (counter > 7) {
		  FAIL("counter should be equal to 7, failure on iteration");
		}
	  }
	  CHECK(7 == counter);
	}// End section : with ++

  }// End section : iterate through all

  SECTION("iterator seek_first") {

	auto opt = ffdb::it_options{"A", "D"};
	auto it = testing::ffdb.make_iterator(std::move(opt));

	// Start seeking
	try {
	  it.seek_first();
	} catch (std::exception &e) {
	  FAIL(fmt::format("error : {}", e.what()));
	}

	REQUIRE(it.is_valid());
	CHECK(it.value() == "A_value_1");
	CHECK(it.key() == "A_key_1");
	CHECK((*it).value == "A_value_1");
	CHECK((*it).key == "A_key_1");
	CHECK(it->value == "A_value_1");
	CHECK(it->key == "A_key_1");

	SECTION("iterate") {

	  // seek first imply a forward iteration
	  it.next();
	  REQUIRE(it.is_valid());
	  CHECK(it.value() == "A_value_2");
	  CHECK(it.key() == "A_key_2");

	  it.next();
	  REQUIRE(it.is_valid());
	  CHECK(it.value() == "A_value_3");
	  CHECK(it.key() == "A_key_3");

	  it.next();
	  REQUIRE(it.is_valid());
	  CHECK(it.value() == "A_value_4");
	  CHECK(it.key() == "A_key_4");

	  it.next();
	  REQUIRE(it.is_valid());
	  CHECK(it.value() == "B_value_1");
	  CHECK(it.key() == "B_key_1");

	  it.next();
	  REQUIRE(it.is_valid());
	  CHECK(it.value() == "B_value_2");
	  CHECK(it.key() == "B_key_2");

	  it.next();
	  // the iterator is finished to be iterated
	  CHECK_FALSE(it.is_valid());
	  CHECK(it.value() == "C_value_1");
	  CHECK(it.key() == "C_key_1");

	}// End section : iterate

  }// End section : iterator seek

  SECTION("iterator seek_last") {

	auto opt = ffdb::it_options{"A", "D"};
	auto it = testing::ffdb.make_iterator(std::move(opt));

	// Start seeking
	try {
	  it.seek_last();
	} catch (std::exception &e) {
	  FAIL(fmt::format("error : {}", e.what()));
	}

	REQUIRE(it.is_valid());
	CHECK(it.value() == "C_value_1");
	CHECK(it.key() == "C_key_1");
	CHECK((*it).value == "C_value_1");
	CHECK((*it).key == "C_key_1");
	CHECK(it->value == "C_value_1");
	CHECK(it->key == "C_key_1");

	SECTION("iterate") {

	  // seek last imply a backward iteration
	  it.next();
	  REQUIRE(it.is_valid());
	  CHECK(it.value() == "B_value_2");
	  CHECK(it.key() == "B_key_2");

	  it.next();
	  REQUIRE(it.is_valid());
	  CHECK(it.value() == "B_value_1");
	  CHECK(it.key() == "B_key_1");

	  it.next();
	  REQUIRE(it.is_valid());
	  CHECK(it.value() == "A_value_4");
	  CHECK(it.key() == "A_key_4");

	  it.next();
	  REQUIRE(it.is_valid());
	  CHECK(it.value() == "A_value_3");
	  CHECK(it.key() == "A_key_3");

	  it.next();
	  REQUIRE(it.is_valid());
	  CHECK(it.value() == "A_value_2");
	  CHECK(it.key() == "A_key_2");

	  it.next();
	  // the iterator is finished to be iterated
	  CHECK_FALSE(it.is_valid());
	  CHECK(it.value() == "A_value_1");
	  CHECK(it.key() == "A_key_1");

	}// End section : iterate

  }// End section : iterator seek_last

  SECTION("iterator seek") {

	auto opt = ffdb::it_options{"A", "F"};
	auto it = testing::ffdb.make_iterator(std::move(opt));

	it.seek("A"); // seek from A to B

	REQUIRE(it.is_valid());
	CHECK(it.value() == "A_value_1");
	CHECK(it.key() == "A_key_1");
	CHECK((*it).value == "A_value_1");
	CHECK((*it).key == "A_key_1");
	CHECK(it->value == "A_value_1");
	CHECK(it->key == "A_key_1");

	SECTION("iterate") {
	  // seek first imply a forward iteration
	  it.next();
	  REQUIRE(it.is_valid());
	  CHECK(it.value() == "A_value_2");
	  CHECK(it.key() == "A_key_2");

	  it.next();
	  REQUIRE(it.is_valid());
	  CHECK(it.value() == "A_value_3");
	  CHECK(it.key() == "A_key_3");

	  it.next();
	  // the iterator is finished to be iterated
	  CHECK_FALSE(it.is_valid());
	  CHECK(it.value() == "A_value_4");
	  CHECK(it.key() == "A_key_4");

	}// End section : iterate

  }// End section : iterator seek

  SECTION("iterate seek_for_prev") {

	auto opt = ffdb::it_options{"A", "F"};
	auto it = testing::ffdb.make_iterator(std::move(opt));

	it.seek_for_prev("B"); // seek from B to A backward
	REQUIRE(it.is_valid());
	CHECK(it.value() == "A_value_4");
	CHECK(it.key() == "A_key_4");
	CHECK((*it).value == "A_value_4");
	CHECK((*it).key == "A_key_4");
	CHECK(it->value == "A_value_4");
	CHECK(it->key == "A_key_4");

	SECTION("iterate") {
	  // seek first imply a backward iteration
	  it.next();
	  REQUIRE(it.is_valid());
	  CHECK(it.value() == "A_value_3");
	  CHECK(it.key() == "A_key_3");

	  it.next();
	  REQUIRE(it.is_valid());
	  CHECK(it.value() == "A_value_2");
	  CHECK(it.key() == "A_key_2");

	  it.next();
	  // the iterator is finished to be iterated
	  CHECK_FALSE(it.is_valid());
	  CHECK(it.value() == "A_value_1");
	  CHECK(it.key() == "A_key_1");

	}// End section : iterate

  }// End section : iterate seek_for_prev

  SECTION("iterator re-use") {

	auto opt = ffdb::it_options{"A", "D"};
	auto it = testing::ffdb.make_iterator(std::move(opt));

	it.seek_first(); // A_key_1
	it.next(); // A_key_2
	it.next(); // A_key_3

	REQUIRE(it.is_valid());
	CHECK(it.value() == "A_value_3");
	CHECK(it.key() == "A_key_3");

	// re-initialize by seeking something else
	it.seek_last(); // A_key_4
	it.next(); // A_key_3
	it.next(); // A_key_2

	REQUIRE(it.is_valid());
	auto& [key, value] = *it;
	CHECK(key == "A_key_2");
	CHECK(value == "A_value_2");

  }// End section : iterator re-use

  SECTION("iterator nothing found on seek") {

	auto it = testing::ffdb.make_iterator();

	it.seek("Z"); // no 'Z' key to be found

	CHECK_FALSE(it.is_valid());

	auto& [key, value] = *it;
	CHECK(key.empty());
	CHECK(value.empty());

	it.seek_for_prev("@");
	CHECK_FALSE(it.is_valid());

	auto& [key_2, value_2] = *it;
	CHECK(key_2.empty());
	CHECK(value_2.empty());

  }// End section : iterator nothing found on seek

}// End TestCase : iterator_testcase