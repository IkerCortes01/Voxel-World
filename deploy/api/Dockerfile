# API de comentarios y correos de testers: Python sin dependencias.
FROM python:3.12-alpine

RUN adduser -D -H -u 10001 app && mkdir /datos && chown app /datos
COPY servidor.py /app/servidor.py
WORKDIR /app
USER app

EXPOSE 8000
HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
    CMD wget -q --spider http://127.0.0.1:8000/health || exit 1
CMD ["python", "-u", "servidor.py"]
