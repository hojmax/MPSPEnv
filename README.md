This package implements the mathematical model of the "Multi Port Stowage Planning" (MPSP) problem, introduced in [Avriel et al. 1998](https://www.researchgate.net/publication/242916342_Stowage_planning_for_container_ships_to_reduce_the_number_of_shifts), as a reinforcement learning environment using the [Gymnasium API](https://gymnasium.farama.org/api/env/).

The Env class has the following input parameters:

- **R** (int): number of rows
- **C** (int): number of columns
- **N** (int): number of ports
- **skip_last_port** (boolean): if true, the environment will terminate at the second to last port. This is useful for speeding up training, as the last port is trivial to solve.
- **take_first_action** (boolean): if true, the environment automaticlly places the first container of every episode. This is useful for speeding up training, as the first action is trivial.
- **strict_mask** (boolean): if true, the environment will break when an invalid action is taken. Otherwise a penalty of -10 is given, and no change is made to the state.

The env observations consist of a dictionary with the one hot encoded bay, and the upper triangular part of the transportation matrix as a flattened array.

![](https://raw.githubusercontent.com/hojmax/MPSPEnv/main/env.png)
*Screenshot from the environment. The left matrix is the bay, and the right matrix is the transportation matrix. "x" denotes blocking containers. The reward decreases by one for each reshuffle necessitated by the agents stowage plan.*

Note that since the mathematical model disregards any stability constraints, the env automatically swaps the column order in a lexocographic manor, based on the containers in the columns. This reduces the state space, by a factor of almost $C!$. Furthmore, the transportation matrix, is reshifted up and to the left, for each port, to reduce the state space even further. This means that from the point of view of the agent, it never leaves the first port.

When using the environment, you should call `env.close()` before exiting your program, to avoid memory leaks.

Minimal usage with [Stable Baselines](https://stable-baselines.readthedocs.io/en/master/):

```python
from MPSPEnv import Env
from sb3_contrib.ppo_mask import MaskablePPO

train_env = Env(R=8, C=4, N=5, skip_last_port=True)

model = MaskablePPO(
    "MultiInputPolicy",
    train_env
)

model.learn(
    total_timesteps=1e6,
    progress_bar=True
)

train_env.close()
```

If you wish to try the env yourself, placing containers using keys 0-3 and removing containers using keys 4-7, you can use the following script:

```python
from MPSPEnv import Env
import pygame, numpy as np


def get_action_from_key(key):
    return key - pygame.K_0 if pygame.K_0 <= key <= pygame.K_7 else None


if __name__ == "__main__":
    env = Env(3, 4, 6)
    env.reset()

    while True:
        env.render()

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                env.close()
                exit()
            elif (
                event.type == pygame.KEYDOWN
                and (action := get_action_from_key(event.key)) is not None
            ):
                _, reward, done, _, _ = env.step(action)
                if done:
                    env.render()
                    pygame.time.wait(1000)
                    env.close()
                    exit()

        pygame.time.wait(10)
```
