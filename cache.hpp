/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Copyright (C) 2012-2014 Chuan Ji <ji@chu4n.com>                          *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *   http://www.apache.org/licenses/LICENSE-2.0                              *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// This file defines the template class Cache, which is a fixed-size generic
// cache that stores key-value pairs. Users will need to supply methods to load
// and free elements in child classes. It also supports asynchronous pre-emptive
// loading with C++11 threads.

#ifndef CACHE_HPP
#define CACHE_HPP

#include <cassert>
#include <condition_variable>
#include <map>
#include <mutex>
#include <queue>
#include <set>
#include <thread>
#include <vector>

// A generic cache that stores <key, value> pairs. The semantics for Load() and
// Discard() are implemented in implementing child classes. Supports
// asynchronous pre-emptive loading with C++11 threads. For performance,
// multiple instances of Load() and Discard() may be executed at the same time,
// so these latter MUST be thread-safe. The class assumes K and V are copy-able
// and also cheap to copy; thus, they should be either primitive values or
// pointers.
template <typename K, typename V>
class Cache {
 public:
  // Create a cache with the given maximum size.
  explicit Cache(int size);
  // DOES NOT CLEAR CACHE because it cannot call the virtual function Discard.
  // Child classes MUST call Clear() in their destructors. Waits for background
  // loading threads to terminate first.
  virtual ~Cache();
  // Retrieves an item. If the item is in the cache, simply returns it. If
  // not, loads it using the Load() function defined in an implementation.
  V Get(const K& key);
  // Starts a new thread to load an item into the cache. Note that this puts a
  // lock on this cache object, and calls to Get() while the asynchronous
  // loading is in progress will block.
  void Prepare(const K& key);
  // Returns the size of the cache.
  int GetSize() const;
  // Clears the cache, calling Discard() on all existing elements. Waits for
  // background loading threads to terminate first. MUST BE CALLED from the
  // destructor of a child class.
  void Clear();

 protected:
  // Loads a new element. This should be overridden in child classes. MUST BE
  // THREAD-SAFE.
  virtual V Load(const K& key) = 0;
  // Frees an element that has been evicted from the cache. This should be
  // overridden in child classes. MUST BE THREAD-SAFE.
  virtual void Discard(const K& key, const V& value) = 0;

 private:
  // A lock on this object. Calls to Get() and Prepare() will block for access.
  std::mutex _mutex;
  // A map from keys to values.
  std::map<K, V> _map;
  // A queue of loaded keys, in the order they were loaded. This is used for
  // eviction.
  std::queue<K> _queue;
  // Max size of this cache.
  int _size;
  // Keys that are being loaded by some thread.
  std::set<K> _work_set;
  // Condition variable used to broadcast work done.
  std::condition_variable _condition;
};


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                              Implementation                               *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
template <typename K, typename V>
Cache<K, V>::Cache(int size)
    : _size(size) {
}

template <typename K, typename V>
Cache<K, V>::~Cache() {
}

template <typename K, typename V>
V Cache<K, V>::Get(const K& key) {
  for (;;) {
    std::unique_lock<std::mutex> lock(_mutex);

    // 1. If key is already loaded, return the corresponding value.
    auto i = _map.find(key);
    if (i != _map.end()) {
      return i->second;
    }

    // 2. Otherwise, schedule loading and wait for a notification. Since the
    // loading thread requires locking _mutex, it will not actually start
    // running until after we release the lock in the following call to
    // _condition.wait(). This is important because otherwise the loading might
    // finish and notify before we start waiting, in which case we would end up
    // waiting forever.
    Prepare(key);

    // 3. Wait for notification. This releases the mutex, which allows the
    // loading thread to start.
    _condition.wait(lock);
  }

  // 4. The notification could have come from the thread that is responsible
  // for loading our key or another thread. Additionally, our key could have
  // been evicted before we could acquire a lock and return its value. Either
  // way, we go back to 1.
}

template <typename K, typename V>
void Cache<K, V>::Prepare(const K& key) {
  std::thread thread([=] (const K& key) {
    {
      std::unique_lock<std::mutex> lock(_mutex);

      // 1. If key is already in the cache or being loaded by another thread, no
      // need to do extra work.
      if (_map.count(key) || _work_set.count(key)) {
        _condition.notify_all();
        return;
      }
      // 2. Tell other threads we're going to load the key.
      _work_set.insert(key);
    }

    // 3. Do the actual loading.
    V value = Load(key);

    {
      std::unique_lock<std::mutex> lock(_mutex);

      // 4. Tell other threads we're done.
      assert(_work_set.count(key));
      _work_set.erase(key);

      // 5. Add (key, value) to cache.
      assert(!_map.count(key));
      _map[key] = value;

      // 6. Add key to queue.
      _queue.push(key);

      // 7. If the cache size is now too large, evict some entries in separate
      // threads.
      while (_queue.size() >= static_cast<size_t>(_size)) {
        K evicted_key = _queue.front();
        V evicted_value = _map[evicted_key];

        _map.erase(evicted_key);
        _queue.pop();

        std::thread eviction_thread([=] {
          Discard(evicted_key, evicted_value);
        });
        eviction_thread.detach();
      }
    }

    // 8. Finally, let everyone know the cache was modified.
    _condition.notify_all();
  }, key);

  thread.detach();
}

template <typename K, typename V>
int Cache<K, V>::GetSize() const {
  return _size;
}

template <typename K, typename V>
void Cache<K, V>::Clear() {
  std::vector<std::thread> discard_threads;
  {
    std::unique_lock<std::mutex> lock(_mutex);
    // 1. Block until all ongoing loads are complete.
    _condition.wait(lock, [=] { return _work_set.empty(); });
    // 2. Clear queue.
    while (_queue.size()) {
      _queue.pop();
    }
    // 3. Clear cache and start a thread to call Discard() on each entry.
    for (auto& i : _map) {
      K key = i.first;
      V value = i.second;
      discard_threads.push_back(std::thread([=] {
        Discard(key, value);
      }));
    }
  }
  // 4. Wait for all Discard() calls to complete.
  for (std::thread& discard_thread : discard_threads) {
    discard_thread.join();
  }
}

#endif

