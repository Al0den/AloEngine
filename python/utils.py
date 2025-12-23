# utils_collate.py (or inside your train script)

import torch

def _to_flat_list(feats):
    if isinstance(feats, torch.Tensor):
        return feats.flatten().tolist()

    if isinstance(feats, (list, tuple)):
        if feats and (
            isinstance(feats[0], (list, tuple, torch.Tensor))
            or (hasattr(feats[0], "shape") and len(getattr(feats[0], "shape", ())) > 0)
        ):
            flat = []
            for part in feats:
                if isinstance(part, torch.Tensor):
                    flat.extend(part.flatten().tolist())
                elif isinstance(part, (list, tuple)):
                    flat.extend(part)
                elif hasattr(part, "flatten") and hasattr(part, "tolist"):
                    flat.extend(part.flatten().tolist())
                else:
                    try:
                        flat.extend(part)
                    except TypeError:
                        flat.append(part)
            return flat
        return list(feats)

    if hasattr(feats, "flatten") and hasattr(feats, "tolist"):
        return feats.flatten().tolist()

    try:
        return list(feats)
    except TypeError:
        return [feats]

def nnue_collate(batch):
    """
    batch: list of (feature_indices: List[int], target: Tensor)
    returns:
        indices: LongTensor [total_features_in_batch]
        offsets: LongTensor [batch_size]
        targets: FloatTensor [batch_size]
    """
    feature_lists, targets = zip(*batch)  # tuples of lists / tensors

    offsets = []
    flat_tensors = []
    offset = 0

    for feats in feature_lists:
        offsets.append(offset)
        if isinstance(feats, torch.Tensor):
            feats_t = feats.to(torch.long).flatten()
        else:
            feats_t = torch.tensor(_to_flat_list(feats), dtype=torch.long)
        flat_tensors.append(feats_t)
        offset += int(feats_t.numel())

    indices = torch.cat(flat_tensors, dim=0) if flat_tensors else torch.empty(0, dtype=torch.long)
    offsets = torch.tensor(offsets, dtype=torch.long)
    targets = torch.stack(
        [t if isinstance(t, torch.Tensor) else torch.tensor(float(t), dtype=torch.float32) for t in targets],
        dim=0,
    )  # [B]

    return indices, offsets, targets
