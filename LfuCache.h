#pragma once
#include <cstdint>
#include <mutex>
#include <cmath>
#include <vector>
#include <unordered_map>
#include <memory>
#include <thread>

#include "Cachepolicy.h"

template<typename Key, typename Value> class LfuCache;

template<typename Key, typename Value> 
class FreqList
{
private:
    struct Node
    {
        int freq;
        Key key;
        Value value;
        std::shared_ptr<Node> pre;
        std::shared_ptr<Node> next;

        Node()
        :freq(1), pre(nullptr), next(nullptr){}
        Node(Key key, Value value)
        :freq(1), key(key), value(value), pre(nullptr), next(nullptr){}
    };

    using NodePtr=std::shared_ptr<Node>;
    int freq_;
    NodePtr head_;
    NodePtr tail_;
public:
    explicit FreqList(int n) : freq_(n)
    {
        head_ = std::make_shared<Node>();
        tail_ = std::make_shared<Node>();
        head_->next = tail_;
        tail_->pre = head_;
    }
    bool isEmpty() const
    {
      return head_->next == tail_;
    }

    void addNode(NodePtr node) 
    {
        if (!node || !head_ || !tail_) 
            return;

        node->pre = tail_->pre;
        node->next = tail_;
        tail_->pre->next = node;
        tail_->pre = node;
    }

    void removeNode(NodePtr node)
    {
        if (!node || !head_ || !tail_)
            return;
        if (!node->pre || !node->next) 
            return;

        node->pre->next = node->next;
        node->next->pre = node->pre;
        node->pre = nullptr;
        node->next = nullptr;
    }

    NodePtr getFirstNode() const { return head_->next; }
    
    friend class LfuCache<Key, Value>;
};

template <typename Key, typename Value>
class LfuCache : public CachePolicy<Key, Value>
{
public:
    using Node = typename FreqList<Key, Value>::Node;
    using NodePtr = std::shared_ptr<Node>;
    using NodeMap = std::unordered_map<Key, NodePtr>;

    LfuCache(int capacity, int maxAverageNum = 10)
    : capacity_(capacity), minFreq_(INT8_MAX), maxAverageNum_(maxAverageNum),
      curAverageNum_(0), curTotalNum_(0) 
    {}

    ~LfuCache() override = default;

    void put(Key key, Value value) override
    {
        if (capacity_ == 0) {
            return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        auto it= nodeMap_.find(key);
        if (it != nodeMap_.end()) {
            it->second->value = value;
            getInternal(it->second, value);
            return;
        }

        putInternal(key, value);
    }
    bool get(Key key, Value& value) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nodeMap_.find(key);
        if (it != nodeMap_.end()) {
            getInternal(it->second, value);
            return true;
        }
        return false;
    }

    Value get(Key key) override
    {
        Value value;
        get(key, value);
        return value;
    }

    void purge()
    {
        nodeMap_.clear();
        freqToFreqList_.clear();
    }

private:
    void putInternal(Key key, Value value); // 添加缓存
    void getInternal(NodePtr node, Value& value); // 获取缓存

    void kickOut(); // 移除缓存中的过期数据

    void removeFromFreqList(NodePtr node); // 从频率列表中移除节点
    void addToFreqList(NodePtr node); // 添加到频率列表

    void addFreqNum(); // 增加平均访问等频率
    void decreaseFreqNum(int num); // 减少平均访问等频率
    void handleOverMaxAverageNum(); // 处理当前平均访问频率超过上限的情况
    void updateMinFreq();

private:
    int                                            capacity_; // 缓存容量
    int                                            minFreq_; // 最小访问频次(用于找到最小访问频次结点)
    int                                            maxAverageNum_; // 最大平均访问频次
    int                                            curAverageNum_; // 当前平均访问频次
    int                                            curTotalNum_; // 当前访问所有缓存次数总数 
    std::mutex                                     mutex_; // 互斥锁
    NodeMap                                        nodeMap_; // key 到 缓存节点的映射
    std::unordered_map<int, FreqList<Key, Value>*> freqToFreqList_;// 访问频次到该频次链表的映射
};

template<typename Key, typename Value>
void LfuCache<Key, Value>::getInternal(NodePtr node, Value& value)
{
    value = node->value;
    removeFromFreqList(node);
    node->freq++;
    addToFreqList(node);
    if (node->freq - 1 == minFreq_ && freqToFreqList_[node->freq - 1]->isEmpty())
        minFreq_++;
    addFreqNum();
}

