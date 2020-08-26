//
// Created by maksbh on 8/25/20.
//


#include <pybind11/pybind11.h>
#include <Input/InputData.h>
#include <Output/writeH5.h>

namespace py = pybind11;

PYBIND11_MODULE(Cy_RSoXS,module)
{
    module.doc() = "pybind11  plugin for Cy-RSoXS";
    py::class_<InputData>(module,"InputData")
            .def("Dimension",InputData::numX);
}