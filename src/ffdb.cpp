// MIT License
//
// Copyright (c) 2021 Quentin Balland
// Repository : https://github.com/FreeYourSoul/FyS
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

#define FDB_API_VERSION 620
#include <free_fdb/ffdb.hh>

namespace ffdb {

void check_fdb_code(fdb_error_t error) {
  if (error != 0) {
	if (fdb_error_predicate(FDBErrorPredicate::FDB_ERROR_PREDICATE_RETRYABLE, error)) {
	  throw transaction_exception(fdb_get_error(error));
	}
	throw fdb_exception(fdb_get_error(error));
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
	if (_data) {
	  if (auto error = fdb_future_block_until_ready(_data); error != 0) {
		throw fdb_exception(fdb_get_error(error));
	  }
	  check_fdb_code(fdb_future_get_error(_data));
	  return std::forward<Handler>(handler)(_data);
	}
  }

private:
  FDBFuture *_data;
};

struct free_fdb::internal {

  explicit internal(const std::string &cluster_file_path) {
	if (auto error = fdb_create_database(cluster_file_path.c_str(), &db); error) {
	  throw fdb_exception(fdb_get_error(error));
	}
  }

  FDBDatabase *db{};
};

free_fdb::~free_fdb() {
  if (_impl->db) {
	fdb_database_destroy(_impl->db);
  }
}

free_fdb::free_fdb(const std::string &cluster_file_path) : _impl(std::make_unique<internal>(cluster_file_path)) {
}

std::shared_ptr<fdb_transaction> free_fdb::make_transaction() {
  return std::make_shared<fdb_transaction>(_impl->db);
}

fdb_transaction::fdb_transaction(FDBDatabase *db) {
  check_fdb_code(fdb_database_create_transaction(db, &_trans));
}

fdb_transaction::~fdb_transaction() {
  if (_trans) {
	fdb_transaction_destroy(_trans);
  }
}
std::optional<fdb_result> fdb_transaction::get(const std::string &key) {
  if (_trans) {
	const auto *key_name = reinterpret_cast<const uint8_t *>(key.c_str());
	auto fut = fdb_future(fdb_transaction_get(_trans, key_name, key.size(), _snapshot_enabled));

	return fut.get([](FDBFuture *f) -> std::optional<fdb_result> {
	  	fdb_bool_t out_present;
	    const uint8_t* out_value;
	  	int out_value_length;
		check_fdb_code(fdb_future_get_value(f, &out_present, &out_value, &out_value_length));
	});
  }
  return std::nullopt;
}

void fdb_transaction::enable_snapshot() {
  _snapshot_enabled = true;
}

}// namespace ffdb