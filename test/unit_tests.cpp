#include "radix_tree.hpp"

#include <catch.hpp>

#include <cstdlib>
#include <string>
#include <vector>

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

bool tree_contains(radix_tree const& tree, std::string const& key)
{
    auto* data = reinterpret_cast<const unsigned char*>(key.data());
    return tree.contains(data, key.size());
}

}


TEST_CASE("insertion", "[insert]")
{
    radix_tree tree;

    REQUIRE(tree.size() == 0);

    SECTION("single key")
    {
        REQUIRE(tree_insert(tree, "key"));
    }

    SECTION("return whether inserted key was not already in the tree")
    {
        REQUIRE(tree_insert(tree, "test"));
        REQUIRE_FALSE(tree_insert(tree, "test"));
    }
}

TEST_CASE("deletion", "[erase]")
{
    radix_tree tree;

    SECTION("empty tree")
    {
        REQUIRE_FALSE(tree_erase(tree, "waldo"));
    }

    SECTION("single key")
    {
        tree_insert(tree, "key");
        REQUIRE(tree_erase(tree, "key"));
    }

    SECTION("common prefix of two keys")
    {
        tree_insert(tree, "checkpoint");
        tree_insert(tree, "checklist");
        REQUIRE_FALSE(tree_erase(tree, "check"));
    }

    SECTION("delete single key twice")
    {
        tree_insert(tree, "key");
        REQUIRE(tree_erase(tree, "key"));
        REQUIRE_FALSE(tree_erase(tree, "key"));
    }
}

TEST_CASE("lookup", "[contains]")
{
    radix_tree tree;

    SECTION("empty tree")
    {
        REQUIRE_FALSE(tree_contains(tree, "key"));
    }

    SECTION("inserted key")
    {
        tree_insert(tree, "key");
        REQUIRE(tree_contains(tree, "key"));
    }

    SECTION("common prefix of two inserted keys")
    {
        tree_insert(tree, "introduce");
        tree_insert(tree, "introspect");
        REQUIRE_FALSE(tree_contains(tree, "intro"));
    }

    SECTION("prefix of an inserted key")
    {
        tree_insert(tree, "toasted");
        REQUIRE_FALSE(tree_contains(tree, "toast"));
        REQUIRE_FALSE(tree_contains(tree, "toaste"));
    }

    SECTION("key not in the tree")
    {
        tree_insert(tree, "red");
        REQUIRE_FALSE(tree_contains(tree, "blue"));
    }
}

TEST_CASE("check if size is updated correctly")
{
    radix_tree tree;

    // Adapted from the example on wikipedia.
    std::vector<std::string> keys = {
        "tester", "water", "slow", "slower", "test", "team", "toast"
    };

    for (auto const& key : keys)
        REQUIRE(tree_insert(tree, key));
    REQUIRE(tree.size() == keys.size());
    for (auto const& key : keys)
        REQUIRE_FALSE(tree_insert(tree, key));
    REQUIRE(tree.size() == 2 * keys.size());
    for (auto const& key : keys)
        REQUIRE(tree_erase(tree, key));
    REQUIRE(tree.size() == keys.size());
    for (auto const& key : keys)
        REQUIRE(tree_erase(tree, key));
    REQUIRE(tree.size() == 0);
}

TEST_CASE("tree structure", "[print]")
{
    radix_tree tree;

    SECTION("delete node with a single outgoing edge")
    {
        tree_insert(tree, "test");
        tree_insert(tree, "testing");
        tree_erase(tree, "test");
        tree.print();
    }

    SECTION("node with a parent that has two outgoing edges")
    {
        tree_insert(tree, "tester");
        tree_insert(tree, "testing");
        tree_erase(tree, "tester");
        tree.print();
    }

    SECTION("insertion cases")
    {
        std::vector<std::string> keys = {
            "test", "toaster", "toasting", "to"
        };

        for (auto const& key : keys)
            tree_insert(tree, key);
        tree.print();
    }

    SECTION("simple example")
    {
        std::vector<std::string> keys = {
            "tester", "water", "slow", "slower", "test", "team", "toast"
        };

        for (auto const& key : keys)
            tree_insert(tree, key);
        tree.print();
    }
}
