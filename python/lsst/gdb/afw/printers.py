import gdb
import math
import re
import sys
import lets_break_this_bad_boy

try:
    debug  # noqa: F821
except Exception:
    debug = False

import optparse
argparse = None                         # we're using optparse


lets_break_this_bad_boy.options(2)


class GdbOptionParser(optparse.OptionParser):
    """A subclass of the standard optparse OptionParser for gdb

GdbOptionParser raises GdbError rather than exiting when asked for help, or
when given an illegal value. E.g.

parser = gdb.printing.GdbOptionParser("show image")
parser.add_option("-a", "--all", action="store_true",
                  help="Display the whole image")
parser.add_option("-w", "--width", type="int", default=8,
                  help="Field width for pixels")

opts, args =  parser.parse_args(args)
"""

    def __init__(self, prog, *args, **kwargs):
        """
Like optparse.OptionParser's API, but with an initial command name argument
"""
        # OptionParser is an old-style class, so no super
        if not kwargs.get("prog"):
            kwargs["prog"] = prog
        optparse.OptionParser.__init__(self, *args, **kwargs)

    def parse_args(self, args, values=None):
        """Call OptionParser.parse_args after running gdb.string_to_argv"""
        if args is None:            # defaults to sys.argv
            args = ""
        try:
            args = gdb.string_to_argv(args)
        except TypeError:
            pass

        help = ("-h" in args or "--help" in args)
        opts, args = optparse.OptionParser.parse_args(self, args, values)
        opts.help = help
        if help:
            args = []

        return opts, args

    def exit(self, status=0, msg=""):
        """Raise GdbError rather than exiting"""
        if status == 0:
            if msg:
                print(msg, file=sys.stderr)
        else:
            raise gdb.GdbError(msg)


