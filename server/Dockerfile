FROM node:boron

EXPOSE 8080
EXPOSE 9000

WORKDIR /app

COPY README.md .
COPY LICENSE .
COPY package.json .
COPY puntd.js .
COPY punt ./punt
COPY maps ./maps

RUN npm install

CMD ["node", "puntd.js"]

HEALTHCHECK --interval=30s --timeout=3s \
    CMD curl -f http://localhost:8080/ || exit 1
