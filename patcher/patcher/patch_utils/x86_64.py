import struct


def get_reljmp_patch(from_addr, to_addr):
    """
    lea rax, [rip + offset]
    jmp rax
    """
    patch = b"\x48\x8D\x05" + struct.pack("<i", to_addr - from_addr - 7)
    patch += b"\xFF\xE0"
    return patch
