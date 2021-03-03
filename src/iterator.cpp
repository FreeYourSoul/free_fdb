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
#include <free_fdb/iterator.hh>

namespace ffdb {

struct fdb_iterator::internal {

  internal(std::shared_ptr<fdb_transaction> t, it_options opt) : trans(std::move(t)), opt(std::move(opt)) {}

  std::shared_ptr<fdb_transaction> trans;
  it_options opt;
  fdb_result current_result;

  int index = 1;
  bool validity = false;

  std::function<FDBFuture *(void)> seeker;
};

fdb_iterator::~fdb_iterator() = default;

fdb_iterator::fdb_iterator(std::shared_ptr<fdb_transaction> t, it_options opt)
	: _impl(std::make_unique<internal>(std::move(t), std::move(opt))) {
}

void fdb_iterator::seek(const std::string &key) {
  std::string end = key;
  ++end.back();
  _impl->seeker = [&]() {
	return fdb_transaction_get_range(
		_impl->trans->raw(),

		FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(reinterpret_cast<const uint8_t *>(key.c_str()), key.size()),
		FDB_KEYSEL_LAST_LESS_OR_EQUAL(reinterpret_cast<const uint8_t *>(end.c_str()), end.size()),

		_impl->opt.iterate_limit, _impl->opt.iterate_max,
		FDBStreamingMode::FDB_STREAMING_MODE_ITERATOR, _impl->index,
		_impl->opt.snapshot, fdb_bool_t{0});
  };
  next();
}

void fdb_iterator::seek_for_prev(const std::string &key) {
  _impl->seeker = [&]() {
	return fdb_transaction_get_range(
		_impl->trans->raw(),

		reinterpret_cast<const uint8_t *>(key.c_str()), key.size(), 0, -1,
		FDB_KEYSEL_LAST_LESS_THAN(reinterpret_cast<const uint8_t *>(key.c_str()), key.size()),

		_impl->opt.iterate_limit, _impl->opt.iterate_max,
		FDBStreamingMode::FDB_STREAMING_MODE_ITERATOR, _impl->index,
		_impl->opt.snapshot, fdb_bool_t{0});
  };
  next();
}

void fdb_iterator::seek_first() {
  _impl->seeker = [&]() {
	return fdb_transaction_get_range(
		_impl->trans->raw(),

		FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(
			reinterpret_cast<const uint8_t *>(_impl->opt.iterate_lower_bound.c_str()), _impl->opt.iterate_lower_bound.size()),
		FDB_KEYSEL_LAST_LESS_OR_EQUAL(
			reinterpret_cast<const uint8_t *>(_impl->opt.iterate_upper_bound.c_str()), _impl->opt.iterate_upper_bound.size()),

		_impl->opt.iterate_limit, _impl->opt.iterate_max,
		FDBStreamingMode::FDB_STREAMING_MODE_ITERATOR, _impl->index,
		_impl->opt.snapshot, fdb_bool_t{0});
  };
  next();
}

void fdb_iterator::seek_last() {
  _impl->seeker = [&]() {
	return fdb_transaction_get_range(
		_impl->trans->raw(),

		FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(
			reinterpret_cast<const uint8_t *>(_impl->opt.iterate_lower_bound.c_str()), _impl->opt.iterate_lower_bound.size()),
		FDB_KEYSEL_LAST_LESS_OR_EQUAL(
			reinterpret_cast<const uint8_t *>(_impl->opt.iterate_upper_bound.c_str()), _impl->opt.iterate_upper_bound.size()),

		_impl->opt.iterate_limit, _impl->opt.iterate_max,
		FDBStreamingMode::FDB_STREAMING_MODE_ITERATOR, _impl->index,
		_impl->opt.snapshot, fdb_bool_t{1});
  };
  next();
}

void fdb_iterator::next() {
  if (_impl->validity) {
	fdb_future fut(_impl->seeker());
	++_impl->index;
	_impl->validity = true;
  }
}

bool fdb_iterator::is_valid() const {
  return _impl->validity;
}

std::string fdb_iterator::value() const {
  return _impl->current_result.value;
}

std::string fdb_iterator::key() const {
  return _impl->current_result.key;
}

void fdb_iterator::operator++() {
  next();
}

fdb_result fdb_iterator::operator*() const {
  return _impl->current_result;
}

}// namespace ffdb