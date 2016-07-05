#pragma once

#include "octree.hpp"
#include <cmath>
#include <functional>
#include <memory>

namespace tectosaur {

struct Kernel {
    using FType = std::function<double(const Vec3&,const Vec3&)>;
    FType f;

    void direct_nbody(const Vec3* obs_pts, const Vec3* src_pts,
        size_t n_obs, size_t n_src, double* out) const;
};

double one(const Vec3&,const Vec3&);
double inv_r(const Vec3&,const Vec3&);

struct FMMConfig {
    // The MAC needs to < (1.0 / (check_r - 1)) so that farfield
    // approximations aren't used when the check surface intersects the 
    // target box. How about we just use that as our MAC, since errors 
    // further from the check surface should flatten out!
    double inner_r;
    double outer_r;
    std::vector<Vec3> surf;
    Kernel kernel;
};

struct Block {
    size_t row_start;
    size_t col_start;
    int n_rows;
    int n_cols;
    size_t data_start;
};

struct BlockSparseMat {
    std::vector<Block> blocks;
    std::vector<double> vals;

    std::vector<double> matvec(double* vec, size_t out_size);
    size_t get_nnz() { return vals.size(); }
};

struct FMMMat {
    BlockSparseMat p2p;
    BlockSparseMat p2m;
    BlockSparseMat p2l;
    BlockSparseMat m2p;
    std::vector<BlockSparseMat> m2m;
    BlockSparseMat m2l;
    BlockSparseMat l2p;
    std::vector<BlockSparseMat> l2l;

    std::vector<BlockSparseMat> uc2e;
    std::vector<BlockSparseMat> dc2e;
};

FMMMat fmmmmmmm(const KDTree& obs_tree, const KDTree& src_tree, const FMMConfig& cfg);

} //end namespace tectosaur
