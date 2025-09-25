import cv2
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D


def load_lut_from_image(image_path, size=32):
    """Load LUT from strip-style image and return RGB array of shape (size^3, 3)"""
    img = cv2.imread(image_path)
    if img is None:
        raise ValueError("Image not found or unreadable.")

    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    h, w, _ = img.shape

    if h != size or w != size * size:
        raise ValueError(f"Expected image size {size}x{size*size}, got {h}x{w}")

    lut = []
    for b in range(size):
        for g in range(size):
            for r in range(size):
                x = r + b * size
                y = g
                color = img[y, x] / 255.0  # Normalize to [0,1]
                lut.append(color)
    return np.array(lut)


def plot_lut_3d(lut, title="LUT from Image"):
    """Plot LUT points in 3D RGB space"""
    fig = plt.figure(figsize=(10, 8))
    ax = fig.add_subplot(111, projection="3d")

    R, G, B = lut[:, 0], lut[:, 1], lut[:, 2]
    ax.scatter(R, G, B, c=lut, marker="o", s=10, edgecolors="k", linewidths=0.2)

    ax.set_xlabel("Red")
    ax.set_ylabel("Green")
    ax.set_zlabel("Blue")
    ax.set_title(title)
    ax.view_init(elev=30, azim=135)
    plt.tight_layout()
    plt.show()


# 🔧 Usage
image_path = "color_grading_LUT.jpg"  # Replace with your LUT image path
lut = load_lut_from_image(image_path, size=32)
plot_lut_3d(lut)
