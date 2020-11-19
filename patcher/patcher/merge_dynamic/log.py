import sys


verbose = True


def set_verbose(verb: bool):
    global verbose
    verbose = bool


def log(message):
    # TODO: implement properly using python logging and setting the loglevel
    if verbose:
        sys.stderr.write(message + "\n")
