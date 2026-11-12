# Pagina web de Voxel World: HTML estatico servido por nginx sin root.
# El instalador (~110 MB) NO va en la imagen: se monta desde el servidor en
# /usr/share/nginx/html/dist (ver docker-compose.yml), asi la imagen pesa
# unos pocos MB y cada deploy no llena el disco.
FROM nginxinc/nginx-unprivileged:1.27-alpine

COPY deploy/web/nginx.conf /etc/nginx/conf.d/default.conf
COPY index.html adquirir.html estilo.css icono.png titulo.mp4 /usr/share/nginx/html/

EXPOSE 8080
HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
    CMD wget -q --spider http://127.0.0.1:8080/ || exit 1
