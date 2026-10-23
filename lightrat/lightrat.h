/**
 * @author TendTo (https://github.com/TendTo)
 * @copyright 2026 lightrat
 * @licence Apache License 2.0
 * @file
 * lightrat struct.
 */
#pragma once

#include <gmp.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * A light rational number is a compact representation of a rational number.
 * It can represent rational numbers whose numerator and denominator fit in 32-bit values.
 * For larger rational numbers, it uses a pointer to a GMP rational value.
 * The least significant bit of the pointer is ensured to be 0, hence the denominator is shifted left by 1 bit.
 */
typedef struct {
#ifdef LIGHTRAT_WORDS_BIGENDIAN
  int32_t num;
  uint32_t den;
#else
  uint32_t den;
  int32_t num;
#endif
} lightrat_32_s;

/**
 * Pointer to a GMP rational value.
 * The least significant bit is set to 1 to indicate that this is a pointer to a GMP rational value.
 * This technique is called [tagged pointer](https://en.wikipedia.org/wiki/Tagged_pointer).
 */
typedef struct {
  intptr_t gmp;
} lightrat_gmp_s;

/**
 * The internal data of a light rational is a union of size 64 bits.
 * If the least bit is 1 it represents a pointer to a gmp number.
 * If the least bit is zero it is a struct consisting of a 32-bit signed numerator and a 31-bit unsigned denominator.
 */
typedef union {
  lightrat_32_s s;   ///< 32-bit representation.
  lightrat_gmp_s p;  ///< Pointer to GMP representation.
} lightrat_u;

typedef lightrat_u lightrat_t[1];  ///< A convenient type for a light rational number.
typedef lightrat_u* lightrat_ptr;  ///< A pointer to a light rational number.

/**
 * Check if a light rational number is uninitialized (i.e., its internal data is all zeros).
 * @param rat The light rational number to check.
 * @return true if the light rational number is uninitialized.
 * @return false if the light rational number is initialized.
 */
extern bool lightrat_is_nan(const lightrat_t rat);
/**
 * Initialize a light rational number to 0/1.
 * @param rat The light rational number to initialize.
 */
extern void lightrat_init(lightrat_t rat);
/**
 * Clear a light rational number, freeing any allocated memory if it is represented with a GMP rational value.
 * The number is reset to 0/1.
 * @param rat The light rational number to clear.
 */
extern void lightrat_clear(lightrat_t rat);
/**
 * Check if a light rational number is represented with a GMP rational value.
 * @param rat The light rational number to check.
 * @return true if the light rational number is represented with a GMP rational value.
 * @return false if the light rational number is represented with a 32-bit signed numerator and a 31-bit unsigned
 * denominator.
 */
extern bool lightrat_is_gmp(const lightrat_t rat);
/**
 * Check if a light rational number is represented with a 32-bit signed numerator and a 31-bit unsigned denominator.
 * @param rat The light rational number to check.
 * @return true if the light rational number is represented with a 32-bit signed numerator and a 31-bit unsigned
 * denominator.
 * @return false if the light rational number is represented with a GMP rational value.
 */
extern bool lightrat_is_rat32(const lightrat_t rat);
/**
 * Check if a light rational number is infinity (i.e., `rat == inf`).
 * @param rat The light rational number to check.
 * @return true if the light rational number is infinity.
 * @return false if the light rational number is not infinity.
 */
extern bool lightrat_is_infinity(const lightrat_t rat);
/**
 * Check if a light rational number is negative infinity (i.e., `rat == -inf`).
 * @param rat The light rational number to check.
 * @return true if the light rational number is negative infinity.
 * @return false if the light rational number is not negative infinity.
 */
extern bool lightrat_is_ninfinity(const lightrat_t rat);
/**
 * Check if a light rational number is finite (not infinity or negative infinity).
 * @param rat The light rational number to check.
 * @return true if the light rational number is finite.
 * @return false if the light rational number is infinity or negative infinity.
 */
extern bool lightrat_is_finite(const lightrat_t rat);
/**
 * Check if a light rational number is zero (i.e., `rat == 0`).
 * @param rat The light rational number to check.
 * @return true if the light rational number is zero.
 * @return false if the light rational number is not zero.
 */
extern bool lightrat_is_zero(const lightrat_t rat);
/**
 * Check if a light rational number is positive (i.e., `rat > 0`).
 * @param rat The light rational number to check.
 * @return true if the light rational number is positive.
 * @return false if the light rational number is negative or zero.
 */
