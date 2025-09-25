import numpy as np
from PIL import Image


def grade_color(rgb, lift=np.array([0.0, 0.0, 0.0]), gamma=np.array([1.0, 1.0, 1.0]), gain=np.array([1.0, 1.0, 1.0])):
    # rgb: [R,G,B] in [0,1]
    x = np.clip(rgb + lift, 0.0, 1.0)
    x = np.clip(x * gain, 0.0, 1.0)
    x = np.power(np.clip(x, 1e-6, 1.0), 1.0 / gamma)  # 防止 0^负数
    return np.clip(x, 0.0, 1.0)


def make_graded_strip_lut(N=32, cell_size=8, out_path="lut_strip_graded.png", lift=(0.02, -0.01, 0.0), gamma=(1.1, 1.0, 0.95), gain=(1.05, 1.0, 1.02)):
    width = N * N * cell_size
    height = N * cell_size
    img = np.zeros((height, width, 3), dtype=np.uint8)

    lift = np.array(lift, dtype=np.float32)
    gamma = np.array(gamma, dtype=np.float32)
    gain = np.array(gain, dtype=np.float32)

    for b in range(N):
        slice_x0 = b * N * cell_size
        B = b / (N - 1)
        for g in range(N):
            G = g / (N - 1)
            for r in range(N):
                R = r / (N - 1)
                src = np.array([R, G, B], dtype=np.float32)
                dst = grade_color(src, lift, gamma, gain)

                x0 = slice_x0 + r * cell_size
                y0 = g * cell_size
                img[y0 : y0 + cell_size, x0 : x0 + cell_size, :] = (np.clip(dst, 0, 1) * 255).astype(np.uint8)

    Image.fromarray(img, mode="RGB").save(out_path)
    print(f"Saved: {out_path}")


if __name__ == "__main__":
    make_graded_strip_lut(N=16, cell_size=1)
