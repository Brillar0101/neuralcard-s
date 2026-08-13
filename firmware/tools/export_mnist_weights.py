#!/usr/bin/env python3
"""Fetch the pretrained MNIST CNN and emit it as a C header.

No training happens here or anywhere else in this project. The weights come
from the ONNX Model Zoo's mnist-8, a 5,998 parameter CNN published by the ONNX
project under the MIT licence. This script downloads that file, pulls the six
weight tensors out of it, and writes main/mnist_weights.h.

Architecture (verified by reading the graph, not assumed):

    input   1 x 28 x 28, float, 0..1
    conv1   8 filters, 5x5, pad 2   -> relu -> maxpool 2x2 stride 2
    conv2   16 filters, 5x5, pad 2  -> relu -> maxpool 3x3 stride 3
    fc      256 -> 10               (logits, softmax applied by the caller)

Usage:  python3 tools/export_mnist_weights.py
Needs:  pip install onnx numpy
"""
import hashlib
import pathlib
import sys
import urllib.request

URL = ("https://github.com/onnx/models/raw/main/"
       "validated/vision/classification/mnist/model/mnist-8.onnx")
CACHE = pathlib.Path(__file__).parent / "mnist-8.onnx"
OUT = pathlib.Path(__file__).parent.parent / "main" / "mnist_weights.h"

# initializer name -> (C name, expected shape)
TENSORS = [
    ("Parameter5",   "conv1_w", (8, 1, 5, 5)),
    ("Parameter6",   "conv1_b", (8, 1, 1)),
    ("Parameter87",  "conv2_w", (16, 8, 5, 5)),
    ("Parameter88",  "conv2_b", (16, 1, 1)),
    ("Parameter193", "fc_w",    (16, 4, 4, 10)),
    ("Parameter194", "fc_b",    (1, 10)),
]


def fetch():
    if not CACHE.exists():
        print("downloading", URL)
        urllib.request.urlretrieve(URL, CACHE)
    data = CACHE.read_bytes()
    print("mnist-8.onnx  %d bytes  sha256 %s" % (len(data), hashlib.sha256(data).hexdigest()[:16]))
    return CACHE


def main():
    try:
        import numpy as np
        import onnx
        from onnx import numpy_helper
    except ImportError:
        sys.exit("needs onnx and numpy: pip install onnx numpy")

    model = onnx.load(str(fetch()))
    got = {i.name: numpy_helper.to_array(i) for i in model.graph.initializer}

    lines = [
        "// Pretrained MNIST CNN weights. GENERATED FILE, do not edit by hand.",
        "// Source: ONNX Model Zoo mnist-8 (MIT licence), exported by",
        "// tools/export_mnist_weights.py. Nothing in this project is trained.",
        "//",
        "// conv1 8x5x5 -> relu -> pool2 | conv2 16x5x5 -> relu -> pool3 | fc 256->10",
        "#pragma once",
        "",
    ]
    total = 0
    for name, cname, shape in TENSORS:
        arr = got[name]
        if tuple(arr.shape) != shape:
            sys.exit("%s: expected shape %s, got %s" % (name, shape, arr.shape))
        flat = arr.astype(np.float32).reshape(-1)
        total += flat.size
        lines.append("// %s %s" % (name, list(shape)))
        lines.append("static const float %s[%d] = {" % (cname, flat.size))
        for i in range(0, flat.size, 8):
            chunk = ", ".join("%.8gf" % v for v in flat[i:i + 8])
            lines.append("    " + chunk + ",")
        lines.append("};")
        lines.append("")
    lines.append("#define MNIST_PARAM_COUNT %d" % total)
    lines.append("")

    OUT.write_text("\n".join(lines))
    print("wrote %s  (%d params, %.1f KB of float32)" % (OUT, total, total * 4 / 1024))


if __name__ == "__main__":
    main()
