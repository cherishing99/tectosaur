<% 
setup_pybind11(cfg)
cfg['compiler_args'].extend(['-std=c++14', '-O3', '-g', '-Wall', '-Werror', '-fopenmp'])
cfg['sources'] = ['fmm_impl.cpp', 'kdtree.cpp', 'blas_wrapper.cpp', 'cpp_tester.cpp', 'fmm_kernels.cpp']
cfg['dependencies'] = ['fmm_impl.hpp', 'kdtree.hpp', 'blas_wrapper.hpp', '../lib/pybind11_nparray.hpp']
cfg['parallel'] = True
cfg['linker_args'] = ['-fopenmp']
cfg['include_dirs'].append('../')

import numpy as np
blas = np.__config__.blas_opt_info
cfg['library_dirs'] = blas['library_dirs']
cfg['libraries'] = blas['libraries']

import os
import mako.template
import mako.exceptions
#TODO: GENERALIZE THIS?
templates = ['fmm_kernels.thpp', 'fmm_kernels.tcpp']
for t in templates:
    cfg['dependencies'].append(t)
    filepath = os.path.join(filedirname, t)
    tmpl = mako.template.Template(filename = filepath)
    try:
        code = tmpl.render()
    except Exception as e:
        raise e
    dirname = os.path.dirname(t)
    filename,ext = os.path.splitext(t)
    out_filename = os.path.join(filedirname, filename + '.' + ext[2:])
    print(out_filename)
    with open(out_filename, 'w') as f:
        f.write(code)

with open(os.path.join(filedirname, '.gitignore'), 'w') as f:
    f.write('# autogenerated. please do not modify\n')
    f.write('.gitignore\n')
    for t in templates:
        filename,ext = os.path.splitext(t)
        f.write(filename + '.' + ext[2:] + '\n')
%>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include "lib/pybind11_nparray.hpp"

#include "fmm_impl.hpp"
#include "kdtree.hpp"

using namespace tectosaur;
namespace py = pybind11;


int main(int,char**);


void check_shape(NPArrayD& arr) {
    auto buf = arr.request();
    if (buf.ndim != 2 || buf.shape[1] != 3) {
        throw std::runtime_error("parameter requires n x 3 array.");
    }
}


PYBIND11_PLUGIN(_fmm) {
    py::module m("_fmm");

    py::class_<Sphere>(m, "Sphere")
        .def_readonly("r", &Sphere::r)
        .def_readonly("center", &Sphere::center);

    py::class_<KDNode>(m, "KDNode")
        .def_readonly("start", &KDNode::start)
        .def_readonly("end", &KDNode::end)
        .def_readonly("bounds", &KDNode::bounds)
        .def_readonly("is_leaf", &KDNode::is_leaf)
        .def_readonly("idx", &KDNode::idx)
        .def_readonly("height", &KDNode::height)
        .def_readonly("depth", &KDNode::depth)
        .def_readonly("children", &KDNode::children);

    py::class_<KDTree>(m, "KDTree")
        .def("__init__", 
        [] (KDTree& kd, NPArrayD np_pts, NPArrayD np_normals, size_t n_per_cell) {
            check_shape(np_pts);
            check_shape(np_normals);
            new (&kd) KDTree(
                reinterpret_cast<Vec3*>(np_pts.request().ptr),
                reinterpret_cast<Vec3*>(np_normals.request().ptr),
                np_pts.request().shape[0], n_per_cell
            );
        })
        .def_readonly("nodes", &KDTree::nodes)
        .def_readonly("pts", &KDTree::pts)
        .def_readonly("normals", &KDTree::normals)
        .def_readonly("orig_idxs", &KDTree::orig_idxs)
        .def_readonly("max_height", &KDTree::max_height);

    py::class_<BlockSparseMat>(m, "BlockSparseMat")
        .def("get_nnz", &BlockSparseMat::get_nnz)
        .def("matvec", [] (BlockSparseMat& s, NPArrayD v, size_t n_rows) {
            auto out = s.matvec(reinterpret_cast<double*>(v.request().ptr), n_rows);
            return array_from_vector(out);
        });

    py::class_<FMMConfig>(m, "FMMConfig")
        .def("__init__", 
            [] (FMMConfig& cfg, double equiv_r,
                double check_r, size_t order, std::string k_name,
                NPArrayD params) 
            {
                new (&cfg) FMMConfig{
                    equiv_r, check_r, order, get_by_name(k_name),
                    get_vector<double>(params)                                    
                };
            }
        );

    py::class_<NewMatrixFreeOp>(m, "NewMatrixFreeOp")
        .def_readonly("obs_n_start", &NewMatrixFreeOp::obs_n_start)
        .def_readonly("obs_n_end", &NewMatrixFreeOp::obs_n_end)
        .def_readonly("obs_n_idx", &NewMatrixFreeOp::obs_n_idx)
        .def_readonly("src_n_start", &NewMatrixFreeOp::src_n_start)
        .def_readonly("src_n_end", &NewMatrixFreeOp::src_n_end)
        .def_readonly("src_n_idx", &NewMatrixFreeOp::src_n_idx);

    py::class_<FMMMat>(m, "FMMMat")
        .def_readonly("obs_tree", &FMMMat::obs_tree)
        .def_readonly("src_tree", &FMMMat::src_tree)
        .def_readonly("p2p", &FMMMat::p2p_new)
        .def_readonly("cfg", &FMMMat::cfg)
        .def_readonly("translation_surface_order", &FMMMat::translation_surface_order)
        .def_readonly("uc2e", &FMMMat::uc2e)
        .def_readonly("dc2e", &FMMMat::dc2e)
        .def_property_readonly("tensor_dim", &FMMMat::tensor_dim)
        .def("p2p_eval", [] (FMMMat& m, NPArrayD v) {
            auto* ptr = reinterpret_cast<double*>(v.request().ptr);
            return array_from_vector(m.p2p_eval(ptr));
        })
        .def("eval", [] (FMMMat& m, NPArrayD v) {
            auto* ptr = reinterpret_cast<double*>(v.request().ptr);
            return array_from_vector(m.eval(ptr));
        });

    m.def("fmmmmmmm", &fmmmmmmm);

    m.def("direct_eval", [](std::string k_name, NPArrayD obs_pts, NPArrayD obs_ns,
                            NPArrayD src_pts, NPArrayD src_ns, NPArrayD params) {
        check_shape(obs_pts);
        check_shape(obs_ns);
        check_shape(src_pts);
        check_shape(src_ns);
        auto K = get_by_name(k_name);
        std::vector<double> out(K.tensor_dim * K.tensor_dim *
                                obs_pts.request().shape[0] *
                                src_pts.request().shape[0]);
        K.f({as_ptr<Vec3>(obs_pts), as_ptr<Vec3>(obs_ns),
           as_ptr<Vec3>(src_pts), as_ptr<Vec3>(src_ns),
           obs_pts.request().shape[0], src_pts.request().shape[0],
           as_ptr<double>(params)},
          out.data());
        return array_from_vector(out);
    });

    m.def("run_tests", [] (std::vector<std::string> str_args) { 
        char** argv = new char*[str_args.size()];
        for (size_t i = 0; i < str_args.size(); i++) {
            argv[i] = const_cast<char*>(str_args[i].c_str());
        }
        main(str_args.size(), argv); 
        delete[] argv;
    });
    
    return m.ptr();
}
