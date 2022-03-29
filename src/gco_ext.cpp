#include <GCoptimization.h>

#include <iostream>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

namespace py = pybind11;
using namespace pybind11::literals;
using namespace GCO;

using SiteID = GCoptimization::SiteID;
using LabelID = GCoptimization::LabelID;
using EnergyValue = GCoptimization::EnergyTermType;

PYBIND11_MODULE(gco_ext, m) {
  static py::exception<GCException> exc(m, "GCException");
  py::register_exception_translator([](std::exception_ptr p) {
    try {
      if (p)
        std::rethrow_exception(p);
    } catch (const GCException &e) {
      exc(e.message);
    }
  });

  py::class_<GCONeighborhood>(m, "GCONeighborhood")
      .def(py::init([](py::array_t<SiteID, py::array::c_style> count,
                       py::array_t<SiteID, py::array::c_style> site,
                       py::array_t<EnergyValue, py::array::c_style> weight) {
             if (site.ndim() != 1 || weight.ndim() != 1 || count.ndim() != 1 ||
                 site.size() != weight.size()) {
               throw std::invalid_argument(
                   "data size does not match graph size");
             }
             return std::unique_ptr<GCONeighborhood>(new GCONeighborhood(
                 count.size(), count.mutable_data(), site.mutable_data(),
                 weight.mutable_data()));
           }),
           py::keep_alive<1, 2>(), py::keep_alive<1, 3>(),
           py::keep_alive<1, 4>());

  py::class_<GCoptimization>(m, "GCoptimization")
      .def("expansion", &GCoptimization::expansion, "max_num_iterations"_a)
      .def("alpha_expansion", &GCoptimization::alpha_expansion, "label"_a)
      .def("swap", &GCoptimization::swap, "max_num_iterations"_a)
      .def("alpha_beta_swap",
           static_cast<void (GCoptimization::*)(LabelID, LabelID)>(
               &GCoptimization::alpha_beta_swap),
           "alpha_label"_a, "beta_label"_a)
      .def_property_readonly("num_sites", &GCoptimization::numSites)
      .def_property_readonly("num_labels", &GCoptimization::numLabels)
      .def_property_readonly("label_energy", &GCoptimization::giveLabelEnergy)
      .def_property_readonly("data_energy", &GCoptimization::giveDataEnergy)
      .def_property_readonly("smooth_energy", &GCoptimization::giveSmoothEnergy)
      .def_property_readonly("energy", &GCoptimization::compute_energy)
      .def_property("verbosity", &GCoptimization::verbosity,
                    &GCoptimization::setVerbosity)
      .def_property("random_label_order", &GCoptimization::randomLabelOrder,
                    static_cast<void (GCoptimization::*)(bool)>(
                        &GCoptimization::setLabelOrder))
      .def("get_label",
           static_cast<LabelID (GCoptimization::*)(SiteID)>(
               &GCoptimization::whatLabel),
           "site"_a)
      .def("set_label",
           static_cast<void (GCoptimization::*)(SiteID, LabelID)>(
               &GCoptimization::setLabel),
           "site"_a, "label"_a)
      .def(
          "set_label_order",
          [](GCoptimization &graph,
             py::array_t<LabelID, py::array::c_style | py::array::forcecast>
                 order) { graph.setLabelOrder(order.data(), order.size()); },
          "order"_a)
      .def_property(
          "label",
          [](GCoptimization &g) {
            py::array_t<SiteID> result{g.numSites()};
            g.whatLabel(0, result.size(), result.mutable_data());
            return result;
          },
          [](GCoptimization &g,
             py::array_t<SiteID, py::array::c_style | py::array::forcecast>
                 label) { g.whatLabel(0, label.size(), label.mutable_data()); })
      .def(
          "set_data_cost",
          [](GCoptimization &graph,
             py::array_t<EnergyValue, py::array::c_style | py::array::forcecast>
                 data) {
            if (data.size() != graph.numSites() * graph.numLabels()) {
              throw std::invalid_argument(
                  "data size does not match graph size");
            }
            graph.setDataCost(data.mutable_data());
          },
          "cost"_a)
      .def("set_data_cost",
           static_cast<void (GCoptimization::*)(SiteID, LabelID, EnergyValue)>(
               &GCoptimization::setDataCost),
           "site"_a, "label"_a, "cost"_a)
      .def("set_smooth_cost",
           static_cast<void (GCoptimization::*)(SiteID, LabelID, EnergyValue)>(
               &GCoptimization::setSmoothCost),
           "site"_a, "label"_a, "cost"_a)
      .def(
          "set_smooth_cost",
          [](GCoptimization &graph,
             py::array_t<EnergyValue, py::array::c_style | py::array::forcecast>
                 data) {
            if (data.size() != graph.numLabels() * graph.numLabels()) {
              throw std::invalid_argument(
                  "data size does not match graph size");
            }
            graph.setSmoothCost(data.mutable_data());
          },
          "cost"_a)
      .def("set_label_cost",
           static_cast<void (GCoptimization::*)(EnergyValue)>(
               &GCoptimization::setLabelCost),
           "cost"_a)
      .def(
          "set_label_cost",
          [](GCoptimization &graph,
             py::array_t<EnergyValue, py::array::c_style | py::array::forcecast>
                 data) {
            if (data.size() != graph.numLabels()) {
              throw std::invalid_argument(
                  "data size does not match graph size");
            }
            graph.setLabelCost(data.mutable_data());
          },
          "cost"_a)
      .def(
          "set_label_cost",
          [](GCoptimization &graph,
             py::array_t<LabelID, py::array::c_style | py::array::forcecast>
                 label,
             EnergyValue cost) {
            graph.setLabelSubsetCost(label.mutable_data(), label.size(), cost);
          },
          "label"_a, "cost"_a);
  py::class_<GCoptimizationGridGraph, GCoptimization>(m,
                                                      "GCoptimizationGridGraph")
      .def(py::init<SiteID, SiteID, LabelID>(), "width"_a, "height"_a,
           "num_labels"_a)
      .def(
          "setSmoothCostVH",
          [](GCoptimizationGridGraph &graph,
             py::array_t<EnergyValue, py::array::c_style | py::array::forcecast>
                 smooth,
             py::array_t<EnergyValue, py::array::c_style | py::array::forcecast>
                 vertical_cost,
             py::array_t<EnergyValue, py::array::c_style | py::array::forcecast>
                 horizontal_cost) {
            if (vertical_cost.size() != graph.numSites() ||
                horizontal_cost.size() != graph.numSites() ||
                smooth.size() != graph.numLabels() * graph.numLabels()) {
              throw std::invalid_argument(
                  "data size does not match graph size");
            }
            graph.setSmoothCostVH(smooth.mutable_data(),
                                  vertical_cost.mutable_data(),
                                  horizontal_cost.mutable_data());
          },
          "smooth_cost"_a, "vertical_cost"_a, "horizontal_cost"_a);
  py::class_<GCoptimizationGeneralGraph, GCoptimization>(
      m, "GCoptimizationGeneralGraph")
      .def(py::init<SiteID, LabelID>(), "num_sites"_a, "num_labels"_a)
      .def(
          "set_neighbors",
          [](GCoptimizationGeneralGraph &g,
             py::array_t<SiteID, py::array::c_style | py::array::forcecast>
                 site_1,
             py::array_t<SiteID, py::array::c_style | py::array::forcecast>
                 site_2,
             py::array_t<EnergyValue, py::array::c_style | py::array::forcecast>
                 weight) {
            if (site_1.ndim() != 1 || site_2.ndim() != 1 || weight.ndim() != 1)
              throw std::runtime_error("Incompatible buffer dimension!");

            if (site_1.size() != site_2.size() ||
                site_2.size() != weight.size())
              throw std::runtime_error("Incompatible buffer size!");

            auto site_1_data = site_1.unchecked<1>();
            auto site_2_data = site_2.unchecked<1>();
            auto weight_data = weight.unchecked<1>();
            for (auto i = 0; i < site_1.size(); i++) {
              g.setNeighbors(site_1_data[i], site_2_data[i], weight_data[i]);
            }
          },
          "site_1"_a, "site_2"_a, "weight"_a)
      .def("set_neighbors", &GCoptimizationGeneralGraph::setNeighbors,
           "site_1"_a, "site_2"_a, "weight"_a);
}
