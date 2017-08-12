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
