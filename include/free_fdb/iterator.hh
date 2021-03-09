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
  std::string iterate_lower_bound;
  std::string iterate_upper_bound;

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
  using value_type        = fdb_result;

  ~fdb_iterator();
  fdb_iterator(std::shared_ptr<fdb_transaction> transaction, it_options opt);

  /**
   *
   * @return
   */
  fdb_iterator& operator++();

  /**
   *
   * @return
   */
  [[nodiscard]] std::optional<value_type> operator*() const;

  /**
   *
   * @return
   */
  [[nodiscard]] std::optional<std::string> value() const;

  /**
   *
   * @return
   */
  [[nodiscard]] std::optional<std::string> key() const;

  /**
   *
   * @return
   */
  [[nodiscard]] bool is_valid() const;

  /**
   *
   * @param key
   */
  void seek(const std::string &key);

  /**
   *
   * @param key
   */
  void seek_for_prev(const std::string &key);

  /**
   *
   */
  void seek_first();

  /**
   *
   */
  void seek_last();

  /*
   *
   */
  void next();

private:
  std::unique_ptr<internal> _impl;
};

}// namespace ffdb

#endif//FREE_FDB_INCLUDE_FREE_FDB_ITERATOR_HH
