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

TEST_CASE("counter_testcase") {
  // full clear db for test
  std::call_once(once, [trans = testing::ffdb.make_transaction()]() {
	trans->del_range("", "\xFF");
	trans->commit();
  });

  SECTION("add test") {

	std::string counter_name = "a_funny_counter";

	auto trans = testing::ffdb.make_transaction();
	ffdb::fdb_counter counter(counter_name);

	CHECK_FALSE(trans->get(counter_name).has_value());
	CHECK(counter.value(*trans) == 0);

	counter.add(*trans);

	CHECK(counter.value(*trans) == 1);

	counter.add(*trans);
	counter.add(*trans);

	CHECK(counter.value(*trans) == 3);

	counter.add(*trans, 255);

	CHECK(counter.value(*trans) == 258);

	SECTION("sub test") {

	  counter.sub(*trans, 259);
	  CHECK(counter.value(*trans) == -1);

	  counter.sub(*trans);
	  counter.sub(*trans);
	  CHECK(counter.value(*trans) == -3);
	  counter.sub(*trans);

	}// End section : sub test

  }// End section : add test

  SECTION("parallel aggressive") {
	std::string counter_name = "a_funny_counter";
	ffdb::fdb_counter counter(counter_name);

	auto t1 = std::thread([&](){
	  auto t = testing::ffdb.make_transaction();
	  for(int i = 0; i < 1000; ++i) {
		counter.add(*t);
	  }
	  t->commit();
	});
	auto t2 = std::thread([&](){
	  for(int i = 0; i < 1337; ++i) {
		auto t = testing::ffdb.make_transaction();
		counter.add(*t);
		t->commit();
	  }
	});
	auto t3 = std::thread([&](){
	  for(int i = 0; i < 500; ++i) {
		auto t = testing::ffdb.make_transaction();
		counter.sub(*t);
		t->commit();
	  }
	});
	t1.join();
	t2.join();
	t3.join();

	auto trans = testing::ffdb.make_transaction();
	CHECK(counter.value(*trans) == 1837);

  }// End section : parallel

}// End TestCase : counter_testcase