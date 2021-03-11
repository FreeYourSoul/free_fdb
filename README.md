[![Codacy Badge](https://app.codacy.com/project/badge/Grade/33a26831117e4d6bb71e0cda88c5692b)](https://www.codacy.com/gh/FreeYourSoul/free_fdb/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=FreeYourSoul/free_fdb&amp;utm_campaign=Badge_Grade)
[![Documentation Status](https://codedocs.xyz/FreeYourSoul/free_fdb.svg)](https://codedocs.xyz/FreeYourSoul/free_fdb/)

# Free FoundationDB

A C++ binding library for [FoundationDB](https://www.foundationdb.org/).

## Dependencies

* fmt : C++ formatting library (standardized in c++20)
* FoundationDB C binding : official C binding library shipped by foundationdb
* C++17 compliant compiler

# Feature & doc

## Quick start

Here is the quick start tutorial with some example of how to use the c++ binding above. The [doxygen documentation](https://codedocs.xyz/FreeYourSoul/free_fdb/) provide more precise information.

**Making a database connection.**

```c++
#include <free_fdb/ffdb.hh>

auto ffdb = ffdb::free_fdb("path/to/fdb.cluster");

// making a transaction
auto transaction = ffdb.make_transaction();

// making an iterator
auto iterator = ffdb.make_iterator();

// transaction & iterator can then be used as shown below
```
> When building an instance of `ffdb::free_fdb` class, at construction time a thread is launched in order to handle the fdb network.
> On the first initialization of the foundationdb `ffdb::free_fdb` instance the network is setup (it is ensured by a std::once_flag semaphore).

---

* Put/Get/Remove key/value
  ```c++
  auto trans = ffdb_instance.make_transaction();
  
  // add kev / value
  trans->put("key_1", "value_1");

  // retrieve kev / value
  auto kv = trans->get("key_1");
  
  assert(kv);
  assert(kv->key == "key_1");
  assert(kv->value == "value_1");

  // delete kev / value
  trans->del("key_1");

  assert(trans->get("key_1"));
  ```
  
* Get/Delete a range of key/value
  ```c++
  // We assume 4 key values are currently present in foundationdb
  // A_key_1/A_value_1, A_key_2/A_value_2, A_key_3/A_value_3, B_key_1/B_value_1 
  
  auto trans = ffdb_instance.make_transaction();

  // get range from [A, B[ (B exclusive by default)
  auto result_A_to_B = trans->get_range("A", "B");

  assert(3 == result_A_to_B.values.size());
  assert(!result_A_to_B.truncated);
  assert(result_A_to_B.values[0].key == "A_key_1");
  assert(result_A_to_B.values[0].value == "A_value_1");
  assert(result_A_to_B.values[1].key == "A_key_2");
  assert(result_A_to_B.values[1].value == "A_value_2");
  assert(result_A_to_B.values[2].key == "A_key_3");
  assert(result_A_to_B.values[2].value == "A_value_3");

  trans->del_range("A", "B");

  // last key still there out of the 4 initial keys
  auto last = trans->get("B_key_1");
  assert(last);
  assert(last->key == "B_key_1");
  assert(last->value == "B_value_1");

  ```
  A useful trick, to delete completely the database:
  ```c++
      auto trans = ffdb_instance.make_transaction();
      trans_clear->del_range("", "\xFF");
  ```
  
* Iterator implementation for range access
  ```c++
  // We assume 4 key values are currently present in foundationdb
  // A_key_1/A_value_1, A_key_2/A_value_2, A_key_3/A_value_3, B_key_1/B_value_1 

  auto it = ffdb_instance.make_iterator();

  // seek the first key starting with 'A', iteration stop until the next character ('B')
  it.seek("A");

  for (; it.is_valid(); ++it) {
    auto &[key, value] = *it;
    std::cout << key << ":" << value << "\n";
  }

  // output : 
  // A_key_1 : A_value_1
  // A_key_2 : A_value_2
  // A_key_3 : A_value_3
  ```  
  
> `seek` is not the only available way to seek initialize your iterator lookup. There are `seek_for_prev(std::string)` or `seek_first()` and `seek_last()` working with the iterator option (parameter of the make_iterator which has not been used in this example). 

* Counter implementation (using foundationdb atomic operations)
  ```c++
  
  ```

A complete doxygen documentation is available [here](https://codedocs.xyz/FreeYourSoul/free_fdb/). 

## Installation

### Using Nix

A nix utility is also present in order to install/build locally free_fdb for local development. It is used by the CI.

```shell
nix-build . -A ffdb
```

If you want to integrate free_fdb from your local nix project, you can add an attribute by fetching the recipe.nix file to your nix file directly with the url. Or you can just copy the recipe.nix file into your repository.

Example taken from the [FyS project](https://github.com/FreeYourSoul/FyS), {commit-hash} has to be replaced in order to fix the recipe to use in your project:

```shell
let pkgs = import <nixpkgs> { };
in with pkgs; rec {

  # External Dependencies (personal)
  ffdb = (callPackage (builtins.fetchurl
    "https://raw.githubusercontent.com/FreeYourSoul/free_fdb/{commit-hash}/recipee.nix") {
      rev = "{commit-hash}";
    });
    
    #.... your nix
```

