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

#include <mutex>
#include <thread>

#include <internal/future.hh>

#include <free_fdb/ffdb.hh>

namespace ffdb {

static std::once_flag version_select_flag;

constexpr fdb_bool_t not_reversed() {
  return fdb_bool_t{0};
}

struct free_fdb::internal {

  explicit internal(const std::string &cluster_file_path) {

	std::call_once(version_select_flag, [this]() {
	  if (auto error = fdb_select_api_version(FDB_API_VERSION); error) {
		throw fdb_exception(fmt::format("Error Selecting version: {}", fdb_get_error(error)));
	  }

	  // Option debug trace
	  // fdb_network_set_option(FDBNetworkOption::FDB_NET_OPTION_TRACE_ENABLE, ...)

	  if (auto error = fdb_setup_network(); error) {
		throw fdb_exception(fmt::format("Error setup network: {}", fdb_get_error(error)));
	  }

	  t = std::thread([]() {
		if (auto error = fdb_run_network(); error) {
		  throw fdb_exception(fmt::format("Error while starting network: {}", fdb_get_error(error)));
		}
	  });

	});

	if (auto error = fdb_create_database(cluster_file_path.c_str(), &db); error) {
	  throw fdb_exception(fmt::format("Error creating DB: {}", fdb_get_error(error)));
	}
  }

  ~internal() {
	auto err = fdb_stop_network();
	if (err) {
	  /* An error occurred (probably network not running) */
	  fmt::print("Error while stopping network");
	}
	if (t.joinable()) {
	  t.join();
	}
  }

  FDBDatabase *db{};

  std::thread t;
};

free_fdb::~free_fdb() {
  if (_impl->db) {
	fdb_database_destroy(_impl->db);
  }
}

free_fdb::free_fdb(const std::string &cluster_file_path) : _impl(std::make_unique<internal>(cluster_file_path)) {
}

std::unique_ptr<fdb_transaction> free_fdb::make_transaction() {
  return std::make_unique<fdb_transaction>(_impl->db);
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

void fdb_transaction::del_range(const std::string &key_begin, const std::string &key_end) {
  if (_trans) {
	fdb_transaction_clear_range(
		_trans,
		reinterpret_cast<const uint8_t *>(key_begin.c_str()), key_begin.size(),
		reinterpret_cast<const uint8_t *>(key_end.c_str()), key_end.size());
  }
}

std::optional<fdb_result> fdb_transaction::get(const std::string &key) {
  if (_trans) {
	const auto *key_name = reinterpret_cast<const uint8_t *>(key.c_str());
	auto fut = fdb_future(fdb_transaction_get(_trans, key_name, key.size(), _snapshot_enabled));

	return fut.get([&key](FDBFuture *f) -> std::optional<fdb_result> {
	  fdb_bool_t out_present;
	  const uint8_t *out_value;
	  int out_length;

	  check_fdb_code(fdb_future_get_value(f, &out_present, &out_value, &out_length));
	  if (!out_present) {
		return std::nullopt;
	  }
	  return fdb_result{key, std::string(reinterpret_cast<const char *>(out_value), out_length)};
	});
  }
  return std::nullopt;
}

range_result fdb_transaction::get_range(const std::string &from, const std::string &to, range_options opt) {
  if (_trans) {

	auto appropriate_range_call = [this, &from, &to](const range_options& opt){
		if (opt.lower_bound_inclusive) {
		  if (opt.upper_bound_inclusive) {
			// [ begin, end ]
			return fdb_transaction_get_range(
				_trans,
				FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(reinterpret_cast<const uint8_t *>(from.c_str()), from.size()),
				FDB_KEYSEL_FIRST_GREATER_THAN(reinterpret_cast<const uint8_t *>(to.c_str()), to.size()),
				opt.limit, opt.max, FDBStreamingMode::FDB_STREAMING_MODE_WANT_ALL, 0, _snapshot_enabled, not_reversed());
		  } else {
			// [ begin, end [
			return fdb_transaction_get_range(
				_trans,
				FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(reinterpret_cast<const uint8_t *>(from.c_str()), from.size()),
				FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(reinterpret_cast<const uint8_t *>(to.c_str()), to.size()),
				opt.limit, opt.max, FDBStreamingMode::FDB_STREAMING_MODE_WANT_ALL, 0, _snapshot_enabled, not_reversed());
		  }
		} else {
		  if (opt.upper_bound_inclusive) {
			// ] begin, end ]
			return fdb_transaction_get_range(
				_trans,
				FDB_KEYSEL_FIRST_GREATER_THAN(reinterpret_cast<const uint8_t *>(from.c_str()), from.size()),
				FDB_KEYSEL_FIRST_GREATER_THAN(reinterpret_cast<const uint8_t *>(to.c_str()), to.size()),
				opt.limit, opt.max, FDBStreamingMode::FDB_STREAMING_MODE_WANT_ALL, 0, _snapshot_enabled, not_reversed());
		  } else {
			// ] begin, end [
			return fdb_transaction_get_range(
				_trans,
				FDB_KEYSEL_FIRST_GREATER_THAN(reinterpret_cast<const uint8_t *>(from.c_str()), from.size()),
				FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(reinterpret_cast<const uint8_t *>(to.c_str()), to.size()),
				opt.limit, opt.max, FDBStreamingMode::FDB_STREAMING_MODE_WANT_ALL, 0, _snapshot_enabled, not_reversed());
		  }
		}
	};

	auto fut = fdb_future(appropriate_range_call(opt));

	return fut.get([](FDBFuture *f) -> range_result {
	  const FDBKeyValue *key_value;
	  int out_count;
	  fdb_bool_t out_more;
	  check_fdb_code(fdb_future_get_keyvalue_array(f, &key_value, &out_count, &out_more));

	  range_result result{};
	  result.truncated = bool(out_more);
	  result.values.reserve(out_count);
	  for (int i = 0; i < out_count; ++i) {
		result.values.emplace_back(fdb_result{
			std::string(reinterpret_cast<const char *>(key_value[i].key), key_value[i].key_length),
			std::string(reinterpret_cast<const char *>(key_value[i].value), key_value[i].value_length)});
	  }
	  return result;
	});
  }
  return range_result{};
}

void fdb_transaction::enable_snapshot() {
  _snapshot_enabled = true;
}

FDBTransaction *fdb_transaction::raw() const {
  return _trans;
}

void fdb_transaction::reset() {
  fdb_transaction_reset(_trans);
}

void fdb_transaction::commit() {
  fdb_future(fdb_transaction_commit(_trans)).get();
}

// Counter

fdb_counter::fdb_counter(std::string key) : _key(std::move(key)) {
}

std::uint64_t fdb_counter::value(fdb_transaction &transaction) {
  return 0;
}

void fdb_counter::add(fdb_transaction &transaction, std::uint32_t increment) {
}

void fdb_counter::sub(fdb_transaction &transaction, std::uint32_t decrement) {
}

}// namespace ffdb