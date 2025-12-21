# utils_collate.py (or inside your train script)

import torch

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
    flat_indices = []
    offset = 0

    for feats in feature_lists:
        offsets.append(offset)
        flat_indices.extend(feats)
        offset += len(feats)

    indices = torch.tensor(flat_indices, dtype=torch.long)
    offsets = torch.tensor(offsets, dtype=torch.long)
    targets = torch.stack(targets, dim=0)  # [B]

    return indices, offsets, targets
