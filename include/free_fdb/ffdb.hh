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

#ifndef FREE_FDB_INCLUDE_FREE_DB_FFDB_HH
#define FREE_FDB_INCLUDE_FREE_DB_FFDB_HH

#include <fmt/format.h>

#include <exception>
#include <memory>

#define FDB_API_VERSION 610
#include <foundationdb/fdb_c.h>

#include <free_fdb/iterator.hh>
#include <utility>

namespace ffdb {

class fdb_exception : public std::exception {
public:
  explicit fdb_exception(std::string message) : std::exception(), _message(std::move(message)) {
  }

  [[nodiscard]] const char *what() const noexcept override {
	return _message.c_str();
  }

private:
  std::string _message;
};

class transaction_exception : public fdb_exception {
public:
  explicit transaction_exception(std::string message) : fdb_exception(std::move(message)) {
  }
};

class free_fdb;

/**
 * @brief Possible Options for range selection on a transaction (used by fdb_transaction::get_range method)
 *
 * by default :
 * - no limit either in bytes or number of item are imposed,
 * - lower_bound is inclusive
 * - upper_bound is exclusive
 */
struct range_options {
  //! if set to 0, no maximum is set, otherwise iteration stop when equal to the maximum iteration
  int limit = 0;
  //! max byte size from a range
  int max = 0;

  bool lower_bound_inclusive = true;
  bool upper_bound_inclusive = false;

};

/**
 * @brief RAII object encapsulating a FDBTransaction
 * If not committed, transaction is rolled back at destruction time.
 *
 * @see https://apple.github.io/foundationdb/api-c.html#transaction
 */
class fdb_transaction {

public:
  ~fdb_transaction();
  fdb_transaction(const fdb_transaction &) = delete;
  explicit fdb_transaction(FDBDatabase *db);

  /**
   * @brief Enable snapshot
   * @see https://apple.github.io/foundationdb/api-c.html#snapshot-reads
   */
  void enable_snapshot();

  /**
   * @brief Commit the current transaction
   *
   * @see https://apple.github.io/foundationdb/api-c.html#c.fdb_transaction_commit
   */
  void commit();

  /**
   * @brief Reset the current transaction to its original state
   *
   * @see https://apple.github.io/foundationdb/api-c.html#c.fdb_transaction_reset
   */
  void reset();

  /**
   * @brief Insert a new key / value pair in foundationdb
   *
   * @param key to insert
   * @param value to insert
   */
  void put(const std::string &key, const std::string &value);

  /**
   * @brief Remove a selected key from the DB
   * @param key to delete in foundationdb
   */
  void del(const std::string &key);

  /**
   * @brief Delete the range of key value between the provided begin and end inclusive
   *
   * @param key_begin key to start the deletion from (included)
   * @param key_end deletion up to that key (included)
   */
  void del_range(const std::string &key_begin, const std::string &key_end);

  /**
   * @warning this method is for internal purpose only and should not be used in order to improvise C API calls
   * @return the raw C API foundationdb transaction encapsulated in the current transaction
   */
  [[nodiscard]] FDBTransaction *raw() const;

  /**
   * @brief Get the key value at the specified key in foundationdb
   *
   * @param key to retrieve from the database
   * @return a key value structure if present, std::nullopt otherwise
   */
  std::optional<fdb_result> get(const std::string &key);

  /**
   * @brief Efficiently retrieve a full (depending on the potential limitation in the given option) range following
   * the provided options.
   *
   * In the case the full list is not wanted, consider using Iterators (free_fdb::make_iterator) which are more
   * convenient to use and would save cpu/memory usage.
   *
   * @param from key from where to start the range (inclusive/exclusive depending on options)
   * @param to key to end the range selection (inclusive/exclusive depending on options)
   * @param opt additional options for selection (limit / inclusion / exclusion etc..)
   *
   * @return range found from the foundation db respecting the provided options.
   *
   * @see https://apple.github.io/foundationdb/api-c.html#c.FDBStreamingMode
   */
  range_result get_range(const std::string &from, const std::string &to, range_options opt = {});

private:
  FDBTransaction *_trans = nullptr;
  bool _snapshot_enabled = false;
};

/**
 * @brief RAII Object representing an instance of the foundationdb,
 * At construction time a thread is launched in order to handle the fdb network.
 * At the first initialization of the foundationdb (ensured by a std::once_flag semaphore) network is setup
 *
 * Encapsulate a FDBDatabase pointer, connection to the database and network thread is closed when the object
 * is destructed
 *
 * @see https://apple.github.io/foundationdb/api-c.html#database
 * @see https://apple.github.io/foundationdb/api-c.html#network
 */
class free_fdb {
  struct internal;

public:
  ~free_fdb();
  explicit free_fdb(const std::string &cluster_file_path);

  /**
   * @return a pointer on a newly created transaction raii object
   */
  [[nodiscard]] std::unique_ptr<fdb_transaction> make_transaction();

  /**
   * @brief Make an iterator on the foundationdb, depending on the function called on the iterator to start the iteration
   * the upper_bound / lower_bound from it_options is used or not.
   *
   * Iterators are to be used in case the number of element to retrieve are unknown and may differ depending on the
   * usage.
   * In case you know you want to totality of elements in a given range, consider using fdb_transation::get_range
   * which returns a range of result at once in an efficient way.
   *
   * @param range options for the iteration (lower/upper bound, limit, etc..)
   * @return a new iterator on the foundationdb
   *
   * @see https://apple.github.io/foundationdb/api-c.html#c.FDBStreamingMode
   */
  fdb_iterator make_iterator(ffdb::it_options range = {});

private:
  std::unique_ptr<internal> _impl;
};

/**
 * Represent a counter in foundationdb,
 * The counter is represented as a std::int64_t in the database. It can be incre/decremented and retrieved.
 */
class fdb_counter {

public:
  explicit fdb_counter(std::string key);

  /**
   * Retrieve the current value of the counter
   * @param transaction from which the counter has to be retrieved
   * @return value of the counter
   */
  [[nodiscard]] std::int64_t value(fdb_transaction &transaction);

  /**
   * Increment a given amount to the counter.
   * The value is not clamped (no overflow management)
   * Modification is taken into account after the provided transaction does a commit.
   *
   * @param transaction on which the action is applied
   * @param increment amount to increment on the counter.
   */
  void add(fdb_transaction &transaction, std::int64_t increment = 1);

  /**
   * Decrement a given amount from the counter.
   * The value is not clamped (no underflow management)
   * Modification is taken into account after the provided transaction does a commit.
   *
   * @param transaction on which the action is applied
   * @param decrement amount to decrement from the counter
   */
  void sub(fdb_transaction &transaction, std::int64_t decrement = 1);

private:
  std::string _key;

};

}// namespace ffdb

#endif//FREE_FDB_INCLUDE_FREE_DB_FFDB_HH
