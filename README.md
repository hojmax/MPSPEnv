This package implements the mathematical model of the "Multi Port Stowage Planning" (MPSP) problem, introduced in [Avriel et. al 1998](https://www.researchgate.net/publication/242916342_Stowage_planning_for_container_ships_to_reduce_the_number_of_shifts), as a reinforcement learning environment using the Gymnasium api. Please see the [Gymnasiun documentation](https://gymnasium.farama.org/api/env/) for more information on what functions are available.

The Env class has the following input parameters:

- **R** (int): number of rows
- **C** (int): number of columns
- **N** (int): number of ports
- **skip_last_port** (boolean): if true, the environment will terminate at the second to last port. This is useful for speeding up training, as the last port is trivial to solve.

Note that since the mathematical model disregards any stability constraints, the env automatically swaps the column order in a lexocographic manor, based on the containers in the columns. This reduces the state space, by a factor of almost $C!$. Furthmore, the transportation matrix, is reshifted up and to the left, for each port, to reduce the state space even further.

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
import numpy as np
import pygame


def get_action_from_key(key):
    if key in [
        pygame.K_0,
        pygame.K_1,
        pygame.K_2,
        pygame.K_3,
        pygame.K_4,
        pygame.K_5,
        pygame.K_6,
        pygame.K_7,
    ]:
        return key - pygame.K_0
    return None


if __name__ == "__main__":
    env = Env(3, 4, 6)
    env.reset()

    running = True
    while running:
        env.render()

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
                break
            if event.type == pygame.KEYDOWN:
                action = get_action_from_key(event.key)
                if action is not None:
                    _, reward, done, _, _ = env.step(action)

                    if done:
                        running = False

        pygame.time.wait(10)

    pygame.time.wait(1000)
    env.close()
```
