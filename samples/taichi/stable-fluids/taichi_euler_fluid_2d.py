import taichi as ti 
import taichi.math as tm
import matplotlib.image as mpimg
import numpy as np
import sys
import os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from utils import *

ti.init(ti.gpu)

image=mpimg.imread("assets/girl.jpg")
extent = [image.shape[0], image.shape[1]]

diffusion_times=8
force_scale=200.0
radius=0.2
advect_speed=1.0
viscosity=0.5

dt = 1/60.0

dx=1.0
halfRdx=1/dx*0.5
dx2=dx*dx

color_field_double_buffer=DoubleBuffer(ti.Vector.field(3, float, shape=extent),ti.Vector.field(3, float, shape=extent))
velocity_field_double_buffer=DoubleBuffer(ti.Vector.field(2, float, shape=extent),ti.Vector.field(2, float, shape=extent))
divergence_field=ti.Vector.field(1, float, shape=extent)
pressure_field_double_buffer=DoubleBuffer(ti.Vector.field(1, float, shape=extent),ti.Vector.field(1, float, shape=extent))

last_cursor_pos:tm.vec2=tm.vec2(0,0)
cur_cursor_pos:tm.vec2=tm.vec2(0,0)

@ti.kernel
def release_pressure_double_buffer():
     for i,j in pressure_field_double_buffer.cur:
         pressure_field_double_buffer.cur[i,j].fill(0)
     for i,j in pressure_field_double_buffer.nxt:
         pressure_field_double_buffer.nxt[i,j].fill(0)

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

#对流平流
#流体会携带流体中的物体, 以及自身流动传送
#采样当前网格的速度, 作用到当前位置, 倒推得到上一时刻的物理量
#上一时刻的物理量就会来到当前位置
#倒推法求出下一时刻会来到当前网格的物理量
@ti.kernel
def advect_speed_step(vf_0: ti.template(),vf_1: ti.template() ,result:ti.template()):
    for i,j in vf_0:
        uv = tm.vec2(i,j)+tm.vec2(0.5,0.5)
        vel=bilerp(vf_0,uv.x,uv.y,(extent))
        
        offset=vel*dt*advect_speed*0.2
        offset.x*=extent[0]
        offset.y*=extent[1]

        newUV=uv-offset
        result[i,j]=bilerp(vf_1,newUV[0],newUV[1],(extent))
    
#diffusion扩散
#用当前像素周围的燃料混合
@ti.kernel
def diffusion_step(vf_0: ti.template(),vf_1: ti.template() ,result:ti.template()):

    for i,j in vf_0:
        uv = tm.vec2(i,j)+tm.vec2(0.5,0.5)
        uvL=ti.Vector([uv.x-1.0,uv.y])
        uvR=ti.Vector([uv.x+1.0,uv.y])
        uvT=ti.Vector([uv.x,uv.y+1.0])
        uvB=ti.Vector([uv.x,uv.y-1.0])

        L=bilerp(vf_0,uvL.x,uvL.y,(extent))
        R=bilerp(vf_0,uvR.x,uvR.y,(extent))
        T=bilerp(vf_0,uvT.x,uvT.y,(extent))
        B=bilerp(vf_0,uvB.x,uvB.y,(extent))

        bC= bilerp(vf_1,uv.x,uv.y,(extent))

        alpha=dx2/(viscosity*dt)
        beta=4+alpha

        result[i,j]=(L+R+T+B+alpha*bC)/beta


@ti.kernel
def force_step(vf_0: ti.template(),result: ti.template(),inputPos:tm.vec2,forceVec:tm.vec2):
      for i,j in vf_0:
        uv = tm.vec2(i,j)+tm.vec2(0.5,0.5)
        vel=bilerp(vf_0,uv.x,uv.y,(extent))
        pos=inputPos-ti.Vector([uv.x/extent[0],uv.y/extent[1]])
        vel+=forceVec*tm.exp(-tm.dot(pos,pos)/(radius*0.001))*dt*200
        result[i,j]=vel

#divergence发散扩散    
@ti.kernel
def divergence_step(vf_0: ti.template(),result: ti.template()):
    for i,j in vf_0:
        uv = tm.vec2(i,j)+tm.vec2(0.5,0.5)
        uvL=ti.Vector([uv.x-1.0,uv.y])
        uvR=ti.Vector([uv.x+1.0,uv.y])
        uvT=ti.Vector([uv.x,uv.y+1.0])
        uvB=ti.Vector([uv.x,uv.y-1.0])

        L=bilerp(vf_0,uvL.x,uvL.y,(extent))
        R=bilerp(vf_0,uvR.x,uvR.y,(extent))
        T=bilerp(vf_0,uvT.x,uvT.y,(extent))
        B=bilerp(vf_0,uvB.x,uvB.y,(extent))
        
        C=bilerp(vf_0,uv.x,uv.y,(extent))

        if uvL.x<=0.0:
            L=-C
        if uvR.x>=extent[0]:
            R=-C
        if uvT.y>=extent[1]:
            T=-C
        if uvB.y<=0:
            B=-C
        
        result[i,j].fill(halfRdx*(R.x - L.x + T.y - B.y))

