#include "radix_tree.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <utility>

node::node(unsigned char* data)
    : data_(data)
{}

std::uint32_t node::refcount()
{
    std::uint32_t u32;
    std::memcpy(&u32, data_, sizeof(u32));
    return u32;
}

void node::set_refcount(std::uint32_t value)
{
    std::memcpy(data_, &value, sizeof(value));
}

std::uint32_t node::prefix_length()
{
    std::uint32_t u32;
    std::memcpy(&u32, data_ + sizeof(std::uint32_t), sizeof(u32));
    return u32;
}

void node::set_prefix_length(std::uint32_t value)
{
    std::memcpy(data_ + sizeof(value), &value, sizeof(value));
}

std::uint32_t node::edgecount()
{
    std::uint32_t u32;
    std::memcpy(&u32, data_ + 2 * sizeof(std::uint32_t), sizeof(u32));
    return u32;
}

void node::set_edgecount(std::uint32_t value)
{
    std::memcpy(data_ + 2 * sizeof(value), &value, sizeof(value));
}

unsigned char* node::prefix()
{
    return data_ + 3 * sizeof(std::uint32_t);
}

void node::set_prefix(unsigned char const* bytes)
{
    std::memcpy(prefix(), bytes, prefix_length());
}

unsigned char* node::first_bytes()
{
    return prefix() + prefix_length();
}

void node::set_first_bytes(unsigned char const* bytes)
{
    std::memcpy(first_bytes(), bytes, edgecount());
}

unsigned char node::first_byte_at(std::size_t i)
{
    assert(i < edgecount());
    return *(first_bytes() + i);
}

void node::set_first_byte_at(std::size_t i, unsigned char byte)
{
    assert(i < edgecount());
    *(first_bytes() + i) = byte;
}

unsigned char* node::node_ptrs()
{
    return prefix() + prefix_length() + edgecount();
}

void node::set_node_ptrs(unsigned char const* ptrs)
{
    std::memcpy(node_ptrs(), ptrs, edgecount() * sizeof(void*));
}

node node::node_at(std::size_t i)
{
    assert(i < edgecount());

    unsigned char* data;
    std::memcpy(&data, node_ptrs() + i * sizeof(void*), sizeof(data));
    return node(data);
}

void node::set_node_at(std::size_t i, node const& n)
{
    assert(i < edgecount());
    std::memcpy(node_ptrs() + i * sizeof(void*),
                &n.data_,
                sizeof(n.data_));
}

void node::set_edge_at(std::size_t i, unsigned char byte, node const& n)
{
    set_first_byte_at(i, byte);
    set_node_at(i, n);
}

bool node::operator==(node const& other) const
{
    return data_ == other.data_;
}

bool node::operator!=(node const& other) const
{
    return !(*this == other);
}

node make_node(std::size_t refs, std::size_t bytes, std::size_t edges)
{
    std::size_t size = 3 * sizeof(std::uint32_t) + bytes
        + edges * (1 + sizeof(void*));

    auto* data = static_cast<unsigned char*>(std::malloc(size));
    assert(data);

    node n(data);
    n.set_refcount(static_cast<std::uint32_t>(refs));
    n.set_prefix_length(static_cast<std::uint32_t>(bytes));
    n.set_edgecount(static_cast<std::uint32_t>(edges));
    return n;
}

node resize(node n, std::size_t prefix_length, std::size_t nedges)
{
    std::size_t sz = 3 * sizeof(std::uint32_t) + prefix_length
        + nedges * (1 + sizeof(void*));
    auto* new_data = static_cast<unsigned char*>(std::realloc(n.data_, sz));
    assert(new_data);
    node new_node(new_data);
    new_node.set_prefix_length(static_cast<std::uint32_t>(prefix_length));
    new_node.set_edgecount(static_cast<std::uint32_t>(nedges));
    return new_node;
}

// ----------------------------------------------------------------------

radix_tree::radix_tree()
    : root_(make_node(0, 0, 0))
    , size_(0)
{}

