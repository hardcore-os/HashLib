#ifndef HASH_HASH_H_
#define HASH_HASH_H_
#include <stdint.h>
#include <algorithm>
#include <memory>

using namespace std;

namespace hard_core {
class HashFunc final {
 public:
  //近似MurMurhash
  uint32_t SimMurMurHash(const char *data, uint32_t len);
  uint32_t MurMurHash2(const char *data, uint32_t len);
  uint32_t PJW(const char *data, uint32_t len);
  uint32_t DJB(const char *data, uint32_t len);
  uint32_t SipHash(const uint8_t *data, uint32_t inlen, const uint8_t *k);
  uint32_t SipHashNoCase(const uint8_t *data, uint32_t inlen, const uint8_t *k);
  uint32_t CalcNrHash(const char *data, uint32_t len);
  uint32_t DEKHash(const char *data, uint32_t len);
  uint32_t FNVHash(const char *data, uint32_t len);
  uint32_t DJB2Hash(const char *data, uint32_t len);
  uint32_t APHash(const char *data, uint32_t len);
  uint32_t BKDRHash(const char *data, uint32_t len);
  uint32_t ELFHash(const char *data, uint32_t len);
  uint32_t JSHash(const char *data, uint32_t len);
  uint32_t RSHash(const char *data, uint32_t len);
  uint32_t SDBMHash(const char *data, uint32_t len);
  uint32_t BPHash(const char *data, uint32_t len);
};
}  // namespace hard_core
#endif
