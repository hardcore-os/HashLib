#ifndef HASH_UTIL_H_
#define HASH_UTIL_H_
#include <stdint.h>
#include <string.h>

#include <functional>

namespace hard_core {
//判断是否是小端
inline bool CheckLittleEndian() {
  union {
    short inum = 0x1234;
    char cnum;
  } dummy;
  return dummy.cnum != 0x12;
}
inline uint32_t DecodeFixed32(const char* ptr) {
  if (CheckLittleEndian()) {
    // Load the raw bytes
    uint32_t result = 0;
    memcpy(&result, ptr, sizeof(result));  // gcc optimizes this to a plain load
    return result;
  } else {
    return ((static_cast<uint32_t>(static_cast<unsigned char>(ptr[0]))) |
            (static_cast<uint32_t>(static_cast<unsigned char>(ptr[1])) << 8) |
            (static_cast<uint32_t>(static_cast<unsigned char>(ptr[2])) << 16) |
            (static_cast<uint32_t>(static_cast<unsigned char>(ptr[3])) << 24));
  }
}
template <typename Functor, typename... Args>
typename std::enable_if<
    std::is_member_pointer<typename std::decay<Functor>::type>::value,
    typename std::result_of<Functor && (Args && ...)>::type>::type
invoke(Functor&& f, Args&&... args) {
  return std::mem_fn(f)(std::forward<Args>(args)...);
}

template <typename Functor, typename... Args>
typename std::enable_if<
    !std::is_member_pointer<typename std::decay<Functor>::type>::value,
    typename std::result_of<Functor && (Args && ...)>::type>::type
invoke(Functor&& f, Args&&... args) {
  return std::forward<Functor>(f)(std::forward<Args>(args)...);
}
}  // namespace hard_core
#endif
