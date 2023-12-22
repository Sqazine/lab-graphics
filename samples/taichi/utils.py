import taichi as ti
import taichi.math as tm
import matplotlib.image as mpimg

class DoubleBuffer:
    def __init__(self, cur, nxt):
        self.cur = cur
        self.nxt = nxt

    def swap(self):
        self.cur, self.nxt = self.nxt, self.cur


@ti.func
def sample(vf, u, v, shape):
    i, j = int(u), int(v)
    # Nearest 
    i = ti.max(0, ti.min(shape[0] - 1, i))
    j = ti.max(0, ti.min(shape[1] - 1, j))
    return vf[i, j]


@ti.func
def lerp(vl, vr, frac):
    # frac: [0.0, 1.0]
    return (1 - frac) * vl + frac * vr


@ti.func
def bilerp(vf, u,v, shape):
    # use -0.5 to decide where bilerp performs in cells
    s, t = u - 0.5, v - 0.5
    iu, iv = int(s), int(t)
    a = sample(vf, iu + 0.5, iv + 0.5, shape)
    b = sample(vf, iu + 1.5, iv + 0.5, shape)
    c = sample(vf, iu + 0.5, iv + 1.5, shape)
    d = sample(vf, iu + 1.5, iv + 1.5, shape)
    # fract
    fu, fv = s - iu, t - iv
    return lerp(lerp(a, b, fu), lerp(c, d, fu), fv)


@ti.func
def random_float_01() -> ti.f32:
   return ti.random(dtype=float)


@ti.func
def random_float(min, max) -> ti.f32:
    return min+(max-min)*random_float_01()

@ti.func
def random_vec3(min, max) -> tm.vec3:
    v = tm.vec3(random_float(min, max), random_float(
        min, max), random_float(min, max))
    return v

@ti.func
def random_in_unit_disk() -> tm.vec3:
    p = random_vec3(-1, 1)
    while (True):
        p = tm.vec3(random_float(-1, 1), random_float(-1, 1), 0)
        if (p.x*p.x+p.y*p.y) >= 1:
            continue
        break
    return p