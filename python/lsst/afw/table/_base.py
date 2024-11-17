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
import numpy as np

from lsst.utils import continueClass, TemplateMeta
from ._table import BaseRecord, BaseCatalog
from ._schema import Key


__all__ = ["Catalog"]


@continueClass
class BaseRecord:  # noqa: F811

    def extract(self, *patterns, **kwargs):
        """Extract a dictionary of {<name>: <field-value>} in which the field
        names match the given shell-style glob pattern(s).

        Any number of glob patterns may be passed; the result will be the union
        of all the result of each glob considered separately.

        Parameters
        ----------
        items : `dict`
            The result of a call to self.schema.extract(); this will be used
            instead of doing any new matching, and allows the pattern matching
            to be reused to extract values from multiple records.  This
            keyword is incompatible with any position arguments and the regex,
            sub, and ordered keyword arguments.
        regex : `str` or `re` pattern object
            A regular expression to be used in addition to any glob patterns
            passed as positional arguments.  Note that this will be compared
            with re.match, not re.search.
        sub : `str`
            A replacement string (see `re.MatchObject.expand`) used to set the
            dictionary keys of any fields matched by regex.
        ordered : `bool`
            If `True`, a `collections.OrderedDict` will be returned instead of
            a standard dict, with the order corresponding to the definition
            order of the `Schema`. Default is `False`.
        """
        d = kwargs.pop("items", None)
        if d is None:
            d = self.schema.extract(*patterns, **kwargs).copy()
        elif kwargs:
            kwargsStr = ", ".join(kwargs.keys())
            raise ValueError(f"Unrecognized keyword arguments for extract: {kwargsStr}")
        return {name: self.get(schemaItem.key) for name, schemaItem in d.items()}

    def __repr__(self):
        return f"{type(self)}\n{self}"


