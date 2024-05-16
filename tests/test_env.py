import numpy as np
import pytest
import subprocess


@pytest.fixture(scope="session", autouse=True)
def build_env():
    subprocess.run("cd MPSPEnv && make build", shell=True)  # Setup
    yield
    subprocess.run("cd MPSPEnv && make clean", shell=True)  # Teardown


def test_no_reorder():
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
