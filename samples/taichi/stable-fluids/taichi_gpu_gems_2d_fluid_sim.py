# https://github.com/Scrawk/GPU-GEMS-2D-Fluid-Simulation.git


import taichi as ti
import taichi.math as tm
import matplotlib.image as mpimg
import sys
import os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from utils import *

ti.init(arch=ti.vulkan)


extent=[512,512]
color_buf=ti.Vector.field(3, float, shape=extent)
divergence_buf=ti.Vector.field(1, float, shape=extent)
obstacles_buf=ti.Vector.field(1, float, shape=extent)

velocity_d_buf=DoubleBuffer(ti.Vector.field(2, float, shape=extent),ti.Vector.field(2, float, shape=extent))
density_d_buf=DoubleBuffer(ti.Vector.field(1, float, shape=extent),ti.Vector.field(1, float, shape=extent))
pressure_d_buf=DoubleBuffer(ti.Vector.field(1, float, shape=extent),ti.Vector.field(1, float, shape=extent))
temperature_d_buf=DoubleBuffer(ti.Vector.field(1, float, shape=extent),ti.Vector.field(1, float, shape=extent))

fluid_color=tm.vec3(1.0,0.0,0.0)
obstacle_color=tm.vec3(1.0,1.0,1.0)

impluse_temperature=10.0
impluse_density=1.0
temperature_dissipation=0.99
velocity_dissipation=0.99
density_dissipation=0.9999
ambient_temperature=0.0
smoke_buoyancy=1.0
smoke_weight=0.05

cell_size=1.0
gradient_scale=1.0

num_jacobi_iterations=50

impluse_pos=tm.vec2(0.5,0.0)

impluse_radius=0.1
mouse_impluse_radius=0.05

obstacle_pos=tm.vec2(0.5,0.5)
obstacle_radius=0.1

dt=0.125

last_cursor_pos:tm.vec2=tm.vec2(0,0)
cur_cursor_pos:tm.vec2=tm.vec2(0,0)
is_mouse_left_button_release=True
is_mouse_right_button_release=True

@ti.kernel
def init_color_field():
    for i, j in ti.ndrange(extent[0], extent[1]):
        color_buf[i, j] = (
           0.0,
           0.0,
           0.0
        )

@ti.kernel
def release_buffer(buf:ti.template()):
     for i,j in buf:
         buf[i,j]=0.0

@ti.kernel
def advect_step(velocity:ti.template(),obstacle:ti.template(),source:ti.template(),dissipation:ti.f32,time_step:ti.f32,dest:ti.template()):
    for i,j in velocity:
        uv=tm.vec2(i+0.5,j+0.5)
        vel=bilerp(velocity,uv.x,uv.y,(extent))
        coord=uv-(vel*time_step)
        result=dissipation*bilerp(source,coord.x,coord.y,(extent))
        solid=bilerp(obstacle,uv.x,uv.y,(extent)).x
        if solid>0.0:
            result=tm.vec2(0.0,0.0)
        dest[i,j]=result

@ti.kernel
def advect_step_temperature(velocity:ti.template(),obstacle:ti.template(),source:ti.template(),dissipation:ti.f32,time_step:ti.f32,dest:ti.template()):
    for i,j in velocity:
        uv=tm.vec2(i+0.5,j+0.5)
        vel=bilerp(velocity,uv.x,uv.y,(extent))
        coord=uv-(vel*time_step)
        result=dissipation*bilerp(source,coord.x,coord.y,(extent)).x
        solid=bilerp(obstacle,uv.x,uv.y,(extent)).x
        if solid>0.0:
            result=0.0
        dest[i,j]=result

@ti.kernel
def apply_buoyancy_step(velocity:ti.template(),temperature:ti.template(),density:ti.template(),ambient_temperature:ti.f32,time_step:ti.f32,sigma:ti.f32,kappa:ti.f32,dest:ti.template()):
    for i,j in velocity:
        uv=tm.vec2(i+0.5,j+0.5)
        T=bilerp(temperature,uv.x,uv.y,(extent)).x
        V=bilerp(velocity,uv.x,uv.y,(extent)).xy
        D=bilerp(density,uv.x,uv.y,(extent)).x

        result=V

        if T> ambient_temperature:
            result+=(time_step*(T-ambient_temperature)*sigma-D* kappa)*tm.vec2(0,1)

        dest[i,j]=result

@ti.kernel
def apply_impluse_step(source:ti.template(),pos:tm.vec2,radius:ti.f32,val:ti.f32,dest:ti.template()):
     for i,j in source:
        uv=tm.vec2(i+0.5,j+0.5)
        norm_uv=tm.vec2(uv.x/extent[0],uv.y/extent[1])
        d=tm.distance(pos,norm_uv)

        impluse=0.0

        if d<radius:
            a=(radius-d)*0.5
            impluse=tm.min(a,1.0)
        
        src_val=bilerp(source,uv.x,uv.y,(extent)).x
        dest[i,j]=tm.max(0,lerp(src_val,val,impluse))

