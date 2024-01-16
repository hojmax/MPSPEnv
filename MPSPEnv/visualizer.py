import os
import seaborn as sns
import numpy as np

os.environ["PYGAME_HIDE_SUPPORT_PROMPT"] = "hide"
import pygame


class Visualizer:
    def __init__(self, R, C, N, scale=30):
        self.scale = scale
        self.R = R
        self.C = C
        self.N = N

        self.set_color_scheme(N)
        self.width = (C + N + 3) * self.scale
        self.height = (max(R, N) + 4) * self.scale
        pygame.init()
        pygame.display.init()
        self.screen = pygame.display.set_mode((self.width, self.height))

    def set_color_scheme(self, N):
        self.container_colors = sns.color_palette("husl", N - 1).as_hex()
        self.container_colors.insert(0, "#ffffff")
        self.prob_colors = sns.color_palette("coolwarm", as_cmap=True)

    def find_blocking_containers(self, Bay):
        blocking_containers = np.zeros_like(Bay)

        for j in range(self.C):
            min_in_column = self.N
            for i in range(self.R - 1, -1, -1):
                if Bay[i, j] == 0:
                    break

                if Bay[i, j] < min_in_column:
                    min_in_column = Bay[i, j]
                elif Bay[i, j] > min_in_column:
                    blocking_containers[i, j] = 1

        return blocking_containers

    def render(self, Bay, T, sum_reward, action_probs):
        pygame.event.pump()
        self.screen.fill((255, 255, 255))
        self._render_bay(Bay)
        self._render_T(T)
        self._render_reward(sum_reward)
        self._render_action_probs(action_probs)
        pygame.display.flip()
        return np.transpose(pygame.surfarray.array3d(self.screen), (1, 0, 2))

    def _render_reward(self, sum_reward):
        if sum_reward is None:
            return

        x = self.width - self.scale * (1.5 + self.N / 2)
        y = self.height - 2 * self.scale
        self.show_text("Reward: " + str(sum_reward), x, y)

    def _render_bay(self, Bay):
        blocking_containers = self.find_blocking_containers(Bay)
        for i in range(self.R):
            for j in range(self.C):
                if Bay[i, j] == 0:
                    continue
                color = self.container_colors[Bay[i, j]]
                x = (j + 1) * self.scale
                y = (i + 1) * self.scale
                rect = pygame.Rect(x, y, self.scale, self.scale)
                pygame.draw.rect(self.screen, color, rect)

                if blocking_containers[i, j] == 1:
                    self.show_text("x", x, y)

        self.draw_grid(1, 1, self.R, self.C)

    def _render_action_probs(self, action_probs):
        if action_probs is None:
            return

        for i in range(2):
            for j in range(self.C):
                index = i * self.C + j
                x = (j + 1) * self.scale
                y = (self.R + 1.5 + i) * self.scale
                rect = pygame.Rect(x, y, self.scale, self.scale)
                color = self.prob_colors(action_probs[index])
                color = tuple([int(255 * c) for c in color])
                pygame.draw.rect(self.screen, color, rect)
                text = f"{action_probs[index]:.2f}"

                if text == "0.00":
                    text = "0"
                elif text == "1.00":
                    text = "1"
                elif text[0] == "0":
                    text = text[1:]
                self.show_text(text, x, y, size=self.scale - 5)

        self.draw_grid(1, self.R + 1.5, 2, self.C)

    def _render_T(self, T):
        for i in range(self.N - 1):
            for j in range(i + 1, self.N):
                color = self.container_colors[j]
                x = (self.C + 2 + j) * self.scale
                y = (i + 1) * self.scale
                rect = pygame.Rect(x, y, self.scale, self.scale)
                pygame.draw.rect(self.screen, color, rect)

                if T[i, j] != 0:
                    self.show_text(str(T[i, j]), x, y)

        self.draw_grid(self.C + 2, 1, self.N, self.N)

    def show_text(self, text, x, y, size=None):
        if size is None:
            size = self.scale - 5

        rect = pygame.Rect(x, y, self.scale, self.scale)
        font = pygame.font.Font(None, size)
        text = font.render(text, True, (0, 0, 0))
        textpos = text.get_rect()
        textpos.centerx = rect.centerx
        textpos.centery = rect.centery
        self.screen.blit(text, textpos)

    def draw_grid(self, x, y, rows, cols):
        for i in range(rows + 1):
            pygame.draw.line(
                self.screen,
                (0, 0, 0),
                (x * self.scale, (y + i) * self.scale),
                ((x + cols) * self.scale, (y + i) * self.scale),
            )

        for i in range(cols + 1):
            pygame.draw.line(
                self.screen,
                (0, 0, 0),
                ((x + i) * self.scale, y * self.scale),
                ((x + i) * self.scale, (y + rows) * self.scale),
            )
