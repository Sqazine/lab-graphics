# https://www.ics.uci.edu/~shz/courses/cs114/docs/proj3/index.html
# https://www.ics.uci.edu/~shz/courses/cs114/docs/proj2/index.html
# https://zhuanlan.zhihu.com/p/365025737

import taichi as ti
import math

ti.init(arch=ti.vulkan)

meshResolution = 25

mass=1

force_gravity = ti.math.vec3(0.0,-9.8,0.0)*mass

damping = 3
dt = 1e-3


stiffness = 5000
shear=5000
flexion=5000
n_fluid=ti.math.vec3(0.0,0.0,1.0)
viscous=0.5

force_wind=ti.math.vec3(0.0,0.0,10.0)

rest_length_structural=4.0/(meshResolution-1)
rest_length_shear=ti.math.sqrt(2.0)*4.0/(meshResolution-1)
rest_length_flexion=2.0*rest_length_structural


position_field = ti.Vector.field(3, float, (meshResolution, meshResolution))
normal_field= ti.Vector.field(3, float, (meshResolution, meshResolution))
velocity_field=ti.Vector.field(3,float,(meshResolution, meshResolution))

indices = ti.field(int, (meshResolution - 1) * (meshResolution - 1) * 6)
vertices = ti.Vector.field(3, float, meshResolution*meshResolution)

d_field=ti.Vector.field(2,int,8)
@ti.kernel
def init_mesh():
    for i, j in ti.ndrange(meshResolution, meshResolution):
        position_field[i, j] = ti.Vector([-2.0 + 4.0 * j / (meshResolution - 1), -2.0 + 4.0 * i / (meshResolution - 1), 0.0])
        velocity_field[i,j]=ti.Vector([0,0,0])

    d_field[0]=ti.Vector([1,0])
    d_field[1]=ti.Vector([1,1])
    d_field[2]=ti.Vector([0,1])
    d_field[3]=ti.Vector([-1,1])
    d_field[4]=ti.Vector([-1,0])
    d_field[5]=ti.Vector([-1,-1])
    d_field[6]=ti.Vector([0,-1])
    d_field[7]=ti.Vector([1,-1])

    calc_normal()

    

@ti.kernel
def set_vertices():
    for i, j in ti.ndrange(meshResolution, meshResolution):
        vertices[i*meshResolution+j] = position_field[i, j]

@ti.kernel
def set_indices():
    for i, j in ti.ndrange(meshResolution, meshResolution):
        if i < meshResolution-1 and j < meshResolution-1:
            square_id = (i*(meshResolution-1))+j
            #正方形的第一个小三角形
            indices[square_id * 6 + 0] = i * meshResolution + j
            indices[square_id * 6 + 1] = (i + 1) * meshResolution + j
            indices[square_id * 6 + 2] = i * meshResolution + (j + 1)
            #正方形的第二个小三角形
            indices[square_id * 6 + 3] = (i + 1) * meshResolution + j + 1
            indices[square_id * 6 + 4] = i * meshResolution + (j + 1)
            indices[square_id * 6 + 5] = (i + 1) * meshResolution + j

@ti.func
def calc_normal():
    for i, j in ti.ndrange(meshResolution, meshResolution):
        p0=position_field[i,j]
        n=ti.math.vec3(0.0,0.0,0.0)

        for t in range(0,8):
            i1=i+d_field[t].x
            j1=j+d_field[t].y
            i2=i+d_field[(t+1)%8].x
            j2=j+d_field[(t+1)%8].y

            if i1>=0 and i1<meshResolution and j1>=0 and j1<meshResolution and i2>=0 and i2<meshResolution and j2>=0 and j2<meshResolution:
                e1=position_field[i1,j1]-p0
                e2=position_field[i2,j2]-p0
                n+=ti.math.normalize(ti.math.cross(e1,e2))
        
        normal_field[i,j]=ti.math.normalize(n)


@ti.func
def get_structural_force(i:ti.int32,j:ti.int32)->ti.math.vec3:
    force_structural=ti.math.vec3(0.0,0.0,0.0)
    if j+1<meshResolution and j+1>=0:
        v=position_field[i,j]-position_field[i,j+1]
        v_len=ti.math.length(v)
        v_norm=ti.math.normalize(v)
        force_structural+=v_norm*stiffness*(rest_length_structural-v_len)
    
    if j-1<meshResolution and j-1>=0:
        v=position_field[i,j]-position_field[i,j-1]
        v_len=ti.math.length(v)
        v_norm=ti.math.normalize(v)
        force_structural+=v_norm*stiffness*(rest_length_structural-v_len)

    if i-1<meshResolution and i-1>=0:
        v=position_field[i,j]-position_field[i-1,j]
        v_len=ti.math.length(v)
        v_norm=ti.math.normalize(v)
        force_structural+=v_norm*stiffness*(rest_length_structural-v_len)
    
    if i+1<meshResolution and i+1>=0:
        v=position_field[i,j]-position_field[i+1,j]
        v_len=ti.math.length(v)
        v_norm=ti.math.normalize(v)
        force_structural+=v_norm*stiffness*(rest_length_structural-v_len)
    
    return force_structural

