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

#include <internal/future.hh>

#include <free_fdb/ffdb.hh>

namespace ffdb {

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

fdb_iterator free_fdb::make_iterator(it_options range) {
  return fdb_iterator(make_transaction(), std::move(range));
}

fdb_transaction::fdb_transaction(FDBDatabase *db) {
  check_fdb_code(fdb_database_create_transaction(db, &_trans));
}

fdb_transaction::~fdb_transaction() {
  if (_trans) {
	fdb_transaction_destroy(_trans);
  }
}

void fdb_transaction::put(const std::string &key, const std::string &value) {
  if (_trans) {
	const auto *key_name = reinterpret_cast<const uint8_t *>(key.c_str());
	const auto *value_name = reinterpret_cast<const uint8_t *>(value.c_str());
	fdb_transaction_set(_trans, key_name, key.size(), value_name, value.size());
  }
}

void fdb_transaction::del(const std::string &key) {
  if (_trans) {
	fdb_transaction_clear(_trans, reinterpret_cast<const uint8_t *>(key.c_str()), key.size());
  }
}

std::optional<fdb_result> fdb_transaction::get(const std::string &key) {
  if (_trans) {
	const auto *key_name = reinterpret_cast<const uint8_t *>(key.c_str());
	auto fut = fdb_future(fdb_transaction_get(_trans, key_name, key.size(), _snapshot_enabled));

	return fut.get([](FDBFuture *f) -> std::optional<fdb_result> {
	  fdb_bool_t out_present;
	  const uint8_t *out_value;
	  const uint8_t *out_key;
	  int out_length;

	  check_fdb_code(fdb_future_get_value(f, &out_present, &out_value, &out_length));
	  std::string value = std::string(reinterpret_cast<const char *>(out_value));

	  check_fdb_code(fdb_future_get_key(f, &out_key, &out_length));
	  std::string key = std::string(reinterpret_cast<const char *>(out_value));
	  return fdb_result{std::move(key), std::move(value)};
	});
  }
  return std::nullopt;
}

//range_result fdb_transaction::get_range(const std::string &from, const std::string &to, std::uint32_t limit) {
//  if (_trans) {
//	const auto *key_name = reinterpret_cast<const uint8_t *>(key.c_str());
//	auto fut = fdb_future(fdb_transaction_get(_trans, key_name, key.size(), _snapshot_enabled));
//
//	return fut.get([](FDBFuture *f) -> range_result {
//
//	  return range_result{};
//	});
//  }
//  return range_result{};
//}

void fdb_transaction::enable_snapshot() {
  _snapshot_enabled = true;
}

FDBTransaction *fdb_transaction::raw() const {
  return _trans;
}

}// namespace ffdb