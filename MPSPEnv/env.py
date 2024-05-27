from MPSPEnv.c_interface import c_lib, Env as c_Env
from MPSPEnv.visualizer import Visualizer
import gymnasium as gym
import numpy as np
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
    """

    def __init__(
        self,
        R: int,
        C: int,
        N: int,
        auto_move: bool = False,
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
        self.auto_move = auto_move
        self.speedy = speedy

    def step(self, action: int):
        assert self._env is not None, "The environment must be reset before stepping."
        self._check_action(action)

        step_info = c_lib.env_step(self._env, action)

        if self.speedy:
            return None
        else:
            return (
                self._get_observation(),
                step_info.reward,
                step_info.terminated,
                False,
                {},
            )

    def copy(self):
        new_env = Env(
            R=self.R,
            C=self.C,
            N=self.N,
            auto_move=self.auto_move,
            speedy=self.speedy,
        )
        new_env._env = c_lib.copy_env(self._env)
        new_env._set_stores()

        return new_env

    def reset(self, seed: int = None, options=None):
        self._reset_random_c_env(seed)
        self._set_stores()

        if self.speedy:
            return None
        else:
            return self._get_observation(), {}

    def reset_to_transportation(self, transportation: np.ndarray):
        self._assert_transportation(transportation)
        self._reset_specific_c_env(transportation)
        self._set_stores()

        if self.speedy:
            return None
        else:
            return self._get_observation(), {}

    def render(self):
        if self.visualizer == None:
            self.visualizer = Visualizer(self.R, self.C, self.N)

        return self.visualizer.render(self.bay, self.T, self.total_reward)

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
    def total_reward(self) -> int:
        return self._env.total_reward.contents.value

    @property
    def containers_left(self) -> int:
        return self._env.containers_left.contents.value

    @property
    def containers_placed(self) -> int:
        return self._env.containers_placed.contents.value

    @property
    def terminated(self) -> bool:
        return self._env.terminated.contents.value

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
    def mask(self) -> np.ndarray:
        return self.mask_store.ndarray.copy()

    def _set_stores(self):
        self.bay_store = LazyNdarray(self._env, ["bay", "matrix"], (self.R, self.C))
        self.T_store = LazyNdarray(
            self._env, ["T", "contents", "matrix"], (self.N, self.N)
        )
        self.mask_store = LazyNdarray(self._env, ["mask"], (2 * self.C * self.R,))

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
        return {"bay": self.bay, "T": self.T, "mask": self.mask}

    def _reset_random_c_env(self, seed: int = None):
        self.close()

        if seed is not None:
            c_lib.set_seed(seed)
        else:
            c_lib.set_seed(np.random.randint(0, 2**32))

        self._env = c_lib.get_random_env(self.R, self.C, self.N, int(self.auto_move))

    def _reset_specific_c_env(self, transportation: np.ndarray):
        self.close()

        self._env = c_lib.get_specific_env(
            self.R,
            self.C,
            self.N,
            transportation.ctypes.data_as(ctypes.POINTER(ctypes.c_int)),
            int(self.auto_move),
        )

    def __del__(self):
        if self._env is not None:
            warnings.warn(
                "Env was not closed properly. Please call .close() to avoid memory leaks."
            )
            self.close()

    def __hash__(self):
        return hash(
            self.bay_store.ndarray.tobytes()
            + self.T_store.ndarray.tobytes()
            + self.mask_store.ndarray.tobytes()
        )

    def __eq__(self, other: "Env"):
        return (
            np.array_equal(self.mask_store.ndarray, other.mask_store.ndarray)
            and np.array_equal(self.bay_store.ndarray, other.bay_store.ndarray)
            and np.array_equal(self.T_store.ndarray, other.T_store.ndarray)
        )