static void free_nodes(node n)
{
    for (std::size_t i = 0; i < n.edgecount(); ++i)
        free_nodes(n.node_at(i));
    std::free(n.data_);
}

radix_tree::~radix_tree()
{
    free_nodes(root_);
}

bool radix_tree::insert(const unsigned char* key, std::size_t size)
{
    assert(key);
    assert(size > 0);

    std::size_t i = 0; // Number of characters matched in key.
    std::size_t j = 0; // Number of characters matched in current node.
    std::size_t edge_idx = 0; // Index of outgoing edge from the parent node.
    node current_node = root_;
    node parent_node = root_;

    while ((current_node.prefix_length() > 0 || current_node.edgecount() > 0)
           && i < size) {
        for (j = 0; j < current_node.prefix_length(); ++j) {
            if (current_node.prefix()[j] != key[i])
                break;
            ++i;
        }
        if (j != current_node.prefix_length())
            break; // Couldn't match the whole string, might need to split.

        // Check if there's an outgoing edge from this node.
        node next_node = current_node;
        for (std::size_t k = 0; k < current_node.edgecount(); ++k) {
            if (i < size && current_node.first_byte_at(k) == key[i]) {
                edge_idx = k;
                next_node = current_node.node_at(k);
                break;
            }
        }
        if (next_node == current_node)
            break; // No outgoing edge.
        parent_node = current_node;
        current_node = next_node;
    }

    if (i != size) {
        // Not all characters match, we might have to split the node.
        if (i == 0 || j == current_node.prefix_length()) {
            // The mismatch is at one of the outgoing edges, so we
            // create an edge from the current node to a new leaf node
            // that has the rest of the key as the prefix.
            node key_node = make_node(1, size - i, 0);
            key_node.set_prefix(key + i);

            // Reallocate for one more edge.
            node n = resize(current_node,
                            current_node.prefix_length(),
                            current_node.edgecount() + 1);

            // Make room for the new edge. We need to shift the chunk
            // of node pointers one byte to the right. Since resize()
            // increments the edgecount by 1, node_ptrs() tells us the
            // destination address. The chunk of node pointers starts
            // at one byte to the left of this destination.
            //
            // Since the regions can overlap, we use memmove.
            std::memmove(n.node_ptrs(),
                         n.node_ptrs() - 1,
                         (n.edgecount() - 1) * sizeof(void*));

            // Add an edge to the new node.
            n.set_edge_at(n.edgecount() - 1, key[i], key_node);

            // We could be at the root node, since it holds zero
            // characters in its prefix.
            if (n.prefix_length() == 0)
                root_.data_ = n.data_;
            else
                parent_node.set_node_at(edge_idx, n);
            ++size_;
            return true;
        }

        // There was a mismatch, so we need to split this node.
        //
        // Create two nodes that will be reachable from the parent.
        // One node will have the rest of the characters from the key,
        // and the other node will have the rest of the characters
        // from the current node's prefix.
        node key_node = make_node(1, size - i, 0);
        node split_node = make_node(current_node.refcount(),
                                    current_node.prefix_length() - j,
                                    current_node.edgecount());

        // Copy the prefix chunks to the new nodes.
        key_node.set_prefix(key + i);
        split_node.set_prefix(current_node.prefix() + j);

        // Copy the current node's edges to the new node.
        split_node.set_first_bytes(current_node.first_bytes());
        split_node.set_node_ptrs(current_node.node_ptrs());

        // Resize the current node to accomodate a prefix of j bytes
        // and 2 outgoing edges to the above nodes. Set the refcount
        // to 0 since this node's prefix wasn't already inserted.
        node n = resize(current_node, j, 2);
        n.set_refcount(0);

        // Add links to the new nodes. We don't need to copy the
        // prefix bytes since the above operation retains those in the
        // resized node.
        n.set_edge_at(0, key_node.prefix()[0], key_node);
        n.set_edge_at(1, split_node.prefix()[0], split_node);

        ++size_;
        parent_node.set_node_at(edge_idx, n);
        return true;
    }

    // All characters in the key match, but we still might need to split.
    if (j != current_node.prefix_length()) {
        // Not all characters from the current node's prefix match,
        // and there are no more characters from the key to match.

        // Create a node that contains the rest of the characters from
        // the current node's prefix and the outgoing edges from the
        // current node.
        node split_node = make_node(current_node.refcount(),
                                    current_node.prefix_length() - j,
                                    current_node.edgecount());
        split_node.set_prefix(current_node.prefix() + j);
        split_node.set_first_bytes(current_node.first_bytes());
        split_node.set_node_ptrs(current_node.node_ptrs());

        // Resize the current node to hold the matched characters from
        // its prefix and one edge to the new node.
        node n = resize(current_node, j, 1);

        // Add an edge to the split node and set the refcount to 1
        // since this key wasn't inserted earlier. We don't need to
        // set the prefix because the first j bytes in the prefix are
        // preserved by resize().
        n.set_edge_at(0, split_node.prefix()[0], split_node);
        n.set_refcount(1);

        ++size_;
        parent_node.set_node_at(edge_idx, n);
        return true;
    }

    assert(i == size);
    assert(j == current_node.prefix_length());

    // This node might not be marked as a key, even if all characters match.
    ++size_;
    current_node.set_refcount(current_node.refcount() + 1);
    return current_node.refcount() == 1;
}

