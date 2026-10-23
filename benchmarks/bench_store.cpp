/**
 * @author TendTo (https://github.com/TendTo)
 * @copyright 2026 lightrat
 * @licence Apache License 2.0
 * @file
 */
#include <benchmark/benchmark.h>

#include <array>
#include <deque>
#include <list>
#include <stack>
#include <vector>

#include "benchmarks/utils.hpp"

namespace opensmt {

class MpqStore {
 public:
  struct mpq_link_t {
    mpq_class mpq{};
    mpq_link_t* next{nullptr};
  };

  MpqStore() : store{}, pool{nullptr} {}
  mpq_ptr alloc() {
    if (pool != nullptr) {  // There are free mpq_link_t objects in the pool
      mpq_link_t* tmp = pool;
      mpq_ptr r = tmp->mpq.get_mpq_t();
      pool = tmp->next;
      tmp->next = nullptr;  // sanity check: when returned it should still be nullptr
      return r;
    }
    // There are no free mpq_link_t objects in the pool, so we need to allocate a new one
    mpq_ptr r = store.emplace().mpq.get_mpq_t();
    mpz_init2(mpq_numref(r), 64);  // Preallocate numerator with 64 bits, since 32-bit would be stored in the cheap way
    mpz_init2(mpq_denref(r), 64);
    mpz_set_ui(mpq_denref(r), 1UL);
    return r;
  }
  bool free(mpq_ptr ptr) {
    // Return the mpq to the pool
    auto* const obj = reinterpret_cast<mpq_link_t*>(ptr);
    obj->next = pool;
    pool = obj;
    return true;
  }

  std::stack<mpq_link_t> store;  // uses deque as storage to avoid realloc
  mpq_link_t* pool;
};

}  // namespace opensmt

namespace Yices {
constexpr int MPQ_BLOCK_COUNT = 1024;

class MpqStore {
 public:
  struct mpq_link_t {
    mpq_t mpq;
    mpq_link_t* next;
  };
  struct mpq_bank_t {
    std::unique_ptr<mpq_bank_t> next;
    mpq_link_t block[MPQ_BLOCK_COUNT] = {};
  };
  MpqStore() : bnk{}, free_list{nullptr}, free_index{0} {}
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

  static MpqStore& instance() {
    static MpqStore instance;
    return instance;
  }
  /** Allocate an mpq. */
  mpq_ptr alloc() {
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

  bool free(mpq_ptr mpq) {
    mpq_link_t* const obj = reinterpret_cast<mpq_link_t*>(mpq);
    // sanity check: it's being returned; it should still be nullptr
    assert(obj->next == nullptr);
    obj->next = free_list;
    free_list = obj;
    return true;
  }
  std::unique_ptr<mpq_bank_t> bnk;  ///< First block in the bank list
  mpq_link_t* free_list;            ///< List of free mpq_links
  std::uint32_t free_index;         ///< Index of last allocated mpq_link in first block
};
}  // namespace Yices

namespace simplified {
constexpr int MPQ_BLOCK_COUNT = 1024;

class MpqStore {
 public:
  struct mpq_link_t {
    mpq_t mpq;
    mpq_link_t* next;
  };
  MpqStore() : bnk{}, free_list{nullptr}, free_index{0} {}
  ~MpqStore() {
    std::uint32_t k = free_index;
    for (mpq_link_t* const block : bnk) {
      for (std::uint32_t i = k; i < MPQ_BLOCK_COUNT; i++) {
        mpq_link_t* const obj = block + i;
        mpq_clear(obj->mpq);
      }
      k = 0;
    }

    bnk.clear();
    free_list = nullptr;
    free_index = 0;
  }

