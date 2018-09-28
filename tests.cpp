#include "radix_tree.hpp"

#include <cassert>
#include <cstdlib>
#include <string>
#include <unordered_set>
#include <vector>

constexpr bool show_trees = false;

static bool tree_insert(radix_tree& tree, std::string& key)
{
    auto* data = reinterpret_cast<const unsigned char*>(key.data());
    return tree.insert(data, key.size());
}

static bool tree_erase(radix_tree& tree, std::string& key)
{
    auto* data = reinterpret_cast<const unsigned char*>(key.data());
    return tree.erase(data, key.size());
}

static bool tree_contains(radix_tree const& tree, std::string& key)
{
    auto* data = reinterpret_cast<const unsigned char*>(key.data());
    return tree.contains(data, key.size());
}

static void smoke_test()
{
    radix_tree tree;

    std::string key = "foo";

    assert(tree_insert(tree, key));
    assert(tree_contains(tree, key));
    assert(tree_erase(tree, key));
    assert(!tree_contains(tree, key));
}

static void insert_test1()
{
    radix_tree tree;

    std::string test = "test";
    std::string testing = "testing";

    assert(tree_insert(tree, test));
    assert(tree_insert(tree, testing));
    assert(!tree_insert(tree, testing));
    assert(!tree_insert(tree, test));

    if (show_trees)
        tree.print();
}

// Example from wikipedia.
static void insert_test2()
{
    radix_tree tree;

    std::vector<std::string> keys = {
        "test", "water", "slow", "slower", "tester", "team", "toast"
    };

    for (auto& key : keys)
        assert(tree_insert(tree, key));
    for (auto& key : keys)
        assert(!tree_insert(tree, key));

    if (show_trees)
        tree.print();
}

// Insert a key which is a prefix of an already-inserted key.
static void insert_test3()
{
    radix_tree tree;

    std::vector<std::string> keys = {
        "test", "toaster", "toasting", "to"
    };

    for (auto& key : keys)
        assert(tree_insert(tree, key));
    for (auto& key : keys)
        assert(!tree_insert(tree, key));

    if (show_trees)
        tree.print();
}

// Erase a node with a single outgoing edge.
static void erase_test1()
{
    radix_tree tree;

    std::string test = "test";
    std::string testing = "testing";

    tree_insert(tree, test);
    tree_insert(tree, testing);

    assert(tree_erase(tree, test));

    if (show_trees)
        tree.print();
}

// Erase a node with a parent that has two outgoing edges.
static void erase_test2()
{
    radix_tree tree;

    std::string tester = "tester";
    std::string testing = "testing";

    tree_insert(tree, tester);
    tree_insert(tree, testing);

    assert(tree_erase(tree, tester));

    if (show_trees)
        tree.print();
}

static std::string random_key(std::size_t key_length)
{
    const char* chars = "abcdefghijklmnopqrstuvwxyz0123456789";
    const int size = 36;

    std::string key;
    key.reserve(key_length);

    for (std::size_t i = 0; i < key_length; ++i)
        key.push_back(chars[std::rand() % size]);

    return key;
}

static bool fuzz_test(std::size_t operations = 100000)
{
    radix_tree tree;
    std::unordered_set<std::string> set;
    std::size_t key_length = 50;

    for (std::size_t i = 0; i < operations; ++i) {
        // Insert or delete a random key from both.
        std::size_t len = (static_cast<std::size_t>(std::rand())
                           % key_length) + 1;
        std::string key = random_key(len);
        if (std::rand() % 2) {
            std::printf("insert: %s\n", key.c_str());
            bool tree_result = tree_insert(tree, key);
            bool set_result = set.insert(key).second;
            if (tree_result != set_result) {
                return false;
            }
        } else {
            std::printf("erase: %s\n", key.c_str());
            bool tree_result = tree_erase(tree, key);
            bool set_result = set.erase(key) == 1;
            if (tree_result != set_result) {
                return false;
            }
        }
    }

    assert(set.size() == tree.size());

    return true;
}

int main()
{
    smoke_test();
    insert_test1();
    insert_test2();
    insert_test3();
    erase_test1();
    erase_test2();
    assert(fuzz_test());
}
