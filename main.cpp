#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <functional>

#include "hash.h"
#include "time_watcher.h"
#include "bloomfilter_test.h"

using namespace std;
using namespace hard_core;
HashFunc h;
TimeWatcher time_watcher;
std::vector<std::string> all_datas;
std::function< uint32_t (const char *data, uint32_t len)> funcs;
std::function< uint32_t (const uint8_t *in, uint32_t inlen,
                           const uint8_t *k)> sip_func;
unordered_map<std::string, decltype(funcs)> hash_funcs;
unordered_map<std::string, decltype(sip_func)> sip_hash_funcs;
void FillHashFunc() {
  //填充已有的函数
    hash_funcs["APHash"] = std::bind(&HashFunc::APHash, h, std::placeholders::_1, std::placeholders::_2);
    hash_funcs["BKDRHash"] = std::bind(&HashFunc::BKDRHash, h, std::placeholders::_1, std::placeholders::_2);
    hash_funcs["BPHash"] = std::bind(&HashFunc::BPHash, h, std::placeholders::_1, std::placeholders::_2);
    hash_funcs["DEKHash"] = std::bind(&HashFunc::DEKHash, h, std::placeholders::_1, std::placeholders::_2);
    hash_funcs["DJB"] = std::bind(&HashFunc::DJB, h, std::placeholders::_1, std::placeholders::_2);
    hash_funcs["DJB2Hash"] = std::bind(&HashFunc::DJB2Hash, h, std::placeholders::_1, std::placeholders::_2);
    hash_funcs["ELFHash"] = std::bind(&HashFunc::ELFHash, h, std::placeholders::_1, std::placeholders::_2);
    hash_funcs["MurMurHash2"] = std::bind(&HashFunc::MurMurHash2, h, std::placeholders::_1, std::placeholders::_2);
    hash_funcs["PJW"] = std::bind(&HashFunc::PJW, h, std::placeholders::_1, std::placeholders::_2);
    hash_funcs["JSHash"] = std::bind(&HashFunc::JSHash, h, std::placeholders::_1, std::placeholders::_2);
    hash_funcs["SimMurMurHash"] = std::bind(&HashFunc::SimMurMurHash, h, std::placeholders::_1, std::placeholders::_2);
    hash_funcs["SDBMHash"] = std::bind(&HashFunc::SDBMHash, h, std::placeholders::_1, std::placeholders::_2);
    hash_funcs["RSHash"] = std::bind(&HashFunc::RSHash, h, std::placeholders::_1, std::placeholders::_2);
    hash_funcs["CalcNrHash"] = std::bind(&HashFunc::CalcNrHash, h, std::placeholders::_1, std::placeholders::_2);
    hash_funcs["FNVHash"] = std::bind(&HashFunc::FNVHash, h, std::placeholders::_1, std::placeholders::_2);
    sip_hash_funcs["SipHash"] = std::bind(&HashFunc::SipHash, h, std::placeholders::_1, std::placeholders::_2,std::placeholders::_3);
    sip_hash_funcs["SipHashNoCase"] = std::bind(&HashFunc::SipHashNoCase, h, std::placeholders::_1, std::placeholders::_2,std::placeholders::_3);
}
void ReadData(const std::string& path, std::vector<std::string>&data) {
  std::ifstream ifstream;
    ifstream.open(path);
    std::string line;
    uint64_t size = 0;
    uint64_t min_val = 10000000, max_val = 0;
    while (std::getline(ifstream, line)){
      size+=line.size();
      max_val = std::max(max_val,line.size());
      min_val = std::min(min_val,line.size());
      data.push_back( line);
    } 
    cout<<path<<",size = "<<size<<endl;
    std::random_shuffle(data.begin(), data.end());
    
}
void TestCommonHash() {
  for(const auto& func : hash_funcs) {
    unordered_set<uint32_t> records;
  uint64_t collison = 0;
  uint64_t time_count = 0;
  for(const auto& item : all_datas) {
    time_watcher.Start();
    
    const auto hash = func.second(item.c_str(), item.size());
    time_count +=time_watcher.GetMs();
    if (records.count(hash) > 0) {
      ++collison;
    } else {
      records.insert(hash);
    }
  }
  cout<<func.first<<","<<time_count <<","<<collison <<","<<all_datas.size()<<endl;
  }
  
}

void TestSipHash() {
  for(const auto& func : sip_hash_funcs) {
    unordered_set<uint32_t> records;
  uint64_t collison = 0;
  uint64_t time_count = 0;
  for(const auto& item : all_datas) {
    time_watcher.Start();
    const auto hash = func.second((uint8_t*)item.c_str(), item.size(), (uint8_t*)"1234567812345678");
    time_count +=time_watcher.GetMs();
    if (records.count(hash) > 0) {
      ++collison;
    } else {
      records.insert(hash);
    }
  }
  cout<<func.first<<","<<time_count <<","<<collison <<","<<all_datas.size()<<endl;
  }
  
}
//测试bf
void TestBloomFilter() {
  std::vector<std::string> inputs;
  ReadData("./random_str.data",inputs);
  BloomTest bt(15);
  bt.SetData(inputs);
  
  bt.Build();
  cout<<"filter_size:"<<bt.FilterSize()<<",FalsePositiveRate="<<bt.FalsePositiveRate(all_datas)<<endl;
}
int main() {
  FillHashFunc();
  ReadData("./random_str.data",all_datas);
  TestCommonHash();
  TestSipHash();
  TestBloomFilter();
  return 0;
}
