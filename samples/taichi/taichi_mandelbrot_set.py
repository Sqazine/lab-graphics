import taichi as ti


ti.init(arch=ti.vulkan)

width = 640
height = 480
pixels = ti.Vector.field(4, float, (width, height))

@ti.kernel
def paint(m: float):
    for i, j in pixels:
        uv = ti.math.vec2(i/width, j/height)

        n = 0.0

        c = ti.math.vec2(-0.445, 0.0)+(uv-0.5)*(2.0+1.7*0.2)
        z = ti.math.vec2(0, 0)
        for _ in range(0, m):
            z = ti.math.vec2(z.x*z.x-z.y*z.y, 2.0*z.x*z.y)+c
            if ti.math.dot(z, z) > 2:
                break
            n += 1

        t = n/m
        d = ti.math.vec3(0.3, 0.3, 0.5)
        e = ti.math.vec3(-0.2, -0.3, -0.5)
        f = ti.math.vec3(2.1, 2.0, 3.0)
        g = ti.math.vec3(0.0, 0.1, 0.0)

        pixels[i, j] = ti.math.vec4(d+e*ti.math.cos(6.28318*(f*t+g)), 1.0)


window = ti.ui.Window("mandelbrot-set", (width, height))
canvas = window.get_canvas()
i = 0
while window.running:
    paint(i*0.01)
    i += 1
    canvas.set_image(pixels)
    window.show()
