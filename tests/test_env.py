import numpy as np
import pytest
import subprocess
import glob
import json
from create_test_plan import string_to_action


@pytest.fixture(scope="session", autouse=True)
def build_env():
    subprocess.run("cd MPSPEnv && make build", shell=True)  # Setup
    yield
    subprocess.run("cd MPSPEnv && make clean", shell=True)  # Teardown


def test_basic():
    from MPSPEnv import Env

    env = Env(2, 2, 4, skip_last_port=True, should_reorder=False)
    env.reset_to_transportation(
        np.array(
            [
                [0, 2, 0, 2],
                [0, 0, 2, 0],
                [0, 0, 0, 2],
                [0, 0, 0, 0],
            ],
            dtype=np.int32,
        ),
    )
    env.step(0)
    expected_bay = np.array(
        [
            [0, 0],
            [3, 0],
        ],
        dtype=np.int32,
    )
    assert np.all(env.bay == expected_bay)

    env.close()


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


def test_local_plans():
    rollouts = get_rollouts()

    for rollout in rollouts:
        env = recreate_env(rollout["settings"], rollout["seed"])
        run_env_against_rollout(env, rollout["states"])
        env.close()
