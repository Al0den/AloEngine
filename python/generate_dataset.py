# precompute_lichess_nnue.py

from datasets import load_dataset
import torch
from pathlib import Path
from tqdm import tqdm
import numpy as np

from feature import fen_to_features  # your existing function

DEFAULT_MAX_CP = 4410.0

def eval_to_cp(row, max_cp: float) -> float:
    mate = row["mate"]
    if mate is not None:
        cp = max_cp if mate > 0 else -max_cp
        return float(max(-1.0, min(1.0, cp / max_cp)))

    cp = row["cp"]
    if cp is None:
        return 0.0

    cp = float(cp)
    cp = max(-max_cp, min(max_cp, cp))
    return float(max(-1.0, min(1.0, cp / max_cp)))


def infer_resume_state(out_dir: Path, split: str, shard_size: int):
    pattern = f"lichess_nnue_{split}_*.pt"
    shards = sorted(out_dir.glob(pattern))
    if not shards:
        return 0, 0

    full_shards = shards[:-1]
    last_shard = shards[-1]

    already_done = len(full_shards) * shard_size

    last = torch.load(last_shard, map_location="cpu")
    if "num_samples" in last:
        n_last = int(last["num_samples"])
    elif "lengths" in last:
        n_last = int(last["lengths"].shape[0])
    elif "offsets" in last:
        n_last = int(last["offsets"].shape[0])
    else:
        last_features = last["features"]
        if isinstance(last_features, torch.Tensor):
            n_last = last_features.shape[0]
        else:
            n_last = len(last_features)
    del last

    already_done += n_last
    next_shard_idx = len(shards)

    print(
        f"Resuming: found {len(shards)} shards, last has {n_last} samples. "
        f"Already done = {already_done} positions."
    )
    return already_done, next_shard_idx

def build_adapted_dataset(
    split: str = "train",
    out_path: str = "../data/lichess_nnue.pt",
    shard_size: int = 1_000_000,
    resume: bool = True,
    max_cp: float = DEFAULT_MAX_CP,
):
    DATASET_ID = "Lichess/chess-position-evaluations"
    ds = load_dataset(DATASET_ID, split=split, streaming=True)

    out_dir = Path(out_path).resolve().parent
    out_dir.mkdir(parents=True, exist_ok=True)

    if resume:
        already_done, shard_idx = infer_resume_state(out_dir, split, shard_size)
        if already_done > 0:
            ds = ds.skip(already_done)
    else:
        existing = list(out_dir.glob(f"lichess_nnue_{split}_*.pt"))
        if existing:
            raise RuntimeError(
                f"Output directory {out_dir} already contains shards for split='{split}'. "
                "Delete them or run with resume=True."
            )
        already_done, shard_idx = 0, 0

    feats_buf, depth_buf, cp_buf = [], [], []

    def _pack_features(feature_lists):
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

    def flush_shard():
        nonlocal shard_idx, feats_buf, depth_buf, cp_buf
        if not feats_buf:
            return

        indices_t, offsets_t, lengths_t = _pack_features(feats_buf)
        depth_t = torch.tensor(depth_buf, dtype=torch.int16)
        cp_t = torch.tensor(cp_buf, dtype=torch.float32)

        shard_file = out_dir / f"lichess_nnue_{split}_{shard_idx:03d}.pt"
        torch.save(
            {
                "indices": indices_t,
                "offsets": offsets_t,
                "lengths": lengths_t,
                "depth": depth_t,
                "cp": cp_t,
                "num_samples": len(feats_buf),
            },
            shard_file,
        )

        shard_idx += 1
        feats_buf, depth_buf, cp_buf = [], [], []

    # IMPORTANT: no manual skip loop anymore
    it = iter(ds)

    for i, row in enumerate(tqdm(it, desc=f"Processing {split}", initial=already_done)):
        fen = row["fen"]
        depth = row["depth"]

        x = fen_to_features(fen)

        if isinstance(x, torch.Tensor):
            x = x.to("cpu", dtype=torch.long).flatten().tolist()
        elif isinstance(x, np.ndarray):
            x = x.astype(np.int64).ravel().tolist()
        elif not isinstance(x, list):
            x = list(x)

        cp = eval_to_cp(row, max_cp=max_cp)

        feats_buf.append(x)
        depth_buf.append(depth)
        cp_buf.append(cp)

        if len(feats_buf) >= shard_size:
            flush_shard()

    flush_shard()
    print("Done.")


if __name__ == "__main__":
    build_adapted_dataset(
        split="train",
        out_path="../data/dataset/lichess_nnue.pt",
        shard_size=1_000_000,
        resume=True,
        max_cp=DEFAULT_MAX_CP,
    )
