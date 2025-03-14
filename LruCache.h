#pragma once
#include "Cachepolicy.h"
#include <mutex>
#include <unordered_map>
#include <memory>
#include <vector>
#include <cmath>
#include <algorithm>
#include <thread>

template<typename Key, typename Value>class LruCache;

template<typename Key, typename Value>
class LruNode
{
private:
    Key key_;
    Value value_;
    std::shared_ptr<LruNode<Key, Value>> prev_;
    std::shared_ptr<LruNode<Key, Value>> next_;
    size_t accessCount_;

public:
    LruNode(Key key, Value value) 
        : key_(key)
        , value_(value)
        , accessCount_(1)
        , prev_(nullptr)
        , next_(nullptr)
    {}

    Key getKey() const { return key_; }
    Value getValue() const { return value_; }
    void setValue(Value value) { value_ = value; }
    size_t getAccessCount() const { return accessCount_; }
    void incrementAccessCount() { ++accessCount_; }

    friend class LruCache<Key, Value>;
};

template<typename Key, typename Value>
class LruCache : public CachePolicy<Key, Value>
{
public:
    using LruNodeType = LruNode<Key, Value>;
    using NodePtr = std::shared_ptr<LruNodeType>;
    using NodeMap = std::unordered_map<Key, NodePtr>;
    
    LruCache(int capacity) : capacity_(capacity)
    {
        dummyHead_ = std::make_shared<LruNodeType>(Key(), Value());
        dummyTail_ = std::make_shared<LruNodeType>(Key(), Value());
        dummyHead_->next_ = dummyTail_;
        dummyTail_->prev_ = dummyHead_;
    } 

    ~LruCache() = default;

    void put(Key key, Value value) override
    {
        if (capacity_ <= 0) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nodeMap_.find(key);
        if (it != nodeMap_.end()) {
            updateExistingNode(it->second, value);
            return;
        }
        
        addNewNode(key, value);
    }

    bool get(Key key, Value& value) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nodeMap_.find(key);
        if (it != nodeMap_.end()) {
            moveToMostRecent(it->second);
            value = it->second->getValue();
            return true;
        }
        return false;
    }

    Value get(Key key) override
    {
        Value value{};
        get(key, value);
        return value;
    }

    void remove(Key key) 
    {   
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nodeMap_.find(key);
        if (it != nodeMap_.end())
        {
            removeNode(it->second);
            nodeMap_.erase(it);
        }
    }

private:
void updateExistingNode(NodePtr node, const Value& value) 
    {
        node->setValue(value);
        moveToMostRecent(node);
    }

    void addNewNode(const Key& key, const Value& value) 
    {
        if (nodeMap_.size() >= capacity_) {
            evictLeastRecent();
        }
        NodePtr newNode = std::make_shared<LruNodeType>(key, value);
        insertNode(newNode);
        nodeMap_[key] = newNode;
    }

    // 将该节点移动到最新的位置
    void moveToMostRecent(NodePtr node) 
    {
        removeNode(node);
        insertNode(node);
    }

    void removeNode(NodePtr node) 
    {
        node->prev_->next_ = node->next_;
        node->next_->prev_ = node->prev_;
    }

    // 从尾部插入结点
    void insertNode(NodePtr node) 
    {
        node->next_ = dummyTail_;
        node->prev_ = dummyTail_->prev_;
        dummyTail_->prev_->next_ = node;
        dummyTail_->prev_ = node;
    }

    // 驱逐最近最少访问
    void evictLeastRecent() 
    {
        NodePtr leastRecent = dummyHead_->next_;
        removeNode(leastRecent);
        nodeMap_.erase(leastRecent->getKey());
    }

private:
    int capacity_;
    NodeMap nodeMap_;
    std::mutex mutex_;
    NodePtr dummyHead_;
    NodePtr dummyTail_;
};

// LRU优化：Lru-k版本。 通过继承的方式进行再优化
template<typename Key, typename Value>
class LruKCache : public LruCache<Key, Value>
{
public:
    LruKCache(int capacity, int historyCapacity, int k) 
        : LruCache<Key, Value>(capacity)
        , historyList_(std::make_unique<LruCache<Key,size_t>>(historyCapacity))
        , k_(k)
    {}

    Value get(Key key)
    {
        int historyCount = historyList_->get(key);
        historyList_->put(key, ++historyCount);
        return LruCache<Key, Value>::get(key);
    }

    void put(Key key, Value value)
    {
        if (LruCache<Key, Value>::get(key) != "") {
            LruCache<Key, Value>::put(key, value);
        }

        int historyCount = historyList_->get(key);
        if (historyCount >= k_) {
            historyList_->remove(key);
            LruCache<Key, Value>::put(key, value);
        }
    }

private:
    int                                    k_;
    std::unique_ptr<LruCache<Key, size_t>> historyList_;
};

template<typename Key, typename Value>
class LruHashCache 
{
public:
    LruHashCache(int capacity, size_t sliceNum)
        : capacity_(capacity)
        , sliceNum_(sliceNum > 0 ? sliceNum : std::max<size_t>(1, std::thread::hardware_concurrency()))
    {
        size_t silceSize = std::ceil(capacity / static_cast<double>(sliceNum_));
        for (size_t i = 0; i < sliceNum_; i++) {
            lruHashCache_.emplace_back(new LruCache<Key, Value>(silceSize));
        }
    }
    
    void put(Key key, Value value) 
    {
        size_t sliceIndex = Hash(key) % sliceNum_;
        lruHashCache_[sliceIndex]->put(key, value);
    }

    bool get(Key key, Value& value) 
    {
        size_t sliceIndex = Hash(key) % sliceNum_;
        return lruHashCache_[sliceIndex]->get(key, value);
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
    std::vector<std::unique_ptr<LruCache<Key, Value>>> lruHashCache_;
};