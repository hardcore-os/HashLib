#include "hash.h"

#include "util.h"

namespace hard_core {
namespace {

/* Fast tolower() alike function that does not care about locale
 * but just returns a-z instead of A-Z. */
int siptlw(int c) {
  if (c >= 'A' && c <= 'Z') {
    return c + ('a' - 'A');
  } else {
    return c;
  }
}

/* Test of the CPU is Little Endian and supports not aligned accesses.
 * Two interesting conditions to speedup the function that happen to be
 * in most of x86 servers. */
#if defined(__X86_64__) || defined(__x86_64__) || defined(__i386__) || \
    defined(__aarch64__) || defined(__arm64__)
#define UNALIGNED_LE_CPU
#endif

#define ROTL(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))

#define U32TO8_LE(p, v)          \
  (p)[0] = (uint8_t)((v));       \
  (p)[1] = (uint8_t)((v) >> 8);  \
  (p)[2] = (uint8_t)((v) >> 16); \
  (p)[3] = (uint8_t)((v) >> 24);

#define U64TO8_LE(p, v)            \
  U32TO8_LE((p), (uint32_t)((v))); \
  U32TO8_LE((p) + 4, (uint32_t)((v) >> 32));

#ifdef UNALIGNED_LE_CPU
#define U8TO64_LE(p) (*((uint64_t *)(p)))
#else
#define U8TO64_LE(p)                                         \
  (((uint64_t)((p)[0])) | ((uint64_t)((p)[1]) << 8) |        \
   ((uint64_t)((p)[2]) << 16) | ((uint64_t)((p)[3]) << 24) | \
   ((uint64_t)((p)[4]) << 32) | ((uint64_t)((p)[5]) << 40) | \
   ((uint64_t)((p)[6]) << 48) | ((uint64_t)((p)[7]) << 56))
#endif

#define U8TO64_LE_NOCASE(p)                                                  \
  (((uint64_t)(siptlw((p)[0]))) | ((uint64_t)(siptlw((p)[1])) << 8) |        \
   ((uint64_t)(siptlw((p)[2])) << 16) | ((uint64_t)(siptlw((p)[3])) << 24) | \
   ((uint64_t)(siptlw((p)[4])) << 32) | ((uint64_t)(siptlw((p)[5])) << 40) | \
   ((uint64_t)(siptlw((p)[6])) << 48) | ((uint64_t)(siptlw((p)[7])) << 56))

#define SIPROUND       \
  do {                 \
    v0 += v1;          \
    v1 = ROTL(v1, 13); \
    v1 ^= v0;          \
    v0 = ROTL(v0, 32); \
    v2 += v3;          \
    v3 = ROTL(v3, 16); \
    v3 ^= v2;          \
    v0 += v3;          \
    v3 = ROTL(v3, 21); \
    v3 ^= v0;          \
    v2 += v1;          \
    v1 = ROTL(v1, 17); \
    v1 ^= v2;          \
    v2 = ROTL(v2, 32); \
  } while (0)
}  // namespace

uint32_t HashFunc::SimMurMurHash(const char *data, uint32_t len) {
  uint32_t seed = 0xbc9f1d34;
  // Similar to murmur hash
  const uint32_t m = 0xc6a4a793;
  const uint32_t r = 24;
  const char *limit = data + len;
  uint32_t h = seed ^ (len * m);

  // Pick up four bytes at a time
  while (data + 4 <= limit) {
    uint32_t w = DecodeFixed32(data);
    data += 4;
    h += w;
    h *= m;
    h ^= (h >> 16);
  }
  switch (limit - data) {
    case 3:
      h += static_cast<uint8_t>(data[2]) << 16;
    case 2:
      h += static_cast<uint8_t>(data[1]) << 8;
    case 1:
      h += static_cast<uint8_t>(data[0]);
      h *= m;
      h ^= (h >> r);
      break;
  }
  return h;
}

uint32_t HashFunc::MurMurHash2(const char *data, uint32_t len) {
  uint32_t seed = 5381;
  /* 'm' and 'r' are mixing constants generated offline.
     They're not really 'magic', they just happen to work well.  */
  const uint32_t m = 0x5bd1e995;
  const int32_t r = 24;

  /* Initialize the hash to a 'random' value */
  uint32_t h = seed ^ len;

  while (len >= 4) {
    uint32_t k = *(uint32_t *)data;

    k *= m;
    k ^= k >> r;
    k *= m;

    h *= m;
    h ^= k;

    data += 4;
    len -= 4;
  }

  /* Handle the last few bytes of the input array  */
  switch (len) {
    case 3:
      h ^= data[2] << 16;
    case 2:
      h ^= data[1] << 8;
    case 1:
      h ^= data[0];
      h *= m;
  };

  /* Do a few final mixes of the hash to ensure the last few
   * bytes are well-incorporated. */
  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;

  return h;
}

uint32_t HashFunc::DJB(const char *data, uint32_t len) {
  uint32_t seed = 5381;
  uint32_t h = seed;
  while (len--) h = ((h << 5) + h) + (tolower(*data++)); /* hash * 33 + c */
  return h;
}

uint32_t HashFunc::SipHash(const uint8_t *in, uint32_t inlen,
                           const uint8_t *k) {
#ifndef UNALIGNED_LE_CPU
  uint64_t hash;
  uint8_t *out = (uint8_t *)&hash;
#endif
  uint64_t v0 = 0x736f6d6570736575ULL;
  uint64_t v1 = 0x646f72616e646f6dULL;
  uint64_t v2 = 0x6c7967656e657261ULL;
  uint64_t v3 = 0x7465646279746573ULL;
  uint64_t k0 = U8TO64_LE(k);
  uint64_t k1 = U8TO64_LE(k + 8);
  uint64_t m;
  const uint8_t *end = in + inlen - (inlen % sizeof(uint64_t));
  const int left = inlen & 7;
  uint64_t b = ((uint64_t)inlen) << 56;
  v3 ^= k1;
  v2 ^= k0;
  v1 ^= k1;
  v0 ^= k0;

  for (; in != end; in += 8) {
    m = U8TO64_LE(in);
    v3 ^= m;

    SIPROUND;

    v0 ^= m;
  }

  switch (left) {
    case 7:
      b |= ((uint64_t)in[6]) << 48; /* fall-thru */
    case 6:
      b |= ((uint64_t)in[5]) << 40; /* fall-thru */
    case 5:
      b |= ((uint64_t)in[4]) << 32; /* fall-thru */
    case 4:
      b |= ((uint64_t)in[3]) << 24; /* fall-thru */
    case 3:
      b |= ((uint64_t)in[2]) << 16; /* fall-thru */
    case 2:
      b |= ((uint64_t)in[1]) << 8; /* fall-thru */
    case 1:
      b |= ((uint64_t)in[0]);
      break;
    case 0:
      break;
  }

  v3 ^= b;

  SIPROUND;

  v0 ^= b;
  v2 ^= 0xff;

  SIPROUND;
  SIPROUND;

  b = v0 ^ v1 ^ v2 ^ v3;
#ifndef UNALIGNED_LE_CPU
  U64TO8_LE(out, b);
  return hash;
#else
  return b;
#endif
}

uint32_t HashFunc::SipHashNoCase(const uint8_t *in, uint32_t inlen,
                                 const uint8_t *k) {
#ifndef UNALIGNED_LE_CPU
  uint64_t hash;
  uint8_t *out = (uint8_t *)&hash;
#endif
  uint64_t v0 = 0x736f6d6570736575ULL;
  uint64_t v1 = 0x646f72616e646f6dULL;
  uint64_t v2 = 0x6c7967656e657261ULL;
  uint64_t v3 = 0x7465646279746573ULL;
  uint64_t k0 = U8TO64_LE(k);
  uint64_t k1 = U8TO64_LE(k + 8);
  uint64_t m;
  const uint8_t *end = in + inlen - (inlen % sizeof(uint64_t));
  const int left = inlen & 7;
  uint64_t b = ((uint64_t)inlen) << 56;
  v3 ^= k1;
  v2 ^= k0;
  v1 ^= k1;
  v0 ^= k0;

  for (; in != end; in += 8) {
    m = U8TO64_LE_NOCASE(in);
    v3 ^= m;

    SIPROUND;

    v0 ^= m;
  }

  switch (left) {
    case 7:
      b |= ((uint64_t)siptlw(in[6])) << 48; /* fall-thru */
    case 6:
      b |= ((uint64_t)siptlw(in[5])) << 40; /* fall-thru */
    case 5:
      b |= ((uint64_t)siptlw(in[4])) << 32; /* fall-thru */
    case 4:
      b |= ((uint64_t)siptlw(in[3])) << 24; /* fall-thru */
    case 3:
      b |= ((uint64_t)siptlw(in[2])) << 16; /* fall-thru */
    case 2:
      b |= ((uint64_t)siptlw(in[1])) << 8; /* fall-thru */
    case 1:
      b |= ((uint64_t)siptlw(in[0]));
      break;
    case 0:
      break;
  }

  v3 ^= b;

  SIPROUND;

  v0 ^= b;
  v2 ^= 0xff;

  SIPROUND;
  SIPROUND;

  b = v0 ^ v1 ^ v2 ^ v3;
#ifndef UNALIGNED_LE_CPU
  U64TO8_LE(out, b);
  return hash;
#else
  return b;
#endif
}

uint32_t HashFunc::PJW(const char *data, uint32_t len) {
  uint32_t h = 0, g = 0;
  while (len--) {
    h = (h << 4) + *data++;
    if ((g = (h & 0xF0000000))) {
      h = h ^ (g >> 24);
      h = h ^ g;
    }
  }
  return h;
}

uint32_t HashFunc::CalcNrHash(const char *data, uint32_t len) {
  uint32_t h = 1, g = 4;
  while (len--) {
    h ^= (((h & 63) + g) * ((uint)*data++)) + (h << 8);
    g += 3;
  }
  return h;
}

uint32_t HashFunc::SDBMHash(const char *data, uint32_t len) {
  uint32_t h = 0;
  while (len--) {
    h = (*data++) + (h << 6) + (h << 16) - h;
  }
  return h;
}

uint32_t HashFunc::RSHash(const char *data, uint32_t len) {
  static constexpr uint32_t b = 378551;
  uint32_t a = 63689;
  uint32_t hash = 0;

  while (len--) {
    hash = hash * a + (*data++);
    a *= b;
  }

  return (hash & 0x7FFFFFFF);
}

// JS Hash Function
uint32_t HashFunc::JSHash(const char *data, uint32_t len) {
  uint32_t hash = 1315423911;

  while (len--) {
    hash ^= ((hash << 5) + (*data++) + (hash >> 2));
  }

  return (hash & 0x7FFFFFFF);
}

// JS Hash Function
uint32_t HashFunc::ELFHash(const char *data, uint32_t len) {
  uint32_t hash = 0, x = 0;

  while (len--) {
    hash = (hash << 4) + (*data++);
    if ((x = hash & 0xF0000000L) != 0) {
      hash ^= (x >> 24);
      hash &= ~x;
    }
  }

  return (hash & 0x7FFFFFFF);
}

// BKDR Hash Function
uint32_t HashFunc::BKDRHash(const char *data, uint32_t len) {
  uint32_t seed = 131;  // 31 131 1313 13131 131313 etc..
  uint32_t hash = 0;

  while (len--) {
    hash = hash * seed + (*data++);
  }

  return (hash & 0x7FFFFFFF);
}

// BKDR Hash Function
uint32_t HashFunc::APHash(const char *data, uint32_t len) {
  uint32_t hash = 0;
  for (uint32_t idx = 0; idx < len; idx++) {
    if ((idx & 1) == 0) {
      hash ^= ((hash << 7) ^ (*data++) ^ (hash >> 3));
    } else {
      hash ^= (~((hash << 11) ^ (*data++) ^ (hash >> 5)));
    }
  }

  return (hash & 0x7FFFFFFF);
}

// BKDR Hash Function
uint32_t HashFunc::DJB2Hash(const char *data, uint32_t len) {
  uint32_t seed = 5381;
  uint32_t h = seed;
  while (len--) {
    h = h * 33 ^ (*data++); /* hash * 33 + c */
  }
  return h;
}

uint32_t HashFunc::FNVHash(const char *data, uint32_t len) {
  uint32_t hash = 2166136261;
  while (len--) {
    hash *= 16777619;
    hash ^= (*data++);
  }
  return hash;
}

uint32_t HashFunc::DEKHash(const char *data, uint32_t len) {
  uint32_t hash = 1315423911;
  while (len--) {
    hash = ((hash << 5) ^ (hash >> 27)) ^ (*data++);
  }
  return hash;
}
uint32_t HashFunc::BPHash(const char *data, uint32_t len) {
  uint32_t hash = len;
  while (len--) {
    hash = (hash << 7) ^ (*data++);
  }
  return hash & 0x7FFFFFFF;
}
}  // namespace hard_core
