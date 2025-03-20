#pragma once

#include "ArcCacheNode.h"
#include <unordered_map>
#include <mutex>

template<typename Key, typename Value>
class ArcLruPart
{
public:
    using NodeType =  ArcNode<Key, Value>;
    using NodePtr  =  std::shared_ptr<NodeType>;
    using NodeMap  =  std::unordered_map<Key, NodeType>;

    explicit ArcLruPart(size_t capacity, size_t transformThreashold)
        : capacity_(capacity)
        , ghostCapacity_(capacity)
        , transformThreashold_(transformThreashold)
    {
        initializeLists();
    }

    bool put(Key key, Value value) 
    {
        if (capacity_ == 0) return false;

        std::lock_guard<std::mutex> lock(mutex_);
        auto it = mainCache_.find(key);
        if (it != mainCache_.end())
        {
            return updateExsitingNode(it->second, value);
        }
        return addNewNode(key, value);
    }

private:
    void initializeLists() 
    {
        mainHead_ = std::make_shared<NodeType>();
        mainTail_ = std::make_shared<NodeType>();
        mainHead_->next_ = mainTail_;
        mainTail_->prev_ = mainHead_;

        ghostHead_ = std::make_shared<NodeType>();
        ghostTail_ = std::make_shared<NodeType>();
        ghostHead_->next_ = ghostTail_;
        ghostTail_->prev_ = ghostHead_;
    }

    bool updateExsitingNode(NodePtr node, const Value& value)
    {
        node->set_Value(value);
        movetoFront(node);
        return true;
    }

    bool addNewNode(const Key& key, const Value& value)
    {
        if (mainCache_.size() >= capacity_)
        {
            evictLeastRecent();
        }
        NodePtr newNode = std::make_shared<NodeType>(key, value);
        mainCache_[key] = newNode;
        addToFront(newNode);
        return true;
    }

    bool updateNodeAccess(NodePtr node) 
    {
        movetoFront(node);
        node->increaseAccessCount();
        return node->getAccessCount() >= transformThreashold_;
    }

    void movetoFront(NodePtr node)
    {
        node->prev_->next_ = node->next_;
        node->next_->prev_ = node->prev_;

        addToFront(node);
    }

    void addToFront(NodePtr node)
    {
        node->next_ = mainHead_->next_;
        node->prev_ = mainHead_;
        mainHead_->next_->prev_ = node;
        mainHead_->next_ = node;
    }

    void evictLeastRecent()
    {
        NodePtr leastRecent = mainTail_->prev_;
        if (leastRecent == mainHead_) 
            return;

        removeFromMain(leastRecent);

        if (ghostCache_.size() >= ghostCapacity_)
        {
            removeOldestGhost();
        }
        addtoGhost(leastRecent);
        
        mainCache_.erase(leastRecent->getKey());
    }

    void removeFromMain(NodePtr node)
    {
        node->prev_->next_ = node->next_;
        node->next_->prev_ = node->prev_;
    }

    void removeFromGhost(NodePtr node)
    {
        node->prev_->next_ = node->next_;
        node->next_->prev_ = node->prev_;
    }

    void addtoGhost(NodePtr node)
    {
        node->accessCount_ = 1;

        node->next_ = ghostHead_->next_;
        node->prev_ = ghostHead_;
        ghostHead_->next_->prev_ = node;
        ghostHead_->next_ = node;

        ghostCache_[node->getKey()] = node;
    }

    void removeOldestGhost()
    {
        NodePtr oldestGhost = ghostTail_->prev_;
        if (oldestGhost == ghostHead_)
            return;
        
        removeFromGhost(oldestGhost);
        ghostCache_.erase(oldestGhost->getKey());
    }


private:
    size_t capacity_;
    size_t ghostCapacity_;
    size_t transformThreashold_;
    std::mutex mutex_;

    NodeMap mainCache_;
    NodeMap ghostCache_;

    NodePtr mainHead_;
    NodePtr mainTail_;

    NodePtr ghostHead_;
    NodePtr ghostTail_;
};