# network.py
import torch
import torch.nn as nn
import torch.nn.functional as F

from feature import TOTAL_FEATURES

INPUT_DIM   = TOTAL_FEATURES   # 81920
HIDDEN_SIZE = 256              # pick what you want
FC2_SIZE    = 32


class NNUEHalfKP(nn.Module):
    def __init__(
        self,
        num_features: int = TOTAL_FEATURES,
        hidden_size: int = HIDDEN_SIZE,
        fc2_size: int = FC2_SIZE,
    ):
        super().__init__()

        # First layer: feature -> hidden, summed over active features
        self.embed = nn.EmbeddingBag(
            num_embeddings=num_features,
            embedding_dim=hidden_size,
            mode="sum",
            sparse=False,
        )

        # Explicit bias for the first (EmbeddingBag) layer
        self.bias1 = nn.Parameter(torch.zeros(hidden_size))

        # Small MLP on top
        self.fc2 = nn.Linear(hidden_size, fc2_size)
        self.out = nn.Linear(fc2_size, 1)

        self._init_weights()

    def _init_weights(self):
        # W1
        nn.init.normal_(self.embed.weight, mean=0.0, std=0.02)
        # B1
        nn.init.zeros_(self.bias1)

        # Second layer
        nn.init.kaiming_normal_(self.fc2.weight, nonlinearity="relu")
        nn.init.zeros_(self.fc2.bias)

        # Output layer
        nn.init.kaiming_normal_(self.out.weight, nonlinearity="linear")
        nn.init.zeros_(self.out.bias)

    def forward(self, indices: torch.Tensor, offsets: torch.Tensor) -> torch.Tensor:
        """
        indices: 1D LongTensor of concatenated feature indices for the whole batch.
        offsets: 1D LongTensor of size [batch_size].
        """
        # First layer: sum embeddings per position + bias
        h = self.embed(indices, offsets)      # [B, hidden_size]
        h = h + self.bias1                   # add bias1 to each row
        h = F.relu(h)

        # Small MLP head
        h = F.relu(self.fc2(h))               # [B, fc2_size]
        out = self.out(h).squeeze(-1)         # [B]

        return out


# Keep your existing import style
NNUEModel = NNUEHalfKP
