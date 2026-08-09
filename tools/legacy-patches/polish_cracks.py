from PIL import Image, ImageDraw, ImageFilter
import os

output_dir = r"D:\Respaldo\Voxel World\resourcepacks\Textures\Blocks\Animaciones"

print("=== Generando grietas sutiles y difuminadas ===")

def draw_smooth_crack(img, points, alpha=120):
    """Dibuja una grieta suave con efecto difuminado"""
    draw = ImageDraw.Draw(img)

    # Dibujar línea principal
    for i in range(len(points) - 1):
        draw.line([points[i], points[i+1]], fill=(0, 0, 0, alpha), width=1)

    # Añadir píxeles adyacentes para difuminado sutil
    for i in range(len(points) - 1):
        x1, y1 = points[i]
        x2, y2 = points[i+1]

        # Píxeles adyacentes muy sutiles para difuminado
        for x, y in [(x1, y1), (x2, y2)]:
            if 0 < x < 15 and 0 < y < 15:
                try:
                    # Píxeles con alpha más bajo para efecto difuminado
                    img.putpixel((x+1, y), (0, 0, 0, alpha//3))
                    img.putpixel((x-1, y), (0, 0, 0, alpha//3))
                    img.putpixel((x, y+1), (0, 0, 0, alpha//3))
                    img.putpixel((x, y-1), (0, 0, 0, alpha//3))
                except:
                    pass

def draw_crack_stage(stage):
    img = Image.new('RGBA', (16, 16), (0, 0, 0, 0))

    # Opacidades muy sutiles que van aumentando gradualmente
    base_alpha = 80 + (stage * 12)  # De 80 a 188

    if stage >= 0:
        # Etapa 0: Una pequeña grieta
        draw_smooth_crack(img, [(7, 6), (8, 7), (9, 8)], alpha=base_alpha)

    if stage >= 1:
        # Etapa 1: Extensión
        draw_smooth_crack(img, [(9, 8), (10, 9)], alpha=base_alpha)
        draw_smooth_crack(img, [(7, 6), (6, 5)], alpha=base_alpha)

    if stage >= 2:
        # Etapa 2: Ramificación suave
        draw_smooth_crack(img, [(8, 7), (7, 7), (6, 8)], alpha=base_alpha)
        draw_smooth_crack(img, [(8, 8), (9, 9), (10, 10)], alpha=base_alpha)

    if stage >= 3:
        # Etapa 3: Más ramificaciones
        draw_smooth_crack(img, [(6, 5), (5, 4), (4, 4)], alpha=base_alpha)
        draw_smooth_crack(img, [(10, 10), (11, 11), (12, 12)], alpha=base_alpha)

    if stage >= 4:
        # Etapa 4: Red expandida
        draw_smooth_crack(img, [(6, 8), (5, 9), (4, 10)], alpha=base_alpha)
        draw_smooth_crack(img, [(9, 9), (10, 8), (11, 8)], alpha=base_alpha)
        draw_smooth_crack(img, [(8, 7), (8, 5), (7, 4)], alpha=base_alpha)

    if stage >= 5:
        # Etapa 5: Extensión hacia bordes
        draw_smooth_crack(img, [(4, 4), (3, 3), (2, 2)], alpha=base_alpha)
        draw_smooth_crack(img, [(12, 12), (13, 13), (14, 14)], alpha=base_alpha)
        draw_smooth_crack(img, [(4, 10), (3, 11), (2, 12)], alpha=base_alpha)
        draw_smooth_crack(img, [(11, 8), (12, 7), (13, 6)], alpha=base_alpha)

    if stage >= 6:
        # Etapa 6: Más densidad
        draw_smooth_crack(img, [(7, 4), (6, 3), (5, 2)], alpha=base_alpha)
        draw_smooth_crack(img, [(8, 5), (9, 4), (10, 3)], alpha=base_alpha)
        draw_smooth_crack(img, [(5, 9), (4, 8), (3, 7)], alpha=base_alpha)
        draw_smooth_crack(img, [(10, 8), (11, 9), (12, 10)], alpha=base_alpha)

    if stage >= 7:
        # Etapa 7: Grietas adicionales
        draw_smooth_crack(img, [(2, 2), (1, 1)], alpha=base_alpha)
        draw_smooth_crack(img, [(14, 14), (15, 15)], alpha=base_alpha)
        draw_smooth_crack(img, [(2, 12), (1, 13), (1, 14)], alpha=base_alpha)
        draw_smooth_crack(img, [(13, 6), (14, 5), (15, 4)], alpha=base_alpha)
        draw_smooth_crack(img, [(5, 2), (4, 1), (3, 0)], alpha=base_alpha)

    if stage >= 8:
        # Etapa 8: Casi roto
        draw_smooth_crack(img, [(3, 7), (2, 7), (1, 6)], alpha=base_alpha)
        draw_smooth_crack(img, [(12, 10), (13, 10), (14, 11)], alpha=base_alpha)
        draw_smooth_crack(img, [(10, 3), (11, 2), (12, 1)], alpha=base_alpha)
        draw_smooth_crack(img, [(4, 8), (3, 9), (2, 10)], alpha=base_alpha)
        draw_smooth_crack(img, [(8, 8), (7, 9), (6, 10)], alpha=base_alpha)

    if stage >= 9:
        # Etapa 9: Completamente agrietado
        draw_smooth_crack(img, [(1, 6), (0, 5)], alpha=base_alpha)
        draw_smooth_crack(img, [(14, 11), (15, 12)], alpha=base_alpha)
        draw_smooth_crack(img, [(12, 1), (13, 0)], alpha=base_alpha)
        draw_smooth_crack(img, [(2, 10), (1, 11)], alpha=base_alpha)
        draw_smooth_crack(img, [(6, 10), (5, 11), (4, 12)], alpha=base_alpha)
        draw_smooth_crack(img, [(11, 2), (12, 3), (13, 4)], alpha=base_alpha)
        draw_smooth_crack(img, [(3, 0), (2, 0), (1, 0)], alpha=base_alpha)
        draw_smooth_crack(img, [(1, 14), (0, 15)], alpha=base_alpha)

    # Aplicar un ligero desenfoque para suavizar más
    img = img.filter(ImageFilter.SMOOTH)

    return img

for stage in range(10):
    img = draw_crack_stage(stage)
    output_path = os.path.join(output_dir, f"destroy_stage_{stage}.png")
    img.save(output_path, 'PNG')
    print(f"  [OK] Etapa {stage}: Grietas sutiles y continuas")

print("\n=== Grietas refinadas completadas ===")
