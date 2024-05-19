from MPSPEnv.c_interface import c_lib, Env as c_Env
import numpy as np
import gymnasium as gym
from MPSPEnv.visualizer import Visualizer
import warnings
import ctypes


class ActionNotAllowed(Exception):
    pass


class LazyNdarray:
    def __init__(self, env: c_Env, attributes: list[str], shape: tuple[int, int]):
        self.env = env
        self.attributes = attributes
        self.shape = shape
        self.store = None

    @property
    def ndarray(self):
        if self.store is None:
            self._build_store()

        return self.store

    def _build_store(self):
        self.store = self.env
        for attr in self.attributes:
            self.store = getattr(self.store, attr)

        self.store = np.ctypeslib.as_array(self.store.values, shape=self.shape)


class Env(gym.Env):
    """
    Gym environment for the Multi-Port Stowage Planning Problem (MPSP).
    The environment is defined by the following parameters:
    - R: number of rows in the bay
    - C: number of columns in the bay
    - N: number of ports
    - skip_last_port: whether to terminate episodes at the second to last port (default: False)
    - take_first_action: whether to automaticlly place the first container of every episode (default: False)
    - should_reorder: whether to reorder the columns of the bay to lexicographical order after each step (default: True)
    """

    def __init__(
        self,
        R: int,
        C: int,
        N: int,
        skip_last_port: bool = False,
        take_first_action: bool = False,
        should_reorder: bool = True,
        speedy: bool = False,
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
        self.should_reorder = should_reorder
        self.speedy = speedy
        self.action_probs = None
        self.total_reward = 0

    def step(self, action: int):
        assert self._env is not None, "The environment must be reset before stepping."
        self._check_action(action)

        step_info = c_lib.step(self._env, action)
        reward = step_info.reward
        self.terminal = bool(step_info.is_terminal)
        self.total_reward += reward

        if self.speedy:
            return None
        else:
            return (
                self._get_observation(),
                reward,
                self.terminal,
                False,
                {},
            )

    def copy(self):
        new_env = Env(
            R=self.R,
            C=self.C,
            N=self.N,
            skip_last_port=self.skip_last_port,
            take_first_action=self.take_first_action,
            should_reorder=self.should_reorder,
        )
        new_env._env = c_lib.copy_env(self._env)
        new_env.total_reward = self.total_reward
        new_env.action_probs = self.action_probs
        new_env.terminal = self.terminal
        new_env._set_stores()

        return new_env

    def reset(self, seed: int = None, options=None):
        self._reset_random_c_env(seed)
        self._set_stores()
        self._reset_constants()

        if self.take_first_action:
            self.step(0)

        if self.speedy:
            return None
        else:
            return self._get_observation(), {}

    def reset_to_transportation(self, transportation: np.ndarray):
        self._assert_transportation(transportation)
        self._reset_specific_c_env(transportation)
        self._set_stores()
        self._reset_constants()

        if self.take_first_action:
            self.step(0)

        if self.speedy:
            return None
        else:
            return self._get_observation(), {}

    def render(self):
        if self.visualizer == None:
            self.visualizer = Visualizer(self.R, self.C, self.N)

        return self.visualizer.render(
            self.bay, self.T, self.total_reward, self.action_probs
        )

    def close(self):
        if self._env is not None:
            c_lib.free_env(self._env)
            self._env = None

    def _check_action(self, action: int):
        if 0 > action or action >= 2 * self.C * self.R:
            raise ActionNotAllowed
        if self.mask_store.ndarray[action] != 1:
            raise ActionNotAllowed

    @property
    def containers_left(self) -> int:
        return self._env.T.contents.containers_left

    @property
    def moves_to_solve(self) -> int:
        return (
            self._env.T.contents.containers_placed
            + self._env.T.contents.containers_left
        )

    @property
    def remaining_ports(self) -> int:
        return self.N - 1 - self._env.T.contents.current_port

    @property
    def bay(self) -> np.ndarray:
        return self.bay_store.ndarray.copy()

    @property
    def T(self) -> np.ndarray:
        return self.T_store.ndarray.copy()

    @property
    def flat_T(self) -> np.ndarray:
        return self.flat_T_store.ndarray.copy()

    @property
    def mask(self) -> np.ndarray:
        return self.mask_store.ndarray.copy()

    def _set_stores(self):
        self.bay_store = LazyNdarray(self._env, ["bay", "matrix"], (self.R, self.C))
        self.T_store = LazyNdarray(
            self._env, ["T", "contents", "matrix"], (self.N, self.N)
        )
        self.flat_T_store = LazyNdarray(
            self._env, ["flat_T_matrix"], ((self.N - 1) * self.N // 2,)
        )
        self.mask_store = LazyNdarray(self._env, ["mask"], (2 * self.C * self.R,))

    def _reset_constants(self):
        self.total_reward = 0
        self.terminal = False
        self.action_probs = None

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

    def _get_observation(self):
        return {
            "bay": self.bay / self.remaining_ports,
            "flat_T": self.flat_T / (self.R * self.C),
            "mask": self.mask,
            "containers_left": self.containers_left,
        }

    def _reset_random_c_env(self, seed: int = None):
        if self._env is not None:
            c_lib.free_env(self._env)

        if seed is not None:
            c_lib.set_seed(seed)
        else:
            c_lib.set_seed(np.random.randint(0, 2**32))

        self._env = c_lib.get_random_env(
            self.R, self.C, self.N, int(self.skip_last_port), int(self.should_reorder)
        )

    def _reset_specific_c_env(self, transportation: np.ndarray):
        if self._env is not None:
            c_lib.free_env(self._env)

        self._env = c_lib.get_specific_env(
            self.R,
            self.C,
            self.N,
            transportation.ctypes.data_as(ctypes.POINTER(ctypes.c_int)),
            int(self.skip_last_port),
            int(self.should_reorder),
        )

    def __del__(self):
        if self._env is not None:
            warnings.warn(
                "Env was not closed properly. Please call .close() to avoid memory leaks."
            )
            self.close()

    def __hash__(self):
        return hash(
            # !!!! PLUS MASK !!!!
            self.bay_store.ndarray.tobytes()
            + self.flat_T_store.ndarray.tobytes()
        )

    def __eq__(self, other: "Env"):
        # !!!! PLUS MASK !!!!
        return np.array_equal(
            self.bay_store.ndarray, other.bay_store.ndarray
        ) and np.array_equal(self.flat_T_store.ndarray, other.flat_T_store.ndarray)
