# train_nnue.py
import torch
from torch.utils.data import DataLoader
from tqdm import tqdm

from network import NNUEHalfKP as NNUEModel
from dataset import NNUEDataset
from utils import nnue_collate


def train_nnue(
    dataset_path,
    epochs=5,
    batch_size=1024,
    lr=1e-3,
    device=None,
    save_path="../data/nnue_model.pth",
):
    if device is None:
        device = torch.device(
            "cuda" if torch.cuda.is_available()
            else "cpu"
        )

    dataset = NNUEDataset(dataset_path)
    loader = DataLoader(
        dataset,
        batch_size=batch_size,
        num_workers=1,
        collate_fn=nnue_collate,   # ← key change
        pin_memory=True,
    )

    model = NNUEModel().to(device)
    optimizer = torch.optim.Adam(model.parameters(), lr=lr)
    loss_fn = torch.nn.MSELoss()

    for epoch in range(1, epochs + 1):
        model.train()
        running_loss = 0.0
        n_batches = 0

        pbar = tqdm(loader, desc=f"Epoch {epoch}", unit="batch")

        for indices, offsets, y in pbar:
            indices = indices.to(device, non_blocking=True)
            offsets = offsets.to(device, non_blocking=True)
            y = y.to(device, non_blocking=True)

            optimizer.zero_grad()
            y_pred = model(indices, offsets)  # ← pass both
            loss = loss_fn(y_pred, y)
            loss.backward()
            optimizer.step()

            running_loss += loss.item()
            n_batches += 1
            pbar.set_postfix(loss=running_loss / n_batches)

        avg_loss = running_loss / max(1, n_batches)
        print(f"Epoch {epoch}: loss={avg_loss:.5f}")
        torch.save(model.state_dict(), save_path)

    return model


if __name__ == "__main__":
    device = torch.device(
        "cuda" if torch.cuda.is_available()
        else "cpu"
    )

    model = train_nnue(
        dataset_path="../data/dataset/",
        epochs=1000,
        batch_size=2048,
        lr=1e-3,
        device=device,
    )
