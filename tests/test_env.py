import numpy as np
import pytest
import subprocess


@pytest.fixture(scope="session", autouse=True)
def build_env():
    subprocess.run("cd MPSPEnv && make build", shell=True)  # Setup
    yield
    subprocess.run("cd MPSPEnv && make clean", shell=True)  # Teardown


def test_history():
    from MPSPEnv import Env

    env = Env(2, 2, 4, skip_last_port=True)
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
    env.step(1)
    env.step(0)
    env.step(3)
    env.step(0)
    env.step(1)
    env.step(0)
    env.step(0)
    env.step(0)
    expected_history = np.array(
        [
            [[0, 0], [0, 0]],
            [[0, 0], [0, 0]],
            [[0, 1], [0, 0]],
            [[0, 1], [0, 0]],
            [[0, 0], [0, 0]],
            [[1, 0], [0, 0]],
            [[0, 0], [0, 0]],
            [[0, 0], [0, 0]],
            [[0, 0], [0, 0]],
            [[0, 0], [0, 0]],
        ],
        dtype=np.int8,
    )
    assert np.all(env.history == expected_history)
    env.close()