extern bool lightrat_is_pos(const lightrat_t rat);
/**
 * Set the value of a light rational number to another light rational number.
 * @param dst The light rational number to set.
 * @param src The light rational number to copy from.
 */
extern void lightrat_set(lightrat_t dst, const lightrat_t src);
/**
 * Set the value of a light rational number to a pair of integers.
 * @pre The denominator must be non-zero.
 * @param rat The light rational number to set.
 * @param num The numerator of the rational number.
 * @param den The denominator of the rational number.
 */
extern void lightrat_set_si(lightrat_t rat, int32_t num, uint32_t den);
/**
 * Set the value of a light rational number to a GMP rational value.
 * @pre The GMP rational value must be initialized and canonicalized.
 * @param rat The light rational number to set.
 * @param gmp The GMP rational value to copy from.
 */
extern void lightrat_set_gmp(lightrat_t rat, const mpq_t gmp);
/**
 * Set the value of a light rational number to a double.
 * @param rat The light rational number to set.
 * @param d The double to set the rational number to.
 */
extern void lightrat_set_d(lightrat_t rat, double d);
/**
 * Set the value of a light rational number to the infinity sentinel.
 * @param rat The light rational number to set.
 */
extern void lightrat_set_infinity(lightrat_t rat);
/**
 * Set the value of a light rational number to the negative infinity sentinel.
 * @param rat The light rational number to set.
 */
extern void lightrat_set_ninfinity(lightrat_t rat);
/**
 * Get the numerator of a light rational number.
 * @pre `lightrat_is_rat32(rat)`
 * @param rat The light rational number to get the numerator from.
 * @return The numerator of the light rational number.
 */
extern int32_t lightrat_get_num(const lightrat_t rat);
/**
 * Get the denominator of a light rational number.
 * @pre `lightrat_is_rat32(rat)`
 * @param rat The light rational number to get the denominator from.
 * @return The denominator of the light rational number.
 */
extern uint32_t lightrat_get_den(const lightrat_t rat);
/**
 * Get the GMP rational value from a light rational number.
 * @pre `lightrat_is_gmp(rat)`
 * @param rat The light rational number to get the GMP rational value from.
 * @return A pointer to the GMP rational value.
 */
extern mpq_ptr lightrat_get_gmp(const lightrat_t rat);
/**
 * Convert a light rational number to a GMP rational value.
 * @pre `lightrat_is_finite(rat)`
 * @param rat The light rational number to convert.
 * @param gmp The GMP rational value to store the result in.
 */
extern void lightrat_to_gmp(const lightrat_t rat, mpq_t gmp);
/**
 * Convert a light rational number to a double.
 * @param rat The light rational number to convert.
 * @return The double representation of the light rational number.
 */
extern double lightrat_to_d(const lightrat_t rat);
/**
 * Compare two light rational numbers.
 * @param rat The first light rational number to compare.
 * @param other The second light rational number to compare.
 * @return a negative value if `rat < other`.
 * @return 0 if `rat == other`.
 * @return a positive value if `rat > other`.
 */
extern int lightrat_compare(const lightrat_t rat, const lightrat_t other);
/**
 * Compare a light rational number with a GMP rational value.
 * @pre `gmp` must be initialized and canonicalized.
 * @param rat The first light rational number to compare.
 * @param gmp The GMP rational value to compare.
 * @return a negative value if `rat < gmp`.
 * @return 0 if `rat == gmp`.
 * @return a positive value if `rat > gmp`.
 */
extern int lightrat_compare_gmp(const lightrat_t rat, const mpq_t gmp);
/**
 * Compare a light rational number with a pair of signed integers.
 * @pre `den` must be non-zero.
 * @param rat The light rational number to compare.
 * @param num The numerator of the second value to compare.
 * @param den The denominator of the second value to compare.
 * @return a negative value if `rat < num/den`.
 * @return 0 if `rat == num/den`.
 * @return a positive value if `rat > num/den`.
 */
extern int lightrat_compare_si(const lightrat_t rat, int32_t num, uint32_t den);
/**
 * Compare a light rational number with a double.
 * @param rat The light rational number to compare.
 * @param d The double to compare.
 * @return a negative value if `rat < d`.
 * @return 0 if `rat == d`.
 * @return a positive value if `rat > d`.
 */
extern int lightrat_compare_d(const lightrat_t rat, double d);
/**
 * Convert a light rational number to a string.
 * @note The returned string is allocated with `malloc` and must be freed by the caller.
 * @param rat The light rational number to convert.
 * @return A string representation of the light rational number.
 */
