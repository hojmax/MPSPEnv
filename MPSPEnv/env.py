from MPSPEnv.c_interface import c_lib
import numpy as np
from gymnasium import spaces
import gymnasium as gym
from MPSPEnv.visualizer import Visualizer
import warnings
from ctypes import POINTER, c_int


class Env(gym.Env):
    """
    Gym environment for the Multi-Port Stowage Planning Problem (MPSP).
    The environment is defined by the following parameters:
    - R: number of rows in the bay
    - C: number of columns in the bay
    - N: number of ports
    - skip_last_port: whether to terminate episodes at the second to last port (default: False)
    - take_first_action: whether to automaticlly place the first container of every episode (default: False)
    - strict_mask: whether to break when an invalid action is taken. Otherwise a penalty of -10 is given (default: False)
    """

    def __init__(
        self,
        R: int,
        C: int,
        N: int,
        skip_last_port: bool = False,
        take_first_action: bool = False,
        strict_mask: bool = False,
    ):
        super().__init__()
        assert R > 0, f"R must be positive but was {R}"
        assert C > 0, f"C must be positive but was {C}"
        assert N > 0, f"N must be positive but was {N}"
        self.R = R
        self.C = C
        self.N = N
        self._env = None
        self.visualizer = None
        self.skip_last_port = skip_last_port
        self.take_first_action = take_first_action
        self.strict_mask = strict_mask
        self.action_probs = None
        self.total_reward = 0

        self._set_gym_interface()

    def _set_gym_interface(self):
        self.render_mode = "human"
        self.action_space = spaces.Discrete(2 * self.C)
        one_hot_bay_def = spaces.Box(
            low=0, high=1, shape=(self.N - 1, self.R, self.C), dtype=np.int32
        )
        flat_T__def = spaces.Box(
            low=0,
            high=self.R * self.C,
            shape=(self.N * (self.N - 1) // 2,),
            dtype=np.int32,
        )
        self.observation_space = spaces.Dict(
            {
                "one_hot_bay": one_hot_bay_def,
                "flat_T": flat_T__def,
            }
        )

    @property
    def moves_to_solve(self):
        return self.containers_placed + self.containers_left

    def step(self, action: int):
        assert (
            0 <= action < 2 * self.C
        ), f"Action must be in the range [0, 2C). The first C actions correspond to adding a container into the respective column, the last C actions correspond to removing a container from the respective column."
        if self.strict_mask:
            assert (
                self._mask[action] == 1
            ), f"The action {action} is not allowed. The mask is {self._mask}"

        reward = -10

        if self._mask[action] == 1:
            step_info = c_lib.step(self._env, action)
            reward = step_info.reward
            self.terminal = bool(step_info.is_terminal)

        self.total_reward += reward

        if action < self.C:
            self.containers_placed += 1

        self.containers_left = np.sum(self.flat_T)

        return (
            self._get_observation(),
            reward,
            self.terminal,
            False,
            {},
        )

    def _reset_random_c_env(self, seed: int = None):
        if self._env is not None:
            c_lib.free_env(self._env)

        if seed is not None:
            c_lib.set_seed(seed)

        self._env = c_lib.get_random_env(
            self.R, self.C, self.N, int(self.skip_last_port)
        )
        self._set_numpy_views()

    def _reset_specific_c_env(self, transportation: np.ndarray):
        if self._env is not None:
            c_lib.free_env(self._env)

        self._env = c_lib.get_specific_env(
            self.R,
            self.C,
            self.N,
            transportation.ctypes.data_as(POINTER(c_int)),
            int(self.skip_last_port),
        )
        self._set_numpy_views()

    def render(self):
        if self.visualizer == None:
            self.visualizer = Visualizer(self.R, self.C, self.N)

        return self.visualizer.render(
            self.bay, self.T, self.total_reward, self.action_probs
        )

    def _reset_constants(self):
        self.total_reward = 0
        self.containers_placed = 0
        self.terminal = False
        self.containers_left = np.sum(self.flat_T)
        self.action_probs = None

    def reset(self, seed: int = None, options=None):
        self._reset_random_c_env(seed)
        self._reset_constants()

        if self.take_first_action:
            self.step(0)

        return self._get_observation(), {}

    def _assert_transportation(self, transportation: np.ndarray):
        assert (
            transportation.dtype == np.int32
        ), f"Transportation matrix must be of type np.int32 but was {transportation.dtype}"
        assert transportation.shape == (
            self.N,
            self.N,
        ), f"Transportation matrix must be of shape (N, N) = ({self.N}, {self.N}) but was {transportation.shape}"
        assert np.allclose(
            transportation, np.triu(transportation)
        ), "Transportation matrix must be upper triangular"
        assert np.any(
            transportation[0, :] != 0
        ), "Transportation matrix must have at least one non-zero element in the first row"
        assert np.all(
            transportation >= 0
        ), "Transportation matrix must not contain negative values"
        assert self._is_feasible(
            transportation
        ), "Transportation matrix is not feasible. This means that the stowage plan requires more containers to be shipped than the bay can hold."

    def _is_feasible(self, transportation: np.ndarray):
        capacity = self.R * self.C

        for i in range(self.N):
            total = 0
            for k in range(i + 1):
                for j in range(i + 1, self.N):
                    total += transportation[k, j]
            if total > capacity:
                return False

        return True

    def reset_to_transportation(self, transportation: np.ndarray):
        self._assert_transportation(transportation)
        self._reset_specific_c_env(transportation)
        self._reset_constants()

        if self.take_first_action:
            self.step(0)

        return self._get_observation(), {}

    def _get_observation(self):
        return {
            "one_hot_bay": self.one_hot_bay,
            "flat_T": self.flat_T / (self.R * self.C),  # Normalize to [0, 1]
        }

    def action_masks(self):
        return self._mask.copy()

    @property
    def bay(self):
        return self._bay.copy()

    @property
    def one_hot_bay(self):
        return self._one_hot_bay.copy()

    @property
    def T(self):
        return self._T.copy()

    @property
    def flat_T(self):
        return self._flat_T.copy()

    def print(self):
        print("Bay:")
        print(self._bay)
        print("T:")
        print(self._T)
        print("Mask:")
        print(self._mask)
        print()

    def close(self):
        if self._env is not None:
            c_lib.free_env(self._env)
            self._env = None

    def __del__(self):
        if self._env is not None:
            warnings.warn(
                "Env was not closed properly. Please call .close() to avoid memory leaks."
            )
            self.close()

    def __hash__(self):
        return hash(self.bay.tobytes() + self.flat_T.tobytes())

    def __eq__(self, other):
        return np.array_equal(self.bay, other.bay) and np.array_equal(
            self.flat_T, other.flat_T
        )

    def _set_numpy_views(self):
        # The following numpy arrays are views of the underlying C arrays
        # You should always .copy() them before using them
        self._bay = np.ctypeslib.as_array(
            self._env.bay.matrix.values, shape=(self.R, self.C)
        )
        self._T = np.ctypeslib.as_array(
            self._env.T.contents.matrix.values, shape=(self.N, self.N)
        )
        self._mask = np.ctypeslib.as_array(
            self._env.bay.mask.values, shape=(2 * self.C,)
        )
        self._flat_T = np.ctypeslib.as_array(
            self._env.flat_T_matrix.values, shape=((self.N - 1) * self.N // 2,)
        )
        self._one_hot_bay = np.ctypeslib.as_array(
            self._env.one_hot_bay.values, shape=(self.N - 1, self.R, self.C)
        )

    def copy(self):
        new_env = Env(
            self.R,
            self.C,
            self.N,
            self.skip_last_port,
            self.take_first_action,
            self.strict_mask,
        )
        new_env._env = c_lib.copy_env(self._env)
        new_env._set_numpy_views()
        new_env.total_reward = self.total_reward
        new_env.containers_placed = self.containers_placed
        new_env.containers_left = self.containers_left
        new_env.action_probs = self.action_probs
        new_env.terminal = self.terminal

        return new_env
