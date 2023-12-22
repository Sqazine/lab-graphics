import taichi as ti
from taichi.math import *

ti.init(arch=ti.vulkan)

PARTICLE_NUM = 20000
PARTICLE_RADIUS = 0.005
PARTICLE_RESTING_DENSITY = 1000
PARTICLE_MASS = 0.02  # m=pv
SMOOTHING_LENGTH = (4*PARTICLE_RADIUS)
PARTICLE_STIFFNESS = 1000
PARTICLE_VISCOSITY = 500
GRAVITY_FORCE = vec2(0, -9806.65)
TIME_STEP = 0.0001
WALL_DAMPING = 0.3
PI = 3.1415927

pixels = ti.Vector.field(3, float, (1024, 768))

positions = ti.Vector.field(2, float, shape=PARTICLE_NUM)
velocitys = ti.Vector.field(2, float, shape=PARTICLE_NUM)
forces = ti.Vector.field(2, float, shape=PARTICLE_NUM)
densitys = ti.Vector.field(1, float, shape=PARTICLE_NUM)
pressures = ti.Vector.field(1, float, shape=PARTICLE_NUM)


@ti.kernel
def calc_density_pressure():
    for i in range(0, PARTICLE_NUM):
        densitySum = 0.0
        for j in range(0, PARTICLE_NUM):
            delta = positions[i]-positions[j]
            r = delta.norm()
            if r < SMOOTHING_LENGTH:
                densitySum += PARTICLE_MASS*315.0 * \
                    pow(SMOOTHING_LENGTH*SMOOTHING_LENGTH-r*r, 3) / \
                    (64.0*PI*pow(SMOOTHING_LENGTH, 9))
        densitys[i] = densitySum
        pressures[i] = max(PARTICLE_STIFFNESS *
                           (densitySum-PARTICLE_RESTING_DENSITY), 0.0)


@ti.kernel
def calc_force():
    for i in range(0, PARTICLE_NUM):
        pressureForce = vec2(0.0, 0.0)
        viscosityForce = vec2(0.0, 0.0)
        for j in range(0, PARTICLE_NUM):
            if i == j:
                continue
            delta = positions[i]-positions[j]
            r = length(delta)
            if r < SMOOTHING_LENGTH:
                pf = PARTICLE_MASS*(pressures[i]+pressures[j])/(2.0*densitys[j][0])*-45.0/(
                    PI*pow(SMOOTHING_LENGTH, 6))*pow(SMOOTHING_LENGTH-r, 2)

                pressureForce -= pf[0]*normalize(delta)

                vf = PARTICLE_MASS*(velocitys[j]-velocitys[i])/densitys[j][0]*45.0/(
                    PI*pow(SMOOTHING_LENGTH, 6))*(SMOOTHING_LENGTH-r)

                viscosityForce += vf[0]

        viscosityForce *= PARTICLE_VISCOSITY
        externalForce = densitys[i][0]*GRAVITY_FORCE

        forces[i] = pressureForce+viscosityForce+externalForce


@ti.kernel
def calc_integrate():
    for i in range(0, PARTICLE_NUM):
        acceleration = forces[i]/vec2(densitys[i], densitys[i])
        newVelocity = velocitys[i]+TIME_STEP*acceleration
        newPosition = positions[i]+TIME_STEP*newVelocity

        if newPosition.x < -1:
            newPosition.x = -1
            newVelocity.x *= -1*WALL_DAMPING
        elif newPosition.x > 1:
            newPosition.x = 1
            newVelocity.x *= -1*WALL_DAMPING
        elif newPosition.y > 1:
            newPosition.y = 1
            newVelocity.y *= -1*WALL_DAMPING
        elif newPosition.y < -1:
            newPosition.y = -1
            newVelocity.y *= -1*WALL_DAMPING

        velocitys[i] = newVelocity
        positions[i] = newPosition


@ti.kernel
def init_particle():
    for x in range(0, 125):
        i = x*160
        for y in range(0, 160):
            positions[i].x = -0.625+PARTICLE_RADIUS*2*x
            positions[i].y = -0.625+PARTICLE_RADIUS*2*y
            i += 1


@ti.kernel
def clear_screen():
    for i in range(0, 1024):
        for j in range(0, 768):
            pixels[i, j] = vec3(0.0, 0.0, 0.0)


@ti.kernel
def draw():
    for i in range(0, PARTICLE_NUM):
        p = positions[i]
        p = vec2(1024, 768)*(p+1.0)/2.0
        p_x = ti.cast(p.x, ti.i32)
        p_y = ti.cast(p.y, ti.i32)
        if i/(PARTICLE_NUM/4) < 1:
            pixels[p_x, p_y] = vec3(1.0, 1.0, 1.0)
        elif i/(PARTICLE_NUM/4) < 2:
            pixels[p_x, p_y] = vec3(1.0, 0.0, 0.0)
        elif i/(PARTICLE_NUM/4) < 3:
            pixels[p_x, p_y] = vec3(0.0, 1.0, 0.0)
        elif i/(PARTICLE_NUM/4) < 4:
            pixels[p_x, p_y] = vec3(0.0, 0.0, 1.0)


window = ti.ui.Window("sph", (1024, 768))
canvas = window.get_canvas()


isPause = False
init_particle()
while window.running:

    if window.is_pressed(ti.GUI.SPACE):
        if isPause == True:
            isPause = False
        elif isPause == False:
            isPause = True

    if not isPause:
        clear_screen()
        calc_density_pressure()
        calc_force()
        calc_integrate()
        draw()
    canvas.set_image(pixels)
    window.show()
