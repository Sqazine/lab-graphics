#[Stable Fluids, Jos Stam](http://www.dgp.toronto.edu/people/stam/reality/Research/pdf/ns.pdf)
#[Real-Time Fluid Dynamics for Games, Jos Stam](https://pdfs.semanticscholar.org/847f/819a4ea14bd789aca8bc88e85e906cfc657c.pdf)
# [Fast Fluid Dynamics Simulation on the GPU, Mark J. Harris](http://developer.download.nvidia.com/books/HTML/gpugems/gpugems_ch38.html)

# StableFluids - A GPU implementation of Jos Stam's Stable Fluids on Unity
# https://github.com/keijiro/StableFluids


import taichi as ti
import taichi.math as tm
import matplotlib.image as mpimg
import sys
import os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from utils import *

ti.init(arch=ti.vulkan)

initial = "assets/girl.jpg"
extent = [896, 896]

image = mpimg.imread(initial)
extent[1]=image.shape[0]
extent[0]=image.shape[1]

dt = 1/60
dx=1.0/extent[1]

viscosity=1e-6
force=300
exponent=200.0

velocity_buffer_1=ti.Vector.field(2, float, shape=extent)
velocity_buffer_2=ti.Vector.field(2, float, shape=extent)
velocity_buffer_3=ti.Vector.field(2, float, shape=extent)

pressure_buffer_1=ti.Vector.field(1, float, shape=extent)
pressure_buffer_2=ti.Vector.field(1, float, shape=extent)

color_field_double_buffer=DoubleBuffer(ti.Vector.field(3, float, shape=extent),ti.Vector.field(3, float, shape=extent))

last_cursor_pos:tm.vec2=tm.vec2(0,0)
cur_cursor_pos:tm.vec2=tm.vec2(0,0)
is_mouse_left_button_release=True

@ti.kernel
def copy_texture(src:ti.template(),dst:ti.template()):
    for i,j in dst:
        dst[i,j]= bilerp(src,i+0.5,j+0.5,(extent))

@ti.kernel
def init_color_field(image: ti.types.ndarray()):
    for i, j in ti.ndrange(extent[0], extent[1]):
        u=extent[1]-j-1
        v=i
        color_field_double_buffer.cur[i, j] = (
            image[u,v, 0]/255.0,
            image[u,v, 1]/255.0,
            image[u,v, 2]/255.0,
        )

@ti.kernel
def advect_step(vf_0:ti.template(),result:ti.template()):
    for i,j in vf_0:
        uv=tm.vec2(i+0.5,j+0.5)
        vel=bilerp(vf_0,uv.x,uv.y,(extent))

        duv=vel*dt*tm.vec2(float(extent[1])/extent[0],1)
        duv.x*=extent[0]
        duv.y*=extent[1]
        
        newUV=uv-duv

        result[i,j]=bilerp(vf_0,newUV.x,newUV.y,(extent))


@ti.kernel
def force_step(vf_0:ti.template(),result:ti.template(),forceExponent:ti.f32,forceOrigin:tm.vec2,forceVector:tm.vec2):
    for i,j in vf_0:
        uv = tm.vec2(i+0.5,j+0.5)
        vel=bilerp(vf_0,uv.x,uv.y,(extent))

        pos=ti.Vector([uv.x/extent[0],uv.y/extent[1]])
        amp=tm.exp(-forceExponent*tm.distance(forceOrigin,pos))
        result[i,j]=vel+forceVector*amp


# Setup for Project step (divW calculation)
@ti.kernel
def project_setup(vf_0:ti.template(),divWResult:ti.template(),pressureResult:ti.template()):
    for i,j in vf_0:
        uv=tm.vec2(i+0.5,j+0.5)
        uvL=tm.vec2(uv.x-1,uv.y)
        uvR=tm.vec2(uv.x+1,uv.y)
        uvT=tm.vec2(uv.x,uv.y+1)
        uvB=tm.vec2(uv.x,uv.y-1)

        L=bilerp(vf_0,uvL.x,uvL.y,(extent))
        R=bilerp(vf_0,uvR.x,uvR.y,(extent))
        T=bilerp(vf_0,uvT.x,uvT.y,(extent))
        B=bilerp(vf_0,uvB.x,uvB.y,(extent))

        divWResult[i,j]=(R.x-L.x+T.y-B.y)*extent[1]/2.0

        pressureResult[i,j]=0

# Finishing for Project step (divergence free field calculation)
@ti.kernel
def project_finish(pf_0:ti.template(),vf_1:ti.template(),result:ti.template()):
    for i,j in pf_0:
        if (i==0 or j==0) or (i==extent[0]-1 or j==extent[1]-1):
            continue
        
        uv=tm.vec2(i+0.5,j+0.5)
        uvL=tm.vec2(uv.x-1,uv.y)
        uvR=tm.vec2(uv.x+1,uv.y)
        uvT=tm.vec2(uv.x,uv.y+1)
        uvB=tm.vec2(uv.x,uv.y-1)

        uvL.x=max(uvL.x,1)
        uvL.y=max(uvL.y,1)
        
        uvR.x=min(uvR.x,extent[0]-2)
        uvR.y=min(uvR.y,extent[1]-2)

        uvT.x=min(uvT.x,extent[0]-2)
        uvT.y=min(uvT.y,extent[1]-2)

        uvB.x=max(uvB.x,1)
        uvB.y=max(uvB.y,1)

        L=bilerp(pf_0,uvL.x,uvL.y,(extent))
        R=bilerp(pf_0,uvR.x,uvR.y,(extent))
        T=bilerp(pf_0,uvT.x,uvT.y,(extent))
        B=bilerp(pf_0,uvB.x,uvB.y,(extent))

        u=bilerp(vf_1,uv.x,uv.y,(extent))
        u-=tm.vec2(R-L,T-B)*extent[1]/2.0

        result[i,j]=u

        if uvL.x==0:
            result[0,j]=-u
        if uvB.y==0:
            result[i,0]=-u
        if uvR.x==extent[0]-1:
            result[extent[0]-1,j]=-u
        if uvT.y==extent[1]-1:
            result[i,extent[1]-1]=-u