#presure压力
@ti.kernel
def pressure_step(pf_0: ti.template(),df_0: ti.template(),result:ti.template()):
    for i,j in pf_0:
        uv = tm.vec2(i,j)+tm.vec2(0.5,0.5)
        uvL=ti.Vector([uv.x-1.0,uv.y])
        uvR=ti.Vector([uv.x+1.0,uv.y])
        uvT=ti.Vector([uv.x,uv.y+1.0])
        uvB=ti.Vector([uv.x,uv.y-1.0])

        L=bilerp(pf_0,uvL.x,uvL.y,(extent)).x
        R=bilerp(pf_0,uvR.x,uvR.y,(extent)).x
        T=bilerp(pf_0,uvT.x,uvT.y,(extent)).x
        B=bilerp(pf_0,uvB.x,uvB.y,(extent)).x

        bC=bilerp(df_0,uv.x,uv.y,(extent))

        alpha=-dx2

        beta=4

        result[i,j]=(L+R+T+B+alpha*bC)/beta

#gradient坡度
@ti.kernel
def gradient_step(pf_0:ti.template(),vf_1:ti.template(),result:ti.template()):
    for i,j in pf_0:
        uv = tm.vec2(i,j)+tm.vec2(0.5,0.5)
        uvL=ti.Vector([uv.x-1.0,uv.y])
        uvR=ti.Vector([uv.x+1.0,uv.y])
        uvT=ti.Vector([uv.x,uv.y+1.0])
        uvB=ti.Vector([uv.x,uv.y-1.0])

        L=bilerp(pf_0,uvL.x,uvL.y,(extent)).x
        R=bilerp(pf_0,uvR.x,uvR.y,(extent)).x
        T=bilerp(pf_0,uvT.x,uvT.y,(extent)).x
        B=bilerp(pf_0,uvB.x,uvB.y,(extent)).x

        bC=bilerp(vf_1,uv.x,uv.y,(extent))
        bC-=halfRdx*tm.vec2(R-L,T-B)
        
        result[i,j]=bC
   
init_color_field(image)

window = ti.ui.Window("taichi_euler_fluid_2d", res=(extent[0], extent[1]),vsync=True)
canvas = window.get_canvas()

is_mouse_left_button_release=True

while window.running:

    advect_speed_step(velocity_field_double_buffer.cur,velocity_field_double_buffer.cur,velocity_field_double_buffer.nxt)
    velocity_field_double_buffer.swap()

    for i in range(0,diffusion_times):
        diffusion_step(velocity_field_double_buffer.cur,velocity_field_double_buffer.cur,velocity_field_double_buffer.nxt)
        velocity_field_double_buffer.swap()

    if(window.get_event(ti.ui.RELEASE)):
        if window.event.key in [ti.ui.LMB]:
            is_mouse_left_button_release=True

    if(window.is_pressed(ti.ui.LMB)):
        cur_cursor_pos_x,cur_cursor_pos_y=window.get_cursor_pos()
        cur_cursor_pos=tm.vec2(cur_cursor_pos_x,cur_cursor_pos_y)
        if(is_mouse_left_button_release):
            last_cursor_pos=cur_cursor_pos
            is_mouse_left_button_release=False


    inputPos=cur_cursor_pos
    forceVec=(cur_cursor_pos-last_cursor_pos) *force_scale
    last_cursor_pos=cur_cursor_pos
    force_step(velocity_field_double_buffer.cur,velocity_field_double_buffer.nxt,inputPos,forceVec)
    velocity_field_double_buffer.swap()

    divergence_step(velocity_field_double_buffer.cur,divergence_field)

    for i in range(0,55):
        pressure_step(pressure_field_double_buffer.cur,divergence_field,pressure_field_double_buffer.nxt)
        pressure_field_double_buffer.swap()

    gradient_step(pressure_field_double_buffer.cur,velocity_field_double_buffer.cur,velocity_field_double_buffer.nxt)
    velocity_field_double_buffer.swap()

    advect_speed_step(velocity_field_double_buffer.cur,color_field_double_buffer.cur,color_field_double_buffer.nxt)

    canvas.set_image(color_field_double_buffer.nxt)
    color_field_double_buffer.swap()

    # canvas.set_image(velocity_field_double_buffer.nxt)

    release_pressure_double_buffer()

    window.show()