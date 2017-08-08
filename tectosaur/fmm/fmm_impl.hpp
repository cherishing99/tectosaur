#pragma once

#include <cmath>
#include <functional>
#include <memory>
#include "fmm_kernels.hpp"
#include "octree.hpp"
#include "translation_surf.hpp"

template <size_t dim>
struct FMMConfig {
    // The MAC needs to < (1.0 / (check_r - 1)) so that farfield
    // approximations aren't used when the check surface intersects the
    // target box. How about we just use that as our MAC, since errors
    // further from the check surface should flatten out!
    double inner_r;
    double outer_r;
    size_t order;
    Kernel<dim> kernel;
    std::vector<double> params;

    std::string kernel_name() const { return kernel.name; }
    int tensor_dim() const { return kernel.tensor_dim; }
};

template <size_t dim>
std::vector<double> c2e_solve(std::vector<std::array<double,dim>> surf,
    const Ball<dim>& bounds, double check_r, double equiv_r, const FMMConfig<dim>& cfg);

struct MatrixFreeOp {
    std::vector<int> obs_n_idx;
    std::vector<int> src_n_idx;

    template <typename TreeT>
    void insert(const TreeT& obs_n, const TreeT& src_n) {
        if (obs_n.end - obs_n.start == 0 || src_n.end - src_n.start == 0) {
            return;
        }
        obs_n_idx.push_back(obs_n.idx);
        src_n_idx.push_back(src_n.idx);
    }
};

struct CompressedInteractionList {
    std::vector<int> obs_n_idxs;//TODO: Make these 64-bit.
    std::vector<int> obs_src_starts;
    std::vector<int> src_n_idxs;
};

template <typename TreeT>
size_t count_interactions(const CompressedInteractionList& list,
    const TreeT& obs_tree, const TreeT& src_tree,
    bool obs_surf, bool src_surf, int n_surf) 
{
    size_t n = 0;
    for (size_t i = 0; i < list.obs_n_idxs.size(); i++) {
        int obs_n_idx = list.obs_n_idxs[i];
        int n_obs = n_surf;
        if (!obs_surf) {
            auto& n = obs_tree.nodes[obs_n_idx];
            n_obs = n.end - n.start;
        }
        for (int j = list.obs_src_starts[i]; j < list.obs_src_starts[i + 1]; j++) {
            int src_n_idx = list.src_n_idxs[j];
            int n_src = n_surf;
            if (!src_surf) {
                auto& n = src_tree.nodes[src_n_idx];
                n_src = n.end - n.start;
            }
            n += n_obs * n_src;
        }
    }
    return n;
}

CompressedInteractionList compress(const std::vector<std::vector<int>>& list);

template <typename TreeT>
struct FMMMat {
    TreeT obs_tree;
    std::vector<std::array<double,TreeT::dim>> obs_normals;
    TreeT src_tree;
    std::vector<std::array<double,TreeT::dim>> src_normals;

    FMMConfig<TreeT::dim> cfg;
    std::vector<std::array<double,TreeT::dim>> surf;

    MatrixFreeOp p2m;
    std::vector<MatrixFreeOp> m2m;
    std::vector<double> u2e_ops;
    std::vector<MatrixFreeOp> u2e;

    std::vector<MatrixFreeOp> l2l;
    MatrixFreeOp l2p;
    std::vector<double> d2e_ops;
    std::vector<MatrixFreeOp> d2e;

    MatrixFreeOp p2p;
    MatrixFreeOp m2p;
    MatrixFreeOp p2l;
    MatrixFreeOp m2l;

    CompressedInteractionList p2m_new;
    std::vector<CompressedInteractionList> m2m_new;
    std::vector<CompressedInteractionList> u2e_new;

    CompressedInteractionList l2p_new;
    std::vector<CompressedInteractionList> l2l_new;
    std::vector<CompressedInteractionList> d2e_new;

    CompressedInteractionList p2p_new;
    CompressedInteractionList p2l_new;
    CompressedInteractionList m2p_new;
    CompressedInteractionList m2l_new;

    FMMMat(TreeT obs_tree, std::vector<std::array<double,TreeT::dim>> obs_normals,
           TreeT src_tree, std::vector<std::array<double,TreeT::dim>> src_normals,
           FMMConfig<TreeT::dim> cfg, std::vector<std::array<double,TreeT::dim>> surf);

    int tensor_dim() const { return cfg.tensor_dim(); }
};

template <typename TreeT>
FMMMat<TreeT> fmmmmmmm(const TreeT& obs_tree,
    const std::vector<std::array<double,TreeT::dim>>& obs_normals,
    const TreeT& src_tree,
    const std::vector<std::array<double,TreeT::dim>>& src_normals,
    const FMMConfig<TreeT::dim>& cfg);
