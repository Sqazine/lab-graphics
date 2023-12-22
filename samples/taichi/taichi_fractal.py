import taichi as ti

ti.init(arch=ti.vulkan)

n = 320
pixels = ti.field(dtype=float, shape=(n*2, n))


@ti.func
def complex_sqr(z):
    return ti.math.vec2(z[0]*z[0]-z[1]*z[1], 2*z[0]*z[1])


@ti.kernel
def paint(t: float):
    for i, j in pixels:
        c = ti.math.vec2(-0.8, ti.math.cos(t)*0.2)
        z = ti.math.vec2(i/n-1, j/n-0.5)*2
        iterations = 0
        while z.norm() < 20 and iterations < 50:
            z = complex_sqr(z)+c
            iterations += 1
        pixels[i, j] = 1-iterations*0.02


window = ti.ui.Window("julia set", (n*2, n), vsync=True)
canvas=window.get_canvas()
i=0
while window.running:
    paint(i*0.01)
    i+=1
    canvas.set_image(pixels)
    window.show()
