
#include <memory>
#include <string>
#include <vector>

#include "bloomfilter.h"
namespace hard_core {

class BloomTest final {
 public:
  BloomTest(uint32_t bit_per_key = 10)
      : policy_(std::make_unique<BloomFilterPolicy>(bit_per_key)) {}

  ~BloomTest() {}

  void Reset() {
    keys_.clear();
    filter_.clear();
  }
  void SetData(std::vector<std::string>&& keys) { keys_ = std::move(keys); }
  void SetData(const std::vector<std::string>& keys) { keys_ = (keys); }

  void Add(const std::string& s) { keys_.emplace_back(s); }

  void Build() {
    filter_.clear();
    policy_->CreateFilter(&keys_[0], static_cast<int>(keys_.size()), &filter_);
    keys_.clear();
  }

  size_t FilterSize() const { return filter_.size(); }
  bool Matches(const std::string& s) {
    if (!keys_.empty()) {
      Build();
    }
    return policy_->KeyMayMatch(s, filter_);
  }

  double FalsePositiveRate(const std::vector<std::string>& data) {
    double result = 0;
    for (const auto& item : data) {
      if (Matches(item)) {
        result++;
      }
    }
    return result / 10000.0;
  }
  double FalsePositiveRate(const std::string& data) {
    double result = 0;
    {
      if (Matches(data)) {
        result++;
      }
    }
    return result / 10000.0;
  }

 private:
  std::unique_ptr<FilterPolicy> policy_;
  std::string filter_;
  std::vector<std::string> keys_;
};
}  // namespace hard_core
