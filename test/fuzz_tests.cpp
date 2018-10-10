#include "radix_tree.hpp"

#include <catch.hpp>

#include <cstdlib>
#include <ctime>
#include <random>
#include <string>
#include <unordered_map>

namespace
{

bool tree_insert(radix_tree& tree, std::string const& key)
{
    auto* data = reinterpret_cast<const unsigned char*>(key.data());
    return tree.insert(data, key.size());
}

bool tree_erase(radix_tree& tree, std::string const& key)
{
    auto* data = reinterpret_cast<const unsigned char*>(key.data());
    return tree.erase(data, key.size());
}

std::string random_key(std::minstd_rand& rng, std::size_t key_length)
{
    const char* chars = "abcdefghijklmnopqrstuvwxyz0123456789";
    const int size = 36;

    std::string key;
    key.reserve(key_length);

    for (std::size_t i = 0; i < key_length; ++i)
        key.push_back(chars[rng() % size]);

    return key;
}

constexpr std::size_t operations = 100000;
constexpr std::size_t key_length = 50;

}


TEST_CASE("fuzz", "[fuzz]")
{
    radix_tree tree;
    std::unordered_map<std::string, std::size_t> set;
    std::size_t set_size = 0;

    auto seed = static_cast<unsigned>(std::time(nullptr));
    std::minstd_rand rng(seed);
    CAPTURE(seed);

    for (std::size_t i = 0; i < operations; ++i) {
        std::string key;
        if (rng() % 2 == 1) {
            std::size_t len = (static_cast<std::size_t>(rng())
                               % key_length) + 1;
            key = random_key(rng, len);
            INFO("insert: " << key);
            bool tree_result = tree_insert(tree, key);
            bool set_result = ++set[key] == 1;
            ++set_size;
            REQUIRE(tree_result == set_result);
        } else if (set.size() > 0) {
            auto idx = static_cast<std::size_t>(rng()) % set.size();
            for (auto& item : set) {
                if (idx == 0) {
                    key = item.first;
                    break;
                }
                --idx;
            }
            INFO("erase: " << key);
            bool set_result =
                [&set,&key,&set_size]() {
                if (set[key] == 0)
                    return false;
                --set[key];
                --set_size;
                return true;
            }();
            bool tree_result = !key.empty() && tree_erase(tree, key);
            REQUIRE(tree_result == set_result);
        }
        REQUIRE(set_size == tree.size());
    }
}
