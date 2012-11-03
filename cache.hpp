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
#include <exception>
#include <map>
#include <queue>

// A generic cache that stores <key, value> pairs. The semantics for Load() and
// Discard() are implemented in implementing child classes. Supports
// asynchronous pre-emptive loading with pthreads.
template <typename K, typename V>
class Cache {
 public:
  // Create a cache with the given maximum size.
  Cache(int size);
  // This will call Discard() on everything still in the cache.
  ~Cache();
  // Retrieves an item. If the item is in the cache, simply returns it. If
  // not, loads it using the Load() function defined in an implementation.
  V Get(const K &key);
  // Starts a new thread to load an item into the cache. Note that this puts a
  // lock on this cache object, and calls to Get() while the asynchronous
  // loading is in progress will block.
  void Prepare(const K &key);
  // This is thrown by Load and Discard if not redefined in child class.
  class UnimplementedError: public std::exception {
   public:
    virtual const char *what() const throw();
  };
 protected:
  // Loads a new element. This should be overridden in child classes.
  V Load(const K &key) {
    throw UnimplementedError();
  }
  // Frees an element that has been evicted from the cache. This should be
  // overridden in child classes.
  void Discard(const K &key, V &value) {
    throw UnimplementedError();
  }
 private:
  // A lock on this object. Calls to Get() and Prepare() will block for access.
  pthread_mutex_t _lock;
  // A map from keys to values.
  std::map<K, V> _map;
  // A queue of loaded keys, in the order they were loaded.
  std::queue<K> _queue;
  // Max size of this cache.
  int _size;
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
};

// Cache worker method started by pthreads. Takes ownership of _arg.
template <typename K, typename V>
void *CacheWorker(void * _arg) {
  CacheWorkerArg<K, V> *arg = reinterpret_cast<CacheWorkerArg<K, V> *>(_arg);
  arg->Caller->Get(arg->Key);
  delete arg;
}

}

template <typename K, typename V>
Cache<K, V>::Cache(int size)
    : _size(size) {
  pthread_mutex_init(&_lock, NULL);
}

template <typename K, typename V>
Cache<K, V>::~Cache() {
  for (typename std::map<K, V>::iterator i = _map.begin();
       i != _map.end(); ++i) {
    Discard(i->first, i->second);
  }
  pthread_mutex_destroy(&_lock);
}

template <typename K, typename V>
V Cache<K, V>::Get(const K &key) {
  pthread_mutex_lock(&_lock);

  typename std::map<K, V>::iterator i = _map.find(key);
  if (i != _map.end()) {
    return i->second;
  }
  while (_queue.size() >= _size) {
    K victim = _queue.front();
    Discard(victim, _map[victim]);
    _queue.pop();
    _map.erase(victim);
  }
  _map[key] = Load(key);
  _queue.push(key);

  pthread_mutex_unlock(&_lock);
}

template <typename K, typename V>
void Cache<K, V>::Prepare(const K &key) {
  CacheInternal::CacheWorkerArg<K, V> *arg =
      new CacheInternal::CacheWorkerArg<K, V>();
  arg->Caller = this;
  arg->Key = key;
  pthread_t thread;
  pthread_create(&thread, NULL, &(CacheInternal::CacheWorker<K, V>), arg);
}

template <typename K, typename V>
const char *Cache<K, V>::UnimplementedError::what() const throw() {
  return "Implementation error: child class of Cache failed to override "
         "Discard and Load().";
}

#endif

