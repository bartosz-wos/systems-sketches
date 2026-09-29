/*
 * Tree of size N defined by parent array
 * Tradable by default, governed by the latest decision on the node or its ancestors
 *
 * Operations:
 * - halt(u) / resume(u): Apply state to node u and its subtree with a new timestamp
 * - canTrade(U): Query if node u is currently permitted to canTrade
 * - controllingNode(u): return the node id of the governing decision (-1 if none)
 *
 * Solution:
 * Flattened the tree via euler tour, then range updates and range queries on a lazy segtree
 *
*/

#pragma once

#include <vector>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <concepts>

template<typename F, typename T>
concept SegmentMergeOp = 
                    std::invocable<F, const T&, const T&> &&
                    std::convertible_to<std::invoke_result_t<F, const T&, const T&>, T>;

template<typename T, SegmentMergeOp<T> Merge>
class SegmentTree{
public:
    explicit SegmentTree(std::size_t n, T identity, Merge merge)
    : n_{n}
    , identity_{std::move(identity)}
    , merge_{std::move(merge)}
    , tree_(4 * n_, identity_)
    , lazy_(4 * n_, identity_)
    , has_lazy_(4 * n_, 0)
    {
        [[assume(n_ > 0)]];
    }

    bool update_range(std::size_t ql, std::size_t qr, const T& val) noexcept{
        if(ql > qr || qr >= n_)
            return false;
        
        update_impl(1, 0, n_ - 1, ql, qr, val);
        return true;
    }

    [[nodiscard]]
    std::optional<T> query_point(std::size_t pos) const noexcept{
        if(pos >= n_)
            return std::nullopt;
        
        return query_impl(1, 0, n_ - 1, pos, pos);
    }

    [[nodiscard]]
    std::optional<T> query_range(std::size_t ql, std::size_t qr) const noexcept{
        if(ql > qr || qr >= n_)
            return std::nullopt;
        
        return query_impl(1, 0, n_ - 1, ql, qr);
    }

    [[nodiscard]]
    constexpr std::size_t size() const noexcept{
        return n_;
    }

private:
    std::size_t n_;
    T identity_;
    [[no_unique_address]] Merge merge_;

    mutable std::vector<T> tree_;
    mutable std::vector<T> lazy_;
    mutable std::vector<std::uint8_t> has_lazy_;

    void apply(std::size_t u, const T& val) const noexcept{
        tree_[u] = val;
        lazy_[u] = val;
        has_lazy_[u] = 1;
    }

    void push(std::size_t u) const noexcept{
        if(has_lazy_[u]){
            apply(u << 1, lazy_[u]);
            apply(u << 1 | 1, lazy_[u]);
            has_lazy_[u] = 0;
        }
    }

    void pull(std::size_t u) const noexcept(noexcept(merge_(tree_[u << 1], tree_[u << 1 | 1]))){
        tree_[u] = merge_(tree_[u << 1], tree_[u << 1 | 1]);
    }

    void update_impl(std::size_t u, std::size_t l, std::size_t r, std::size_t ql, std::size_t qr, const T& val) noexcept{
        if(ql <= l && r <= qr){
            apply(u, val);
            return;
        }

        push(u);
        const std::size_t mid = l + (r - l) / 2;

        if(ql <= mid){
            update_impl(u << 1, l, mid, ql, qr, val);
        }
        if(qr > mid){
            update_impl(u << 1 | 1, mid + 1, r, ql, qr, val);
        }

        pull(u);
    }

    T query_impl(std::size_t u, std::size_t l, std::size_t r, std::size_t ql, std::size_t qr) const noexcept{
        if(ql <= l && r <= qr){
            return tree_[u];
        }

        push(u);
        const std::size_t mid = l + (r - l) / 2;

        if(qr <= mid){
            return query_impl(u << 1, l, mid, ql, qr);
        }

        if(ql > mid){
            return query_impl(u << 1 | 1, mid + 1, r, ql, qr);
        }

        return merge_(query_impl(u << 1, l, mid, ql, qr),
                        query_impl(u << 1 | 1, mid + 1, r, ql, qr));
    }
};