bool radix_tree::erase(const unsigned char* key, std::size_t size)
{
    assert(key);
    assert(size > 0);

    std::size_t i = 0; // Number of characters matched in key.
    std::size_t j = 0; // Number of characters matched in current node.
    std::size_t edge_idx = 0; // Index of outgoing edge from the parent node.
    std::size_t gp_edge_idx = 0; // Index of outgoing edge from grandparent.
    node current_node = root_;
    node parent_node = current_node;
    node grandparent_node = current_node;

    while ((current_node.prefix_length() > 0 || current_node.edgecount() > 0)
           && i < size) {
        for (j = 0; j < current_node.prefix_length(); ++j) {
            if (current_node.prefix()[j] != key[i])
                break;
            ++i;
        }
        if (j != current_node.prefix_length())
            break;

        // Check if there's an outgoing edge from this node.
        node next_node = current_node;
        for (std::size_t k = 0; k < current_node.edgecount(); ++k) {
            if (i < size && current_node.first_byte_at(k) == key[i]) {
                gp_edge_idx = edge_idx;
                edge_idx = k;
                next_node = current_node.node_at(k);
                break;
            }
        }
        if (next_node == current_node)
            break; // No outgoing edge.
        grandparent_node = parent_node;
        parent_node = current_node;
        current_node = next_node;
    }

    if (i != size || j != current_node.prefix_length()
        || !current_node.refcount())
        return false;

    assert(parent_node != current_node);

    current_node.set_refcount(current_node.refcount() - 1);
    --size_;
    if (current_node.refcount()) {
        puts("1");
        return true;
    }

    std::size_t outgoing_edges = current_node.edgecount();
    if (outgoing_edges > 1)
        return true;

    if (outgoing_edges == 1) {
        // Merge this node with the single child node.
        node child = current_node.node_at(0);

        // Make room for the child node's prefix and edges. We need to
        // keep the old prefix length since resize() will overwrite
        // it.
        uint32_t old_prefix_length = current_node.prefix_length();
        node n = resize(current_node,
                        old_prefix_length + child.prefix_length(),
                        child.edgecount());

        // Append the child node's prefix to the current node.
        std::memcpy(n.prefix() + old_prefix_length,
                    child.prefix(),
                    child.prefix_length());

        // Copy the rest of child node's data to the current node.
        n.set_first_bytes(child.first_bytes());
        n.set_node_ptrs(child.node_ptrs());
        n.set_refcount(child.refcount());

        std::free(child.data_);
        parent_node.set_node_at(edge_idx, n);
        return true;
    }

    if (parent_node.edgecount() == 2 && !parent_node.refcount()
        && parent_node != root_) {
        // The current node is a leaf node, and its parent is not a
        // key. We can merge the parent node with its other child node.
        assert(edge_idx < 2);
        node other_child = parent_node.node_at(!edge_idx);

        // Make room for the child node's prefix and edges. We need to
        // keep the old prefix length since resize() will overwrite
        // it.
        uint32_t old_prefix_length = parent_node.prefix_length();
        node n = resize(parent_node,
                        old_prefix_length + other_child.prefix_length(),
                        other_child.edgecount());

        // Append the child node's prefix to the current node.
        std::memcpy(n.prefix() + old_prefix_length,
                    other_child.prefix(),
                    other_child.prefix_length());

        // Copy the rest of child node's data to the current node.
        n.set_first_bytes(other_child.first_bytes());
        n.set_node_ptrs(other_child.node_ptrs());
        n.set_refcount(other_child.refcount());

        std::free(current_node.data_);
        std::free(other_child.data_);
        grandparent_node.set_node_at(gp_edge_idx, n);
        return true;
    }

    // This is a leaf node that doesn't leave its parent with one
    // outgoing edge. Remove the outgoing edge to this node from the
    // parent.
    assert(outgoing_edges == 0);

    // Move the first byte and node pointer to the back of the byte
    // and pointer chunks respectively.
    std::size_t last_idx = parent_node.edgecount() - 1;
    unsigned char last_byte = parent_node.first_byte_at(last_idx);
    node last_ptr = parent_node.node_at(last_idx);
    parent_node.set_edge_at(edge_idx, last_byte, last_ptr);

    // Move the chunk of pointers one byte to the left, effectively
    // deleting the last byte in the region of first bytes by
    // overwriting it.
    std::memmove(parent_node.node_ptrs() - 1,
                 parent_node.node_ptrs(),
                 parent_node.edgecount() * sizeof(void*));

    // Shrink the parent node to the new size, which "deletes" the
    // last pointer in the chunk of node pointers.
    node n = resize(parent_node,
                    parent_node.prefix_length(),
                    parent_node.edgecount() - 1);

    // Nothing points to this node now, so we can reclaim it.
    std::free(current_node.data_);

    if (n.prefix_length() == 0)
        root_.data_ = n.data_;
    else
        grandparent_node.set_node_at(gp_edge_idx, n);
    return true;
}
#if 0
bool radix_tree::contains(const unsigned char* key, std::size_t size) const
{
    assert(key);
    assert(size > 0);

    std::size_t i = 0; // Number of characters matched in key.
    std::size_t j = 0; // Number of characters matched in current node.
    node* current_node = root_;

    while ((current_node->size_ > 0 || current_node->children_.size() > 0)
           && i < size) {
        for (j = 0; j < current_node->size_; ++j) {
            if (current_node->data_[j] != key[i])
                break;
            ++i;
        }
        if (j != current_node->size_)
            break;

        // Check if there's an outgoing edge from this node.
        node* parent_node = current_node;
        for (std::size_t k = 0; k < current_node->children_.size(); ++k) {
            if (i < size && current_node->next_chars_[k] == key[i]) {
                current_node = current_node->children_[k];
                break;
            }
        }
        if (current_node == parent_node)
            break; // No outgoing edge.
    }

    return i == size && j == current_node->size_ && current_node->key_;
}
#endif
std::size_t radix_tree::size() const
{
    return size_;
}

static void visit_child(node child_node, std::size_t level)
{
    assert(level > 0);

    for (std::size_t i = 0; i < 4 * (level - 1) + level; ++i)
        std::putchar(' ');
    std::printf("`-> ");
    for (std::uint32_t i = 0; i < child_node.prefix_length(); ++i)
        std::printf("%c", child_node.prefix()[i]);
    if (child_node.refcount())
        std::printf(" [*]");
    std::printf("\n");
    for (std::uint32_t i = 0; i < child_node.edgecount(); ++i) {
        visit_child(child_node.node_at(i), level + 1);
    }
}

void radix_tree::print()
{
    std::puts("[root]");
    for (std::uint32_t i = 0; i < root_.edgecount(); ++i) {
        visit_child(root_.node_at(i), 1);
    }
}
