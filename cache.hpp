/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Copyright (C) 2012 Chuan Ji <jichuan89@gmail.com>                        *
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
// loading with pthreads.

#ifndef CACHE_HPP
#define CACHE_HPP

#include <pthread.h>
#include <map>
#include <queue>
#include <set>

// A generic cache that stores <key, value> pairs. The semantics for Load() and
// Discard() are implemented in implementing child classes. Supports
// asynchronous pre-emptive loading with pthreads.
template <typename K, typename V>
class Cache {
 public:
  // Create a cache with the given maximum size.
  Cache(int size);
  // DOES NOT CLEAR CACHE because it cannot call the virtual function Discard.
  // Child classes MUST call Clear() in their destructors. Waits for background
  // loading threads to terminate first.
  virtual ~Cache();
  // Retrieves an item. If the item is in the cache, simply returns it. If
  // not, loads it using the Load() function defined in an implementation.
  V Get(const K &key);
  // Starts a new thread to load an item into the cache. Note that this puts a
  // lock on this cache object, and calls to Get() while the asynchronous
  // loading is in progress will block.
  void Prepare(const K &key);
  // Returns the size of the cache.
  int GetSize() const;
  // Clears the cache, calling Discard() on all existing elements. Waits for
  // background loading threads to terminate first.
  void Clear();

 protected:
  // Loads a new element. This should be overridden in child classes.
  virtual V Load(const K &key) = 0;
  // Frees an element that has been evicted from the cache. This should be
  // overridden in child classes.
  virtual void Discard(const K &key, V &value) = 0;

 private:
  // A lock on this object. Calls to Get() and Prepare() will block for access.
  pthread_mutex_t _lock;
  // A map from keys to values.
  std::map<K, V> _map;
  // A queue of loaded keys, in the order they were loaded.
  std::queue<K> _queue;
  // Max size of this cache.
  int _size;
  // Keys that are being loaded by some thread.
  std::set<K> _work_set;
  // Condition variable used to broadcast work done.
  pthread_cond_t _work_set_update;
  // The set of currently running async loading threads.
  std::set<pthread_t> _threads;
};


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                              Implementation                               *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
namespace CacheInternal {

// Argument passed to pthread worker.
template <typename K, typename V>
struct CacheWorkerArg {
  // The Cache object that started the worker.
  Cache<K, V> *Caller;
  // The desired key.
  K Key;

  CacheWorkerArg(Cache<K, V> *caller, const K &key)
      : Caller(caller), Key(key) {
  }
};

// Cache worker method started by pthreads to load a key in the background.
// Takes ownership of _arg.
template <typename K, typename V>
void *CacheWorker(void * _arg) {
  CacheWorkerArg<K, V> *arg = reinterpret_cast<CacheWorkerArg<K, V> *>(_arg);
  arg->Caller->Get(arg->Key);
  delete arg;
  return NULL;
}

}

template <typename K, typename V>
Cache<K, V>::Cache(int size)
    : _size(size) {
  pthread_mutex_init(&_lock, NULL);
  pthread_cond_init(&_work_set_update, NULL);
}

template <typename K, typename V>
Cache<K, V>::~Cache() {
  Clear();
  pthread_cond_destroy(&_work_set_update);
  pthread_mutex_destroy(&_lock);
}

template <typename K, typename V>
V Cache<K, V>::Get(const K &key) {
  // 1. If key exists, return it.
  pthread_mutex_lock(&_lock);
  typename std::map<K, V>::iterator i = _map.find(key);
  if (i != _map.end()) {
    V value = i->second;
    _threads.erase(pthread_self());  // Safe even if called from main thread.
    pthread_mutex_unlock(&_lock);
    return value;
  }

  // 2. If key is being worked on, wait for it to finish, then go back to 1.
  bool was_in_work_set = false;
  while (_work_set.count(key)) {
    was_in_work_set = true;
    pthread_cond_wait(&_work_set_update, &_lock);
  }
  if (was_in_work_set) {
    pthread_mutex_unlock(&_lock);
    return Get(key);
  }

  // 2. Otherwise, do work to load it.
  _work_set.insert(key);
  pthread_cond_broadcast(&_work_set_update);
  pthread_mutex_unlock(&_lock);
  V value = Load(key);

  // 3. If someone else removed the key, we should discard our work. Otherwise,
  //    remove it and send out broadcast.
  pthread_mutex_lock(&_lock);
  if (!_work_set.count(key)) {
    pthread_mutex_unlock(&_lock);
    Discard(key, value);
    return Get(key);
  }
  _work_set.erase(key);
  pthread_cond_broadcast(&_work_set_update);

  // 4. Try inserting <key, value> into map. If it's already there, throw away
  //    away our result and return existing result.
  if (_map.count(key)) {
    V other_value = _map[key];
    _threads.erase(pthread_self());  // Safe even if called from main thread.
    pthread_mutex_unlock(&_lock);
    Discard(key, value);
    return other_value;
  }
  std::map<K, V> evicted;
  while (_queue.size() >= static_cast<size_t>(_size)) {
    K victim_key = _queue.front();
    evicted[victim_key] = _map[victim_key];
    _map.erase(victim_key);
    _queue.pop();
  }
  _map[key] = value;
  _queue.push(key);
  pthread_mutex_unlock(&_lock);

  for (typename std::map<K, V>::iterator i = evicted.begin();
       i != evicted.end(); ++i) {
    Discard(i->first, i->second);
  }
  pthread_mutex_lock(&_lock);
  _threads.erase(pthread_self());  // Safe even if called from main thread.
  pthread_mutex_unlock(&_lock);
  return value;
}

template <typename K, typename V>
void Cache<K, V>::Prepare(const K &key) {
  CacheInternal::CacheWorkerArg<K, V> *arg =
      new CacheInternal::CacheWorkerArg<K, V>(this, key);
  pthread_t thread;
  pthread_create(&thread, NULL, &(CacheInternal::CacheWorker<K, V>), arg);
  pthread_mutex_lock(&_lock);
  _threads.insert(thread);
  pthread_mutex_unlock(&_lock);
}

template <typename K, typename V>
int Cache<K, V>::GetSize() const {
  return _size;
}

template <typename K, typename V>
void Cache<K, V>::Clear() {
  pthread_mutex_lock(&_lock);
  std::set<pthread_t> threads_copy(_threads);
  pthread_mutex_unlock(&_lock);
  for (std::set<pthread_t>::const_iterator i = threads_copy.begin();
       i != threads_copy.end(); ++i) {
    pthread_join(*i, NULL);
  }

  pthread_mutex_lock(&_lock);
  std::map<K, V> map_copy(_map);
  _map.clear();
  while (_queue.size()) {
    _queue.pop();
  }
  pthread_mutex_unlock(&_lock);
  for (typename std::map<K, V>::iterator i = map_copy.begin();
       i != map_copy.end(); ++i) {
    Discard(i->first, i->second);
  }
}

#endif

