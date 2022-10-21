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

// Helper class for jagged neighborhoods
class GCONeighborhood {
public:
  GCONeighborhood(SiteID num_sites, SiteID count[], SiteID site[],
                  EnergyValue weight[])
      : m_numSites{num_sites} {
    m_numNeighborsTotal = 0;
    m_numNeighbors = (SiteID *)count;

    m_neighborsIndexes = new SiteID *[m_numSites];
    m_neighborsWeights = new EnergyValue *[m_numSites];

    for (int i = 0; i < m_numSites; i++) {
      auto count_i = count[i];

      m_neighborsIndexes[i] = site + m_numNeighborsTotal;
      m_neighborsWeights[i] = weight + m_numNeighborsTotal;

      m_numNeighborsTotal += count_i;
    }
  }
  virtual ~GCONeighborhood() {
    // We don't own the pointers within these arrays
    // only delete the outer array
    delete[] m_neighborsIndexes;
    delete[] m_neighborsWeights;
  }

  SiteID *numNeighbors() const { return m_numNeighbors; }

  EnergyValue **neighborsWeights() const { return m_neighborsWeights; }

  SiteID **neighborsIndexes() const { return m_neighborsIndexes; }

  SiteID numSites() const { return m_numSites; }

private:
  SiteID **m_neighborsIndexes;
  EnergyValue **m_neighborsWeights;
  SiteID *m_numNeighbors; // holds num of neighbors for each site

  SiteID m_numNeighborsTotal; // holds total num of neighbor relationships
  SiteID m_numSites;          // number of sites
};

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
             return std::make_unique<GCONeighborhood>(
                 count.size(), count.mutable_data(), site.mutable_data(),
                 weight.mutable_data());
           }),
           py::keep_alive<1, 2>(), py::keep_alive<1, 3>(),
           py::keep_alive<1, 4>());

  py::class_<GCoptimization>(m, "GCOBase")
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
             py::array_t<LabelID, py::array::c_style> order) {
            graph.setLabelOrder(order.data(), order.size());
          },
          "order"_a)
      .def_property_readonly("label",
                             [](GCoptimization &g) {
                               py::array_t<SiteID> result{g.numSites()};
                               g.whatLabel(0, result.size(),
                                           result.mutable_data());
                               return result;
                             })
      .def(
          "set_data_cost",
          [](GCoptimization &graph,
             py::array_t<EnergyValue, py::array::c_style> data) {
            if (data.size() != graph.numSites() * graph.numLabels()) {
              throw std::invalid_argument(
                  "data size does not match graph size");
            }
            graph.setDataCost(data.mutable_data());
          },
          "cost"_a, py::keep_alive<1, 2>())
      .def("set_data_cost",
           static_cast<void (GCoptimization::*)(SiteID, LabelID, EnergyValue)>(
               &GCoptimization::setDataCost),
           "site"_a, "label"_a, "cost"_a)
      .def("set_smooth_cost",
           static_cast<void (GCoptimization::*)(LabelID, LabelID, EnergyValue)>(
               &GCoptimization::setSmoothCost),
           "site_1"_a, "site_2"_a, "cost"_a)
      .def(
          "set_smooth_cost",
          [](GCoptimization &graph,
             py::array_t<EnergyValue, py::array::c_style> data) {
            if (data.size() != graph.numLabels() * graph.numLabels()) {
              throw std::invalid_argument(
                  "data size does not match graph size");
            }
            graph.setSmoothCost(data.mutable_data());
          },
          "cost"_a, py::keep_alive<1, 2>())
      .def("set_label_cost",
           static_cast<void (GCoptimization::*)(EnergyValue)>(
               &GCoptimization::setLabelCost),
           "cost"_a)
      .def(
          "set_label_cost",
          [](GCoptimization &graph,
             py::array_t<EnergyValue, py::array::c_style> data) {
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
             py::array_t<LabelID, py::array::c_style> label, EnergyValue cost) {
            graph.setLabelSubsetCost(label.mutable_data(), label.size(), cost);
          },
          "label"_a, "cost"_a);
  py::class_<GCoptimizationGridGraph, GCoptimization>(m, "GCOGridGraph")
      .def(py::init<SiteID, SiteID, LabelID>(), "width"_a, "height"_a,
           "num_labels"_a)
      .def(
          "set_smooth_cost",
          [](GCoptimizationGridGraph &graph,
             py::array_t<EnergyValue, py::array::c_style> smooth,
             py::array_t<EnergyValue, py::array::c_style> vertical_cost,
             py::array_t<EnergyValue, py::array::c_style> horizontal_cost) {
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
          "smooth_cost"_a, "vertical_cost"_a, "horizontal_cost"_a, py::keep_alive<1, 2>());
  py::class_<GCoptimizationGeneralGraph, GCoptimization>(m, "GCOGeneralGraph")
      .def(py::init<SiteID, LabelID>(), "num_sites"_a, "num_labels"_a)
      .def("set_neighbors",
           static_cast<void (GCoptimizationGeneralGraph::*)(SiteID, SiteID,
                                                            EnergyValue)>(
               &GCoptimizationGeneralGraph::setNeighbors),
           "site_1"_a, "site_2"_a, "weight"_a)
      .def(
          "set_neighbors",
          [](GCoptimizationGeneralGraph &graph,
             const GCONeighborhood &neighborhood) {
            if (neighborhood.numSites() != graph.numSites()) {
              throw std::invalid_argument(
                  "neighborhood size does not match graph size");
            }

            graph.setAllNeighbors(neighborhood.numNeighbors(),
                                  neighborhood.neighborsIndexes(),
                                  neighborhood.neighborsWeights());
          },
          "neighborhood"_a, py::keep_alive<1, 2>());
}
