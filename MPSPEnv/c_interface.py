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
        ("right_most_added_column", POINTER(c_int)),
        ("left_most_updated_column", POINTER(c_int)),
        ("added_since_sailing", POINTER(c_int)),
        ("matrix", Array),
        ("min_container_per_column", Array),
        ("column_counts", Array),
    ]


class Transportation_Info(Structure):
    _fields_ = [
        ("matrix", Array),
        ("containers_per_port", Array),
        ("N", c_int),
        ("seed", c_int),
        ("last_non_zero_column", c_int),
        ("current_port", c_int),
    ]


class Env(Structure):
    _fields_ = [
        ("T", POINTER(Transportation_Info)),
        ("bay", Bay),
        ("mask", Array),
        ("auto_move", c_int),
        ("total_reward", POINTER(c_int)),
        ("containers_left", POINTER(c_int)),
        ("containers_placed", POINTER(c_int)),
        ("terminated", POINTER(c_int)),
    ]


class StepInfo(Structure):
    _fields_ = [
        ("terminated", c_int),
        ("reward", c_int),
    ]


directory = os.path.dirname(os.path.abspath(__file__))
c_lib_files = glob.glob(os.path.join(directory, "c_lib*.so"))

if len(c_lib_files) == 0:
    raise FileNotFoundError("Can't find C library")

c_lib_path = c_lib_files[0]
c_lib = ctypes.CDLL(c_lib_path)

c_lib.env_step.argtypes = [Env, c_int]
c_lib.env_step.restype = StepInfo

c_lib.get_random_env.argtypes = [c_int, c_int, c_int, c_int]
c_lib.get_random_env.restype = Env

c_lib.get_specific_env.argtypes = [c_int, c_int, c_int, POINTER(c_int), c_int]
c_lib.get_specific_env.restype = Env

c_lib.free_env.argtypes = [Env]
c_lib.free_env.restype = None

c_lib.copy_env.argtypes = [Env]
c_lib.copy_env.restype = Env

c_lib.set_seed.argtypes = [c_int]
c_lib.set_seed.restype = None
