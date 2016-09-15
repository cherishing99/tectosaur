<%
setup_pybind11(cfg)
cfg['compiler_args'].extend(['-std=c++14', '-O3', '-Wall'])
cfg['sources'] = ['cubature/hcubature.c', 'cubature/pcubature.c']
cfg['dependencies'] = ['adaptive_integrate.hpp']
%>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "adaptive_integrate.hpp"

namespace py = pybind11;

PYBIND11_PLUGIN(adaptive_integrator) {
    py::module m("adaptive_integrator");

    m.def("integrate",
        [] (std::string k_name, std::array<std::array<double,3>,3> tri,
            double tol, double eps, double sm, double pr) 
        {
            std::map<std::string,Kernel> Ks;
            Ks["U"] = Ukernel;
            Ks["T"] = Tkernel;
            Ks["A"] = Akernel;
            Ks["H"] = Hkernel;
            Data d(tol, false, Ks[k_name], tri, eps, sm, pr);
            auto result = integrate(d);
            return result;
        }
    );

    return m.ptr();
}