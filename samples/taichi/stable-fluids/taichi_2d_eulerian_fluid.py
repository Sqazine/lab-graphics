# reference:http://yangwc.com/2019/05/01/fluidSimulation/

import taichi as ti
import taichi.math as tm
import matplotlib.image as mpimg
import sys
import os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from utils import *

ti.init(arch=ti.vulkan)

img_path = "assets/test.jpg"
extent = [1200, 850]
dt = 1/60
iteration_step = 20
mouse_radius = 0.001
mouse_speed = 125
mouse_respond_distance = 0.5

curl_param = 15  # 旋度

# 速度场
velocity_field_0 = ti.Vector.field(2, float, shape=extent)
velocity_field_1 = ti.Vector.field(2, float, shape=extent)
velocity_double_buffer = DoubleBuffer(velocity_field_0, velocity_field_1)

# 压力场
pressure_field_0 = ti.field(float, shape=extent)
pressure_field_1 = ti.field(float, shape=extent)
pressure_double_buffer = DoubleBuffer(pressure_field_0, pressure_field_1)

# 散度场
divergence_field = ti.field(float, shape=extent)

# 旋度场
curl_field = ti.field(float, shape=extent)

color_field_0 = ti.Vector.field(3, float, shape=extent)
color_field_1 = ti.Vector.field(3, float, shape=extent)
color_double_buffer = DoubleBuffer(color_field_0, color_field_1)


@ti.kernel
def init_color_field(image: ti.types.ndarray()):
    for i, j in ti.ndrange(extent[0], extent[1]):
        color_field_0[i, j] = (
            image[extent[1]-j-1, i, 0]/255.0,
            image[extent[1]-j-1, i, 1]/255.0,
            image[extent[1]-j-1, i, 2]/255.0,
        )


@ti.kernel
def advection(vf: ti.template(), qf_0: ti.template(), qf_1: ti.template()):
    for i, j in vf:
        coord_cur = ti.Vector([i, j]) + ti.Vector([0.5, 0.5])
        velocity_cur = vf[i, j]
        coord_prev = coord_cur-velocity_cur*dt
        q_prev = bilerp(qf_0, coord_prev[0], coord_prev[1], (extent))
        qf_1[i, j] = q_prev


@ti.kernel
def curl(vf: ti.template(), cf: ti.template()):
    for i, j in vf:
        cf[i, j] = 0.5 * ((vf[i+1, j][1] - vf[i-1, j][1]) -
                          (vf[i, j+1][0] - vf[i, j-1][0]))


@ti.kernel
def vorticity_projection(cf: ti.template(), vf_0: ti.template(), vf_1: ti.template()):
    for i, j in cf:
        grad_curl = ti.Vector([
            0.5*(cf[i+1, j]-cf[i-1, j]),
            0.5 * (cf[i, j + 1] - cf[i, j - 1]),
            0
        ])

        grad_curl_length = tm.length(grad_curl)
        if grad_curl_length > 1e-5:
            force = curl_param * \
                tm.cross(grad_curl/grad_curl_length, ti.Vector([0, 0, 1]))
            vf_1[i, j] = vf_0[i, j]+dt*force[:2]


@ti.kernel
def divergence(vf: ti.template(), divf: ti.template()):
    for i, j in vf:
        divf[i, j] = 0.5 * (vf[i + 1, j][0] - vf[i - 1, j]
                            [0] + vf[i, j + 1][1] - vf[i, j - 1][1])


@ti.kernel
def pressure_iteration(divf: ti.template(), pf_0: ti.template(), pf_1: ti.template()):
    for i, j in pf_0:
        pf_1[i, j] = (pf_0[i+1, j]+pf_0[i-1, j] +
                      pf_0[i, j-1]+pf_0[i, j+1]-divf[i, j])/4


def pressure_solve(press_df: DoubleBuffer, divf: ti.template()):
    for i in range(iteration_step):
        pressure_iteration(divf, press_df.cur, press_df.nxt)
        press_df.swap()
        apply_pressure_bc(press_df.cur)


@ti.kernel
def pressure_projection(pf: ti.template(), vf_0: ti.template(), vf_1: ti.template()):
    for i, j in vf_0:
        vf_1[i, j] = vf_0[i, j]-ti.Vector([
            pressure_with_boundary(pf, i+1, j, extent) -
            pressure_with_boundary(pf, i-1, j, extent),
            pressure_with_boundary(pf, i, j+1, extent) -
            pressure_with_boundary(pf, i, j-1, extent)
        ])


@ti.func
def velocity_with_boundary(vf: ti.template(), i: int, j: int, shape) -> ti.f32:
    if (i <= 0) or (i >= shape[0]-1) or (j >= shape[1]-1) or (j <= 0):
        vf[i, j] = ti.Vector([0.0, 0.0])
    return vf[i, j]


