#pragma once

#include "ArcCacheNode.h"
#include <memory>
#include <unordered_map>
#include <map>
#include <mutex>
#include <list>

template<typename Key, typename Value>
class ArcLfuPart
{
public:
    using NodeType = ArcNode<Key, Value>;
    using NodePtr = std::shared_ptr<NodeType>;
    using NodeMap = std::unordered_map<Key, NodePtr>;
    using FreqMap = std::map<size_t, std::list<NodePtr>>;

    explicit ArcLfuPart(size_t capacity, size_t transformThreshold)
        : capacity_(capacity)
        , ghostCapacity_(capacity)
        , transformThreshold_(transformThreshold)
        , minFreq_(0)
    {
        initializeLists();
    }

    bool put(Key key, Value value) {}

    bool get(Key key, Value& value) {}

    bool checkGhost(Key key) {}

    void increaseCapacity() { ++capacity_; }

    bool decreaseCapacity() {}

    
private:
    void initializeLists() {}

    bool updateExistingNode(NodePtr node, const Value& value) {}

    bool addNewNode(const Key& key, const Value& value) {}

    void updateNodeFrequency(NodePtr node) {}

    void evictLeastFrequent(){}

    void removeFromGhost(NodePtr node){}

    void addToGhost(NodePtr node){}

    void removeOldestGhost() {}
private:
    size_t capacity_;
    size_t ghostCapacity_;
    size_t transformThreshold_;
    size_t minFreq_;
    std::mutex mutex_;

    NodeMap mainCache_;
    NodeMap ghostCache_;
    FreqMap freqMap_;
    
    NodePtr ghostHead_;
    NodePtr ghostTail_;

};