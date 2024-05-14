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

__all__ = []  # import only for the side effects

import lsst.pex.exceptions
from lsst.utils import continueClass

from ._schema import Field, Schema
from ._table import SchemaMapper


@continueClass
class SchemaMapper:  # noqa: F811

    def addOutputField(self, field, type=None, doc=None, units="", size=None,
                       doReplace=False, parse_strict="raise"):
        """Add an un-mapped field to the output Schema.

        Parameters
        ----------
        field : `str` or `~lsst.afw.table.Field`
            The string name of the `Field`, or a fully-constructed
            `Field` object.  If the latter, all other arguments
            besides doReplace are ignored.
        type : `str`
            The type of field to create.  Valid types are the keys of the
            afw.table.Field dictionary.
        doc : `str`
            Documentation for the field.
        unit : `str`
            Units for the field, or an empty string if unitless.
        size : `int`
            Size of the field; valid for string and array fields only.
        doReplace : `bool`
            If a field with this name already exists, replace it instead of
            raising pex.exceptions.InvalidParameterError.
        parse_strict : `str`
            One of 'raise' (default), 'warn', or 'strict', indicating how to
            handle unrecognized unit strings.  See also astropy.units.Unit.

        Returns
        -------
        key : `~lsst.afw.table.Key`
            The key of the field added.
        """
        if isinstance(field, str):
            if size is None:
                field = Field[type](field, doc=doc, units=units,
                                    parse_strict=parse_strict)
            else:    
                field = Field[type](field, doc=doc, units=units,
                                    size=size, parse_strict=parse_strict)
        return field._addTo(self.editOutputSchema(), doReplace)

    def addMapping(self, input, output=None, doReplace=True):
        """Add a mapped field to the output schema.

        Parameters
        ----------
        input : `~lsst.afw.table.Key`
            A `Key` from the input schema whose values will be mapped to the new
            field.
        output : `str` or `~lsst.afw.table.Field`
            A `Field` object that describes the new field to be added to the
            output schema, or the name of the field (with documentation and
            units copied from the input schema).  May be None to copy everything
            from the input schema.
        doReplace : `bool`
            If a field with this name already exists in the output schema,
            replace it instead of raising `pex.exceptions.InvalidParameterError`.

        Returns
        -------
        key : `~lsst.afw.table.Key`
            The key for the new mapped field.
        """
        # Workaround for calling positional arguments; avoids an API change during pybind11 conversion,
        # but we should just make that change and encourage using kwargs in the
        # future.
        if output is True or output is False:
            doReplace = output
            output = None
        if output is None:
            return input._addMappingTo(self, doReplace)
        else:
            return input._addMappingTo(self, output, doReplace)

    def __eq__(self, other):
        """SchemaMappers are equal if their respective input and output
        schemas are identical, and they have the same mappings defined.

        Note: It was simpler to implement equality in python than in C++.
        """
        iSchema = self.getInputSchema()
        oSchema = self.getOutputSchema()
        if (not (iSchema.compare(other.getInputSchema(), Schema.IDENTICAL) == Schema.IDENTICAL
                 and oSchema.compare(other.getOutputSchema(), Schema.IDENTICAL) == Schema.IDENTICAL)):
            return False

        for item in iSchema:
            if self.isMapped(item.key) and other.isMapped(item.key):
                if (self.getMapping(item.key) == other.getMapping(item.key)):
                    continue
                else:
                    return False
            elif (not self.isMapped(item.key)) and (not other.isMapped(item.key)):
                continue
            else:
                return False

        return True

    def __reduce__(self):
        """To support pickle."""
        mappings = {}
        for item in self.getInputSchema():
            try:
                key = self.getMapping(item.key)
            except lsst.pex.exceptions.NotFoundError:
                # Not all fields may be mapped, so just continue if a mapping is not found.
                continue
            mappings[item.key] = self.getOutputSchema().find(key).field
        return (makeSchemaMapper, (self.getInputSchema(), self.getOutputSchema(), mappings))


def makeSchemaMapper(input, output, mappings):
    """Build a mapper from two Schemas and the mapping between them.
    For pickle support.

    Parameters
    ----------
    input : `lsst.afw.table.Schema`
        The input schema for the mapper.
    output : `lsst.afw.table.Schema`
        The output schema for the mapper.
    mappings : `dict` [`lsst.afw.table.Key`, `lsst.afw.table.Key`]
        The mappings to define between the input and output schema.

    Returns
    -------
    mapper : `lsst.afw.table.SchemaMapper`
        The constructed SchemaMapper.
    """
    mapper = SchemaMapper(input, output)
    for key, value in mappings.items():
        mapper.addMapping(key, value)
    return mapper
