"""Generate the original MIT-licensed orbital demo; Python stdlib only."""
from math import sin, cos, pi
from pathlib import Path

vertices, normals, uvs, faces = [], [], [], []

def rotate(p):
    x, y, z = p
    a, b = 0.42, -0.32
    y, z = cos(a)*y-sin(a)*z, sin(a)*y+cos(a)*z
    return cos(b)*x+sin(b)*z, y, -sin(b)*x+cos(b)*z

def surface(rows, columns, evaluate):
    base = len(vertices) + 1
    for i in range(rows+1):
        for j in range(columns+1):
            u, v = i/rows, j/columns
            position, normal = evaluate(u*2*pi, v*2*pi)
            vertices.append(rotate(position))
            normals.append(rotate(normal))
            uvs.append((u, v))
    for i in range(rows):
        for j in range(columns):
            a = base+i*(columns+1)+j
            b = a+columns+1
            faces.extend(((a,b,b+1),(a,b+1,a+1)))

def torus(u,v):
    radius, tube = 0.95, 0.29
    n = cos(v)*cos(u), cos(v)*sin(u), sin(v)
    return ((radius+tube*cos(v))*cos(u), (radius+tube*cos(v))*sin(u), tube*sin(v)), n

surface(96,32,torus)
# Sphere uses latitude in [-pi/2, pi/2]; reverse angular direction for CCW.
def sphere(u,v):
    latitude = v/2-pi/2
    n = cos(latitude)*cos(u), cos(latitude)*sin(u), sin(latitude)
    # Parameter derivatives have the same outward orientation as the torus.
    return (0.48*n[0],0.48*n[1],0.48*n[2]+0.04), n

surface(64,32,sphere)
root = Path(__file__).resolve().parents[1]
out = ['# Original procedural orbital sculpture. MIT; see assets/README.md.']
out += ['v %.8f %.8f %.8f' % p for p in vertices]
out += ['vt %.8f %.8f' % p for p in uvs]
out += ['vn %.8f %.8f %.8f' % p for p in normals]
out += ['f ' + ' '.join(f'{i}/{i}/{i}' for i in face) for face in faces]
(root/'assets'/'orbital.obj').write_text('\n'.join(out)+'\n',encoding='ascii')
print(f'Generated {len(vertices)} vertices, {len(faces)} triangles')
