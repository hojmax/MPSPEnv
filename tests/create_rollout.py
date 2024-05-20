import sys
import os
import numpy as np
import subprocess
import re
import json


def setup():
    subprocess.run("make build", shell=True)


def teardown():
    subprocess.run("make clean", shell=True)


def take_snapshot(env, action=None):
    snapshot = {
        "bay": env.bay.tolist(),
        "T": env.T.tolist(),
        "mask": env.mask.tolist(),
    }

    if action:
        snapshot["action"] = action

    return snapshot


def string_to_action(action, R, C):
    match = re.match(r"^(r|a)(\d+)c(\d+)$", action)
    if not match:
        raise ValueError("Invalid action format")
    action_type = match.group(1)
    n_containers = int(match.group(2))
    column = int(match.group(3))
    if n_containers < 0 or column < 0:
        raise ValueError("Invalid action format")

    if action_type == "r":
        return R * C + column * R + n_containers - 1
    elif action_type == "a":
        return column * R + n_containers - 1


def print_snapshot(snapshot):
    print("Bay:")
    for row in snapshot["bay"]:
        print(row)
    print("T:")
    for row in snapshot["T"]:
        print(row)
    print("Mask:")
    print(snapshot["mask"])


def run_interactive_game(env):
    states = [take_snapshot(env)]
    print(
        "Actions follow the format ^[r|a](\d+)c(\d+)$. E.g. r1c0 for removing 1 container from column 0, or a3c2 for adding 3 containers to column 2. Columns are zero-indexed."
    )
    print("Initial state:")
    print_snapshot(take_snapshot(env))

    while not env.terminal:
        action_str = input("Take action:")
        try:
            action = string_to_action(action_str, env.R, env.C)
        except ValueError:
            print("ERROR: Invalid action format")
            continue

        try:
            env.step(action)
        except ActionNotAllowed:
            print("ERROR: Action not allowed")
            continue

        states.append(take_snapshot(env, action_str))
        print_snapshot(states[-1])

    return states


def save_states(states, settings, seed, filename):
    with open(filename, "w") as f:
        json.dump({"states": states, "settings": settings, "seed": seed}, f)


def set_path():
    local_env_path = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    sys.path.insert(0, local_env_path)


if __name__ == "__main__":
    setup()
    set_path()
    from MPSPEnv import Env, ActionNotAllowed

    settings = {
        "R": 2,
        "C": 2,
        "N": 4,
        "auto_move": False,
        "speedy": True,
    }
    seed = 0

    env = Env(**settings)
    env.reset(seed)
    states = run_interactive_game(env)
    save_states(states, settings, seed, "new_rollout.json")
    env.close()

    teardown()
