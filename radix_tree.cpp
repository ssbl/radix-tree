#include "radix_tree.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>

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

void node::add_child(const unsigned char* key, std::size_t size)
{
    assert(key);
    assert(size > 0);
    assert(next_chars_.size() == children_.size());

    unsigned char first_char = key[0];
    node* child = new node();
    child->key_ = true;
    child->size_ = size;
    child->data_ = new unsigned char[size];
    std::memcpy(child->data_, key, size);

    next_chars_.emplace_back(first_char);
    children_.emplace_back(child);
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
        if (current_node->compressed()) {
            for (j = 0; j < current_node->size_; ++j) {
                if (current_node->data_[j] != key[i])
                    break;
                ++i;
            }
            if (j != current_node->size_)
                break; // Couldn't match the whole string, might need to split.
            node* parent_node = current_node;
            for (std::size_t k = 0; k < current_node->children_.size(); ++k) {
                if (i < size && current_node->next_chars_[k] == key[i]) {
                    current_node = current_node->children_[k];
                    break;
                }
            }
            if (current_node == parent_node)
                break; // No outgoing edges.
        } else {
            if (current_node->data_ && key[i] == current_node->data_[0])
                ++i;
            node* parent_node = current_node;
            for (std::size_t k = 0; k < current_node->children_.size(); ++k) {
                if (i < size && current_node->next_chars_[k] == key[i]) {
                    current_node = current_node->children_[k];
                    break;
                }
            }
            if (current_node == parent_node)
                break; // No outgoing edges.
        }
    }

    if (i == 0) {
        // No characters matched, create a new child node.
        current_node->add_child(key, size);
        ++size_;
        return true;
    }

    if (i != size) {
        // Some characters match, we might have to split the node.
        current_node->add_child(key + i, size - i);
        if (j == current_node->size_)
            current_node->key_ = true;
        else {
            current_node->key_ = false;
            current_node->add_child(current_node->data_ + j,
                                    current_node->size_ - j);
            current_node->size_ = j;
        }
        ++size_;
        return true;
    }

    // All characters match.
    assert(i == size);
    assert(current_node->key_);
    return false;
}

static void visit_child(node const* child_node, std::size_t level)
{
    for (std::size_t i = 0; i < 4 * (level - 1) + level; ++i)
        std::putchar(' ');
    std::printf("`-> ");
    for (std::size_t i = 0; i < child_node->size_; ++i)
        std::printf("%c", child_node->data_[i]);
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
