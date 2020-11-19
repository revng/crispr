from ..symbols_utils import find_symbol


def ensure_patch_fits(original_symbols, new_symbols):
    for new_symbol in new_symbols:
        old_symbol = find_symbol(new_symbol.name, original_symbols)
        if old_symbol is None:
            print(f"Symbol {new_symbol.name} not found in original binary")
            continue

        assert new_symbol.size != 0, f"Size for new symbol {new_symbol.name} is zero"

        if old_symbol.size < new_symbol.size:
            raise ValueError(f"New code for symbol {new_symbol.name} is bigger than old code, "
                             f"can't patch in place (new: {new_symbol.size} bytes, old: {old_symbol.size} bytes)")
