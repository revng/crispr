from typing import Iterable, List


def only(i: Iterable):
    i = list(i)
    assert len(i) == 1
    return i[0]


def first_or_none(i: Iterable):
    i = list(i)
    return i[0] if len(i) != 0 else None


def chunks(list: List, size):
    for i in range(0, len(list), size):
        yield list[i:i + size]

