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

#ifndef FREE_FDB_TEST_DB_SETUP_TEST_HH
#define FREE_FDB_TEST_DB_SETUP_TEST_HH

#include <mutex>
#include <thread>

#include "../include/free_fdb/ffdb.hh"

namespace testing {

[[nodiscard]] static std::string local_path_cluster_file() {
  std::string file_path = __FILE__;
  std::string dir_path = file_path.substr(0, file_path.rfind('\\'));
  if (dir_path.size() == file_path.size())
	dir_path = file_path.substr(0, file_path.rfind('/'));
  return dir_path + "/fdb.cluster";
}


// static, only one instance need to be done, re-starting network doesn't work well with foundationdb
// https://github.com/apple/foundationdb/issues/2981
static auto ffdb = ffdb::free_fdb(local_path_cluster_file());

}

#endif//FREE_FDB_TEST_DB_SETUP_TEST_HH
