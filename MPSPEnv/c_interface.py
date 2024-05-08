import ctypes
from ctypes import POINTER, c_int, Structure, c_char
import os
import glob


class Array(Structure):
    _fields_ = [
        ("values", POINTER(c_int)),
        ("n", c_int),
    ]


class Bay(Structure):
    _fields_ = [
        ("R", c_int),
        ("C", c_int),
        ("N", c_int),
        ("matrix", Array),
        ("min_container_per_column", Array),
        ("column_counts", Array),
        ("mask", Array),
    ]


class Transportation_Info(Structure):
    _fields_ = [
        ("matrix", Array),
        ("containers_per_port", Array),
        ("N", c_int),
        ("seed", c_int),
        ("last_non_zero_column", c_int),
        ("current_port", c_int),
        ("containers_left", c_int),
        ("containers_placed", c_int),
    ]


class Env(Structure):
    _fields_ = [
        ("T", POINTER(Transportation_Info)),
        ("bay", Bay),
        ("one_hot_bay", Array),
        ("flat_T_matrix", Array),
        ("skip_last_port", c_int),
        ("history_index", POINTER(c_int)),
        ("history", POINTER(c_char)),
    ]


class StepInfo(Structure):
    _fields_ = [
        ("is_terminal", c_int),
        ("reward", c_int),
    ]


directory = os.path.dirname(os.path.abspath(__file__))
c_lib_files = glob.glob(os.path.join(directory, "c_lib*.so"))

if len(c_lib_files) == 0:
    raise FileNotFoundError("Can't find C library")

c_lib_path = c_lib_files[0]
c_lib = ctypes.CDLL(c_lib_path)

c_lib.step.argtypes = [Env, c_int]
c_lib.step.restype = StepInfo

c_lib.get_random_env.argtypes = [c_int, c_int, c_int, c_int, c_int]
c_lib.get_random_env.restype = Env

c_lib.get_specific_env.argtypes = [c_int, c_int, c_int, POINTER(c_int), c_int, c_int]
c_lib.get_specific_env.restype = Env

c_lib.free_env.argtypes = [Env]

c_lib.set_random_seed.argtypes = []
c_lib.set_seed.argtypes = [c_int]

c_lib.copy_env.argtypes = [Env, c_int]
c_lib.copy_env.restype = Env

c_lib.get_moves_upper_bound.argtypes = [Env]
c_lib.get_moves_upper_bound.restype = c_int
