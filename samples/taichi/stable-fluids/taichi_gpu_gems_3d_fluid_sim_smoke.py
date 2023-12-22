# https://github.com/Scrawk/GPU-GEMS-3D-Fluid-Simulation.git

from enum import Enum
import taichi as ti
import taichi.math as tm
import sys
import os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from utils import *

ti.init(arch=ti.vulkan)

class Advection(Enum):
    NORMAL=0,
    BFECC=1
    MACCORMACK=2

phi_n_hat=0
phi_n_1_hat=1

time_step=0.1

extent=[512,512]

advection_type=Advection.NORMAL
width=128
height=128
depth=128

smoke_size=tm.vec3(width,height,depth)

iterations=10
vorticity_strength=1.0
density_amount=1.0
density_dissipation=0.999
density_buoyancy=1.0
density_weight=0.0125
temperature_amount=10.0
temperature_dissipation=0.995
velocity_dissipation=0.995
input_radius=0.04
input_pos=tm.vec3(0.5,0.1,0.5)

ambient_temperature=0.0

smoke_buffer_size=width*height*depth

density_d_buf=DoubleBuffer(ti.Vector.field(1, float, shape=smoke_buffer_size),ti.Vector.field(1, float, shape=smoke_buffer_size))
temperature_d_buf=DoubleBuffer(ti.Vector.field(1, float, shape=smoke_buffer_size),ti.Vector.field(1, float, shape=smoke_buffer_size))
phi_d_buf=DoubleBuffer(ti.Vector.field(1, float, shape=smoke_buffer_size),ti.Vector.field(1, float, shape=smoke_buffer_size))
velocity_d_buf=DoubleBuffer(ti.Vector.field(3, float, shape=smoke_buffer_size),ti.Vector.field(3, float, shape=smoke_buffer_size))
pressure_d_buf=DoubleBuffer(ti.Vector.field(1, float, shape=smoke_buffer_size),ti.Vector.field(1, float, shape=smoke_buffer_size))
obstacle_buf=ti.Vector.field(1, float, shape=smoke_buffer_size)
tmp_buf=ti.Vector.field(3, float, shape=smoke_buffer_size)

last_cursor_pos:tm.vec2=tm.vec2(0,0)
cur_cursor_pos:tm.vec2=tm.vec2(0,0)
is_mouse_left_button_release=True
is_mouse_right_button_release=True

color_buf=ti.Vector.field(3, float, shape=extent)

smoke_color=tm.vec3(0.5,0.5,0.5)
smoke_absorption=60.0
smoke_position=tm.vec3(0,0,0)
smoke_scale=tm.vec3(4,8,4)

NUM_SAMPLES=64
		
@ti.kernel
def compute_obstacle(obstacle_buf:ti.template()):
    for i,j,k in obstacle_buf:
        uv=tm.vec3(i+0.5,j+0.5,k+0.5)
        obstacle=0.0
        if uv.x-1<0:
            obstacle=1.0
        if uv.x+1.0>width-1:
            obstacle=1.0
        if uv.y-1<0:
            obstacle=1.0
        if uv.y+1>height-1:
            obstacle=1.0
        if uv.z-1<0:
            obstacle=1.0
        if uv.z+1>depth-1:
            obstacle=1.0
        obstacle_buf[i,j,k]=obstacle

@ti.func
def sample_bilinear(buffer:ti.template(),uv:tm.vec3,size:tm.vec3)->ti.f32:
    uv=tm.clamp(uv,tm.vec3(0),tm.vec3(1))
    uv=uv*(size-tm.vec3(1))

    x=uv.x
    y=uv.y
    z=uv.z

    X=size.x
    XY=size.x*size.y

    fx=uv.x-x
    fy=uv.y-y
    fz=uv.z-z

    xp1=tm.min(smoke_size.x-1,x+1)
    yp1=tm.min(smoke_size.y-1,y+1)
    zp1=tm.min(smoke_size.z-1,z+1)

    x0=buffer[x + y * X + z * XY] * (1.0 - fx) + buffer[xp1 + y * X + z * XY] * fx
    x1=buffer[x + y * X + zp1 * XY] * (1.0 - fx) + buffer[xp1 + y * X + zp1 * XY] * fx

    x2=buffer[x + yp1 * X + z * XY] * (1.0 - fx) + buffer[xp1 + yp1 * X + z * XY] * fx
    x3=buffer[x + yp1 * X + zp1 * XY] * (1.0 - fx) + buffer[xp1 + yp1 * X + zp1 * XY] * fx

    z0=x0*(1.0-fz)+x1*fz
    z1=x2*(1.0-fz)+x3*fz

    return z0*(1.0-fy)+z1*fy

