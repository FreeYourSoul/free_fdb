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

#ifndef FREE_FDB_INCLUDE_INTERNAL_FUTURE_HH
#define FREE_FDB_INCLUDE_INTERNAL_FUTURE_HH

#include <free_fdb/ffdb.hh>

namespace ffdb {

static void check_fdb_code(fdb_error_t error) {
  if (error != 0) {
	if (fdb_error_predicate(FDBErrorPredicate::FDB_ERROR_PREDICATE_RETRYABLE, error)) {
	  throw transaction_exception(fmt::format("Future, Non retry-able error : {}", fdb_get_error(error)));
	}
	throw fdb_exception(fmt::format("Future, Other error : {}", fdb_get_error(error)));
  }
}

class fdb_future {
public:
  ~fdb_future() {
	if (_data) {
	  fdb_future_destroy(_data);
	}
  }
  fdb_future(const fdb_future &) = delete;

  explicit fdb_future(FDBFuture *fut) : _data(fut) {
  }

  template<typename Handler>
  auto get(Handler &&handler) {
	if (!_data) {
	  throw fdb_exception("Error: Future data is null and thus cant be awaited.");
	}
	if (auto error = fdb_future_block_until_ready(_data); error != 0) {
	  throw fdb_exception(fmt::format("Error on future block : {}", fdb_get_error(error)));
	}
	check_fdb_code(fdb_future_get_error(_data));
	return std::forward<Handler>(handler)(_data);
  }

  void get() {
	get([](FDB_future *) { return std::optional<fdb_result>{}; });
  }

private:
  FDBFuture *_data;
};

}// namespace ffdb

#endif//FREE_FDB_INCLUDE_INTERNAL_FUTURE_HH