@ti.func
def get_shear_force(i:ti.int32,j:ti.int32)->ti.math.vec3:
    force_shear=ti.math.vec3(0.0,0.0,0.0)
    if j+1<meshResolution and j+1>=0 and i+1<meshResolution and i+1 >=0:
        v=position_field[i,j]-position_field[i+1,j+1]
        v_len=ti.math.length(v)
        v_norm=ti.math.normalize(v)
        force_shear+=v_norm*shear*(rest_length_shear-v_len)
    
    if j-1<meshResolution and j-1>=0 and i+1<meshResolution and i+1 >=0:
        v=position_field[i,j]-position_field[i+1,j-1]
        v_len=ti.math.length(v)
        v_norm=ti.math.normalize(v)
        force_shear+=v_norm*shear*(rest_length_shear-v_len)

    if i-1<meshResolution and i-1>=0 and j-1<meshResolution and j-1 >=0:
        v=position_field[i,j]-position_field[i-1,j-1]
        v_len=ti.math.length(v)
        v_norm=ti.math.normalize(v)
        force_shear+=v_norm*shear*(rest_length_shear-v_len)
    
    if i-1<meshResolution and i-1>=0 and j+1<meshResolution and j+1 >=0:
        v=position_field[i,j]-position_field[i-1,j+1]
        v_len=ti.math.length(v)
        v_norm=ti.math.normalize(v)
        force_shear+=v_norm*shear*(rest_length_shear-v_len)
    
    return force_shear

@ti.func
def get_flexion_force(i:ti.int32,j:ti.int32)->ti.math.vec3:
    force_flexion=ti.math.vec3(0.0,0.0,0.0)
    if j+2<meshResolution and j+2>=0:
        v=position_field[i,j]-position_field[i,j+2]
        v_len=ti.math.length(v)
        v_norm=ti.math.normalize(v)
        force_flexion+=v_norm*flexion*(rest_length_flexion-v_len)
    
    if j-2<meshResolution and j-2>=0:
        v=position_field[i,j]-position_field[i,j-2]
        v_len=ti.math.length(v)
        v_norm=ti.math.normalize(v)
        force_flexion+=v_norm*flexion*(rest_length_flexion-v_len)

    if i + 2 < meshResolution and i + 2 >= 0:
        v=position_field[i,j]-position_field[i+2,j]
        v_len=ti.math.length(v)
        v_norm=ti.math.normalize(v)
        force_flexion+=v_norm*flexion*(rest_length_flexion-v_len)
    
    if i - 2 < meshResolution and i - 2 >= 0:
        v=position_field[i,j]-position_field[i-2,j]
        v_len=ti.math.length(v)
        v_norm=ti.math.normalize(v)
        force_flexion+=v_norm*flexion*(rest_length_flexion-v_len)
    
    return force_flexion

@ti.func
def get_force_damping(v:ti.math.vec3)->ti.math.vec3:
    return -damping*v

@ti.func
def get_force_viscous(i:ti.int32,j:ti.int32)->ti.math.vec3:
    n=normal_field[i,j]
    v=velocity_field[i,j]
    return viscous*(ti.math.dot(n,n_fluid-v))*n

@ti.kernel
def simulate(dt:ti.float32,t:ti.float32):
    for i,j in ti.ndrange(meshResolution,meshResolution):
        force_structural=get_structural_force(i,j)
        force_shear=get_shear_force(i,j)
        force_flexion=get_flexion_force(i,j)
        force_damping=get_force_damping(velocity_field[i,j])
        force_viscous=get_force_viscous(i,j)

        force_wind_dir=force_wind*(ti.sin(t)*2.0-1.0)

        force_all=force_gravity+force_structural+force_shear+force_flexion+force_damping+force_viscous+force_wind_dir

        accel= force_all/mass

        velocity_field[i,j]+=accel*dt

        if not( i == (meshResolution - 1) and (j == 0 or j == meshResolution - 1)):
            position_field[i,j] += dt*velocity_field[i,j]
            
        calc_normal()

t=0.0
if __name__ == "__main__":
    init_mesh()
    set_indices()
    window = ti.ui.Window("clothsim", (800, 800), vsync=False)
    canvas = window.get_canvas()
    scene = ti.ui.Scene()
    camera = ti.ui.Camera()
    camera.position(-5, 0.5, 5)
    camera.lookat(0.0, 0.0, 0)
    scene.set_camera(camera)
    while window.running:

        for i in range(0,10):
            t += dt*20
            simulate(dt,math.radians(t))

        set_vertices()
        scene.point_light(pos=(1, 1, 1), color=(1, 1, 1))
        scene.mesh(vertices, indices=indices, color=(0.2392, 0.5216, 0.7765), two_sided=True, show_wireframe=True)
        canvas.scene(scene)
        window.show()
