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

#include <internal/future.hh>

#include <free_fdb/ffdb.hh>
#include <free_fdb/iterator.hh>

namespace {

constexpr fdb_bool_t not_reversed() {
  return fdb_bool_t{0};
}

constexpr fdb_bool_t reversed() {
  return fdb_bool_t{1};
}

}// namespace
namespace ffdb {

struct fdb_iterator::internal {

  internal(std::shared_ptr<fdb_transaction> t, it_options opt) : trans(std::move(t)), opt(std::move(opt)) {}

  void reset_iterator() {
	trans->reset();
	validity = false;
	current_result = {};
	seeker = nullptr;
  }

  std::shared_ptr<fdb_transaction> trans;
  it_options opt;

  fdb_result current_result{};

  int index = 1;
  bool validity = false;

  std::function<FDBFuture *(void)> seeker = nullptr;
};

fdb_iterator::~fdb_iterator() = default;

fdb_iterator::fdb_iterator(std::shared_ptr<fdb_transaction> t, it_options opt)
	: _impl(std::make_unique<internal>(std::move(t), std::move(opt))) {
}

void fdb_iterator::seek(std::string key) {
  std::string end = key;
  ++end.back();

  if (_impl->seeker) {
	_impl->reset_iterator();
  }
  _impl->validity = true;
  _impl->seeker = [this, key = std::move(key), end = std::move(end)]() {
	return fdb_transaction_get_range(
		_impl->trans->raw(),

		FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(reinterpret_cast<const uint8_t *>(key.c_str()), key.size()),
		FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(reinterpret_cast<const uint8_t *>(end.c_str()), end.size()),

		_impl->opt.limit, _impl->opt.max,
		FDBStreamingMode::FDB_STREAMING_MODE_ITERATOR, _impl->index,
		_impl->opt.snapshot, not_reversed());
  };
  next();
}

void fdb_iterator::seek_for_prev(std::string key) {
  std::string begin = key;
  --begin.back();
  if (_impl->seeker) {
	_impl->reset_iterator();
  }
  _impl->validity = true;
  _impl->seeker = [this, key = std::move(key), begin = std::move(begin)]() {
	return fdb_transaction_get_range(
		_impl->trans->raw(),

		FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(reinterpret_cast<const uint8_t *>(begin.c_str()), begin.size()),
		FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(reinterpret_cast<const uint8_t *>(key.c_str()), key.size()),

		_impl->opt.limit, _impl->opt.max,
		FDBStreamingMode::FDB_STREAMING_MODE_ITERATOR, _impl->index,
		_impl->opt.snapshot, reversed());
  };
  next();
}

void fdb_iterator::seek_first() {
  if (_impl->seeker) {
	_impl->reset_iterator();
  }
  _impl->validity = true;
  _impl->seeker = [this]() {
	return fdb_transaction_get_range(
		_impl->trans->raw(),

		FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(
			reinterpret_cast<const uint8_t *>(_impl->opt.iterate_lower_bound.c_str()), _impl->opt.iterate_lower_bound.size()),
		FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(
			reinterpret_cast<const uint8_t *>(_impl->opt.iterate_upper_bound.c_str()), _impl->opt.iterate_upper_bound.size()),

		_impl->opt.limit, _impl->opt.max,
		FDBStreamingMode::FDB_STREAMING_MODE_ITERATOR, _impl->index,
		_impl->opt.snapshot, not_reversed());
  };
  next();
}

void fdb_iterator::seek_last() {
  if (_impl->seeker) {
	_impl->reset_iterator();
  }
  _impl->validity = true;
  _impl->seeker = [&]() {
	return fdb_transaction_get_range(
		_impl->trans->raw(),

		FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(
			reinterpret_cast<const uint8_t *>(_impl->opt.iterate_lower_bound.c_str()), _impl->opt.iterate_lower_bound.size()),
		FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(
			reinterpret_cast<const uint8_t *>(_impl->opt.iterate_upper_bound.c_str()), _impl->opt.iterate_upper_bound.size()),

		_impl->opt.limit, _impl->opt.max,
		FDBStreamingMode::FDB_STREAMING_MODE_ITERATOR, _impl->index,
		_impl->opt.snapshot, reversed());
  };
  next();
}

void fdb_iterator::next() {
  if (is_valid()) {
	fdb_future fut(_impl->seeker());
	fut.get([this](FDBFuture *f) {
	  const FDBKeyValue *kv;
	  int count;
	  fdb_bool_t more;
	  check_fdb_code(fdb_future_get_keyvalue_array(f, &kv, &count, &more));

	  if (count == 0 || !_impl->validity) {
		_impl->validity = false;
		return std::nullopt;
	  }
	  int index = _impl->index - 1;
	  _impl->current_result = fdb_result{
		  std::string(static_cast<const char *>(kv[index].key), kv[index].key_length),
		  std::string(static_cast<const char *>(kv[index].value), kv[index].value_length)};
	  ++_impl->index;
	  _impl->validity = _impl->index <= count;
	  return std::nullopt;
	});
  }
}

bool fdb_iterator::is_valid() const {
  return _impl->validity;
}

const std::string &fdb_iterator::value() const {
  return _impl->current_result.value;
}

const std::string &fdb_iterator::key() const {
  return _impl->current_result.key;
}

fdb_iterator &fdb_iterator::operator++() {
  next();
  return *this;
}

const fdb_result &fdb_iterator::operator*() const {
  return _impl->current_result;
}

const fdb_iterator::value_type *fdb_iterator::operator->() const {
  return &_impl->current_result;
}

}// namespace ffdb
