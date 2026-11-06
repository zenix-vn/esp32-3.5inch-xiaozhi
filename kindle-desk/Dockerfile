FROM mcr.microsoft.com/playwright/python:v1.49.0-jammy
WORKDIR /app
RUN pip install --no-cache-dir playwright==1.49.0 pillow tzdata
COPY server.py desk.html settings.html config.json cards.json ./
COPY fonts ./fonts
ENV PORT=8080 DESK_DATA=/app/data
EXPOSE 8080
CMD ["python", "server.py"]
