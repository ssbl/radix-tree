#include "radix_tree.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <utility>

node::node()
    : refcount_(0)
    , prefix_len_(0)
    , nedges_(0)
{}

std::size_t node::size() const
{
    return sizeof(node) + prefix_len_ + nedges_ + nedges_ * sizeof(node*);
}

unsigned char* node::prefix()
{
    return data_;
}

void node::set_prefix(unsigned char const* bytes)
{
    std::memcpy(prefix(), bytes, prefix_len_);
}

unsigned char* node::first_bytes()
{
    return data_ + prefix_len_;
}

void node::set_first_bytes(unsigned char const* bytes)
{
    std::memcpy(first_bytes(), bytes, nedges_);
}

unsigned char node::first_byte_at(std::size_t i)
{
    assert(i < nedges_);
    return *(first_bytes() + i);
}

void node::set_first_byte_at(std::size_t i, unsigned char byte)
{
    assert(i < nedges_);
    *(first_bytes() + i) = byte;
}

unsigned char* node::node_ptrs()
{
    return data_ + prefix_len_ + nedges_;
}

void node::set_node_ptrs(unsigned char const* ptrs)
{
    std::memcpy(node_ptrs(), ptrs, nedges_ * sizeof(node*));
}

node* node::node_at(std::size_t i)
{
    assert(i < nedges_);

    node* ptr;
    std::memcpy(&ptr, node_ptrs() + i * sizeof(node*), sizeof(node*));
    return ptr;
}

void node::set_node_at(std::size_t i, node const* ptr)
{
    assert(i < nedges_);
    std::memcpy(node_ptrs() + i * sizeof(node*), &ptr, sizeof(node*));
}

node* make_node(std::uint32_t refs, std::uint32_t bytes, std::uint32_t edges)
{
    std::size_t size = sizeof(node) + bytes
        + edges * (1 + sizeof(node*));

    node* n = static_cast<node*>(std::malloc(size));
    n->refcount_ = refs;
    n->prefix_len_ = bytes;
    n->nedges_ = edges;
    return n;
}

void resize(node** n, std::uint32_t prefix_length, std::size_t nedges)
{
    std::size_t sz = sizeof(node) + prefix_length
        + nedges * (1 + sizeof(node*));
    (*n) = static_cast<node*>(std::realloc((*n), sz));
    (*n)->prefix_len_ = prefix_length;
    (*n)->nedges_ = nedges;
}

// ----------------------------------------------------------------------

radix_tree::radix_tree()
    : root_(new node())
    , size_(0)
{}

radix_tree::~radix_tree()
{}

