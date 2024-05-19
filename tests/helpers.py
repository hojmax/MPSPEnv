import glob
import json
from tests.create_rollout import string_to_action
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
