# !/usr/bin/env python3

import os
import subprocess
import tempfile

import lief

from .merge_dynamic import merge_dynamic
from .patch_utils import ensure_patch_fits
from .patch_utils.x86_64 import get_reljmp_patch
from .symbols_utils import get_symbols, exported_functions_only, exported_functions_and_all_variables, \
    get_symbols_from_csv, find_symbol


def main(module_path,
         input_binary_path,
         output_binary_path,
         map_new_code_to,
         additional_dylib_paths=[],
         inplace=False,
         fill_with_nops=True,
         additional_symbols_path=None):
    # WARNING: DO NOT PARSE BINARIES WITH LIEF IN A FUNCTION AND
    # RETURN OBJECTS TAKEN FROM PROPERTIES OF THE PARSED FILE.
    # LIEF Python API does not use reference counting,
    # when the lief.ELF object goes out of scope
    # it is freed with all its members

    parsed_input_binary = lief.parse(input_binary_path)

    symbols = get_symbols(parsed_input_binary, filter=exported_functions_and_all_variables, include_pltsyms=inplace)
    if additional_symbols_path:
        additional_symbols = get_symbols_from_csv(additional_symbols_path)
        symbols.append(additional_symbols)

    with tempfile.NamedTemporaryFile(mode="w") as symbols_tmpfile:
        for s in symbols:
            symbols_tmpfile.write(f"{s.name},{hex(s.value)[2:]},{hex(s.size)[2:]}\n")
        symbols_tmpfile.flush()

        print("[+] JITting new code")
        run_compiler(module_path, symbols_tmpfile.name)

    # TODO: don't hardcode objfile name
    parsed_objfile = lief.parse("tmp/obj_0")

    # Read symbols from the newly generated object files
    objfile_symbols = get_symbols(parsed_objfile, filter=exported_functions_only)
    objfile_symbol_names = {s.name for s in objfile_symbols}

    if inplace:
        ensure_patch_fits(symbols, objfile_symbols)

    print("[+] Linking")
    # TODO: do not hardcode this path
    object_file_paths = ["tmp/obj_0"]
    object_file_paths += additional_dylib_paths
    linked_binary_path = "tmp/linked.so"

    section_mappings = {}
    if inplace:
        for new_symbol in objfile_symbols:
            old_symbol = find_symbol(new_symbol.name, symbols)
            section_mappings[f".funcs.{new_symbol.name}"] = old_symbol.value

    symbols_for_linker = {}
    for symbol in symbols:
        if symbol.name in objfile_symbol_names or "@" in symbol.name:
            continue
        symbols_for_linker[symbol.name] = symbol.value

    res = run_linker(object_file_paths,
                     linked_binary_path,
                     code_segment_start=map_new_code_to,
                     section_start=section_mappings,
                     defsym=symbols_for_linker
                     )

    if res.returncode:
        print("Linker returned nonzero exit code, exiting")
        exit(res.returncode)

    # Read addresses of new symbols
    parsed_linked_binary = lief.parse(linked_binary_path)
    linkedfile_symbols = get_symbols(parsed_linked_binary)
    linkedfile_symbols_dict = {s.name: s.value for s in linkedfile_symbols}
    new_symbols = {name: linkedfile_symbols_dict[name] for name in objfile_symbol_names}

    if not inplace:
        print("[+] Merging new segments and dynamic libraries")
        with open(input_binary_path, "rb") as input_binary, \
                open(linked_binary_path, "rb") as linked_binary, \
                open(output_binary_path, "wb") as output_binary:
            merge_dynamic(input_binary, linked_binary, output_binary, base=0, merge_load_segments=True)

        print("[+] Applying detours")
        with open(output_binary_path, "rb") as f:
            binary = bytearray(f.read())

        for symbol_name, new_symbol_addr in new_symbols.items():
            old_symbol = find_symbol(symbol_name, symbols)
            if old_symbol is None:
                raise Exception(f"Symbol {symbol_name} was not found in the old binary and was not manually provided")
            old_symbol_addr = old_symbol.value
            old_symbol_offset = parsed_input_binary.virtual_address_to_offset(old_symbol.value)
            print(f"Patching {symbol_name} at offset {hex(old_symbol_offset)}")
            patch = get_reljmp_patch(old_symbol_addr, new_symbol_addr)
            for i, b in enumerate(patch):
                binary[old_symbol_offset + i] = b

        with open(output_binary_path, "wb") as f:
            f.write(binary)

    else:
        print("[+] Applying in place patches")
        parsed_input_binary_2 = lief.parse(input_binary_path)
        for symbol_name, new_symbol_addr in new_symbols.items():
            new_symbol = find_symbol(symbol_name, linkedfile_symbols)
            old_symbol = find_symbol(symbol_name, symbols)
            patch = parsed_linked_binary.get_content_from_virtual_address(new_symbol.value, new_symbol.size)
            if fill_with_nops:
                patch += b"\x90" * (old_symbol.size - new_symbol.size)
            parsed_input_binary_2.patch_address(old_symbol.value, patch)

        parsed_input_binary_2.write(output_binary_path)

    os.chmod(output_binary_path, 0o755)


def run_compiler(module_path, symbols_path):
    jit_cmd = ["crispr", "-m", module_path, "--symbols", symbols_path]
    print(f"[i] JIT cmd: {' '.join(jit_cmd)}")
    return subprocess.run(jit_cmd)


def run_linker(input_paths, output_path,
               code_segment_start=None,
               section_start=None,
               defsym=None):

    if section_start is None:
        section_start = {}

    if defsym is None:
        defsym = {}

    linker_cmd = ["ld", "-shared", "-pie", "--relax", "--as-needed", "-o", output_path]
    if code_segment_start is not None:
        linker_cmd.append("-Ttext-segment")
        linker_cmd.append(hex(code_segment_start))
    for sym_name, sym_addr in defsym.items():
        linker_cmd.append("--defsym")
        linker_cmd.append(f"{sym_name}={hex(sym_addr)}")
    for section_name, section_addr in section_start.items():
        linker_cmd.append("--section-start")
        linker_cmd.append(f"{section_name}={hex(section_addr)}")
    for input_path in input_paths:
        linker_cmd.append(input_path)

    print(f"[i] Linker cmd: {' '.join(linker_cmd)}")
    return subprocess.run(linker_cmd)
