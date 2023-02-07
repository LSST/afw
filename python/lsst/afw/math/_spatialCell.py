# This file is part of afw.
#
# Developed for the LSST Data Management System.
# This product includes software developed by the LSST Project
# (https://www.lsst.org).
# See the COPYRIGHT file at the top-level directory of this distribution
# for details of code ownership.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

from lsst.pex.exceptions import NotFoundError
from ._math import (SpatialCellCandidate, SpatialCellCandidateIterator, SpatialCell, SpatialCellSet,
                    SpatialCellImageCandidate)

__all__ = []  # import this module only for its side effects


def spatialCellCandidateIter(self):
    while True:
        try:
            yield self.__deref__()
        except NotFoundError:
            return
        self.__incr__()


SpatialCellCandidateIterator.__iter__ = spatialCellCandidateIter


def spatialCellCandidateRepr(self):
    return (f"center=({self.getXCenter()},{self.getYCenter()}), "
            f"status={self.getStatus().name}, rating={self.getCandidateRating()}")


SpatialCellCandidate.__repr__ = spatialCellCandidateRepr


def spatialCellImageCandidateRepr(self):
    # NOTE: super doesn't work outside of class members, but this is only
    # single inheritance.
    string = (f"{SpatialCellCandidate.__repr__(self)}, size=({self.getWidth()}, {self.getHeight()}), "
              f"chi2={self.getChi2()}")
    return string


SpatialCellImageCandidate.__repr__ = spatialCellImageCandidateRepr


def spatialCellIter(self):
    return self.begin().__iter__()


SpatialCell.__iter__ = spatialCellIter


def spatialCellGetitem(self, idx):
    idx = int(idx)
    num_cells = len(self)
    if idx < -num_cells or idx >= num_cells:
        raise IndexError("idx={} < -{} or >= {})".format(idx,
                                                         num_cells, num_cells))
    if idx < 0:
        idx += num_cells
    for i, cell in enumerate(self):
        if i >= idx:
            return cell


SpatialCell.__getitem__ = spatialCellGetitem


def spatialCellRepr(self):
    candidates = "\n".join(f"({str(x)})" for x in self)
    # If there are no candidates, don't make a multi-line list.
    candidatesStr = f"\n{candidates}" if self.size() != 0 else ""
    return (f"{self.getLabel()}: bbox={self.getBBox()}, ignoreBad={self.getIgnoreBad()}, "
            f"candidates=[{candidatesStr}]")


SpatialCell.__repr__ = spatialCellRepr


def spatialCellSetRepr(self):
    cellsStr = "\n".join(str(cell) for cell in self.getCellList())
    return (f"bbox={self.getBBox()}, {len(self.getCellList())} cells\n{cellsStr}")


SpatialCellSet.__repr__ = spatialCellSetRepr