@ti.dataclass
class Ray:
    origin: tm.vec3
    dir: tm.vec3

    def __init__(self, o: tm.vec3 = tm.vec3(0, 0, 0), d: tm.vec3 = tm.vec3(0, 0, 0)) -> None:
        self.origin = o
        self.dir = d

    @ti.func
    def at(self, t: ti.f32) -> tm.vec3:
        return self.origin+t*self.dir

@ti.dataclass
class Camera:           # 相机类
    lookfrom: tm.vec3      # 视点位置
    lookat: tm.vec3        # 目标位置
    vup: tm.vec3           # 向上的方向
    vfov: float         # 视野
    aspect: float       # 传感器长宽比
    aperture: float     # 光圈大小

    @ti.func
    def get_ray(c, s, t) -> Ray:
        # 根据 vfov 和画布长宽比计算出半高和半宽
        theta = tm.radians(c.vfov) # 角度转弧度
        half_height = tm.tan(theta * 0.5)
        half_width = c.aspect * half_height

        # 以目标位置到摄像机位置为 Z 轴正方向
        z = tm.normalize(c.lookfrom - c.lookat)
        # 计算出摄像机传感器的 XY 轴正方向
        x = tm.normalize(tm.cross(c.vup, z))
        y = tm.cross(z, x)

        # 计算出画布左下角
        lower_left_corner = c.lookfrom  - half_width  * x - half_height * y -               z

        horizontal = 2.0 * half_width  * x
        vertical   = 2.0 * half_height * y

        # 计算光线起点和方向 
        ro = c.lookfrom 
        rp = lower_left_corner  + s*horizontal  + t*vertical
        rd = tm.normalize(rp - ro)
    
        return Ray(ro, rd)

ti.dataclass
class AABB:
    min_pointer:tm.vec3
    max_pointer:tm.vec3

    def __init__(self,min:tm.vec3,max:tm.vec3) -> None:
        self.min_pointer=min
        self.max_pointer=max

    @ti.func
    def intersect(self,ray:Ray)->[bool,ti.f32,ti.f32]:
        invR = 1.0 / ray.dir
        tbot = invR * (self.min_pointer-ray.origin)			
        ttop = invR * (self.max_pointer-ray.origin)
        tmin = tm.min(ttop, tbot)
        tmax = tm.max(ttop, tbot)
        t = tm.max(tmin.xx, tmin.yz)
        t0 = tm.max(t.x, t.y)
        t = tm.min(tmax.xx, tmax.yz)
        t1 = tm.min(t.x, t.y)
        return t0 <= t1,t0,t1

camera = Camera()
camera.lookfrom = tm.vec3(0, 0, 24) # 设置摄像机位置
camera.lookat = tm.vec3(0, 0, 0)   # 设置目标位置
camera.vup = tm.vec3(0, 1, 0)      # 设置向上的方向
camera.aspect = float(extent[0])/extent[1]    # 设置长宽比
camera.vfov = 40                # 设置视野
camera.aperture = 0.01          # 设置光圈大小

smoke_block=AABB(tm.vec3(-1,-1,-1)*smoke_scale+smoke_position,
                 tm.vec3(1,1,1)*smoke_scale+smoke_position)

@ti.func
def ray_march(r):
    result_color = tm.vec3(0.2, 0.3, 0.5)
    isHit,t_min,t_max=smoke_block.intersect(r)
    
    if isHit:
        if t_min < 0.0:
             t_min = 0.0

        rayStart = r.origin + r.dir * t_min
        rayStop = r.origin + r.dir * t_max

        rayStart -= smoke_position
        rayStop -= smoke_position
        rayStart = (rayStart + 0.5*smoke_scale)/smoke_scale
        rayStop = (rayStop + 0.5*smoke_scale)/smoke_scale

        start=rayStart
        dist=tm.distance(rayStop,rayStart)
        stepSize=dist/float(NUM_SAMPLES)
        ds=tm.normalize(rayStop-rayStart)*stepSize
        alpha=1.0

        for i in range(0,NUM_SAMPLES):
            start+=ds
            D=sample_bilinear(density_d_buf.cur,start,smoke_size)
            alpha*=1.0-tm.clamp(D*stepSize*smoke_absorption,tm.vec3(0),tm.vec3(1))
            if alpha<=0.01:
                break

        result_color = smoke_color*1-alpha

    return result_color

@ti.kernel
def render():
    for i, j in color_buf:
        u = float(i)/float(extent[0])
        v = float(j)/float(extent[1])
        color_buf[i, j] = ray_march(camera.get_ray(u, v))

window = ti.ui.Window("taichi_gpu_gems_3d_fluid_sim_smoke", res=(extent[0], extent[1]))
canvas = window.get_canvas()

while window.running:
    render()
    canvas.set_image(color_buf)
    window.show()