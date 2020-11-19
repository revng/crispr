import argparse

from .patcher import main

argparser = argparse.ArgumentParser()
argparser.add_argument("input_binary",
                       help="Path to the binary to patch")
argparser.add_argument("module",
                       help="Path to the LLVM module with patches")
argparser.add_argument("--symbols",
                       help="Path to a CSV containing additional symbols for the input binary")
argparser.add_argument("--map-new-code-to", default="0x700000",
                       help="Address where new code (and data, etc) will be mapped. Defaults to 0x700000")
argparser.add_argument("--dylib", action="append", default=[],
                       help="Path to an additional dynamic library to be linked together with the output.\n" +
                            "By default, all symbols are imported. To import only some symbols, "
                            "use mylib.so:symbol_name[,symbol_name,...].\n"
                            "Can be specified multiple times")
argparser.add_argument("--output-binary", "-o",
                       help="Path to the output binary (default: <input-binary>.patched")
argparser.add_argument("--inplace", action="store_true",
                       help="Patch the binary in place, if possible, else exit with an error")
argparser.add_argument("--nofill", dest="fill", action="store_false",
                       help="Don't fill with NOPs until function end when patching in place")


def cmdline_main():
    args = argparser.parse_args()

    if not args.output_binary:
        args.output_binary = args.input_binary + ".patched"

    args.map_new_code_to = int(args.map_new_code_to, base=0)

    main(args.module,
         args.input_binary,
         args.output_binary,
         args.map_new_code_to,
         additional_dylib_paths=args.dylib,
         inplace=args.inplace,
         fill_with_nops=args.fill,
         additional_symbols_path=args.symbols
         )
