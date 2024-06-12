import sys
import os
import numpy as np
import subprocess
import re
import json
import argparse


def setup():
    subprocess.run("make build", shell=True)


def teardown():
    subprocess.run("make clean", shell=True)


def take_snapshot(env, action=None):
    snapshot = {
        "bay": env.bay.tolist(),
        "T": env.T.tolist(),
        "mask": env.mask.tolist(),
        "total_reward": env.total_reward,
        "containers_placed": env.containers_placed,
        "containers_left": env.containers_left,
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


def action_to_string(action, R, C):
    if action >= R * C:
        return f"r{action % R + 1}c{(action - R * C) // R}"
    else:
        return f"a{action % R + 1}c{action // R}"


def print_snapshot(snapshot):
    print("Bay:")
    for row in snapshot["bay"]:
        print(row)
    print("T:")
    for row in snapshot["T"]:
        print(row)
    print("Mask:")
    print(snapshot["mask"])
    print("Total reward:", snapshot["total_reward"])
    print("Containers placed:", snapshot["containers_placed"])
    print("Containers left:", snapshot["containers_left"])
    print("Valid actions:")
    for action, is_valid in enumerate(snapshot["mask"]):
        if is_valid == 1:
            print(
                action_to_string(action, len(snapshot["bay"]), len(snapshot["bay"][0])),
                end=" ",
            )
    print()


def run_interactive_game(env):
    states = [take_snapshot(env)]
    print(
        "Actions follow the format ^[r|a](\d+)c(\d+)$. E.g. r1c0 for removing 1 container from column 0, or a3c2 for adding 3 containers to column 2. Columns are zero-indexed."
    )
    print("Initial state:")
    print_snapshot(take_snapshot(env))

    while not env.terminated:
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
        print()
        print_snapshot(states[-1])

    return states


def save_states(states, settings, seed, filename):
    with open(filename, "w") as f:
        json.dump({"settings": settings, "seed": seed, "states": states}, f)


def set_path():
    local_env_path = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    sys.path.insert(0, local_env_path)


def get_filename(settings, seed):
    return f"R{settings['R']}_C{settings['C']}_N{settings['N']}_seed{seed}.json"


def get_args():
    parser = argparse.ArgumentParser(
        description="Run the MPSP environment with specified settings."
    )
    parser.add_argument("--R", type=int, default=8, help="Number of rows")
    parser.add_argument("--C", type=int, default=8, help="Number of columns")
    parser.add_argument("--N", type=int, default=8, help="Number of containers")
    parser.add_argument("--auto-move", action="store_true", help="Enable auto_move")
    parser.add_argument(
        "--no-auto-move",
        action="store_false",
        dest="auto_move",
        help="Disable auto_move",
    )
    parser.add_argument(
        "--speedy", type=bool, default=True, help="Enable speedy mode (True/False)"
    )
    parser.add_argument("--seed", type=int, default=0, help="Random seed")
    parser.add_argument(
        "--premove",
        type=str,
        help='Premove comma-separated actions, e.g. "r1c0,a2c1"',
    )

    args = parser.parse_args()

    return args


def premove_actions(env, actions):
    actions = actions.split(",")
    for action in actions:
        action = string_to_action(action, env.R, env.C)
        env.step(action)


if __name__ == "__main__":
    args = get_args()

    setup()
    set_path()
    from MPSPEnv import Env, ActionNotAllowed

    settings = {
        "R": args.R,
        "C": args.C,
        "N": args.N,
        "auto_move": args.auto_move,
        "speedy": args.speedy,
    }
    seed = args.seed

    env = Env(**settings)
    env.reset(seed)

    if args.premove:
        premove_actions(env, args.premove)

    states = run_interactive_game(env)
    save_states(states, settings, seed, get_filename(settings, seed))
    env.close()

    teardown()
