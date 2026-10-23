/**
 * @author TendTo (https://github.com/TendTo)
 * @copyright 2026 lightrat
 * @licence Apache License 2.0
 * @file
 * MpqStore class.
 */
#pragma once

#include <gmp.h>

#include <cassert>
#include <deque>
#include <memory>
#include <stack>
#include <utility>

#ifdef LIGHTRAT_THREAD_SAFE
#include <mutex>
#endif

namespace lightrat {

#if 1
/**
 * Store for GMP `mpq_t` objects.
 * The store allocates `mpq_t` objects on demand and maintains a free list of
 * returned objects for reuse. The store is a singleton and is accessed through
 * the static `alloc()`, `free()`, and `clear()` methods.
 * @note The store retains ownership of an `mpq_t` returned by `alloc()`.
 * Call `free()` when the object is no longer in use.
 */
class MpqStore {
 private:
  struct mpq_wrapper {
    mpq_t q;  ///< The GMP rational value managed by this wrapper.
    /**
     * Initialize the rational in canonical form with a denominator of one.
     * The numerator and denominator are preallocated with 64 bits of storage,
     * and the denominator is initialized to one, since a 32-bit denominator would be stored as an integer pair
     * in the Rational class.
     */
    mpq_wrapper() : q{} {
      mpz_init2(mpq_numref(q), 64);
      mpz_init2(mpq_denref(q), 64);
      mpz_set_ui(mpq_denref(q), 1UL);
    }
    /** Release the GMP storage associated with the rational value. */
    ~mpq_wrapper() { mpq_clear(q); }
  };
  /**
   * An allocated rational value together with a link to the next free value.
   * The `next` pointer is `nullptr` while the link is in use.
   */
  struct mpq_link_t {
    mpq_wrapper mpq{};          ///< The GMP rational value returned to the caller.
    mpq_link_t* next{nullptr};  ///< Pointer to the next free link in the pool.
  };

 public:
  /**
   * Allocate a GMP rational value from the store.
   * A returned value remains owned by the store and must be returned with
   * `free()` when it is no longer in use.
   * @return A pointer to an allocated GMP rational value.
   */
  static mpq_ptr alloc() {
    MpqStore& store = instance();
#ifdef LIGHTRAT_THREAD_SAFE
    std::lock_guard<std::mutex> lock(store.mtx);
#endif
    if (store.pool != nullptr) {  // There are free mpq_link_t objects in the pool
      mpq_link_t* tmp = store.pool;
      mpq_ptr r = tmp->mpq.q;
      store.pool = tmp->next;
#ifndef NDEBUG
      tmp->next = nullptr;  // sanity check: when returned it should still be nullptr
#endif
      return r;
    }
    // There are no free mpq_link_t objects in the pool, so we need to allocate a new one
    return store.store.emplace_back().mpq.q;
  }

  /**
   * Return a GMP rational value to the store.
   * @pre `mpq` must have been returned by `alloc()` and must not already have been returned with `free()`.
   * @param q The GMP rational value to return to the store.
   */
  static void free(mpq_ptr q) {
    MpqStore& store = instance();  // Return the mpq to the pool
#ifdef LIGHTRAT_THREAD_SAFE
    std::lock_guard<std::mutex> lock(store.mtx);
#endif
    auto* const obj = reinterpret_cast<mpq_link_t*>(q);
    assert(obj->next == nullptr);
    obj->next = store.pool;
    store.pool = obj;
  }

  /**
   * Release all rational values held by the store.
   * Pointers previously returned by `alloc()` become invalid after this call.
   */
  static void clear() {
    MpqStore& store = instance();
#ifdef LIGHTRAT_THREAD_SAFE
    std::lock_guard<std::mutex> lock(store.mtx);
#endif
    store.store.clear();
    store.pool = nullptr;
  }

 private:
  /**
   * Return the singleton store instance.
   * @return A reference to the singleton store instance.
   */
  static MpqStore& instance() {
    static MpqStore instance;
    return instance;
  }

  /** Construct an empty store. */
  MpqStore() : store{}, pool{nullptr} {}

