#ifndef RADIX_TREE_HPP
#define RADIX_TREE_HPP

#include <cstddef>
#include <functional>
#include <vector>

struct node
{
    bool key_;
    std::size_t refs_;
    std::size_t size_;
    unsigned char* data_;
    std::vector<unsigned char> next_chars_;
    std::vector<node*> children_;

    node();
    ~node() = default;

    bool compressed() const;
    void split(const unsigned char* key, std::size_t size,
               bool single);
};

class radix_tree
{
public:
    radix_tree();
    ~radix_tree();

    // Returns true if the key wasn't already present in the tree.
    bool insert(const unsigned char* key, std::size_t size);

    // Returns true if the key was actually removed from the tree.
    bool erase(unsigned char* key, std::size_t size);

    bool contains(unsigned char* key, std::size_t size) const;

    // TODO: apply

    void print() const;

private:

    node* root_;
    std::size_t size_;
};

#endif
