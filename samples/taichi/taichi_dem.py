# https://github.com/houkensjtu/taichi_dem_tutorial

import taichi as ti
from taichi.math import *
import math
import os

ti.init(arch=ti.vulkan)

window_size = 600
n = 8192

density = 100.0
stiffness = 8e3
restitution_coef = 0.001
gravity = -9.81
dt = 0.0001
substeps = 60

grid_n = 128
grid_size = 1.0/grid_n

print(f"Grid size: {grid_n}x{grid_n}")

grain_r_min = 0.002
grain_r_max = 0.003

assert grain_r_max * 2 < grid_size

@ti.dataclass
class Grain:
    p: vec2  # Position
    m: ti.f32  # Mass
    r: ti.f32  # Radius
    v: vec2  # Velocity
    a: vec2  # Acceleration
    f: vec2  # Force

grain_field = Grain.field(shape=(n, ))


@ti.kernel
def init():
    for i in grain_field:
        l=i*grid_size
        padding=0.1
        region_width=1.0-padding*2
        pos=vec2(l%region_width+padding+grid_size*ti.random()*0.2,
                 l//region_width*grid_size+0.3)
        grain_field[i].p=pos
        grain_field[i].r=ti.random()*(grain_r_max-grain_r_min)+grain_r_min
        grain_field[i].m=density*math.pi*grain_field[i].r**2

@ti.kernel
def update():
    for i in grain_field:
        a=grain_field[i].f/grain_field[i].m
        grain_field[i].v+=(grain_field[i].a+a)*dt/2.0
        grain_field[i].p+=grain_field[i].v*dt+0.5*a*dt**2
        grain_field[i].a=a
        
@ti.kernel
def apply_boundary_collision():
    bounce_coef=0.3
    for i in grain_field:
        x=grain_field[i].p[0]
        y=grain_field[i].p[1]

        if y-grain_field[i].r<0:
            grain_field[i].p[1]=grain_field[i].r
            grain_field[i].v[1]*=-bounce_coef

        elif y+grain_field[i].r>1.0:
            grain_field[i].p[1]=1.0-grain_field[i].r
            grain_field[i].v[1]*=-bounce_coef

        if x-grain_field[i].r<0:
            grain_field[i].p[0]=grain_field[i].r
            grain_field[i].v[0]*=-bounce_coef

        elif x+grain_field[i].r>1.0:
            grain_field[i].p[0]=1.0-grain_field[i].r
            grain_field[i].v[0]*=-bounce_coef

@ti.func
def resolve(i,j):

    rel_pos=grain_field[j].p-grain_field[i].p
    dist=rel_pos.norm()#dist=ti.sqrt(rel_pos[0]**2+rel_pos[1]**2)
    delta=-dist+grain_field[i].r+grain_field[j].r#delta=d-2*r

    if delta>0:
        normal=rel_pos/dist
        f1=normal*delta*stiffness
        #damping force
        M=(grain_field[i].m*grain_field[j].m)/(grain_field[i].m+grain_field[j].m)
        K=stiffness
        C=2.0*(1.0/ti.sqrt(1.0+(math.pi/ti.log(restitution_coef))**2))*ti.sqrt(K*M)
        V=(grain_field[j].v-grain_field[i].v)*normal
        f2=C*V*normal
        grain_field[i].f+=f2-f1
        grain_field[j].f-=f2-f1

list_head=ti.field(dtype=ti.i32,shape=grid_n*grid_n)
list_cur=ti.field(dtype=ti.i32,shape=grid_n*grid_n)
list_tail=ti.field(dtype=ti.i32,shape=grid_n*grid_n)

grain_count=ti.field(dtype=ti.i32,shape=(grid_n,grid_n),name="grain_count")
column_sum=ti.field(dtype=ti.i32,shape=grid_n,name="column_sum")
prefix_sum=ti.field(dtype=ti.i32,shape=(grid_n,grid_n),name="prefix_sum")
particle_id=ti.field(dtype=ti.i32,shape=n,name="particle_id")

@ti.kernel
def contact(gf:ti.template()):
    '''
    Handle the collision between grains.
    '''
    for i in gf:
        gf[i].f=vec2(0.0,gravity*gf[i].m)#apply gravity

    grain_count.fill(0)
    for i in range(n):
        grid_idx=ti.floor(grain_field[i].p*grid_n,int)
        grain_count[grid_idx]+=1

    for i in range(grid_n):
        sum=0
        for j in range(grid_n):
            sum+=grain_count[i,j]
        column_sum[i]=sum

    prefix_sum[0,0]=0

    ti.loop_config(serialize=True)

    for i in range(1,grid_n):
        prefix_sum[i,0]=prefix_sum[i-1,0]+column_sum[i-1]

    for i in range(grid_n):
        for j in range(grid_n):
            if j==0:
                prefix_sum[i,j]+=grain_count[i,j]
            else:
                prefix_sum[i,j]=prefix_sum[i,j-1]+grain_count[i,j]

    for i in range(grid_n):
        for j in range(grid_n):
            linear_idx=i*grid_n+j
            list_head[linear_idx]=prefix_sum[i,j]-grain_count[i,j]
            list_cur[linear_idx]=list_head[linear_idx]
            list_tail[linear_idx]=prefix_sum[i,j]

    for i in range(n):
        grid_idx=ti.floor(grain_field[i].p*grid_n,int)
        linear_idx=grid_idx[0]*grid_n+grid_idx[1]
        grain_location=ti.atomic_add(list_cur[linear_idx],1)
        particle_id[grain_location]=i

     # Brute-force collision detection
    
    # for i in range(n):
    #     for j in range(i + 1, n):
    #         resolve(i, j)

    # fast collision detection
    for i in range(n):
        grid_idx=ti.floor(grain_field[i].p*grid_n,int)
        x_begin=max(grid_idx[0]-1,0)
        x_end=min(grid_idx[0]+2,grid_n)

        y_begin=max(grid_idx[1]-1,0)
        y_end=min(grid_idx[1]+2,grid_n)

        for neighbour_i in range(x_begin,x_end):
            for neighbour_j in range(y_begin,y_end):
                neighbour_linear_idx=neighbour_i*grid_n+neighbour_j
                for p_idx in range(list_head[neighbour_linear_idx],list_tail[neighbour_linear_idx]):
                    j=particle_id[p_idx]
                    if i<j:
                        resolve(i,j)
     


if __name__=="__main__":
    gui=ti.GUI("DEM",(window_size,window_size))

    init()
    while(gui.running):
        for s in range(substeps):
            update()
            apply_boundary_collision()
            contact(grain_field)
        pos = grain_field.p.to_numpy()
        r = grain_field.r.to_numpy() * window_size
        gui.circles(pos,radius=r)
        gui.show()