  std::deque<mpq_link_t>
      store;         ///< Storage for allocated rational values.
                     ///< A deque is used so that adding elements does not invalidate pointers to existing elements.
  mpq_link_t* pool;  ///< Pointer to the first free link in the pool.
#ifdef LIGHTRAT_THREAD_SAFE
  std::mutex mtx;  ///< Mutex for thread safety when allocating and freeing rational values.
#endif
};

#else
constexpr int MPQ_BLOCK_COUNT = 1024;

/**
 * A store for GMP mpq_t objects.
 * The store allocates blocks of mpq_t objects and maintains a free list of available objects.
 * When an mpq_t object is allocated, it is initialized and returned to the caller,
 * and when it is freed, it is added back to the free list for reuse.
 * The store is a singleton and is accessed through the static `alloc()` and `free()` methods,
 * and can be built to be thread safe.
 */
class MpqStore {
  /**
   * An mpq_link is an mpq together with a pointer to the "next" free mpq_link
   * needed to prevent "modifying the contents" of the mpq, as is done
   * in utils/object_stores.[ch]
   */
  struct mpq_link_t {
    mpq_t mpq;
    mpq_link_t* next;
  };

  /**
   * A bank is a block of mpq links.
   * - blocks are linked in a list
   */
  struct mpq_bank_t {
    std::unique_ptr<mpq_bank_t> next;
    mpq_link_t block[MPQ_BLOCK_COUNT] = {};
  };

  /**
   * The store is initialized with a fixed number of objects in each block.
   */
  MpqStore() {
    bnk = nullptr;
    free_list = nullptr;
    free_index = 0;
  }

  /** Delete the full store: all banks are freed. */
  ~MpqStore() {
    std::unique_ptr<mpq_bank_t> b = std::move(bnk);
    std::uint32_t k = free_index;
    while (b != nullptr) {
      for (std::uint32_t i = k; i < MPQ_BLOCK_COUNT; i++) {
        mpq_link_t* obj = b->block + i;
        mpq_clear(obj->mpq);
      }
      k = 0;
      b = std::move(b->next);
    }

    bnk = nullptr;
    free_list = nullptr;
    free_index = 0;
  }

 public:
  static mpq_ptr alloc() { return instance().alloc_impl(); }
  static void free(mpq_ptr mpq) { instance().free_impl(mpq); }

 private:
  static MpqStore& instance() {
    static MpqStore instance;
    return instance;
  }
  /** Allocate an mpq. */
  mpq_ptr alloc_impl() {
    mpq_link_t* obj = free_list;

    if (obj != nullptr) {
      free_list = obj->next;
      // sanity check: when returned it should still be nullptr
      obj->next = nullptr;
      return reinterpret_cast<mpq_ptr>(&obj->mpq);
    }

    std::uint32_t i = free_index;
    if (i == 0) {
      std::unique_ptr<mpq_bank_t> new_bank = std::make_unique<mpq_bank_t>();
      new_bank->next = std::move(bnk);
      bnk = std::move(new_bank);
      i = MPQ_BLOCK_COUNT;
    }

    i--;
    free_index = i;
    obj = bnk->block + i;

    // only initialize when we give it out, ensuring q has enough room for numerator and denominator with 64 bits
    mpz_init2(mpq_numref(obj->mpq), 64);
    mpz_init2(mpq_denref(obj->mpq), 64);
    mpz_set_ui(mpq_denref(obj->mpq), 1UL);

    obj->next = nullptr;  // sanity check: when returned it should still be nullptr

    return reinterpret_cast<mpq_ptr>(&obj->mpq);  // same as obj
  }

  /**
   * Free an allocated mpq_link object: add it to free_list.
   * next pointer is stored in mpq_link->next
   */
  void free_impl(mpq_ptr mpq) {
    mpq_link_t* const obj = reinterpret_cast<mpq_link_t*>(mpq);
    // sanity check: it's being returned; it should still be nullptr
    assert(obj->next == nullptr);
    obj->next = free_list;
    free_list = obj;
  }
  /**
   * Store = a list of blocks
   * - allocations are performed via a free list,
   * - or in the first block of the bank list,
   * - or by adding a new block.
   */
  std::unique_ptr<mpq_bank_t> bnk;  ///< First block in the bank list
  mpq_link_t* free_list;            ///< List of free mpq_links
  std::uint32_t free_index;         ///< Index of last allocated mpq_link in first block
};
#endif

}  // namespace lightrat
