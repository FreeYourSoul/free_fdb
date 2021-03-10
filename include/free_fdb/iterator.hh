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

#ifndef FREE_FDB_INCLUDE_FREE_FDB_ITERATOR_HH
#define FREE_FDB_INCLUDE_FREE_FDB_ITERATOR_HH

#include <iterator>
#include <memory>
#include <string>
#include <vector>

namespace ffdb {

class fdb_transaction;

/**
 *
 */
struct it_options {
  std::string iterate_lower_bound{};
  std::string iterate_upper_bound{};

  //! if set to 0, no maximum is set, otherwise iteration stop when equal to the maximum iteration
  int limit = 0;
  //! max byte size from a range
  int max = 0;

  fdb_bool_t snapshot = 0;
};

/**
 *
 */
struct fdb_result {
  std::string key;
  std::string value;
};

/**
 *
 */
struct range_result {
  std::vector<fdb_result> values;
  bool truncated;
};

/**
 *
 */
class fdb_iterator {

  struct internal;

public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = fdb_result;

  ~fdb_iterator();
  fdb_iterator(std::shared_ptr<fdb_transaction> transaction, it_options opt);

  /**
   * Same as calling next
   * @return a reference to the current iterator
   */
  fdb_iterator &operator++();

  /**
   * @return the key/value pair the iterator currently hold
   */
  [[nodiscard]] const value_type &operator*() const;
  /**
   * @return the key/value pair the iterator currently hold
   */
  [[nodiscard]] const value_type *operator->() const;

  /**
   * @return value of the key/value pair the iterator currently hold
   */
  [[nodiscard]] const std::string &value() const;

  /**
   * @return key of the key/value pair the iterator currently hold
   */
  [[nodiscard]] const std::string &key() const;

  /**
   * Check if the iterator an be consumed from (next() method call).
   *
   * @return true if the iterator is not fully consumed yet
   */
  [[nodiscard]] bool is_valid() const;

  /**
   * Seek for the key provided
   * From there, goes forward (lexicographically speaking) after each next() call
   *
   * If none is found, the iterator is invalidated and an empty key/value pair is set for the current value held
   *
   * @param key to look for in foundationdb
   */
  void seek(std::string key);

  /**
   * Seek for the previous key before the one provided
   * From there, goes backward (lexicographically speaking) after each next() call
   *
   * If none is found, the iterator is invalidated and an empty key/value pair is set for the current value held
   *
   * @param key to find the previous key from in foundationdb
   */
  void seek_for_prev(std::string key);

  /**
   * Seek for the first element in the range from the options set at construction time of the iterator.
   * From there, goes forward (lexicographically speaking) after each next() call
   *
   * If none is found, the iterator is invalidated and an empty key/value pair is set for the current value held
   */
  void seek_first();

  /**
   * Seek for the last element in the range from the options set at construction time of the iterator.
   * From there, goes backward (lexicographically speaking) after each next() call
   *
   * If none is found, the iterator is invalidated and an empty key/value pair is set for the current value held
   */
  void seek_last();

  /**
   * Make the iterator go to the next element, if no such element exists or if the range is going beyond the options
   * set at construction time of the iterator, validity of the iterator is impacted (is_valid() return false)
   *
   * Going forward or backward (lexicographically speaking) depend on the seek that has been selected.
   * Refer to the documentation of the seeking method you use in order to know if it goes forward or backward
   *
   * Applying next to an invalid iterator do nothing.
   */
  void next();

private:
  std::unique_ptr<internal> _impl;
};

}// namespace ffdb

#endif//FREE_FDB_INCLUDE_FREE_FDB_ITERATOR_HH