@ti.kernel
def compute_divergence_step(velocity:ti.template(),obstacle:ti.template(),half_inverse_cell_size:ti.f32,dest:ti.template()):
    for i,j in velocity:
        uv=tm.vec2(i+0.5,j+0.5)

        uvN=uv+tm.vec2(0,1)
        uvS=uv+tm.vec2(0,-1)
        uvE=uv+tm.vec2(1,0)
        uvW=uv+tm.vec2(-1,0)

        # Find neighboring velocities:
        vN=bilerp(velocity,uvN.x,uvN.y,(extent)).xy
        vS=bilerp(velocity,uvS.x,uvS.y,(extent)).xy
        vE=bilerp(velocity,uvE.x,uvE.y,(extent)).xy
        vW=bilerp(velocity,uvW.x,uvW.y,(extent)).xy

        # Find neighboring obstacles:
        bN=bilerp(obstacle,uvN.x,uvN.y,(extent)).x
        bS=bilerp(obstacle,uvS.x,uvS.y,(extent)).x
        bE=bilerp(obstacle,uvE.x,uvE.y,(extent)).x
        bW=bilerp(obstacle,uvW.x,uvW.y,(extent)).x
        
        # Set velocities to 0 for solid cells:
        if bN>0.0:
            vN=0.0
        if bS>0.0:
            vS=0.0
        if bE>0.0:
            vE=0.0
        if bW>0.0:
            vW=0.0

        dest[i,j]=half_inverse_cell_size*(vE.x-vW.x+vN.y-vS.y)

@ti.kernel
def jacobi(pressure:ti.template(),divergence:ti.template(),obstacle:ti.template(),alpha:ti.f32,inverse_beta:ti.f32,dest:ti.template()):
    for i,j in pressure:
        uv=tm.vec2(i+0.5,j+0.5)

        uvN=uv+tm.vec2(0.0,1)
        uvS=uv+tm.vec2(0.0,-1)
        uvE=uv+tm.vec2(1,0.0)
        uvW=uv+tm.vec2(-1,0.0)

        # Find neighboring pressure:
        pN=bilerp(pressure,uvN.x,uvN.y,(extent)).x        
        pS=bilerp(pressure,uvS.x,uvS.y,(extent)).x        
        pE=bilerp(pressure,uvE.x,uvE.y,(extent)).x        
        pW=bilerp(pressure,uvW.x,uvW.y,(extent)).x    

        pC=bilerp(pressure,uv.x,uv.y,(extent)).x

	    # Find neighboring obstacles:
        bN=bilerp(obstacle,uvN.x,uvN.y,(extent)).x
        bS=bilerp(obstacle,uvS.x,uvS.y,(extent)).x
        bE=bilerp(obstacle,uvE.x,uvE.y,(extent)).x
        bW=bilerp(obstacle,uvW.x,uvW.y,(extent)).x

        if bN>0.0:
            pN=pC
        if bS>0.0:
            pS=pC
        if bE>0.0:
            pE=pC
        if bW>0.0:
            pW=pC

        bC=bilerp(divergence,uv.x,uv.y,(extent)).x

        dest[i,j]=(pW+pE+pS+pN+alpha*bC)*inverse_beta

@ti.kernel
def subtract_gradient(velocity:ti.template(),pressure:ti.template(),obstacle:ti.template(),gradient_scale:ti.f32,dest:ti.template()):
    for i,j in velocity:
        uv=tm.vec2(i+0.5,j+0.5)

        uvN=uv+tm.vec2(0,1)
        uvS=uv+tm.vec2(0,-1)
        uvE=uv+tm.vec2(1,0)
        uvW=uv+tm.vec2(-1,0)

        # Find neighboring pressure:
        pN=bilerp(pressure,uvN.x,uvN.y,(extent)).x
        pS=bilerp(pressure,uvS.x,uvS.y,(extent)).x
        pE=bilerp(pressure,uvE.x,uvE.y,(extent)).x
        pW=bilerp(pressure,uvW.x,uvW.y,(extent)).x

        pC=bilerp(pressure,uv.x,uv.y,(extent)).x

        # Find neighboring obstacles:
        bN=bilerp(obstacle,uvN.x,uvN.y,(extent)).x
        bS=bilerp(obstacle,uvS.x,uvS.y,(extent)).x
        bE=bilerp(obstacle,uvE.x,uvE.y,(extent)).x
        bW=bilerp(obstacle,uvW.x,uvW.y,(extent)).x

        # Use center pressure for solid cells:
        if bN > 0.0: 
            pN = pC
        if bS > 0.0: 
            pS = pC
        if bE > 0.0: 
            pE = pC
        if bW > 0.0: 
            pW = pC

        oldV=bilerp(velocity,uv.x,uv.y,(extent)).xy
        grad=tm.vec2(pE-pW,pN-pS)*gradient_scale
        newV=oldV-grad

        dest[i,j]=newV