template<typename Key, typename Value>
void LfuCache<Key, Value>::putInternal(Key key, Value value)
{
    if (nodeMap_.size() == capacity_) {
        kickOut();
    }

    NodePtr node = std::make_shared<Node>(key, value);
    nodeMap_[key] = node;
    addToFreqList(node);
    addFreqNum();
    minFreq_ = std::min(minFreq_, 1);
}

template<typename Key, typename Value>
void LfuCache<Key, Value>::kickOut()
{
    NodePtr node = freqToFreqList_[minFreq_]->getFirstNode();
    removeFromFreqList(node);
    nodeMap_.erase(node->key);
    decreaseFreqNum(node->freq);
}

template<typename Key,typename Value>
void LfuCache<Key, Value>::removeFromFreqList(NodePtr node)
{
    if (!node) {
        return;
    }
    auto freq = node->freq;
    freqToFreqList_[freq]->removeNode(node);
}

template<typename Key, typename Value>
void LfuCache<Key, Value>::addToFreqList(NodePtr node)
{
    if (!node) {
        return;
    }
    auto freq = node->freq;
    if (freqToFreqList_.find(freq) == freqToFreqList_.end()) {
        freqToFreqList_[freq] = new FreqList<Key, Value>(freq);
    }

    freqToFreqList_[freq]->addNode(node);
}

template<typename Key, typename Value>
void LfuCache<Key, Value>::addFreqNum() // 增加平均访问等频率
{
    curTotalNum_++;
    if (nodeMap_.empty())
        curAverageNum_ = 0;
    else
        curAverageNum_ = curTotalNum_  / nodeMap_.size();
    if (curAverageNum_ > maxAverageNum_)
    {
        handleOverMaxAverageNum();
    }
}

template<typename Key, typename Value>
void LfuCache<Key, Value>::decreaseFreqNum(int num)
{
    curTotalNum_ -= num;
    if (nodeMap_.empty()) 
        curAverageNum_ = 0;
    else
        curAverageNum_ = curTotalNum_ / nodeMap_.size();
}

template<typename Key, typename Value>
void LfuCache<Key, Value>::handleOverMaxAverageNum()
{
    if (nodeMap_.empty()) {
        return;
    }
    
    // 当前平均访问频次已经超过了最大平均访问频次，所有结点的访问频次- (maxAverageNum_ / 2)
    for (auto it = nodeMap_.begin(); it != nodeMap_.end(); ++it) {
        if (!it->second) {
            return;
        }
        NodePtr node = it->second;
        removeFromFreqList(node);
        node->freq = node->freq - (maxAverageNum_ / 2);
        if (node->freq < 1) {
            node->freq = 1;
        }
        addToFreqList(node);
    }

    updateMinFreq();
}

template<typename Key, typename Value>
void LfuCache<Key, Value>::updateMinFreq()
{
    minFreq_ = INT8_MAX;
    for (const auto& pair : freqToFreqList_) {
        if (pair.second && !pair.second->isEmpty()) {
            minFreq_ = std::min(minFreq_, INT8_MAX);
        }
    }
    if (minFreq_ == INT8_MAX) {
        minFreq_ = 1;
    }
}

template<typename Key, typename Value>
class LfuHashCache 
{
public:
    LfuHashCache(int capacity, size_t sliceNum)
        : capacity_(capacity)
        , sliceNum_(sliceNum > 0 ? sliceNum : std::max<size_t>(1, std::thread::hardware_concurrency()))
    {
        size_t silceSize = std::ceil(capacity / static_cast<double>(sliceNum_));
        for (size_t i = 0; i < sliceNum_; i++) {
            lfuHashCache_.emplace_back(new LfuCache<Key, Value>(silceSize));
        }
    }
    
    void put(Key key, Value value) 
    {
        size_t sliceIndex = Hash(key) % sliceNum_;
        lfuHashCache_[sliceIndex]->put(key, value);
    }

    bool get(Key key, Value& value) 
    {
        size_t sliceIndex = Hash(key) % sliceNum_;
        return lfuHashCache_[sliceIndex]->get(key, value);
    }

    Value get(Key key) 
    {
        Value value{};
        get(key, value);
        return value;
    }
public:
    size_t Hash(Key key) {
        std::hash<Key> hashFunc;  // 确保这里使用了正确的模板类型
        return hashFunc(key);
    }
private:
    int                                    capacity_;
    size_t                                 sliceNum_;
    std::vector<std::unique_ptr<LfuCache<Key, Value>>> lfuHashCache_;
};