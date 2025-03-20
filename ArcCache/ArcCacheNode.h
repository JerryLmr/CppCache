#pragma once

#include <memory>

template<typename Key, typename Value>
class ArcNode
{
private:
    Key key_;
    Value value_;
    size_t accessCount_;
    std::shared_ptr<ArcNode>prev_;
    std::shared_ptr<ArcNode>next_;

public:
    ArcNode() : accessCount_(1), prev_(nullptr), next_(nullptr) {}
    ArcNode(Key key, Value value)
        : key_(key)
        , value_(value)
        , accessCount_(1)
        , prev_(nullptr)
        , next_(nullptr)
    {}

    Key getKey() const {return key_;}
    Value getValue() const {return value_;}
    size_t getAccessCount() const {return accessCount_;}

    void set_Value(Value value) {value_ = value;}
    void increaseAccessCount() {++accessCount_;}

    template<typename k, typename v> friend class ArcLruPart;
    template<typename k, typename v> friend class ArcLfuPart;

};