from taichi.math import *
import taichi as ti

ti.init(arch=ti.vulkan)


aspect_ratio: ti.f32 = 3.0/2.0
image_width = 1200
image_height = int(image_width/aspect_ratio)
image_pixels = ti.Vector.field(3, float, (image_width, image_height))
image_accum = ti.Vector.field(3, float, (image_width, image_height))
samples_per_pixel = 1
max_depth = 10

DOUBLE_MAX: ti.f32 = 1.7976931348623158E+308


@ti.func
def random_float01() -> ti.f32:
   return ti.random(dtype=float)


@ti.func
def random_float(min, max) -> ti.f32:
    return min+(max-min)*random_float01()


@ti.func
def random_vec301() -> vec3:
    v = vec3(random_float01(), random_float01(), random_float01())
    return v


@ti.func
def random_vec3(min, max) -> vec3:
    v = vec3(random_float(min, max), random_float(
        min, max), random_float(min, max))
    return v


@ti.func
def random_in_unit_sphere() -> vec3:
    p = random_vec3(-1, 1)
    while (True):
        p = random_vec3(-1, 1)
        if (p.x*p.x+p.y*p.y) >= 1:
            continue
        break
    return p


@ti.func
def random_in_unit_vector() -> vec3:
    return normalize(random_in_unit_sphere())


@ti.func
def random_in_hemisphere(normal: vec3) -> vec3:
    in_unit_sphere = random_in_unit_sphere()
    if dot(in_unit_sphere, normal) <= 0.0:
        in_unit_sphere *= -1.0
    return in_unit_sphere


@ti.func
def random_in_unit_disk() -> vec3:
    p = random_vec3(-1, 1)
    while (True):
        p = vec3(random_float(-1, 1), random_float(-1, 1), 0)
        if (p.x*p.x+p.y*p.y) >= 1:
            continue
        break
    return p


@ti.func
def Fab(v: ti.f32) -> ti.f32:
    result = v
    if result < 0:
        result *= -1
    return result


@ti.func
def Schlick(cosine, ref_idx):
    r0 = (1.0-ref_idx)/(1.0+ref_idx)
    r0 *= r0
    return r0+(1-r0)*pow((1-cosine), 5)


@ti.func
def IsNearZero(v: vec3) -> ti.f32:
    result = 1
    if Fab(v.x >= 1e-8) or Fab(v.y >= 1e-8) or Fab(v.z >= 1e-8):
        result = 0
    return result


@ti.dataclass
class Ray:
    origin: vec3
    dir: vec3

    def __init__(self, o: vec3 = vec3(0, 0, 0), d: vec3 = vec3(0, 0, 0)) -> None:
        self.origin = o
        self.dir = d

    @ti.func
    def at(self, t: ti.f32) -> vec3:
        return self.origin+t*self.dir


MATERIAL_DIFFUSE = 1
MATERIAL_LAMBERTIAN = 2
MATERIAL_HALF_LAMBERTIAN = 3
MATERIAL_METAL = 4
MATERIAL_DIELECTRIC = 5


@ti.dataclass
class MaterialScatter:
    hasScatter: ti.i32
    attenuation: vec3
    scatterRay: Ray


@ti.dataclass
class Material:
    type: ti.i32
    albedo: vec3
    fuzz: ti.f32
    ir: ti.f32


@ti.func
def DiffuseMatScatter(mat, r_in, rec) -> MaterialScatter:
    result = MaterialScatter()
    scatter_dir = rec.normal+random_in_unit_sphere()
    result.scatterRay = Ray(rec.p, scatter_dir)
    result.attenuation = mat.albedo
    result.hasScatter = 1
    return result


@ti.func
def LambertianMatScatter(mat, r_in, rec) -> MaterialScatter:
    result = MaterialScatter()
    scatter_dir = rec.normal+random_in_unit_vector()
    result.scatterRay = Ray(rec.p, scatter_dir)
    result.attenuation = mat.albedo
    result.hasScatter = 1
    return result


@ti.func
def HalfLambertianMatScatter(mat, r_in, rec) -> MaterialScatter:
    result = MaterialScatter()
    scatter_dir = random_in_hemisphere(rec.normal)
    if (IsNearZero(scatter_dir)):
        scatter_dir = rec.normal
    result.scatterRay = Ray(rec.p, scatter_dir)
    result.attenuation = mat.albedo
    result.hasScatter = 1
    return result


@ti.func
def MetalMatScatter(mat, r_in, rec) -> MaterialScatter:
    result = MaterialScatter()
    scatter_dir = reflect(normalize(r_in.dir), rec.normal)
    result.scatterRay = Ray(
        rec.p, scatter_dir+mat.fuzz*random_in_unit_sphere())
    result.attenuation = mat.albedo
    result.hasScatter = dot(scatter_dir, rec.normal) > 0
    return result


