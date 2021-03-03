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

#ifndef FREE_FDB_INCLUDE_FREE_DB_FFDB_HH
#define FREE_FDB_INCLUDE_FREE_DB_FFDB_HH

#include <fmt/format.h>

#include <exception>
#include <memory>

#include <foundationdb/fdb_c.h>

#include <free_fdb/iterator.hh>

namespace ffdb {

class fdb_exception : public std::exception {
public:
  explicit fdb_exception(const std::string& message) : std::exception(), _message(message) {
  }

  [[nodiscard]] const char *what() const noexcept override {
	return fmt::format(FMT_STRING("FoundationDB error {}"), std::exception::what()).c_str();
  }

private:
  std::string _message;
};

class transaction_exception : public fdb_exception {
public:
  explicit transaction_exception(const char *message) : fdb_exception(message) {
  }

  [[nodiscard]] const char *what() const noexcept override {
	return fmt::format(FMT_STRING("Transaction error {}"), fdb_exception::what()).c_str();
  }
};

struct fdb_result {
  std::string key;
  std::string value;
};

class free_fdb;


/**
 *
 */
class fdb_transaction {

public:
  ~fdb_transaction();
  fdb_transaction(const fdb_transaction &) = delete;
  explicit fdb_transaction(FDBDatabase *db);

  void enable_snapshot();

  void put(const std::string &key, const std::string &value);
  void del(const std::string &key);

  std::optional<fdb_result> get(const std::string &key);

  fdb_iterator make_iterator();

  //  void del_range(const std::string &from, const std::string &to, std::uint32_t limit);
  //  fdb_iterator get_range(const std::string &from, const std::string &to, std::uint32_t limit);

private:
  FDBTransaction *_trans = nullptr;
  bool _snapshot_enabled = false;
};

/**
 *
 */
class free_fdb {
  struct internal;

public:
  ~free_fdb();
  explicit free_fdb(const std::string &cluster_file_path);

  std::shared_ptr<fdb_transaction> make_transaction();

private:
  std::unique_ptr<internal> _impl;
};

}// namespace ffdb

#endif//FREE_FDB_INCLUDE_FREE_DB_FFDB_HH
