#include "radix_tree.hpp"

#include <cassert>
#include <string>
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

    tree.print();
}

int main()
{
    smoke_test();
    insert_test1();
    insert_test2();
    insert_test3();
    erase_test1();
    erase_test2();
}
