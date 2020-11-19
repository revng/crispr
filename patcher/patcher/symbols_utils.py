import lief


def exported_functions_only(symbol):
    # LIEF symbol.exported is buggy for our use case
    # -- symbols with value 0 are not considered exported
    return symbol.type == lief.ELF.SYMBOL_TYPES.FUNC \
           and symbol.imported is False \
           and symbol.visibility in [
               lief.ELF.SYMBOL_VISIBILITY.PROTECTED,
               lief.ELF.SYMBOL_VISIBILITY.DEFAULT,
           ]


def exported_functions_and_all_variables(symbol):
    return exported_functions_only(symbol) or symbol.is_variable


def get_symbols(parsed_elf, filter=lambda sym: True, include_pltsyms=False):
    syms = [s for s in parsed_elf.symbols if filter(s)]
    if not include_pltsyms:
        return syms

    plt_addr = parsed_elf.get_section(".plt").virtual_address
    plt_entry_size = parsed_elf.get_section(".plt").entry_size
    for reloc in parsed_elf.pltgot_relocations:
        if reloc.purpose != lief.ELF.RELOCATION_PURPOSES.PLTGOT:
            continue

        symbol = reloc.symbol
        plt_index = reloc.info
        new_sym = new_symbol(symbol.name, plt_addr + plt_entry_size * plt_index)
        syms.append(new_sym)

    return syms


def find_symbol(name, symbols_list):
    for s in symbols_list:
        if s.name == name:
            return s


def new_symbol(name, value, size=0x0,
               exported=True, imported=False,
               binding=lief.ELF.SYMBOL_BINDINGS.GLOBAL,
               type=lief.ELF.SYMBOL_TYPES.FUNC,
               visibility=lief.ELF.SYMBOL_VISIBILITY.DEFAULT,
               shndx=lief.ELF.SYMBOL_SECTION_INDEX.UNDEF):
    newsym = lief.ELF.Symbol()
    newsym.name = name
    newsym.value = value
    newsym.size = size
    newsym.exported = exported
    newsym.imported = imported
    newsym.binding = binding
    newsym.type = type
    newsym.visibility = visibility
    newsym.shndx = shndx
    return newsym


def get_symbols_from_csv(csv_path):
    symbols = []
    with open(csv_path) as f:
        for line in f:
            name, addr_str, size_str = line.split(",")
            addr = int(addr_str, base=0)
            size = int(size_str, base=0)
            new_sym = new_symbol(name, addr, size=size)
            symbols.append(new_sym)
    return symbols