  static MpqStore& instance() {
    static MpqStore instance;
    return instance;
  }
  /** Allocate an mpq. */
  mpq_ptr alloc() {
    mpq_link_t* obj = free_list;

    if (obj != nullptr) {
      free_list = obj->next;
      // sanity check: when returned it should still be nullptr
      obj->next = nullptr;
      return reinterpret_cast<mpq_ptr>(&obj->mpq);
    }

    std::uint32_t i = free_index;
    if (i == 0) {
      bnk.emplace_back();
      i = MPQ_BLOCK_COUNT;
    }

    i--;
    free_index = i;
    obj = bnk.back() + i;

    // only initialize when we give it out, ensuring q has enough room for numerator and denominator with 64 bits
    mpz_init2(mpq_numref(obj->mpq), 64);
    mpz_init2(mpq_denref(obj->mpq), 64);
    mpz_set_ui(mpq_denref(obj->mpq), 1UL);

    obj->next = nullptr;  // sanity check: when returned it should still be nullptr

    return reinterpret_cast<mpq_ptr>(&obj->mpq);  // same as obj
  }

  bool free(mpq_ptr mpq) {
    mpq_link_t* const obj = reinterpret_cast<mpq_link_t*>(mpq);
    // sanity check: it's being returned; it should still be nullptr
    assert(obj->next == nullptr);
    obj->next = free_list;
    free_list = obj;
    return true;
  }
  std::list<mpq_link_t[MPQ_BLOCK_COUNT]> bnk;  ///< First block in the bank list
  mpq_link_t* free_list;                       ///< List of free mpq_links
  std::uint32_t free_index;                    ///< Index of last allocated mpq_link in first block
};
}  // namespace simplified

template <class MpqStore>
void alloc(benchmark::State& state) {
  MpqStore store{};
  for (auto _ : state) {
    for (std::int64_t i = 0; i < state.range(0); ++i) {
      benchmark::DoNotOptimize(store.alloc());
    }
  }
  state.SetComplexityN(state.range(0));
}

template <class MpqStore>
void alloc_and_then_free(benchmark::State& state) {
  std::vector<mpq_ptr> allocated;
  allocated.reserve(state.range(0));
  MpqStore store{};
  for (auto _ : state) {
    for (std::int64_t i = 0; i < state.range(0); ++i) {
      allocated.push_back(store.alloc());
    }
    for (std::int64_t i = 0; i < state.range(0); ++i) {
      benchmark::DoNotOptimize(store.free(allocated[i]));
    }
  }
  state.SetComplexityN(state.range(0));
}

template <class MpqStore>
void alloc_and_free_intertwined(benchmark::State& state) {
  MpqStore store{};
  for (auto _ : state) {
    for (std::int64_t i = 0; i < state.range(0); ++i) {
      benchmark::DoNotOptimize(store.free(store.alloc()));
    }
  }
  state.SetComplexityN(state.range(0));
}

BENCHMARK(alloc<opensmt::MpqStore>)->Range(1, std::int64_t{1} << 24)->Complexity(benchmark::oN);
BENCHMARK(alloc<Yices::MpqStore>)->Range(1, std::int64_t{1} << 24)->Complexity(benchmark::oN);
BENCHMARK(alloc<simplified::MpqStore>)->Range(1, std::int64_t{1} << 24)->Complexity(benchmark::oN);

BENCHMARK(alloc_and_then_free<opensmt::MpqStore>)->Range(1, std::int64_t{1} << 24)->Complexity(benchmark::oN);
BENCHMARK(alloc_and_then_free<Yices::MpqStore>)->Range(1, std::int64_t{1} << 24)->Complexity(benchmark::oN);
BENCHMARK(alloc_and_then_free<simplified::MpqStore>)->Range(1, std::int64_t{1} << 24)->Complexity(benchmark::oN);

BENCHMARK(alloc_and_free_intertwined<opensmt::MpqStore>)->Range(1, std::int64_t{1} << 24)->Complexity(benchmark::oN);
BENCHMARK(alloc_and_free_intertwined<Yices::MpqStore>)->Range(1, std::int64_t{1} << 24)->Complexity(benchmark::oN);
BENCHMARK(alloc_and_free_intertwined<simplified::MpqStore>)->Range(1, std::int64_t{1} << 24)->Complexity(benchmark::oN);
