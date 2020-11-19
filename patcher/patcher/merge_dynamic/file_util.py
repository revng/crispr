import os
import stat


def file_size(file):
    file.seek(0, 2)
    return file.tell()


def read_at_offset(file, start, size):
    file.seek(start)
    return file.read(size)


def set_executable(path):
    st = os.stat(path)
    os.chmod(path, st.st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
