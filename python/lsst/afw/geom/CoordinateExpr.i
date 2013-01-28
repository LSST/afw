/* 
 * LSST Data Management System
 * Copyright 2008, 2009, 2010 LSST Corporation.
 * 
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the LSST License Statement and 
 * the GNU General Public License along with this program.  If not, 
 * see <http://www.lsstcorp.org/LegalNotices/>.
 */

%module(package="lsst.afw.geom") geomLib
%include "CoordinateBase.i"

%{
#include "lsst/afw/geom/CoordinateExpr.h"
%}

%define %CoordinateExpr_POSTINCLUDE(N)
%template(CoordinateExprBase ## N) lsst::afw::geom::CoordinateBase<lsst::afw::geom::CoordinateExpr<N>,bool,N>;
%template(CoordinateExpr ## N) lsst::afw::geom::CoordinateExpr<N>;
%CoordinateBase_POSTINCLUDE(bool, N, lsst::afw::geom::CoordinateExpr<N>);
%enddef

%include "lsst/afw/geom/CoordinateExpr.h"

%CoordinateExpr_POSTINCLUDE(2);
%CoordinateExpr_POSTINCLUDE(3);
