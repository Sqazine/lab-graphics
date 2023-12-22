import math
import time
import taichi as ti

ti.init(arch=ti.vulkan)

N = 64
cell_size = 1.0/N
gravity = 0.5
stiffness = 100
damping = 1.0
dt = 5e-4

intersection_offset = 0.001

x = ti.Vector.field(3, float, (N, N))
v = ti.Vector.field(3, float, (N, N))

num_triangles = (N-1)*(N-1)*2
indices = ti.field(int, num_triangles*3)
vertices = ti.Vector.field(3, float, N*N)
colors = ti.Vector.field(3, dtype=float, shape=N * N)


def init_scene():
    for i, j in ti.ndrange(N, N):
        x[i, j] = ti.Vector(
            [i*cell_size, j*cell_size/ti.sqrt(2), (N-j)*cell_size/ti.sqrt(2)])


@ti.kernel
def set_vertices():
    for i, j in ti.ndrange(N, N):
        vertices[i*N+j] = x[i, j]

@ti.kernel
def set_indices():
    for i, j in ti.ndrange(N, N):
        if i < N-1 and j < N-1:
            square_id = (i*(N-1))+j
            #正方形的第一个小三角形
            indices[square_id * 6 + 0] = i * N + j
            indices[square_id * 6 + 1] = (i + 1) * N + j
            indices[square_id * 6 + 2] = i * N + (j + 1)
            #正方形的第二个小三角形
            indices[square_id * 6 + 3] = (i + 1) * N + j + 1
            indices[square_id * 6 + 4] = i * N + (j + 1)
            indices[square_id * 6 + 5] = (i + 1) * N + j

    for i, j in ti.ndrange(N, N):
        if (i // 4 + j // 4) % 2 == 0:
            colors[i * N + j] = (0., 0.5, 1)
        else:
            colors[i * N + j] = (1, 0.5, 0.)


links = [[-1, 0], [1, 0], [0, -1], [0, 1], [-1, -1], [1, -1], [-1, 1], [1, 1]]
links = [ti.Vector(v) for v in links]


@ti.kernel
def step(t: float):
    for i in ti.grouped(x):
        v[i].y -= gravity*dt
    for i in ti.grouped(x):
        force = ti.Vector([0.0, 5.0, 0.0])*ti.sin(t)
        for d in ti.static(links):
            j = min(max(i+d, 0), [N-1, N-1])
            relative_pos = x[j]-x[i]
            current_length = relative_pos.norm()
            original_length = cell_size*float(i-j).norm()
            if original_length != 0:
                force += stiffness*relative_pos.normalized()*(current_length-original_length) / \
                    original_length
        v[i] += force*dt

    for i in ti.grouped(x):
        if all(i%(N-1)==0):
            continue

        v[i] *= ti.exp(-damping*dt)
        x[i] += dt*v[i]


t = 0.0
if __name__ == "__main__":
    init_scene()
    set_indices()
    window = ti.ui.Window("clothsim", (800, 800), vsync=True)
    canvas = window.get_canvas()
    scene = ti.ui.Scene()
    camera = ti.ui.Camera()
    camera.position(0.5, 1.0, 3)
    camera.lookat(0.5, 0.0, 0)
    scene.set_camera(camera)
    while window.running:

        if window.get_event(ti.ui.PRESS) and window.is_pressed(ti.ui.SPACE):
            init_scene()

        for i in range(30):
            t += dt*20
            step(math.radians(t))

        set_vertices()
        scene.point_light(pos=(1, 1, 1), color=(1, 1, 1))
        scene.mesh(vertices, indices=indices, color=(
            0.5, 0.5, 0.5), per_vertex_color=colors, two_sided=True, show_wireframe=True)
        canvas.scene(scene)
        window.show()
