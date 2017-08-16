<%
from tectosaur.util.build_cfg import setup_module
setup_module(cfg)
cfg['dependencies'].extend(['../include/pybind11_nparray.hpp'])
%> 

#include "../include/pybind11_nparray.hpp"

namespace py = pybind11;

py::tuple make_bsr_matrix(size_t n_rows, size_t n_cols, size_t blockrows, size_t blockcols,
        NPArray<double> in_data, NPArray<long> rows, NPArray<long> cols) 
{
    auto n_row_blocks = n_rows / blockrows;
    auto n_blocks = rows.request().shape[0];
    auto blocksize = blockrows * blockcols;

    auto* rows_ptr = as_ptr<long>(rows);
    auto* cols_ptr = as_ptr<long>(cols);
    auto* in_data_ptr = as_ptr<double>(in_data);

    auto indptr = make_array<int>({n_row_blocks + 1});
    auto indices = make_array<int>({n_blocks});
    auto data = make_array<double>({n_blocks, blockrows, blockcols});

    auto* indptr_ptr = as_ptr<int>(indptr);
    auto* indices_ptr = as_ptr<int>(indices);
    auto* data_ptr = as_ptr<double>(data);

    std::fill(indptr_ptr, indptr_ptr + n_row_blocks, 0.0);

    for (size_t i = 0; i < n_blocks; i++) {
        indptr_ptr[rows_ptr[i]]++;
    }

    for (size_t i = 0, cumsum = 0; i < n_row_blocks; i++) {
        int temp = indptr_ptr[i];
        indptr_ptr[i] = cumsum;
        cumsum += temp;
    }
    indptr_ptr[n_row_blocks] = n_blocks;

    for (size_t n = 0; n < n_blocks; n++) {
        int row = rows_ptr[n];
        int dest = indptr_ptr[row];

        indices_ptr[dest] = cols_ptr[n];
        for (size_t k = 0; k < blocksize; k++) {
            data_ptr[dest * blocksize + k] = in_data_ptr[n * blocksize + k];
        }

        indptr_ptr[row]++;
    }

    for (size_t i = 0, last = 0; i <= n_row_blocks; i++) {
        int temp = indptr_ptr[i];
        indptr_ptr[i] = last;
        last = temp;
    }
    return py::make_tuple(data, indices, indptr);
}

PYBIND11_PLUGIN(fast_assembly) {
    pybind11::module m("fast_assembly", "");
    m.def("make_bsr_matrix", &make_bsr_matrix);
    return m.ptr();
}