class Catalog(metaclass=TemplateMeta):

    def getColumnView(self):
        self._columns = self._getColumnView()
        return self._columns

    def __getColumns(self):
        if not hasattr(self, "_columns") or self._columns is None:
            self._columns = self._getColumnView()
        return self._columns
    columns = property(__getColumns, doc="a column view of the catalog")

    def __getitem__(self, key):
        """Return the record at index key if key is an integer,
        return a column if `key` is a string field name or Key,
        or return a subset of the catalog if key is a slice
        or boolean NumPy array.
        """
        if type(key) is slice:
            (start, stop, step) = (key.start, key.stop, key.step)
            if step is None:
                step = 1
            if start is None:
                start = 0
            if stop is None:
                stop = len(self)
            return self.subset(start, stop, step)
        elif isinstance(key, np.ndarray):
            if key.dtype == bool:
                return self.subset(key)
            raise RuntimeError(f"Unsupported array type for indexing non-contiguous Catalog: {key.dtype}")
        elif isinstance(key, str):
            key = self.schema.find(key).key
            result, self._columns = self._get_column_from_key(key, self._columns)
            return result
        elif isinstance(key, Key):
            result, self._columns = self._get_column_from_key(key, self._columns)
            return result
        else:
            return self._getitem_(key)

    def __setitem__(self, key, value):
        """If ``key`` is an integer, set ``catalog[key]`` to
        ``value``. Otherwise select column ``key`` and set it to
        ``value``.
        """
        self._columns = None
        if isinstance(key, str):
            key = self.schema[key].asKey()
        if isinstance(key, Key):
            if isinstance(key, Key["Flag"]):
                self._set_flag(key, value)
            else:
                self.columns[key] = value
        else:
            return self.set(key, value)

    def __delitem__(self, key):
        self._columns = None
        if isinstance(key, slice):
            self._delslice_(key)
        else:
            self._delitem_(key)

    def append(self, record):
        self._columns = None
        self._append(record)

    def insert(self, key, value):
        self._columns = None
        self._insert(key, value)

    def clear(self):
        self._columns = None
        self._clear()

    def addNew(self):
        self._columns = None
        return self._addNew()

    def cast(self, type_, deep=False):
        """Return a copy of the catalog with the given type.

        Parameters
        ----------
        type_ :
            Type of catalog to return.
        deep : `bool`, optional
            If `True`, clone the table and deep copy all records.

        Returns
        -------
        copy :
            Copy of catalog with the requested type.
        """
        if deep:
            table = self.table.clone()
            table.preallocate(len(self))
        else:
            table = self.table
        copy = type_(table)
        copy.extend(self, deep=deep)
        return copy

    def copy(self, deep=False):
        """
        Copy a catalog (default is not a deep copy).
        """
        return self.cast(type(self), deep)

    def extend(self, iterable, deep=False, mapper=None):
        """Append all records in the given iterable to the catalog.

        Parameters
        ----------
        iterable :
            Any Python iterable containing records.
        deep : `bool`, optional
            If `True`, the records will be deep-copied; ignored if
            mapper is not `None` (that always implies `True`).
        mapper : `lsst.afw.table.schemaMapper.SchemaMapper`, optional
            Used to translate records.
        """
        self._columns = None
        # We can't use isinstance here, because the SchemaMapper symbol isn't available
        # when this code is part of a subclass of Catalog in another package.
        if type(deep).__name__ == "SchemaMapper":
            mapper = deep
            deep = None
        if isinstance(iterable, type(self)):
            if mapper is not None:
                self._extend(iterable, mapper)
            else:
                self._extend(iterable, deep)
        else:
            for record in iterable:
                if mapper is not None:
                    self._append(self.table.copyRecord(record, mapper))
                elif deep:
                    self._append(self.table.copyRecord(record))
                else:
                    self._append(record)

    def __reduce__(self):
        import lsst.afw.fits
        return lsst.afw.fits.reduceToFits(self)

    def asAstropy(self, cls=None, copy=False, unviewable="copy"):
        """Return an astropy.table.Table (or subclass thereof) view into this catalog.

        Parameters
        ----------
        cls :
            Table subclass to use; `None` implies `astropy.table.Table`
            itself.  Use `astropy.table.QTable` to get Quantity columns.
        copy : bool, optional
            If `True`, copy data from the LSST catalog to the astropy
            table.  Not copying is usually faster, but can keep memory
            from being freed if columns are later removed from the
            Astropy view.
        unviewable : `str`, optional
            One of the following options (which is ignored if
            copy=`True` ), indicating how to handle field types (`str`
            and `Flag`) for which views cannot be constructed:

            - 'copy' (default): copy only the unviewable fields.
            - 'raise': raise ValueError if unviewable fields are present.
            - 'skip': do not include unviewable fields in the Astropy Table.

        Returns
        -------
        cls : `astropy.table.Table`
            Astropy view into the catalog.

        Raises
        ------
        ValueError
            Raised if the `unviewable` option is not a known value, or
            if the option is 'raise' and an uncopyable field is found.

        """
        import astropy.table
        if cls is None:
            cls = astropy.table.Table
        if unviewable not in ("copy", "raise", "skip"):
            raise ValueError(
                f"'unviewable'={unviewable!r} must be one of 'copy', 'raise', or 'skip'")
        ps = self.getMetadata()
        meta = ps.toOrderedDict() if ps is not None else None
        columns = []
        items = self.schema.extract("*", ordered=True)
        for name, item in items.items():
            key = item.key
            unit = item.field.getUnits() or None  # use None instead of "" when empty
            if key.getTypeString() == "String":
                if not copy:
                    if unviewable == "raise":
                        raise ValueError("Cannot extract string "
                                         "unless copy=True or unviewable='copy' or 'skip'.")
                    elif unviewable == "skip":
                        continue
                data = np.zeros(
                    len(self), dtype=np.dtype((str, key.getSize())))
                for i, record in enumerate(self):
                    data[i] = record.get(key)
            elif key.getTypeString() == "Flag":
                if not copy:
                    if unviewable == "raise":
                        raise ValueError("Cannot extract packed bit columns "
                                         "unless copy=True or unviewable='copy' or 'skip'.")
                    elif unviewable == "skip":
                        continue
                data = self[key]
            elif key.getTypeString() == "Angle":
                data = self.columns.get(key)
                unit = "radian"
                if copy:
                    data = data.copy()
            elif "Array" in key.getTypeString() and key.isVariableLength():
                # Can't get columns for variable-length array fields.
                if unviewable == "raise":
                    raise ValueError("Cannot extract variable-length array fields unless unviewable='skip'.")
                elif unviewable == "skip" or unviewable == "copy":
                    continue
            else:
                data = self.columns.get(key)
                if copy:
                    data = data.copy()
            columns.append(
                astropy.table.Column(
                    data,
                    name=name,
                    unit=unit,
                    description=item.field.getDoc()
                )
            )
        return cls(columns, meta=meta, copy=False)

    def __dir__(self):
        """
        This custom dir is necessary due to the custom getattr below.
        Without it, not all of the methods available are returned with dir.
        See DM-7199.
        """
        def recursive_get_class_dir(cls):
            """
            Return a set containing the names of all methods
            for a given class *and* all of its subclasses.
            """
            result = set()
            if cls.__bases__:
                for subcls in cls.__bases__:
                    result |= recursive_get_class_dir(subcls)
            result |= set(cls.__dict__.keys())
            return result
        return sorted(set(dir(self.columns)) | set(dir(self.table))
                      | recursive_get_class_dir(type(self)) | set(self.__dict__.keys()))

    def __getattr__(self, name):
        # Catalog forwards unknown method calls to its table and column view
        # for convenience.  (Feature requested by RHL; complaints about magic
        # should be directed to him.)
        if name == "_columns":
            self._columns = None
            return None
        try:
            return getattr(self.table, name)
        except AttributeError:
            return getattr(self.columns, name)

    def __str__(self):
        if self.isContiguous():
            return str(self.asAstropy())
        else:
            fields = ' '.join(x.field.getName() for x in self.schema)
            return f"Non-contiguous afw.Catalog of {len(self)} rows.\ncolumns: {fields}"

    def __repr__(self):
        return "%s\n%s" % (type(self), self)

    def extract(self, *patterns, **kwds):
        """Extract a dictionary of {<name>: <column-array>} in which the field
        names match the given shell-style glob pattern(s).

        Any number of glob patterns may be passed (including none); the result
        will be the union of all the result of each glob considered separately.

        Note that extract("*", copy=True) provides an easy way to transform a
        catalog into a set of writeable contiguous NumPy arrays.

        This routines unpacks `Flag` columns into full boolean arrays.  String
        fields are silently ignored.

        Parameters
        ----------
        patterns : Array of `str`
            List of glob patterns to use to select field names.
        kwds : `dict`
            Dictionary of additional keyword arguments.  May contain:

            ``items`` : `list`
                The result of a call to self.schema.extract(); this will be
                used instead of doing any new matching, and allows the pattern
                matching to be reused to extract values from multiple records.
                This keyword is incompatible with any position arguments and
                the regex, sub, and ordered keyword arguments.
            ``where`` : array index expression
                Any expression that can be passed as indices to a NumPy array,
                including slices, boolean arrays, and index arrays, that will
                be used to index each column array.  This is applied before
                arrays are copied when copy is True, so if the indexing results
                in an implicit copy no unnecessary second copy is performed.
            ``copy`` : `bool`
                If True, the returned arrays will be contiguous copies rather
                than strided views into the catalog.  This ensures that the
                lifetime of the catalog is not tied to the lifetime of a
                particular catalog, and it also may improve the performance if
                the array is used repeatedly. Default is False.  Copies are
                always made if the catalog is noncontiguous, but if
                ``copy=False`` these set as read-only to ensure code does not
                assume they are views that could modify the original catalog.
            ``regex`` : `str` or `re` pattern
                A regular expression to be used in addition to any glob
                patterns passed as positional arguments.  Note that this will
                be compared with re.match, not re.search.
            ``sub`` : `str`
                A replacement string (see re.MatchObject.expand) used to set
                the dictionary keys of any fields matched by regex.
            ``ordered`` : `bool`
                If True, a collections.OrderedDict will be returned instead of
                a standard dict, with the order corresponding to the definition
                order of the Schema. Default is False.

        Returns
        -------
        d : `dict`
            Dictionary of extracted name-column array sets.

        Raises
        ------
        ValueError
            Raised if a list of ``items`` is supplied with additional keywords.
        """
        copy = kwds.pop("copy", False)
        where = kwds.pop("where", None)
        d = kwds.pop("items", None)
        # If ``items`` is given as a kwd, an extraction has already been
        # performed and there shouldn't be any additional keywords. Otherwise
        # call schema.extract to load the dictionary.
        if d is None:
            d = self.schema.extract(*patterns, **kwds).copy()
        elif kwds:
            raise ValueError(
                "kwd 'items' was specified, which is not compatible with additional keywords")

        def processArray(a):
            if where is not None:
                a = a[where]
            if copy:
                a = a.copy()
            return a

        # must use list because we might be adding/deleting elements
        for name, schemaItem in list(d.items()):
            key = schemaItem.key
            if key.getTypeString() == "String":
                del d[name]
            else:
                d[name] = processArray(self[schemaItem.key])
        return d


Catalog.register("Base", BaseCatalog)
