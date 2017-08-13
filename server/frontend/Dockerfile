FROM node:boron

EXPOSE 8080

WORKDIR /web

COPY README.md .
COPY LICENSE .
COPY package.json .
COPY index.js .
COPY app ./app

RUN npm install

CMD ["node", "index.js"]

HEALTHCHECK --interval=5m --timeout=3s \
    CMD curl -f http://localhost:8080/status || exit 1
