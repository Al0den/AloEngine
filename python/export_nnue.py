# export_nnue_to_bin.py
import argparse
from pathlib import Path
import struct

import torch


# Order matters: C++ loader will assume *exactly* this order.
TENSOR_KEYS = [
    "embed.weight",
    "bias1",
    "fc2.weight",
    "fc2.bias",
    "out.weight",
    "out.bias",
]


def export_to_bin(state_dict_path: Path, output_path: Path):
    """
    Export NNUE state_dict to a compact binary file.

    Layout (little-endian):
      - 4 bytes: magic "NNUE"
      - int32:   version (currently 1)
      - int32:   num_tensors (len(TENSOR_KEYS))

      For each tensor in TENSOR_KEYS:
        - int32: ndim
        - ndim * int32: shape dimensions
        - float32[prod(shape)]: raw data (row-major, same as PyTorch)
    """
    state = torch.load(state_dict_path, map_location="cpu")

    # If saved as {"state_dict": ...}
    if isinstance(state, dict) and "state_dict" in state:
        state = state["state_dict"]

    if not isinstance(state, dict):
        raise ValueError(
            f"Expected a state_dict (dict), got {type(state)}. "
            "Make sure you saved with torch.save(model.state_dict(), path)."
        )

    # Check that all expected keys are present
    missing = [k for k in TENSOR_KEYS if k not in state]
    if missing:
        raise KeyError(f"Missing expected keys in state_dict: {missing}")

    with open(output_path, "wb") as f:
        # Magic + version + num_tensors
        f.write(b"NNUE")
        f.write(struct.pack("<i", 1))  # version = 1
        f.write(struct.pack("<i", len(TENSOR_KEYS)))

        for key in TENSOR_KEYS:
            t = state[key]
            if not isinstance(t, torch.Tensor):
                raise TypeError(f"State entry '{key}' is not a tensor")

            t = t.detach().cpu().to(torch.float32).contiguous()
            shape = list(t.shape)
            ndim = len(shape)

            # Write ndim
            f.write(struct.pack("<i", ndim))
            # Write shape dims
            for d in shape:
                f.write(struct.pack("<i", d))
            # Write raw data
            f.write(t.numpy().tobytes(order="C"))

    print(f"Wrote NNUE binary weights to: {output_path}")


def main():
    parser = argparse.ArgumentParser(
        description="Export NNUE .pth (state_dict) to a binary weights file."
    )
    parser.add_argument(
        "--model-path",
        type=Path,
        default=Path("../data/nnue_model.pth"),
        help="Path to the .pth file (state_dict).",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("../data/nnue_weights.bin"),
        help="Output binary file.",
    )

    args = parser.parse_args()
    export_to_bin(
        state_dict_path=args.model_path,
        output_path=args.output,
    )


if __name__ == "__main__":
    main()
