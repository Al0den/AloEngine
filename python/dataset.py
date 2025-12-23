from bisect import bisect_right
from collections import OrderedDict
from pathlib import Path

import torch
from torch.utils.data import Dataset


class NNUEDataset(Dataset):
    def __init__(self, pt_path, cache_size=2):
        path = Path(pt_path)
        if path.is_dir():
            shards = sorted(path.glob("lichess_nnue_*.pt"))
            if not shards:
                raise FileNotFoundError(f"No lichess_nnue shards found in {path}")
        elif path.exists():
            shards = [path]
        else:
            shards = sorted(path.parent.glob("lichess_nnue_*.pt"))
            if not shards:
                raise FileNotFoundError(f"Path not found and no shards in {path.parent}")

        self.shards = shards
        self.shard_lengths = []
        self.shard_starts = []
        self.cache_size = max(1, int(cache_size))
        self._cache = OrderedDict()

        start = 0
        for shard in self.shards:
            data = torch.load(shard, map_location="cpu")

            if "num_samples" in data:
                n_feats = int(data["num_samples"])
            elif "lengths" in data:
                n_feats = int(data["lengths"].shape[0])
            elif "offsets" in data:
                n_feats = int(data["offsets"].shape[0])
            else:
                feats = data.get("features", data.get("X"))
                if feats is None:
                    raise KeyError(f"Missing features in shard: {shard}")
                if isinstance(feats, torch.Tensor):
                    n_feats = feats.shape[0] if feats.dim() > 0 else 1
                else:
                    n_feats = len(feats)

            targs = data.get("cp", data.get("Y"))
            if targs is None:
                raise KeyError(f"Missing targets in shard: {shard}")
            if isinstance(targs, torch.Tensor):
                n_targs = targs.shape[0] if targs.dim() > 0 else 1
            else:
                n_targs = len(targs)

            if n_feats != n_targs:
                raise ValueError(f"Features/targets size mismatch in shard: {shard}")

            self.shard_starts.append(start)
            self.shard_lengths.append(n_feats)
            start += n_feats

            del data

        self.total_len = start

    def __len__(self):
        return self.total_len

    def _pack_feature_lists(self, feature_lists):
        offsets = []
        lengths = []
        flat = []
        offset = 0
        for feats in feature_lists:
            offsets.append(offset)
            l = len(feats)
            lengths.append(l)
            flat.extend(feats)
            offset += l
        indices_t = torch.tensor(flat, dtype=torch.int32)
        offsets_t = torch.tensor(offsets, dtype=torch.int32)
        lengths_t = torch.tensor(lengths, dtype=torch.int32)
        return indices_t, offsets_t, lengths_t

    def _load_shard(self, shard_idx: int):
        if shard_idx in self._cache:
            self._cache.move_to_end(shard_idx)
            return self._cache[shard_idx]

        data = torch.load(self.shards[shard_idx], map_location="cpu")
        if "indices" in data and "offsets" in data:
            indices = data["indices"]
            offsets = data["offsets"]
            lengths = data.get("lengths")
            targs = data.get("cp", data.get("Y"))

            if targs is None:
                raise KeyError(f"Missing targets in shard: {self.shards[shard_idx]}")
            if lengths is None:
                lengths = torch.diff(
                    torch.cat([offsets, indices.new_tensor([indices.numel()])])
                )

            if not isinstance(indices, torch.Tensor):
                indices = torch.tensor(indices, dtype=torch.int32)
            if not isinstance(offsets, torch.Tensor):
                offsets = torch.tensor(offsets, dtype=torch.int32)
            if not isinstance(lengths, torch.Tensor):
                lengths = torch.tensor(lengths, dtype=torch.int32)

            if isinstance(targs, torch.Tensor):
                targs = targs.to(torch.float32)
            else:
                targs = torch.tensor(targs, dtype=torch.float32)

            entry = ("packed", indices, offsets, lengths, targs)
        else:
            feats = data.get("features", data.get("X"))
            targs = data.get("cp", data.get("Y"))

            if feats is None or targs is None:
                raise KeyError(f"Missing features/targets in shard: {self.shards[shard_idx]}")

            if isinstance(feats, torch.Tensor):
                if feats.dim() == 1:
                    feats = [feats.tolist()]
                else:
                    feats = [row.tolist() for row in feats]
            elif not isinstance(feats, list):
                feats = list(feats)

            indices, offsets, lengths = self._pack_feature_lists(feats)

            if isinstance(targs, torch.Tensor):
                targs = targs.to(torch.float32)
            else:
                targs = torch.tensor(targs, dtype=torch.float32)

            entry = ("packed", indices, offsets, lengths, targs)

        self._cache[shard_idx] = entry
        if len(self._cache) > self.cache_size:
            self._cache.popitem(last=False)

        return entry

    def __getitem__(self, idx):
        if idx < 0:
            idx += self.total_len
        if idx < 0 or idx >= self.total_len:
            raise IndexError("Index out of range")

        shard_idx = bisect_right(self.shard_starts, idx) - 1
        local_idx = idx - self.shard_starts[shard_idx]

        entry = self._load_shard(shard_idx)
        _, indices, offsets, lengths, targs = entry
        start = int(offsets[local_idx].item())
        length = int(lengths[local_idx].item())
        feats = indices.narrow(0, start, length)
        targ = targs[local_idx]
        return feats, targ
