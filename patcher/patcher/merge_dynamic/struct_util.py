from .iter_util import chunks


def parse(buffer, struct):
    struct_size = struct.sizeof()
    assert len(buffer) % struct_size == 0
    return list(map(struct.parse, chunks(buffer, struct_size)))


def serialize(list, struct):
    return b"".join(map(struct.build, list))