@ti.func
def DielectricMatScatter(mat, r_in, rec) -> MaterialScatter:
    result = MaterialScatter()

    refraction_ratio = mat.ir
    if (rec.front_face == 1.0):
        refraction_ratio = 1.0/mat.ir

    unitDir = normalize(r_in.dir)

    cos_theta = min(dot(-unitDir, rec.normal), 1.0)
    sin_theta = sqrt(1.0-cos_theta*cos_theta)

    cannot_refract = refraction_ratio*sin_theta > 1.0

    dir = vec3(0.0, 0.0, 0.0)
    if (cannot_refract or Schlick(cos_theta, refraction_ratio) > random_float01()):
        dir = reflect(unitDir, rec.normal)
    else:
        dir = refract(unitDir, rec.normal, refraction_ratio)

    result.scatterRay = Ray(rec.p, dir)
    result.attenuation = vec3(1.0, 1.0, 1.0)
    result.hasScatter = 1
    return result


@ti.func
def MaterialScatterCalc(mat, r_in, rec) -> MaterialScatter:
    result = MaterialScatter()

    if mat.type == MATERIAL_DIFFUSE:
        result = DiffuseMatScatter(mat, r_in, rec)
    elif mat.type == MATERIAL_LAMBERTIAN:
        result = LambertianMatScatter(mat, r_in, rec)
    elif mat.type == MATERIAL_HALF_LAMBERTIAN:
        result = HalfLambertianMatScatter(mat, r_in, rec)
    elif mat.type == MATERIAL_METAL:
        result = MetalMatScatter(mat, r_in, rec)
    elif mat.type == MATERIAL_DIELECTRIC:
        result = DielectricMatScatter(mat, r_in, rec)
    return result


@ti.dataclass
class HitRecord:
    p: vec3
    normal: vec3
    t: ti.f32
    front_face: int
    hit: ti.i32
    material: Material

    @ti.func
    def set_face_normal(self, r, outward_normal):
        self.front_face = int(dot(r.dir, outward_normal) < 0)
        if self.front_face == 1:
            self.normal = outward_normal
        else:
            self.normal = -outward_normal


OBJECT_SPHERE = 1


@ti.dataclass
class Object:
    type: ti.i32
    center: vec3
    radius: ti.f32
    material: Material


@ti.func
def SphereHit(obj, r, t_min, t_max) -> HitRecord:
    rec = HitRecord()
    oc = r.origin-obj.center
    a = pow(length(r.dir), 2)
    half_b = dot(oc, r.dir)
    c = pow(length(oc), 2)-obj.radius*obj.radius
    discriminant = half_b*half_b-a*c
    rec.hit = True
    if discriminant < 0.0:
        rec.hit = False
    sqrtd = sqrt(discriminant)
    root = (-half_b-sqrtd)/a
    if root < t_min or root > t_max:
        root = (-half_b+sqrtd)/a
        if root < t_min or root > t_max:
            rec.hit = False

    rec.t = root
    rec.p = r.at(rec.t)
    rec.normal = (rec.p-obj.center)/obj.radius
    outward_normal = rec.normal
    rec.set_face_normal(r, outward_normal)
    rec.material = obj.material
    return rec


@ti.func
def ObjectHit(obj, r, t_min, t_max) -> HitRecord:
    rec = HitRecord()
    if obj.type == OBJECT_SPHERE:
        rec = SphereHit(obj, r, t_min, t_max)
    return rec


@ti.dataclass
class Camera:
    origin: vec3
    lower_left_corner: vec3
    horizontal: vec3
    vertical: vec3
    u: vec3
    v: vec3
    w: vec3
    lens_radius: ti.f32


@ti.func
def InitCamera(self, lookfrom: vec3, lookat: vec3, vup: vec3, vfov: ti.f32, aspect_ratio: ti.f32, aperture:ti.f32, focus_dist:ti.f32) -> None:
    theta = radians(vfov)
    h = tan(theta/2.0)
    viewport_height = 2.0*h
    viewport_width = aspect_ratio * viewport_height

    self.w = normalize(lookfrom-lookat)
    self.u = normalize(cross(vup, self.w))
    self.v = cross(self.w, self.u)
    self.origin = lookfrom
    self.horizontal = focus_dist * viewport_width*self.u
    self.vertical = focus_dist * viewport_height*self.v
    self.lower_left_corner = self.origin - self.horizontal / \
        2.0 - self.vertical/2.0 - focus_dist*self.w
    self.lens_radius = aperture/2

    return self


@ti.func
def CamGetRay(self, s, t) -> Ray:
    rd = self.lens_radius*random_in_unit_disk()
    offset = self.u*rd.x+self.v*rd.y
    dir = self.lower_left_corner+s*self.horizontal+t*self.vertical-self.origin-offset
    r = Ray(self.origin+offset, dir)
    return r