try:
    import gdb.printing

    class SharedPtrPrinter:
        "Print a shared_ptr"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            if self.val["px"]:
                return "shared_ptr(%s)" % self.val["px"].dereference()
            else:
                return "NULL"

    class GilPixelPrinter:
        "Print a boost::gil pixel"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            import pdb
            pdb.set_trace()
            return self.val["_v0"]

    def getEigenMatrixDimensions(val):
        m_storage = val["m_storage"]
        try:
            nx, ny = m_storage["m_cols"], m_storage["m_rows"]
        except gdb.error:           # only available for dynamic Matrices
            try:
                nx, ny = val.type.template_argument(1), \
                    val.type.template_argument(2)
            except RuntimeError:
                # should get dimens from template, but that's gdb bug #11060
                size = m_storage["m_data"]["array"].type.sizeof
                size0 = m_storage["m_data"]["array"].dereference().type.sizeof
                # guess! Assume square
                nx = int(math.sqrt(size/size0))
                ny = size/(nx*size0)

        return nx, ny

    def getEigenValue(var, x, y=0):
        if re.search(r"Matrix", str(var.type)):
            if False:
                return var["operator()(int, int)"](x, y)

            NX, NY = getEigenMatrixDimensions(var)

            if x < 0 or x >= NX or y < 0 or y >= NY:
                raise gdb.GdbError("Element (%d, %d) is out of range 0:%d, 0:%d" %
                                   (x, y, NX - 1, NY - 1))

            m_data = var["m_storage"]["m_data"]
            if False:
                # convert to a pointer to the start of the array
                import pdb
                pdb.set_trace()
                m_data = m_data.address.cast(m_data.type)

            try:
                val = m_data[x + y*NX]
            except Exception:
                val = m_data["array"][x + y*NX]
        else:                       # Vector
            if False:
                return var["operator()(int)"](x)

            NX = getEigenMatrixDimensions(var)[0]

            if x < 0 or x >= NX:
                raise gdb.GdbError(
                    "Element (%d) is out of range 0:%d" % (x, NX - 1))

            m_data = var["m_storage"]["m_data"]

            if False:
                # convert to a pointer to the start of the array
                m_data = m_data.address.cast(m_data.type)

            try:
                val = m_data[x]
            except Exception:
                val = m_data["array"][x]

        if val.type.code == gdb.TYPE_CODE_INT:
            val = int(val)
        elif val.type.code == gdb.TYPE_CODE_FLT:
            val = float(val)

        return val

    class EigenMatrixPrinter:
        "Print an Eigen Matrix"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            nx, ny = getEigenMatrixDimensions(self.val)

            return "%s{%dx%d}" % (self.val.type, nx, ny)

    class EigenVectorPrinter:
        "Print an Eigen Vector"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            m_storage = self.val["m_storage"]

            try:
                n = m_storage["n"]
            except gdb.error:           # only available for dynamic Matrices
                try:
                    n = m_storage.type.template_argument(1)
                except RuntimeError:
                    # should get dimens from template, but that's gdb bug
                    # #11060
                    size = m_storage["m_data"]["array"].type.sizeof
                    size0 = m_storage["m_data"]["array"].dereference(
                    ).type.sizeof
                    n = math.sqrt(size/size0)

            return "{%d}" % (n)

    class PrintEigenCommand(gdb.Command):
        """Print an eigen Matrix or Vector
    Usage: show eigen <matrix> [x0 y0 [nx ny]]
           show eigen <vector> [x0 [nx]]
    """

        def __init__(self):
            super(PrintEigenCommand, self).__init__("show eigen",
                                                    gdb.COMMAND_DATA,
                                                    gdb.COMPLETE_SYMBOL)

        def _mget(self, var, x, y=0):
            return getEigenValue(var, x, y)

        def _vget(self, var, x):
            return getEigenValue(var, x)

        def invoke(self, args, fromTty):
            self.dont_repeat()

            parser = GdbOptionParser("show eigen")
            parser.add_option("-d", "--dataFmt", default="%.2f",
                              help="Format for values")
            parser.add_option("-f", "--formatWidth", type="int",
                              default=8, help="Field width for values")
            parser.add_option("-o", "--origin", type="str", nargs="+",
                              help="Origin of the part of the object to print")
            if False:
                parser.add_option(
                    "eigenObject", help="Expression giving Eigen::Matrix/Vector to show")
                parser.add_option(
                    "nx", help="Width of patch to print", type="int", default=0, nargs="?")
                parser.add_option(
                    "ny", help="Height of patch to print", type="int", default=0, nargs="?")

                opts = parser.parse_args(args)
                if opts.help:
                    return
            else:
                (opts, args) = parser.parse_args(args)
                if opts.help:
                    return

                if not args:
                    raise gdb.GdbError("Please specify an object")
                opts.eigenObject = args.pop(0)

                opts.nx, opts.ny = 0, 0
                if args:
                    opts.nx = int(args.pop(0))
                if args:
                    opts.ny = int(args.pop(0))

                if args:
                    raise gdb.GdbError(
                        "Unrecognised trailing arguments: %s" % " ".join(args))

            var = gdb.parse_and_eval(opts.eigenObject)

            if not re.search(r"(Eigen|LinearTransform)::(Matrix|Vector)", str(var.type)):
                raise gdb.GdbError(
                    "Please specify an eigen matrix or vector, not %s" % var.type)

            if re.search(r"shared_ptr<", str(var.type)):
                var = var["px"].dereference()

            if var.type.code == gdb.TYPE_CODE_PTR:
                var = var.dereference()     # be nice

            isMatrix = re.search(r"Matrix", str(var.type))
            if isMatrix:
                NX, NY = getEigenMatrixDimensions(var)

                if opts.origin:
                    if len(opts.origin) != 2:
                        raise gdb.GdbError("Please specify both x0 and y0")

                    x0 = gdb.parse_and_eval(opts.origin[0])
                    y0 = gdb.parse_and_eval(opts.origin[1])
                else:
                    x0, y0 = 0, 0

                nx = opts.nx
                ny = opts.ny
                if nx == 0:
                    nx = NX
                if ny == 0:
                    ny = NY

                if nx == 1 and ny == 1:
                    print("%g" % self._vget(var, x0))
                    return
            else:
                NX = 0, var["m_storage"]["n"]

                if opts.origin:
                    if len(opts.origin) != 1:
                        raise gdb.GdbError("Please only specify x0")

                    x0 = gdb.parse_and_eval(opts.origin[0])
                else:
                    x0 = 0

                nx = opts.nx
                if nx == 0:
                    nx = NX

                if nx == 1:
                    print("%g" % self._vget(var, x0))
                    return
            #
            # OK, finally time to print
            #
            if isMatrix:
                print("%-4s" % "", end=' ')
                for x in range(x0, min(NX, x0 + nx)):
                    print("%*d" % (opts.formatWidth, x), end=' ')
                print("")

                for y in range(y0, min(NY, y0 + ny)):
                    print("%-4d" % y, end=' ')
                    for x in range(x0, min(NX, x0 + nx)):
                        print("%*s" % (opts.formatWidth, (opts.dataFmt %
                                                          self._mget(var, x, y))), end=' ')
                    print("")
            else:
                for x in range(x0, min(NX, x0 + nx)):
                    print("%*s" % (opts.formatWidth, (opts.dataFmt %
                                                      self._vget(var, x))), end=' ')
                print("")

    PrintEigenCommand()

    # afw

    class BaseSourceAttributesPrinter:
        "Print a BaseSourceAttributes"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            return "Base: {id=%d astrom=(%.3f, %.3f)}" % (self.val["_id"],
                                                          self.val["_xAstrom"],
                                                          self.val["_yAstrom"])

    class SourcePrinter:
        "Print a Source"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            return "Source{id=%d astrom=(%.3f, %.3f)}" % (self.val["_id"],
                                                          self.val["_xAstrom"],
                                                          self.val["_yAstrom"])

    class DetectorPrinter:
        "Print a cameraGeom::Detector"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            return "Detector{name: %s id: %s type: %s bbox: %s}" % (self.val["_name"], self.val["_id"],
                                                                    self.val["_type"], self.val["_bbox"])

    class FootprintPrinter:
        "Print a Footprint"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            if False:
                # Fails (as its type is METHOD, not CODE)
                nspan = self.val["_spans"]["size"]()
            else:
                vec_impl = self.val["_spans"]["_M_impl"]
                nspan = vec_impl["_M_finish"] - vec_impl["_M_start"]

            return "Footprint{id=%d, nspan=%d, area=%d; BBox %s}" % (self.val["_fid"], nspan,
                                                                     self.val["_area"], self.val["_bbox"])

    class FootprintSetPrinter:
        "Print a FootprintSet"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            return "FootprintSet{%s; %s}" % (self.val["_region"], self.val["_footprints"])

    class PeakPrinter:
        "Print a Peak"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            return "Peak{%d, (%.2f, %.2f)}" % (self.val["_id"], self.val["_fx"], self.val["_fy"])

    class PsfPrinter:
        "Print a Psf"

        def to_string(self):
            return "%s" % (self.typeName())

    class Box2Printer:
        "Print a Box2"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            # Make sure &foo works, too.
            type = self.val.type
            if type.code == gdb.TYPE_CODE_REF:
                type = type.target()

            llc = [getEigenValue(self.val["_minimum"]["_vector"], i)
                   for i in range(2)]
            dims = [getEigenValue(self.val["_dimensions"]["_vector"], i)
                    for i in range(2)]

            return "Box2{(%s,%s)--(%s,%s)}" % (llc[0], llc[1],
                                               llc[0] + dims[0] - 1, llc[1] + dims[1] - 1)

        def display_hint(self):
            return "array"

    class CoordinateBasePrinter:
        "Print a CoordinateBase"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            return self.val["_vector"]["m_storage"]["m_data"]["array"]

        def display_hint(self):
            return "array"

    class AxesPrinter:
        "Print an ellipse::Axes"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            vec = self.val["_vector"]
            return "[%g, %g, %g]" % (getEigenValue(vec, 0), getEigenValue(vec, 1), getEigenValue(vec, 2))

    class QuadrupolePrinter:
        "Print an ellipse::Quadrupole"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            mat = self.val["_matrix"]

            if False:
                return mat
            else:
                return "[[%g, %g], [%g, %g]]" % (getEigenValue(mat, 0, 0), getEigenValue(mat, 0, 1),
                                                 getEigenValue(mat, 1, 0), getEigenValue(mat, 1, 1))

    class ImagePrinter:
        "Print an ImageBase or derived class"

        def dimenStr(self, val=None):
            if val is None:
                val = self.val

            # Make sure &foo works, too.
            type = val.type
            if type.code == gdb.TYPE_CODE_REF:
                type = type.target()

            gilView = val["_gilView"]
            arr = val["_origin"]["_vector"]["m_storage"]["m_data"]["array"]

            x0, y0 = arr[0], arr[1]
            return "%dx%d%s%d%s%d" % (
                # val["getWidth"](), val["getHeight"](),
                gilView["_dimensions"]["x"], gilView["_dimensions"]["y"],
                # i.e. "+" if x0 >= 0 else "" in python >= 2.5
                ["", "+"][x0 >= 0], x0,
                ["", "+"][y0 >= 0], y0)

        def typeName(self):
            return self.typename.split(":")[-1]

        def __init__(self, val):
            self.typename = str(val.type)
            self.val = val

        def to_string(self):
            return "%s(%s)" % (self.typeName(), self.dimenStr())

    class MaskedImagePrinter(ImagePrinter):
        "Print a MaskedImage"

        def to_string(self):
            return "%s(%s)" % (self.typeName(), self.dimenStr(self.val["_image"]["px"].dereference()))

    class ExposurePrinter(ImagePrinter):
        "Print an Exposure"

        def to_string(self):
            return "%s(%s)" % (self.typeName(),
                               self.dimenStr(self.val["_maskedImage"]["_image"]["px"].dereference()))

    class PrintImageCommand(gdb.Command):
        """Print an Image"""

        def __init__(self):
            super(PrintImageCommand, self).__init__("show image",
                                                    gdb.COMMAND_DATA,
                                                    gdb.COMPLETE_SYMBOL)

        def get(self, var, x, y):
            if False:
                return var["operator()(int, int, bool)"](x, y, True)
            else:
                dimensions = var["_gilView"]["_dimensions"]
                if x < 0 or x >= dimensions["x"] or y < 0 or y >= dimensions["y"]:
                    raise gdb.GdbError("Pixel (%d, %d) is out of range 0:%d, 0:%d" %
                                       (x, y, dimensions["x"] - 1, dimensions["y"] - 1))

                pixels = var["_gilView"]["_pixels"]["_p"]
                step = pixels["_step_fn"]["_step"] / \
                    var.type.template_argument(0).sizeof

                return pixels["m_iterator"][x + y*step]["_v0"]

        def invoke(self, args, fromTty):
            self.dont_repeat()

            parser = GdbOptionParser(
                "show image" + ("" if argparse else " <image> [<nx> [<ny>]]"))
            parser.add_option("-a", "--all", action="store_true",
                              help="Display the whole image/mask")
            parser.add_option("-c", "--center", type="str", nargs=2, default=(None, None,),
                              help="Center the output at (x, y)")
            parser.add_option("-o", "--origin", type="str", nargs=2, default=(None, None,),
                              help="Print the region starting at (x, y)")
            parser.add_option("-x", "--xy0", action="store_true",
                              help="Obey the image's (x0, y0)")
            parser.add_option("-f", "--formatWidth", type="int",
                              default=8, help="Field width for values")
            parser.add_option("-d", "--dataFmt", default="%.2f",
                              help="Format for values")

            if argparse:
                parser.add_option(
                    "image", help="Expression giving image to show")
                parser.add_option(
                    "width", help="Width of patch to print", default=1, nargs="?")
                parser.add_option(
                    "height", help="Height of patch to print", default=1, nargs="?")

                opts = parser.parse_args(args)
                if opts.help:
                    return
            else:
                opts, args = parser.parse_args(args)
                if opts.help:
                    return

                if not args:
                    raise gdb.GdbError("Please specify an image")

                opts.image = args.pop(0)

                opts.width, opts.height = 1, 1
                if args:
                    opts.width = int(args.pop(0))
                if args:
                    opts.height = int(args.pop(0))

                if args:
                    raise gdb.GdbError(
                        "Unrecognised trailing arguments: %s" % " ".join(args))

            for i in range(2):
                val = "0"
                if opts.origin[i] is None:
                    if opts.center[i] is not None:
                        val = opts.center[i]
                else:
                    val = opts.origin[i]
                    if opts.center[i] is not None:
                        raise gdb.GdbError(
                            "You may not specify both --center and --origin")

                val = gdb.parse_and_eval(val)
                if i == 0:
                    x0 = val
                else:
                    y0 = val

            if opts.all:
                nx, ny = 0, 0
            else:
                nx, ny = opts.width, opts.height

            var = gdb.parse_and_eval(opts.image)

            if re.search(r"shared_ptr<", str(var.type)):
                var = var["px"].dereference()

            if not re.search(r"(lsst::afw::image::)?(Image|Mask|MaskedImage)", str(var.type.unqualified())):
                raise gdb.GdbError(
                    "Please specify an image, not %s" % var.type)

            if re.search(r"MaskedImage", str(var.type)) and \
                    not re.search(r"::Image(\s*&)?$", str(var.type)):
                print("N.b. %s is a MaskedImage; showing image" % (opts.image))
                var = var["_image"]

            if re.search(r"shared_ptr<", str(var.type)):
                var = var["px"].dereference()

            if var.type.code == gdb.TYPE_CODE_PTR:
                var = var.dereference()     # be nice

            pixelTypeName = str(var.type.template_argument(0))
            if opts.dataFmt:
                dataFmt = opts.dataFmt
            elif pixelTypeName in ["short", "unsigned short"]:
                dataFmt = "0x%x"
            elif pixelTypeName in ["int", "unsigned int"]:
                dataFmt = "%d"
            else:
                dataFmt = "%.2f"

            if nx == 0:
                nx = var["_gilView"]["_dimensions"]["x"]
            if ny == 0:
                ny = var["_gilView"]["_dimensions"]["y"]

            if opts.center[0]:
                x0 -= nx//2
                y0 -= ny//2

            if opts.xy0 and not opts.all:
                arr = var["_origin"]["_vector"]["m_storage"]["m_data"]["array"]

                x0 -= arr[0]
                y0 -= arr[1]
            #
            # OK, finally time to print
            #
            print("%-4s" % "", end=' ')
            for x in range(x0, x0 + nx):
                print("%*d" % (opts.formatWidth, x), end=' ')
            print("")

            for y in reversed(list(range(y0, y0 + ny))):
                print("%-4d" % y, end=' ')
                for x in range(x0, x0 + nx):
                    print("%*s" % (opts.formatWidth, dataFmt %
                                   self.get(var, x, y)), end=' ')
                print("")

    PrintImageCommand()

    class BackgroundPrinter:
        "Print a Background"

        def __init__(self, val):
            self.typename = str(val.type)
            self.val = val

        def to_string(self):
            return "Background(%dx%d) %s" % (
                self.val["_imgWidth"], self.val["_imgHeight"],
                self.val["_bctrl"])

    class BackgroundControlPrinter:
        "Print a BackgroundControl"

        def __init__(self, val):
            self.typename = str(val.type)
            self.val = val

        def to_string(self):
            return "{%s %s %s %s}" % (re.sub(r"lsst::afw::math::Interpolate::", "", str(self.val["_style"])),
                                      re.sub(r"lsst::afw::math::", "",
                                             str(self.val["_prop"])),
                                      re.sub(r"lsst::afw::math::", "", str(
                                          self.val["_undersampleStyle"])),
                                      self.val["_sctrl"]["px"].dereference())

    class KernelPrinter:
        "Print a Kernel"

        def __init__(self, val):
            self.typename = str(val.type)
            self.val = val

        def to_string(self):
            return "%s(%dx%d)" % (self.typename,
                                  self.val["_width"], self.val["_height"])

    class StatisticsControlPrinter:
        "Print a StatisticsControl"

        def __init__(self, val):
            self.typename = str(val.type)
            self.val = val

        def to_string(self):
            return "{nSigma=%g nIter=%d ignore=0x%x}" % (self.val["_numSigmaClip"],
                                                         self.val["_numIter"],
                                                         self.val["_andMask"])

    class TablePrinter:
        "Print a table::Table"

        def __init__(self, val):
            self.typename = str(val.type)
            self.val = val

        def to_string(self):
            return "{schema = %s, md=%s}" % (self.val["_schema"], self.val["_metadata"])

    class TableSchemaPrinter:
        "Print a table::Schema"

        def __init__(self, val):
            self.typename = str(val.type)
            self.val = val

        def to_string(self):
            names = str(self.val["_impl"]["px"]["_names"])
            names = re.sub(r"^[^{]*{|}|[\[\]\"\"]|\s*=\s*[^,]*", "", names)

            return "%s" % (names)

    printers = []

    def register(obj=None):
        "Register my pretty-printers with objfile Obj."

        if obj is None:
            obj = gdb

        for p in printers:
            gdb.printing.register_pretty_printer(obj, p, replace=True)

    def build_boost_dictionary():
        """Surely this must be somewhere standard?"""

        printer = gdb.printing.RegexpCollectionPrettyPrinter("rhl-boost")

        printer.add_printer('boost::shared_ptr',
                            '^(boost|tr1|std)::shared_ptr', SharedPtrPrinter)
        printer.add_printer('boost::gil::pixel',
                            'boost::gil::.*pixel_t', GilPixelPrinter)

        return printer

    printers.append(build_boost_dictionary())

    def build_eigen_dictionary():
        """Surely this must be somewhere standard?"""

        printer = gdb.printing.RegexpCollectionPrettyPrinter("rhl-eigen")

        printer.add_printer('eigen::Matrix',
                            '^Eigen::Matrix', EigenMatrixPrinter)
        printer.add_printer('eigen::Vector',
                            '^Eigen::Vector', EigenVectorPrinter)

        return printer

    printers.append(build_eigen_dictionary())

    def build_afw_dictionary():
        printer = gdb.printing.RegexpCollectionPrettyPrinter("afw")

        printer.add_printer('lsst::afw::cameraGeom::Detector',
                            '^lsst::afw::cameraGeom::(Amp|Ccd|Detector|DetectorMosaic)$', DetectorPrinter)

        printer.add_printer('lsst::afw::detection::Footprint',
                            '^lsst::afw::detection::Footprint$', FootprintPrinter)
        printer.add_printer('lsst::afw::detection::FootprintSet',
                            '^lsst::afw::detection::FootprintSet', FootprintSetPrinter)
        printer.add_printer('lsst::afw::detection::Peak',
                            '^lsst::afw::detection::Peak$', PeakPrinter)
        printer.add_printer('lsst::afw::detection::Psf',
                            '^lsst::afw::detection::Psf$', PsfPrinter)
        printer.add_printer('lsst::afw::detection::Source',
                            '^lsst::afw::detection::Source$', SourcePrinter)
        printer.add_printer('lsst::afw::detection::BaseSourceAttributes',
                            '^lsst::afw::detection::BaseSourceAttributes$', BaseSourceAttributesPrinter)

        printer.add_printer('lsst::afw::geom::Box',
                            '^lsst::afw::geom::Box', Box2Printer)
        printer.add_printer('lsst::afw::geom::Extent',
                            '^lsst::afw::geom::Extent', CoordinateBasePrinter)
        printer.add_printer('lsst::afw::geom::Point',
                            '^lsst::afw::geom::Point', CoordinateBasePrinter)

        printer.add_printer('lsst::afw::geom::ellipses::Axes',
                            '^lsst::afw::geom::ellipses::Axes', AxesPrinter)
        printer.add_printer('lsst::afw::geom::ellipses::Quadrupole',
                            '^lsst::afw::geom::ellipses::Quadrupole', QuadrupolePrinter)

        printer.add_printer('lsst::afw::image::ImageBase',
                            'lsst::afw::image::ImageBase<[^>]+>$', ImagePrinter)
        printer.add_printer('lsst::afw::image::Image',
                            'lsst::afw::image::Image<[^>]+>$', ImagePrinter)
        printer.add_printer('lsst::afw::image::Mask',
                            '^lsst::afw::image::Mask<[^>]+>$', ImagePrinter)
        printer.add_printer('lsst::afw::image::MaskedImage',
                            '^lsst::afw::image::MaskedImage<[^>]+>$', MaskedImagePrinter)
        printer.add_printer('lsst::afw::image::Exposure',
                            '^lsst::afw::image::Exposure', ExposurePrinter)

        printer.add_printer('lsst::afw::math::Background',
                            '^lsst::afw::math::Background$', BackgroundPrinter)
        printer.add_printer('lsst::afw::math::BackgroundControl',
                            '^lsst::afw::math::BackgroundControl$', BackgroundControlPrinter)
        printer.add_printer('lsst::afw::math::Kernel',
                            '^lsst::afw::math::.*Kernel', KernelPrinter)
        printer.add_printer('lsst::afw::math::StatisticsControl',
                            '^lsst::afw::math::StatisticsControl', StatisticsControlPrinter)

        printer.add_printer('lsst::afw::table::Table',
                            '^lsst::afw::table::.*Table$', TablePrinter)
        printer.add_printer('lsst::afw::table::Schema',
                            '^lsst::afw::table::Schema$', TableSchemaPrinter)

        return printer

    printers.append(build_afw_dictionary())

    def build_daf_base_dictionary():
        printer = gdb.printing.RegexpCollectionPrettyPrinter("daf::base")

        return printer

    printers.append(build_daf_base_dictionary())
except ImportError as e:
    print("RHL", e)
    from .printers_oldgdb import *  # noqa: F401, F403
