#include "radix_tree.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <utility>

node::node()
    : key_(false)
    , refs_(1)
    , size_(0)
    , data_(nullptr)
    , next_chars_()
    , children_()
{}

bool node::compressed() const
{
    return size_ > 1;
}

node* node::make_edge(const unsigned char* key, std::size_t size) const
{
    assert(key);
    assert(size > 0);

    node* child = new node();
    child->key_ = true;
    child->size_ = size;
    child->data_ = new unsigned char[size];
    std::memcpy(child->data_, key, size);

    return child;
}

node* node::add_edge(const unsigned char* key, std::size_t size)
{
    node* child = make_edge(key, size);

    next_chars_.emplace_back(key[0]);
    children_.emplace_back(child);

    return child;
}

node* node::split(const unsigned char* key, std::size_t size, bool is_key)
{
    node* child = make_edge(key, size);

    child->key_ = is_key;
    std::swap(child->children_, children_);
    std::swap(child->next_chars_, next_chars_);

    next_chars_.emplace_back(key[0]);
    children_.emplace_back(child);

    return child;
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
    node* current_node = root_;

    while ((current_node->size_ > 0 || current_node->children_.size() > 0)
           && i < size) {
        for (j = 0; j < current_node->size_; ++j) {
            if (current_node->data_[j] != key[i])
                break;
            ++i;
        }
        if (j != current_node->size_)
            break; // Couldn't match the whole string, might need to split.

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

    if (i == 0) {
        // No characters matched, create a new outgoing edge.
        current_node->add_edge(key, size);
        ++size_;
        return true;
    }

    if (i != size) {
        // Some characters match, we might have to split the node.
        if (j == current_node->size_) {
            // No need to split since all characters in the current
            // node match. Create an outgoing edge with the remaining
            // characters in the key.
            current_node->add_edge(key + i, size - i);
            ++size_;
            return true;
        }

        // There was a mismatch, so we need to split this node.
        current_node->split(current_node->data_ + j,
                            current_node->size_ - j,
                            current_node->key_);
        current_node->key_ = false;
        current_node->size_ = j;
        current_node->add_edge(key + i, size - i);
        ++size_;
        return true;
    }

    // All characters match, but we still might need to split.
    if (j != current_node->size_) {
        // This key is a prefix of the current node.
        current_node->split(current_node->data_ + j,
                            current_node->size_ - j,
                            current_node->key_);
        current_node->size_ = j;
        current_node->key_ = true;
        ++size_;
        return true;
    }

    assert(i == size);
    assert(j == current_node->size_);
    assert(current_node->key_);
    return false;
}

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

    if (i != size && j != current_node->size_ && !current_node->key_)
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

static void visit_child(node const* child_node, std::size_t level)
{
    assert(level > 0);

    for (std::size_t i = 0; i < 4 * (level - 1) + level; ++i)
        std::putchar(' ');
    std::printf("`-> ");
    for (std::size_t i = 0; i < child_node->size_; ++i)
        std::printf("%c", child_node->data_[i]);
    if (child_node->key_)
        std::printf(" [*]");
    std::printf("\n");
    for (std::size_t i = 0; i < child_node->children_.size(); ++i)
        visit_child(child_node->children_[i], level + 1);
}

void radix_tree::print() const
{
    std::puts("[root]");
    for (std::size_t i = 0; i < root_->children_.size(); ++i)
        visit_child(root_->children_[i], 1);
}