@ti.kernel
def add_obstacle(point:tm.vec2,radius:ti.f32,dest:ti.template()):
    for i,j in dest:
        uv=tm.vec2(i+0.5,j+0.5)

        result=0

        if uv.x<=1:
            result=1
        if uv.x>=extent[0]-1:
            result=1
        if uv.y<=1:
            result=1
        if uv.y>=extent[1]-1:
            result=1

        # draw point   
        norm_uv=tm.vec2(uv.x/extent[0],uv.y/extent[1])
        d=tm.distance(point,norm_uv)

        if d<radius:
            result=1

        dest[i,j]=result

@ti.kernel
def draw_step(obstacle:ti.template(),color:ti.template(),fluid_color:tm.vec3,obstacle_color:tm.vec3,dest:ti.template()):
    for i,j in obstacle:
        uv=tm.vec2(i+0.5,j+0.5)
        col=fluid_color*bilerp(color,uv.x,uv.y,(extent)).x
        obs=bilerp(obstacle,uv.x,uv.y,(extent)).x
        result=lerp(col,obstacle_color,obs)
        dest[i,j]=result


window = ti.ui.Window("taichi_gpu_gems_2d_fluid_sim", res=(extent[0], extent[1]))
canvas = window.get_canvas()

init_color_field()
while window.running:

    #Obstacles only need to be added once unless changed
    add_obstacle(obstacle_pos,obstacle_radius,obstacles_buf)

    #Advect velocity against its self
    advect_step(velocity_d_buf.cur,obstacles_buf,velocity_d_buf.cur,velocity_dissipation,dt,velocity_d_buf.nxt)    

    # Advect temperature against velocity
    advect_step_temperature(velocity_d_buf.cur,obstacles_buf,temperature_d_buf.cur,temperature_dissipation,dt,temperature_d_buf.nxt)

    # Advect density against velocity
    advect_step_temperature(velocity_d_buf.cur,obstacles_buf,density_d_buf.cur,density_dissipation,dt,density_d_buf.nxt)

    velocity_d_buf.swap()
    temperature_d_buf.swap()
    density_d_buf.swap()

    # Determine how the flow of the fluid changes the velocity
    apply_buoyancy_step(velocity_d_buf.cur,temperature_d_buf.cur,density_d_buf.cur,ambient_temperature,dt,smoke_buoyancy,smoke_weight,velocity_d_buf.nxt)

    velocity_d_buf.swap()

    # Refresh the impluse of density and temperature
    apply_impluse_step(temperature_d_buf.cur,impluse_pos,impluse_radius,impluse_temperature,temperature_d_buf.nxt)
    apply_impluse_step(density_d_buf.cur,impluse_pos,impluse_radius,impluse_density,density_d_buf.nxt)

    temperature_d_buf.swap()
    density_d_buf.swap()
    
    if window.get_event(ti.ui.RELEASE):
        if window.event.key in [ti.ui.LMB]:
            is_mouse_left_button_release=True

    if window.get_event(ti.ui.RELEASE) :
        if window.event.key in [ti.ui.RMB]:
            is_mouse_right_button_release=False

    if window.is_pressed(ti.ui.LMB) or window.is_pressed(ti.ui.RMB):
        cur_cursor_pos_x,cur_cursor_pos_y=window.get_cursor_pos()
        cur_cursor_pos=tm.vec2(cur_cursor_pos_x,cur_cursor_pos_y)
        if(is_mouse_left_button_release):
            last_cursor_pos=cur_cursor_pos
            is_mouse_left_button_release=False
        if is_mouse_right_button_release:
            last_cursor_pos=cur_cursor_pos
            is_mouse_right_button_release=False
    
        sign=-1.0
        if window.is_pressed(ti.ui.LMB):
            sign=1.0
    
        apply_impluse_step(temperature_d_buf.cur,cur_cursor_pos,mouse_impluse_radius,impluse_temperature,temperature_d_buf.nxt)
        apply_impluse_step(density_d_buf.cur,cur_cursor_pos,mouse_impluse_radius,impluse_density * sign,density_d_buf.nxt)

        temperature_d_buf.swap()
        density_d_buf.swap()

    compute_divergence_step(velocity_d_buf.cur,obstacles_buf,0.5/cell_size,divergence_buf)

    release_buffer(pressure_d_buf.cur)

    for i in range(0,num_jacobi_iterations):
        jacobi(pressure_d_buf.cur,divergence_buf,obstacles_buf,-cell_size*cell_size,0.25,pressure_d_buf.nxt)
        pressure_d_buf.swap()

    #Use the pressure tex that was last rendered into. This computes divergence free velocity
    subtract_gradient(velocity_d_buf.cur,pressure_d_buf.cur,obstacles_buf,gradient_scale,velocity_d_buf.nxt)

    velocity_d_buf.swap()

    draw_step(obstacles_buf,density_d_buf.cur,fluid_color,obstacle_color,color_buf)
    canvas.set_image(color_buf)

    window.show()