@ti.kernel
def jacobi1(x1_in:ti.template(),b1_in:ti.template(),alpha:ti.f32,beta:ti.f32,x1_out:ti.template()):
    for i,j in x1_in:
        uv=tm.vec2(i+0.5,j+0.5)
        uvL=tm.vec2(uv.x-1,uv.y)
        uvR=tm.vec2(uv.x+1,uv.y)
        uvT=tm.vec2(uv.x,uv.y+1)
        uvB=tm.vec2(uv.x,uv.y-1)

        L=bilerp(x1_in,uvL.x,uvL.y,(extent))
        R=bilerp(x1_in,uvR.x,uvR.y,(extent))
        T=bilerp(x1_in,uvT.x,uvT.y,(extent))
        B=bilerp(x1_in,uvB.x,uvB.y,(extent))

        bC=bilerp(b1_in,uv.x,uv.y,(extent))

        x1_out[i,j]=(L+R+T+B+alpha*bC.x)/beta
        
@ti.kernel
def jacobi2(x2_in:ti.template(),b2_in:ti.template(),alpha:ti.f32,beta:ti.f32,x2_out:ti.template()):
    for i,j in x2_out:
        uv=tm.vec2(i+0.5,j+0.5)
        uvL=tm.vec2(uv.x-1,uv.y)
        uvR=tm.vec2(uv.x+1,uv.y)
        uvT=tm.vec2(uv.x,uv.y+1)
        uvB=tm.vec2(uv.x,uv.y-1)

        L=bilerp(x2_in,uvL.x,uvL.y,(extent))
        R=bilerp(x2_in,uvR.x,uvR.y,(extent))
        T=bilerp(x2_in,uvT.x,uvT.y,(extent))
        B=bilerp(x2_in,uvB.x,uvB.y,(extent))

        bC=bilerp(b2_in,uv.x,uv.y,(extent))

        x2_out[i,j]=(L+R+T+B+alpha*bC)/beta

@ti.kernel
def frag_advect_step(col_0:ti.template(),vf_0:ti.template(),result:ti.template()):
    aspect=extent[0]/float(extent[1])
    aspect_inv=1.0/aspect
    for i,j in col_0:
        uv=tm.vec2(i+0.5,j+0.5)

        vel=bilerp(vf_0,uv.x,uv.y,(extent))

        delta=vel*aspect_inv*dt
        newUV=uv-delta*tm.vec2(extent[0],extent[1])

        color=bilerp(col_0,newUV.x,newUV.y,(extent))

        result[i,j]=color

init_color_field(image)

window = ti.ui.Window("taichi_2d_euler_stable_fluid", res=(extent[0], extent[1]),vsync=False)
canvas = window.get_canvas()

while window.running:

    # advection
    advect_step(velocity_buffer_1,velocity_buffer_2)

    copy_texture(velocity_buffer_2,velocity_buffer_1)

    #diffusion
    diffusion_alpha=dx*dx/(viscosity*dt)
    alpha=diffusion_alpha
    beta=4+diffusion_alpha
    #Jacobi iteration
    for i in range(0,20):
        jacobi2(velocity_buffer_2,velocity_buffer_1,alpha,beta,velocity_buffer_3)
        jacobi2(velocity_buffer_3,velocity_buffer_1,alpha,beta,velocity_buffer_2)

    if(window.get_event(ti.ui.RELEASE)):
        if window.event.key in [ti.ui.LMB]:
            is_mouse_left_button_release=True

    if(window.is_pressed(ti.ui.LMB)):
        cur_cursor_pos_x,cur_cursor_pos_y=window.get_cursor_pos()
        cur_cursor_pos=tm.vec2(cur_cursor_pos_x,cur_cursor_pos_y)
        if(is_mouse_left_button_release):
            last_cursor_pos=cur_cursor_pos
            is_mouse_left_button_release=False

    #force step
    forceVector=(cur_cursor_pos-last_cursor_pos)*force
    force_step(velocity_buffer_2,velocity_buffer_3,exponent,cur_cursor_pos,forceVector)

    last_cursor_pos=cur_cursor_pos

    #projection setup
    project_setup(velocity_buffer_3,velocity_buffer_2,pressure_buffer_1)

    #jacobi iteration
    alpha=-dx*dx
    beta=4
    for i in range(0,20):
        jacobi1(pressure_buffer_1,velocity_buffer_2,alpha,beta,pressure_buffer_2)
        jacobi1(pressure_buffer_2,velocity_buffer_2,alpha,beta,pressure_buffer_1)

    # #projection finish
    project_finish(pressure_buffer_1,velocity_buffer_3,velocity_buffer_1)

    # Apply the velocity field to the color buffer.
    frag_advect_step(color_field_double_buffer.cur,velocity_buffer_1,color_field_double_buffer.nxt)

    canvas.set_image(color_field_double_buffer.nxt)
    color_field_double_buffer.swap()

    window.show()