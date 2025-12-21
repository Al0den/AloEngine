# export_nnue_to_cpp.py
import argparse
import re
import torch
from pathlib import Path


def sanitize_name(name: str) -> str:
    """
    Make a valid C++ identifier from a PyTorch parameter name.
    Example: 'layers.0.weight' -> 'layers_0_weight'
    """
    name = name.replace('.', '_').replace(':', '_')
    # Remove anything not alnum or underscore
    name = re.sub(r'[^0-9a-zA-Z_]', '_', name)
    # Avoid leading digit
    if name and name[0].isdigit():
        name = "_" + name
    return name


def tensor_to_cpp(name: str, tensor: torch.Tensor) -> str:
    """
    Convert a tensor to C++ constexpr array + dimension constants.
    """
    t = tensor.detach().cpu().to(torch.float32)
    shape = list(t.shape)
    flat = t.reshape(-1)

    cpp_name = sanitize_name(name)

    # Dimension declarations
    lines = []
    if len(shape) == 0:
        # Scalar
        lines.append(f"inline constexpr float {cpp_name} = {flat.item():.8f}f;")
        return "\n".join(lines)

    # Emit dimension constants
    if len(shape) == 1:
        lines.append(
            f"inline constexpr std::size_t {cpp_name}_size = {shape[0]};"
        )
    else:
        for i, dim in enumerate(shape):
            lines.append(
                f"inline constexpr std::size_t {cpp_name}_dim{i} = {dim};"
            )

    # Emit array declaration
    total_size = flat.numel()
    lines.append(
        f"inline constexpr float {cpp_name}[{total_size}] = {{"
    )

    # Values, wrapped nicely
    per_line = 8
    current_line = "    "
    for i, v in enumerate(flat):
        current_line += f"{float(v):.8f}f"
        if i != total_size - 1:
            current_line += ", "
        if (i + 1) % per_line == 0 and i != total_size - 1:
            lines.append(current_line)
            current_line = "    "

    if current_line.strip():
        lines.append(current_line)

    lines.append("};")
    return "\n".join(lines)


def export_state_dict_to_cpp_header(
    state_dict_path: Path,
    output_path: Path,
    namespace: str = "nnue_weights",
):
    # Load .pth (state_dict)
    state = torch.load(state_dict_path, map_location="cpu")

    if isinstance(state, dict) and "state_dict" in state:
        state = state["state_dict"]

    if not isinstance(state, dict):
        raise ValueError(
            f"Expected a state_dict (dict), got {type(state)}. "
            "Make sure you saved with torch.save(model.state_dict(), path)."
        )

    lines = []
    lines.append("#pragma once")
    lines.append("#include <cstddef>")
    lines.append("")
    lines.append(f"namespace {namespace} {{")
    lines.append("")

    # Keep deterministic order
    for name, tensor in state.items():
        if not isinstance(tensor, torch.Tensor):
            continue
        lines.append(f"// Tensor: {name}, shape {tuple(tensor.shape)}")
        lines.append(tensor_to_cpp(name, tensor))
        lines.append("")

    lines.append(f"}} // namespace {namespace}")
    lines.append("")

    output_path.write_text("\n".join(lines))
    print(f"Wrote C++ header to: {output_path}")


def main():
    parser = argparse.ArgumentParser(
        description="Export NNUE .pth (state_dict) to a C++ header with constexpr arrays."
    )
    parser.add_argument(
        "--model-path",
        type=Path,
        help="Path to the .pth file (state_dict).",
        default=Path("../data/nnue_model.pth"),
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("../data/nnue_weights.hpp"),
        help="Output C++ header file.",
    )
    parser.add_argument(
        "--namespace",
        type=str,
        default="nnue_weights",
        help="C++ namespace to wrap everything in.",
    )

    args = parser.parse_args()
    export_state_dict_to_cpp_header(
        state_dict_path=args.model_path,
        output_path=args.output,
        namespace=args.namespace,
    )


if __name__ == "__main__":
    main()
