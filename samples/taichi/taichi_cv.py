#https://zhuanlan.zhihu.com/p/573894977?utm_campaign=&utm_medium=social&utm_oi=710638601452396544&utm_psn=1565676769222508544&utm_source=com.tencent.wework&from_wecom=1
# https://github.com/taichi-dev/image-processing-with-taichi

import cv2
import numpy as np
import taichi as ti

ti.init(arch=ti.gpu)

#图像转置
src = cv2.imread("cat.jpg")
h, w, c = src.shape
dst = np.zeros((w, h, c), dtype=src.dtype)

img2d = ti.types.ndarray(element_dim=1)


@ti.kernel
def transpose(src: img2d, dst: img2d):
    for i, j in ti.ndrange(h, w):
        dst[j, i] = src[i, j]


transpose(src, dst)
cv2.imwrite("cat_transpose.jpg", dst)


#图像最近邻滤波(NEAREST)
scale = 5
src = cv2.imread("cat_96x64.jpg")
w, h, c = src.shape
dst = np.zeros((w*scale, h*scale, c), dtype=src.dtype)

img2d = ti.types.ndarray(element_dim=1)


@ti.kernel
def nearest(src: img2d, dst: img2d):
    for i, j in ti.ndrange(w*5, h*5):
        dst[i, j] = src[i//scale, j//scale]


nearest(src, dst)
cv2.imwrite("cat_nearest.jpg", dst)

#图像双线性插值(BILINEAR)


@ti.kernel
def bilinear_interp(src: img2d, dst: img2d):
    for i in ti.grouped(dst):
        x, y = i/scale
        x1, y1 = int(x), int(y)
        x2, y2 = min(x1+1, w-1), min(y1+1, h-1)
        Q11 = src[x1, y1]
        Q21 = src[x2, y1]
        Q12 = src[x1, y2]
        Q22 = src[x2, y2]
        R1 = ti.math.mix(Q11, Q21, x-x1)
        R2 = ti.math.mix(Q12, Q22, x-x1)
        dst[i] = ti.round(ti.math.mix(R1, R2, y-y1), ti.u8)


bilinear_interp(src, dst)
cv2.imwrite("cat_bilinear.jpg", dst)


#高斯滤波
weights = ti.field(dtype=float, shape=2014, offset=-512)


@ti.func
def compute_weights(radius: int, sigma: float):
    total = 0.0
    ti.loop_config(serialize=True)
    for i in range(-radius, radius+1):
        val = ti.exp(-0.5*(i/sigma)**2)
        weights[i] = val
        total += val

    ti.loop_config(serialize=True)
    for i in range(-radius, radius+1):
        weights[i] /= total


@ti.kernel
def gaussian_blur(img: img2d, sigma: float):
    img_blurred.fill(0)
    blur_radius = ti.ceil(sigma*3, int)
    compute_weights(blur_radius, sigma)

    n,m=img.shape[0],img.shape[1]
    for i,j in ti.ndrange(n,m):
        l_begin,l_end=max()

img = cv2.imread('./images/mountain.jpg')
img_blurred = ti.Vector.field(3, dtype=ti.u8, shape=(1024, 1024))
