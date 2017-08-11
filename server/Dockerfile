FROM node:boron

EXPOSE 9000

WORKDIR /app

COPY README.md .
COPY package.json .
COPY puntd.js .
COPY punt ./punt
COPY maps ./maps

RUN npm install

CMD ["node", "puntd.js"]
