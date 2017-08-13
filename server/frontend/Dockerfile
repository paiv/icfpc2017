FROM node:boron

EXPOSE 3000

WORKDIR /web

COPY README.md .
COPY LICENSE .
COPY package.json .
COPY index.js .
COPY app ./app

RUN npm install

CMD ["node", "index.js"]

HEALTHCHECK --interval=5m --timeout=3s \
    CMD curl -f http://localhost:3000/status || exit 1
