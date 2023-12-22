# https://www.ics.uci.edu/~shz/courses/cs114/docs/proj3/index.html
# https://www.ics.uci.edu/~shz/courses/cs114/docs/proj2/index.html
# https://zhuanlan.zhihu.com/p/365025737
# https://zhuanlan.zhihu.com/p/554895518

import taichi as ti

ti.init(arch=ti.vulkan)

meshResolution = 25


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



if __name__ == "__main__":
    init_mesh()
    set_indices()
    window = ti.ui.Window("clothsim", (800, 800), vsync=True)
    canvas = window.get_canvas()
    scene = ti.ui.Scene()
    camera = ti.ui.Camera()
    camera.position(-5, 0.5, 5)
    camera.lookat(0.0, 0.0, 0)
    scene.set_camera(camera)
    while window.running:

        set_vertices()
        scene.point_light(pos=(1, 1, 1), color=(1, 1, 1))
        scene.mesh(vertices, indices=indices, color=(0.2392, 0.5216, 0.7765), two_sided=True, show_wireframe=True)
        canvas.scene(scene)
        window.show()
