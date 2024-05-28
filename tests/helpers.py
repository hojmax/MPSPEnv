import glob
import json
from create_rollout import string_to_action
import numpy as np


def run_env_against_rollout(
    env,
    states,
):
    for state in states:
        action = state.get("action", None)

        if action:
            action = string_to_action(action, env.R, env.C)
            env.step(action)
        assert np.all(env.bay == state["bay"]), f"Bay: {env.bay} != {state['bay']}"
        assert np.all(env.T == state["T"]), f"T: {env.T} != {state['T']}"
        assert np.all(env.mask == state["mask"]), f"Mask: {env.mask} != {state['mask']}"
        assert (
            env.total_reward == state["total_reward"]
        ), f"Total Reward: {env.total_reward} != {state['total_reward']}"
        assert (
            env.containers_placed == state["containers_placed"]
        ), f"Containers Placed: {env.containers_placed} != {state['containers_placed']}"
        assert (
            env.containers_left == state["containers_left"]
        ), f"Containers Left: {env.containers_left} != {state['containers_left']}"


def get_rollouts():
    files = glob.glob("tests/reference_rollouts/*.json")
    rollouts = []

    for file in files:
        with open(file, "r") as f:
            rollouts.append(json.load(f))

    return rollouts


def recreate_env(settings, seed):
    from MPSPEnv import Env

    env = Env(**settings)
    env.reset(seed)
    return env


def get_random_settings():
    return {
        "R": np.random.randint(6, 13),
        "C": np.random.randint(2, 13),
        "N": np.random.randint(4, 17),
        "auto_move": np.random.choice([True, False]),
        "speedy": True,
    }


def get_initial_containers(settings, seed):
    from MPSPEnv import Env

    new_settings = {**settings, "auto_move": False}
    env = Env(**new_settings)
    env.reset(seed)
    initial_containers = env.containers_left
    env.close()

    return initial_containers


def get_additional_env_info(env):
    containers_per_port = np.ctypeslib.as_array(
        env._env.T.contents.containers_per_port.values, shape=(env.N,)
    )
    min_container_per_column = np.ctypeslib.as_array(
        env._env.bay.min_container_per_column.values, shape=(env.C,)
    )
    column_counts = np.ctypeslib.as_array(
        env._env.bay.column_counts.values, shape=(env.C,)
    )
    return {
        "containers_per_port": containers_per_port,
        "min_container_per_column": min_container_per_column,
        "column_counts": column_counts,
    }


def get_random_action(mask):
    return np.random.choice(np.where(mask == 1)[0])


def get_remaining_ports(T):
    num_cols = T.shape[1]
    for col in range(num_cols - 1, -1, -1):
        if np.any(T[:, col] != 0):
            return col
    return 0


def get_sorted_matrix(bay):
    array = bay.copy()

    for i in range(len(array) - 1, -1, -1):
        array = array[:, np.argsort(array[i])]

    return array


def floating_containers(bay):
    for col in range(bay.shape[1]):
        for row in range(bay.shape[0] - 1, -1, -1):
            if bay[row, col] == 0 and np.any(bay[:row, col] != 0):
                return True
    return False


def sanity_check_env(env, containers_per_port, min_container_per_column, column_counts):
    if env.terminated:
        assert np.all(env.mask == 0), "Mask should be all zeros"
        assert np.all(env.bay == 0), "Bay should be all zeros"
        assert np.all(env.T == 0), "T should be all zeros"
    else:
        assert np.any(env.mask), "Mask should not be all zeros"

    search_bay = env.bay
    search_bay[search_bay == 0] = env.N
    min_check = np.min(search_bay, axis=0)
    assert np.all(containers_per_port == np.sum(env.T, axis=1))
    assert np.all(
        min_container_per_column == min_check
    ), f"{min_container_per_column} != {min_check}, {env.bay}"
    assert np.all(np.sum(env.bay > 0, axis=0) == column_counts)
    assert env.remaining_ports == get_remaining_ports(env.T)
    assert np.all(get_sorted_matrix(env.bay) == env.bay)
    assert not floating_containers(env.bay)
    assert np.all(env.bay <= env.remaining_ports)
