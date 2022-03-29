# gco
[![Tests](https://github.com/agoose77/gco-ext/actions/workflows/tests.yml/badge.svg)](https://github.com/agoose77/gco-ext/actions/workflows/tests.yml)

A Python interface to the [gco](https://github.com/agoose77/gco-v3.0) library (modified for repackaging from [the original source](https://vision.cs.uwaterloo.ca/code/))

The exposed `GCO<XXX>Graph` classes are available under the `gco` root namespace.

See the [research homepage](https://vision.cs.uwaterloo.ca/code/) for license and usage information.

## Neighborhood
Without the ability to easily create C++ arrays-of-arrays in Python, a `GCONeighborhood` class is exposed which builds pointers into flat arrays:
```python
neighborhood = gco.GCONeighborhood(count, site, cost)
```
where `count[i]` is the number of neighbours for site `i`, `site[j]` is the site of the jth cumulative neighbour, and `cost` the corresponding cost for such a neighbour.
