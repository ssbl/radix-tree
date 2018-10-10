#ifndef RADIX_TREE_HPP
#define RADIX_TREE_HPP

#include <cstddef>
#include <cstdint>

struct node
{
    unsigned char* data_;

    explicit node(unsigned char* data);

    bool operator==(node other) const;
    bool operator!=(node other) const;

    std::uint32_t refcount();
    std::uint32_t prefix_length();
    std::uint32_t edgecount();
    unsigned char* prefix();
    unsigned char* first_bytes();
    unsigned char first_byte_at(std::size_t i);
    unsigned char* node_ptrs();
    node node_at(std::size_t i);
    void set_refcount(std::uint32_t value);
    void set_prefix_length(std::uint32_t value);
    void set_edgecount(std::uint32_t value);
    void set_prefix(unsigned char const* prefix);
    void set_first_bytes(unsigned char const* bytes);
    void set_first_byte_at(std::size_t i, unsigned char byte);
    void set_node_ptrs(unsigned char const* ptrs);
    void set_node_at(std::size_t i, node n);
    void set_edge_at(std::size_t i, unsigned char byte, node n);
    void resize(std::size_t prefix_length, std::size_t edgecount);
};

node make_node(std::size_t refcount,
               std::size_t prefix_length,
               std::size_t nedges);

struct match_result
{
    std::size_t nkey;
    std::size_t nprefix;
    std::size_t edge_index;
    std::size_t gp_edge_index;
    node current_node;
    node parent_node;
    node grandparent_node;
};

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

    // Applies the function supplied to each key in the tree.
    void apply(void (*func)(unsigned char* data, std::size_t size, void* arg),
                void* arg);

    void print();
    std::size_t size() const;

private:
    match_result match(const unsigned char* key, std::size_t size) const;
    node root_;
    std::size_t size_;
};

#endif
