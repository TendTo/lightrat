/**
 * @author TendTo (https://github.com/TendTo)
 * @copyright 2026 lightrat
 * @licence Apache License 2.0
 * @file
 */
#include "lightrat/lightrat.h"

#include <cstdio>
#include <string>

#include "lightrat/Rational.h"

using Rational =
#if defined(LIGHTRAT_INFINITY) && defined(LIGHTRAT_NAN)
    lightrat::Rational<true, true>;
#elif defined(LIGHTRAT_INFINITY)
    lightrat::Rational<true, false>;
#elif defined(LIGHTRAT_NAN)
    lightrat::Rational<false, true>;
#else
    lightrat::Rational<false, false>;
#endif

namespace {
Rational* as_light_rat(lightrat_t rat) { return reinterpret_cast<Rational*>(rat); }
const Rational* as_light_rat(const lightrat_t rat) { return reinterpret_cast<const Rational*>(rat); }
}  // namespace

extern "C" bool lightrat_is_nan(const lightrat_t rat) { return as_light_rat(rat)->is_nan(); }

extern "C" void lightrat_init(lightrat_t rat) { new (as_light_rat(rat)) Rational; }

extern "C" void lightrat_clear(lightrat_t rat) { as_light_rat(rat)->clear(); }

extern "C" bool lightrat_is_gmp(const lightrat_t rat) { return as_light_rat(rat)->is_ratgmp(); }

extern "C" bool lightrat_is_rat32(const lightrat_t rat) { return as_light_rat(rat)->is_rat32(); }

extern "C" bool lightrat_is_infinity(const lightrat_t rat) { return as_light_rat(rat)->is_infinity(); }

extern "C" bool lightrat_is_ninfinity(const lightrat_t rat) { return as_light_rat(rat)->is_ninfinity(); }

extern "C" bool lightrat_is_finite(const lightrat_t rat) { return as_light_rat(rat)->is_finite(); }

extern "C" bool lightrat_is_zero(const lightrat_t rat) { return as_light_rat(rat)->is_zero(); }

extern "C" bool lightrat_is_pos(const lightrat_t rat) { return as_light_rat(rat)->is_pos(); }

extern "C" void lightrat_set(lightrat_t dst, const lightrat_t src) { *as_light_rat(dst) = *as_light_rat(src); }

extern "C" void lightrat_set_si(lightrat_t rat, const int32_t num, const uint32_t den) {
  as_light_rat(rat)->set(num, den);
}

extern "C" void lightrat_set_gmp(lightrat_t rat, const mpq_t gmp) { *as_light_rat(rat) = gmp; }

extern "C" void lightrat_set_d(lightrat_t rat, const double d) { *as_light_rat(rat) = d; }

extern "C" void lightrat_set_infinity(lightrat_t rat) { as_light_rat(rat)->set_infinity(); }

extern "C" void lightrat_set_ninfinity(lightrat_t rat) { as_light_rat(rat)->set_ninfinity(); }

extern "C" double lightrat_to_d(const lightrat_t rat) { return static_cast<double>(*as_light_rat(rat)); }

extern "C" int lightrat_compare(const lightrat_t rat, const lightrat_t other) {
  return as_light_rat(rat)->compare(*as_light_rat(other));
}

extern "C" int lightrat_compare_gmp(const lightrat_t rat, const mpq_t gmp) { return as_light_rat(rat)->compare(gmp); }

extern "C" int lightrat_compare_si(const lightrat_t rat, const int32_t num, const uint32_t den) {
  return as_light_rat(rat)->compare(Rational{num, den});
}

extern "C" int lightrat_compare_d(const lightrat_t rat, const double d) {
  return as_light_rat(rat)->compare(Rational{d});
}

extern "C" void lightrat_to_gmp(const lightrat_t rat, mpq_t gmp) {
  const Rational* value = as_light_rat(rat);
  assert(value->is_finite());
  if (value->is_rat32()) {
    mpq_set_si(gmp, value->num(), value->den());
    mpq_canonicalize(gmp);
  } else {
    mpq_set(gmp, value->get_gmp());
  }
}

extern "C" int32_t lightrat_get_num(const lightrat_t rat) { return as_light_rat(rat)->num(); }

extern "C" uint32_t lightrat_get_den(const lightrat_t rat) { return as_light_rat(rat)->den(); }

extern "C" mpq_ptr lightrat_get_gmp(const lightrat_t rat) { return as_light_rat(rat)->get_gmp(); }

extern "C" char* lightrat_to_str(const lightrat_t rat) {
  const std::string string = as_light_rat(rat)->str();
  char* const str = static_cast<char*>(malloc(string.size() + 1));
  if (str == nullptr) return nullptr;
  snprintf(str, string.size() + 1, "%s", string.c_str());
  return str;
}

extern "C" void lightrat_invert(lightrat_t rat) { as_light_rat(rat)->invert(); }

extern "C" void lightrat_negate(lightrat_t rat) { as_light_rat(rat)->negate(); }

extern "C" void lightrat_add(lightrat_t dst, const lightrat_t src) { *as_light_rat(dst) += *as_light_rat(src); }

extern "C" void lightrat_add_gmp(lightrat_t rat, const mpq_t gmp) { *as_light_rat(rat) += Rational{gmp}; }

extern "C" void lightrat_add_si(lightrat_t rat, const int32_t num, const uint32_t den) {
  *as_light_rat(rat) += Rational{num, den};
}

extern "C" void lightrat_sub(lightrat_t dst, const lightrat_t src) { *as_light_rat(dst) -= *as_light_rat(src); }

extern "C" void lightrat_sub_gmp(lightrat_t rat, const mpq_t gmp) { *as_light_rat(rat) -= Rational{gmp}; }

extern "C" void lightrat_sub_si(lightrat_t rat, const int32_t num, const uint32_t den) {
  *as_light_rat(rat) -= Rational{num, den};
}

extern "C" void lightrat_mul(lightrat_t dst, const lightrat_t src) { *as_light_rat(dst) *= *as_light_rat(src); }

extern "C" void lightrat_mul_gmp(lightrat_t rat, const mpq_t gmp) { *as_light_rat(rat) *= Rational{gmp}; }

extern "C" void lightrat_mul_si(lightrat_t rat, const int32_t num, const uint32_t den) {
  *as_light_rat(rat) *= Rational{num, den};
}

extern "C" void lightrat_div(lightrat_t dst, const lightrat_t src) { *as_light_rat(dst) /= *as_light_rat(src); }

extern "C" void lightrat_div_gmp(lightrat_t rat, const mpq_t gmp) { *as_light_rat(rat) /= Rational{gmp}; }

extern "C" void lightrat_div_si(lightrat_t rat, const int32_t num, const uint32_t den) {
  *as_light_rat(rat) /= Rational{num, den};
}

extern "C" size_t lightrat_hash(const lightrat_t rat) { return as_light_rat(rat)->hash(); }
