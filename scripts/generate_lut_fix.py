import numpy as np
from PIL import Image


def apply_gamma(rgb, gamma=2.2):
    """Apply gamma correction to simulate perceptual brightness"""
    return np.power(np.clip(rgb, 0.0, 1.0), 1.0 / gamma)


def filmic_curve(x):
    return np.clip((x * 0.6 + 0.4) ** 0.9, 0.0, 1.0)


def generate_standard_lut_image(size=32, cell_size=8, out_path="lut_standard.png"):
    """Generate a perceptually corrected LUT image in strip layout"""
    width = size * size * cell_size
    height = size * cell_size
    img = np.zeros((height, width, 3), dtype=np.uint8)

    for b in range(size):
        for g in range(size):
            for r in range(size):
                # Normalized RGB
                rgb = np.array([r, g, b], dtype=np.float32) / (size - 1)

                # Apply gamma correction
                graded = apply_gamma(rgb)
                # graded = filmic_curve(rgb)

                # Compute pixel position in strip layout
                x = (r + b * size) * cell_size
                y = g * cell_size

                # Fill cell with color
                color = (graded * 255).astype(np.uint8)
                img[y : y + cell_size, x : x + cell_size] = color

    Image.fromarray(img, mode="RGB").save(out_path)
    print(f"LUT saved to: {out_path}")


# Run it
generate_standard_lut_image(size=16, cell_size=1, out_path="lut_standard.png")