bool radix_tree::insert(const unsigned char* key, std::size_t size)
{
    assert(key);
    assert(size > 0);

    std::size_t i = 0; // Number of characters matched in key.
    std::size_t j = 0; // Number of characters matched in current node.
    std::size_t edge_idx = 0; // Index of outgoing edge from the parent node.
    node* current_node = root_;
    node* parent_node = current_node;

    while ((current_node->prefix_len_ > 0 || current_node->nedges_ > 0)
           && i < size) {
        for (j = 0; j < current_node->prefix_len_; ++j) {
            if (current_node->data_[j] != key[i])
                break;
            ++i;
        }
        if (j != current_node->prefix_len_)
            break; // Couldn't match the whole string, might need to split.

        // Check if there's an outgoing edge from this node.
        node* next_node = current_node;
        for (std::size_t k = 0; k < current_node->nedges_; ++k) {
            if (i < size && current_node->data_[current_node->prefix_len_ + k]
                == key[i]) {
                edge_idx = k;
                next_node = current_node->node_at(k);
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
        if (i == 0 || j == current_node->prefix_len_) {
            // The mismatch is at one of the outgoing edges, so we
            // create an edge from the current node to a new leaf node
            // that has the rest of the key as the prefix.
            node* key_node = make_node(1, size - i, 0);
            key_node->set_prefix(key + i);

            // Reallocate for one more edge.
            resize(&current_node,
                   current_node->prefix_len_,
                   current_node->nedges_ + 1);

            // Make room for the new edge. We need to shift the chunk
            // of node pointers one byte to the right. Since resize()
            // sets nedges_ to nedges_ + 1, node_ptrs() tells us the
            // destination address. The chunk of node pointers starts
            // at one byte to the left of this destination.
            //
            // Since the regions can overlap, we use memmove.
            std::memmove(current_node->node_ptrs(),
                         current_node->node_ptrs() - 1,
                         (current_node->nedges_ - 1) * sizeof(node*));

            // Add an edge to the new node.
            current_node->set_first_byte_at(current_node->nedges_ - 1, key[i]);
            current_node->set_node_at(current_node->nedges_ - 1, key_node);

            // We could be at the root node, since it holds zero
            // characters in its prefix.
            if (current_node->prefix_len_ == 0)
                root_ = current_node;
            else
                parent_node->set_node_at(edge_idx, current_node);
            ++size_;
            return true;
        }

        // There was a mismatch, so we need to split this node.
        //
        // Create two nodes that will be reachable from the parent.
        // One node will have the rest of the characters from the key,
        // and the other node will have the rest of the characters
        // from the current node's prefix.
        node* key_node = make_node(1, size - i, 0);
        node* split_node = make_node(current_node->refcount_,
                                     current_node->prefix_len_ - j,
                                     current_node->nedges_);

        // Copy the prefix chunks to the new nodes.
        key_node->set_prefix(key + i);
        split_node->set_prefix(current_node->prefix() + j);

        // Copy the current node's edges to the new node.
        split_node->set_first_bytes(current_node->first_bytes());
        split_node->set_node_ptrs(current_node->node_ptrs());

        // Resize the current node to accomodate a prefix of j bytes
        // and 2 outgoing edges to the above nodes. Set the refcount
        // to 0 since this node's prefix wasn't already inserted.
        resize(&current_node, j, 2);
        current_node->refcount_ = 0;

        // Add links to the new nodes. We don't need to copy the
        // prefix bytes since the above operation retains those in the
        // resized node.
        current_node->set_first_byte_at(0, key_node->prefix()[0]);
        current_node->set_first_byte_at(1, split_node->prefix()[0]);
        current_node->set_node_at(0, key_node);
        current_node->set_node_at(1, split_node);

        ++size_;
        parent_node->set_node_at(edge_idx, current_node);
        return true;
    }

    // All characters in the key match, but we still might need to split.
    if (j != current_node->prefix_len_) {
        // Not all characters from the current node's prefix match,
        // and there are no more characters from the key to match.

        // Create a node that contains the rest of the characters from
        // the current node's prefix and the outgoing edges from the
        // current node.
        node* split_node = make_node(current_node->refcount_,
                                     current_node->prefix_len_ - j,
                                     current_node->nedges_);
        split_node->set_prefix(current_node->prefix() + j);
        split_node->set_first_bytes(current_node->first_bytes());
        split_node->set_node_ptrs(current_node->node_ptrs());

        // Resize the current node to hold the matched characters from
        // its prefix and one edge to the new node.
        resize(&current_node, j, 1);

        // Add an edge to the split node and set the refcount to 1
        // since this key wasn't inserted earlier. We don't need to
        // set the prefix because the first j bytes in the prefix are
        // preserved by resize().
        current_node->set_first_byte_at(0, split_node->prefix()[0]);
        current_node->set_node_at(0, split_node);
        current_node->refcount_ = 1;

        ++size_;
        parent_node->set_node_at(edge_idx, current_node);
        return true;
    }

    assert(i == size);
    assert(j == current_node->prefix_len_);

    // This node might not be marked as a key, even if all characters match.
    if (!current_node->refcount_) {
        ++current_node->refcount_;
        ++size_;
        return true;
    }

    ++current_node->refcount_;
    return false;
}
#if 0
bool radix_tree::erase(const unsigned char* key, std::size_t size)
{
    assert(key);
    assert(size > 0);

    std::size_t i = 0; // Number of characters matched in key.
    std::size_t j = 0; // Number of characters matched in current node.
    std::size_t edge_idx = 0; // Index of outgoing edge from the parent node.
    node* current_node = root_;
    node* parent_node = current_node;

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
        node* next_node = current_node;
        for (std::size_t k = 0; k < current_node->children_.size(); ++k) {
            if (i < size && current_node->next_chars_[k] == key[i]) {
                edge_idx = k;
                next_node = current_node->children_[k];
                break;
            }
        }
        if (next_node == current_node)
            break; // No outgoing edge.
        parent_node = current_node;
        current_node = next_node;
    }

    if (i != size || j != current_node->size_ || !current_node->key_)
        return false;

    assert(parent_node != current_node);

    std::size_t outgoing_edges = current_node->children_.size();
    if (outgoing_edges > 1) {
        current_node->key_ = false;
        --size_;
        return true;
    }

    if (outgoing_edges == 1) {
        // Merge this node with the single child node.
        node* child = current_node->children_.front();
        std::size_t merged_len = current_node->size_ + child->size_;
        auto* merged_data = new unsigned char[merged_len];
        std::memcpy(merged_data, current_node->data_, current_node->size_);
        std::memcpy(merged_data + current_node->size_, child->data_,
                    child->size_);
        child->data_ = merged_data;
        child->size_ = merged_len;
        parent_node->children_[edge_idx] = child;
        --size_;
        return true;
    }

    if (parent_node->children_.size() == 2 && !parent_node->key_) {
        // We can merge the parent node with its other child node.
        node* other_child = parent_node->children_[!edge_idx];
        std::size_t merged_len = parent_node->size_ + other_child->size_;
        auto* merged_data = new unsigned char[merged_len];
        std::memcpy(merged_data, parent_node->data_, parent_node->size_);
        std::memcpy(merged_data + parent_node->size_, other_child->data_,
                    other_child->size_);
        parent_node->data_ = merged_data;
        parent_node->size_ = merged_len;
        std::swap(parent_node->key_, other_child->key_);
        std::swap(parent_node->children_, other_child->children_);
        std::swap(parent_node->next_chars_, other_child->next_chars_);
        --size_;
        return true;
    }

    // This is a leaf node that doesn't leave its parent with one
    // outgoing edge. Remove the outgoing edge to this node from the
    // parent.
    assert(outgoing_edges == 0);

    std::swap(parent_node->children_[edge_idx], parent_node->children_.back());
    std::swap(parent_node->next_chars_[edge_idx],
              parent_node->next_chars_.back());
    parent_node->children_.pop_back();
    parent_node->next_chars_.pop_back();
    --size_;
    return true;
}

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
std::size_t radix_tree::size() const
{
    return size_;
}
#endif

static void visit_child(node* child_node, std::size_t level)
{
    assert(level > 0);

    for (std::size_t i = 0; i < 4 * (level - 1) + level; ++i)
        std::putchar(' ');
    std::printf("`-> ");
    for (std::size_t i = 0; i < child_node->prefix_len_; ++i)
        std::printf("%c", child_node->data_[i]);
    if (child_node->refcount_)
        std::printf(" [*]");
    std::printf("\n");
    for (std::size_t i = 0; i < child_node->nedges_; ++i) {
        visit_child(child_node->node_at(i), level + 1);
    }
}

void radix_tree::print()
{
    std::puts("[root]");
    for (std::size_t i = 0; i < root_->nedges_; ++i) {
        visit_child(root_->node_at(i), 1);
    }
}
