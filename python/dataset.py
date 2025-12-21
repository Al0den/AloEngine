import json
import torch
from torch.utils.data import Dataset, DataLoader
import chess

from feature import fen_to_features

class NNUEDataset(Dataset):
    def __init__(self, jsonl_path, max_abs_cp=4410.0):
        self.samples = []
        self.max_abs_cp = max_abs_cp

        with open(jsonl_path, "r") as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                obj = json.loads(line)
                fen = obj["fen"]
                eval_cp = float(obj["eval"])
                self.samples.append((fen, eval_cp))

    def __len__(self):
        return len(self.samples)

    def __getitem__(self, idx):
        fen, eval_cp = self.samples[idx]
        x = fen_to_features(fen)
        y = max(-self.max_abs_cp, min(self.max_abs_cp, eval_cp)) / self.max_abs_cp
        y = torch.tensor(y, dtype=torch.float32)
        return x, y