class HaltTree {
private:
    struct Decision{
        std::uint64_t timestamp{ 0 };
        std::optional<std::size_t> node{ std::nullopt };
        bool can_trade{ true };

        [[nodiscard]]
        friend constexpr bool operator==(const Decision& a, const Decision& b) noexcept = default;
    };

    struct MergeDecision{
        [[nodiscard]]
        constexpr Decision operator()(const Decision&a, const Decision& b) const noexcept{
            return (a.timestamp >= b.timestamp) ? a : b;
        }
    };

    std::size_t n_;
    std::uint64_t current_time_{ 0 };
    std::vector<std::size_t> tin_;
    std::vector<std::size_t> tout_;
    SegmentTree<Decision, MergeDecision> seg_tree_;

public:
    explicit HaltTree(const std::vector<int>& parent)
    : n_{parent.size()}
    , tin_(n_)
    , tout_(n_)
    , seg_tree_(n_, Decision{0, std::nullopt, true}, MergeDecision{}){
        [[assume(n_ > 0)]];

        std::vector<std::size_t> head(n_ + 1, 0);
        std::size_t root = 0;
        for(std::size_t i = 0; i < n_; ++i){
            if(parent[i] == -1){
                root = i;
            }else{
                ++head[static_cast<std::size_t>(parent[i]) + 1];
            }
        }

        [[assume(root < n_)]];

        for(std::size_t i = 0; i < n_; ++i){
            head[i + 1] += head[i];
        }

        std::vector<std::size_t> children(head[n_]);
        std::vector<std::size_t> cur_head = head;

        for(std::size_t i = 0; i < n_; ++i){
            if(parent[i] != -1){
                children[cur_head[static_cast<std::size_t>(parent[i])]++] = i;
            }
        }

        struct StackFrame{
            std::size_t node;
            std::size_t edge_idx;
            std::size_t edge_end;
        };

        std::vector<StackFrame> stack;
        stack.reserve(n_);

        std::size_t timer = 0;
        tin_[root] = timer++;
        stack.push_back({root, head[root], head[root + 1]});

        while(!stack.empty()){
            auto& [u, cur_idx, end_idx] = stack.back();
            if(cur_idx < end_idx) [[likely]]{
                const std::size_t v = children[cur_idx++];
                tin_[v] = timer++;
                stack.push_back({v, head[v], head[v + 1]});
            }else{
                tout_[u] = timer - 1;
                stack.pop_back();
            }
        }
    }

    void halt(int node) noexcept{
        if(node < 0) [[unlikely]]{
            return;
        }

        const auto u = static_cast<std::size_t>(node);
        if(u >= n_) [[unlikely]]{
            return;
        }

        ++current_time_;
        seg_tree_.update_range(tin_[u], tout_[u], Decision{current_time_, u, false});
    }

    void resume(int node) noexcept{
        if(node < 0) [[unlikely]]{
            return;
        }

        const auto u = static_cast<std::size_t>(node);
        if(u >= n_) [[unlikely]]{
            return;
        }

        ++current_time_;
        seg_tree_.update_range(tin_[u], tout_[u], Decision{current_time_, u, true});
    }

    [[nodiscard]]
    bool canTrade(int node) const noexcept{
        if(node < 0) [[unlikely]]{
            return true;
        }

        const auto u = static_cast<std::size_t>(node);
        if(u >= n_) [[unlikely]]{
            return true;
        }

        return seg_tree_.query_point(tin_[u])
                        .transform([](const Decision& d) noexcept { return d.can_trade; })
                        .value_or(true);
    }

    [[nodiscard]]
    int controllingNode(int node) const noexcept{
        if(node < 0) [[unlikely]]{
            return -1;
        }

        const auto u = static_cast<std::size_t>(node);
        if(u >= n_) [[unlikely]]{
            return -1;
        }

        return seg_tree_.query_point(tin_[u])
                        .and_then([](const Decision& d) { return d.node; })
                        .transform([](std::size_t id) { return static_cast<int>(id); })
                        .value_or(-1);
    }
};
