#https://www.zhihu.com/question/536224169
import taichi as ti

ti.init(arch=ti.vulkan)

n = 128
quad_size = 1.0/n
gravity=ti.Vector([0,-9.8,0])
spring_Y=3e4
dashpot_damping=1e4
drag_damping=1

dt = 4e-2/n
substeps = int(1/60//dt)
x = ti.Vector.field(3, dtype=float, shape=(n, n))  # 质点的位置
v = ti.Vector.field(3, dtype=float, shape=(n, n))  # 质点的速度

ball_radius = 0.3
ball_center = ti.Vector.field(3, dtype=float, shape=(1,))
ball_center[0] = [0, 0, 0]

num_triangles = (n - 1) * (n - 1) * 2
indices = ti.field(int, shape=num_triangles * 3)
vertices = ti.Vector.field(3, dtype=float, shape=n * n)
colors = ti.Vector.field(3, dtype=float, shape=n * n)

@ti.kernel
def initialize_mass_points():
    random_offset = ti.Vector([ti.random()-0.5, ti.random()-0.5])*0.1
    for i, j in x:
        x[i, j] = [i*quad_size-0.5+random_offset[0],
                   0.6,
                   j*quad_size-0.5+random_offset[1]]
        v[i, j] = [0, 0, 0]

spring_offsets=[]
for i in range(-1,2):
    for j in range(-1,2):
        if (i,j)!=(0,0):
            spring_offsets.append(ti.Vector([i,j]))

@ti.kernel
def substep():
    for i in ti.grouped(v):
        v[i]+=gravity*dt

    for i in ti.grouped(x):
        force=ti.Vector([0.0,0.0,0.0])
        for spring_offset in ti.static(spring_offsets):
            j=i+spring_offset
            if 0<=j[0]<n and 0<=j[1]<n:
                x_ij=x[i]-x[j]
                v_ij=v[i]-v[j]
                d=x_ij.normalized()
                current_dist=x_ij.norm()
                original_dist=quad_size*float(i-j).norm()
                #spring force
                force+=-spring_Y*d*(current_dist/original_dist-1)
                #dashpot damping
                force+=-v_ij.dot(d)*d*dashpot_damping*quad_size
        v[i]+=force*dt

    for i in ti.grouped(x):
        v[i]*=ti.exp(-drag_damping*dt)
        offset_to_center=x[i]-ball_center[0]
        if offset_to_center.norm()<=ball_radius:
            normal=offset_to_center.normalized()
            v[i]-=min(v[i].dot(normal),0)*normal
        x[i]+=dt*v[i]

@ti.kernel
def update_vertices():
    for i,j in ti.ndrange(n,n):
        vertices[i*n+j]=x[i,j]

@ti.kernel
def initialize_mesh_indices():
    for i, j in ti.ndrange(n - 1, n - 1):
        quad_id = (i * (n - 1)) + j
        # 1st triangle of the square
        indices[quad_id * 6 + 0] = i * n + j
        indices[quad_id * 6 + 1] = (i + 1) * n + j
        indices[quad_id * 6 + 2] = i * n + (j + 1)
        # 2nd triangle of the square
        indices[quad_id * 6 + 3] = (i + 1) * n + j + 1
        indices[quad_id * 6 + 4] = i * n + (j + 1)
        indices[quad_id * 6 + 5] = (i + 1) * n + j
    
    for i, j in ti.ndrange(n, n):
        if (i // 4 + j // 4) % 2 == 0:
            colors[i * n + j] = (0., 0.5, 1)
        else:
            colors[i * n + j] = (1, 0.5, 0.)

window=ti.ui.Window("clothSim2",(800,800),vsync=True)
canvas=window.get_canvas()
canvas.set_background_color((0,0,0))
scene=ti.ui.Scene()
camera=ti.ui.make_camera()


initialize_mesh_indices()
current_t=0.0
initialize_mass_points()

while window.running:
    if(current_t>1.5):
        initialize_mass_points()
        current_t=0

    for i in range(substeps):
        substep()
        current_t+=dt
    update_vertices()
    camera.position(0.0,0.0,3)
    camera.lookat(0.0,0.0,0)
    scene.set_camera(camera)
    scene.point_light(pos=(0,1,2),color=(1,1,1))
    scene.mesh(vertices,indices=indices,per_vertex_color=colors,two_sided=True)

    scene.particles(ball_center,radius=ball_radius*0.95,color=(0.7,0.0,0.0))
    canvas.scene(scene)
    window.show()