extern char* lightrat_to_str(const lightrat_t rat);
/**
 * Invert a light rational number.
 * @pre `lightrat_is_finite(rat)` and `!lightrat_is_zero(rat)`
 * @param rat The light rational number to invert.
 */
extern void lightrat_invert(lightrat_t rat);
/**
 * Negate a light rational number.
 * @param rat The light rational number to negate.
 */
extern void lightrat_negate(lightrat_t rat);
/**
 * Add two light rational numbers.
 * The result is stored in `dst`.
 * @param dst The first light rational number to add.
 * @param src The second light rational number to add.
 */
extern void lightrat_add(lightrat_t dst, const lightrat_t src);
/**
 * Subtract two light rational numbers.
 * The result is stored in `dst`.
 * @param dst The first light rational number to subtract from.
 * @param src The second light rational number to subtract.
 */
extern void lightrat_sub(lightrat_t dst, const lightrat_t src);
/**
 * Multiply two light rational numbers.
 * The result is stored in `dst`.
 * @param dst The first light rational number to multiply.
 * @param src The second light rational number to multiply.
 */
extern void lightrat_mul(lightrat_t dst, const lightrat_t src);
/**
 * Divide two light rational numbers.
 * The result is stored in `dst`.
 * @pre `!lightrat_is_zero(src)`.
 * @param dst The first light rational number to divide.
 * @param src The second light rational number to divide by.
 */
extern void lightrat_div(lightrat_t dst, const lightrat_t src);
/**
 * Add a GMP rational value to a light rational number.
 * The result is stored in `rat`.
 * @param rat The light rational number to store the result in.
 * @param gmp The GMP rational value to add.
 */
extern void lightrat_add_gmp(lightrat_t rat, const mpq_t gmp);
/**
 * Subtract a GMP rational value from a light rational number.
 * The result is stored in `rat`.
 * @param rat The light rational number to store the result in.
 * @param gmp The GMP rational value to subtract.
 */
extern void lightrat_sub_gmp(lightrat_t rat, const mpq_t gmp);
/**
 * Multiply a light rational number by a GMP rational value.
 * The result is stored in `rat`.
 * @param rat The light rational number to store the result in.
 * @param gmp The GMP rational value to multiply by.
 */
extern void lightrat_mul_gmp(lightrat_t rat, const mpq_t gmp);
/**
 * Divide a light rational number by a GMP rational value.
 * The result is stored in `rat`.
 * @pre `gmp` is not zero.
 * @param rat The light rational number to store the result in.
 * @param gmp The GMP rational value to divide by.
 */
extern void lightrat_div_gmp(lightrat_t rat, const mpq_t gmp);
/**
 * Add a pair of signed integers to a light rational number.
 * The result is stored in `rat`.
 * @param rat The light rational number to store the result in.
 * @param num The numerator of the pair of signed integers to add.
 * @param den The denominator of the pair of signed integers to add.
 */
extern void lightrat_add_si(lightrat_t rat, int32_t num, uint32_t den);
/**
 * Subtract a pair of signed integers from a light rational number.
 * The result is stored in `rat`.
 * @param rat The light rational number to store the result in.
 * @param num The numerator of the pair of signed integers to subtract.
 * @param den The denominator of the pair of signed integers to subtract.
 */
extern void lightrat_sub_si(lightrat_t rat, int32_t num, uint32_t den);
/**
 * Multiply a light rational number by a pair of signed integers.
 * The result is stored in `rat`.
 * @param rat The light rational number to store the result in.
 * @param num The numerator of the pair of signed integers to multiply by.
 * @param den The denominator of the pair of signed integers to multiply by.
 */
extern void lightrat_mul_si(lightrat_t rat, int32_t num, uint32_t den);
/**
 * Divide a light rational number by a pair of signed integers.
 * The result is stored in `rat`.
 * @pre `num` is not zero.
 * @param rat The light rational number to store the result in.
 * @param num The numerator of the pair of signed integers to divide by.
 * @param den The denominator of the pair of signed integers to divide by.
 */
extern void lightrat_div_si(lightrat_t rat, int32_t num, uint32_t den);
/**
 * Compute the hash of a light rational number.
 * @param rat The light rational number to compute the hash of.
 * @return The hash of the light rational number.
 */
extern size_t lightrat_hash(const lightrat_t rat);

#ifdef __cplusplus
}
#endif
