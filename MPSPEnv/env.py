from MPSPEnv.c_interface import c_lib, Env as c_Env
import numpy as np
from gymnasium import spaces
import gymnasium as gym
from MPSPEnv.visualizer import Visualizer
import warnings
import ctypes


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
    - strict_mask: whether to break when an invalid action is taken. Otherwise a penalty of -10 is given (default: False)
    - speedy: whether to skip the gym interface and return observations as None (default: False)
    """

    def __init__(
        self,
        R: int,
        C: int,
        N: int,
        skip_last_port: bool = False,
        take_first_action: bool = False,
        strict_mask: bool = False,
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
        self.strict_mask = strict_mask
        self.speedy = speedy
        self.action_probs = None
        self.total_reward = 0
        self._port_tracker = 0
        self.reshuffles_per_port = 0
        self.steps_taken = 0

        if not self.speedy:
            self._set_gym_interface()

    def step(self, action: int):
        assert self._env is not None, "The environment must be reset before stepping."
        assert (
            0 <= action < 2 * self.C
        ), f"Action must be in the range [0, 2C). The first C actions correspond to adding a container into the respective column, the last C actions correspond to removing a container from the respective column."
        if self.strict_mask:
            assert (
                self.mask_store.ndarray[action] == 1
            ), f"The action {action} is not allowed. The mask is {self.mask_store.ndarray}"

        reward = -10
        self.steps_taken += 1

        if self.mask_store.ndarray[action] == 1:
            step_info = c_lib.step(self._env, action)
            reward = step_info.reward
            self.terminal = bool(step_info.is_terminal)

        if self._port_tracker != self._env.T.contents.current_port:
            self._port_tracker = self._env.T.contents.current_port
            self.reshuffles_per_port = 0

        self.reshuffles_per_port += reward
        self.total_reward += reward

        if not self.speedy:
            return (
                self._get_observation(),
                reward,
                self.terminal,
                False,
                {},
            )
        else:
            return None

    def copy(self, track_history: bool = True):
        new_env = Env(
            self.R,
            self.C,
            self.N,
            self.skip_last_port,
            self.take_first_action,
            self.strict_mask,
            self.speedy,
        )
        new_env._env = c_lib.copy_env(self._env, int(track_history))
        new_env.total_reward = self.total_reward
        new_env.action_probs = self.action_probs
        new_env.terminal = self.terminal
        new_env._port_tracker = self._port_tracker
        new_env.reshuffles_per_port = self.reshuffles_per_port
        new_env.steps_taken = self.steps_taken
        new_env._set_stores()

        return new_env

    def reset(self, seed: int = None, options=None):
        self._reset_random_c_env(seed)
        self._set_stores()
        self._reset_constants()

        if self.take_first_action:
            self.step(0)

        if not self.speedy:
            return self._get_observation(), {}
        else:
            return None

    def reset_to_transportation(self, transportation: np.ndarray):
        self._assert_transportation(transportation)
        self._reset_specific_c_env(transportation)
        self._set_stores()
        self._reset_constants()

        if self.take_first_action:
            self.step(0)

        if not self.speedy:
            return self._get_observation(), {}
        else:
            return None

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

    @property
    def moves_upper_bound(self) -> int:
        return c_lib.get_moves_upper_bound(self._env)

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
    def one_hot_bay(self) -> np.ndarray:
        return self.one_hot_bay_store.ndarray.copy()

    @property
    def T(self) -> np.ndarray:
        return self.T_store.ndarray.copy()

    @property
    def flat_T(self) -> np.ndarray:
        return self.flat_T_store.ndarray.copy()

    @property
    def mask(self) -> np.ndarray:
        return self.mask_store.ndarray.copy()

    @property
    def history(self) -> np.ndarray:
        if type(self.history_store) == type(None):
            n_states = self.R * self.C * (self.N - 1)
            char_array = ctypes.cast(
                self._env.history,
                ctypes.POINTER(ctypes.c_char * n_states * self.R * self.C),
            ).contents
            self.history_store = np.frombuffer(char_array, dtype=np.int8).reshape(
                (n_states, self.R, self.C)
            )

        return self.history_store[: self._env.history_index[0] + 1].copy()

    def action_masks(self) -> np.ndarray:
        return self.mask

    def _set_stores(self):
        self.bay_store = LazyNdarray(self._env, ["bay", "matrix"], (self.R, self.C))
        self.T_store = LazyNdarray(
            self._env, ["T", "contents", "matrix"], (self.N, self.N)
        )
        self.mask_store = LazyNdarray(self._env, ["bay", "mask"], (2 * self.C,))
        self.one_hot_bay_store = LazyNdarray(
            self._env, ["one_hot_bay"], (self.N - 1, self.R, self.C)
        )
        self.flat_T_store = LazyNdarray(
            self._env, ["flat_T_matrix"], ((self.N - 1) * self.N // 2,)
        )
        self.history_store = None

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
            "one_hot_bay": self.one_hot_bay,
            "flat_T": self.flat_T / (self.R * self.C),  # Normalize to [0, 1]
        }

    def _set_gym_interface(self):
        self.render_mode = "human"
        self.action_space = spaces.Discrete(2 * self.C)
        one_hot_bay_def = spaces.Box(
            low=0, high=1, shape=(self.N - 1, self.R, self.C), dtype=np.int32
        )
        flat_T_def = spaces.Box(
            low=0,
            high=self.R * self.C,
            shape=(self.N * (self.N - 1) // 2,),
            dtype=np.int32,
        )
        self.observation_space = spaces.Dict(
            {
                "one_hot_bay": one_hot_bay_def,
                "flat_T": flat_T_def,
            }
        )

    def _reset_random_c_env(self, seed: int = None):
        if self._env is not None:
            c_lib.free_env(self._env)

        if seed is not None:
            c_lib.set_seed(seed)

        self._env = c_lib.get_random_env(
            self.R, self.C, self.N, int(self.skip_last_port), int(True)
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
            int(True),
        )

    def __del__(self):
        if self._env is not None:
            warnings.warn(
                "Env was not closed properly. Please call .close() to avoid memory leaks."
            )
            self.close()

    def __hash__(self):
        return hash(
            self.bay_store.ndarray.tobytes() + self.flat_T_store.ndarray.tobytes()
        )

    def __eq__(self, other: "Env"):
        return np.array_equal(
            self.bay_store.ndarray, other.bay_store.ndarray
        ) and np.array_equal(self.flat_T_store.ndarray, other.flat_T_store.ndarray)
