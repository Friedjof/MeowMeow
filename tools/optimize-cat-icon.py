#!/usr/bin/env python3
import argparse
import base64
from io import BytesIO
from pathlib import Path
import sys

from PIL import Image


def parse_args():
    parser = argparse.ArgumentParser(
        description="Compress a cat icon and inline it as a data URI in HTML."
    )
    parser.add_argument(
        "--source",
        default="web/cat-icon.png",
        help="Path to the source cat icon PNG.",
    )
    parser.add_argument(
        "--html",
        nargs="+",
        default=["web/index.html"],
        help="HTML file(s) to update.",
    )
    parser.add_argument(
        "--max-dim",
        type=int,
        default=512,
        help="Max width/height for the icon.",
    )
    parser.add_argument(
        "--colors",
        type=int,
        default=256,
        help="Palette size for PNG quantization.",
    )
    parser.add_argument(
        "--no-quantize",
        action="store_true",
        help="Disable palette quantization.",
    )
    parser.add_argument(
        "--out",
        default="",
        help="Optional output path for the optimized PNG.",
    )
    parser.add_argument(
        "--allow-missing",
        action="store_true",
        help="Skip HTML files that do not exist.",
    )
    return parser.parse_args()


def load_and_resize(image_path, max_dim):
    image = Image.open(image_path).convert("RGBA")
    width, height = image.size
    if max(width, height) <= max_dim:
        return image

    scale = max_dim / float(max(width, height))
    new_size = (
        max(1, int(round(width * scale))),
        max(1, int(round(height * scale))),
    )
    resample = Image.Resampling.LANCZOS if hasattr(Image, "Resampling") else Image.LANCZOS
    return image.resize(new_size, resample)


def quantize_image(image, colors):
    method = Image.LIBIMAGEQUANT if hasattr(Image, "LIBIMAGEQUANT") else Image.FASTOCTREE
    return image.quantize(colors=colors, method=method, dither=Image.FLOYDSTEINBERG)


def encode_png_bytes(image, colors, quantize):
    target = image
    if quantize and colors > 0:
        try:
            target = quantize_image(image, colors)
        except Exception:
            target = image

    buffer = BytesIO()
    target.save(buffer, format="PNG", optimize=True)
    return buffer.getvalue()


def build_data_uri(png_bytes):
    payload = base64.b64encode(png_bytes).decode("ascii")
    return f"data:image/png;base64,{payload}"


def replace_cat_img_src(html_text, data_uri):
    marker = 'class="cat-img"'
    marker_index = html_text.find(marker)
    if marker_index < 0:
        raise ValueError("Could not find cat image marker.")

    img_start = html_text.rfind("<img", 0, marker_index)
    if img_start < 0:
        raise ValueError("Could not find <img> tag for cat image.")

    img_end = html_text.find(">", img_start)
    if img_end < 0:
        raise ValueError("Could not find end of <img> tag for cat image.")

    tag = html_text[img_start : img_end + 1]
    for quote in ('"', "'"):
        src_key = f"src={quote}"
        src_pos = tag.find(src_key)
        if src_pos >= 0:
            value_start = img_start + src_pos + len(src_key)
            value_end = html_text.find(quote, value_start)
            if value_end < 0:
                raise ValueError("Could not find end of src attribute for cat image.")
            return html_text[:value_start] + data_uri + html_text[value_end:]

    raise ValueError("Could not find src attribute for cat image.")


def main():
    args = parse_args()
    source_path = Path(args.source)
    if not source_path.exists():
        print(f"Missing source image: {source_path}", file=sys.stderr)
        return 1

    image = load_and_resize(source_path, args.max_dim)
    png_bytes = encode_png_bytes(image, args.colors, not args.no_quantize)
    data_uri = build_data_uri(png_bytes)

    if args.out:
        Path(args.out).write_bytes(png_bytes)

    for html_path in args.html:
        path = Path(html_path)
        if not path.exists():
            if args.allow_missing:
                print(f"Skipping missing HTML: {path}")
                continue
            print(f"Missing HTML file: {path}", file=sys.stderr)
            return 1

        html_text = path.read_text(encoding="utf-8")
        try:
            updated = replace_cat_img_src(html_text, data_uri)
        except ValueError as exc:
            print(f"{path}: {exc}", file=sys.stderr)
            return 1

        if updated != html_text:
            path.write_text(updated, encoding="utf-8")

    print(
        f"Optimized {source_path} -> {len(png_bytes)} bytes PNG, "
        f"{len(data_uri)} bytes data URI."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