@ti.func
def pressure_with_boundary(pf: ti.template(), i: int, j: int, shape) -> ti.f32:
    if (i == j == 0) or (i == shape[0]-1 and j == shape[1]-1) or (i == 0 and j == shape[1]-1) or (i == shape[0]-1 and j == 0):
        pf[i, j] = 0.0
    elif i == 0:
        pf[0, j] = pf[1, j]
    elif j == 0:
        pf[i, 0] = pf[i, 1]
    elif i == shape[0]-1:
        pf[shape[0]-1, j] = pf[shape[0]-2, j]
    elif j == shape[1]-1:
        pf[i, shape[1]-1] = pf[i, shape[1]-2]
    return pf[i, j]


@ti.func
def curl_with_boundary(cf: ti.template(), i: int, j: int, shape) -> ti.f32:
    if (i <= 0) or (i >= shape[0]-1) or (j >= shape[1]-1) or (j <= 0):
        cf[i, j] = 0.0
    return cf[i, j]


@ti.kernel
def apply_velocity_bc(vf: ti.template()):
    for i, j in vf:
        velocity_with_boundary(vf, i, j, extent)


@ti.kernel
def apply_pressure_bc(pf: ti.template()):
    for i, j in pf:
        pressure_with_boundary(pf, i, j, extent)


@ti.kernel
def apply_curl_bc(cf: ti.template()):
    for i, j in cf:
        curl_with_boundary(cf, i, j, extent)


def mouse_interaction(prev_posx: int, prev_posy: int):
    if(window.is_pressed(ti.ui.LMB)):
        mouse_x, mouse_y = window.get_cursor_pos()
        mousePos_x = int(mouse_x*extent[0])
        mousePos_y = int(mouse_y*extent[1])
        if prev_posx == 0 and prev_posy == 0:
            prev_posx = mousePos_x
            prev_posy = mousePos_y
        mouseRadius = mouse_radius*min(extent[0], extent[1])

        mouse_add_speed(mousePos_x,
                        mousePos_y,
                        prev_posx,
                        prev_posy,
                        mouseRadius,
                        velocity_double_buffer.cur,
                        velocity_double_buffer.nxt)
    
        velocity_double_buffer.swap()
        prev_posx=mousePos_x
        prev_posy=mousePos_y
    return prev_posx,prev_posy


@ti.kernel
def mouse_add_speed(cur_posx: int, cur_posy: int, prev_posx: int, prev_posy: int, mouseRadius: float, vf_0: ti.template(), vf_1: ti.template()):
    for i, j in vf_0:
        vec1 = ti.Vector([cur_posx-prev_posx, cur_posy-prev_posy])
        vec2 = ti.Vector([i-prev_posx, j-prev_posy])
        dotans = tm.dot(vec1, vec2)
        distance = abs(tm.cross(vec1, vec2))/(tm.length(vec1)+0.001)
        if dotans >= 0 and dotans <= mouse_respond_distance*tm.length(vec1) and distance <= mouseRadius:
            vf_1[i, j] = vf_0[i, j]+vec1*mouse_speed
        else:
            vf_1[i, j] = vf_0[i, j]


def advaction_step():
    advection(velocity_double_buffer.cur,
              color_double_buffer.cur, color_double_buffer.nxt)
    advection(velocity_double_buffer.cur,
              velocity_double_buffer.cur, velocity_double_buffer.nxt)
    color_double_buffer.swap()
    velocity_double_buffer.swap()
    apply_velocity_bc(velocity_double_buffer.cur)


def voricity_step():
    curl(velocity_double_buffer.cur, curl_field)
    apply_curl_bc(curl_field)
    vorticity_projection(
        curl_field, velocity_double_buffer.cur, velocity_double_buffer.nxt)
    velocity_double_buffer.swap()
    apply_velocity_bc(velocity_double_buffer.cur)


def pressure_step():
    divergence(velocity_double_buffer.cur, divergence_field)
    pressure_solve(pressure_double_buffer,divergence_field)
    pressure_projection(pressure_double_buffer.cur,
                   velocity_double_buffer.cur, velocity_double_buffer.nxt)
    velocity_double_buffer.swap()
    apply_velocity_bc(velocity_double_buffer.cur)


image = mpimg.imread(img_path)
extent[1]=image.shape[0]
extent[0]=image.shape[1]
print(image.shape)
init_color_field(image)

velocity_field_0.fill(0)
pressure_field_0.fill(0)
apply_velocity_bc(velocity_double_buffer.cur)

window = ti.ui.Window("taichi_2d_eulerian_fluid", res=(extent[0], extent[1]))
canvas = window.get_canvas()

mouse_prevposx, mouse_prevposy = 0, 0

while window.running:

    advaction_step()
    voricity_step()

    mouse_prevposx, mouse_prevposy = mouse_interaction(
        mouse_prevposx, mouse_prevposy)

    pressure_step()

    canvas.set_image(color_double_buffer.cur)
    window.show()
