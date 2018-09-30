#ifndef RADIX_TREE_HPP
#define RADIX_TREE_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

struct node
{
    std::uint32_t refcount_;
    std::uint32_t prefix_len_;
    std::uint32_t nedges_;
    unsigned char data_[];

    node();
    ~node() = default;

    std::size_t size() const;
    unsigned char* prefix();
    unsigned char* first_bytes();
    unsigned char first_byte_at(std::size_t i);
    unsigned char* node_ptrs();
    node* node_at(std::size_t i);
    void set_prefix(unsigned char const* prefix);
    void set_first_bytes(unsigned char const* bytes);
    void set_first_byte_at(std::size_t i, unsigned char byte);
    void set_node_ptrs(unsigned char const* ptrs);
    void set_node_at(std::size_t i, node const* ptr);
    void set_edge_at(std::size_t i, unsigned char byte, node const* ptr);
};

node* make_node(std::uint32_t refcount,
                std::uint32_t prefix_length,
                std::uint32_t nedges);
void resize(node** n, std::uint32_t prefix_length, std::size_t nedges);

class radix_tree
{
public:
    radix_tree();
    ~radix_tree();

    // Returns true if the key wasn't already present in the tree.
    bool insert(const unsigned char* key, std::size_t size);

    // Returns true if the key was actually removed from the tree.
    bool erase(const unsigned char* key, std::size_t size);

    bool contains(const unsigned char* key, std::size_t size) const;

    // TODO: apply

    void print();
    std::size_t size() const;

private:
    node* root_;
    std::size_t size_;
};

#endif
