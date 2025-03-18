#include <iostream>
#include "LruCache.h"
#include "LfuCache.h"
#include <cassert>
int main() {
    // // 创建一个容量为 3 的 LRU Cache
    // LruCache<int, std::string> cache(3);

    // // 测试插入数据
    // cache.put(1, "one");
    // cache.put(2, "two");
    // cache.put(3, "three");

    // // 获取数据，应该成功
    // std::string value;
    // if (cache.get(1, value)) {
    //     std::cout << "Get 1: " << value << std::endl;  // 预期输出: "one"
    // } else {
    //     std::cout << "Key 1 not found!" << std::endl;
    // }

    // // 继续插入数据，触发 LRU 淘汰
    // cache.put(4, "four");  // 由于容量是 3，最老的 key 2 应该被淘汰

    // // 尝试获取 key 2，应该失败
    // if (!cache.get(2, value)) {
    //     std::cout << "Key 2 has been evicted as expected." << std::endl;
    // }

    // // 访问 key 3，使其变为最近使用
    // cache.get(3, value);
    
    // // 插入新数据，触发 LRU 淘汰
    // cache.put(5, "five");  // 由于 key 1 最近没访问，应该被淘汰

    // // 检查 key 1 是否被淘汰
    // if (!cache.get(1, value)) {
    //     std::cout << "Key 1 has been evicted as expected." << std::endl;
    // }

    // // 检查 key 3 是否仍然存在（因为之前访问过）
    // if (cache.get(3, value)) {
    //     std::cout << "Key 3 still exists: " << value << std::endl;  // 预期输出: "three"
    // } else {
    //     std::cout << "Key 3 not found, which is incorrect!" << std::endl;
    // }

    // // 测试 remove 方法
    // cache.remove(3);
    // if (!cache.get(3, value)) {
    //     std::cout << "Key 3 has been successfully removed." << std::endl;
    // }

    // return 0;
    // 创建一个缓存，容量为 5，最大平均访问频次为 3
    LfuCache<int, std::string> cache(5, 3);

    // 插入一些数据
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");
    cache.put(4, "four");
    cache.put(5, "five");

    // 输出缓存中已有的数据
    std::cout << "After inserting 5 items:" << std::endl;
    std::string value;
    for (int i = 1; i <= 5; ++i) {
        if (cache.get(i, value)) {
            std::cout << "Key " << i << ": " << value << std::endl;
        }
    }

    // 执行多次访问，以触发 handleOverMaxAverageNum
    cache.get(1, value);  // 访问 1
    cache.get(2, value);  // 访问 2
    cache.get(3, value);  // 访问 3
    cache.get(4, value);  // 访问 4
    cache.get(5, value);  // 访问 5

    // 插入更多数据，触发 handleOverMaxAverageNum
    cache.put(6, "six");
    cache.put(7, "seven");
    cache.put(8, "eight");

    // 输出缓存数据，检查是否触发了频率调整
    std::cout << "\nAfter inserting more items:" << std::endl;
    for (int i = 1; i <= 8; ++i) {
        if (cache.get(i, value)) {
            std::cout << "Key " << i << ": " << value << std::endl;
        }
    }

    return 0;
}

