#include <iostream>
#include "LruCache.h"
#include <cassert>
int main() {
    // 创建一个容量为 3 的 LRU Cache
    LruCache<int, std::string> cache(3);

    // 测试插入数据
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");

    // 获取数据，应该成功
    std::string value;
    if (cache.get(1, value)) {
        std::cout << "Get 1: " << value << std::endl;  // 预期输出: "one"
    } else {
        std::cout << "Key 1 not found!" << std::endl;
    }

    // 继续插入数据，触发 LRU 淘汰
    cache.put(4, "four");  // 由于容量是 3，最老的 key 2 应该被淘汰

    // 尝试获取 key 2，应该失败
    if (!cache.get(2, value)) {
        std::cout << "Key 2 has been evicted as expected." << std::endl;
    }

    // 访问 key 3，使其变为最近使用
    cache.get(3, value);
    
    // 插入新数据，触发 LRU 淘汰
    cache.put(5, "five");  // 由于 key 1 最近没访问，应该被淘汰

    // 检查 key 1 是否被淘汰
    if (!cache.get(1, value)) {
        std::cout << "Key 1 has been evicted as expected." << std::endl;
    }

    // 检查 key 3 是否仍然存在（因为之前访问过）
    if (cache.get(3, value)) {
        std::cout << "Key 3 still exists: " << value << std::endl;  // 预期输出: "three"
    } else {
        std::cout << "Key 3 not found, which is incorrect!" << std::endl;
    }

    // 测试 remove 方法
    cache.remove(3);
    if (!cache.get(3, value)) {
        std::cout << "Key 3 has been successfully removed." << std::endl;
    }

    return 0;
}

