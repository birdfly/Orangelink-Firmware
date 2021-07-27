#ifndef __4B6B_H__
#define __4B6B_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Encode bytes using 4b/6b encoding.
// Encoding n bytes produces 3 * (n / 2) + 2 * (n % 2) output bytes.
// Return number of bytes written to dst.

uint16_t encode_4b6b(const uint8_t *src, uint8_t *dst, uint16_t len);

// Decode bytes using 4b/6b encoding.
// Decoding n bytes produces 2 * (n / 3) + (n % 3) / 2 output bytes.
// Return number of bytes written to dst if successful,
// or -1 if invalid input was encountered.

uint16_t decode_4b6b(const uint8_t *src, uint8_t *dst, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* _4B6B_H */