cam = Camera()

objects_num = 500
objects = Object.field(shape=objects_num)


@ti.func
def InitScene():
    objects[0] = Object(type=OBJECT_SPHERE,
                        center=vec3(0, -1000, -0),
                        radius=1000,
                        material=Material(type=MATERIAL_LAMBERTIAN, albedo=(0.5, 0.5, 0.5)))

    objects[1] = Object(type=OBJECT_SPHERE,
                        center=vec3(0, 1, 0),
                        radius=1,
                        material=Material(type=MATERIAL_DIELECTRIC, ir=1.5))

    objects[2] = Object(type=OBJECT_SPHERE,
                        center=vec3(-4, 1, 0),
                        radius=1,
                        material=Material(type=MATERIAL_LAMBERTIAN, albedo=(0.4, 0.2, 0.1)))

    objects[3] = Object(type=OBJECT_SPHERE,
                        center=vec3(4, 1, 0),
                        radius=1,
                        material=Material(type=MATERIAL_METAL, albedo=(0.7, 0.6, 0.5), fuzz=0.0))

    count = 4
    for a in range(-11, 11):
        for b in range(-11, 11):
            choose_mat = random_float01()
            center = (a+0.9*random_float01(), 0.2, b+0.9*random_float01())

            if (center-vec3(4.0, 0.2, 0.0)).norm() > 0.9:
                if choose_mat < 0.8:
                    albedo = random_vec301()*random_vec301()
                    objects[count] = Object(type=OBJECT_SPHERE,
                                            center=center,
                                            radius=0.2,
                                            material=Material(type=MATERIAL_LAMBERTIAN, albedo=albedo))
                elif choose_mat < 0.95:
                    albedo = random_vec3(0.5, 1)
                    fuzz = random_float(0, 0.5)
                    objects[count] = Object(type=OBJECT_SPHERE,
                                            center=center,
                                            radius=0.2,
                                            material=Material(type=MATERIAL_METAL, albedo=albedo, fuzz=fuzz))
                else:
                    objects[count] = Object(type=OBJECT_SPHERE,
                                            center=center,
                                            radius=0.2,
                                            material=Material(type=MATERIAL_DIELECTRIC, ir=1.5))
            count += 1


@ti.func
def WorldHit(r, t_min: ti.f32, t_max: ti.f32) -> HitRecord:
    rec = HitRecord(hit=False)
    temp_rec = HitRecord()
    closest_so_far: ti.f32 = t_max
    for i in range(objects_num):
        o = objects[i]
        temp_rec = ObjectHit(o, r, t_min, closest_so_far)
        if temp_rec.hit == 1:
            closest_so_far = temp_rec.t
            rec = temp_rec

    return rec


@ti.func
def RayTrace(r, depth) -> vec3:
    ray = r
    result_color = vec3(1, 1, 1)

    for _ in range(depth):
        rec = WorldHit(ray, 0.001, DOUBLE_MAX)
        if rec.hit:
            scatter = MaterialScatterCalc(rec.material, ray, rec)
            if scatter.hasScatter:
                ray = scatter.scatterRay
                result_color *= scatter.attenuation
            else:
                result_color *= vec3(0.0, 0.0, 0.0)
        else:
            unit_dir = normalize(ray.dir)
            t = 0.5*unit_dir.y+0.5
            env_color = (1.0-t)*vec3(1.0, 1.0, 1.0)+t*vec3(0.5, 0.7, 1.0)
            result_color *= env_color
            break

    return result_color


lookfrom = vec3(13, 2, 3)
lookat = vec3(0, 0, 0)
vup = vec3(0, 1, 0)
aperture = 0.1
focus_dist = 10

@ti.kernel
def Init():
    InitScene()

    cam = InitCamera(cam, lookfrom, lookat,
                     vup, 20, aspect_ratio, aperture, focus_dist)


@ti.kernel
def Render(frame_count: ti.f32):

    for i, j in image_pixels:
        color = vec3(0.0, 0.0, 0.0)
        for _ in range(samples_per_pixel):
            u = float(i+random_float01())/image_width
            v = float(j+random_float01())/image_height
            r = CamGetRay(cam, u, v)

            color += RayTrace(r, max_depth)

        color /= samples_per_pixel

        image_accum[i, j] += color
        image_pixels[i, j] = pow(image_accum[i, j]/frame_count, 1.0/2.2)


window = ti.ui.Window("RayTracingInTaichi", (image_width, image_height))
canvas = window.get_canvas()
frame_count = 1

Init()
while window.running:
    Render(frame_count)
    canvas.set_image(image_pixels)
    window.show()
    frame_count += 1
