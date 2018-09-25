#include "radix_tree.hpp"

#include <cassert>
#include <string>
#include <vector>

constexpr bool show_trees = true;

static bool tree_insert(radix_tree& tree, std::string& key)
{
    auto* data = reinterpret_cast<const unsigned char*>(key.data());
    return tree.insert(data, key.size());
}

static void smoke_test()
{
    radix_tree tree;

    std::string key = "foo";

    assert(tree_insert(tree, key));
    // assert(tree.contains(data, key.size()));
    // assert(tree.erase(data, key.size()));
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
        "test", "toaster", "toasting", "slow", "slowly"
    };

    for (auto& key : keys)
        assert(tree_insert(tree, key));
    for (auto& key : keys)
        assert(!tree_insert(tree, key));

    if (show_trees)
        tree.print();
}

// Keys with a common prefix, with the smaller key being inserted
// after the larger.
static void insert_test3()
{
    radix_tree tree;

    std::string larger = "xyzzy";
    std::string smaller = "xyz";

    assert(tree_insert(tree, larger));
    assert(tree_insert(tree, smaller));

    if (show_trees)
        tree.print();
}

int main()
{
    smoke_test();
    insert_test1();
    insert_test2();
    insert_test3();
}
